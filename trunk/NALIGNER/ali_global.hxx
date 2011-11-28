// =============================================================== //
//                                                                 //
//   File      : ali_global.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_GLOBAL_HXX
#define ALI_GLOBAL_HXX

#ifndef ALI_PREALIGNER_HXX
#include "ali_prealigner.hxx"
#endif

class ALI_GLOBAL {
private:
public:

    // misc
    const char *prog_name;
    char       *species_name;
    char       *default_file;
    char       *db_server;
    char       *pt_server;

    // other classes
    ALI_ARBDB       arbdb;
    ALI_PT          *pt;

    // flags
    int                  mark_species_flag;

    // limits
    float       cost_low;
    float       cost_middle;
    float       cost_high;

    // Contexts
    ALI_PT_CONTEXT       pt_context;
    ALI_PROFILE_CONTEXT prof_context;
    ALI_PREALIGNER_CONTEXT preali_context;

    // functions
    void init(int *argc, const char *argv[]);
};

#else
#error ali_global.hxx included twice
#endif // ALI_GLOBAL_HXX
