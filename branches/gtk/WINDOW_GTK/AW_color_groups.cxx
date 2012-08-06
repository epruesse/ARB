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

GB_ERROR AW_set_color_group(GBDATA */*gbd*/, long /*color_group*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}


long AW_find_color_group(GBDATA */*gbd*/, bool /*ignore_usage_flag*/ /*= false*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *AW_get_color_group_name(AW_root */*awr*/, int /*color_group*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_init_color_group_defaults(const char */*for_program*/) {
    GTK_NOT_IMPLEMENTED;
}

#if defined(UNIT_TESTS) 
void fake_AW_init_color_groups() {
}
#endif
