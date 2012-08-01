// ============================================================= //
//                                                               //
//   File      : AW_root.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_gtk_migration_helpers.hxx"
#include "aw_root.hxx"


AW_root *AW_root::SINGLETON = 0;


AW_root::AW_root(const char *properties, const char *program, bool no_exit) {
    GTK_NOT_IMPLEMENTED
}

void AW_root::add_timed_callback(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

void AW_root::add_timed_callback_never_disabled(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

AW_awar *AW_root::awar(const char *awar) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

AW_awar *AW_root::awar_float(const char *var_name, float default_value/* = 0.0*/, AW_default default_file/* = AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

AW_awar *AW_root::awar_int(const char *var_name, long default_value /*= 0*/, AW_default default_file /*= AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

AW_awar *AW_root::awar_no_error(const char *awar) {
    GTK_NOT_IMPLEMENTED
}

AW_awar *AW_root::awar_pointer(const char *var_name, void *default_value/* = NULL*/,     AW_default default_file/* = AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR AW_root::check_for_remote_command(AW_default gb_main, const char *rm_base) {
    GTK_NOT_IMPLEMENTED
}

void AW_root::dont_save_awars_with_default_value(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR AW_root::execute_macro(GBDATA *gb_main, const char *file, AW_RCB1 execution_done_cb, AW_CL client_data) {
    GTK_NOT_IMPLEMENTED
}

bool AW_root::is_recording_macro() const {
    GTK_NOT_IMPLEMENTED
}

void AW_root::main_loop() {
    GTK_NOT_IMPLEMENTED
}

void set_focus_callback(AW_RCB fcb, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR start_macro_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing) {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR AW_root::stop_macro_recording() {
    GTK_NOT_IMPLEMENTED
}

void unlink_awars_from_DB(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED
}
AW_root::~AW_root() {
    GTK_NOT_IMPLEMENTED
}

AW_default AW_root::check_properties(AW_default aw_props) {
    GTK_NOT_IMPLEMENTED
}

