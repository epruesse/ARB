#include <cstdio>
#include <cstdlib>
#include "names_server.h"

struct sigcontext;
#include "C/server.h"

#include <aisc_server_proto.h>
#include <aisc_server_extern.h>
#include <import_proto.h>

extern AN_main *aisc_main;
int names_server_save();

int names_destroy_locs(AN_local *THIS) {
    // called when client closes connection
    destroy_AN_local(THIS);
    if (aisc_main->ploc_st.cnt <= 0) { // last client disconnected
        names_server_save();
    }
    return 0;
}

int names_init_socket(AN_local *THIS) {
    aisc_add_destroy_callback((aisc_destroy_callback)names_destroy_locs, (long)THIS);
    return 0;
}
void names_destroy_socket(AN_local *) {
    aisc_remove_destroy_callback();
}
