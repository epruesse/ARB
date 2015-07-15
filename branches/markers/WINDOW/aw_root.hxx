#ifndef AW_ROOT_HXX
#define AW_ROOT_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef AW_BASE_HXX
#include "aw_base.hxx" // @@@ remove later
#endif

#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

#if defined(ASSERTION_USED)
#define legal_mask(m) (((m)&AWM_ALL) == (m))
#endif // ASSERTION_USED

typedef char *AW_error;

// asynchronous messages:
extern char AW_ERROR_BUFFER[1024];

void aw_set_local_message(); // no message window, AWAR_ERROR_MESSAGES instead (used by EDIT4)

// Read a string from the user :
char *aw_input(const char *title, const char *prompt, const char *default_input);
char *aw_input(const char *prompt, const char *default_input);
inline char *aw_input(const char *prompt) { return aw_input(prompt, NULL); }

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix);

class  AW_root_Motif;
class  AW_awar;
struct AW_buttons_struct;
class  AW_root_cblist;
struct GB_HASH;
class  AW_cb;

enum AW_ProcessEventType {
    NO_EVENT     = 0,
    KEY_PRESSED  = 2,
    KEY_RELEASED = 3
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
    AW_default         application_database;
    AW_buttons_struct *button_sens_list;

    UserActionTracker *tracker;

    void create_colormap();

    void init_variables(AW_default database);
    void exit_variables();

    void init_root(const char *programname, bool no_exit);
    void exit_root();
    AW_default load_properties(const char *default_name);

public:
    static AW_root *SINGLETON;

    AW_root_Motif *prvt;                            // Do not use !!!
    bool           value_changed;
    Widget         changer_of_variable;
    int            y_correction_for_input_labels;
    AW_active      global_mask;
    bool           focus_follows_mouse;
    GB_HASH       *hash_table_for_variables;
    int            number_of_toggle_fields;
    int            number_of_option_menus;
    char          *program_name;

    bool            disable_callbacks;
    AW_window      *current_modal_window;
    AW_root_cblist *focus_callback_list; // @@@ always NULL?

    int  active_windows;
    void window_show();         // a window is set to screen
    void window_hide(AW_window *aww);

    // the read only public section:
    short       font_width;
    short       font_height;
    short       font_ascent;
    GB_HASH    *hash_for_windows;

    // the real public section:
    AW_root(const char *propertyFile, const char *program, bool no_exit, UserActionTracker *user_tracker, int* argc, char*** argv);
#if defined(UNIT_TESTS) // UT_DIFF
    AW_root(const char *properties); // fake-root for unit-tests (allows access to awar-subsystem)
#endif
    ~AW_root();

    void setUserActionTracker(UserActionTracker *user_tracker);
    UserActionTracker *getTracker() { return tracker; }

    enum { AW_MONO_COLOR, AW_RGB_COLOR }    color_mode;

    void main_loop();

    void                process_events();           // might block
    void                process_pending_events();   // non-blocking
    AW_ProcessEventType peek_key_event(AW_window *);

    void add_timed_callback(int ms, const TimedCallback& tcb);
    void add_timed_callback_never_disabled(int ms, const TimedCallback& tcb);

    bool is_focus_callback(AW_RCB fcb) const;

    AW_awar *awar(const char *awar);
    AW_awar *awar_no_error(const char *awar);

    void dont_save_awars_with_default_value(GBDATA *gb_db);

    AW_awar *awar_string (const char *var_name, const char *default_value = "", AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_int    (const char *var_name, long default_value = 0,         AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_float  (const char *var_name, float default_value = 0.0,      AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_pointer(const char *var_name, GBDATA *default_value = NULL,   AW_default default_file = AW_ROOT_DEFAULT);

    AW_awar *label_is_awar(const char *label); // returns awar, if label refers to one (used by buttons, etc.)

    void unlink_awars_from_DB(GBDATA *gb_main);     // use before calling GB_close for 'gb_main', if you have AWARs in DB

    AW_default check_properties(AW_default aw_props) {
        return aw_props ? aw_props : application_database;
    }

    GB_ERROR save_properties(const char *filename = NULL) __ATTR__USERESULT;

    // Control sensitivity of buttons etc.:
    void apply_sensitivity(AW_active mask);
    void apply_focus_policy(bool follow_mouse);
    void make_sensitive(Widget w, AW_active mask);
    bool remove_button_from_sens_list(Widget button);

    void track_action(const char *action_id) { tracker->track_action(action_id); }
    void track_awar_change(AW_awar *changed_awar) { tracker->track_awar_change(changed_awar); }

    bool is_tracking() const { return tracker->is_tracking(); }
    UserActionTracker *get_tracker() { return tracker; }

    void define_remote_command(class AW_cb *cbs);
    AW_cb *search_remote_command(const char *action);

#if defined(DEBUG)
    size_t callallcallbacks(int mode);
    class ConstStrArray *get_action_ids();
#endif // DEBUG
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

void AW_system(AW_window *aww, const char *command, const char *auto_help_file);

#else
#error aw_root.hxx included twice
#endif


