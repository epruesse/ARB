#ifndef awtc_import_hxx_included
#define awtc_import_hxx_included

#define AWTC_RCB(func) void (*func)(AW_root*,AW_CL,AW_CL)

GBDATA *open_AWTC_import_window(AW_root *awr,const char *defname, int do_exit,AWTC_RCB(func), AW_CL cd1, AW_CL cd2);

#endif