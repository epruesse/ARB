#ifndef AW_WINDOW_XM_HXX
#define AW_WINDOW_XM_HXX

#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif
#ifndef AW_KEYSYM_HXX
#include <aw_keysym.hxx>
#endif
#ifndef _Xm_h
#include <Xm/Xm.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif
#ifndef AW_SCALAR_HXX
#include "aw_scalar.hxx"
#endif
#ifndef ARB_MSG_H
#include <arb_msg.h>
#endif

// macro definitions
#define  p_r        prvt
#define  p_global   (root->prvt)
#define  p_aww(aww) (aww->p_w)

#define MAP_ARAM(ar) p_w->areas[ar]

#define INFO_WIDGET p_w->areas[AW_INFO_AREA]->get_area()
#define INFO_FORM p_w->areas[AW_INFO_AREA]->get_form()
#define MIDDLE_WIDGET p_w->areas[AW_MIDDLE_AREA]->get_area()
#define BOTTOM_WIDGET p_w->areas[AW_BOTTOM_AREA]->get_area()

#define AW_SCROLL_MAX 100
#define AW_MAX_MENU_DEEP 10

#define RES_CONVERT(res_name, res_value)  \
    XtVaTypedArg, (res_name), XmRString, (res_value), strlen(res_value) + 1

#define AW_MOTIF_LABEL

class Label : virtual Noncopyable {
    char *label;
    int   len;
    bool  imageref;
public:
    Label(const char *labeltext, AW_window *aww);
    ~Label() { free(label); }

    size_t length() const { return len; }
    const char *String() const { return label; }
    bool is_imageref() const { return imageref; }
};


#define RES_LABEL_CONVERT(label)                                                \
    XmNlabelType, label.is_imageref() ? XmPIXMAP : XmSTRING,                    \
    XtVaTypedArg, label.is_imageref() ? XmNlabelPixmap : XmNlabelString,        \
    XmRString,                                                                  \
    label.String(),                                                             \
    label.length()+1

#define AW_JUSTIFY_LABEL(widget, corr)                                                \
    switch (corr) {                                                                   \
        case 1: XtVaSetValues(widget, XmNalignment, XmALIGNMENT_CENTER, NULL); break; \
        case 2: XtVaSetValues(widget, XmNalignment, XmALIGNMENT_END, NULL); break;    \
        default: break;                                                               \
    }


struct AW_buttons_struct : virtual Noncopyable {
    AW_buttons_struct(AW_active maski, Widget w, AW_buttons_struct *next);
    ~AW_buttons_struct();

    AW_active          mask;
    Widget             button;
    AW_buttons_struct *next;
};

struct AW_widget_value_pair : virtual Noncopyable {
    template<typename T> explicit AW_widget_value_pair(T t, Widget w) : value(t), widget(w), next(NULL) {}
    ~AW_widget_value_pair() { aw_assert(!next); } // has to be unlinked from list BEFORE calling dtor

    AW_scalar value;
    Widget    widget;

    AW_widget_value_pair *next;
};

struct AW_option_menu_struct {
    AW_option_menu_struct(int numberi, const char *variable_namei, AW_VARIABLE_TYPE variable_typei, Widget label_widgeti, Widget menu_widgeti, AW_pos xi, AW_pos yi, int correct);

    int               option_menu_number;
    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            label_widget;
    Widget            menu_widget;

    AW_widget_value_pair *first_choice;
    AW_widget_value_pair *last_choice;
    AW_widget_value_pair *default_choice;
    
    AW_pos x;
    AW_pos y;
    int    correct_for_at_center_intern;            // needed for centered and right justified menus (former member of AW_at)

    AW_option_menu_struct *next;
};


struct AW_toggle_field_struct {
    AW_toggle_field_struct(int toggle_field_numberi, const char *variable_namei, AW_VARIABLE_TYPE variable_typei, Widget label_widgeti, int correct);

    int               toggle_field_number;
    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            label_widget;

    AW_widget_value_pair *first_toggle;
    AW_widget_value_pair *last_toggle;
    AW_widget_value_pair *default_toggle;
    
    int correct_for_at_center_intern;                   // needed for centered and right justified toggle fields (former member of AW_at)

    AW_toggle_field_struct *next;
};


// -------------------------------------------
//      area management: devices callbacks

class AW_common_Xm;
class AW_device_Xm;
class AW_cb;

class AW_area_management {
    Widget form; // for resizing
    Widget area; // for displaying additional information

    AW_common_Xm *common;

    AW_device_Xm    *device;
    AW_device_size  *size_device;
    AW_device_print *print_device;
    AW_device_click *click_device;

    AW_cb *expose_cb;
    AW_cb *resize_cb;
    AW_cb *double_click_cb;

    long click_time;

public:
    AW_area_management(Widget form, Widget widget);

    Widget get_form() const { return form; }
    Widget get_area() const { return area; }

    AW_common_Xm *get_common() const { return common; }

    AW_device_Xm *get_screen_device();
    AW_device_size *get_size_device();
    AW_device_print *get_print_device();
    AW_device_click *get_click_device();

    void create_devices(AW_window *aww, AW_area ar);

    void set_expose_callback(AW_window *aww, const WindowCallback& cb);
    void set_resize_callback(AW_window *aww, const WindowCallback& cb);

    void set_input_callback(AW_window *aww, const WindowCallback& wcb);
    void set_double_click_callback(AW_window *aww, const WindowCallback& wcb);
    void set_motion_callback(AW_window *aww, const WindowCallback& wcb);

    bool is_expose_callback(AnyWinCB f);
    bool is_resize_callback(AnyWinCB f);

    void run_expose_callback();
    void run_resize_callback();

    AW_cb *get_double_click_cb() { return double_click_cb; }
    long get_click_time() const { return click_time; }
    void set_click_time(long click_time_) { click_time = click_time_; }
};

class AW_selection_list;

class AW_root_Motif : virtual Noncopyable {
    Widget           last_widget;                   // last created (sensitive) widget
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

    AW_option_menu_struct *option_menu_list;
    AW_option_menu_struct *last_option_menu;
    AW_option_menu_struct *current_option_menu;

    AW_toggle_field_struct *toggle_field_list;
    AW_toggle_field_struct *last_toggle_field;

    AW_selection_list *selection_list;
    AW_selection_list *last_selection_list;

    int       screen_depth;
    AW_rgb   *color_table;
    Colormap  colormap;

    int      help_active;
    Cursor   clock_cursor;
    Cursor   question_cursor;
    Display *old_cursor_display;
    Window   old_cursor_window;
    bool     no_exit;

    GB_HASH *action_hash;

    AW_root_Motif();
    ~AW_root_Motif();

    Widget get_last_widget() const { return last_widget; }
    void set_last_widget(Widget w) { last_widget = w; }

    void set_cursor(Display *d, Window w, Cursor c);
    void normal_cursor();
};

const int AW_NUMBER_OF_F_KEYS = 20;
#define AW_CALC_OFFSET_ON_EXPOSE -12345

class AW_window_Motif : virtual Noncopyable {
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

    AW_cb **modes_f_callbacks;
    Widget        *modes_widgets;
    int            selected_mode;
    AW_cb  *popup_cb;
    Widget         frame;

    Widget            toggle_field;
    Widget            toggle_label;
    char             *toggle_field_var_name;
    AW_VARIABLE_TYPE  toggle_field_var_type;
    AW_key_mod        keymodifier;

    AW_area_management *areas[AW_MAX_AREA];

    int WM_top_offset;                 // correction between position set and position reported ( = size of window frame - in most cases!)
    int WM_left_offset;

    static int WM_max_top_offset;
    static int WM_max_left_offset;

    bool knows_WM_offset() const { return WM_top_offset != AW_CALC_OFFSET_ON_EXPOSE; }

    AW_window_Motif();
    ~AW_window_Motif() {};
};


void aw_root_init_font(Display *tool_d);
void AW_label_in_awar_list(AW_window *aww, Widget widget, const char *str);
void AW_server_callback(Widget wgt, XtPointer aw_cb_struct, XtPointer call_data);

// ------------------------------------------------------------
// do not use the following
extern int aw_message_cb_result;
#define AW_MESSAGE_LISTEN_DELAY 500 // look in ms whether a father died
void message_cb(AW_window*, int result);
void input_cb(AW_window *aw, int buttonNr);
void input_history_cb(AW_window *aw, int mode);
void file_selection_cb(AW_window *aw, int ok_cancel_flag);
unsigned aw_message_timer_listen_event(AW_root *awr, AW_window *aww);
// ------------------------------------------------------------

Widget aw_create_shell(AW_window *aww, bool allow_resize, bool allow_close, int width, int height, int posx, int posy);
void   aw_realize_widget(AW_window *aww);
void   aw_insert_default_help_entries(AW_window *aww);

__ATTR__NORETURN inline void type_mismatch(const char *triedType, const char *intoWhat) {
    GBK_terminatef("Cannot insert %s into %s which uses a non-%s AWAR", triedType, intoWhat, triedType);
}

#else
#error aw_window_Xm.hxx included twice
#endif
