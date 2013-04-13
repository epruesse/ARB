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
#include "aw_gtk_forward_declarations.hxx"
#include "aw_root_gtk.hxx"
#include "aw_assert.hxx"
//#include "aw_select.hxx"
#include <string>
#include <vector>
class AW_selection_list;
class AW_option_menu_struct;
class AW_button;


typedef void (*AW_RCB0)(AW_root*);
typedef void (*AW_RCB1)(AW_root*, AW_CL);
typedef AW_RCB AW_RCB2;

typedef AW_window *(*AW_PPP)(AW_root*, AW_CL, AW_CL);

typedef char *AW_error;
typedef void (*AW_RCB)(AW_root*, AW_CL, AW_CL);
// asynchronous messages:
extern char AW_ERROR_BUFFER[1024];

void aw_set_local_message(); // no message window, AWAR_ERROR_MESSAGES instead (used by EDIT4)

// Read a string from the user :
char *aw_input(const char *title, const char *prompt, const char *default_input);
char *aw_input(const char *prompt, const char *default_input);
inline char *aw_input(const char *prompt) { return aw_input(prompt, NULL); }

char *aw_input2awar(const char *prompt, const char *awar_value);

char *aw_string_selection     (const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*));
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*));

int aw_string_selection_button();   // returns index of last selected button (destroyed by aw_string_selection and aw_input)

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix);


class  AW_awar;
class  AW_root_cblist;
class  GB_HASH;
struct AW_cb_struct;

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

    // gtk dependent attributes are defined in a different header to keep this header portable
    AW_root_gtk        prvt; /** < Contains all gtk dependent attributes. */
    GB_HASH           *action_hash; /** < Is used to buffer and replay remote actions. */
    AW_default         application_database; /** < FIXME */
    std::vector<AW_button> button_list;
    bool               no_exit; /** < FIXME; (was/should be) used to protect status window from being closed */
    bool               help_active; /** < true if the help mode is active, false otherwise */
    UserActionTracker *tracker;

    /**
     * Main initialization function (until we can use C++11)
     */
    void init_root(const char* properties, const char *programname, bool NoExit, UserActionTracker *user_tracker, int *argc, char** argv[]);

    /**
     * Initializes prvt.colormap
     */
    void create_colormap();

    /**
     * Initializes the database and loads some default awars.
     * @param database FIXME
     */
    void init_variables(AW_default database);

    /**
     * FIXME
     */
    AW_default load_properties(const char *default_name);
    
    
    AW_selection_list *selection_list; /** < List of current selection lists */
    AW_selection_list *last_selection_list; /** < Last created selection list */
    
//    void exit_variables();



//    void exit_root();


public:
    static AW_root *SINGLETON;

    char        *program_name;
    bool           value_changed;
    GtkWidget     *changer_of_variable;
    int            y_correction_for_input_labels;
    AW_active      global_mask;
    bool           focus_follows_mouse;
    GB_HASH       *hash_table_for_variables;
    bool           variable_set_by_toggle_field;
    int            number_of_toggle_fields;
   

    bool            disable_callbacks;
    AW_window      *current_modal_window;
    AW_window      *root_window;
    AW_root_cblist *focus_callback_list;

    AW_option_menu_struct *option_menu_list;
    AW_option_menu_struct *last_option_menu;
    AW_option_menu_struct *current_option_menu;
    int number_of_option_menus;
    
    int  active_windows;
    void window_show();         // a window is set to screen
    void window_hide(AW_window *aww);

    // the read only public section:
    GB_HASH    *hash_for_windows;

    // the real public section:

    AW_rgb*& getColorTable() {
        return prvt.color_table;
    }

    /**
     * FIXME
     */
    AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker);
    AW_root(const char *properties, const char *program, bool NoExit, UserActionTracker *user_tracker,
            int *argc, char **argv[]);
          
#if defined(UNIT_TESTS)
    AW_root(const char *properties); // fake-root for unit-tests (allows access to awar-subsystem)
#endif
    ~AW_root();

    void setUserActionTracker(UserActionTracker *user_tracker);
    UserActionTracker *getTracker() { return tracker; }

    enum { AW_MONO_COLOR, //DO NOT USE!!!
           AW_RGB_COLOR
    } color_mode;

    void main_loop();

    void                process_events();           // might block
    void                process_pending_events();   // non-blocking
    AW_ProcessEventType peek_key_event(AW_window *);

    void add_timed_callback               (int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2);
    void add_timed_callback_never_disabled(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2);

    void add_timed_callback               (int ms, AW_RCB1 f, AW_CL cd1) { add_timed_callback               (ms, (AW_RCB2)f, cd1, 0); }
    void add_timed_callback_never_disabled(int ms, AW_RCB1 f, AW_CL cd1) { add_timed_callback_never_disabled(ms, (AW_RCB2)f, cd1, 0); }

    void add_timed_callback               (int ms, AW_RCB0 f) { add_timed_callback               (ms, (AW_RCB2)f, 0, 0); }
    void add_timed_callback_never_disabled(int ms, AW_RCB0 f) { add_timed_callback_never_disabled(ms, (AW_RCB2)f, 0, 0); }

    /**
     * @return true if help mode is active, false otherwise.
     */
    bool is_help_active() const;
    
    /**
     * Enable/disable help mode
     * @param value
     */
    void set_help_active(bool value);
    
    
    /**
     * Set normal cursor
     */
    void normal_cursor();
    
    /**
     * Set help cursor.
     */
    void help_cursor();
    
    /*
     * Set wait cursor
     */
    void wait_cursor();
    
    /**
     * Appends the specified list to the list of selection lists
     */
    void append_selection_list(AW_selection_list* pList);
    
    /**
     * returns the last selection list
     */
    AW_selection_list* get_last_selection_list();
    
    
    /**
     * Allocates a named color in the global colormap.
     * @param colorname
     * @return The pixel value of the color. This value can be used to retrieve the color from the
     *         global colormap (@see getColor()).
     */
    unsigned int alloc_named_data_color(char *colorname);
    
    /**
     * Retrieves a color from the global colormap.
     * @param pixel The pixel value is used as index of the colormap. This should be a value returned by alloc_named_data_color()
     * @return The color or NULL in case of error.
     */
    GdkColor getColor(unsigned int pixel);
    
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
    AW_awar *awar_pointer(const char *var_name, void *default_value = NULL,     AW_default default_file = AW_ROOT_DEFAULT);

    AW_awar *label_is_awar(const char *label); // returns awar, if label refers to one (used by buttons, etc.)

    void unlink_awars_from_DB(GBDATA *gb_main);     // use before calling GB_close for 'gb_main', if you have AWARs in DB

    AW_default check_properties(AW_default aw_props);

    GB_ERROR save_properties(const char *filename = NULL) __ATTR__USERESULT;

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
    void register_widget(GtkWidget* w, AW_active mask);

    void track_action(const char *action_id) { tracker->track_action(action_id); }
    void track_awar_change(AW_awar *changed_awar) { tracker->track_awar_change(changed_awar); }

    bool is_tracking() const { return tracker->is_tracking(); }
    UserActionTracker *get_tracker() { return tracker; }

    void define_remote_command(AW_cb_struct *cbs);
    AW_cb_struct *search_remote_command(const char *action);

#if defined(DEBUG)
    size_t callallcallbacks(int mode);
#endif // DEBUG
};

const char *AW_font_2_ascii(AW_font font_nr);
int         AW_font_2_xfig(AW_font font_nr);

bool ARB_global_awars_initialized();

void ARB_declare_global_awars(AW_root *aw_root, AW_default aw_def);
GB_ERROR ARB_bind_global_awars(GBDATA *gb_main) __ATTR__USERESULT;

__ATTR__USERESULT_TODO inline GB_ERROR ARB_init_global_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    ARB_declare_global_awars(aw_root, aw_def);
    return ARB_bind_global_awars(gb_main);
}

inline AW_default get_AW_ROOT_DEFAULT() { return AW_root::SINGLETON->check_properties(NULL); }

void AW_system(AW_window *aww, const char *command, const char *auto_help_file);

#else
//#error aw_root.hxx included twice
#endif


