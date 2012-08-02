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


//globals
//FIXME use static class or namespace for globals
AW_root *AW_root::SINGLETON = 0;


GB_ERROR ARB_bind_global_awars(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

void AW_system(AW_window *aww, const char *command, const char *auto_help_file) {
    GTK_NOT_IMPLEMENTED
}


char *aw_string_selection(const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    GTK_NOT_IMPLEMENTED
    return 0;
}
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

int aw_string_selection_button() {
    GTK_NOT_IMPLEMENTED
    return 0;
}

char *aw_input(const char *title, const char *prompt, const char *default_input) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

char *aw_input(const char *prompt, const char *default_input) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

void aw_initstatus() {
    GTK_NOT_IMPLEMENTED
}


void ARB_declare_global_awars(AW_root *aw_root, AW_default aw_def) {
    GTK_NOT_IMPLEMENTED
}



char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix) {
    GTK_NOT_IMPLEMENTED
    return 0;
}


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


AW_awar *AW_root::awar_string (const char *var_name, const char *default_value /*= ""*/, AW_default default_file /*= AW_ROOT_DEFAULT*/) {
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

void AW_root::set_focus_callback(AW_RCB fcb, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR AW_root::start_macro_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing) {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR AW_root::stop_macro_recording() {
    GTK_NOT_IMPLEMENTED
}

void AW_root::unlink_awars_from_DB(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED
}
AW_root::~AW_root() {
    GTK_NOT_IMPLEMENTED
}

AW_default AW_root::check_properties(AW_default aw_props) {
    GTK_NOT_IMPLEMENTED
}
