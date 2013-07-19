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
#include "arb_handlers.h"

#include "aw_awar.hxx"
#include "aw_awar_impl.hxx"
#include <gdk/gdkx.h>
#include <vector>
#include <unordered_map>
#include "aw_awar_defs.hxx"
#include "aw_window.hxx"
#include "aw_global_awars.hxx"
#include "aw_nawar.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"
#include "aw_status.hxx"
#include "aw_action.hxx"

// for gdb detection; should go into CORE, actually
#include <sys/ptrace.h>
#include <sys/wait.h>

//globals
//TODO use static class or namespace for globals

AW_root *AW_root::SINGLETON = NULL;

typedef std::unordered_map<std::string, AW_action*> action_hash_t;
typedef std::unordered_map<std::string, AW_awar*> awar_hash_t;

struct AW_root::pimpl : public Noncopyable {
    GdkColormap* colormap; /** < Contains a color for each value in AW_base::AW_color_idx */
    AW_rgb  *color_table; /** < Contains pixel values that can be used to retrieve a color from the colormap  */
    AW_rgb foreground; /** <Pixel value of the foreground color */
    GdkCursor* cursors[3]; /** < The available cursors. Use AW_root::AW_Cursor as index when accessing */    
    AW_active  active_mask;

    action_hash_t action_hash;
    awar_hash_t   awar_hash;

    pimpl() : colormap(NULL), color_table(NULL), foreground(0) {}
};

/// functions

void AW_system(AW_window *aww, const char *command, const char *auto_help_file) {
    aw_return_if_fail(command);

    if (auto_help_file) AW_POPUP_HELP(aww, (AW_CL)auto_help_file);
    aw_message_if(GBK_system(command));
}

void AW_clock_cursor(AW_root *awr) {
    awr->set_cursor(WAIT_CURSOR);
}

void AW_normal_cursor(AW_root *awr) {
    awr->set_cursor(NORMAL_CURSOR);
}

void AW_root::process_events() {
    gtk_main_iteration();
}

static bool AW_IS_VALID_HKEY(const char* key) {
    GB_ERROR err = GB_check_hkey(key);
    if (err) g_warning("%s",err);
    return err == NULL;
}

AW_ProcessEventType AW_root::peek_key_event(AW_window*) {
    GdkEvent *ev = gdk_event_peek();
    if (!ev) return NO_EVENT;

    AW_ProcessEventType awev = (AW_ProcessEventType)ev->type;

    gdk_event_free(ev);
    return awev;
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

void AW_root::setUserActionTracker(UserActionTracker *user_tracker) {
    aw_return_if_fail(user_tracker);
    aw_return_if_fail(tracker->is_replaceable()); // there is already another tracker (program-logic-error)

    delete tracker;
    tracker = user_tracker;
}

unsigned int AW_root::alloc_named_data_color(char *colorname){
  
    GdkColor allocatedColor;
    gdk_color_parse(colorname, &allocatedColor);
    gdk_colormap_alloc_color(prvt->colormap, &allocatedColor, true, true);
    return allocatedColor.pixel;
}

GdkColor AW_root::getColor(unsigned int pixel) {
    GdkColor color;
    gdk_colormap_query_color(prvt->colormap, pixel, &color);
    return color;
    //note: a copy is returned to avoid memory leaks. GdkColor is small and this should not be a problem.
}

/** Fetch action from hash, return NULL on failure */
AW_action* AW_root::action_try(const char* action_id) {
    if (action_id == NULL) return NULL;
    std::string id(action_id);
    if (prvt->action_hash.find(id) == prvt->action_hash.end()) return NULL;
    return prvt->action_hash[id];
}

/** Fetch action from hash, terminate on failure */
AW_action* AW_root::action(const char* action_id) { 
    AW_action *res = action_try(action_id);
    if (res == NULL) {
        GBK_terminatef("Invalid action name '%s'!", action_id);
    }
    return res;
}

/** Register action */
AW_action* AW_root::action_register(const char* action_id, const AW_action& _act) {
    if (!action_id) {
        // WORKAROUND for empty action_id (no macro_name supploed)
        int i = 1;
        while (action_try(action_id = GBS_global_string("unnamed_%i", i)) != NULL) i++;
        aw_warning("replaced missing action name with '%s'", action_id);
    } 
    else if (action_try(action_id)) {
        // WORKAROUND for duplicate action_id (e.g. CLOSE on SPECIES_INFO DETACH)
        int i = 1;
        while (action_try(GBS_global_string("%s_%i", action_id, i)) != NULL) i++;
        action_id = GBS_global_string("%s_%i", action_id, i);
        aw_warning("replaced duplicate action name with '%s'", action_id);
    }
    
    AW_action* act = new AW_action(_act);
    act->set_id(action_id);
    prvt->action_hash[std::string(action_id)] = act;

    return act;
}

/** Register action */
AW_action* AW_root::action_register(const char* action_id, 
                           const char* label, const char *icon,
                           const char* tooltip, const char* help_entry,
                           AW_active mask) {
    AW_action act;
    act.set_label(label);
    act.set_icon(icon);
    act.set_tooltip(tooltip);
    act.set_help(help_entry);
    act.set_active_mask(mask);
    
    return action_register(action_id, act);
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
    aw_return_if_fail(legal_mask(mask));

    for (action_hash_t::iterator it = prvt->action_hash.begin();
         it != prvt->action_hash.end(); ++it) {
        it->second->enable_by_mask(mask);
    }
    prvt->active_mask = mask;
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

static void aw_log_handler(const char* /*domain*/, 
                           GLogLevelFlags /*log_level*/,
                           const gchar *message,
                           gpointer /*user_data*/) {
    aw_message(message);
}

AW_root::AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker,
                 int *argc, char **argv[]) 
    : prvt(new pimpl),
      application_database(load_properties(properties)),
      no_exit(NoExit),
      help_active(false),      tracker(user_tracker),
      
      program_name(strdup(program)),
      value_changed(false),
      changer_of_variable(NULL),
      disable_callbacks(false),
      current_modal_window(NULL),
      root_window(NULL)
{
    // hmm. we should probably throw an exception here. 
    aw_return_if_fail(AW_root::SINGLETON == NULL);  // only one instance allowed
    
    AW_root::SINGLETON = this;
    
    for (int i=0; aw_fb[i].awar; ++i) {
        awar_string(aw_fb[i].awar, aw_fb[i].init, application_database);
    }

    gtk_init(argc, argv);

    // add our own icon path to the gtk theme search path
    gtk_icon_theme_prepend_search_path(gtk_icon_theme_get_default(),
                                       GB_path_in_ARBLIB("pixmaps/icons"));

    color_mode = AW_RGB_COLOR; //mono color mode is not supported
    create_colormap();//load the colortable from database

    prvt->cursors[NORMAL_CURSOR] = gdk_cursor_new(GDK_LEFT_PTR);
    prvt->cursors[WAIT_CURSOR] = gdk_cursor_new(GDK_WATCH);
    prvt->cursors[HELP_CURSOR] = gdk_cursor_new(GDK_QUESTION_ARROW);
    
    ARB_install_handlers(aw_handlers);

    // register handler with glib to use our own message display system
    g_log_set_handler(NULL, // just application messages
                      (GLogLevelFlags)(G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                      aw_log_handler, 
                      NULL);
    
    atexit(destroy_AW_root); // do not call this before opening properties DB!
}

#if defined(UNIT_TESTS)
AW_root::AW_root(const char *propertyFile) 
    : prvt(new pimpl),
      application_database(load_properties(propertyFile)),
      no_exit(false),
      help_active(false),
      tracker(NULL),
      program_name(strdup("dummy")),
      value_changed(false),
      changer_of_variable(NULL),
      disable_callbacks(false),
      current_modal_window(NULL),
      root_window(NULL)
{
    g_return_if_fail(AW_root::SINGLETON == NULL);

    AW_root::SINGLETON = this;
    atexit(destroy_AW_root); // do not call this before opening properties DB!
}
#endif

AW_root::~AW_root() {
    delete tracker; tracker = NULL;

    // delete awars
    for (awar_hash_t::iterator it = prvt->awar_hash.begin();
         it != prvt->awar_hash.end(); ++it) {
        delete it->second;
    }

    // delete actions
    for (action_hash_t::iterator it = prvt->action_hash.begin();
         it != prvt->action_hash.end(); ++it) {
        delete it->second;
    }

    // close config database
    if (application_database) {
        GB_close(application_database);
        application_database = NULL;
    }

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
    prvt->color_table = (AW_rgb*)GB_calloc(sizeof(AW_rgb), AW_STD_COLOR_IDX_MAX);
    GBDATA *gbd = check_properties(NULL);
    prvt->colormap = gdk_colormap_get_system();

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
            gdk_colormap_alloc_color(prvt->colormap, &gdkColor, false, true);
            prvt->color_table[color] = gdkColor.pixel;

        }
    }

    GdkColor black;
    if(gdk_color_black(prvt->colormap, &black)) {
        prvt->foreground = black.pixel;
    }
    else {
        fprintf(stderr, "gdk_color_black() failed\n");
    }
}

AW_rgb*& AW_root::getColorTable() {
    return prvt->color_table;
}


void AW_root::window_hide(AW_window *aww) {
    active_windows--;
    if (active_windows < 0) {
        exit(0);
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

    typedef GSourceFunc timer_callback;

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
        g_timeout_add(ms, tc, this);
    }
    void recallOrUninstall(unsigned restart, timer_callback tc) {
        if (restart) callAfter(restart, tc);
        else delete this;
    }
};

static gboolean AW_timer_callback(gpointer aw_timer_cb_struct) {
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *)aw_timer_cb_struct;
    if (tcbs) {
        unsigned restart = tcbs->callOrDelayIfDisabled();
        tcbs->recallOrUninstall(restart, AW_timer_callback);
    }
    return false;
}
static gboolean AW_timer_callback_never_disabled(gpointer aw_timer_cb_struct) {
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *)aw_timer_cb_struct;
    if (tcbs) {
        unsigned restart = tcbs->call();
        tcbs->recallOrUninstall(restart, AW_timer_callback_never_disabled);
    }
    return false;
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
    aw_return_val_if_fail(AW_IS_VALID_HKEY(var_name), NULL);

    if (prvt->awar_hash.find(var_name) != prvt->awar_hash.end()) {
        return prvt->awar_hash[var_name];
    } else {
        return NULL;
    }
}

AW_awar *AW_root::label_is_awar(const char *label) {
    if (GB_check_hkey(label)) { 
        GB_clear_error();
        return NULL; // not a valid key
    } else {
        return awar_no_error(label); // might exist
    }
}

AW_awar *AW_root::awar(const char *var_name) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) GBK_terminatef("AWAR '%s' not defined", var_name);
    return vs;
}

AW_awar *AW_root::awar_float(const char *var_name, float default_value, AW_default default_file) {
    aw_return_val_if_fail(AW_IS_VALID_HKEY(var_name), NULL);

    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_float(var_name, (double)default_value, default_file, this);
        prvt->awar_hash[var_name] = vs;
    }
    return vs;
}

AW_awar *AW_root::awar_string(const char *var_name, const char *default_value, AW_default default_file) {
    aw_return_val_if_fail(AW_IS_VALID_HKEY(var_name), NULL);

    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_string(var_name, default_value, default_file, this);
        prvt->awar_hash[var_name] = vs;
    }
    return vs;
}

AW_awar *AW_root::awar_int(const char *var_name, long default_value, AW_default default_file) {
    aw_return_val_if_fail(AW_IS_VALID_HKEY(var_name), NULL);

    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_int(var_name, default_value, default_file, this);
        prvt->awar_hash[var_name] = vs;
    }
    return vs;
}

AW_awar *AW_root::awar_pointer(const char *var_name, void *default_value, AW_default default_file) {
    aw_return_val_if_fail(AW_IS_VALID_HKEY(var_name), NULL);

    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_pointer(var_name, default_value, default_file, this);
        prvt->awar_hash[var_name] = vs;
    }
    return vs;
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

    for (awar_hash_t::iterator it = prvt->awar_hash.begin();
         it != prvt->awar_hash.end(); ++it) {
        ((AW_awar_impl*)it->second)->set_temp_if_is_default(gb_db);
    }
}

void AW_root::main_loop() {
    gtk_main();
}

void AW_root::unlink_awars_from_DB(GBDATA* gb_main) {
    g_return_if_fail(GB_get_root(gb_main) == gb_main);

    GB_transaction ta(gb_main);

    for (awar_hash_t::iterator it = prvt->awar_hash.begin();
         it != prvt->awar_hash.end(); ++it) {
        ((AW_awar_impl*)it->second)->unlink_from_DB(gb_main);
    }
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
void AW_root::set_cursor(AW_Cursor cursor) {
    GdkWindow* rootWindow = gdk_screen_get_root_window(gdk_screen_get_default());

    aw_return_if_fail(rootWindow);

    gdk_window_set_cursor(rootWindow, prvt->cursors[cursor]);
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
