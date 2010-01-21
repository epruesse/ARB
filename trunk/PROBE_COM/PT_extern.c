#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <string.h>
#include "PT_server.h"
#include "C/server.h"
#include <aisc_server_proto.h>
#include <aisc_server_extern.h>
#include <import_proto.h>

// this source is compiled as C++ !!!

#ifdef __cplusplus
extern "C" {
#endif

    int pt_init_bond_matrix(PT_pdc *THIS);

#ifdef __cplusplus
}
#endif

int init_bond_matrix(PT_pdc *THIS) {
    pt_init_bond_matrix(THIS);
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

    void pt_destroy_locs(PT_local *THIS) {
        destroy_PT_local(THIS);
    }

#ifdef __cplusplus
}
#endif

int pt_init_socket(PT_local *THIS) {
    return aisc_add_destroy_callback((aisc_callback_func)pt_destroy_locs, (long)THIS);
}

void  pt_destroy_socket(PT_local *THIS) {
    THIS = THIS;
    aisc_remove_destroy_callback();
}

aisc_string get_LOCS_ERROR(PT_local *THIS) {
    static char *error_buf = 0;
    if (error_buf) free(error_buf);
    error_buf = THIS->ls_error;
    THIS->ls_error = (char *)strdup("");
    return error_buf;
}
