// ============================================================= //
//                                                               //
//   File      : AW_color_groups.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_color_groups.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_assert.hxx"
#ifndef ARBDB_H
#include <arbdb.h>
#endif





GB_ERROR AW_set_color_group(GBDATA */*gbd*/, long /*color_group*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}




char *AW_get_color_group_name(AW_root */*awr*/, int /*color_group*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}



#if defined(UNIT_TESTS) 
void fake_AW_init_color_groups() {
    GTK_NOT_IMPLEMENTED;
}
#endif
