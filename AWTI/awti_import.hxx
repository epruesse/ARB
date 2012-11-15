// =========================================================== //
//                                                             //
//   File      : awti_import.hxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWTI_IMPORT_HXX
#define AWTI_IMPORT_HXX


#define AWAR_READ_GENOM_DB "tmp/import/genom_db"

typedef enum { IMP_GENOME_FLATFILE, IMP_PLAIN_SEQUENCE } AWTI_ImportType;

#define AWTC_RCB(func) void (*func)(AW_root*,AW_CL,AW_CL)

GBDATA *open_AWTC_import_window(AW_root *awr,const char *defname, bool do_exit, GBDATA *gb_main, AWTC_RCB(func), AW_CL cd1, AW_CL cd2);
void    AWTC_import_set_ali_and_type(AW_root *awr, const char *ali_name, const char *ali_type, GBDATA *gbmain);


#else
#error awti_import.hxx included twice
#endif // AWTI_IMPORT_HXX
