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

#include "aw_root_gtk.hxx"
#include <string>

#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

typedef void (*AW_RCB0)(AW_root*);
typedef void (*AW_RCB1)(AW_root*, AW_CL);
typedef AW_RCB AW_RCB2;

typedef AW_window *(*AW_PPP)(AW_root*, AW_CL, AW_CL);

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

char *aw_input2awar(const char *prompt, const char *awar_value);

char *aw_string_selection     (const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*));
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*));

int aw_string_selection_button();   // returns index of last selected button (destroyed by aw_string_selection and aw_input)

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix);


class  AW_awar;
struct AW_buttons_struct;
class  AW_root_cblist;
class  GB_HASH;

enum AW_ProcessEventType {
    NO_EVENT     = 0,
    KEY_PRESSED  = 2,
    KEY_RELEASED = 3
};

void aw_initstatus(); 

class AW_root : virtual Noncopyable {

    // gtk dependent attributes are defined in a different header to keep this header portable
    AW_root_gtk prvt; /*< Contains all gtk dependent attributes. */
    GB_HASH     *action_hash; /*< Is used to buffer and replay remote actions. */
    AW_default  application_database; /*< FIXME */
    bool        no_exit; /*< FIXME no idea what this is used for*/
    char        *program_name;


//    AW_buttons_struct *button_sens_list;

    /**
     * FIXME
     */
    void init_root(const char *programname, bool no_exit);

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

//    void exit_variables();



//    void exit_root();


public:
    static AW_root *SINGLETON;


    bool           value_changed;
    Widget         changer_of_variable;
    int            y_correction_for_input_labels;
    AW_active      global_mask;
    bool           focus_follows_mouse;
    GB_HASH       *hash_table_for_variables;
    bool           variable_set_by_toggle_field;
    int            number_of_toggle_fields;
    int            number_of_option_menus;


    bool            disable_callbacks;
    AW_window      *current_modal_window;
    AW_root_cblist *focus_callback_list;

    int  active_windows;
    void window_show();         // a window is set to screen
    void window_hide(AW_window *aww);

    // the read only public section:
    short       font_width;
    short       font_height;
    short       font_ascent;
    GB_HASH    *hash_for_windows;

    // the real public section:


    /**
     * FIXME
     */
    AW_root(const char *properties, const char *program, bool no_exit);
#if defined(UNIT_TESTS)
    AW_root(const char *properties); // fake-root for unit-tests (allows access to awar-subsystem)
#endif
    ~AW_root();

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

    void set_focus_callback(AW_RCB fcb, AW_CL cd1, AW_CL cd2); // any focus callback in any window
    bool is_focus_callback(AW_RCB fcb) const;

    AW_awar *awar(const char *awar);
    AW_awar *awar_no_error(const char *awar);

    void dont_save_awars_with_default_value(GBDATA *gb_main);

    AW_awar *awar_string (const char *var_name, const char *default_value = "", AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_int    (const char *var_name, long default_value = 0,         AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_float  (const char *var_name, float default_value = 0.0,      AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_pointer(const char *var_name, void *default_value = NULL,     AW_default default_file = AW_ROOT_DEFAULT);

    AW_awar *label_is_awar(const char *label); // returns awar, if label refers to one (used by buttons, etc.)

    void unlink_awars_from_DB(GBDATA *gb_main);     // use before calling GB_close for 'gb_main', if you have AWARs in DB

    AW_default check_properties(AW_default aw_props);

    GB_ERROR save_properties(const char *filename = NULL) __ATTR__USERESULT;

    // Control sensitivity of buttons etc.:
    void apply_sensitivity(AW_active mask);
    void apply_focus_policy(bool follow_mouse);
    void make_sensitive(Widget w, AW_active mask);
    bool remove_button_from_sens_list(Widget button);

    GB_ERROR start_macro_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing);
    GB_ERROR stop_macro_recording();
    bool is_recording_macro() const;
    GB_ERROR execute_macro(GBDATA *gb_main, const char *file, AW_RCB1 execution_done_cb, AW_CL client_data);

    void define_remote_command(struct AW_cb_struct *cbs);
    GB_ERROR check_for_remote_command(AW_default gb_main, const char *rm_base);

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
#error aw_root.hxx included twice
#endif


