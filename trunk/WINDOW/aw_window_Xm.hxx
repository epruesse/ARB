#ifndef AW_WINDOW_XM_HXX
#define AW_WINDOW_XM_HXX

// Makrodefinitionen
#define  p_r        prvt
#define  p_global   (root->prvt)
#define  p_aww(aww) (aww->p_w)

#define AW_INSERT_BUTTON_IN_SENS_LIST(root,id,mask,widget)              \
    do { new AW_buttons_struct(root,id,mask,widget); } while (0)

bool AW_remove_button_from_sens_list(AW_root *aw_root, Widget w);

#define MAP_ARAM(ar) p_w->areas[ar]

#define INFO_WIDGET p_w->areas[AW_INFO_AREA]->area
#define INFO_FORM p_w->areas[AW_INFO_AREA]->form
#define MIDDLE_WIDGET p_w->areas[AW_MIDDLE_AREA]->area
#define BOTTOM_WIDGET p_w->areas[AW_BOTTOM_AREA]->area

#define AW_SCROLL_MAX 100
#define AW_MAX_MENU_DEEP 10

#define RES_CONVERT( res_name, res_value) \
    XtVaTypedArg, (res_name), XmRString, (res_value), strlen(res_value) + 1

#define AW_MOTIF_LABEL

#define RES_LABEL_CONVERT(str) \
    XmNlabelType, (str[0]=='#')?XmPIXMAP:XmSTRING,\
    XtVaTypedArg, (str[0]=='#')?XmNlabelPixmap:XmNlabelString, \
    XmRString, \
    aw_str_2_label(str,this), \
    strlen(aw_str_2_label(str,this))+1

#define RES_LABEL_CONVERT2(str,aww) \
    XmNlabelType, (str[0]=='#')?XmPIXMAP:XmSTRING,\
    XtVaTypedArg, (str[0]=='#')?XmNlabelPixmap:XmNlabelString, \
    XmRString, \
    aw_str_2_label(str,aww), \
    strlen(aw_str_2_label(str,aww))+1


#define AW_JUSTIFY_LABEL(widget,corr) \
    switch(corr){\
        case 1: XtVaSetValues( widget,XmNalignment,XmALIGNMENT_CENTER,NULL);break;\
        case 2: XtVaSetValues( widget,XmNalignment,XmALIGNMENT_END,NULL);break;\
        default: break;\
    }


/*************************************************************************/
struct AW_timer_cb_struct {
    AW_timer_cb_struct(AW_root *ari, void (*g)(AW_root*,AW_CL ,AW_CL), AW_CL cd1i, AW_CL cd2i);
    ~AW_timer_cb_struct(void);
    AW_root *ar;
    void (*f)(AW_root*,AW_CL ,AW_CL);
    AW_CL cd1;
    AW_CL cd2;
};



/********************************* Used for single items in lists ****************************************/
struct AW_variable_update_struct {
    AW_variable_update_struct( Widget widgeti, AW_widget_type widget_typei, AW_awar *awari, const char *var_s_i, int var_i_i, float var_f_i, AW_cb_struct *cbsi );
    AW_awar        *awar;
    Widget          widget;
    AW_widget_type  widget_type;
    char           *variable_value;
    long            variable_int_value;
    float           variable_float_value;
    AW_cb_struct   *cbs;
    void           *id;         // selection id etc ...
};


/************************************************************************/
struct AW_buttons_struct {
    AW_buttons_struct(AW_root *rooti, const char *idi, AW_active maski, Widget w);
    ~AW_buttons_struct();

    char              *id;
    AW_active          mask;
    Widget             button;
    AW_buttons_struct *next;
};


/************************************************************************/
struct AW_config_struct {
    AW_config_struct( const char *idi, AW_active maski, Widget w, const char *variable_namei, const char *variable_valuei, AW_config_struct *nexti );

    const char       *id;
    AW_active         mask;
    Widget            widget;
    const char       *variable_name;
    const char       *variable_value;
    AW_config_struct *next;
};


struct AW_option_struct {
    AW_option_struct( const char *variable_valuei, Widget choice_widgeti );
    AW_option_struct( int variable_valuei, Widget choice_widgeti );
    AW_option_struct( float variable_valuei, Widget choice_widgeti );
    ~AW_option_struct();

    char             *variable_value;
    int               variable_int_value;
    float             variable_float_value;
    Widget            choice_widget;
    AW_option_struct *next;
};

struct AW_option_menu_struct {
    AW_option_menu_struct( int numberi, const char *unique_option_menu_namei, const char *variable_namei, AW_VARIABLE_TYPE variable_typei, Widget label_widgeti, Widget menu_widgeti, AW_pos xi, AW_pos yi, int correct);

    int               option_menu_number;
    char             *unique_option_menu_name;
    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            label_widget;
    Widget            menu_widget;
    AW_option_struct *first_choice;
    AW_option_struct *last_choice;
    AW_option_struct *default_choice;
    AW_pos            x;
    AW_pos            y;
    int               correct_for_at_center_intern; // needed for centered and right justified menus (former member of AW_at)

    AW_option_menu_struct *next;
};


struct AW_toggle_struct {

    AW_toggle_struct( const char *variable_valuei, Widget choice_widgeti );
    AW_toggle_struct( int variable_valuei, Widget choice_widgeti );
    AW_toggle_struct( float variable_valuei, Widget choice_widgeti );

    char   *variable_value;
    int     variable_int_value;
    float   variable_float_value;
    Widget  toggle_widget;

    AW_toggle_struct *next;

};

struct AW_toggle_field_struct {

    AW_toggle_field_struct( int toggle_field_numberi, const char *variable_namei, AW_VARIABLE_TYPE variable_typei, Widget label_widgeti, int correct);

    int               toggle_field_number;
    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            label_widget;
    AW_toggle_struct *first_toggle;
    AW_toggle_struct *last_toggle;
    AW_toggle_struct *default_toggle;
    int               correct_for_at_center_intern; // needed for centered and right justified toggle fields (former member of AW_at)

    AW_toggle_field_struct *next;

};


struct AW_select_table_struct {

    AW_select_table_struct( const char *displayedi, const char *value );
    ~AW_select_table_struct( void );
    AW_select_table_struct( const char *displayedi, long value );
    AW_select_table_struct( const char *displayedi, float value );

    static char *copy_string(const char *str);

    char                   *displayed;
    char                   *char_value;
    long                    int_value;
    float                   float_value;
    int                     is_selected; // internal use only
    AW_select_table_struct *next;
};



/*****************************************************************************************************
        area management:    devices callbacks
*****************************************************************************************************/
class AW_area_management {
    friend class AW_window;
    friend class AW_window_simple;
    friend class AW_window_simple_menu;
    friend class AW_window_menu;
    friend class AW_window_menu_modes;
    friend class AW_window_message;

public:
    Widget           form;      // for resizing
    Widget           area;      // for displaying additional information
    AW_common       *common;
    AW_device_Xm    *device;
    AW_device_click *click_device;
    AW_device_size  *size_device;
    AW_device_print *print_device;

    AW_cb_struct *expose_cb;
    AW_cb_struct *resize_cb;
    int           do_resize_if_expose;
    AW_cb_struct *double_click_cb;
    long          click_time;

    void create_devices(AW_window *aww, AW_area ar);

    void set_expose_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_resize_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_input_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_double_click_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);
    void set_motion_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2);

    AW_BOOL is_expose_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_resize_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_input_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_double_click_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL));
    AW_BOOL is_motion_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL));

    AW_area_management(AW_root *awr,Widget form,Widget widget);
};
/************************************************************************/
class AW_root_Motif {
private:
protected:
public:
    Display         *display;
    XtAppContext     context;
    Widget           toplevel_widget;
    Widget           main_widget;
    class AW_window *main_aww;
    Widget           message_shell;

    Pixel foreground;
    Pixel background;

    XmFontList fontlist;

    AW_buttons_struct *button_list;
    AW_buttons_struct *last_button;

    AW_config_struct *config_list;
    AW_config_struct *last_config;

    AW_option_menu_struct *option_menu_list;
    AW_option_menu_struct *last_option_menu;
    AW_option_menu_struct *current_option_menu;

    AW_toggle_field_struct *toggle_field_list;
    AW_toggle_field_struct *last_toggle_field;

    AW_selection_list *selection_list;
    AW_selection_list *last_selection_list;

    int            screen_depth;
    unsigned long *color_table;
    Colormap       colormap;

    int      help_active;
    Cursor   clock_cursor;
    Cursor   question_cursor;
    Display *old_cursor_display;
    Window   old_cursor_window;
    void     normal_cursor(void);
    void     set_cursor(Display *d, Window w, Cursor c);
    AW_BOOL  no_exit;

    char    *recording_macro_path;
    FILE    *recording_macro_file;
    char    *application_name_for_macros;
    char    *stop_action_name;
    FILE    *executing_macro_file;
    GB_HASH *action_hash;

    AW_root_Motif() {};
    ~AW_root_Motif() {};

    Widget get_last_button_widget() { return last_button ? last_button->button : 0; }
};

// void create_help_entry(AW_window *aww);


/**********************************************************************/
const int AW_NUMBER_OF_F_KEYS = 20;

class AW_window_Motif {
public:

    Widget shell;               // for show, hide
    Widget scroll_bar_vertical; // for scrolling the
    Widget scroll_bar_horizontal; // draw area
    Widget menu_bar[AW_MAX_MENU_DEEP]; // for inserting topics to menu bar
    int    menu_deep;
    Widget help_pull_down;      // for inserting subtopics to the help pull_down
    Widget mode_area;           // for inserting buttons to the mode area
    short  number_of_modes;     // holds number of mode buttons in window

    // ********** local modifiers **********

    AW_cb_struct **modes_f_callbacks;
    Widget        *modes_widgets;
    int            selected_mode;
    AW_cb_struct  *popup_cb;
    Widget         frame;

    Widget            toggle_field;
    Widget            toggle_label;
    char             *toggle_field_var_name;
    AW_VARIABLE_TYPE  toggle_field_var_type;
    AW_key_mod        keymodifier;

    AW_area_management *areas[AW_MAX_AREA];
    int                 WM_top_offset; // WM top area
    int                 WM_left_offset;

    AW_window_Motif();
    ~AW_window_Motif() {};
};


void        aw_root_init_font(Display *tool_d);
const char *aw_str_2_label(const char *str,AW_window *aww);
void        AW_label_in_awar_list(AW_window *aww,Widget widget,const char *str);
void        AW_server_callback(Widget wgt, XtPointer aw_cb_struct, XtPointer call_data);
void        aw_message_timer_listen_event(AW_root *awr, AW_CL cl1, AW_CL cl2);
void        message_cb( AW_window *aw, AW_CL cd1 );
// void        macro_message_cb( AW_window *aw, AW_CL cd1 );
void        input_cb( AW_window *aw, AW_CL cd1 );


#else
#error aw_window_Xm.hxx included twice
#endif
