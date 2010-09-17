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

int  aw_question  (const char *msg, const char *buttons, bool fixedSizeButtons = true, const char *helpfile = 0);
bool aw_ask_sure  (const char *msg, bool fixedSizeButtons = true, const char *helpfile = 0);
void aw_popup_ok  (const char *msg, bool fixedSizeButtons = true, const char *helpfile = 0);
void aw_popup_exit(const char *msg, bool fixedSizeButtons = true, const char *helpfile = 0) __ATTR__NORETURN;

// asynchronous messages:
extern char AW_ERROR_BUFFER[1024];

void aw_set_local_message(); // no message window, AWAR_ERROR_MESSAGES instead (used by EDIT4)

void aw_errorbuffer_message() __ATTR__DEPRECATED;               // prints AW_ERROR_BUFFER
void aw_macro_message(const char *temp, ...) __ATTR__FORMAT(1); // gives control to the user

// Read a string from the user :
char *aw_input(const char *title, const char *prompt, const char *default_input);
char *aw_input(const char *prompt, const char *default_input);
inline char *aw_input(const char *prompt) { return aw_input(prompt, NULL); }

char *aw_input2awar(const char *title, const char *prompt, const char *awar_value);
char *aw_input2awar(const char *prompt, const char *awar_value);

char *aw_string_selection     (const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*));
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*));

int aw_string_selection_button();   // returns index of last selected button (destroyed by aw_string_selection and aw_input)

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix);

void AW_ERROR(const char *templat, ...) __ATTR__FORMAT(1);

void aw_error(const char *text, const char *text2);     // internal error: asks for core

class  AW_root_Motif;
class  AW_awar;
struct AW_buttons_struct;
class  AW_root_cblist;

typedef enum {
    NO_EVENT     = 0,
    KEY_PRESSED  = 2,
    KEY_RELEASED = 3
} AW_ProcessEventType;

class AW_root {
    AW_default         application_database;
    AW_buttons_struct *button_sens_list;

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
    GB_HASH       *hash_table_for_variables;
    bool           variable_set_by_toggle_field;
    int            number_of_toggle_fields;
    int            number_of_option_menus;
    char          *program_name;

    void            *get_aw_var_struct(char *awar);
    void            *get_aw_var_struct_no_error(char *awar);

    bool            disable_callbacks;
    AW_root_cblist *focus_callback_list;

    int  active_windows;
    void window_show();         // a window is set to screen
    void window_hide();

    // the read only public section:
    short       font_width;
    short       font_height;
    short       font_ascent;
    GB_HASH    *hash_for_windows;

    // the real public section:
    AW_root(const char *properties, const char *program, bool no_exit);
    ~AW_root();

    enum { AW_MONO_COLOR, AW_RGB_COLOR }    color_mode;

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

    void set_focus_callback(AW_RCB fcb, AW_CL cd1, AW_CL cd2); /* any focus callback in any window */

    AW_awar *awar(const char *awar);
    AW_awar *awar_no_error(const char *awar);

    AW_awar *awar_string (const char *var_name, const char *default_value = "", AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_int    (const char *var_name, long default_value = 0,         AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_float  (const char *var_name, float default_value = 0.0,      AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_pointer(const char *var_name, void *default_value = NULL,     AW_default default_file = AW_ROOT_DEFAULT);

    AW_awar *label_is_awar(const char *label); // returns awar, if label refers to one (used by buttons, etc.)

    void unlink_awars_from_DB(GBDATA *gb_main);     // use before calling GB_close for 'gb_main', if you have AWARs in DB

    static const char *property_DB_fullname(const char *default_name);
    static bool        property_DB_exists(const char *default_name);

    AW_default check_properties(AW_default aw_props) {
        return aw_props ? aw_props : application_database;
    }

    GB_ERROR save_properties(const char *filename = NULL) __ATTR__USERESULT;

    AW_default get_gbdata(const char *varname) __ATTR__DEPRECATED; // replace by awar + gb_var


    // Control sensitivity of buttons etc.:
    void apply_sensitivity(AW_active mask);
    void make_sensitive(Widget w, AW_active mask);
    bool remove_button_from_sens_list(Widget button);

    GB_ERROR start_macro_recording(const char *file, const char *application_id, const char *stop_action_name);
    GB_ERROR stop_macro_recording();
    GB_ERROR execute_macro(const char *file);
    void     stop_execute_macro(); // Starts macro window main loop, delayed return
    GB_ERROR enable_execute_macro(FILE *mfile, const char *mname); // leave macro window main loop, returns stop_execute_macro

    void define_remote_command(struct AW_cb_struct *cbs);
    GB_ERROR check_for_remote_command(AW_default gb_main, const char *rm_base);

    // --------------
    //      Fonts

    const char *font_2_ascii(AW_font font_nr);
    int         font_2_xfig(AW_font font_nr);

#if defined(DEBUG)
    size_t callallcallbacks(int mode);
#endif // DEBUG
};


bool ARB_global_awars_initialized();
GB_ERROR ARB_init_global_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) __ATTR__USERESULT;

// #define AW_ROOT_DEFAULT AW_root::SINGLETON->check_properties(NULL)
inline AW_default get_AW_ROOT_DEFAULT() {
    return AW_root::SINGLETON->check_properties(NULL);
}


#else
#error aw_root.hxx included twice
#endif
