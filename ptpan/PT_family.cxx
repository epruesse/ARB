#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <PT_server.h>
#include <struct_man.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"
#include <arbdbt.h>

extern "C" int ff_find_family(PT_local *locs, bytestring *species) {
    printf("ff_find_family is not implemented in PTPan\n");
    arb_assert(false);
    return 0;
}

extern "C" int find_family(PT_local *locs, bytestring *species)
{
  //int probe_len = locs->pr_len;
  //int mismatch_nr = locs->mis_nr;

  printf("EXTERN: find_family\n");
  return 0;
}


