#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdio.h>
#include <stdlib.h>
#include "ors_server.h"
#include "C/server.h"
#include <aisc_server_proto.h>
#include <aisc_server_extern.h>
#include <import_proto.h>

#ifdef __cplusplus
}
#endif

extern ORS_main *aisc_main;
int ors_server_shutdown(void);

#ifdef __cplusplus
extern "C" {
#endif
    
    //    aisc_callback_func_proto(destroy_ORS_local); /* prototyp */

    void ors_destroy_locs(ORS_local	*THIS) {
        destroy_ORS_local(THIS);
    }

#ifdef __cplusplus
}
#endif

int ors_init_socket(ORS_local	*THIS) {
    aisc_add_destroy_callback((aisc_callback_func)ors_destroy_locs,(long)THIS);
    return 0;
}

void ors_destroy_socket(ORS_local	*THIS) {
    THIS = THIS;
#if 0
    if (aisc_main->ploc_st.cnt <=0) {
	names_server_shutdown();
	exit(0);
    }
#endif
    aisc_remove_destroy_callback();
}
