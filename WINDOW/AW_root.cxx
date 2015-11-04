// =============================================================== //
//                                                                 //
//   File      : AW_root.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_nawar.hxx"
#include "aw_msg.hxx"
#include "aw_window.hxx"
#include "aw_window_Xm.hxx"
#include "aw_status.hxx"
#include "aw_xkey.hxx"

#include <arb_handlers.h>
#include <arbdbt.h>

#include <list>

#include <X11/cursorfont.h>

AW_root *AW_root::SINGLETON = NULL;

void AW_system(AW_window *aww, const char *command, const char *auto_help_file) {
    if (auto_help_file) AW_help_popup(aww, auto_help_file);
    aw_message_if(GBK_system(command));
}

void AW_clock_cursor(AW_root *awr) {
    awr->prvt->set_cursor(0, 0, awr->prvt->clock_cursor);
}

void AW_normal_cursor(AW_root *awr) {
    awr->prvt->set_cursor(0, 0, 0);
}

void AW_help_entry_pressed(AW_window *aww) {
    AW_root *root = aww->get_root();
    p_global->help_active = 1;
}

void AW_root::process_events() {
    XtAppProcessEvent(p_r->context, XtIMAll);
}
void AW_root::process_pending_events() {
    XtInputMask pending = XtAppPending(p_r->context);
    while (pending) {
        XtAppProcessEvent(p_r->context, pending);
        pending = XtAppPending(p_r->context);
    }
}


AW_ProcessEventType AW_root::peek_key_event(AW_window *) {
    //! Returns type if key event follows, else 0

    XEvent xevent;
    Boolean result = XtAppPeekEvent(p_r->context, &xevent);

    if (!result) return NO_EVENT;
    if ((xevent.type != KeyPress) && (xevent.type != KeyRelease)) return NO_EVENT;
    return (AW_ProcessEventType)xevent.type;
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


AW_root::AW_root(const char *propertyFile, const char *program, bool no_exit, UserActionTracker *user_tracker, int */*argc*/, char ***/*argv*/)
    : tracker(user_tracker),
      changer_of_variable(0),
      focus_follows_mouse(false),
      number_of_toggle_fields(0),
      number_of_option_menus(0),
      disable_callbacks(false),
      current_modal_window(NULL),
      active_windows(0)
{
    aw_assert(!AW_root::SINGLETON);                 // only one instance allowed
    AW_root::SINGLETON = this;

    prvt = new AW_root_Motif;

    init_variables(load_properties(propertyFile));
    init_root(program, no_exit);

    tracker = user_tracker;

    atexit(destroy_AW_root); // do not call this before opening properties DB!
}

#if defined(UNIT_TESTS)
AW_root::AW_root(const char *propertyFile)
    : button_sens_list(NULL),
      tracker(NULL),
      prvt(NULL),
      value_changed(false),
      changer_of_variable(0),
      y_correction_for_input_labels(0),
      global_mask(0),
      focus_follows_mouse(false),
      number_of_toggle_fields(0),
      number_of_option_menus(0),
      program_name(NULL),
      disable_callbacks(false),
      current_modal_window(NULL),
      active_windows(0),
      font_width(0),
      font_height(0),
      font_ascent(0),
      color_mode(AW_MONO_COLOR)
{
    aw_assert(!AW_root::SINGLETON);                 // only one instance allowed
    AW_root::SINGLETON = this;

    init_variables(load_properties(propertyFile));
    atexit(destroy_AW_root); // do not call this before opening properties DB!
}
#endif

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

void AW_root::define_remote_command(AW_cb *cbs) {
    if (cbs->contains(AnyWinCB(AW_POPDOWN))) {
        aw_assert(!cbs->get_cd1() && !cbs->get_cd2()); // popdown takes no parameters (please pass ", 0, 0"!)
    }

    AW_cb *old_cbs = (AW_cb*)GBS_write_hash(prvt->action_hash, cbs->id, (long)cbs);
    if (old_cbs) {
        if (!old_cbs->is_equal(*cbs)) {                  // existing remote command replaced by different callback
#if defined(DEBUG)
            fputs(GBS_global_string("Warning: reused callback id '%s' for different callback\n", old_cbs->id), stderr);
#if defined(DEVEL_RALF) && 1
            aw_assert(0);
#endif // DEVEL_RALF
#endif // DEBUG
        }
        // do not free old_cbs, cause it's still reachable from first widget that defined this remote command
    }
}

AW_cb *AW_root::search_remote_command(const char *action) {
    return (AW_cb *)GBS_read_hash(prvt->action_hash, action);
}

static long set_focus_policy(const char *, long cl_aww, void *) {
    AW_window *aww = (AW_window*)cl_aww;
    aww->set_focus_policy(aww->get_root()->focus_follows_mouse);
    return cl_aww;
}
void AW_root::apply_focus_policy(bool follow_mouse) {
    focus_follows_mouse = follow_mouse;
    GBS_hash_do_loop(hash_for_windows, set_focus_policy, 0);
}

void AW_root::apply_sensitivity(AW_active mask) {
    aw_assert(legal_mask(mask));
    AW_buttons_struct *list;

    global_mask = mask;
    for (list = button_sens_list; list; list = list->next) {
        XtSetSensitive(list->button, (list->mask & mask) ? True : False);
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

    for (int i=0; aw_fb[i].awar; ++i) {
        awar_string(aw_fb[i].awar, aw_fb[i].init, application_database);
    }
}

static void aw_message_and_dump_stderr(const char *msg) {
    fflush(stdout);
    fprintf(stderr, "ARB: %s\n", msg); // print to console as well
    fflush(stderr);
    aw_message(msg);
}
static void dump_stdout(const char *msg) {
    fprintf(stdout, "ARB: %s\n", msg);
}

static arb_status_implementation AW_status_impl = {
    AST_RANDOM, 
    aw_openstatus,
    aw_closestatus,
    aw_status_title, // set_title
    AW_status, // set_subtitle
    AW_status, // set_gauge
    AW_status, // user_abort
};

static arb_handlers aw_handlers = {
    aw_message_and_dump_stderr,
    aw_message,
    dump_stdout,
    AW_status_impl,
};

static void free_action(long action) {
    AW_cb *cb = (AW_cb*)action;
    delete cb;
}

void AW_root::init_root(const char *programname, bool no_exit) {
    // initialize ARB X application
    int          a             = 0;
    XFontStruct *fontstruct;
    const int    MAX_FALLBACKS = 30;
    char        *fallback_resources[MAX_FALLBACKS];

    prvt->action_hash = GBS_create_dynaval_hash(1000, GB_MIND_CASE, free_action); // actions are added via define_remote_command()

    p_r-> no_exit = no_exit;
    program_name  = strdup(programname);

    int i;
    for (i=0; aw_fb[i].fb; i++) {
        GBDATA *gb_awar       = GB_search((GBDATA*)application_database, aw_fb[i].awar, GB_FIND);
        fallback_resources[i] = GBS_global_string_copy("*%s: %s", aw_fb[i].fb, GB_read_char_pntr(gb_awar));
    }
    fallback_resources[i] = 0;
    aw_assert(i<MAX_FALLBACKS);

    ARB_install_handlers(aw_handlers);

    // @@@ FIXME: the next line hangs if program runs inside debugger
    p_r->toplevel_widget = XtOpenApplication(&(p_r->context), programname,
            NULL, 0, // XrmOptionDescRec+numOpts
            &a, // &argc
            NULL, // argv
            fallback_resources,
            applicationShellWidgetClass, // widget class
            NULL, 0);

    for (i=0; fallback_resources[i]; i++) free(fallback_resources[i]);

    p_r->display = XtDisplay(p_r->toplevel_widget);

    if (p_r->display == NULL) {
        printf("cannot open display\n");
        exit(EXIT_FAILURE);
    }
    {
        GBDATA *gbd = (GBDATA*)application_database;
        const char *font = GB_read_char_pntr(GB_search(gbd, "window/font", GB_FIND));
        if (!(fontstruct = XLoadQueryFont(p_r->display, font))) {
            if (!(fontstruct = XLoadQueryFont(p_r->display, "fixed"))) {
                printf("can not load font\n");
                exit(EXIT_FAILURE);
            }
        }
    }

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

    p_r->fontlist = XmFontListCreate(fontstruct, XmSTRING_DEFAULT_CHARSET);

    button_sens_list = 0;

    p_r->last_option_menu = p_r->current_option_menu = p_r->option_menu_list = NULL;
    p_r->last_toggle_field = p_r->toggle_field_list = NULL;
    p_r->last_selection_list = p_r->selection_list = NULL;

    value_changed = false;
    y_correction_for_input_labels = 5;
    global_mask = AWM_ALL;

    p_r->screen_depth = PlanesOfScreen(XtScreen(p_r->toplevel_widget));
    if (p_r->screen_depth == 1) {
        color_mode = AW_MONO_COLOR;
    }
    else {
        color_mode = AW_RGB_COLOR;
    }
    p_r->colormap = DefaultColormapOfScreen(XtScreen(p_r->toplevel_widget));
    p_r->clock_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget), XC_watch);
    p_r->question_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget), XC_question_arrow);

    create_colormap();
    aw_root_init_font(XtDisplay(p_r->toplevel_widget));
    aw_install_xkeys(XtDisplay(p_r->toplevel_widget));

}

AW_root::~AW_root() {
    delete tracker;          tracker = NULL;
    delete button_sens_list; button_sens_list = NULL;

    exit_root();
    exit_variables();
    aw_assert(this == AW_root::SINGLETON);

    delete prvt;

    free(program_name);

    AW_root::SINGLETON = NULL;
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

    XColor xcolor_returned, xcolor_exakt;
    GBDATA *gbd = check_properties(NULL);
    prvt->color_table = (AW_rgb*)GB_calloc(sizeof(AW_rgb), AW_STD_COLOR_IDX_MAX);

    // Color monitor, B&W monitor is no longer supported
    const char **awar_2_color;
    int color;
    for (color = 0, awar_2_color = aw_awar_2_color;
         *awar_2_color;
         awar_2_color++, color++)
    {
        const char *name_of_color = GB_read_char_pntr(GB_search(gbd, *awar_2_color, GB_FIND));
        if (XAllocNamedColor(prvt->display, prvt->colormap, name_of_color, &xcolor_returned, &xcolor_exakt) == 0) {
            fprintf(stderr, "XAllocColor failed: %s\n", name_of_color);
        }
        else {
            prvt->color_table[color] = xcolor_returned.pixel;
        }
    }

    prvt->foreground = BlackPixelOfScreen(XtScreen(prvt->toplevel_widget));
    XtVaGetValues(prvt->toplevel_widget, XmNbackground, &prvt->background, NULL);
    // AW_WINDOW_DRAG see init_devices
}


void AW_root::window_hide(AW_window *aww) {
    active_windows--;
    if (active_windows<0) {
        exit(EXIT_SUCCESS);
    }
    if (current_modal_window == aww) {
        current_modal_window = NULL;
    }
}
void AW_root::window_show() {
    active_windows++;
}

/// begin timer stuff 

class AW_timer_cb_struct : virtual Noncopyable {
    AW_root       *awr;
    TimedCallback  cb;

    AW_timer_cb_struct(AW_root *aw_root, const TimedCallback& tcb) : awr(aw_root), cb(tcb) {} // use install!
public:

    typedef XtTimerCallbackProc timer_callback;

    static void install(AW_root *aw_root, const TimedCallback& tcb, unsigned ms, timer_callback tc) {
        AW_timer_cb_struct *tcbs = new AW_timer_cb_struct(aw_root, tcb);
        tcbs->callAfter(ms, tc);
    }

    unsigned call() {
        return cb(awr);
    }
    unsigned callOrDelayIfDisabled() {
        return awr->disable_callbacks
            ? 25 // delay by 25 ms
            : cb(awr);
    }
    void callAfter(unsigned ms, timer_callback tc) {
        XtAppAddTimeOut(awr->prvt->context, ms, tc, this);
    }
    void recallOrUninstall(unsigned restart, timer_callback tc) {
        if (restart) callAfter(restart, tc);
        else delete this;
    }
};

static void AW_timer_callback(XtPointer aw_timer_cb_struct, XtIntervalId*) {
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *)aw_timer_cb_struct;
    if (tcbs) {
        unsigned restart = tcbs->callOrDelayIfDisabled();
        tcbs->recallOrUninstall(restart, AW_timer_callback);
    }
}
static void AW_timer_callback_never_disabled(XtPointer aw_timer_cb_struct, XtIntervalId*) {
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *)aw_timer_cb_struct;
    if (tcbs) {
        unsigned restart = tcbs->call();
        tcbs->recallOrUninstall(restart, AW_timer_callback_never_disabled);
    }
}

void AW_root::add_timed_callback(int ms, const TimedCallback& tcb) {
    AW_timer_cb_struct::install(this, tcb, ms, AW_timer_callback);
}
void AW_root::add_timed_callback_never_disabled(int ms, const TimedCallback& tcb) {
    AW_timer_cb_struct::install(this, tcb, ms, AW_timer_callback_never_disabled);
}

/// end timer stuff

/// begin awar stuff

AW_awar *AW_root::awar_no_error(const char *var_name) {
    return hash_table_for_variables ? (AW_awar *)GBS_read_hash(hash_table_for_variables, var_name) : NULL;
}


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

AW_awar *AW_root::awar_pointer(const char *var_name, GBDATA *default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_POINTER, var_name, (const char *)default_value, 0.0, default_file, this);
        GBS_write_hash(hash_table_for_variables, var_name, (long)vs);
    }
    return vs;
}

static long awar_set_temp_if_is_default(const char *, long val, void *cl_gb_db) {
    AW_awar *awar = (AW_awar*)val;
    awar->set_temp_if_is_default((GBDATA*)cl_gb_db);
    return val;
}

void AW_root::dont_save_awars_with_default_value(GBDATA *gb_db) {
    // this should only be called
    // - before saving properties
    // - before saving any main application DB that may contain AWARs
    //
    // Bug: Awars created in main DB by clients (e.g. EDIT4) will stay temporary
    //      and will never be saved again.
    //
    // Note: uninstanciated AWARs will not be affected, so different applications with
    //       different AWAR subsets should be no problem.
    //       'different applications' here e.g. also includes different calls to arb_ntree
    //       (e.g. merge-tool, importer, tree-window, ...)
    //
    // Problems arise when an awar with the same name is used for two different purposes
    // or with different default values (regardless whether in properties or main-DB).
    // But this has already been problematic before.

    GBS_hash_do_loop(hash_table_for_variables, awar_set_temp_if_is_default, (void*)gb_db);
}

void AW_root::main_loop() {
    XtAppMainLoop(p_r->context);
}

static long AW_unlink_awar_from_DB(const char */*key*/, long cl_awar, void *cl_gb_main) {
    AW_awar *awar    = (AW_awar*)cl_awar;
    GBDATA  *gb_main = (GBDATA*)cl_gb_main;

    awar->unlink_from_DB(gb_main);
    return cl_awar;
}

void AW_root::unlink_awars_from_DB(GBDATA *gb_main) {
    aw_assert(GB_get_root(gb_main) == gb_main);

    GB_transaction ta(gb_main); // needed in awar-callbacks during unlink
    GBS_hash_do_loop(hash_table_for_variables, AW_unlink_awar_from_DB, gb_main);
}

typedef std::list<GBDATA*> DataPointers;

static GB_ERROR set_parents_with_only_temp_childs_temp(GBDATA *gbd, DataPointers& made_temp) {
    GB_ERROR error = NULL;

    if (GB_read_type(gbd) == GB_DB && !GB_is_temporary(gbd)) {
        bool has_savable_child = false;
        for (GBDATA *gb_child = GB_child(gbd); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
            bool is_tmp = GB_is_temporary(gb_child);
            if (!is_tmp) {
                error              = set_parents_with_only_temp_childs_temp(gb_child, made_temp);
                if (!error) is_tmp = GB_is_temporary(gb_child);         // may have changed

                if (!is_tmp) has_savable_child = true;
            }
        }
        if (!error && !has_savable_child) {
            error = GB_set_temporary(gbd);
            made_temp.push_back(gbd);
        }
    }

    return error;
}

static GB_ERROR clear_temp_flags(DataPointers& made_temp) {
    GB_ERROR error = NULL;
    for (DataPointers::iterator mt = made_temp.begin(); mt != made_temp.end() && !error; ++mt) {
        error = GB_clear_temporary(*mt);
    }
    return error;
}

GB_ERROR AW_root::save_properties(const char *filename) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_prop = application_database;

    if (!gb_prop) {
        error = "No properties loaded - won't save";
    }
    else {
        error = GB_push_transaction(gb_prop);
        if (!error) {
            aw_update_all_window_geometry_awars(this);
            error = GB_pop_transaction(gb_prop);
            if (!error) {
                dont_save_awars_with_default_value(gb_prop);

                DataPointers made_temp;
                error             = set_parents_with_only_temp_childs_temp(gb_prop, made_temp); // avoid saving empty containers
                if (!error) error = GB_save_in_arbprop(gb_prop, filename, "a");
                if (!error) error = clear_temp_flags(made_temp);
            }
        }
    }

    return error;
}
