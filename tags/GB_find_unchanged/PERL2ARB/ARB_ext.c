void GBP_callback(GBDATA *gbd, int *cl, GB_CB_TYPE cb_type){
    char *perl_func;
    char *perl_cl;
    dSP ;
    I32 i;
    SV *sv;
    perl_func = (char *)cl;
    perl_cl = perl_func + strlen(perl_func) + 1;

    PUSHMARK(sp);
    sv =  sv_newmortal();
    sv_setref_pv(sv, "GBDATAPtr", (void*)gbd);
    XPUSHs(sv);
    XPUSHs(sv_2mortal(newSVpv(perl_cl, 0)));
    if (cb_type & GB_CB_DELETE) {
	XPUSHs(sv_2mortal(newSVpv("DELETED", 0)));
    }else{
	XPUSHs(sv_2mortal(newSVpv("CHANGED", 0)));
    }

    PUTBACK;
    i = perl_call_pv(perl_func,G_DISCARD);
    if (i) {
	croak("Your perl function '%s' should not return any values",perl_func);
    }
    return; 
}


static GB_HASH *gbp_cp_hash_table = 0;

GB_ERROR GBP_add_callback(GBDATA *gbd, char *perl_func, char *perl_cl){
    char *data = 0;
    char *arg = 0;
    if (gbp_cp_hash_table ==0) gbp_cp_hash_table = GBS_create_hash(4096,0);
    data = (char *)calloc(1,strlen(perl_func) + strlen(perl_cl) + 100);
    arg = (char *)calloc(1,strlen(perl_func) + strlen(perl_cl) + 2);
    sprintf(arg,"%s%c%s",perl_func,'\0',perl_cl);
    sprintf(data,"%p:%s%c%s",gbd,perl_func,'\1',perl_cl);
    if (!GBS_read_hash(gbp_cp_hash_table,data)){
	GBS_write_hash(gbp_cp_hash_table,data,(long)data);
	GB_add_callback(gbd,GB_CB_DELETE|GB_CB_CHANGED,GBP_callback, (int *)arg);
    }else{
	free(arg);
    }
    free(data);
    return 0;
}

GB_ERROR GBP_remove_callback(GBDATA *gbd, char *perl_func, char *perl_cl){
    char *data = 0;
    char *arg = 0;
    if (gbp_cp_hash_table ==0) gbp_cp_hash_table = GBS_create_hash(4096,0);
    data = (char *)calloc(1,strlen(perl_func) + strlen(perl_cl) + 100);
    sprintf(data,"%p:%s%c%s",gbd,perl_func,'\1',perl_cl);
    arg =(char *)GBS_read_hash(gbp_cp_hash_table,data);
    if (!arg){
	free(data);
	return GB_export_error("Sorry: You never installed a callback '%s:%s'",perl_func,perl_cl);
    }else{
	GBS_write_hash(gbp_cp_hash_table,data,0);
	free(data);
	free(arg);
	return GB_remove_callback(gbd,GB_CB_DELETE|GB_CB_CHANGED,GBP_callback, (int *)data);
    }
    return 0;
}


char *static_pntr = 0;
