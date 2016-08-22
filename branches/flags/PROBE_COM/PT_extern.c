#include "PT_server.h"
#include "C/server.h"

#include <arb_string.h>

#include <aisc_server_proto.h>
#include <aisc_server_extern.h>
#include <import_proto.h>

int pt_init_bond_matrix(PT_local *THIS);

int init_bond_matrix(PT_local *THIS) {
    pt_init_bond_matrix(THIS);
    return 0;
}

void pt_destroy_locs(PT_local *THIS) {
    destroy_PT_local(THIS);
}

int pt_init_socket(PT_local *THIS) {
    aisc_add_destroy_callback((aisc_destroy_callback)pt_destroy_locs, (long)THIS);
    return 0;
}

void pt_destroy_socket(PT_local *) {
    aisc_remove_destroy_callback();
}

aisc_string get_LOCS_ERROR(PT_local *THIS) {
    static char *error_buf = 0;
    if (error_buf) free(error_buf);
    error_buf = THIS->ls_error;
    THIS->ls_error = ARB_strdup("");
    return error_buf;
}
