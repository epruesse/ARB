#ifndef awtc_import_hxx_included
#define awtc_import_hxx_included

#define AWAR_READ_GENOM_DB "tmp/import/genom_db"

typedef enum { IMP_GENOME_FLATFILE, IMP_PLAIN_SEQUENCE } AWTI_ImportType;

#define AWTC_RCB(func) void (*func)(AW_root*,AW_CL,AW_CL)

GBDATA *open_AWTC_import_window(AW_root *awr,const char *defname, int do_exit,AWTC_RCB(func), AW_CL cd1, AW_CL cd2);
void    AWTC_import_set_ali_and_type(AW_root *awr, const char *ali_name, const char *ali_type, GBDATA *gbmain);

#endif
