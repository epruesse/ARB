#ifndef AW_ROOT_HXX
#define AW_ROOT_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef AW_BASE_HXX
#include "aw_base.hxx" // @@@ remove later
#endif
#ifndef CB_H
#include <cb.h>
#endif

#include "aw_gtk_forward_declarations.hxx"
#include "aw_assert.hxx"
#include "aw_area_management.hxx"

#include <string>
#include <vector>
class AW_selection_list;
class AW_action;

typedef char *AW_error;
// asynchronous messages:
extern char AW_ERROR_BUFFER[1024];

void aw_set_local_message(); // no message window, AWAR_ERROR_MESSAGES instead (used by EDIT4)

class AW_awar;
class AW_root_cblist;
class GB_HASH;

enum AW_Cursor {
    NORMAL_CURSOR,
    WAIT_CURSOR,
    HELP_CURSOR
};


void aw_initstatus();

// ---------------------------
//      UserActionTracker

class UserActionTracker : virtual Noncopyable {
    bool tracking;

protected:
    void set_tracking(bool track) { tracking = track; }

public:
    UserActionTracker() : tracking(false) {}
    virtual ~UserActionTracker() {}

    bool is_tracking() const { return tracking; }

    virtual void track_action(const char *action_id) = 0;
    virtual void track_awar_change(AW_awar *awar)    = 0;
    virtual bool is_replaceable() const = 0;
};
class NullTracker : public UserActionTracker {
public:
    void track_action(const char */*action_id*/) OVERRIDE {}
    void track_awar_change(AW_awar */*awar*/) OVERRIDE {}
    bool is_replaceable() const OVERRIDE { return true; }
};

// -----------------
//      AW_root

class AW_root : virtual Noncopyable {
    struct pimpl;

    // gtk dependent attributes are defined in a different header to keep this header portable
    pimpl             *prvt; /** < Contains all gtk dependent attributes. */
    AW_default         application_database; /** < FIXME */
    int                active_windows;  // number of open windows
    bool               no_exit; /** < FIXME; (was/should be) used to protect status window from being closed */
    bool               help_active; /** < true if the help mode is active, false otherwise */
    UserActionTracker *tracker;

    /**
     * Initializes the database and loads some default awars.
     * @param database FIXME
     */
    void init_variables(AW_default database);

    /**
     * FIXME
     */
    AW_default load_properties(const char *default_name);

public:
    static AW_root *SINGLETON;

    char          *program_name;
    bool           value_changed;
    GtkWidget     *changer_of_variable;

    bool delay_timer_callbacks; // true => delay timer callbacks
    bool forbid_dialogs;        // true => forbid modul dialogs

    AW_window *root_window;

    // keep track of open windows
    void window_show();
    void window_hide(AW_window *aww);


    // the real public section:

    /**
     * FIXME
     */
    AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker,
            int *argc, char **argv[]);
#if defined(UNIT_TESTS) // UT_DIFF
    AW_root(const char *properties); // fake-root for unit-tests (allows access to awar-subsystem)
#endif
    ~AW_root();

    void setUserActionTracker(UserActionTracker *user_tracker);
    UserActionTracker *getTracker() { return tracker; }

    void main_loop();

    void                process_events();           // might block
    void                process_pending_events();   // non-blocking
    AW_ProcessEventType peek_key_event(AW_window *);

    void add_timed_callback(int ms, const TimedCallback& tcb);
    void add_timed_callback_never_disabled(int ms, const TimedCallback& tcb);

    /**
     * @return true if help mode is active, false otherwise.
     */
    bool is_help_active() const;
    
    /**
     * Enable/disable help mode
     * @param value
     */
    void set_help_active(bool value);
    
    
    void set_cursor(AW_Cursor cursor);
    
    /**
     * Returns the awar specified by <code>awar</code>
     * @param awar Name of the awar that should be returned.
     * @note Terminates the program if the awar does not exist.
     */
    AW_awar *awar(const char *awar);
    AW_awar *awar_no_error(const char *awar);

    void dont_save_awars_with_default_value(GBDATA *gb_db);

    AW_awar *awar_string (const char *var_name, const char *default_value = "", AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_int    (const char *var_name, long default_value = 0,         AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_float  (const char *var_name, float default_value = 0.0,      AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_pointer(const char *var_name, GBDATA *default_value = NULL,   AW_default default_file = AW_ROOT_DEFAULT);

    AW_awar *label_is_awar(const char *label); // returns awar, if label refers to one (used by buttons, etc.)

    void unlink_awars_from_DB(GBDATA *gb_main);     // use before calling GB_close for 'gb_main', if you have AWARs in DB

    AW_default check_properties(AW_default aw_props);

    GB_ERROR save_properties(const char *filename = NULL) __ATTR__USERESULT;

    AW_action* action(const char* action_id);
    AW_action* action_try(const char* action_id);
    AW_action* action_register(const char* action_id, const AW_action& act);

    // Control sensitivity of buttons etc.:
    /**
     * Enable or disable the sensitivity of buttons defined in button_sense_list.  
     * If a button is sensitive it will receive keyboard and mouse events.
     * @param mask Every button that matches this mask will be enabled.
     *             A mask matches if (button->mask & mask != 0).
     */
    void apply_sensitivity(AW_active mask);

    /**
     * This function used to set "focus follows mouse" for motif.
     * GTK does not have this type of focus.
     **/ 
    void apply_focus_policy(bool follow_mouse);

    void track_action(const char *action_id) { tracker->track_action(action_id); }
    void track_awar_change(AW_awar *changed_awar) { tracker->track_awar_change(changed_awar); }

    bool is_tracking() const { return tracker->is_tracking(); }
    UserActionTracker *get_tracker() { return tracker; }

#if defined(DEBUG)
    size_t callallcallbacks(int mode);
    class ConstStrArray *get_action_ids();
#endif // DEBUG
};

class AW_dialog_disabled {
    bool old_state;
public:
    AW_dialog_disabled()
        : old_state(AW_root::SINGLETON->forbid_dialogs)
    {
        AW_root::SINGLETON->forbid_dialogs = false;
    }
    ~AW_dialog_disabled() {
        AW_root::SINGLETON->forbid_dialogs = old_state;
    }
};

const char *AW_font_2_ascii(AW_font font_nr);
int         AW_font_2_xfig(AW_font font_nr);

bool ARB_global_awars_initialized();
bool ARB_in_expert_mode(AW_root *awr);
inline bool ARB_in_novice_mode(AW_root *awr) { return !ARB_in_expert_mode(awr); }

void ARB_declare_global_awars(AW_root *aw_root, AW_default aw_def);
GB_ERROR ARB_bind_global_awars(GBDATA *gb_main) __ATTR__USERESULT;

__ATTR__USERESULT_TODO inline GB_ERROR ARB_init_global_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    ARB_declare_global_awars(aw_root, aw_def);
    return ARB_bind_global_awars(gb_main);
}

inline AW_default get_AW_ROOT_DEFAULT() { return AW_root::SINGLETON->check_properties(NULL); }

#else
//#error aw_root.hxx included twice
#endif


