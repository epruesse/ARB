#ifndef AW_WINDOW_HXX
#define AW_WINDOW_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif
#ifndef AW_KEYSYM_HXX
#include <aw_keysym.hxx>
#endif

class AW_window;
typedef struct _WidgetRec *Widget;

typedef void (*AW_CB)(AW_window*,AW_CL,AW_CL);
typedef void (*AW_CB0)(AW_window*);
typedef void (*AW_CB1)(AW_window*,AW_CL);
typedef void (*AW_CB2)(AW_window*,AW_CL,AW_CL);
typedef AW_window *(*AW_Window_Creator)(AW_root*,AW_CL);

//--------------------------------------------------------------------------------
// For Applications Using OpenGL Windows 
// Variable "AW_alpha_Size_Supported" says whether the hardware (Graphics Card) 
// supports alpha channel or not. Alpha channel is used for shading/ multi textures 
// in OpenGL applications.

extern bool AW_alpha_Size_Supported;
//--------------------------------------------------------------------------------

#define AW_POPUP  ((AW_CB)(-1))
// AW_POPDOWN is defined later in this section

#define AW_MESSAGE_TIME 2000
#define AW_HEADER_MAIN  extern "C" { int XtAppInitialize(); } void aw_never_called_main(void) { XtAppInitialize(); }

//======= Used in Tune background function ================================= 
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

    int     to_offset_x; // here we use offsets (not positions like in AW_at)
    int     to_offset_y;
    AW_BOOL to_position_exists;

    AW_BOOL attach_x;           // attach right side to right form
    AW_BOOL attach_y;
    AW_BOOL attach_lx;          // attach left side to right form
    AW_BOOL attach_ly;
    AW_BOOL attach_any;


public:
    void store(const AW_at *at);
    void restore(AW_at *at) const;
};

class AW_at_maxsize {
    int maxx;
    int maxy;

public:
    void store(const AW_at *at);
    void restore(AW_at *at) const;
};

typedef enum {
    AW_Keyboard_Press   = 1,
    AW_Keyboard_Release = 2,
    AW_Mouse_Press      = 3,
    AW_Mouse_Release    = 4,
    AW_Mouse_Drag       = 5
} AW_event_type;

typedef const char *AW_label;       // label for buttons menues etc
// "fsdf" simple label  // no '/' symbol !!!
// "awarname/asdf"  // awar name (any '/' in string)
// "#file.bitmap"   // bitmap in $ARBHOME/lib/pixmaps/file.bitmap


typedef struct {
    AW_event_type       type;       /* AW_Keyboard or AW_Mouse */
    unsigned long       time;       /* time in msec, when event occured */
    //***** button fields
    unsigned int        button;     /* which mouse button was pressed 1,2,3 */
    int         x,y;        /* pointer x,y coordinates in the event window */
    //****** key fields
    AW_key_mod      keymodifier;    /* control alt meta .... */
    AW_key_code     keycode;    /* which key type was pressed */
    char            character;  /* the c character */
} AW_event;


void AW_POPDOWN(AW_window *);
void AW_POPUP_HELP(AW_window *,AW_CL /*char */ helpfile);
void AW_help_entry_pressed(AW_window *);
void AW_clock_cursor(AW_root *);
void AW_normal_cursor(AW_root *);

/*************************************************************************/
class AW_cb_struct {
private:

    AW_CL               cd1;
    AW_CL               cd2;
    class AW_cb_struct *next;

public:
    // ************ This is not the public section *************
    AW_window  *pop_up_window;
    void (*f)(AW_window*,AW_CL ,AW_CL);
    AW_window  *aw;
    const char *help_text;
    char       *id;

    // ************ The real public section *************
    AW_cb_struct(AW_window    *awi,
                 void (*g)(AW_window*,AW_CL ,AW_CL),
                 AW_CL         cd1i       = 0,
                 AW_CL         cd2i       = 0,
                 const char   *help_texti = 0,
                 AW_cb_struct *next       = 0);

    void    run_callback(void); // runs the whole list
    AW_BOOL contains(void (*g)(AW_window*,AW_CL ,AW_CL)); // test if contained in list
};


enum {
    AWM_ALL = -1,
    AWM_EXP = 1,
    AWM_TREE = 2,       // Basic
    AWM_TREE2 = 4,
    AWM_PRO = 8,        // Protein data
    AWM_SEC = 16,       // Secundaerystructure
    AWM_SEQ = 32,       // Sequence Basic
    AWM_SEQ2 = 64,
    AWM_PRB = 0x100,        // probe design
    AWM_TUM = 0x200,        // W. Ludwig Specials
    AWM_BASIC = 2 + 32
};


typedef char *AW_pixmap;
typedef struct GBDATA_SET_STRUCT GBDATA_SET;

class AW_window_Motif;

struct AW_select_table_struct;

class AW_selection_list {
    AW_select_table_struct *loop_pntr;
public:
    AW_selection_list( const char *variable_namei, int variable_typei, Widget select_list_widgeti );

    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            select_list_widget;
    bool              value_equal_display; // set true to fix load/save of some selection lists

    AW_select_table_struct *list_table;
    AW_select_table_struct *last_of_list_table;
    AW_select_table_struct *default_select;
    AW_selection_list      *next;

    // ******************** real public ***************
    void selectAll();
    void deselectAll();
    const char *first_selected();
    const char *first_element();
    const char *next_element();
};

struct AW_option_menu_struct;
struct aw_toggle_data;

class AW_window {
private:
    void all_menus_created();
    void create_toggle(const char *var_name, aw_toggle_data *tdata);
    
protected:
    AW_root *root;

    void check_at_pos( void );
    void create_devices(void);
    void set_background(const char *colorname, Widget w);
    
public:

    // ************ This is not the public section *************

    AW_window_Motif *p_w;       // Do not use !!!
    AW_at           *_at;
    AW_cb_struct    *_callback;
    AW_cb_struct    *_d_callback;

    AW_window();
    virtual ~AW_window();


    AW_event       event;
    unsigned long  click_time;
    long           color_table_size;
    unsigned long *color_table;

    int recalc_size_at_show;
    // 0 = do not resize
    // 1 = do resize to default size
    // 2 = do resize to user size (or default size if that is bigger)

    int number_of_timed_title_changes;

    void /*AW_xfig*/    *xfig_data;

    void create_window_variables( void );
    void show_grabbed(void);
    void set_window_title_intern( char *title );

    int calculate_string_width( int columns );
    int calculate_string_height( int columns, int offset );
    char *align_string( const char *string, int columns );

    void update_label( int *widget, const char *var_value );
    void update_toggle( int *widget, const char *var_value, AW_CL cd );
    void update_input_field( int *widget, const char *var_value );
    void update_text_field( int *widget, const char *var_value );

    void create_invisible( int columns );
    // void update_option_menu( int option_menu_number );
    void *_create_option_entry(AW_VARIABLE_TYPE type, const char *name, const char *mnemonic,const char *name_of_color );
    void update_toggle_field( int toggle_field_number );
    void update_selection_list_intern( AW_selection_list *selection_list );
    void _set_activate_callback(void *widget);
    void unset_at_commands( void );
    void increment_at_commands( int width, int height );


    AW_color    alloc_named_data_color(int colnum, char *colorname);
    const char *GC_to_RGB(AW_device *device, int gc, int& red, int& green, int& blue); // returns colors in result-parameters or error message in return value
    // Converts GC to RGB float values to the range (0 - 1.0)
    const char *GC_to_RGB_float(AW_device *device, int gc, float& red, float& green, float& blue);
    void        _get_area_size(AW_area area, AW_rectangle *square);
    int         label_widget( void *wgt, AW_label str, char *mnemonic=0 , int width =0 , int alignment =0 );
    //***************************** *********************** ***********************************
    //***************************** The read only   section ***********************************
    //***************************** *********************** ***********************************
    char       *window_name;    // window title
    char       *window_defaults_name;
    AW_BOOL     window_is_shown;

    int left_indent_of_horizontal_scrollbar;
    int top_indent_of_vertical_scrollbar;
    int bottom_indent_of_vertical_scrollbar;
    int slider_pos_vertical;
    int slider_pos_horizontal;
    int main_drag_gc;

    AW_rectangle *picture;      // the result of tell scrolled
                                // picture size

    //***************************** *********************** ***********************************
    //***************************** The real public section ***********************************
    //***************************** *********************** ***********************************
    AW_root *get_root(void) { return root; };

    //******************* Global layout functions **********************
    void    show(void);         // bring hidden window to front of screen
    void    hide(void);         // hide (dont destroy) a window (<->show)
    AW_BOOL get_show(void);     // is window shown (== AW_TRUE) or hidden (== AW_FALSE)
    void    message( char *title, int ms ); // Set for ms milliseconds the title of the window
    void    set_window_title( const char *title ); // Set the window title forever
    void    set_icon( const char *pixmap_name = 0,const char *default_icon = 0); // Set the Pixmap

    const char *get_window_title( void ); // Get the window's title
    const char *get_window_id() const { return window_defaults_name; } // Get the window's internal name


    void set_info_area_height(int height);
    void set_bottom_area_height(int height);

    //******************* Input and Motion Events **********************

    void set_popup_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_focus_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);

    void set_expose_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_resize_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_input_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_motion_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1=0, AW_CL cd2=0);
    void set_double_click_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1=0, AW_CL cd2=0);

    AW_BOOL is_expose_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_resize_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_input_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_motion_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_double_click_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL));

    void get_event(AW_event *eventi);       // In an event callback get the events info

    //******************* Get the devices **********************
    AW_device *get_device (AW_area area);
    AW_device *get_click_device     (AW_area area, int mousex,int mousey, AW_pos max_distance_linei,
                                     AW_pos max_distance_texti, AW_pos radi);
    AW_device *get_size_device  (AW_area area);
    AW_device *get_print_device (AW_area area);

    // ************** Create the menu buttons *********
    void create_menu(   const char *id, AW_label name, const char *mnemonic, const char *help_text = 0, AW_active mask = -1);
    void insert_sub_menu(   const char *id, AW_label name, const char *mnemonic, const char *help_text = 0, AW_active mask = -1);
    void insert_menu_topic( const char *id, AW_label name, const char *mnemonic, const char *help_text, AW_active mask, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void close_sub_menu(void);

    void insert_separator(void);
    void insert_help_topic(const char *id, AW_label name, const char *mnemonic, const char *help_text, AW_active mask, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void insert_separator_help(void);
    // ************** Create modes on the left side ******************
    int create_mode(const char *id, const char *pixmap, const char *help_text, AW_active mask, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void select_mode(int mode);


    // ************** Control the size of the main drawing area + scrollbars  *********
    void tell_scrolled_picture_size(AW_rectangle rectangle);
    void tell_scrolled_picture_size(AW_world rectangle);
    AW_pos get_scrolled_picture_width();
    AW_pos get_scrolled_picture_height();
    void reset_scrolled_picture_size();

    void calculate_scrollbars(void);
    void set_vertical_scrollbar_position(int position);
    void set_horizontal_scrollbar_position(int position);
    void set_vertical_change_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_horizontal_change_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_horizontal_scrollbar_left_indent(int indent);
    void set_vertical_scrollbar_top_indent(int indent);
    void set_vertical_scrollbar_bottom_indent(int indent);


    // ************** Control window size  *********
    void set_window_size( int width, int height );
    void get_window_size( int& width, int& height );
    void window_fit(void);      // Recalculate the size of a window with buttons

    // ************** ********************************************************************  *********
    // ************** Create buttons: First set modify flags and finally create the button  *********
    // ************** ********************************************************************  *********

    // *** global modifier: ****
    void load_xfig(const char *file, AW_BOOL resize=AW_TRUE); // Loads the background graphic
    void draw_line(int x1, int y1, int x2, int y2, int width, AW_BOOL resize); // draws a line on the background

    void label_length( int length ); // Justifies all following labels
    void button_length( int length ); // Sets the width of all following buttons
    int  get_button_length() const; // returns the current width of buttons
    void highlight( void );     // Creates a frame around the button
    void auto_increment( int dx, int dy ); // enable automatic placement of buttons
    // dx is the horizontal distance between the left
    // borders of two buttons
    void auto_space( int xspace, int yspace ); // enable automatic placement of buttons
    // xspace is the horizontal space between 2 buttons

    void auto_off( void );      // disable auto_xxxxx
    void shadow_width (int shadow_thickness); // set the shadow_thickness of buttons

    void TuneBackground(Widget w,int modStrength);
    void TuneOrSetBackground(Widget w, const char *color, int modStrength);

    // *** local modifiers: ********
    void at( int x, int y );    // abs pos of a button (>10,10)
    void at_x( int x );         // abs x pos
    void at_y( int y );         // abs y pos
    void at_shift( int x, int y ); // rel pos of a button
    void at_newline( void );    // in auto_space mode only: newline

    void    at( const char *id ); // place the button at the position set in the .fig
    // file (loaded with load_xfig) by the string $id
    AW_BOOL at_ifdef(const  char *id); // check whether 'id' is an element if the .fig file

    void label( const char *label ); // Create a label before the button

    void get_at_position(int *x, int *y);
    int get_at_xposition();
    int get_at_yposition();

    void dump_at_position(const char *debug_label) const; // for debugging (uses printf)

    void at_attach(AW_BOOL attach_x, AW_BOOL attach_y); // attach to X, Y or both
    void at_set_to(AW_BOOL attach_x, AW_BOOL attach_y, int xoff, int yoff); // set "to:XY:id" manually
    void at_unset_to();         // unset "to:id" manually
    void at_set_min_size(int xmin, int ymin); // define minimum window size

    void store_at_size_and_attach( AW_at_size *at_size ); // get size of at-element
    void restore_at_size_and_attach( const AW_at_size *at_size ); // set size of a at-element

    void id( const char *id );  // Set the id of the button (for set_sensitive)
    void mask( AW_active mask ); // Set the mask used for set_sensitive
    void help_text(const char *id ); // Set the help text of a button
    void callback( void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 ); // normal callbacks
    void callback( void (*f)(AW_window*,AW_CL), AW_CL cd1);
    void callback( void (*f)(AW_window*));
    void callback( AW_cb_struct * /*owner*/awcbs); // Calls f with
    // aww in awcbs
    void d_callback( void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 ); // double click callbacks
    void d_callback( void (*f)(AW_window*,AW_CL), AW_CL cd1); // selection lists only !!
    void d_callback( void (*f)(AW_window*));
    void d_callback( AW_cb_struct * /*owner*/awcbs); // Calls f with
    // *** create the buttons ********

    //    void   create_button( const char *macro_name, AW_label label,const char *mnemonic = 0); // simple button; shadow only when callback
    void   create_button( const char *macro_name, AW_label label,const char *mnemonic = 0, const char *color = 0); // simple button; shadow only when callback
    void   create_autosize_button( const char *macro_name, AW_label label,const char *mnemonic = 0, unsigned xtraSpace = 1); // as above, but ignores button_length
    Widget get_last_button_widget() const;

    void create_toggle( const char *awar_name); // int 0/1  string yes/no   float undef
    void create_inverse_toggle( const char *awar_name); // like create_toggle, but displays inverted toggle value

    void create_toggle( const char *awar_name, const char *nobitmap,const char *yesbitmap, int buttonWidth = 0);
    void create_text_toggle(const char *var_name, const char *noText, const char *yesText, int buttonWidth = 0);

    void create_input_field( const char *awar_name, int columns = 0 ); // One line textfield
    void create_text_field( const char *awar_name, int columns = 20, int rows = 4 ); // Multi line textfield
    // with scrollbars


    // ***** option_menu is a menu where only one selection is visible at a time
    AW_option_menu_struct *create_option_menu( const char *awar_name, AW_label label=0,const char *mnemonic=0 );
    void clear_option_menu( AW_option_menu_struct *oms); // used to redefine available options

private:
    void insert_option_internal( AW_label choice_label, const char *mnemonic,const char *var_value,  const char *name_of_color, bool default_option);
    void insert_option_internal( AW_label choice_label, const char *mnemonic,int var_value,          const char *name_of_color, bool default_option);
    void insert_option_internal( AW_label choice_label, const char *mnemonic,float var_value,        const char *name_of_color, bool default_option);
public: 
    
    void insert_option         ( AW_label choice_label, const char *mnemonic, const char *var_value, const char *name_of_color = 0); // for string
    void insert_default_option ( AW_label choice_label, const char *mnemonic, const char *var_value, const char *name_of_color = 0 );
    void insert_option         ( AW_label choice_label, const char *mnemonic, int var_value,         const char *name_of_color = 0 ); // for int
    void insert_default_option ( AW_label choice_label, const char *mnemonic, int var_value,         const char *name_of_color = 0 );
    void insert_option         ( AW_label choice_label, const char *mnemonic, float var_value,       const char *name_of_color = 0 ); // for float
    void insert_default_option ( AW_label choice_label, const char *mnemonic, float var_value,       const char *name_of_color = 0 );
    
    void update_option_menu( void );
    void update_option_menu( AW_option_menu_struct *); // dont use this


    // ***** toggle_field is a static menu (all items are visible and only one is selected)
    void create_toggle_field( const char *awar_name, AW_label label, const char *mnemonic );
    void create_toggle_field( const char *awar_name, int orientation = 0 );// 1 = horizontal
    // for string
    void insert_toggle( AW_label toggle_label, const char *mnemonic, const char *var_value );
    void insert_default_toggle( AW_label toggle_label, const char *mnemonic, const char *var_value );
    // for int
    void insert_toggle( AW_label toggle_label, const char *mnemonic, int var_value );
    void insert_default_toggle( AW_label toggle_label, const char *mnemonic, int var_value );
    // for float
    void insert_toggle( AW_label toggle_label, const char *mnemonic, float var_value );
    void insert_default_toggle( AW_label toggle_label, const char *mnemonic, float var_value );
    void update_toggle_field( void );

    // ***** selection list is a redefinable scrolled list of items

    AW_selection_list *create_selection_list( const char *awar_name, const char *label = 0, const char *mnemonic = 0, int columns = 4, int rows = 4 );
    // to make a copy of the selection list
    AW_selection_list *copySelectionList(AW_selection_list *sourceList, AW_selection_list *destinationList);
    // for string
    void insert_selection( AW_selection_list *selection_list, const char *displayed, const char *value );
    void insert_default_selection(  AW_selection_list * selection_list, const char *displayed, const char *value );
    // for int
    void insert_selection(  AW_selection_list * selection_list, const char *displayed, long value );
    void insert_default_selection(  AW_selection_list * selection_list, const char *displayed, long value );
    // for float
    void insert_selection(  AW_selection_list * selection_list, const char *displayed, float value );
    void insert_default_selection(  AW_selection_list * selection_list, const char *displayed, float value );

    void delete_selection_from_list(  AW_selection_list * selection_list, const char *displayed_string );
    void conc_list( AW_selection_list * from_list_id,  AW_selection_list * to_list_id);

    // --- selection list iterator:
    void        init_list_entry_iterator(AW_selection_list *selection_list);
    void        iterate_list_entry(int offset);
    // --- the following functions work on the currently iterated element:
    const char *get_list_entry_char_value();
    const char *get_list_entry_displayed();
    void        set_list_entry_char_value(const char *new_char_value);
    void        set_list_entry_displayed(const char *new_displayed);
    // ---------------------------------------------------------

    void clear_selection_list(  AW_selection_list * selection_list );
    void update_selection_list(  AW_selection_list * selection_list );

    int   get_no_of_entries(  AW_selection_list * selection_list );
    int   get_index_of_element(AW_selection_list *selection_list, const char *selected_element);
    char *get_element_of_index(AW_selection_list *selection_list, int  index);

    int  get_index_of_current_element(AW_selection_list *selection_list, const char *awar_name);
    void select_index(AW_selection_list *selection_list, const char *awar_name, int wanted_index);
    void  move_selection(AW_selection_list *selection_list, const char *awar_name, int offset);


    char       *get_selection_list_contents( AW_selection_list * selection_list, long nr_of_lines = -1);
    void        sort_selection_list(  AW_selection_list * selection_list, int backward, int case_sensitive);
    GB_ERROR    save_selection_list(  AW_selection_list * selection_list, const char *filename,long number_of_lines = 0);
    void        set_selection_list_suffix( AW_selection_list * selection_list, const char *suffix);
    GB_ERROR    load_selection_list(  AW_selection_list * selection_list, const char *filename);
    GBDATA_SET *selection_list_to_species_set(GBDATA *gb_main,AW_selection_list *selection_list);

    AW_selection_list * create_multi_selection_list( const char *label = 0, const char *mnemonic = 0, int columns = 4, int rows = 4 );
    // all works, but its not connected to an AWAR may be converted to AW_mselection_list_helper
};


class AW_window_menu_modes : public AW_window {
private:
    void    *AW_window_menu_modes_private;    // Do not use !!!
public:
    AW_window_menu_modes(void);
    ~AW_window_menu_modes(void);
    void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
};

/// Extended by Daniel Koitzsch & Christian Becker 19-05-04
class AW_window_menu_modes_opengl : public AW_window_menu_modes {
private:
    void    *AW_window_menu_modes_private;    // Do not use !!!
public:
    AW_window_menu_modes_opengl(void);
    ~AW_window_menu_modes_opengl(void);
    virtual void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
	//void create_window_variables( void ) {
	//}

    /*void calculate_scrollbars(void) {}
    void set_vertical_scrollbar_position(int position) {}
    void set_horizontal_scrollbar_position(int position) {}
    void set_vertical_change_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {}
    void set_horizontal_change_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {}
    void set_horizontal_scrollbar_left_indent(int indent) {}
    void set_vertical_scrollbar_top_indent(int indent){}
    void set_vertical_scrollbar_bottom_indent(int indent) {}*/
};


class AW_window_menu : public AW_window {
private:
public:
    AW_window_menu(void);
    ~AW_window_menu(void);
    void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
};

class AW_window_simple_menu : public AW_window {
private:
public:
    AW_window_simple_menu(void);
    ~AW_window_simple_menu(void);
    void init(AW_root *root, const char *wid, const char *windowname);
};


class AW_window_simple : public AW_window {
private:
public:
    AW_window_simple(void);
    ~AW_window_simple(void);
    void init(AW_root *root, const char *wid, const char *windowname);
};


class AW_window_message : public AW_window {
private:
public:
    AW_window_message(void);
    ~AW_window_message(void);
    void init(AW_root *root, const char *windowname, bool allow_close);
};


typedef void* AW_gc_manager;

class AW_detach_information {
    Awar_Callback_Info *cb_info;
    Widget              detach_button;
public:
    AW_detach_information(Awar_Callback_Info *cb_info_)
        : cb_info(cb_info_) , detach_button(0) { }

    Awar_Callback_Info *get_cb_info() { return cb_info; }
    Widget get_detach_button() { aw_assert(detach_button); return detach_button; }
    void set_detach_button(Widget w) { detach_button = w; }
};

#else
#error aw_window.hxx included twice
#endif
