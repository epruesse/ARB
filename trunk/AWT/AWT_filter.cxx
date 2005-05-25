#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

#include "awt.hxx"
#include "awtfilter.hxx"
#include <awt_tree.hxx>
#include <awt_sel_boxes.hxx>



/** recalc filter */
void awt_create_select_filter_window_aw_cb(void *dummy, struct adfiltercbstruct *cbs)
{       // update the variables
    AW_root *aw_root = cbs->awr;
    AWUSE(dummy);
    char    buffer[256];
    GB_push_transaction(cbs->gb_main);
    char *target = aw_root->awar(cbs->def_subname)->read_string();
    char *to_free_target = target;
    char *use = aw_root->awar(cbs->def_alignment)->read_string();
    char *name = strchr(target,1);
    GBDATA *gbd = 0;
    if (name){
        *(name++) = 0;
        target++;
        GBDATA *gb_species;
        if (target[-1] == '@'){
            gb_species = GBT_find_species(cbs->gb_main,name);
        }else{
            gb_species = GBT_find_SAI(cbs->gb_main,name);
        }
        if (gb_species){
            GBDATA *gb_ali = GB_search(gb_species,use,GB_FIND);
            if (gb_ali) {
                gbd = GB_search(gb_ali,target,GB_FIND);
            }
        }
    }
    if (!gbd){      // nothing selected
        aw_root->awar(cbs->def_name)->write_string("none");
        aw_root->awar(cbs->def_source)->write_string("No Filter Sequence ->All Columns Selected");
        aw_root->awar(cbs->def_filter)->write_string("");
        aw_root->awar(cbs->def_len)   ->write_int(-1);  // export filter
    }else{
        GBDATA *gb_name = GB_get_father(gbd);   // ali_xxxx
        gb_name = GB_find(gb_name,"name",0,this_level);
        char *name2 = GB_read_string(gb_name);
        aw_root->awar(cbs->def_name)->write_string(name2);
        free(name2);
        char *_2filter = aw_root->awar(cbs->def_2filter)->read_string();
        long _2filter_len = strlen(_2filter);

        char *s,*str;
        long len = GBT_get_alignment_len(cbs->gb_main,use);
        void *strstruct = GBS_stropen(5000);
        long i; for (i=0;i<len;i++) {   // build position line
            if (i%10 == 0) {
                GBS_chrcat(strstruct,'#');
            }else if (i%5==0) {
                GBS_chrcat(strstruct,'|');
            }else{
                GBS_chrcat(strstruct,'.');
            }
        }
        GBS_chrcat(strstruct,'\n');
        char *data = GBS_mempntr(strstruct);

        for (i=0;i<len-10;i++) {    // place markers
            if (i%10 == 0) {
                sprintf(buffer,"%li",i+1);
                strncpy(data+i+1,buffer,strlen(buffer));
            }
        }

        if (GB_read_type(gbd) == GB_STRING) {   // read the filter
            str = GB_read_string(gbd);
        }else{
            str = GB_read_bits(gbd,'-','+');
        }
        GBS_strcat(strstruct,str);
        GBS_chrcat(strstruct,'\n');
        char *canc = aw_root->awar(cbs->def_cancel)->read_string();
        long min = aw_root->awar(cbs->def_min)->read_int()-1;
        long max = aw_root->awar(cbs->def_max)->read_int()-1;
        long flen = 0;
        for (i=0,s=str; *s; ++s,++i){       // transform the filter
            if (strchr(canc,*s) || (i<min) || (max>0 && i > max) )
            {
                *s = '0';
            }else{
                if (i > _2filter_len || _2filter[i] != '0') {
                    *s = '1';
                    flen++;
                }else{
                    *s = '0';
                }
            }
        }
        GBS_strcat(strstruct,str);
        GBS_chrcat(strstruct,'\n');
        data = GBS_strclose(strstruct);
        aw_root->awar(cbs->def_len)   ->write_int(flen);    // export filter
        aw_root->awar(cbs->def_filter)->write_string(str);  // export filter
        aw_root->awar(cbs->def_source)->write_string(data); // set display
        free(_2filter);
        free(str);
        free(canc);
        free(data);
    }
    free(to_free_target);
    free(use);
    GB_pop_transaction(cbs->gb_main);
}

static void awt_add_sequences_to_list(struct adfiltercbstruct *cbs, const char *use, GBDATA *gb_extended,const char *pre,char tpre){
    GBDATA *gb_name;
    GBDATA *gb_ali;
    GBDATA *gb_data;
    int count;

    gb_ali = GB_find(gb_extended,use,0,down_level);
    if (!gb_ali) return;
    count = 0;
    GBDATA *gb_type = GB_find(gb_ali,"_TYPE",0,down_level);
    char *TYPE = strdup("");
    if (gb_type) TYPE = GB_read_string(gb_type);

    gb_name = GB_find(gb_extended,"name",0,down_level);
    if (!gb_name) return;
    char *name = GB_read_string(gb_name);

    for (   gb_data = GB_find(gb_ali,0,0,down_level);
            gb_data;
            gb_data = GB_find(gb_data,0,0,search_next|this_level)){

        if (GB_read_key_pntr(gb_data)[0] == '_') continue;
        long type = GB_read_type(gb_data);
        if (type == GB_BITS || type == GB_STRING) {
            char *str;
            if (count){
                str  = GBS_global_string_copy("%s%-20s SEQ_%i   %s",    pre,name,count+1,TYPE);
            }else{
                str = GBS_global_string_copy("%s%-20s     %s",  pre,name,TYPE);
            }
            char *target = (char *)GBS_global_string("%c%s%c%s",tpre,GB_read_key_pntr(gb_data),1,name);
            cbs->aws->insert_selection( cbs->id,(char *)str, target );
            free(str);
            count++;
        }
    }
    free(TYPE);
    free(name);
}

void awt_create_select_filter_window_gb_cb(void *dummy,struct adfiltercbstruct *cbs){           // update list widget and variables
    AWUSE(dummy);
    AW_root *aw_root = cbs->awr;
    GB_push_transaction(cbs->gb_main);
    char *use = aw_root->awar(cbs->def_alignment)->read_string();
    GBDATA *gb_extended;

    if (cbs->id) {

        cbs->aws->clear_selection_list(cbs->id);
        cbs->aws->insert_default_selection( cbs->id, "none", "" );
        GBDATA *gb_sel = GB_search(cbs->gb_main,AWAR_SPECIES_NAME,GB_STRING);
        char *name = GB_read_string(gb_sel);
        if (strlen(name)){
            GBDATA *gb_species = GBT_find_species(cbs->gb_main,name);
            if (gb_species){
                awt_add_sequences_to_list(cbs,use,gb_species,"SEL. SPECIES:",'@');
            }
        }
        free(name);
        for (   gb_extended = GBT_first_SAI(cbs->gb_main);
                gb_extended;
                gb_extended = GBT_next_SAI(gb_extended)){
            awt_add_sequences_to_list(cbs,use,gb_extended,"",' ');
        }

        cbs->aws->update_selection_list( cbs->id );
    }
    awt_create_select_filter_window_aw_cb(0,cbs);
    GB_pop_transaction(cbs->gb_main);
    free(use);
}


AW_CL   awt_create_select_filter(AW_root *aw_root,GBDATA *gb_main,const char *def_name)
{
    struct adfiltercbstruct *acbs = new adfiltercbstruct ;
    acbs->gb_main = gb_main;
    GB_push_transaction(acbs->gb_main);
    AW_default aw_def = aw_root->get_default(def_name);

    acbs->def_name = GBS_string_eval(def_name,"/name=/name",0);
    acbs->def_filter = GBS_string_eval(def_name,"/name=/filter",0);
    acbs->def_alignment = GBS_string_eval(def_name,"/name=/alignment",0);

    acbs->def_min = GBS_string_eval(def_name,"*/name=tmp/*1/min:tmp/tmp=tmp",0);
    aw_root->awar_int( acbs->def_min)   ->add_callback((AW_RCB1)awt_create_select_filter_window_aw_cb,(AW_CL)acbs);
    acbs->def_max = GBS_string_eval(def_name,"*/name=tmp/*1/max:tmp/tmp=tmp",0);
    aw_root->awar_int( acbs->def_max)   ->add_callback((AW_RCB1)awt_create_select_filter_window_aw_cb,(AW_CL)acbs);

    acbs->def_len = GBS_string_eval(def_name,"*/name=tmp/*1/len:tmp/tmp=tmp",0);
    aw_root->awar_int( acbs->def_len);

    acbs->def_dest = GBS_string_eval(def_name,"*/name=tmp/*1/dest:tmp/tmp=tmp",0);
    aw_root->awar_string( acbs->def_dest,"",aw_def);

    acbs->def_cancel = GBS_string_eval(def_name,"*/name=*1/cancel",0);
    aw_root->awar_string( acbs->def_cancel,".0-=",aw_def);

    acbs->def_simplify = GBS_string_eval(def_name,"*/name=*1/simplify",0);
    aw_root->awar_int( acbs->def_simplify,0,aw_def);

    acbs->def_subname = GBS_string_eval(def_name,"*/name=tmp/*1/subname:tmp/tmp=tmp",0);
    aw_root->awar_string(   acbs->def_subname);

    acbs->def_source = GBS_string_eval(def_name,"*/name=tmp/*/source:tmp/tmp=tmp",0);
    aw_root->awar_string( acbs->def_source);

    acbs->def_2name = GBS_string_eval(def_name,"*/name=tmp/*/2filter/name:tmp/tmp=tmp",0);
    acbs->def_2filter = GBS_string_eval(def_name,"*/name=tmp/*/2filter/filter:tmp/tmp=tmp",0);
    acbs->def_2alignment = GBS_string_eval(def_name,"*/name=tmp/*/2filter/alignment:tmp/tmp=tmp",0);

    aw_root->awar_string( acbs->def_2name ) ->write_string( "- none -" );
    aw_root->awar_string( acbs->def_2filter );
    aw_root->awar_string( acbs->def_2alignment );

    acbs->id = 0;
    acbs->aws = 0;
    acbs->awr = aw_root;
    {
        char *fname = aw_root->awar(acbs->def_name)->read_string();
        const char *fsname = GBS_global_string(" data%c%s",1,fname);
        free(fname);
        aw_root->awar(acbs->def_subname)->write_string(fsname);     // cause an callback
    }

    aw_root->awar(acbs->def_subname)->touch();      // cause an callback

    GBDATA *gb_extended_data =  GB_search(acbs->gb_main,"extended_data",GB_CREATE_CONTAINER);

    GB_add_callback(gb_extended_data,GB_CB_CHANGED,     (GB_CB)awt_create_select_filter_window_gb_cb, (int *)acbs);

    GBDATA *gb_sel = GB_search(acbs->gb_main,AWAR_SPECIES_NAME,GB_STRING);

    GB_add_callback(gb_sel,GB_CB_CHANGED,           (GB_CB)awt_create_select_filter_window_gb_cb, (int *)acbs);

    aw_root->awar(acbs->def_alignment)->add_callback(   (AW_RCB1)awt_create_select_filter_window_gb_cb,(AW_CL)acbs);
    aw_root->awar(acbs->def_2filter)->add_callback(     (AW_RCB1)awt_create_select_filter_window_aw_cb,(AW_CL)acbs);
    aw_root->awar(acbs->def_subname)->add_callback(     (AW_RCB1)awt_create_select_filter_window_aw_cb,(AW_CL)acbs);

    awt_create_select_filter_window_gb_cb(0,acbs);

    GB_pop_transaction(acbs->gb_main);
    return (AW_CL)acbs;
}


void awt_set_awar_to_valid_filter_good_for_tree_methods(GBDATA *gb_main, AW_root *awr, const char *awar_name){
    GB_transaction transaction_var(gb_main);
    if (GBT_find_SAI(gb_main,"POS_VAR_BY_PARSIMONY")){
        awr->awar(awar_name)->write_string("POS_VAR_BY_PARSIMONY");
        return;
    }
    if (GBT_find_SAI(gb_main,"ECOLI")){
        awr->awar(awar_name)->write_string("ECOLI");
        return;
    }
}

AW_window *awt_create_2_filter_window(AW_root *aw_root,AW_CL res_of_create_select_filter)
{
    struct adfiltercbstruct *acbs = (struct adfiltercbstruct *) res_of_create_select_filter;
    GB_push_transaction(acbs->gb_main);
    aw_root->awar(acbs->def_2alignment)->map( acbs->def_alignment);
    AW_CL s2filter = awt_create_select_filter(aw_root, acbs->gb_main,acbs->def_2name);
    GB_pop_transaction(acbs->gb_main);
    return awt_create_select_filter_win(aw_root,s2filter);
}

char *AWT_get_combined_filter_name(AW_root *aw_root, GB_CSTR prefix) {
    char       *combined_name = aw_root->awar(GBS_global_string("%s/filter/name", prefix))->read_string(); // "gde/filter/name"
    const char *awar_prefix   = "tmp/gde/filter";
    const char *awar_repeated = "/2filter";
    const char *awar_postfix  = "/name";
    int         prefix_len    = strlen(awar_prefix);
    int         repeated_len  = strlen(awar_repeated);
    int         postfix_len   = strlen(awar_postfix);
    int         count;

    for (count = 1; ; ++count) {
        char *awar_name = new char[prefix_len + count*repeated_len + postfix_len + 1];
        strcpy(awar_name, awar_prefix);
        int c;
        for (c=0; c<count; ++c) strcat(awar_name, awar_repeated);
        strcat(awar_name, awar_postfix);

        AW_awar *awar_found = aw_root->awar_no_error(awar_name);
        delete [] awar_name;

        if (!awar_found) break; // no more filters defined
        char *content = awar_found->read_string();

        if (strstr(content, "none")==0) { // don't add filters named 'none'
            char *new_combined_name = (char*)malloc(strlen(combined_name)+1+strlen(content)+1);
            sprintf(new_combined_name, "%s/%s", combined_name, content);
            free(combined_name);
            combined_name = new_combined_name;
        }
    }

    return combined_name;
}

AW_window *awt_create_select_filter_win(AW_root *aw_root,AW_CL res_of_create_select_filter)
{
    struct adfiltercbstruct *acbs = (struct adfiltercbstruct *) res_of_create_select_filter;
    GB_push_transaction(acbs->gb_main);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "FILTER_SELECT", "Select Filter");
    aws->load_xfig("awt/filter.fig");
    aws->button_length( 10 );

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"sel_fil.hlp");
    aws->create_button("HELP", "HELP","H");

    acbs->aws = (AW_window *)aws;
    aws->at("filter");
    acbs->id = aws->create_selection_list(acbs->def_subname,0,"",20,3);

    aws->at("2filter");
    aws->callback(AW_POPUP,(AW_CL)awt_create_2_filter_window,(AW_CL)acbs);
    aws->create_button(acbs->def_2name,acbs->def_2name);

    aws->at("zero");
    aws->callback((AW_CB1)awt_create_select_filter_window_aw_cb,(AW_CL)acbs);
    aws->create_input_field(acbs->def_cancel,10);

    aws->at("sequence");
    aws->create_text_field(acbs->def_source,1,1);

    aws->at("min");
    aws->create_input_field(acbs->def_min,4);

    aws->at("max");
    aws->create_input_field(acbs->def_max,4);

    aws->at("simplify");
    aws->create_option_menu(acbs->def_simplify);
    aws->insert_option("ORIGINAL DATA","O",0);
    aws->insert_option("TRANSVERSIONS ONLY","T",1);
    aws->insert_option("SIMPLIFIED AA","A",2);
    aws->update_option_menu();

    awt_create_select_filter_window_gb_cb(0,acbs);

    aws->button_length( 7 );
    aws->at("len");    aws->create_button(0,acbs->def_len);

    GB_pop_transaction(acbs->gb_main);
    return (AW_window *)aws;
}

AP_filter *awt_get_filter(AW_root *aw_root,AW_CL res_of_create_select_filter)
{
    struct adfiltercbstruct *acbs = (struct adfiltercbstruct *) res_of_create_select_filter;
    AP_filter *filter = new AP_filter;
    if (!acbs) {
        filter->init("","0",10);
        return filter;
    }
    GB_push_transaction(acbs->gb_main);

    char *filter_string = aw_root->awar(acbs->def_filter)->read_string();
    long len = 0;
    char *use = aw_root->awar(acbs->def_alignment)->read_string();
    len = GBT_get_alignment_len(acbs->gb_main,use);
    free(use);
    filter->init(filter_string,"0",len);

    int sim = aw_root->awar(acbs->def_simplify)->read_int();
    filter->enable_simplify((AWT_FILTER_SIMPLIFY)sim);
    free(filter_string);

    GB_pop_transaction(acbs->gb_main);
    return filter;
}

