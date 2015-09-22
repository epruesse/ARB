#ifndef AW_WINDOW_HXX
#define AW_WINDOW_HXX

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
#ifndef CB_H
#include <cb.h>
#endif

#if defined(ARB_GTK)
# if defined(ARB_MOTIF)
#  error ARB_GTK and ARB_MOTIF cannot both be defined
# endif
#else // !defined(ARB_GTK)
# if !defined(ARB_MOTIF)
#  error Either ARB_GTK or ARB_MOTIF has to be defined
# endif
#endif

class AW_window;
class AW_xfig;
class AW_device;
struct AW_screen_area;
struct GB_HASH;
class AW_device_click;
class AW_device_print;
class AW_device_size;

// --------------------------------------------------------------------------------

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

enum AW_orientation { AW_HORIZONTAL, AW_VERTICAL };

typedef const char *AW_label;       // label for buttons menus etc
// "fsdf" simple label  // no '/' symbol !!!
// "awarname/asdf"  // awar name (any '/' in string)
// "#file.xpm"   // pixmap in $ARBHOME/lib/pixmaps/file.xpm

const char *AW_get_pixmapPath(const char *pixmapName);

void AW_POPDOWN(AW_window *);
void AW_POPUP(AW_window*, AW_CL, AW_CL) __ATTR__DEPRECATED_TODO("directly pass CreateWindowCallback");

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

    AW_event()
        : type(AW_event_type(0)),
          time(0),
          keymodifier(AW_KEYMODE_NONE),
          button(AW_BUTTON_NONE),
          x(0),
          y(0),
          keycode(AW_KEY_NONE),
          character(0)
    {}
};

void AW_help_popup(AW_window *aw, const char *help_file);
inline WindowCallback makeHelpCallback(const char *helpfile) { return makeWindowCallback(AW_help_popup, helpfile); }

void AW_help_entry_pressed(AW_window *);
void AW_clock_cursor(AW_root *);
void AW_normal_cursor(AW_root *);

void AW_openURL(AW_root *aw_root, const char *url);

typedef void (*AW_cb_struct_guard)();

class AW_cb : virtual Noncopyable {
    WindowCallback cb;

    AW_cb *next;

    static AW_cb_struct_guard guard_before;
    static AW_cb_struct_guard guard_after;
    static AW_postcb_cb       postcb; // called after each cb triggered via interface

public:
    // private (read-only):
    AW_window  *aw;
    const char *help_text;
    char       *id;

    // real public section:
    AW_cb(AW_window  *awi,
          AW_CB       g,
          AW_CL       cd1i       = 0,
          AW_CL       cd2i       = 0,
          const char *help_texti = 0,
          AW_cb      *next       = 0);

    AW_cb(AW_window             *awi,
          const WindowCallback&  wcb,
          const char            *help_texti = 0,
          AW_cb                 *next       = 0);

    ~AW_cb() {
        delete next; next = NULL;
        free(id);
    }

    void run_callbacks();                           // runs the whole list
    bool contains(AW_CB g);                         // test if contained in list
    bool is_equal(const AW_cb& other) const;

    int compare(const AW_cb& other) const { return cb<other.cb ? -1 : (other.cb<cb ? 1 : 0); }

#if defined(ASSERTION_USED)
    AW_CL get_cd1() const { return cb.inspect_CD1(); }
    AW_CL get_cd2() const { return cb.inspect_CD2(); }
#endif // DEBUG

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
class  AW_selection_list_entry;
class  AW_selection_list;
struct AW_option_menu_struct;
struct aw_toggle_data;

enum AW_at_storage_type {
    AW_AT_SIZE_AND_ATTACH,
    AW_AT_AUTO,
    AW_AT_MAXSIZE,
};
struct AW_at_storage {
    //! store/restore some properties from/to AW_at
    virtual ~AW_at_storage() {}

    // will be called via AW_window
    virtual void store(const AW_at& at)   = 0;
    virtual void restore(AW_at& at) const = 0;

    static AW_at_storage *make(AW_window *aww, AW_at_storage_type type); // factory
};

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

enum AW_ScalerType {
    AW_SCALER_LINEAR,
    AW_SCALER_EXP_LOWER,  // fine-tuned at lower border, big steps at upper border
    AW_SCALER_EXP_UPPER,  // fine-tuned at upper border, big steps at lower border
    AW_SCALER_EXP_CENTER, // fine-tuned at center, big steps at borders
    AW_SCALER_EXP_BORDER, // fine-tuned at borders, big steps at center
};

class AW_ScalerTransformer {
    AW_ScalerType type;
public:
    AW_ScalerTransformer(AW_ScalerType type_) : type(type_) {}

    float scaler2awar(float scaler, AW_awar *awar); // [0..1] -> awar-range
    float awar2scaler(AW_awar *awar); // returns [0..1]
};

class AW_window : virtual Noncopyable {
    AW_SizeRecalc recalc_size_at_show;
    AW_PosRecalc  recalc_pos_at_show;

    AW_CB0 hide_cb;
    bool expose_callback_added;

    AW_cb *focus_cb;

    AW_xfig *xfig_data;
    AW_at   *_at; /** < Defines the next position at which something will be inserted into the window.  */

    int left_indent_of_horizontal_scrollbar;
    int top_indent_of_vertical_scrollbar;

    void all_menus_created() const;
    void create_toggle(const char *var_name, aw_toggle_data *tdata);

#if defined(ARB_MOTIF)
    int calculate_string_width(int columns) const;
    int calculate_string_height(int columns, int offset) const;
    char *align_string(const char *string, int columns);
    void calculate_label_size(int *width, int *height, bool in_pixel, const char *non_at_label);
#endif

protected:
    AW_root *root;

    void create_devices();
    void set_background(const char *colorname, Widget w);

    void wm_activate();                                // un-minimize window and give it the focus (use show_and_activate())

public:

#if defined(ARB_MOTIF)
    // ---------------------------------------- [start read-only section] @@@ should go private

    AW_event         event;
    unsigned long    click_time;
    long             color_table_size;
    AW_rgb          *color_table;
    int              number_of_timed_title_changes;
    AW_window_Motif *p_w;
    AW_cb           *_callback;
    AW_cb           *_d_callback;

    // ---------------------------------------- [end read-only section]
#endif

#if defined(ARB_MOTIF)
#if defined(IN_ARB_WINDOW)
    // only used internal and in motif (alternative would be to move a bunch of code into AW_window)
    const AW_at& get_at() const { return *_at; }
    AW_at& get_at() { return *_at; }
#endif
#endif

    AW_window();
    virtual ~AW_window();

    const char    *window_local_awarname(const char *localname, bool tmp = true);
    class AW_awar *window_local_awar(const char *localname, bool tmp = true);
    void           create_window_variables();

    void         recalc_pos_atShow(AW_PosRecalc pr);
    void         recalc_size_atShow(enum AW_SizeRecalc sr);
    AW_PosRecalc get_recalc_pos_atShow() const;

    void allow_delete_window(bool allow_close);
    void on_hide(AW_CB0 call_on_hide);


#if defined(ARB_MOTIF)
    void run_focus_callback();
    void show_modal();
    void set_window_title_intern(char *title);
#endif

    void update_label(Widget widget, const char *var_value);
    void update_toggle(Widget widget, const char *var_value, AW_CL cd);
    void update_input_field(Widget widget, const char *var_value);
    void update_text_field(Widget widget, const char *var_value);
    void update_scaler(Widget widget, AW_awar *awar, AW_ScalerType scalerType);

    void  create_invisible(int columns);
    void *_create_option_entry(AW_VARIABLE_TYPE type, const char *name, const char *mnemonic, const char *name_of_color);
    void  refresh_toggle_field(int toggle_field_number); 
    void  _set_activate_callback(void *widget);
    void  increment_at_commands(int width, int height);


    AW_color_idx alloc_named_data_color(int colnum, const char *colorname);

    // special for EDIT4
    void _get_area_size(AW_area area, AW_screen_area *square);
    
    int label_widget(void *wgt, AW_label str, char *mnemonic=0, int width = 0, int alignment = 0);

    // ------------------------------
    //      The read only section

    char *window_name;          //! window title
    char *window_defaults_name; //! window id
    bool  window_is_shown;

    int slider_pos_vertical;
    int slider_pos_horizontal;
    int main_drag_gc;

    AW_screen_area *picture;      // the result of tell scrolled picture size

    // --------------------------------
    //      The real public section

    AW_root *get_root() { return root; }
    // ******************* Global layout functions **********************

    void show(); // show newly created window or unhide hidden window (aka closed window)
    void hide(); // hide (don't destroy) a window (<->show)

    void activate() { show(); wm_activate(); }      // make_visible, pop window to front and give it the focus

    bool is_shown() const;  // is window visible (== true) or hidden (== false). ?

    void hide_or_notify(const char *error);

    void    message(char *title, int ms);   // Set for ms milliseconds the title of the window
    void    set_window_title(const char *title);   // Set the window title forever

    const char *get_window_title() const;       // Get the window's title
    const char *get_window_id() const { return window_defaults_name; } // Get the window's internal name

    const char *local_id(const char *id) const;

    void set_info_area_height(int height);
    void set_bottom_area_height(int height);

    // ******************* Input and Motion Events **********************

    void set_popup_callback(const WindowCallback& wcb);
    void set_focus_callback(const WindowCallback& wcb);
    bool is_focus_callback(void (*f)(AW_window*, AW_CL, AW_CL));

    void set_expose_callback(AW_area area, const WindowCallback& wcb);
    void set_resize_callback(AW_area area, const WindowCallback& wcb);

private:
    // motif relicts:
    void set_expose_callback(AW_area area, AW_CB0 cb) { set_expose_callback(area, makeWindowCallback(cb)); }
    void set_resize_callback(AW_area area, AW_CB0 cb) { set_resize_callback(area, makeWindowCallback(cb)); }
public:

    void set_input_callback(AW_area area, const WindowCallback& wcb);
    void set_motion_callback(AW_area area, const WindowCallback& wcb);

    void set_double_click_callback(AW_area area, const WindowCallback& wcb);

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

    /**
     * Creates a new top level menu.
     * @param name     Name of the menu.
     * @param mnemonic Shortcut (optional)
     * @param mask     Experts only?
     */
    void create_menu(const char *name, const char *mnemonic, AW_active mask = AWM_ALL);

    /**
     * Insert a sub menu into the last created menu.
     * @param name     Name of the sub menu.
     * @param mnemonic Shortcut (optional)
     * @param mask     Experts only?
     */
    void insert_sub_menu(const char *name, const char *mnemonic, AW_active mask = AWM_ALL);

    /**
     * Insert a menu item into the last created menu or sub menu.
     * @param id           Unique id (for macros)
     * @param name         Name of the item.
     * @param mnemonic     Shortcut (optional)
     * @param help_text_   Name of helpfile (optional)
     * @param mask         Experts only?
     * @param wcb Callback that should be called when the item is activated.
     */
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, const WindowCallback& wcb);

    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, const CreateWindowCallback& cwcb) { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowPopper(cwcb)); }
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, WindowCallbackSimple cb)          { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowCallback(cb)); }
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, CreateWindowCallbackSimple cb)    { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeCreateWindowCallback(cb)); }

    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB cb, AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass WindowCallback") { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowCallback(cb, cd1, cd2)); }
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB1 cb, AW_CL cd1) __ATTR__DEPRECATED_TODO("pass WindowCallback") { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowCallback(cb, cd1)); }

    void sep______________();
    void close_sub_menu();

    void insert_help_topic(const char *labeli, const char *mnemonic, const char *helpText, AW_active mask, const WindowCallback& cb);
    void insert_help_topic(const char *labeli, const char *mnemonic, const char *helpText, AW_active mask, AW_CB0 cb) { insert_help_topic(labeli, mnemonic, helpText, mask, makeWindowCallback(cb)); }
    void insert_help_topic(const char *labeli, const char *mnemonic, const char *helpText, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass WindowCallback") {
        insert_help_topic(labeli, mnemonic, helpText, mask, makeWindowCallback(f, cd1, cd2));
    }

    // ************** Create modes on the left side ******************
    int create_mode(const char *pixmap, const char *help_text_, AW_active mask, const WindowCallback& cb);
    void select_mode(int mode);

    // ************** Control the size of the main drawing area + scrollbars  *********
    void tell_scrolled_picture_size(AW_screen_area rectangle);
    void tell_scrolled_picture_size(AW_world rectangle);
    AW_pos get_scrolled_picture_width() const;
    AW_pos get_scrolled_picture_height() const;
    void reset_scrolled_picture_size();

    void get_scrollarea_size(AW_screen_area *square);

    void calculate_scrollbars();
    void set_vertical_scrollbar_position(int position);
    void set_horizontal_scrollbar_position(int position);

    void set_vertical_change_callback(const WindowCallback& wcb);
    void set_horizontal_change_callback(const WindowCallback& wcb);
    void set_vertical_scrollbar_top_indent(int indent);
    void set_horizontal_scrollbar_left_indent(int indent);


    void update_scrollbar_settings_from_awars(AW_orientation orientation);

    void create_user_geometry_awars(int posx, int posy, int width, int height);
    
    // ************** Control window size  *********
#if defined(IN_ARB_WINDOW)
    void set_window_size(int width, int height);
#endif
    void get_window_size(int& width, int& height);
    void window_fit();                              // Recalculate the size of a window with buttons

#if defined(IN_ARB_WINDOW)
    void store_size_in_awars(int width, int height);
    void get_size_from_awars(int& width, int& height);

    // ************** Control window position  *********
    void set_window_frame_pos(int xpos, int ypos);
    void get_window_content_pos(int& xpos, int& ypos);

    void store_pos_in_awars(int xpos, int ypos);
    void get_pos_from_awars(int& xpos, int& ypos);

#if defined(ARB_MOTIF)
    void reset_geometry_awars();
#endif

    // *****************
    void get_screen_size(int& width, int& height);
    bool get_mouse_pos(int& x, int& y);
    void set_focus_policy(bool follow_mouse);
    void get_font_size(int& w, int& h);
#endif
    
    // ************** ********************************************************************  *********
    // ************** Create buttons: First set modify flags and finally create the button  *********
    // ************** ********************************************************************  *********

    // *** global modifier: ****
    void load_xfig(const char *file, bool resize=true); // Loads the background graphic
    void draw_line(int x1, int y1, int x2, int y2, int width, bool resize); // draws a line on the background

    void label_length(int length);   // Justifies all following labels
    void button_length(int length);   // Sets the width of all following buttons (in chars)
#if defined(ARB_MOTIF)
    void button_height(int height);   // Sets the height of all following buttons (in lines)
#endif
    int  get_button_length() const; // returns the current width of buttons
    void highlight();           // Creates a frame around the button
    void auto_increment(int dx, int dy);   // enable automatic placement of buttons
    // dx is the horizontal distance between the left
    // borders of two buttons
    void auto_space(int xspace, int yspace);   // enable automatic placement of buttons
    // xspace is the horizontal space between 2 buttons

    void shadow_width (int shadow_thickness); // set the shadow_thickness of buttons

#if defined(ARB_MOTIF)
    void TuneBackground(Widget w, int modStrength);
    void TuneOrSetBackground(Widget w, const char *color, int modStrength);
#endif

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

    void at_set_to(bool attach_x, bool attach_y, int xoff, int yoff); // set "to:XY:id" manually
    void at_unset_to();                                               // unset "to:id" manually
    void unset_at_commands();

    void store_at_to(AW_at_storage& storage) { storage.store(*_at); }
    void restore_at_from(const AW_at_storage& stored) { stored.restore(*_at); }

    void sens_mask(AW_active mask);   // Set the sensitivity mask used for following widgets (Note: reset by next at()-command)
    void help_text(const char *id);   // Set the help text of a button

private:
    static void popper(AW_window *, CreateWindowCallback *windowMaker);
    static void replacer(AW_window *aww, CreateWindowCallback *windowMaker);
    static void destroyCreateWindowCallback(CreateWindowCallback *windowMaker);
public:
    static WindowCallback makeWindowPopper(const CreateWindowCallback& cwcb) {
        return makeWindowCallback(popper, destroyCreateWindowCallback, new CreateWindowCallback(cwcb));
    }
    static WindowCallback makeWindowReplacer(const CreateWindowCallback& cwcb) {
        return makeWindowCallback(replacer, destroyCreateWindowCallback, new CreateWindowCallback(cwcb));
    }

    // normal callbacks
    void callback(const WindowCallback& cb);

    void callback(const CreateWindowCallback& cwcb) { callback(makeWindowPopper(cwcb)); }
    void callback(CreateWindowCallbackSimple cb)    { callback(makeCreateWindowCallback(cb)); }
    void callback(WindowCallbackSimple cb)          { callback(makeWindowCallback(cb)); }

    void callback(void (*f)(AW_window*, AW_CL), AW_CL cd1) __ATTR__DEPRECATED_TODO("pass WindowCallback") { callback(makeWindowCallback(f, cd1)); }
    void callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass WindowCallback") { callback(makeWindowCallback(f, cd1, cd2)); }

    void d_callback(const WindowCallback& cb); // secondary callback (called for 'double click into selection list' and 'text field hit ENTER')

    // *** create the buttons ********
    void   create_button(const char *macro_name, AW_label label, const char *mnemonic = 0, const char *color = 0); // simple button; shadow only when callback
    void   create_autosize_button(const char *macro_name, AW_label label, const char *mnemonic = 0, unsigned xtraSpace = 1); // as above, but ignores button_length
    Widget get_last_widget() const;

    void create_toggle(const char *awar_name);  // int 0/1  string yes/no   float undef
    void create_inverse_toggle(const char *awar_name);  // like create_toggle, but displays inverted toggle value

    void create_toggle(const char *awar_name, const char *nobitmap, const char *yesbitmap, int buttonWidth = 0);
    void create_text_toggle(const char *var_name, const char *noText, const char *yesText, int buttonWidth = 0);

    void create_input_field(const char *awar_name, int columns = 0);                 // One line textfield
    void create_text_field(const char *awar_name, int columns = 20, int rows = 4);   // Multi line textfield with scrollbars
    void create_input_field_with_scaler(const char *awar_name, int textcolumns = 4, int scaler_length = 250, AW_ScalerType scalerType = AW_SCALER_LINEAR);


    // ***** option_menu is a menu where only one selection is visible at a time
    AW_option_menu_struct *create_option_menu(const char *awar_name, bool fallback2default);
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

    AW_selection_list *create_selection_list(const char *awar_name, int columns, int rows, bool fallback2default);
    AW_selection_list *create_selection_list(const char *awar_name, bool fallback2default) { return create_selection_list(awar_name, 4, 4, fallback2default); }
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
    void init(AW_root *root_in, const char *wid, const char *windowname, bool allow_close);
    void init(AW_root *root_in, const char *windowname, bool allow_close); // auto-generates window id from title
};

typedef struct aw_gc_manager *AW_gc_manager;


#else
#error aw_window.hxx included twice
#endif
