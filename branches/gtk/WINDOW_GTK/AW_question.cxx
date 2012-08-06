// ============================================================= //
//                                                               //
//   File      : AW_question.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_question.hxx"
#include "aw_gtk_migration_helpers.hxx"



bool aw_ask_sure(const char *uniqueID, const char *msg) {
    GTK_NOT_IMPLEMENTED
    return false;
}

int aw_question(const char *uniqueID, const char *msg, const char *buttons, bool fixedSizeButtons /*= true*/, const char *helpfile /*= 0*/) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

void aw_popup_ok (const char *msg) {
    GTK_NOT_IMPLEMENTED
}

void aw_popup_exit(const char *msg) {
    GTK_NOT_IMPLEMENTED
}


void AW_repeated_question::add_help(const char *help_file) {
    GTK_NOT_IMPLEMENTED
}

int AW_repeated_question::get_answer(const char *uniqueID, const char *question, const char *buttons, const char *to_all, bool add_abort) {
    GTK_NOT_IMPLEMENTED
    return 0;
}
