#ifndef awtc_import_hxx_included
#define awtc_import_hxx_included

#define AWAR_READ_GENOM_DB "tmp/import/genom_db"

#if defined(DEVEL_ARTEM)
typedef enum { IMP_GENOME_FLATFILE, IMP_PLAIN_SEQUENCE } AWTI_ImportType;
#else
typedef enum { IMP_GENOME_GENEBANK, IMP_GENOME_EMBL, IMP_PLAIN_SEQUENCE } AWTI_ImportType;
#endif // DEVEL_ARTEM


#define AWTC_RCB(func) void (*func)(AW_root*,AW_CL,AW_CL)

GBDATA *open_AWTC_import_window(AW_root *awr,const char *defname, int do_exit,AWTC_RCB(func), AW_CL cd1, AW_CL cd2);
void    AWTC_import_set_ali_and_type(AW_root *awr, const char *ali_name, const char *ali_type, GBDATA *gbmain);

#endif
