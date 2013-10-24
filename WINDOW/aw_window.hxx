#pragma once

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARB_ASSERT_H 
#include <arb_assert.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif

#include "aw_gtk_forward_declarations.hxx"
#include "aw_root.hxx"
#include "aw_at.hxx"

#include "aw_help.hxx"
#include "aw_cb_struct.hxx"


//the following types are not forward declared because ARB needs to know about them
//and I dont want to change includes in ARB
#include "aw_device_print.hxx" 
#include "aw_device_click.hxx"
#include "aw_device_size.hxx"

// aw modal calls

// Read a string from the user :

char *aw_input(const char *title, const char *prompt, const char *default_input);
char *aw_input(const char *prompt, const char *default_input);
inline char *aw_input(const char *prompt) { return aw_input(prompt, NULL); }

char *aw_input2awar(const char *prompt, const char *awar_value, const char* title = "Enter text");

char *aw_string_selection     (const char *title, const char *prompt, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*));
char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name,     const char *value_list, const char *buttons, char *(*check_fun)(const char*));

int aw_string_selection_button();   // returns index of last selected button (destroyed by aw_string_selection and aw_input)

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix);



class AW_window;
class AW_xfig;
class AW_device;
class AW_screen_area;
struct GB_HASH;

// --------------------------------------------------------------------------------

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


typedef const char *AW_label;       // label for buttons menus etc
// "fsdf" simple label  // no '/' symbol !!!
// "awarname/asdf"  // awar name (any '/' in string)
// "#file.xpm"   // pixmap in $ARBHOME/lib/pixmaps/file.xpm

const char *AW_get_pixmapPath(const char *pixmapName);

void AW_POPDOWN(AW_window *);
void AW_POPUP(AW_window*, AW_CL, AW_CL) __ATTR__DEPRECATED_TODO("directly pass CreateWindowCallback");

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
struct AW_selection_list_entry;
class  AW_selection_list;
typedef AW_selection_list AW_option_menu_struct; //for compatibility reasons with old arb code
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

typedef AW_CB0 aw_hide_cb;

class AW_window : virtual Noncopyable {
    AW_SizeRecalc recalc_size_at_show;
    AW_PosRecalc  recalc_pos_at_show;
//    enum AW_PosRecalc  recalc_pos_at_show;
//    aw_hide_cb         hide_cb;
//
//    bool expose_callback_added;
//
//    void all_menus_created() const;

    AW_awar *awar_posx, *awar_posy, *awar_width, *awar_height;
protected:
    class AW_window_gtk;
    AW_window_gtk* prvt; /*< Contains all gtk dependent attributes */
  
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

    // ************ This is not the public section *************
    //FIXME move aw_at into pimpl
    //FIXME make _at private. Right now some global functions want to access it. Remove those global functions.
    AW_at _at; /** < Defines the next position at which something will be inserted into the window.  */

    AW_window();
    virtual ~AW_window();

    AW_xfig  *xfig_data;

    void create_window_variables();

    void recalc_pos_atShow(AW_PosRecalc pr);

    AW_PosRecalc get_recalc_pos_atShow() const;

    void recalc_size_atShow(enum AW_SizeRecalc sr);

   
    void allow_delete_window(bool allow_close);
    void on_hide(AW_CB0 call_on_hide);

    /**
     * @return The current xfig data or NULL.
     */
    AW_xfig* get_xfig_data();

    void set_window_title_intern(char *title);

    void update_label(GtkWidget* widget, const char *var_value);
    void update_toggle(GtkWidget *widget, const char *var, AW_CL /*cd_toggle_data*/);
    void update_progress_bar(GtkWidget* progressBar, const double var_value);

    void  create_invisible(int columns);
    void *_create_option_entry(GB_TYPES type, const char *name, const char *mnemonic, const char *name_of_color);
    void  unset_at_commands();

    void _get_area_size(AW_area area, AW_screen_area *square);
    
    int label_widget(void *wgt, const char *str, char *mnemonic=0, int width = 0, int alignment = 0);

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


    // picture size

    // --------------------------------
    //      The real public section

    AW_root *get_root() { return AW_root::SINGLETON; }
    /**
     * 
     * @param index
     * @return The area at index i or NULL of that area does not exist 
     */
    AW_area_management* get_area(int index);
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
    void create_menu(const char *name, const char *mnemonic, AW_active mask = AWM_ALL);
    /**
     * Insert a sub menu into the last created menu.
     * @param name Name of the sub menu.
     * @param mnemonic FIXME
     * @param mask FIXME
     */
    void insert_sub_menu(const char *name, const char *mnemonic, AW_active mask = AWM_ALL);
    /**
     * Insert a menu item into the last created menu or sub menu.
     * @param id FIXME
     * @param name Name of the item.
     * @param mnemonic FIXME
     * @param help_text_ FIXME
     * @param mask FIXME
     * @param cb Callback that should be called when the item is activated.
     */
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, const WindowCallback& cb);
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, const CreateWindowCallback& cwcb) { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowPopper(cwcb)); }
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB0 cb) { insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowCallback(cb)); }

    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB cb, AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass WindowCallback") {
        insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowCallback(cb, cd1, cd2));
    }
    void insert_menu_topic(const char *id, const char *name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB1 cb, AW_CL cd1) __ATTR__DEPRECATED_TODO("pass WindowCallback") {
        insert_menu_topic(id, name, mnemonic, help_text_, mask, makeWindowCallback(cb, cd1));
    }
    /**
     * insert a separator into the currently open menu
     */
    void sep______________(); 
    /**
     * Closes the currently open sub menu.
     * If no menu is open this method will crash.
     */
    void close_sub_menu();

    void insert_help_topic(const char *name, const char *mnemonic, const char *help_text_, AW_active mask, const WindowCallback& cb);
    void insert_help_topic(const char *name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB0 cb) { insert_help_topic(name, mnemonic, help_text_, mask, makeWindowCallback(cb)); }
    void insert_help_topic(const char *name, const char *mnemonic, const char *help_text_, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass WindowCallback") {
        insert_help_topic(name, mnemonic, help_text_, mask, makeWindowCallback(f, cd1, cd2));
    }

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
    void get_font_size(int& w, int& h);

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
    void highlight();
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
    void help_text(const char *id);   // Set the help text of a button

private:
    static void popper(AW_window *, CreateWindowCallback *windowMaker);
    static WindowCallback makeWindowPopper(const CreateWindowCallback& cwcb) {
        return makeWindowCallback(popper, new CreateWindowCallback(cwcb));
    }

public:
    // normal callbacks
    void callback(const WindowCallback& cb);
    void callback(const CreateWindowCallback& cwcb) { callback(makeWindowPopper(cwcb)); }
    void callback(void (*f)(AW_window*)) { callback(makeWindowCallback(f)); }
    void callback(void (*f)(AW_window*, AW_CL), AW_CL cd1) __ATTR__DEPRECATED_TODO("pass WindowCallback") { callback(makeWindowCallback(f, cd1)); }
    void callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass WindowCallback") { callback(makeWindowCallback(f, cd1, cd2)); }

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

    void create_input_field(const char *awar_name, int columns = 0);   // One line textfield
    void create_text_field(const char *awar_name, int columns = 20, int rows = 4);   // Multi line textfield
    // with scrollbars


    // ***** option_menu is a menu where only one selection is visible at a time
    AW_selection_list* create_option_menu(const char *awar_name, const char *label= NULL, const char *mnemonic= NULL);
    void clear_option_menu(AW_selection_list *list);  // used to redefine available options

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
    void insert_toggle_internal(const char *toggle_label, const char *mnemonic, T var_value, bool default_toggle);

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

    AW_selection_list *create_selection_list(const char *awar_name, int columns = 4, int rows = 4);

    // gtk ready made buttons
    void create_font_button(const char *awar_name, const char *label);
    void create_color_button(const char *awar_name, const char *label);

    // *** awar/action access helpers 
    AW_action* action(const char* action_id);
    AW_action* action_try(const char* action_id);
    AW_action* action_register(const char* action_id);
    AW_action* action_register(const char* action_id, const char* label, const char* icon,
                      const char* tooltip, const char* help_entry, AW_active mask);
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

typedef class _AW_gc_manager* AW_gc_manager;


