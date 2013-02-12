#pragma once

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H 
#include <arb_assert.h>
#endif
#ifndef AW_KEYSYM_HXX
#include "aw_keysym.hxx"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#include "aw_at.hxx"
#include "aw_gtk_forward_declarations.hxx"
#include "aw_area_management.hxx"
#include "aw_help.hxx"

//the following types are not forward declared because ARB needs to know about them
//and I dont want to change includes in ARB
#include "aw_device_print.hxx" 
#include "aw_device_click.hxx"
#include "aw_device_size.hxx"
#include "aw_xfig.hxx"

class AW_window;
class AW_xfig;
class AW_device;
class AW_screen_area;
struct GB_HASH;

// --------------------------------------------------------------------------------

#define AW_POPUP  ((AW_CB)(-1))
// AW_POPDOWN is defined later in this section

#define AW_MESSAGE_TIME 2000
#define AW_HEADER_MAIN

// ======= Used in Tune background function =================================
#define TUNE_BUTTON    8
#define TUNE_INPUT     (-TUNE_BUTTON)
#define TUNE_SUBMENU   0
#define TUNE_MENUTOPIC (-12)
#define TUNE_BRIGHT    (256+30)
#define TUNE_DARK      (-TUNE_BRIGHT)
// ==========================================================================

#ifndef AW_AT_HXX
class AW_at;
#endif

enum AW_orientation { AW_HORIZONTAL, AW_VERTICAL };


class AW_at_maxsize {
    int maxx;
    int maxy;

public:
    AW_at_maxsize()
        : maxx(0),
          maxy(0)
    {}
    
    void store(const AW_at &at);
    void restore(AW_at &at) const;
};

class AW_at_auto {
    enum { INC, SPACE, OFF } type;
    int x, y;
    int xfn, xfnb, yfnb, bhob;
public:
    AW_at_auto() : type(OFF) {}

    void store(const AW_at &at);
    void restore(AW_at &at) const;
};

typedef const char *AW_label;       // label for buttons menus etc
// "fsdf" simple label  // no '/' symbol !!!
// "awarname/asdf"  // awar name (any '/' in string)
// "#file.bitmap"   // bitmap in $ARBHOME/lib/pixmaps/file.bitmap


enum AW_event_type {
    AW_Keyboard_Press   = 1,
    AW_Keyboard_Release = 2,
    AW_Mouse_Press      = 3,
    AW_Mouse_Release    = 4,
    AW_Mouse_Drag       = 5
};

enum AW_MouseButton {
    AW_BUTTON_NONE   = 0,
    AW_BUTTON_LEFT   = 1,
    AW_BUTTON_MIDDLE = 2,
    AW_BUTTON_RIGHT  = 3,
    AW_WHEEL_UP      = 4,
    AW_WHEEL_DOWN    = 5,
};

struct AW_event {
    // fields always valid
    AW_event_type type;             // AW_Keyboard or AW_Mouse
    unsigned long time;             // time in msec, when event occurred
    AW_key_mod    keymodifier;      // control, alt/meta (shift only for type == AW_Mouse)

    // fields valid for type == AW_Mouse
    AW_MouseButton button;
    int            x, y;            // pointer x,y coordinates in the event window

    // fields valid for type == AW_Keyboard
    AW_key_code keycode;            // which key type was pressed
    char        character;          // the c character
};

void AW_POPDOWN(AW_window *);


/**
 * Switches the window into help mode.
 * The next button click will open the corresponding help entry.
 * @param 
 */
void AW_help_entry_pressed(AW_window *);
void AW_clock_cursor(AW_root *);
void AW_normal_cursor(AW_root *);

void AW_openURL(AW_root *aw_root, const char *url);

typedef void (*AW_cb_struct_guard)();


struct AW_buttons_struct : virtual Noncopyable {
    AW_buttons_struct(AW_active maski, GtkWidget* w, AW_buttons_struct *next);
    ~AW_buttons_struct();

    AW_active          mask;
    GtkWidget*         button;
    AW_buttons_struct *next;
};


/**
 * A list of callback functions.
 */
class AW_cb_struct {
    AW_CL         cd1; //callback parameter 2
    AW_CL         cd2; //callback parameter 3
    AW_cb_struct *next;

    static AW_cb_struct_guard guard_before;
    static AW_cb_struct_guard guard_after;
    static AW_postcb_cb       postcb; // called after each cb triggered via interface

public:
    // private (read-only):
    AW_window  *pop_up_window; //callback parameter 1
    AW_CB       f; //the callback function
    AW_window  *aw;
    const char *help_text;
    char       *id;

    // real public section:
    AW_cb_struct(AW_window    *awi,
                 AW_CB         g,
                 AW_CL         cd1i       = 0,
                 AW_CL         cd2i       = 0,
                 const char   *help_texti = 0,
                 AW_cb_struct *next       = 0);

    void run_callback();                            // runs the whole list
    bool contains(AW_CB g);                         // test if contained in list
    bool is_equal(const AW_cb_struct& other) const;

#if defined(ASSERTION_USED)
    AW_CL get_cd1() const { return cd1; }
    AW_CL get_cd2() const { return cd2; }
#endif // ASSERTION_USED

    static void set_AW_cb_guards(AW_cb_struct_guard before, AW_cb_struct_guard after) {
        guard_before = before;
        guard_after  = after;
    }
    static void set_AW_postcb_cb(AW_postcb_cb postcb_cb) {
        postcb = postcb_cb;
    }

    static void useraction_init() {
        if (guard_before) guard_before();
    }
    static void useraction_done(AW_window *aw) {
        if (guard_after) guard_after();
        if (postcb) postcb(aw);
    }
};


enum {
    AWM_DISABLED = 0,           // disabled items (used for dynamically dis-/enabled items)
    AWM_BASIC    = 1,
    AWM_EXP      = 2,
    AWM_ALL      = AWM_BASIC|AWM_EXP
};

enum {
    AWM_MASK_UNKNOWN = AWM_DISABLED,
    AWM_MASK_DEFAULT = AWM_BASIC,
    AWM_MASK_EXPERT  = AWM_ALL
};

typedef char *AW_pixmap;

class  AW_window_Motif;
struct AW_selection_list_entry;
class  AW_selection_list;
struct AW_option_menu_struct;
struct aw_toggle_data;

enum AW_SizeRecalc {
    AW_KEEP_SIZE      = 0,                          // do not resize
    AW_RESIZE_DEFAULT = 1,                          // do resize to default size
    AW_RESIZE_USER    = 2,                          // do resize to user size (or default size if that is bigger)
    AW_RESIZE_ANY     = 3                           // keep AW_RESIZE_USER or set AW_RESIZE_DEFAULT
};

enum AW_PosRecalc {
    AW_KEEP_POS            = 0,                     // do not change position on show
    AW_REPOS_TO_CENTER     = 1,                     // center the window on show (unused atm)
    AW_REPOS_TO_MOUSE      = 2,                     // move the window under the current mouse position
    AW_REPOS_TO_MOUSE_ONCE = 3,                     // like AW_REPOS_TO_MOUSE, but only done once!
};

typedef void (*aw_hide_cb)(AW_window *aww);

class AW_window : virtual Noncopyable {
    friend class AW_cb_struct;//the cb struct needs access to prvt for strange reasons
    enum AW_SizeRecalc recalc_size_at_show;
//    enum AW_PosRecalc  recalc_pos_at_show;
//    aw_hide_cb         hide_cb;
//
//    bool expose_callback_added;
//
//    AW_cb_struct *focus_cb;
//
//    int left_indent_of_horizontal_scrollbar;
//    int top_indent_of_vertical_scrollbar;

//    void all_menus_created() const;
//    void create_toggle(const char *var_name, aw_toggle_data *tdata);

protected:
    AW_root *root;
    class AW_window_gtk;
    AW_window_gtk* prvt; /*< Contains all gtk dependent attributes */



    void create_devices();
    void set_background(const char *colorname, GtkWidget* w);

    void wm_activate();                                // un-minimize window and give it the focus (use show_and_activate())
    
    
public:

    // ************ This is not the public section *************
    //FIXME move aw_at into pimpl
    //FIXME make _at private. Right now some global functions want to access it. Remove those global functions.
    AW_at _at; /** < Defines the next position at which something will be inserted into the window.  */
    AW_cb_struct    *_d_callback; //FIXME what is this?

    AW_window();
    virtual ~AW_window();


    AW_event       event;
    unsigned long  click_time;
    long           color_table_size;
    AW_rgb        *color_table;

    int number_of_timed_title_changes;

    AW_xfig  *xfig_data;

    void create_window_variables();

    void recalc_pos_atShow(AW_PosRecalc pr);

    AW_PosRecalc get_recalc_pos_atShow() const;

    void recalc_size_atShow(enum AW_SizeRecalc sr);

    void run_focus_callback();
    
    void allow_delete_window(bool allow_close);
    void on_hide(aw_hide_cb call_on_hide);

    /**
     * @return The current xfig data or NULL.
     */
    AW_xfig* get_xfig_data();

    void show_modal();
    void set_window_title_intern(char *title);

    int calculate_string_width(int columns) const;
    int calculate_string_height(int columns, int offset) const;
    char *align_string(const char *string, int columns);

    void update_label(GtkWidget* widget, const char *var_value);
    void update_toggle(GtkWidget* widget, const char *var_value, AW_CL cd);
    void update_input_field(GtkWidget* widget, const char *var_value);
    void update_text_field(GtkWidget* widget, const char *var_value);

    void  create_invisible(int columns);
    void *_create_option_entry(AW_VARIABLE_TYPE type, const char *name, const char *mnemonic, const char *name_of_color);
    void  refresh_toggle_field(int toggle_field_number); 
    void  _set_activate_callback(GtkWidget *widget);
    void  unset_at_commands();
    void  increment_at_commands(int width, int height);


    AW_color_idx alloc_named_data_color(int colnum, char *colorname);

    void _get_area_size(AW_area area, AW_screen_area *square);
    void get_scrollarea_size(AW_screen_area *square);
    
    int label_widget(void *wgt, AW_label str, char *mnemonic=0, int width = 0, int alignment = 0);

    // ------------------------------
    //      The read only section

    char *window_name;                              // window title
    char *window_defaults_name;
    bool  window_is_shown;


    int slider_pos_vertical; /** < current position of the vertical slider */
    int slider_pos_horizontal;/** < current position of the horizontal slider */
    int main_drag_gc;

    AW_screen_area *picture;      // the result of tell scrolled
    // picture size

    // --------------------------------
    //      The real public section

    AW_root *get_root() { return root; }
    /**
     * 
     * @param index
     * @return The area at index i or NULL of that area does not exist 
     */
    AW_area_management* get_area(int index);
    // ******************* Global layout functions **********************

    void show(); // show newly created window or unhide hidden window (aka closed window)
    void hide(); // hide (don't destroy) a window (<->show)

    void activate() { show(); wm_activate(); }      // make_visible, pop window to front and give it the focus

    bool is_shown() const;  // is window visible (== true) or hidden (== false). ?

    void hide_or_notify(const char *error);

    void    message(char *title, int ms);   // Set for ms milliseconds the title of the window
    void    set_window_title(const char *title);   // Set the window title forever

    const char *get_window_title();       // Get the window's title
    const char *get_window_id() const { return window_defaults_name; } // Get the window's internal name

    const char *local_id(const char *id) const;

    void set_info_area_height(int height);
    void set_bottom_area_height(int height);

    // ******************* Input and Motion Events **********************

    void set_popup_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_focus_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    bool is_focus_callback(void (*f)(AW_window*, AW_CL, AW_CL));

    void set_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_input_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_motion_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_double_click_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0);

    bool is_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_input_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_motion_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_double_click_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL));

    void get_event(AW_event *eventi) const;       // In an event callback get the events info

    void force_expose(); // forces the window to expose instantly

    // ******************* Get the devices **********************
    AW_device *get_device(AW_area area);
    AW_device_click *get_click_device(AW_area area, int mousex, int mousey, AW_pos max_distance_linei,
                                      AW_pos max_distance_texti, AW_pos radi);
    AW_device_size *get_size_device(AW_area area);
    AW_device_print *get_print_device(AW_area area);

    // ************** Create the menu buttons *********
    
    /**
     * Creates a new top level menu.
     * @param name Name of the menu.
     * @param mnemonic FIXME
     * @param mask FIXME
     */
    void create_menu(AW_label name, const char *mnemonic, AW_active mask = AWM_ALL);
    /**
     * Insert a sub menu into the last created menu.
     * @param name Name of the sub menu.
     * @param mnemonic FIXME
     * @param mask FIXME
     */
    void insert_sub_menu(AW_label name, const char *mnemonic, AW_active mask = AWM_ALL);
    /**
     * Insert a menu item into the last created menu or sub menu.
     * @param id FIXME
     * @param name Name of the item.
     * @param mnemonic FIXME
     * @param help_text_ FIXME
     * @param mask FIXME
     * @param cb Callback that should be called when the item is activated.
     * @param cd1 Callback parameter 1.
     * @param cd2 Callback parameter 2.
     */
    void insert_menu_topic(const char *id, AW_label name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB cb, AW_CL cd1, AW_CL cd2);
    void insert_menu_topic(const char *id, AW_label name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB1 cb, AW_CL cd1) { insert_menu_topic(id, name, mnemonic, help_text_, mask, (AW_CB)cb, cd1, 0); }
    void insert_menu_topic(const char *id, AW_label name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB0 cb) { insert_menu_topic(id, name, mnemonic, help_text_, mask, (AW_CB)cb, 0, 0); }
    /**
     * insert a separator into the currently open menu
     */
    void sep______________(); 
    /**
     * Closes the currently open sub menu.
     * If no menu is open this method will crash.
     */
    void close_sub_menu();

    void insert_help_topic(AW_label name, const char *mnemonic, const char *help_text, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);

    // ************** Create modes on the left side ******************
    int create_mode(const char *pixmap, const char *help_text, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void select_mode(int mode);


    // ************** Control the size of the main drawing area + scrollbars  *********
    void tell_scrolled_picture_size(AW_screen_area rectangle);
    void tell_scrolled_picture_size(AW_world rectangle);
    AW_pos get_scrolled_picture_width() const;
    AW_pos get_scrolled_picture_height() const;
    void reset_scrolled_picture_size();

    void calculate_scrollbars();
    void set_vertical_scrollbar_position(int position);
    void set_horizontal_scrollbar_position(int position);
    void set_vertical_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_horizontal_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_horizontal_scrollbar_left_indent(int indent);
    void set_vertical_scrollbar_top_indent(int indent);

    void update_scrollbar_settings_from_awars(AW_orientation orientation);

    void create_user_geometry_awars(int posx, int posy, int width, int height);
    
    // ************** Control window size  *********
    void set_window_size(int width, int height);
    void get_window_size(int& width, int& height);
    void window_fit();                              // Recalculate the size of a window with buttons

    void store_size_in_awars(int width, int height);
    void get_size_from_awars(int& width, int& height);


    // ************** Control window position  *********
    void set_window_frame_pos(int xpos, int ypos);
    void get_window_content_pos(int& xpos, int& ypos);

    void store_pos_in_awars(int xpos, int ypos);
    void get_pos_from_awars(int& xpos, int& ypos);
    
    
    // *****************

    void get_screen_size(int& width, int& height);
    bool get_mouse_pos(int& x, int& y);

    void set_focus_policy(bool follow_mouse);
    
    // ************** ********************************************************************  *********
    // ************** Create buttons: First set modify flags and finally create the button  *********
    // ************** ********************************************************************  *********

    // *** global modifier: ****
    void load_xfig(const char *file, bool resize=true); // Loads the background graphic
    void draw_line(int x1, int y1, int x2, int y2, int width, bool resize); // draws a line on the background

    void label_length(int length);   // Justifies all following labels
    void button_length(int length);   // Sets the width of all following buttons (in chars)
    void button_height(int height);   // Sets the height of all following buttons (in lines)
    int  get_button_length() const; // returns the current width of buttons
    int  get_button_height() const; // returns the current height of buttons
    void highlight();           // Creates a frame around the button
    void auto_increment(int dx, int dy);   // enable automatic placement of buttons
    // dx is the horizontal distance between the left
    // borders of two buttons
    void auto_space(int xspace, int yspace);   // enable automatic placement of buttons
    // xspace is the horizontal space between 2 buttons

    void auto_off();            // disable auto_xxxxx
    void shadow_width (int shadow_thickness); // set the shadow_thickness of buttons


    // *** local modifiers: ********
    void at(int x, int y);      // abs pos of a button (>10,10)
    void at_x(int x);           // abs x pos
    void at_y(int y);           // abs y pos
    void at_shift(int x, int y);   // rel pos of a button
    void at_newline();          // in auto_space mode only: newline

    void at(const char *id);                        /* place the button at the position set in the .fig
                                                     * file (loaded with load_xfig) by the string $id */
    bool at_ifdef(const  char *id);                 // check whether 'id' is an element if the .fig file

    void label(const char *label);   // Create a label before the button

    void get_at_position(int *x, int *y) const;
    int get_at_xposition() const;
    int get_at_yposition() const;

    void dump_at_position(const char *debug_label) const; // for debugging (uses printf)

    void at_attach(bool attach_x, bool attach_y); // attach to X, Y or both
    void at_set_to(bool attach_x, bool attach_y, int xoff, int yoff); // set "to:XY:id" manually
    void at_unset_to();         // unset "to:id" manually
    void at_set_min_size(int xmin, int ymin); // define minimum window size

    void store_at_size_and_attach(AW_at_size *at_size);   // get size of at-element
    void restore_at_size_and_attach(const AW_at_size *at_size);   // set size of a at-element

    void sens_mask(AW_active mask);   // Set the sensitivity mask used for following widgets (Note: reset by next at()-command)
    void help_text(const char *id);  // Set the help text of a button
    void callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2); // normal callbacks
    void callback(void (*f)(AW_window*, AW_CL), AW_CL cd1);
    void callback(void (*f)(AW_window*));
    void callback(AW_cb_struct * /* owner */ awcbs); // Calls f with
    // aww in awcbs
    void d_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2); // double click callbacks
    void d_callback(void (*f)(AW_window*, AW_CL), AW_CL cd1); // selection lists only !!
    void d_callback(void (*f)(AW_window*));
    void d_callback(AW_cb_struct * /* owner */ awcbs); // Calls f with
    // *** create the buttons ********

    void   create_button(const char *macro_name, AW_label label, const char *mnemonic = 0, const char *color = 0); // simple button; shadow only when callback
    void   create_autosize_button(const char *macro_name, AW_label label, const char *mnemonic = 0, unsigned xtraSpace = 1); // as above, but ignores button_length
    GtkWidget* get_last_widget() const;

    void create_toggle(const char *var_name);  // int 0/1  string yes/no   float undef
    void create_inverse_toggle(const char *var_name);  // like create_toggle, but displays inverted toggle value

    void create_toggle(const char *var_name, const char *nobitmap, const char *yesbitmap, int buttonWidth = 0);
    void create_text_toggle(const char *var_name, const char *noText, const char *yesText, int buttonWidth = 0);

    void create_input_field(const char *awar_name, int columns = 0);   // One line textfield
    void create_text_field(const char *awar_name, int columns = 20, int rows = 4);   // Multi line textfield
    // with scrollbars


    // ***** option_menu is a menu where only one selection is visible at a time
    AW_option_menu_struct *create_option_menu(const char *awar_name, AW_label label= NULL, const char *mnemonic= NULL);
    void clear_option_menu(AW_option_menu_struct *oms);  // used to redefine available options

private:
    /**
     * Converts a aw variable type to gtk variable type
     * @param aw_type
     * @return The converted type or G_TYPE_INVALID if the conversion fails.
     */
    GType convert_aw_type_to_gtk_type(AW_VARIABLE_TYPE aw_type);
    
    template <class T>
    void insert_option_internal(AW_label choice_label, const char *mnemonic, T var_value,  const char *name_of_color, bool default_option);

    template <class T>
    void insert_toggle_internal(AW_label toggle_label, const char *mnemonic, T var_value, bool default_toggle);

    
public:

    // for string
    void insert_option         (AW_label choice_label, const char *mnemonic, const char *var_value, const char *name_of_color = 0);  // for string
    void insert_default_option (AW_label choice_label, const char *mnemonic, const char *var_value, const char *name_of_color = 0);
    // for int
    void insert_option         (AW_label choice_label, const char *mnemonic, int var_value,          const char *name_of_color = 0);  // for int
    void insert_default_option (AW_label choice_label, const char *mnemonic, int var_value,          const char *name_of_color = 0);
    // for float
    void insert_option         (AW_label choice_label, const char *mnemonic, float var_value,        const char *name_of_color = 0);  // for float
    void insert_default_option (AW_label choice_label, const char *mnemonic, float var_value,        const char *name_of_color = 0);

    void update_option_menu();
    void refresh_option_menu(AW_option_menu_struct *);  // don't use this


    // ***** toggle_field is a static menu (all items are visible and only one is selected)
    void create_toggle_field(const char *awar_name, AW_label label, const char *mnemonic);
    void create_toggle_field(const char *awar_name, int orientation = 0);  // 1 = horizontal
    // for string
    void insert_toggle(AW_label toggle_label, const char *mnemonic, const char *var_value);
    void insert_default_toggle(AW_label toggle_label, const char *mnemonic, const char *var_value);
    // for int
    void insert_toggle(AW_label toggle_label, const char *mnemonic, int var_value);
    void insert_default_toggle(AW_label toggle_label, const char *mnemonic, int var_value);
    // for float
    void insert_toggle(AW_label toggle_label, const char *mnemonic, float var_value);
    void insert_default_toggle(AW_label toggle_label, const char *mnemonic, float var_value);
    void update_toggle_field();

    // ***** selection list is a redefinable scrolled list of items

    AW_selection_list *create_selection_list(const char *awar_name, int columns = 4, int rows = 4);

    // gtk ready made buttons
    void create_font_button(const char *awar_name, AW_label label);
    void create_color_button(const char *awar_name, AW_label label);
};


/**
 * A window with a menu bar on top and a mode selection bar on the left side.
 */
class AW_window_menu_modes : public AW_window { // derived from a Noncopyable
    void *AW_window_menu_modes_private;       // Do not use !!!

public:
    AW_window_menu_modes();
    ~AW_window_menu_modes() OVERRIDE;
    void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
};

// AW_window_menu is the same as AW_window_menu_modes
// except for a line separating the left toolbar from
// the window content. It's only used by DI_view_matrix
// and SaiView anyway. 
typedef AW_window_menu_modes AW_window_menu;

class AW_window_simple_menu : public AW_window {
private:
public:
    AW_window_simple_menu();
    ~AW_window_simple_menu() OVERRIDE;
    void init(AW_root *root, const char *wid, const char *windowname);
};

/**
 * Am simple window that only uses the INFO_AREA.
 */
class AW_window_simple : public AW_window {
private:
public:
    AW_window_simple();
    ~AW_window_simple() OVERRIDE;
    void init(AW_root *root, const char *wid, const char *windowname);
};


class AW_window_message : public AW_window {
private:
public:
    AW_window_message();
    ~AW_window_message() OVERRIDE;
    void init(AW_root *root, const char *windowname, bool allow_close);
};


typedef void* AW_gc_manager;

