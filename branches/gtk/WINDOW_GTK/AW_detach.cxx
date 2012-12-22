// ============================================================= //
//                                                               //
//   File      : AW_detach.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_detach.hxx"
#include "aw_gtk_migration_helpers.hxx"

void Awar_Callback_Info::init (AW_root *awr_, const char *awar_name_, Awar_CB2 callback_, AW_CL cd1_, AW_CL cd2_) {
    awr           = awr_;
    callback      = callback_;
    cd1           = cd1_;
    cd2           = cd2_;
    awar_name     = strdup(awar_name_);
    org_awar_name = strdup(awar_name_);
}

void Awar_Callback_Info::remap(char const *new_awar) {
    if (strcmp(awar_name, new_awar) != 0) {
        remove_callback();
        freedup(awar_name, new_awar);
        add_callback();
    }
}
