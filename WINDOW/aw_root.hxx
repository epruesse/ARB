#ifndef AW_ROOT_HXX
#define AW_ROOT_HXX

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef aw_assert
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define aw_assert(bed) arb_assert(bed)
#endif

#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif


#define AW_ROOT_DEFAULT (aw_main_root_default)
class        AW_root;
class        AW_window;
typedef long AW_CL;             // generic client data type (void *)

typedef void (*AW_RCB)(AW_root*,AW_CL,AW_CL);
typedef void (*AW_RCB0)(AW_root*);
typedef void (*AW_RCB1)(AW_root*,AW_CL);
typedef void (*AW_RCB2)(AW_root*,AW_CL,AW_CL);
typedef AW_window *(*AW_PPP)(AW_root*,AW_CL,AW_CL);

typedef const char *AWAR;
typedef long        AW_bitset;
typedef double      AW_pos;
typedef float       AW_grey_level;                  // <0 dont fill  0.0 white 1.0 black
typedef GBDATA     *AW_default;
typedef AW_bitset   AW_active;                      // bits to activate/inactivate buttons
typedef int         AW_font;

typedef const char             *GB_ERROR;
typedef struct gbs_hash_struct  GB_HASH;

extern AW_default aw_main_root_default;

typedef struct _WidgetRec *Widget;

// #define AWUSE(variable) variable = variable
#if defined(DEBUG) && defined(DEVEL_RALF) && 0
#define AWUSE(variable) (void)variable; int DONT_USE_AWUSE_FOR_##variable
#else
#define AWUSE(variable) (void)variable
#endif // DEBUG
// AWUSE is a obsolete way to get rid of unused-warnings. Will be removed in the future - do not use!
// If your warning is about a parameter, skip the parameters name.
// If your warning is about a variable, the variable is superfluous and should most likely be removed.


#if defined(DEBUG)
#define legal_mask(m) (((m)&AWM_ALL) == (m))
#endif // DEBUG

typedef enum {
    AW_NONE             = 0,
    AW_BIT              = 1,
    AW_BYTE             = 2,
    AW_INT              = 3,
    AW_FLOAT            = 4,
    AW_BITS             = 6,
    AW_BYTES            = 8,
    AW_INTS             = 9,
    AW_FLOATS           = 10,
    AW_STRING           = 12,
    AW_SYTRING_SHORT        = 13, // unused!
    AW_DB               = 15,
    AW_TYPE_MAX         = 16
} AW_VARIABLE_TYPE;     // identical to GBDATA types

typedef struct {
    int t, b, l, r;
} AW_rectangle;

typedef struct {
    AW_pos t, b, l, r;
} AW_world;

typedef char *AW_error;

int  aw_question  (const char *msg, const char *buttons, bool fixedSizeButtons = true, const char *helpfile = 0);
bool aw_ask_sure  (const char *msg, bool fixedSizeButtons = true, const char *helpfile = 0);
void aw_popup_ok  (const char *msg, bool fixedSizeButtons = true, const char *helpfile = 0);
void aw_popup_exit(const char *msg, bool fixedSizeButtons = true, const char *helpfile = 0) __ATTR__NORETURN;

// asynchronous messages:
extern char AW_ERROR_BUFFER[1024];

void aw_set_local_message();                                           // no message window, AWAR_ERROR_MESSAGES instead
void aw_message(const char *msg);
void aw_message();                                                     // prints AW_ERROR_BUFFER;
void aw_macro_message(const char *temp, ...) __ATTR__FORMAT(1);        // gives control to the user

// Read a string from the user :
char *aw_input(const char *title, const char *prompt, const char *default_input);
char *aw_input(const char *prompt, const char *default_input);
inline char *aw_input(const char *prompt) { return aw_input(prompt, NULL); }

char *aw_input2awar(const char *title, const char *prompt, const char *awar_value);
char *aw_input2awar(const char *prompt, const char *awar_value);

char *aw_string_selection     (const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*));
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*));

int aw_string_selection_button();   // returns index of last selected button (destroyed by aw_string_selection and aw_input)

void AW_ERROR(const char *templat, ...) __ATTR__FORMAT(1);

void aw_initstatus( void );     // call this function only once as early as possible
void aw_openstatus( const char *title ); // show status
void aw_closestatus( void );    // hide status

int aw_status( const char *text );      // return 1 if exit button is pressed + set statustext

#ifdef __cplusplus
extern "C" {
#endif

    int aw_status( double gauge );  // return 1 if exit button is pressed + set statusslider

#ifdef __cplusplus
}
#endif

int aw_status( void );      // return 1 if exit button is pressed

void aw_error( const char *text, const char *text2 );   // internal error: asks for core

class  AW_root_Motif;
class  AW_awar;
struct AW_var_callback;
struct AW_xfig_vectorfont;

typedef enum  {
    NO_EVENT     = 0,
    KEY_PRESSED  = 2,
    KEY_RELEASED = 3
} AW_ProcessEventType;

class AW_root {
public:
    static  AW_root *THIS;
    AW_root_Motif   *prvt;                          // Do not use !!!
    bool             value_changed;
    long             changer_of_variable;
    int              y_correction_for_input_labels;
    AW_active        global_mask;
    GB_HASH         *hash_table_for_variables;
    bool             variable_set_by_toggle_field;
    int              number_of_toggle_fields;
    int              number_of_option_menues;
    char            *program_name;

    void            *get_aw_var_struct(char *awar);
    void            *get_aw_var_struct_no_error(char *awar);

    bool                    disable_callbacks;
    struct AW_var_callback *focus_callback_list;

    int  active_windows;
    void window_show();         // a window is set to screen
    void window_hide();

    /********************* the read only  public section ***********************/
    AW_default  application_database;
    short       font_width;
    short       font_height;
    short       font_ascent;
    GB_HASH    *hash_for_windows;

    /* PJ - vectorfont stuff */
    float  vectorfont_userscale; // user scaling
    char  *vectorfont_name;     // name of font
    int    vectorfont_zoomtext; // zoomtext calls: 0 = Xfont, 1 = vectorfont
    
    AW_xfig_vectorfont *vectorfont_lines; // graphic data of the font

    /************************* the real public section *************************/

    AW_root();
    ~AW_root();
    enum { AW_MONO_COLOR, AW_RGB_COLOR }    color_mode;

    void init_variables( AW_default database );
    void init_root( const char *programmname , bool no_exit);
    void main_loop(void);
    void process_events(void); // might block
    void process_pending_events(void); // non-blocking
    AW_ProcessEventType peek_key_event(AW_window *);

    void add_timed_callback               (int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2);
    void add_timed_callback_never_disabled(int ms, AW_RCB2 f, AW_CL cd1, AW_CL cd2);

    void add_timed_callback               (int ms, AW_RCB1 f, AW_CL cd1) { add_timed_callback               (ms, (AW_RCB2)f, cd1, 0); }
    void add_timed_callback_never_disabled(int ms, AW_RCB1 f, AW_CL cd1) { add_timed_callback_never_disabled(ms, (AW_RCB2)f, cd1, 0); }

    void add_timed_callback               (int ms, AW_RCB0 f) { add_timed_callback               (ms, (AW_RCB2)f, 0, 0); }
    void add_timed_callback_never_disabled(int ms, AW_RCB0 f) { add_timed_callback_never_disabled(ms, (AW_RCB2)f, 0, 0); }

    void set_focus_callback(void(*f)(class AW_root*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2); /* any focus callback in any window */

    AW_awar *awar(const char *awar);
    AW_awar *awar_no_error(const char *awar);

    AW_awar *awar_string(const char *var_name, const char *default_value = "", AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_int   (const char *var_name, long default_value = 0,         AW_default default_file = AW_ROOT_DEFAULT);
    AW_awar *awar_float (const char *var_name, float default_value = 0.0,      AW_default default_file = AW_ROOT_DEFAULT);

    AW_awar *label_is_awar(const char *label); // returns awar, if label refers to one (used by buttons, etc.)

    void unlink_awars_from_DB(GBDATA *gb_main);     // use before calling GB_close for 'gb_main', if you have AWARs in DB

    AW_default  open_default(const char *default_name, bool create_if_missing = true);
    AW_error   *save_default( const char *awar_name );
    AW_error   *save_default( const char *awar_name, const char *file_name );
    AW_error   *save_default(AW_default aw_default, const char *file_name);
    AW_default  get_default(const char *varname);
    AW_default  get_gbdata(const char *varname);

    // ************** Set and clear sensitivity of buttons and menus  *********
    void apply_sensitivity(AW_active mask);
    void make_sensitive(Widget w, AW_active mask);

    GB_ERROR start_macro_recording(const char *file,const char *application_id, const char *stop_action_name);
    GB_ERROR stop_macro_recording();
    GB_ERROR execute_macro(const char *file);
    void     stop_execute_macro(); // Starts macro window main loop, delayed return
    GB_ERROR enable_execute_macro(FILE *mfile,const char *mname); // leave macro window main loop, returns stop_execute_macro

    void define_remote_command(struct AW_cb_struct *cbs);
    GB_ERROR check_for_remote_command(AW_default gb_main,const char *rm_base);

    /*************************************************************************
                                          Fonts
    *************************************************************************/
    const char  *font_2_ascii(AW_font font_nr); // converts fontnr to string
    // returns 0 if font is not available
    int font_2_xfig(AW_font font_nr);   // converts fontnr to xfigid
    // negative values indicate monospaced f.

#if defined(DEBUG)
    size_t callallcallbacks(int mode);
#endif // DEBUG
};


/*************************************************************************
                AWARS
*************************************************************************/
struct AW_var_callback;
struct AW_var_target;

typedef void (*Awar_CB)(AW_root *, AW_CL, AW_CL);
typedef void (*Awar_CB2)(AW_root *, AW_CL, AW_CL);
typedef void (*Awar_CB1)(AW_root *, AW_CL);
typedef void (*Awar_CB0)(AW_root *);

typedef struct gb_data_base_type GBDATA;

class AW_awar {
    struct {
        struct {
            float min;
            float max;
        } f;
        const char *srt;
    } pp;

    struct AW_var_callback *callback_list;
    struct AW_var_target   *target_list;

#if defined(DEBUG)
    bool is_global;
#endif // DEBUG

    void remove_all_callbacks();
    void remove_all_target_vars();
    bool unlink_from_DB(GBDATA *gb_main);

    friend long AW_unlink_awar_from_DB(const char *key, long cl_awar, void *cl_gb_main);
    friend void AW_var_gbdata_callback_delete_intern(GBDATA *gbd, int *cl);

public:
    // read only
    class AW_root *root;

    GBDATA *gb_var;                                 // if unmapped, points to same DB elem as 'gb_origin'
    GBDATA *gb_origin;                              // this is set ONCE on creation of awar

    // read only

    AW_VARIABLE_TYPE  variable_type;                // type of the awar
    char             *awar_name;                    // name of the awar

    void run_callbacks();
    void update_target(AW_var_target*pntr);
    void update_targets();
    
    AW_awar( AW_VARIABLE_TYPE var_type, const char *var_name, const char *var_value, double var_double_value, AW_default default_file, AW_root *root );

    AW_awar *add_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2);
    AW_awar *add_callback(Awar_CB1 f, AW_CL cd1);
    AW_awar *add_callback(Awar_CB0 f);

    AW_awar *remove_callback( Awar_CB2 f, AW_CL cd1, AW_CL cd2 ); // remove a callback
    AW_awar *remove_callback( Awar_CB1 f, AW_CL cd1 );
    AW_awar *remove_callback( Awar_CB0 f);

    AW_awar *add_target_var( char **ppchr);
    AW_awar *add_target_var( long *pint);
    AW_awar *add_target_var( float *pfloat);
    void    update();       // awar has changed

    AW_awar *set_minmax(float min, float max);
    AW_awar *set_srt(const char *srt);

    AW_awar *map(const char *awarn) { return this->map(root->awar(awarn)); }
    AW_awar *map(AW_default dest); /* map to new address */
    AW_awar *map(AW_awar *dest); /* map to new address */
    AW_awar *unmap();           /* map to original address */

    void get(char **p_string )  { freeset(*p_string, read_string()); } /* deletes existing targets !!!*/
    void get(long *p_int )      { *p_int =  (long)read_int(); }
    void get(double *p_double ) { *p_double =  read_float(); }
    void get(float *p_float )   { *p_float = read_float(); }

    AW_VARIABLE_TYPE get_type();

    char       *read_string();
    const char *read_char_pntr();
    char       *read_as_string();
    double      read_float();
    long        read_int();

    GB_ERROR write_string(const char *aw_string);
    GB_ERROR write_as_string(const char *aw_string);
    GB_ERROR write_int(long aw_int);
    GB_ERROR write_float(double aw_double);
    GB_ERROR write_as(char *aw_value) { return write_as_string(aw_value);};

    // same as write_-versions above, but always touches the database field
    GB_ERROR rewrite_string(const char *aw_string);
    GB_ERROR rewrite_as_string(const char *aw_string);
    GB_ERROR rewrite_int(long aw_int);
    GB_ERROR rewrite_float(double aw_double);
    GB_ERROR rewrite_as(char *aw_value) { return rewrite_as_string(aw_value);};

    GB_ERROR toggle_toggle();   /* switches between 1/0 */
    void     touch(void);

    GB_ERROR make_global() __ATTR__USERESULT;       // should be used by ARB_init_global_awars only
};

bool ARB_global_awars_initialized();
GB_ERROR ARB_init_global_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) __ATTR__USERESULT;

// ----------------------------------
//      class Awar_Callback_Info
// ----------------------------------

class Awar_Callback_Info {
    // this structure is used to store all information on an awar callback
    // and can be used to remove or remap awar callback w/o knowing anything else

    AW_root *awr;
    Awar_CB  callback;
    AW_CL    cd1, cd2;
    char    *awar_name;
    char    *org_awar_name;

    void init (AW_root *awr_, const char *awar_name_, Awar_CB2 callback_, AW_CL cd1_, AW_CL cd2_);

public:
    Awar_Callback_Info(AW_root *awr_, const char *awar_name_, Awar_CB2 callback_, AW_CL cd1_, AW_CL cd2_)   { init(awr_, awar_name_, callback_, cd1_, cd2_); }
    Awar_Callback_Info(AW_root *awr_, const char *awar_name_, Awar_CB1 callback_, AW_CL cd1_)               { init(awr_, awar_name_, (Awar_CB2)callback_, cd1_, 0); }
    Awar_Callback_Info(AW_root *awr_, const char *awar_name_, Awar_CB0 callback_)                           { init(awr_, awar_name_, (Awar_CB2)callback_, 0, 0); }

    ~Awar_Callback_Info() {
        delete awar_name;
        delete org_awar_name;
    }

    void add_callback() { awr->awar(awar_name)->add_callback(callback, cd1, cd2); }
    void remove_callback() { awr->awar(awar_name)->remove_callback(callback, cd1, cd2); }

    void remap(const char *new_awar);

    const char *get_awar_name() const { return awar_name; }
    const char *get_org_awar_name() const { return org_awar_name; }
    AW_root *get_root() { return awr; }
};

#else
#error aw_root.hxx included twice
#endif
