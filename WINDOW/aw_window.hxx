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
#ifndef CB_H
#include <cb.h>
#endif

#include "aw_gtk_forward_declarations.hxx"
#include "aw_root.hxx"

#include "aw_help.hxx"
#include "aw_cb_struct.hxx"


//the following types are not forward declared because ARB needs to know about them
//and I dont want to change includes in ARB
#include "aw_device_print.hxx" 
#include "aw_device_click.hxx"
#include "aw_device_size.hxx"

#if defined(ARB_GTK)
# if defined(ARB_MOTIF)
#  error ARB_GTK and ARB_MOTIF cannot both be defined
# endif
#else // !defined(ARB_GTK)
# if !defined(ARB_MOTIF)
#  error Either ARB_GTK or ARB_MOTIF has to be defined
# endif
#endif

// aw modal calls

// Read a string from the user :

char *aw_input(const char *title, const char *prompt, const char *default_input);
char *aw_input(const char *prompt, const char *default_input);
inline char *aw_input(const char *prompt) { return aw_input(prompt, NULL); }
char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix);
char* aw_convert_mnemonic(const char* text, const char* mnemonic);

class AW_window;
class AW_xfig;
class AW_device;
struct AW_screen_area;
struct GB_HASH;
class AW_window_gtk;

// --------------------------------------------------------------------------------

#define AW_MESSAGE_TIME 2000
#define AW_HEADER_MAIN

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

/**
 * Switches the window into help mode.
 * The next button click will open the corresponding help entry.
 * @param 
 */
void AW_help_entry_pressed(AW_window *);
void AW_clock_cursor(AW_root *);
void AW_normal_cursor(AW_root *);

void AW_openURL(AW_root *aw_root, const char *url);

enum {
    AWM_DISABLED = 0,           // disabled items (used for dynamically dis-/enabled items)
    AWM_BASIC    = 1,
    AWM_EXP      = 2,
    AWM_ALL      = AWM_BASIC|AWM_EXP
};

#define legal_mask(m) (((m)&AWM_ALL) == (m))

enum {
    AWM_MASK_UNKNOWN = AWM_DISABLED,
    AWM_MASK_DEFAULT = AWM_BASIC,
    AWM_MASK_EXPERT  = AWM_ALL
};

typedef char *AW_pixmap;

class  AW_window_Motif;
class  AW_selection_list_entry;
class  AW_selection_list;
typedef AW_selection_list AW_option_menu_struct; //for compatibility reasons with old arb code
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

    AW_awar *awar_posx, *awar_posy, *awar_width, *awar_height;

protected:
    AW_window_gtk* prvt; /*< Contains all gtk dependent attributes */

private:
    AW_xfig *xfig_data;
    AW_at   *_at; /** < Defines the next position at which something will be inserted into the window.  */

    void all_menus_created() const;

protected:
  
    /* put a widget into prvt->fixedArea according to _at.
     * @param label_alignment the alignment for the label.
     *        Do NOT delete label_alignment after this call. Gtk will take care of it when the window is closed.*/
    void put_with_label(GtkWidget *widget, GtkAlignment *label_alignment);
    
    /* put a widget into prvt->fixedArea according to _at.
     * Label will be centered 
     **/
    void put_with_label(GtkWidget *widget);
    
    GtkWidget *make_label(const char* label_text, short label_length=0, const char* mnemonic=NULL);

    /** 
     * Common initialization function called from AW_window_xxx::init()
     * - set up window title and name ('id')
     * - set icon based on window title
     * - set window position and size 
     * - connect pos/size to awars
     */
    void init_window(const char *window_id, const char* window_name, 
                     int width, int height, bool resizable);

public:

    AW_window();
    virtual ~AW_window();

    const char    *window_local_awarname(const char *localname, bool tmp = true);
    class AW_awar *window_local_awar(const char *localname, bool tmp = true);
    void           create_window_variables();

    void         recalc_pos_atShow(AW_PosRecalc pr);
    void         recalc_size_atShow(enum AW_SizeRecalc sr);
    AW_PosRecalc get_recalc_pos_atShow() const;

    void allow_delete_window(bool allow_close);
    void on_hide(WindowCallbackSimple call_on_hide);

    AW_xfig* get_xfig_data(); //! @return The current xfig data or NULL.

#if defined(ARB_MOTIF)
    void run_focus_callback();
    void show_modal();
    void set_window_title_intern(char *title);
#endif

    // special for EDIT4 
    void _get_area_size(AW_area area, AW_screen_area *square);
    
    // ------------------------------
    //      The read only section

    AW_event event;             //! holds information about the most recent event
    char *window_name;          //! window title
    char *window_defaults_name; //! window id

    long int slider_pos_vertical;                 //! current position of the vertical slider 
    long int slider_pos_horizontal;               //! current position of the horizontal slider 
    int left_indent_of_horizontal_scrollbar; //! unscrolled part of screen area
    int top_indent_of_vertical_scrollbar;    //! unscrolled part of screen area
    AW_screen_area *picture;                 //! size of scrollable

    int main_drag_gc;

    // --------------------------------
    //      The real public section

    AW_root *get_root() { return AW_root::SINGLETON; }
    // ******************* Global layout functions **********************

    void show(); // show newly created window or unhide hidden window (aka closed window)
    void hide(); // hide (don't destroy) a window (<->show)

    void activate(); // make_visible, pop window to front and give it the focus

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

    void set_expose_callback(AW_area area, const WindowCallback& wcb);
    void set_resize_callback(AW_area area, const WindowCallback& wcb);

    void set_input_callback(AW_area area, const WindowCallback& wcb);
    void set_motion_callback(AW_area area, const WindowCallback& wcb);

    void set_double_click_callback(AW_area area, const WindowCallback& wcb);

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

    void sep______________();
    void close_sub_menu();

    void insert_help_topic(const char *labeli, const char *mnemonic, const char *helpText, AW_active mask, const WindowCallback& cb);
    void insert_help_topic(const char *labeli, const char *mnemonic, const char *helpText, AW_active mask, WindowCallbackSimple cb) { insert_help_topic(labeli, mnemonic, helpText, mask, makeWindowCallback(cb)); }

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

    void d_callback(const WindowCallback& cb); // secondary callback (called for 'double click into selection list' and 'text field hit ENTER')

    // *** create the buttons ********
    void create_label(const char* label, bool highlight=false);
    void create_button(const char *macro_name, const char *label, const char *mnemonic = 0, const char *color = 0); // simple button; shadow only when callback
    void create_autosize_button(const char *macro_name, const char *label, const char *mnemonic = 0, unsigned xtraSpace = 1); // as above, but ignores button_length
    GtkWidget* get_last_widget() const;

    void create_progressBar(const char *awar); 
    void create_checkbox(const char* var_name, bool inverse=false);
    void create_checkbox_inverse(const char* var_name);
    void create_toggle(const char *var_name, const char *no, const char *yes, int width = 0);  

    void create_input_field(const char *awar_name, int columns = 0); // One line textfield
    void create_text_field(const char *awar_name, int columns = 20, int rows = 4);   // Multi line textfield with scrollbars
    void create_input_field_with_scaler(const char *awar_name, int textcolumns = 4, int scaler_length = 250, AW_ScalerType scalerType = AW_SCALER_LINEAR);


    // ***** option_menu is a menu where only one selection is visible at a time
    AW_selection_list* create_option_menu(const char *awar_name, bool fallback2default);
    void               clear_option_menu(AW_selection_list *list); // used to redefine available options

    /**If set to true the window is not destroyed on close. instead it is hidden.*/
    void set_hide_on_close(bool value);
    
    void set_close_action(AW_action*  action);
    void set_close_action(const char* action); 


    // refactoring wrappers -- remove calls after @@@MERGE
    void create_toggle(const char *var_name) { create_checkbox(var_name); }
    void create_inverse_toggle(const char *var_name) { create_checkbox_inverse(var_name); }
    void create_text_toggle(const char *var_name, const char *no, const char *yes, int width = 0) {
        create_toggle(var_name, no, yes, width);
    }
    
    
private:
    
    template <class T>
    void insert_option_internal(const char *choice_label, const char *mnemonic, T var_value, bool default_option);

    template <class T>
    void insert_toggle_internal(const char *toggle_label, const char *mnemonic, T var_value);

    /**Is called if the window is closed*/
    static bool close_window_handler(GtkWidget*, GdkEvent*, gpointer data);
    
public:

    // for string
    void insert_option         (const char *choice_label, const char *mnemonic, const char *var_value);  // for string
    void insert_default_option (const char *choice_label, const char *mnemonic, const char *var_value);
    // for int
    void insert_option         (const char *choice_label, const char *mnemonic, int var_value);  // for int
    void insert_default_option (const char *choice_label, const char *mnemonic, int var_value);
    // for float
    void insert_option         (const char *choice_label, const char *mnemonic, float var_value);  // for float
    void insert_default_option (const char *choice_label, const char *mnemonic, float var_value);

    void update_option_menu();

    // ***** toggle_field is a static menu (all items are visible and only one is selected)
    void create_toggle_field(const char *awar_name, const char *label, const char *mnemonic);
    void create_toggle_field(const char *awar_name, int orientation = 0);  // 1 = horizontal
    // for string
    void insert_toggle(const char *toggle_label, const char *mnemonic, const char *var_value);
    void insert_default_toggle(const char *toggle_label, const char *mnemonic, const char *var_value);
    // for int
    void insert_toggle(const char *toggle_label, const char *mnemonic, int var_value);
    void insert_default_toggle(const char *toggle_label, const char *mnemonic, int var_value);
    // for float
    void insert_toggle(const char *toggle_label, const char *mnemonic, float var_value);
    void insert_default_toggle(const char *toggle_label, const char *mnemonic, float var_value);
    void update_toggle_field();

    // ***** selection list is a redefinable scrolled list of items

    AW_selection_list *create_selection_list(const char *awar_name, int columns, int rows, bool fallback2default);
    AW_selection_list *create_selection_list(const char *awar_name, bool fallback2default) { return create_selection_list(awar_name, 4, 4, fallback2default); }

    // gtk ready made buttons
    void create_font_button(const char *awar_name, const char *label);
    void create_color_button(const char *awar_name, const char *label);

    // *** awar/action access helpers 
    AW_action* action(const char* action_id, bool localize);
    AW_action* action_try(const char* action_id, bool localize);
    AW_action* action_register(const char* action_id, bool localize);
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

    // ************** Create modes on the left side ******************
    void create_mode(const char *pixmap, const char *help_text_, AW_active mask, const WindowCallback& cb);
    void select_mode(int mode);

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

class AW_gc_manager;

