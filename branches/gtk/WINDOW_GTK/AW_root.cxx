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
#include <gtk/gtk.h>
#include <arbdb.h>

//globals
//FIXME use static class or namespace for globals
AW_root *AW_root::SINGLETON = 0;


GB_ERROR ARB_bind_global_awars(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void aw_set_local_message() {
    GTK_NOT_IMPLEMENTED;
}


void AW_system(AW_window *aww, const char *command, const char *auto_help_file) {
    GTK_NOT_IMPLEMENTED;
}


char *aw_string_selection(const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

int aw_string_selection_button() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *aw_input(const char *title, const char *prompt, const char *default_input) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *aw_input(const char *prompt, const char *default_input) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void ARB_declare_global_awars(AW_root *aw_root, AW_default aw_def) {
    GTK_NOT_IMPLEMENTED;
}



char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}


AW_ProcessEventType AW_root::peek_key_event(AW_window *) {
    GTK_NOT_IMPLEMENTED;
    return (AW_ProcessEventType)0;
}

AW_default AW_root::load_properties(const char *default_name) {

    GBDATA   *gb_default = GB_open(default_name, "rwcD");
    GB_ERROR  error      = NULL;

    if (gb_default) {
        GB_no_transaction(gb_default);

        GBDATA *gb_tmp = GB_search(gb_default, "tmp", GB_CREATE_CONTAINER);
        error          = GB_set_temporary(gb_tmp);
    }
    else {
        error = GB_await_error();
    }

    if (error) {
        const char *shown_name      = strrchr(default_name, '/');
        if (!shown_name) shown_name = default_name;

        GBK_terminatef("Error loading properties '%s': %s", shown_name, error);
    }

    return (AW_default) gb_default;
}

static void destroy_AW_root() {
    delete AW_root::SINGLETON; AW_root::SINGLETON = NULL;
}

AW_root::AW_root(const char *properties, const char *program, bool no_exit) {
    aw_assert(!AW_root::SINGLETON);                 // only one instance allowed
    AW_root::SINGLETON = this;
    printf("props: %s", properties);

    memset((char *)this, 0, sizeof(AW_root));//initialize all attributes with 0

    init_variables(load_properties(properties));

    init_root(program, no_exit);

    atexit(destroy_AW_root); // do not call this before opening properties DB!
}

void AW_root::init_variables(AW_default database) {
    application_database     = database;
    hash_table_for_variables = GBS_create_hash(1000, GB_MIND_CASE);
    hash_for_windows         = GBS_create_hash(100, GB_MIND_CASE);

    //FIXME font stuff
//    for (int i=0; aw_fb[i].awar; ++i) {
//        awar_string(aw_fb[i].awar, aw_fb[i].init, application_database);
//    }
}

void AW_root::init_root(const char *programname, bool no_exit) {
    // initialize ARB gtk application
    int          a = 0;
    //FIXME font stuff
//    XFontStruct *fontstruct;
//    char        *fallback_resources[100];

    action_hash = GBS_create_hash(1000, GB_MIND_CASE);
    this->no_exit = no_exit;
    program_name  = strdup(programname);

    //simulate minimal argc and argv.
    //FIXME gain access to the real argc and argv

    char *argv[] = { program_name };
    int argc = 0;
    gtk_init(&argc, NULL);


    //FIXME font stuff
//    int i;
//    for (i=0; i<1000 && aw_fb[i].fb; i++) {
//        GBDATA *gb_awar       = GB_search((GBDATA*)application_database, aw_fb[i].awar, GB_FIND);
//        fallback_resources[i] = GBS_global_string_copy("*%s: %s", aw_fb[i].fb, GB_read_char_pntr(gb_awar));
//    }
//    fallback_resources[i] = 0;

   // ARB_install_handlers(aw_handlers);


    //FIXME font stuff
//    {
//        GBDATA *gbd = (GBDATA*)application_database;
//        const char *font = GB_read_char_pntr(GB_search(gbd, "window/font", GB_FIND));
//        if (!(fontstruct = XLoadQueryFont(p_r->display, font))) {
//            if (!(fontstruct = XLoadQueryFont(p_r->display, "fixed"))) {
//                printf("can not load font\n");
//                exit(-1);
//            }
//        }
//    }


//    if (fontstruct->max_bounds.width == fontstruct->min_bounds.width) {
//        font_width = fontstruct->max_bounds.width;
//    }
//    else {
//        font_width = (fontstruct->min_bounds.width
//                + fontstruct->max_bounds.width) / 2;
//    }
//
//    font_height = fontstruct->max_bounds.ascent
//            + fontstruct->max_bounds.descent;
//    font_ascent = fontstruct->max_bounds.ascent;
//
//    p_r->fontlist = XmFontListCreate(fontstruct, XmSTRING_DEFAULT_CHARSET);

    //button_sens_list = 0;

//    p_r->last_option_menu = p_r->current_option_menu = p_r->option_menu_list = NULL;
//    p_r->last_toggle_field = p_r->toggle_field_list = NULL;
//    p_r->last_selection_list = p_r->selection_list = NULL;

//    value_changed = false;
//    y_correction_for_input_labels = 5;
//    global_mask = AWM_ALL;


//    p_r->colormap = DefaultColormapOfScreen(XtScreen(p_r->toplevel_widget));
//    p_r->clock_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget), XC_watch);
//    p_r->question_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget), XC_question_arrow);

//    aw_root_create_color_map(this);
//    aw_root_init_font(XtDisplay(p_r->toplevel_widget));
//    aw_install_xkeys(XtDisplay(p_r->toplevel_widget));

}




void AW_root::window_hide(AW_window *aww) {
    GTK_NOT_IMPLEMENTED;
}

void AW_root::add_timed_callback(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED;
}

void AW_root::add_timed_callback_never_disabled(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED;
}

#if defined(DEBUG)
size_t AW_root::callallcallbacks(int mode) {
    GTK_NOT_IMPLEMENTED;
}
#endif

AW_awar *AW_root::awar(const char *awar) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_root::awar_float(const char *var_name, float default_value/* = 0.0*/, AW_default default_file/* = AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}


AW_awar *AW_root::awar_string (const char *var_name, const char *default_value /*= ""*/, AW_default default_file /*= AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_root::awar_int(const char *var_name, long default_value /*= 0*/, AW_default default_file /*= AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_root::awar_no_error(const char *awar) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_root::awar_pointer(const char *var_name, void *default_value/* = NULL*/,     AW_default default_file/* = AW_ROOT_DEFAULT*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

GB_ERROR AW_root::check_for_remote_command(AW_default gb_main, const char *rm_base) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_root::dont_save_awars_with_default_value(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED;
}

GB_ERROR AW_root::execute_macro(GBDATA *gb_main, const char *file, AW_RCB1 execution_done_cb, AW_CL client_data) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

bool AW_root::is_recording_macro() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_root::main_loop() {
    gtk_main();
}

void AW_root::set_focus_callback(AW_RCB fcb, AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED;
}

GB_ERROR AW_root::start_macro_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

GB_ERROR AW_root::stop_macro_recording() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_root::unlink_awars_from_DB(GBDATA *gb_main) {
    GTK_NOT_IMPLEMENTED;
}
AW_root::~AW_root() {
    GTK_NOT_IMPLEMENTED;
}

AW_default AW_root::check_properties(AW_default aw_props) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
