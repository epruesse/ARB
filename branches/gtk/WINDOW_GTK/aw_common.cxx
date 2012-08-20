// ============================================================= //
//                                                               //
//   File      : aw_common.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 20, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_common.hxx"

void AW_GC_set::add_gc(int gi, AW_GC *agc) {
    if (gi >= count) {
        int new_count = gi+10;
        gcs           = (AW_GC **)realloc((char *)gcs, sizeof(*gcs)*new_count);
        memset(&gcs[count], 0, sizeof(*gcs)*(new_count-count));
        count         = new_count;
    }
    if (gcs[gi]) delete gcs[gi];
    gcs[gi] = agc;
}



