#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* aisc_extern.c */
dll_public *create_dll_public P_((void));
int move_dll_header P_((dll_header *sobj, dll_header *dobj));
int get_COMMON_CNT P_((dll_header *THIS));
dllheader_ext *get_COMMON_PARENT P_((dll_header *THIS));
dllheader_ext *get_COMMON_LAST P_((dll_header *THIS));
char *aisc_get_keystring P_((int *obj));
char *aisc_get_keystring_dll_header P_((dll_header *x));

#ifdef __cplusplus
}
#endif

#undef P_
