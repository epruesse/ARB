#include <stdio.h>
#include <string.h>
#include <aisc.h>
#include "aisc_extern_privat.h"

extern int aisc_d_flags[];

#ifdef __cplusplus
extern "C" {
#endif
    
    const char *aisc_get_object_names(int i);
    
#ifdef __cplusplus
}
#endif

dll_public *create_dll_public()
{
    return 0;
}

int move_dll_header(dll_header *sobj,dll_header *dobj){
    dobj->ident = (char *)strdup(sobj->ident);
    return 0;
}

int get_COMMON_CNT(dll_header *THIS){
    int key = (int)(THIS->key) >> 16;
    if (aisc_d_flags[key] == 0) return -1;
    if(!((THIS->parent))){return 0; }
    return THIS->parent->cnt;
}

dllheader_ext *get_COMMON_PARENT(dll_header *THIS){
    int key = (int)(THIS->key) >> 16;
    if (aisc_d_flags[key] == 0) return 0;
    if(!((THIS->parent))){return 0; }
    return (dllheader_ext *)THIS->parent->parent;
}

dllheader_ext *get_COMMON_LAST(dll_header *THIS){
    int key = (int)(THIS->key) >> 16;
    if (aisc_d_flags[key] == 0) return 0;
    if(!((THIS->parent))){return 0; }
    return (dllheader_ext *)THIS->parent->last;
}

char *aisc_get_keystring(int *obj) {
    int i;
    i = *obj>>16;
    return (char*)aisc_get_object_names(i);
}

char *aisc_get_keystring_dll_header(dll_header *x) { return aisc_get_keystring((int*)x); }
/*  char *aisc_get_keystring_AN_revers(AN_revers *x) { return aisc_get_keystring((int*)x); } */
/*  char *aisc_get_keystring_AN_shorts(AN_shorts *x) { return aisc_get_keystring((int*)x); } */
/*  char *aisc_get_keystring_AN_local(AN_local *x) { return aisc_get_keystring((int*)x); } */
