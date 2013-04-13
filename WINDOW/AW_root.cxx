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
#include <gtk/gtk.h>
#include "aw_root.hxx"
#ifndef ARBDB_H
#include <arbdb.h>
#endif
#include <arbdbt.h>
#include "aw_awar.hxx"
#include <gdk/gdkx.h>
#include <vector>
#include "aw_xfont.hxx"
#include "aw_awar_defs.hxx"
#include "aw_window.hxx"
#include "aw_global_awars.hxx"
#include "aw_nawar.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"

//globals
//TODO use static class or namespace for globals

AW_root *AW_root::SINGLETON = NULL;

void AW_system(AW_window *aww, const char *command, const char *auto_help_file) {
    if (auto_help_file) AW_POPUP_HELP(aww, (AW_CL)auto_help_file);
    aw_message_if(GBK_system(command));
}


void AW_root::process_events() {
    gtk_main_iteration();
}


AW_ProcessEventType AW_root::peek_key_event(AW_window *) {
    GTK_NOT_IMPLEMENTED;
    return (AW_ProcessEventType)0;
}

AW_default AW_root::load_properties(const char *default_name) {
    GBDATA   *gb_default = GB_open(default_name, "rwcD");
    GB_ERROR  error;

    if (gb_default) {
        error = GB_no_transaction(gb_default);
        if (!error) {
            GBDATA *gb_tmp = GB_search(gb_default, "tmp", GB_CREATE_CONTAINER);
            error          = GB_set_temporary(gb_tmp);
        }
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
    delete AW_root::SINGLETON;
    AW_root::SINGLETON = NULL;
}

AW_root::AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker) {
    int argc = 0;
    init_root(properties, program, NoExit, user_tracker, &argc, NULL);
}

AW_root::AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker,
                 int *argc, char **argv[]) {
    init_root(properties, program, NoExit, user_tracker, argc, argv);
}

void AW_root::setUserActionTracker(UserActionTracker *user_tracker) {
    aw_assert(user_tracker);
    aw_assert(tracker->is_replaceable()); // there is already another tracker (program-logic-error)

    delete tracker;
    tracker = user_tracker;
}

AW_awar *AW_root::label_is_awar(const char *label) {
    AW_awar *awar_exists = NULL;
    size_t   off         = strcspn(label, "/ ");

    if (label[off] == '/') {                        // contains '/' and no space before first '/'
        awar_exists = awar_no_error(label);
    }
    return awar_exists;
}


unsigned int AW_root::alloc_named_data_color(char *colorname){
  
    GdkColor allocatedColor;
    gdk_color_parse(colorname, &allocatedColor);
    gdk_colormap_alloc_color(prvt.colormap, &allocatedColor, true, true);
    return allocatedColor.pixel;
}

GdkColor AW_root::getColor(unsigned int pixel) {
    GdkColor color;
    gdk_colormap_query_color(prvt.colormap, pixel, &color);
    return color;
    //note: a copy is returned to avoid memory leaks. GdkColor is small and this should not be a problem.
}

void AW_root::define_remote_command(AW_cb_struct *cbs) {
    if (cbs->f == (AW_CB)AW_POPDOWN) {
        aw_assert(!cbs->get_cd1() && !cbs->get_cd2()); // popdown takes no parameters (please pass ", 0, 0"!)
    }

    AW_cb_struct *old_cbs = (AW_cb_struct*)GBS_write_hash(action_hash, cbs->id, (long)cbs);
    if (old_cbs) {
        if (!old_cbs->is_equal(*cbs)) {                  // existing remote command replaced by different callback
#if defined(DEBUG)
            fputs(GBS_global_string("Warning: reused callback id '%s' for different callback\n", old_cbs->id), stderr);
#if defined(DEVEL_RALF) && 0
            aw_assert(0);
#endif // DEVEL_RALF
#endif // DEBUG
        }
        // do not free old_cbs, cause it's still reachable from first widget that defined this remote command
    }
}

AW_cb_struct *AW_root::search_remote_command(const char *action) {
    return (AW_cb_struct *)GBS_read_hash(action_hash, action);
}

/**
 * This function used to set "focus follows mouse" for motif.
 * GTK does not have this type of focus.
 **/ 
void AW_root::apply_focus_policy(bool /*follow_mouse*/) {
}

/**
 * Enables/Disabes widgets according to AW_active mask
 **/
void AW_root::apply_sensitivity(AW_active mask) {
    aw_assert(legal_mask(mask));

    for (std::vector<AW_button>::iterator btn = button_list.begin();
         btn != button_list.end(); ++btn) {
      btn->apply_sensitivity(mask);
    }
}

void AW_root::register_widget(GtkWidget* w, AW_active mask) {
    // Don't call register_widget directly!
    //
    // Simply set sens_mask(AWM_EXP) and after creating the expert-mode-only widgets,
    // set it back using sens_mask(AWM_ALL)

    aw_assert(w);
    aw_assert(legal_mask(mask));
    
    prvt.set_last_widget(w);

    if (mask != AWM_ALL) { // no need to make widget sensitive, if its shown unconditionally
        AW_button btn(mask, w);
        button_list.push_back(btn);
        btn.apply_sensitivity(global_mask);
    }
}


struct fallbacks {
    const char *fb;
    const char *awar;
    const char *init;
};

static struct fallbacks aw_fb[] = { 
    // Name         fallback awarname    default value
    { "FontList",   "window/font",       "8x13bold" }, 
    { "background", "window/background", "grey" },
    { "foreground", "window/foreground", "Black", },
    { 0,            "window/color_1",    "red", },
    { 0,            "window/color_2",    "green", },
    { 0,            "window/color_3",    "blue", },
    { 0,            0,                   0 }
};


void AW_root::init_variables(AW_default database) {
    application_database     = database;
    hash_table_for_variables = GBS_create_hash(1000, GB_MIND_CASE);
    hash_for_windows         = GBS_create_hash(100, GB_MIND_CASE);
    help_active              = false;
    selection_list           = NULL;
    last_selection_list      = NULL;
    option_menu_list         = NULL;
    last_option_menu         = NULL;
    current_option_menu      = NULL;
    number_of_option_menus   = 0;
    changer_of_variable      = NULL;

    
    FIXME("not sure if aw_fb is still needed");
    for (int i=0; aw_fb[i].awar; ++i) {
        awar_string(aw_fb[i].awar, aw_fb[i].init, application_database);
    }
}

void AW_root::append_selection_list(AW_selection_list* pList) {
    if (selection_list) {
        last_selection_list->next = pList;
        last_selection_list = last_selection_list->next;
    }
    else {
        last_selection_list = pList;
        selection_list = pList;
    }
}

AW_selection_list* AW_root::get_last_selection_list() {
    return last_selection_list;
}

void AW_root::init_root(const char* properties, const char *programname, bool NoExit, UserActionTracker *user_tracker, int *argc, char** argv[]) {
    aw_assert(!AW_root::SINGLETON);                 // only one instance allowed
    AW_root::SINGLETON = this;
    printf("props: %s", properties);

    memset((char *)this, 0, sizeof(AW_root));//initialize all attributes with 0

    init_variables(load_properties(properties));


    // initialize ARB gtk application
    //TODO font stuff
    XFontStruct *fontstruct;
    char        *fallback_resources[100];

    GTK_PARTLY_IMPLEMENTED;

    action_hash  = GBS_create_hash(1000, GB_MIND_CASE);
    no_exit      = NoExit;
    program_name = strdup(programname);

    int dargc = 2;
    char *name = strdup("arb_ntree");
    char *arg = strdup("--g-fatal-warnings");
    char *dargv[] = {name, arg};
    char **dargvp = dargv;

    gtk_init(&dargc, &dargvp);

    // add our own icon path to the gtk theme search path
    gtk_icon_theme_prepend_search_path(gtk_icon_theme_get_default(),
                                       GB_path_in_ARBLIB("pixmaps/icons"));

    color_mode = AW_RGB_COLOR; //mono color mode is not supported
    create_colormap();//load the colortable from database


    int i;
    for (i=0; i<1000 && aw_fb[i].fb; i++) {
        GBDATA *gb_awar       = GB_search((GBDATA*)application_database, aw_fb[i].awar, GB_FIND);
        fallback_resources[i] = GBS_global_string_copy("*%s: %s", aw_fb[i].fb, GB_read_char_pntr(gb_awar));
    }
    fallback_resources[i] = 0;

    tracker = user_tracker;

    //ARB_install_handlers(aw_handlers);


    /**
     *  Font Hack!
     * The whole font system is still using X.
     * right before drawing the fonts are converted to GdkFont and drawn.
     * This is a quick hack to be able to get the correct fonts.
    FIXME("The whole font system is still using X");


    //TODO font stuff
    {
        GBDATA *gbd = (GBDATA*)application_database;
        const char *font = GB_read_char_pntr(GB_search(gbd, "window/font", GB_FIND));
        if (!(fontstruct = XLoadQueryFont(display, font))) {
            if (!(fontstruct = XLoadQueryFont(display, "fixed"))) {
                printf("can not load font\n");
                exit(-1);
            }
        }
    }
//
//
    if (fontstruct->max_bounds.width == fontstruct->min_bounds.width) {
        font_width = fontstruct->max_bounds.width;
    }
    else {
        font_width = (fontstruct->min_bounds.width
                + fontstruct->max_bounds.width) / 2;
    }

    font_height = fontstruct->max_bounds.ascent
            + fontstruct->max_bounds.descent;
    font_ascent = fontstruct->max_bounds.ascent;
     */

//

    //prvt.fontlist = XmFontListCreate(fontstruct, XmSTRING_DEFAULT_CHARSET);
//
//    button_sens_list = 0;
//
//    p_r->last_option_menu = p_r->current_option_menu = p_r->option_menu_list = NULL;
//    p_r->last_toggle_field = p_r->toggle_field_list = NULL;
//    p_r->last_selection_list = p_r->selection_list = NULL;
//
//    value_changed = false;
//    y_correction_for_input_labels = 5;
//    global_mask = AWM_ALL;
//
//
//    p_r->colormap = DefaultColormapOfScreen(XtScreen(p_r->toplevel_widget));
//    p_r->clock_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget), XC_watch);
//    p_r->question_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget), XC_question_arrow);
//
//    aw_root_create_color_map(this);
    Display* display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    aw_root_init_font(display);
//    aw_install_xkeys(XtDisplay(p_r->toplevel_widget));
    
    atexit(destroy_AW_root); // do not call this before opening properties DB!
}


/**
 * A list of awar names that contain color names
 */
static const char *aw_awar_2_color[] = {
    "window/background",
    "window/foreground",
    "window/color_1",
    "window/color_2",
    "window/color_3",
    0
};

void AW_root::create_colormap() {

    prvt.color_table = (AW_rgb*)GB_calloc(sizeof(AW_rgb), AW_STD_COLOR_IDX_MAX);
    GBDATA *gbd = check_properties(NULL);
    prvt.colormap = gdk_colormap_get_system();

    // Color monitor, B&W monitor is no longer supported
    const char **awar_2_color;
    int color;
    for (color = 0, awar_2_color = aw_awar_2_color;
         *awar_2_color;
         awar_2_color++, color++)
    {
        const char *name_of_color = GB_read_char_pntr(GB_search(gbd, *awar_2_color, GB_FIND));
        GdkColor gdkColor;

        if(!gdk_color_parse (name_of_color, &gdkColor)) {
            fprintf(stderr, "gdk_color_parse(%s) failed\n", name_of_color);
        }
        else {
            gdk_colormap_alloc_color(prvt.colormap, &gdkColor, false, true);
            prvt.color_table[color] = gdkColor.pixel;

        }
    }

    GdkColor black;
    if(gdk_color_black(prvt.colormap, &black)) {
        prvt.foreground = black.pixel;
    }
    else {
        fprintf(stderr, "gdk_color_black() failed\n");
    }
}


void AW_root::window_hide(AW_window */*aww*/) {
    GTK_NOT_IMPLEMENTED;
}

/// begin timer stuff 

// internal struct to pass data to handler
struct AW_timer_cb_struct : virtual Noncopyable {
    AW_root *ar;
    AW_RCB   f;
    AW_CL    cd1;
    AW_CL    cd2;

    AW_timer_cb_struct(AW_root *ari, AW_RCB cb, AW_CL cd1i, AW_CL cd2i)
        : ar(ari), f(cb), cd1(cd1i), cd2(cd2i) {}
};


static gboolean AW_timer_callback(gpointer aw_timer_cb_struct) {
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *) aw_timer_cb_struct;
    if (!tcbs)
        return false;

    AW_root *root = tcbs->ar;
    if (root->disable_callbacks) {
        // delay the timer callback for 25ms
        g_timeout_add(25, AW_timer_callback, aw_timer_cb_struct);
    }
    else {
        tcbs->f(root, tcbs->cd1, tcbs->cd2);
        delete tcbs; // timer only once
    }
    return false;
}

static gboolean AW_timer_callback_never_disabled(gpointer aw_timer_cb_struct) {
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *) aw_timer_cb_struct;
    if (!tcbs)
        return false;

    tcbs->f(tcbs->ar, tcbs->cd1, tcbs->cd2);
    delete tcbs; // timer only once
    return false;
}

void AW_root::add_timed_callback(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2) {
    g_timeout_add(ms, AW_timer_callback, 
                  new AW_timer_cb_struct(this, f, cd1, cd2));
}

void AW_root::add_timed_callback_never_disabled(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2) {
    g_timeout_add(ms,  AW_timer_callback_never_disabled,
                  new AW_timer_cb_struct(this, f, cd1, cd2));
}
/// end timer stuff


#if defined(DEBUG)
size_t AW_root::callallcallbacks(int mode) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
#endif

AW_awar *AW_root::awar(const char *var_name) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) GBK_terminatef("AWAR %s not defined", var_name);
    return vs;
}

AW_awar *AW_root::awar_float(const char *var_name, float default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_FLOAT, var_name, "", (double)default_value, default_file, this);
        GBS_write_hash(hash_table_for_variables, var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_string(const char *var_name, const char *default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_STRING, var_name, default_value, 0, default_file, this);
        GBS_write_hash(hash_table_for_variables, var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_int(const char *var_name, long default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_INT, var_name, (const char *)default_value, 0, default_file, this);
        GBS_write_hash(hash_table_for_variables, var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_no_error(const char *var_name) {
    return hash_table_for_variables ? (AW_awar *)GBS_read_hash(hash_table_for_variables, var_name) : NULL;
}


AW_awar *AW_root::awar_pointer(const char *var_name, void *default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_POINTER, var_name, (const char *)default_value, 0.0, default_file, this);
        GBS_write_hash(hash_table_for_variables, var_name, (long)vs);
    }
    return vs;
}

void AW_root::dont_save_awars_with_default_value(GBDATA */*gb_db*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_root::main_loop() {
    gtk_main();
}

void AW_root::unlink_awars_from_DB(AW_default database) {
    GTK_NOT_IMPLEMENTED;
}

AW_root::~AW_root() {
    delete tracker; tracker = NULL;

    if (application_database) {
        GB_close(application_database);
        application_database = NULL;
    }
    GTK_PARTLY_IMPLEMENTED;
}

AW_default AW_root::check_properties(AW_default aw_props) {
    return aw_props ? aw_props : application_database;
}


/**
 * @return true if help mode is active, false otherwise.
 */
bool AW_root::is_help_active() const {
    return help_active;
}

/**
 * Enable/disable help mode
 * @param value
 */
void AW_root::set_help_active(bool value) {
    help_active = value;
}

void AW_root::normal_cursor() {
    GTK_NOT_IMPLEMENTED;
}

void AW_root::help_cursor()  {
    GTK_NOT_IMPLEMENTED;
    FIXME("merge help_cursor(), normal_cursor(), wait_cursor() into set_cursor(enum)");
}

void AW_root::wait_cursor() {
    GTK_NOT_IMPLEMENTED;
}

