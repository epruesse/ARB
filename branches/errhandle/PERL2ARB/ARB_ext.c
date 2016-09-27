/* ================================================================ */
/*                                                                  */
/*   File      : ARB_ext.c                                          */
/*   Purpose   : additional code for perllib                        */
/*                                                                  */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
/*                                                                  */
/* ================================================================ */

#include <ad_cb.h>

char *static_pntr                 = 0; // see ../PERLTOOLS/arb_proto_2_xsub.cxx@static_pntr

static GB_HASH *gbp_cp_hash_table = 0;

// defined in ../ARBDB/adperl.c@GBP_croak_function
extern void (*GBP_croak_function)(const char *message);

__ATTR__NORETURN static void GBP_croak(const char *message) {
    Perl_croak(aTHX_ "ARBDB croaks %s", message);
}

void GBP_callback(GBDATA *gbd, const char *perl_func, GB_CB_TYPE cb_type) {
    // perl_func contains 'func\0cl'
    dSP;

    const char *perl_cl = perl_func + strlen(perl_func) + 1;

    PUSHMARK(sp);
    SV *sv = sv_newmortal();
    sv_setref_pv(sv, "GBDATAPtr", (void*)gbd);
    XPUSHs(sv);
    XPUSHs(sv_2mortal(newSVpv(perl_cl, 0)));
    if (cb_type & GB_CB_DELETE) {
        XPUSHs(sv_2mortal(newSVpv("DELETED", 0)));
    }
    else {
        XPUSHs(sv_2mortal(newSVpv("CHANGED", 0)));
    }

    PUTBACK;
    I32 i = perl_call_pv(perl_func, G_DISCARD);
    if (i) {
        croak("Your perl function '%s' should not return any values", perl_func);
    }
    return;
}

inline char *gbp_create_callback_hashkey(GBDATA *gbd, const char *perl_func, const char *perl_cl) {
    return GBS_global_string_copy("%p:%s%c%s", gbd, perl_func, '\1', perl_cl);
}

GB_ERROR GBP_add_callback(GBDATA *gbd, const char *perl_func, const char *perl_cl) {
    if (!gbp_cp_hash_table) gbp_cp_hash_table = GBS_create_hash(20, GB_MIND_CASE);

    char     *data  = gbp_create_callback_hashkey(gbd, perl_func, perl_cl);
    GB_ERROR  error = 0;

    if (GBS_read_hash(gbp_cp_hash_table, data)) {
        error = GBS_global_string("Error: Callback '%s:%s' is already installed", perl_func, perl_cl);
    }
    else {
        char *arg = GBS_global_string_copy("%s%c%s", perl_func, '\0', perl_cl);

        GBS_write_hash(gbp_cp_hash_table, data, (long)arg);
        error = GB_add_callback(gbd, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(GBP_callback, arg));

        GBS_optimize_hash(gbp_cp_hash_table);
    }
    free(data);

    return error;
}

GB_ERROR GBP_remove_callback(GBDATA *gbd, const char *perl_func, const char *perl_cl) {
    GB_ERROR  error = 0;
    char     *data  = gbp_create_callback_hashkey(gbd, perl_func, perl_cl);
    char     *arg   = gbp_cp_hash_table ? (char *)GBS_read_hash(gbp_cp_hash_table, data) : (char*)NULL;

    if (!arg) {
        error = GBS_global_string("Error: You never installed a callback '%s:%s'", perl_func, perl_cl);
    }
    else {
        GBS_write_hash(gbp_cp_hash_table, data, 0);
        GB_remove_callback(gbd, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(GBP_callback, arg));
        free(arg);
    }
    free(data);

    return error;
}


struct ARB_init_perl_interface {
    ARB_init_perl_interface() {
        GBP_croak_function = GBP_croak;
    }
};

static ARB_init_perl_interface init; /* automatically initialize this module */


