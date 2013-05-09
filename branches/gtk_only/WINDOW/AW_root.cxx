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
#include "aw_xfont.hxx"
#include "aw_awar_defs.hxx"
#include "aw_window.hxx"
#include "aw_global_awars.hxx"
#include "aw_nawar.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"
#include "aw_status.hxx"

// for gdb detection; should go into CORE, actually
#include <sys/ptrace.h>
#include <sys/wait.h>

//globals
//TODO use static class or namespace for globals

AW_root *AW_root::SINGLETON = NULL;

class AW_root::pimpl {
private:
    friend class AW_root;

    GdkColormap* colormap; /** < Contains a color for each value in AW_base::AW_color_idx */
    AW_rgb  *color_table; /** < Contains pixel values that can be used to retrieve a color from the colormap  */
    AW_rgb foreground; /** <Pixel value of the foreground color */
    GtkWidget* last_widget; /** < TODO, no idea what it does */
    GdkCursor* cursors[3]; /** < The available cursors. Use AW_root::AW_Cursor as index when accessing */    
    
    pimpl() : colormap(NULL), color_table(NULL), foreground(0), last_widget(NULL)  {}
    
    
    GtkWidget* get_last_widget() const { return last_widget; }
    void set_last_widget(GtkWidget* w) { last_widget = w; }
    
};

void AW_system(AW_window *aww, const char *command, const char *auto_help_file) {
    aw_assert(NULL != command);
    if (auto_help_file) AW_POPUP_HELP(aww, (AW_CL)auto_help_file);
    aw_message_if(GBK_system(command));
}


void AW_root::process_events() {
    gtk_main_iteration();
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
    gdk_colormap_alloc_color(prvt->colormap, &allocatedColor, true, true);
    return allocatedColor.pixel;
}

GdkColor AW_root::getColor(unsigned int pixel) {
    GdkColor color;
    gdk_colormap_query_color(prvt->colormap, pixel, &color);
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

    active_mask = mask;
    for (std::vector<AW_button>::iterator btn = button_list.begin();
         btn != button_list.end(); ++btn) {
        btn->apply_sensitivity(active_mask);
    }
}

void AW_root::register_widget(GtkWidget* w, AW_active mask) {
    // Don't call register_widget directly!
    //
    // Simply set sens_mask(AWM_EXP) and after creating the expert-mode-only widgets,
    // set it back using sens_mask(AWM_ALL)

    aw_assert(w);
    aw_assert(legal_mask(mask));
    
    prvt->set_last_widget(w);

    if (mask != AWM_ALL) { // no need to make widget sensitive, if its shown unconditionally
        AW_button btn(mask, w);
        button_list.push_back(btn);
        btn.apply_sensitivity(active_mask);
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



AW_root::AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker,
                 int *argc, char **argv[]) 
    : prvt(new pimpl),
      action_hash(GBS_create_hash(1000, GB_MIND_CASE)),
      application_database(load_properties(properties)),
      button_list(),
      no_exit(NoExit),
      help_active(false),
      tracker(user_tracker),
      
      program_name(strdup(program)),
      value_changed(false),
      changer_of_variable(NULL),
      active_mask(AWM_ALL),
      awar_hash(GBS_create_hash(1000, GB_MIND_CASE)),
      disable_callbacks(false),
      current_modal_window(NULL),
      root_window(NULL)
{
    aw_assert(!AW_root::SINGLETON);                 // only one instance allowed
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
    
    atexit(destroy_AW_root); // do not call this before opening properties DB!
}

#if defined(UNIT_TESTS)
AW_root::AW_root(const char *propertyFile) 
    : prvt(new pimpl),
      action_hash(GBS_create_hash(1000, GB_MIND_CASE)),
      application_database(load_properties(propertyFile)),
      button_list(),
      no_exit(false),
      help_active(false),
      tracker(NULL),
      
      program_name("dummy"),
      value_changed(false),
      changer_of_variable(NULL),
      active_mask(AWM_ALL),
      awar_hash(GBS_create_hash(1000, GB_MIND_CASE)),
      disable_callbacks(false),
      current_modal_window(NULL),
      root_window(NULL)
{
    aw_assert(!AW_root::SINGLETON);                 // only one instance allowed
    AW_root::SINGLETON = this;
    
    atexit(destroy_AW_root); // do not call this before opening properties DB!
}
#endif


    static long delete_awar(const char*, long val, void*) {
    AW_awar* awar = (AW_awar*) val;
    delete awar;
    return 0; // remove from hash
}

AW_root::~AW_root() {
    delete tracker; tracker = NULL;

    GBS_hash_do_loop(awar_hash, delete_awar, NULL);
    GBS_free_hash(awar_hash);
    awar_hash = NULL;
    
    // FIXME: clear action hash

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


AW_awar *AW_root::awar_no_error(const char *var_name) {
    return (AW_awar*)GBS_read_hash(awar_hash, var_name);
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
        vs           = new AW_awar_float(var_name, (double)default_value, default_file, this);
        GBS_write_hash(awar_hash, var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_string(const char *var_name, const char *default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_string(var_name, default_value, default_file, this);
        GBS_write_hash(awar_hash, var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_int(const char *var_name, long default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_int(var_name, default_value, default_file, this);
        GBS_write_hash(awar_hash, var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_pointer(const char *var_name, void *default_value, AW_default default_file) {
    AW_awar *vs = awar_no_error(var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar_pointer(var_name, default_value, default_file, this);
        GBS_write_hash(awar_hash, var_name, (long)vs);
    }
    return vs;
}

static long _aw_root_awar_set_temp_if_is_default(const char *, long val, void *cl_gb_db) {
    AW_awar_impl *awar = (AW_awar_impl*)val;
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

    GBS_hash_do_loop(awar_hash, _aw_root_awar_set_temp_if_is_default, (void*)gb_db);
}

void AW_root::main_loop() {
    gtk_main();
}

static long _aw_root_unlink_awar_from_db(const char*/*key*/, long cl_awar, void *cl_gb_main) {
    AW_awar_impl *awar   = (AW_awar_impl*)cl_awar;
    GBDATA       *gbmain = (GBDATA*)cl_gb_main;
    awar->unlink_from_DB(gbmain);
    return cl_awar;
}

void AW_root::unlink_awars_from_DB(AW_default database) {
    GBDATA *gb_main = (GBDATA*)database;
    aw_assert(GB_get_root(gb_main) == gb_main);
    GB_transaction ta(gb_main);
    GBS_hash_do_loop(awar_hash, _aw_root_unlink_awar_from_db, gb_main);
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
    aw_assert(NULL != rootWindow);
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
