#include <stdio.h>
#include <stdlib.h>
#include <aisc.h>
#include <aisc_com.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
    // #include <malloc.h>
#include <memory.h>
    int aisc_make_sets(long *obj);  /* sets in a create request */
    int aisc_talking_get_index (int u, int o);

#include <aisc_server_proto.h>
#include <aisc_server_extern.h>
#include <import_proto.h>

#include <struct_man.h>

#ifdef __cplusplus
}
#endif

/************** SAVE and LOAD **************/

#ifdef AISC_SAVE_YES
#include <math.h>
#endif                  /* for atof */
