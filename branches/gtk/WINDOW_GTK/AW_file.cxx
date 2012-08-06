// ============================================================= //
//                                                               //
//   File      : AW_file.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_file.hxx"
#include "aw_gtk_migration_helpers.hxx"

char *AW_unfold_path(const char */*pwd_envar*/, const char */*path*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *AW_extract_directory(const char */*path*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

// -----------------------------
//      file selection boxes

void AW_create_fileselection_awars(AW_root    */*awr*/, const char */*awar_base*/,
                                   const char */*directory*/, const char */*filter*/, const char */*file_name*/,
                                   AW_default  /*default_file*/ /*= AW_ROOT_DEFAULT*/, bool /*resetValues = false*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_create_fileselection(AW_window */*aws*/, const char */*awar_prefix*/, const char */*at_prefix*/ /* = ""*/, const char */*pwd*/ /*= "PWD"*/, bool /*show_dir*/ /*= true*/, bool /*allow_wildcards*/ /* = false*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_refresh_fileselection(AW_root */*awr*/, const char */*awar_prefix*/) {
    GTK_NOT_IMPLEMENTED;
}


char *AW_get_selected_fullname(AW_root */*awr*/, const char */*awar_prefix*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_set_selected_fullname(AW_root */*awr*/, const char */*awar_prefix*/, const char */*to_fullname*/) {
    GTK_NOT_IMPLEMENTED;
}




