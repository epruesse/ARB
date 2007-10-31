#ifndef _ALI_GLOBAL_INC_
#define _ALI_GLOBAL_INC_

#include "ali_arbdb.hxx"
#include "ali_pt.hxx"
#include "ali_profile.hxx"
#include "ali_prealigner.hxx"


class ALI_GLOBAL {
private:
public:

    /* misc */
    char        *prog_name;
    char        *species_name;
    char        *default_file;
    char        *db_server;
    char        *pt_server;

    /* other classes */
    ALI_ARBDB       arbdb;
    ALI_PT          *pt;

    /* flags */
    int                  mark_species_flag;

    /* limits */
    float       cost_low;
    float       cost_middle;
    float       cost_high;

    /* Contexts */
    ALI_PT_CONTEXT       pt_context;
    ALI_PROFILE_CONTEXT prof_context;
    ALI_PREALIGNER_CONTEXT preali_context;

    /* functions */
    void init(int *argc, char *argv[]);
};

#endif
