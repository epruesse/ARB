#ifndef AW_WINDOW_HXX
#define AW_WINDOW_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef CB_H
#include <cb.h>
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

class AW_window;
class AW_device;
class AW_device_click;
class AW_device_print;
class AW_device_size;
class AW_screen_area;
struct GB_HASH;

#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

// --------------------------------------------------------------------------------

#define AW_POPUP  ((AW_CB)(-1))
// AW_POPDOWN is defined later in this section

#define AW_MESSAGE_TIME 2000
#define AW_HEADER_MAIN  extern "C" { int XtAppInitialize(); } void aw_never_called_main() { XtAppInitialize(); }

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

class AW_at_size {
    int  to_offset_x;                               // here we use offsets (not positions like in AW_at)
    int  to_offset_y;
    bool to_position_exists;

    bool attach_x;           // attach right side to right form
    bool attach_y;
    bool attach_lx;          // attach left side to right form
    bool attach_ly;
    bool attach_any;

public:
    AW_at_size()
        : to_offset_x(0),
          to_offset_y(0),
          to_position_exists(false),
          attach_x(false), 
          attach_y(false), 
          attach_lx(false), 
          attach_ly(false), 
          attach_any(false) 
    {}

    void store(const AW_at *at);
    void restore(AW_at *at) const;
};

class AW_at_maxsize {
    int maxx;
    int maxy;

public:
    AW_at_maxsize()
        : maxx(0),
          maxy(0)
    {}
    
    void store(const AW_at *at);
    void restore(AW_at *at) const;
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

struct AW_event {
    AW_event_type       type;       // AW_Keyboard or AW_Mouse
    unsigned long       time;       // time in msec, when event occurred
    // ***** button fields
    unsigned int        button;     // which mouse button was pressed 1,2,3
    int         x, y;       // pointer x,y coordinates in the event window
    // ****** key fields
    AW_key_mod      keymodifier;    // control alt meta ....
    AW_key_code     keycode;    // which key type was pressed
    char            character;  // the c character
};

void AW_POPDOWN(AW_window *);
void AW_POPUP_HELP(AW_window *, AW_CL /* char */ helpfile);
void AW_help_entry_pressed(AW_window *);
void AW_clock_cursor(AW_root *);
void AW_normal_cursor(AW_root *);

void AW_openURL(AW_root *aw_root, const char *url);

typedef void (*AW_cb_guard)();

class AW_cb : virtual Noncopyable  {
    AW_window      *caller;
    WindowCallback  CB;

    const char *help;
    AW_window  *pop_up_window;
    char       *id;

    AW_cb *prev; 

    static AW_cb_guard guard_before;
    static AW_cb_guard guard_after;

public:
    __ATTR__DEPRECATED_CALLBACK_IN_CTOR AW_cb(AW_window  *caller_, AW_CB cb_, AW_CL cd1_  = 0, AW_CL cd2_  = 0, const char *help_ = 0) 
        : caller(caller_),
          CB(makeWindowCallback(cb_, cd1_, cd2_)), 
          help(help_),
          pop_up_window(NULL),
          id(NULL),
          prev(NULL)
        {} 

    AW_cb(AW_window  *caller_, const WindowCallback& wcb, const char *help_ = 0)
        : caller(caller_),
          CB(wcb), 
          help(help_),
          pop_up_window(NULL),
          id(NULL),
          prev(NULL)
        {}
    ~AW_cb() { free(id); }

    static void append(AW_cb*& head, AW_cb *addcb) {
        aw_assert(!addcb->prev);
        addcb->prev = head;
        head        = addcb;
    }

    void run_callbacks();
    bool contains(AW_CB g); // test whether 'g' is contained in callback-list

    const char *get_id() const { return id; }
    void set_id(char *newId) { freeset(id, newId); }

    bool has_id(const char *test) const { aw_assert(id); return strcmp(test, id) == 0; }
    bool has_id_other_than(const char *test) const { return id && !has_id(test); }

    AW_window *popup_window() const { return pop_up_window; }
    AW_window *caller_window() const { return caller; }
    bool has_callee(AW_CB cb_) const { return CB.callee() == (AW_CL)cb_; }

    const char *get_help() const { return help; }
    void set_help(const char *help_) { help = help_; }

    int callback_cmp(const AW_cb& other) const { return CB<other.CB ? -1 : (other.CB<CB ? 1 : 0); }
    bool has_parameters() const { return CB.inspect_CD1() || CB.inspect_CD2(); }

    static void set_AW_cb_guards(AW_cb_guard before, AW_cb_guard after) {
        guard_before = before;
        guard_after  = after;
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
    AW_REPOS_TO_CENTER     = 1,                     // center the window on show
    AW_REPOS_TO_MOUSE      = 2,                     // move the window under the current mouse position
    AW_REPOS_TO_MOUSE_ONCE = 3,                     // like AW_REPOS_TO_MOUSE, but only done once!
};

typedef void (*aw_hide_cb)(AW_window *aww);

class AW_window : virtual Noncopyable {
    enum AW_SizeRecalc recalc_size_at_show;
    enum AW_PosRecalc  recalc_pos_at_show;
    aw_hide_cb         hide_cb;

    void all_menus_created() const;
    void create_toggle(const char *var_name, aw_toggle_data *tdata);

    void show_internal(void *grab);

protected:
    AW_root *root;

    void create_devices();
    void set_background(const char *colorname, Widget w);

    void wm_activate();                                // un-minimize window and give it the focus (use show_and_activate())

public:

    // ************ This is not the public section *************

    AW_window_Motif *p_w;       // Do not use !!!

    AW_at *_at;
    AW_cb *_callback;
    AW_cb *_d_callback;

    AW_window();
    virtual ~AW_window();


    AW_event       event;
    unsigned long  click_time;
    long           color_table_size;
    AW_rgb        *color_table;

    int number_of_timed_title_changes;

    void /* AW_xfig */   *xfig_data;

    void create_window_variables();

    void recalc_pos_atShow(AW_PosRecalc pr) { recalc_pos_at_show = pr; }
    AW_PosRecalc get_recalc_pos_atShow() const { return recalc_pos_at_show; }

    void recalc_size_atShow(enum AW_SizeRecalc sr) {
        if (sr == AW_RESIZE_ANY) {
            sr = (recalc_size_at_show == AW_RESIZE_USER) ? AW_RESIZE_USER : AW_RESIZE_DEFAULT;
        }
        recalc_size_at_show = sr;
    }

    void allow_delete_window(bool allow_close);
    void on_hide(aw_hide_cb call_on_hide) { hide_cb = call_on_hide; }

    void show_grabbed();
    void set_window_title_intern(char *title);

    int calculate_string_width(int columns) const;
    int calculate_string_height(int columns, int offset) const;
    char *align_string(const char *string, int columns);

    void update_label(Widget widget, const char *var_value);
    void update_toggle(Widget widget, const char *var_value, AW_CL cd);
    void update_input_field(Widget widget, const char *var_value);
    void update_text_field(Widget widget, const char *var_value);

    void  create_invisible(int columns);
    void *_create_option_entry(AW_VARIABLE_TYPE type, const char *name, const char *mnemonic, const char *name_of_color);
    void  update_toggle_field(int toggle_field_number);
    void  update_selection_list_intern(AW_selection_list *selection_list);
    void  _set_activate_callback(void *widget);
    void  unset_at_commands();
    void  increment_at_commands(int width, int height);


    AW_color_idx alloc_named_data_color(int colnum, char *colorname);
    void        _get_area_size(AW_area area, AW_screen_area *square);
    int         label_widget(void *wgt, AW_label str, char *mnemonic=0, int width = 0, int alignment = 0);

    // ------------------------------
    //      The read only section
    // ------------------------------

    char *window_name;                              // window title
    char *window_defaults_name;
    bool  window_is_shown;

    int left_indent_of_horizontal_scrollbar;
    int top_indent_of_vertical_scrollbar;
    int bottom_indent_of_vertical_scrollbar;
    int slider_pos_vertical;
    int slider_pos_horizontal;
    int main_drag_gc;

    AW_screen_area *picture;      // the result of tell scrolled
                                // picture size

    // --------------------------------
    //      The real public section
    // --------------------------------

    AW_root *get_root() { return root; }

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

    void set_expose_callback(AW_area area, const WindowCallback& wcb);
    void set_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0) __ATTR__DEPRECATED_CALLBACK_LATER;
    void set_resize_callback(AW_area area, const WindowCallback& wcb);
    void set_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1=0, AW_CL cd2=0) __ATTR__DEPRECATED_CALLBACK_LATER;
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
    AW_device_click *get_click_device(AW_area area, int mousex, int mousey, int max_distance);
    AW_device_size *get_size_device(AW_area area);
    AW_device_print *get_print_device(AW_area area);

    // ************** Create the menu buttons *********
    void create_menu(AW_label name, const char *mnemonic, AW_active mask = AWM_ALL);
    void insert_sub_menu(AW_label name, const char *mnemonic, AW_active mask = AWM_ALL);
    void insert_menu_topic(const char *topic_id, AW_label name, const char *mnemonic, const char *helpText, AW_active Mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_CALLBACK_LATER;
    void insert_menu_topic(const char *topic_id, AW_label name, const char *mnemonic, const char *helpText, AW_active Mask, const WindowCallback& cb);
    void close_sub_menu();

    void insert_separator();
    void insert_help_topic(AW_label name, const char *mnemonic, const char *help_text, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void insert_separator_help();
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

    void set_vertical_change_callback(const WindowCallback& wcb);
    void set_horizontal_change_callback(const WindowCallback& wcb);
    void set_vertical_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_CALLBACK_LATER;
    void set_horizontal_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_CALLBACK_LATER;
    
    void set_horizontal_scrollbar_left_indent(int indent);
    void set_vertical_scrollbar_top_indent(int indent);
    void set_vertical_scrollbar_bottom_indent(int indent);


    // ************** Control window size  *********
    void set_window_size(int width, int height);
    void get_window_size(int& width, int& height);
    void window_fit();                              // Recalculate the size of a window with buttons


    // ************** Control window position  *********
    void set_window_pos(int width, int height);
    void get_window_pos(int& xpos, int& ypos);

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

    void TuneBackground(Widget w, int modStrength);
    void TuneOrSetBackground(Widget w, const char *color, int modStrength);

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

    void sens_mask(AW_active mask); // Set the sensitivity mask used for following widgets (Note: reset by next at()-command)
    void help_text(const char *id); // Set the help text of a button

    // normal callbacks
    void callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_CALLBACK_LATER;
    void callback(void (*f)(AW_window*, AW_CL), AW_CL cd1) __ATTR__DEPRECATED_CALLBACK_LATER;
    void callback(void (*f)(AW_window*)) __ATTR__DEPRECATED_CALLBACK_LATER;
    
    void callback(const WindowCallback& cb);
    void callback(AW_cb *awcbs);

    // callbacks for
    // - 'double click' (in selection lists)
    // - 'ENTER'        (in input fields)
    // (might also work elsewhere, please document here if you find such 'elsewhere')

    void d_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_CALLBACK_LATER;
    void d_callback(void (*f)(AW_window*, AW_CL), AW_CL cd1) __ATTR__DEPRECATED_CALLBACK_LATER;

    void d_callback(const WindowCallback& cb);
    void d_callback(AW_cb *awcbs);

    // *** create the buttons ********

    void   create_button(const char *macro_name, AW_label label, const char *mnemonic = 0, const char *color = 0); // simple button; shadow only when callback
    void   create_autosize_button(const char *macro_name, AW_label label, const char *mnemonic = 0, unsigned xtraSpace = 1); // as above, but ignores button_length
    Widget get_last_widget() const;

    void create_toggle(const char *awar_name);  // int 0/1  string yes/no   float undef
    void create_inverse_toggle(const char *awar_name);  // like create_toggle, but displays inverted toggle value

    void create_toggle(const char *awar_name, const char *nobitmap, const char *yesbitmap, int buttonWidth = 0);
    void create_text_toggle(const char *var_name, const char *noText, const char *yesText, int buttonWidth = 0);

    void create_input_field(const char *awar_name, int columns = 0);   // One line textfield
    void create_text_field(const char *awar_name, int columns = 20, int rows = 4);   // Multi line textfield
    // with scrollbars


    // ***** option_menu is a menu where only one selection is visible at a time
    AW_option_menu_struct *create_option_menu(const char *awar_name, AW_label label=0, const char *mnemonic=0);
    void clear_option_menu(AW_option_menu_struct *oms);  // used to redefine available options

private:
    void insert_option_internal(AW_label choice_label, const char *mnemonic, const char *var_value,  const char *name_of_color, bool default_option);
    void insert_option_internal(AW_label choice_label, const char *mnemonic, int var_value,          const char *name_of_color, bool default_option);
    void insert_option_internal(AW_label choice_label, const char *mnemonic, float var_value,        const char *name_of_color, bool default_option);

    void insert_toggle_internal(AW_label toggle_label, const char *mnemonic, const char *var_value, bool default_toggle);
    void insert_toggle_internal(AW_label toggle_label, const char *mnemonic, int var_value,          bool default_toggle);
    void insert_toggle_internal(AW_label toggle_label, const char *mnemonic, float var_value,        bool default_toggle);
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
    void update_option_menu(AW_option_menu_struct *);  // don't use this


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

    AW_selection_list *create_selection_list(const char *awar_name, const char *label = 0, const char *mnemonic = 0, int columns = 4, int rows = 4);
    // to make a copy of the selection list
    AW_selection_list *copySelectionList(AW_selection_list *sourceList, AW_selection_list *destinationList);
    // for string
    void insert_selection(AW_selection_list *selection_list, const char *displayed, const char *value);
    void insert_default_selection(AW_selection_list * selection_list, const char *displayed, const char *value);
    // for int
    void insert_selection(AW_selection_list * selection_list, const char *displayed, int32_t value);
    void insert_default_selection(AW_selection_list * selection_list, const char *displayed, int32_t value);
    // for float
    void insert_selection(AW_selection_list * selection_list, const char *displayed, float value);
    void insert_default_selection(AW_selection_list * selection_list, const char *displayed, float value);
    // for pointers (only allowed if selection list uses a tmp-awars)
    void insert_selection(AW_selection_list * selection_list, const char *displayed, void *pointer);
    void insert_default_selection(AW_selection_list * selection_list, const char *displayed, void *pointer);

    void delete_selection_from_list(AW_selection_list * selection_list, const char *displayed_string);
    void conc_list(AW_selection_list * from_list_id,   AW_selection_list * to_list_id);

    // --- selection list iterator:
    void        init_list_entry_iterator(AW_selection_list *selection_list) const;
    void        iterate_list_entry(int offset);
    // --- the following functions work on the currently iterated element:
    const char *get_list_entry_char_value() const;
    const char *get_list_entry_displayed() const;
    void        set_list_entry_char_value(const char *new_char_value);
    void        set_list_entry_displayed(const char *new_displayed);
    // ---------------------------------------------------------

    void clear_selection_list(AW_selection_list *selection_list);
    void update_selection_list(AW_selection_list *selection_list);
    void init_selection_list_from_array(AW_selection_list *selection_list, const class CharPtrArray& entries, const char *defaultEntry);

    int         get_index_of_element(AW_selection_list *selection_list, const char *selected_element);
    const char *get_element_at_index(AW_selection_list *selection_list, int index);

    int  get_index_of_selected_element(AW_selection_list *selection_list);
    void select_element_at_index(AW_selection_list *selection_list, int wanted_index);
    void move_selection(AW_selection_list *selection_list, int offset);

    void selection_list_to_array(class StrArray& array, AW_selection_list *sel_list, bool values);
    char     *get_selection_list_contents(AW_selection_list * selection_list, long nr_of_lines = -1);
    GB_HASH  *selection_list_to_hash(AW_selection_list *sel_list, bool case_sens);

    void      sort_selection_list(AW_selection_list * selection_list, int backward, int case_sensitive);
    GB_ERROR  save_selection_list(AW_selection_list * selection_list, const char *filename, long number_of_lines = 0);
    void      set_selection_list_suffix(AW_selection_list * selection_list, const char *suffix);
    GB_ERROR  load_selection_list(AW_selection_list * selection_list, const char *filename);

    AW_selection_list *create_multi_selection_list(const char *label = 0, const char *mnemonic = 0, int columns = 4, int rows = 4);
    // all works, but its not connected to an AWAR may be converted to AW_mselection_list_helper
};


class AW_window_menu_modes : public AW_window { // derived from a Noncopyable
    void *AW_window_menu_modes_private;       // Do not use !!!
    
public:
    AW_window_menu_modes();
    ~AW_window_menu_modes();
    void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
};

class AW_window_menu : public AW_window {
private:
public:
    AW_window_menu();
    ~AW_window_menu();
    void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
};

class AW_window_simple_menu : public AW_window {
private:
public:
    AW_window_simple_menu();
    ~AW_window_simple_menu();
    void init(AW_root *root, const char *wid, const char *windowname);
};


class AW_window_simple : public AW_window {
private:
public:
    AW_window_simple();
    ~AW_window_simple();
    void init(AW_root *root, const char *wid, const char *windowname);
};


class AW_window_message : public AW_window {
private:
public:
    AW_window_message();
    ~AW_window_message();
    void init(AW_root *root, const char *windowname, bool allow_close);
};


typedef void* AW_gc_manager;

#else
#error aw_window.hxx included twice
#endif