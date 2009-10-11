#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <Xm/Xm.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <Xm/AtomMgr.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumn.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/MainW.h>
#include <Xm/CascadeB.h>
#include <Xm/MenuShell.h>
#include <Xm/ScrollBar.h>
#include <Xm/MwmUtil.h>

#include <arbdb.h>
#include <arbdbt.h>

/* Eigene Klassendefinition */
#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include "aw_keysym.hxx"
#include "aw_at.hxx"
#include "aw_window.hxx"
#include "aw_awar.hxx"
#include "aw_xfig.hxx"
#include "aw_xfigfont.hxx"
/* hier die Motif abhaengigen Teile */
#include "aw_Xm.hxx"
#include "aw_click.hxx"
#include "aw_size.hxx"
#include "aw_print.hxx"
#include "aw_window_Xm.hxx"
#include "aw_xkey.hxx"

#include "aw_global.hxx"

AW_root *AW_root::THIS= NULL;

AW_cb_struct::AW_cb_struct(AW_window *awi, void (*g)(AW_window*,AW_CL,AW_CL), AW_CL cd1i, AW_CL cd2i,
        const char *help_texti, class AW_cb_struct *nexti) {
    aw = awi;
    f = g;
    cd1 = cd1i;
    cd2 = cd2i;
    help_text = help_texti;
    pop_up_window = NULL;
    this->next = nexti;
}

AW_timer_cb_struct::AW_timer_cb_struct(AW_root *ari, void (*g)(AW_root*,AW_CL,AW_CL), AW_CL cd1i, AW_CL cd2i) {
    ar = ari;
    f = g;
    cd1 = cd1i;
    cd2 = cd2i;
}
AW_timer_cb_struct::~AW_timer_cb_struct(void) {
}

void AW_root::make_sensitive(Widget w, AW_active mask) {
    // Don't call make_sensitive directly!
    // 
    // Simply set sens_mask(AWM_EXP) and after creating the expert-mode-only widgets,
    // set it back using sens_mask(AWM_ALL)

    aw_assert(w);
    aw_assert(legal_mask(mask));
    
    prvt->set_last_widget(w);

    if (mask != AWM_ALL) { // no need to make widget sensitive, if its shown unconditionally
        prvt->button_list = new AW_buttons_struct(mask, w, prvt->button_list);
        if (!(mask & global_mask)) XtSetSensitive(w, False); // disable widget if mask doesn't match
    }
}

AW_buttons_struct::AW_buttons_struct(AW_active maski, Widget w, AW_buttons_struct *prev_button) {
    aw_assert(w);
    aw_assert(legal_mask(maski));

    mask     = maski;
    button   = w;
    next     = prev_button;
}

AW_buttons_struct::~AW_buttons_struct() {
    aw_assert(next == 0); // has to be removed from global list before calling dtor
}

bool AW_remove_button_from_sens_list(AW_root *root, Widget w) {
    bool removed = false;
    if (p_global->button_list) {
        AW_buttons_struct *prev = 0;
        AW_buttons_struct *bl   = p_global->button_list;

        while (bl) {
            if (bl->button == w) break; // found wanted widget
            prev = bl;
            bl = bl->next;
        }

        if (bl) {
            // remove from list
            if (prev) prev->next       = bl->next;
            else p_global->button_list = bl->next;

            bl->next = 0;
            removed  = true;
            
            delete bl;
        }
    }
    return removed;
}

AW_config_struct::AW_config_struct(const char *idi, AW_active maski, Widget w,
                                   const char *variable_namei, const char *variable_valuei,
                                   AW_config_struct *nexti)
{
    aw_assert(legal_mask(maski));
    
    id             = strdup(idi);
    mask           = maski;
    widget         = w;
    variable_name  = strdup(variable_namei);
    variable_value = strdup(variable_valuei);
    next           = nexti;
}

/*************************************************************************************************************/

AW_option_struct::AW_option_struct(const char *variable_valuei,
        Widget choice_widgeti) :
    variable_value(strdup(variable_valuei)), choice_widget(choice_widgeti),
            next(0) {
}

AW_option_struct::AW_option_struct(int variable_valuei, Widget choice_widgeti) :
    variable_value(0), variable_int_value(variable_valuei),
            choice_widget(choice_widgeti), next(0) {
}

AW_option_struct::AW_option_struct(float variable_valuei, Widget choice_widgeti) :
    variable_value(0), variable_float_value(variable_valuei),
            choice_widget(choice_widgeti), next(0) {
}

AW_option_struct::~AW_option_struct() {
    aw_assert(next == 0);
    // has to be unlinked from list BEFORE calling dtor
    free(variable_value);
}

AW_option_menu_struct::AW_option_menu_struct(int numberi, const char *variable_namei,
                                             AW_VARIABLE_TYPE variable_typei, Widget label_widgeti,
                                             Widget menu_widgeti, AW_pos xi, AW_pos yi, int correct) {
    option_menu_number = numberi;
    variable_name      = strdup(variable_namei);
    variable_type      = variable_typei;
    label_widget       = label_widgeti;
    menu_widget        = menu_widgeti;
    first_choice       = NULL;
    last_choice        = NULL;
    default_choice     = NULL;
    next               = NULL;
    x                  = xi;
    y                  = yi;

    correct_for_at_center_intern = correct;
}

AW_toggle_field_struct::AW_toggle_field_struct(int toggle_field_numberi,
        const char *variable_namei, AW_VARIABLE_TYPE variable_typei,
        Widget label_widgeti, int correct) {

    toggle_field_number = toggle_field_numberi;
    variable_name = strdup(variable_namei);
    variable_type = variable_typei;
    label_widget = label_widgeti;
    first_toggle = NULL;
    last_toggle = NULL;
    default_toggle = NULL;
    next = NULL;
    correct_for_at_center_intern = correct;
}
AW_toggle_struct::AW_toggle_struct(const char *variable_valuei,
        Widget toggle_widgeti) {

    variable_value = strdup(variable_valuei);
    toggle_widget = toggle_widgeti;
    next = NULL;

}
AW_toggle_struct::AW_toggle_struct(int variable_valuei, Widget toggle_widgeti) {

    variable_int_value = variable_valuei;
    toggle_widget = toggle_widgeti;
    next = NULL;

}
AW_toggle_struct::AW_toggle_struct(float variable_valuei, Widget toggle_widgeti) {

    variable_float_value = variable_valuei;
    toggle_widget = toggle_widgeti;
    next = NULL;

}
char *AW_select_table_struct::copy_string(const char *str) {
    char *out = strdup(str);
    char *p   = out;
    int   ch;
    
    while ((ch=*(p++)) != 0) {
        if (ch==',')
            p[-1] = ';';
        if (ch=='\n')
            p[-1] = '#';
    }
    return out;
}

AW_select_table_struct::AW_select_table_struct(const char *displayedi, const char *valuei) {
    memset((char *)this, 0, sizeof(AW_select_table_struct));
    displayed = copy_string(displayedi);
    char_value = strdup(valuei);
}
AW_select_table_struct::AW_select_table_struct(const char *displayedi, long valuei) {
    memset((char *)this, 0, sizeof(AW_select_table_struct));
    displayed = copy_string(displayedi);
    int_value = valuei;;
}
AW_select_table_struct::AW_select_table_struct(const char *displayedi, float valuei) {
    memset((char *)this, 0, sizeof(AW_select_table_struct));
    displayed = copy_string(displayedi);
    float_value = valuei;
}
AW_select_table_struct::AW_select_table_struct(const char *displayedi, void *pointer) {
    memset((char *)this, 0, sizeof(AW_select_table_struct));
    displayed = copy_string(displayedi);
    pointer_value = pointer;
}
AW_select_table_struct::~AW_select_table_struct(void) {
    free(displayed);
    free(char_value);
}

AW_selection_list::AW_selection_list(const char *variable_namei, int variable_typei, Widget select_list_widgeti) {
    memset((char *)this, 0, sizeof(AW_selection_list));
    variable_name = nulldup(variable_namei);
    variable_type = (AW_VARIABLE_TYPE)variable_typei;
    select_list_widget = select_list_widgeti;
    list_table = NULL;
    last_of_list_table = NULL;
    default_select = NULL;
    value_equal_display = false;
}

AW_root::AW_root(void) {
    memset((char *)this, 0, sizeof(AW_root));
    this->THIS = this;
    this->prvt = (AW_root_Motif *)GB_calloc(sizeof(AW_root_Motif), 1);
}

AW_root::~AW_root(void) {
    delete prvt;
}

AW_window_Motif::AW_window_Motif() {
    memset((char*)this, 0, sizeof(AW_window_Motif));
}

AW_window::AW_window(void) {
    memset((char *)this, 0, sizeof(AW_window));
    p_w = new AW_window_Motif;
    _at = new AW_at; // Note to valgrinders : the whole AW_window memory management suffers because Windows are NEVER deleted
    picture = new AW_rectangle;
    reset_scrolled_picture_size();
    slider_pos_vertical = 0;
    slider_pos_horizontal = 0;

}

AW_window::~AW_window(void) {
    delete p_w;
    delete picture;
}

#if defined(DEBUG)
// #define DUMP_MENU_LIST          // this should NOT be defined normally (if defined, every window writes all menu-entries to stdout)
#endif // DEBUG
#if defined(DUMP_MENU_LIST)

static char *window_name = 0;
static char *sub_menu = 0;

static void initMenuListing(const char *win_name) {
    aw_assert(win_name);

    freedup(window_name, win_name);
    freeset(sub_menu, 0);

    printf("---------------------------------------- list of menus for '%s'\n", window_name);
}

static void dumpMenuEntry(const char *entry) {
    aw_assert(window_name);
    if (sub_menu) {
        printf("'%s/%s/%s'\n", window_name, sub_menu, entry);
    }
    else {
        printf("'%s/%s'\n", window_name, entry);
    }
}

static void dumpOpenSubMenu(const char *sub_name) {
    aw_assert(sub_name);

    dumpMenuEntry(sub_name); // dump the menu itself

    if (sub_menu) freeset(sub_menu, GBS_global_string_copy("%s/%s", sub_menu, sub_name));
    else sub_menu = strdup(sub_name);
}

static void dumpCloseSubMenu() {
    aw_assert(sub_menu);
    char *lslash = strrchr(sub_menu, '/');
    if (lslash) {
        lslash[0] = 0;
    }
    else freeset(sub_menu, 0);
}

static void dumpCloseAllSubMenus() {
    freeset(sub_menu, 0);
}

#endif // DUMP_MENU_LIST
AW_window_menu_modes::AW_window_menu_modes(void) {
}
AW_window_menu_modes::~AW_window_menu_modes(void) {
}

AW_window_menu::AW_window_menu(void) {
}
AW_window_menu::~AW_window_menu(void) {
}

AW_window_simple::AW_window_simple(void) {
}
AW_window_simple::~AW_window_simple(void) {
}

AW_window_simple_menu::AW_window_simple_menu(void) {
}
AW_window_simple_menu::~AW_window_simple_menu(void) {
}

AW_window_message::AW_window_message(void) {
}
AW_window_message::~AW_window_message(void) {
}

/***********************************************************************/
void AW_window::set_horizontal_scrollbar_left_indent(int indent) {
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNleftOffset, (int)indent, NULL);
    left_indent_of_horizontal_scrollbar = indent;
}

/***********************************************************************/
static void value_changed_scroll_bar_horizontal(Widget wgt,
        XtPointer aw_cb_struct, XtPointer call_data) {
    AWUSE(wgt);
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    (cbs->aw)->slider_pos_horizontal = sbcbs->value; //setzt Scrollwerte im AW_window
    cbs->run_callback();
}
static void drag_scroll_bar_horizontal(Widget wgt, XtPointer aw_cb_struct,
        XtPointer call_data) {
    AWUSE(wgt);
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    (cbs->aw)->slider_pos_horizontal = sbcbs->value; //setzt Scrollwerte im AW_window
    cbs->run_callback();
}
void AW_window::set_horizontal_change_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAddCallback(p_w->scroll_bar_horizontal, XmNvalueChangedCallback,
            (XtCallbackProc) value_changed_scroll_bar_horizontal,
            (XtPointer) new AW_cb_struct(this, f, cd1, cd2, "") );
    XtAddCallback(p_w->scroll_bar_horizontal, XmNdragCallback,
            (XtCallbackProc) drag_scroll_bar_horizontal,
            (XtPointer) new AW_cb_struct(this, f, cd1, cd2, "") );
}

static void value_changed_scroll_bar_vertical(Widget wgt,
        XtPointer aw_cb_struct, XtPointer call_data) {
    AWUSE(wgt);
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    cbs->aw->slider_pos_vertical = sbcbs->value; //setzt Scrollwerte im AW_window
    cbs->run_callback();
}
static void drag_scroll_bar_vertical(Widget wgt, XtPointer aw_cb_struct,
        XtPointer call_data) {
    AWUSE(wgt);
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    cbs->aw->slider_pos_vertical = sbcbs->value; //setzt Scrollwerte im AW_window
    cbs->run_callback();
}

void AW_window::set_vertical_change_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAddCallback(p_w->scroll_bar_vertical, XmNvalueChangedCallback,
            (XtCallbackProc) value_changed_scroll_bar_vertical,
            (XtPointer) new AW_cb_struct(this, f, cd1, cd2, "") );
    XtAddCallback(p_w->scroll_bar_vertical, XmNdragCallback,
            (XtCallbackProc) drag_scroll_bar_vertical,
            (XtPointer) new AW_cb_struct(this, f, cd1, cd2, "") );

    XtAddCallback(p_w->scroll_bar_vertical, XmNpageIncrementCallback,
            (XtCallbackProc) drag_scroll_bar_vertical,
            (XtPointer) new AW_cb_struct(this, f, cd1, cd2, "") );
    XtAddCallback(p_w->scroll_bar_vertical, XmNpageDecrementCallback,
            (XtCallbackProc) drag_scroll_bar_vertical,
            (XtPointer) new AW_cb_struct(this, f, cd1, cd2, "") );
}

void AW_window::tell_scrolled_picture_size(AW_rectangle rectangle) {
    picture->l = rectangle.l;
    picture->r = rectangle.r;
    picture->t = rectangle.t;
    picture->b = rectangle.b;
}
void AW_window::tell_scrolled_picture_size(AW_world rectangle) {
    picture->l = (int)rectangle.l;
    picture->r = (int)rectangle.r;
    picture->t = (int)rectangle.t;
    picture->b = (int)rectangle.b;
}

void AW_window::reset_scrolled_picture_size() {
    picture->l = 0;
    picture->r = 0;
    picture->t = 0;
    picture->b = 0;
}
AW_pos AW_window::get_scrolled_picture_width() {
    return (picture->r - picture->l);
}

AW_pos AW_window::get_scrolled_picture_height() {
    return (picture->b - picture->t);
}

void AW_window::calculate_scrollbars(void) {
    AW_rectangle screen;
    
    int  slider_size_horizontal;
    int  slider_size_vertical;
    bool vertical, horizontal;
    int  position_of_slider;
    int  slider_max;

    vertical = horizontal = true; // es gibt verticalen & horizontalen scrollbar

    this->_get_area_size(AW_MIDDLE_AREA, &screen);

    // HORIZONTAL
    slider_max = (int)get_scrolled_picture_width();
    if (slider_max <1) {
        slider_max = 1;
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize, 1, NULL);
    }

    slider_size_horizontal = (int)( (screen.r
            -left_indent_of_horizontal_scrollbar));
    if (slider_size_horizontal < 1)
        slider_size_horizontal = 1; // ist der slider zu klein (<1) ?
    if (slider_size_horizontal > slider_max) { // Schirm groesser als Bild
        slider_size_horizontal = slider_max; // slider nimmt ganze laenge ein
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNvalue, 0, NULL); // slider ganz links setzen
        horizontal = false; // kein horizontaler slider mehr
    }

    // check wether XmNValue is to big
    XtVaGetValues(p_w->scroll_bar_horizontal, XmNvalue, &position_of_slider,
            NULL);
    if (position_of_slider > (slider_max-slider_size_horizontal)) {//steht der slider fuer slidergroesse zu rechts ?
        position_of_slider = slider_max-slider_size_horizontal; //-1 ? vielleicht !
        if (position_of_slider < 0)
            position_of_slider = 0;
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNvalue, position_of_slider,
                NULL);
    }
    // Anpassung fuer resize, wenn unbeschriebener Bereich vergroessert wird
    if ( -slider_pos_horizontal + get_scrolled_picture_width() < screen.r
            -left_indent_of_horizontal_scrollbar) {
        if (horizontal == true)
            slider_pos_horizontal = (int)(get_scrolled_picture_width()
                    - (screen.r-left_indent_of_horizontal_scrollbar) );
        else
            slider_pos_horizontal = 0; //slider nach ganz oben, da alles sichtbar
    }
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize, 1, NULL);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNmaximum, slider_max, NULL);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize,
            slider_size_horizontal, NULL);
    char buffer[200];
    sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNpageIncrement, (int)((screen.r
            -left_indent_of_horizontal_scrollbar)*(get_root()->awar( buffer )->read_int()*0.01)), 
    NULL);

    sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNincrement, (int)(get_root()->awar( buffer )->read_int()), NULL);

    sprintf(buffer, "window/%s/scroll_delay_horizontal", window_defaults_name);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNrepeatDelay, (int)(get_root()->awar( buffer )->read_int()), NULL);

    // VERTICAL
    slider_max = (int)get_scrolled_picture_height();
    if (slider_max <1) {
        slider_max = 1;
        XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, 1, NULL);
    }

    slider_size_vertical = (int)( (screen.b-top_indent_of_vertical_scrollbar
            -bottom_indent_of_vertical_scrollbar));
    if (slider_size_vertical < 1)
        slider_size_vertical = 1;
    if (slider_size_vertical > slider_max) {
        slider_size_vertical = slider_max;
        XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, 0, NULL);
        vertical = false;
    }

    // check wether XmNValue is to big
    XtVaGetValues(p_w->scroll_bar_vertical, XmNvalue, &position_of_slider, NULL);
    if (position_of_slider > (slider_max-slider_size_vertical)) {
        position_of_slider = slider_max-slider_size_vertical; //-1 ? vielleicht !
        if (position_of_slider < 0)
            position_of_slider = 0;
        XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, position_of_slider,
                NULL);
    }
    // Anpassung fuer resize, wenn unbeschriebener Bereich vergroessert wird
    if ( -slider_pos_vertical + get_scrolled_picture_height() < screen.b
            -top_indent_of_vertical_scrollbar
            -bottom_indent_of_vertical_scrollbar) {
        if (vertical == true)
            slider_pos_vertical = (int)(get_scrolled_picture_height()
                    - (screen.b-top_indent_of_vertical_scrollbar
                            -bottom_indent_of_vertical_scrollbar));
        else
            slider_pos_vertical = 0; //slider nach ganz oben, da alles sichtbar
    }
    XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, 1, NULL);
    XtVaSetValues(p_w->scroll_bar_vertical, XmNmaximum, slider_max, NULL);
    XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize,
            slider_size_vertical, NULL);
    sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);
    XtVaSetValues(p_w->scroll_bar_vertical, XmNpageIncrement, (int)((screen.b
            -top_indent_of_vertical_scrollbar
            -bottom_indent_of_vertical_scrollbar)*(get_root()->awar( buffer )->read_int()*0.01)), 
    NULL);

    sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
    XtVaSetValues(p_w->scroll_bar_vertical, XmNincrement, (int)(get_root()->awar( buffer )->read_int()), NULL);

    sprintf(buffer, "window/%s/scroll_delay_vertical", window_defaults_name);
    XtVaSetValues(p_w->scroll_bar_vertical, XmNrepeatDelay, (int)(get_root()->awar( buffer )->read_int()), NULL);
}

void AW_window::set_vertical_scrollbar_position(int position) {
    slider_pos_vertical = position;
    XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, position, NULL);
}

void AW_window::set_horizontal_scrollbar_position(int position) {
    slider_pos_horizontal = position;
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNvalue, position, NULL);
}

/***********************************************************************/
static void AW_timer_callback(XtPointer aw_timer_cb_struct, XtIntervalId *id) {
    AWUSE(id);
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *) aw_timer_cb_struct;
    if (!tcbs)
        return;

    AW_root *root = tcbs->ar;
    if (root->disable_callbacks) {
        // delay the timer callback for 25ms
        XtAppAddTimeOut(p_global->context,
        (unsigned long)25, // wait 25 msec = 1/40 sec
        (XtTimerCallbackProc)AW_timer_callback,
        aw_timer_cb_struct);
    } else {
        tcbs->f(root, tcbs->cd1, tcbs->cd2);
        delete tcbs; // timer only once
    }
}

static void AW_timer_callback_never_disabled(XtPointer aw_timer_cb_struct,
        XtIntervalId *id) {
    AWUSE(id);
    AW_timer_cb_struct *tcbs = (AW_timer_cb_struct *) aw_timer_cb_struct;
    if (!tcbs)
        return;

    tcbs->f(tcbs->ar, tcbs->cd1, tcbs->cd2);
    delete tcbs; // timer only once
}

void AW_root::add_timed_callback(int ms, void (*f)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAppAddTimeOut(p_r->context,
    (unsigned long)ms,
    (XtTimerCallbackProc)AW_timer_callback,
    (XtPointer) new AW_timer_cb_struct( this, f, cd1, cd2));
}

void AW_root::add_timed_callback_never_disabled(int ms, void (*f)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAppAddTimeOut(p_r->context,
    (unsigned long)ms,
    (XtTimerCallbackProc)AW_timer_callback_never_disabled,
    (XtPointer) new AW_timer_cb_struct( this, f, cd1, cd2));
}

/***********************************************************************/
void AW_POPDOWN(AW_window *aww) {
    aww->hide();
}

#define BUFSIZE 256
static char aw_size_awar_name_buffer[BUFSIZE];
static const char *aw_size_awar_name(AW_window *aww, const char *sub_entry) {
#if defined(DEBUG)
    int size =
#endif // DEBUG
            sprintf(aw_size_awar_name_buffer, "window/windows/%s/%s",
                    aww->window_defaults_name, sub_entry);
#if defined(DEBUG)
    aw_assert(size < BUFSIZE);
#endif // DEBUG
    return aw_size_awar_name_buffer;
}
#undef BUFSIZE

#define aw_awar_name_posx(aww)   aw_size_awar_name((aww), "posx")
#define aw_awar_name_posy(aww)   aw_size_awar_name((aww), "posy")
#define aw_awar_name_width(aww)  aw_size_awar_name((aww), "width")
#define aw_awar_name_height(aww) aw_size_awar_name((aww), "height")

static void aw_calculate_WM_offsets(AW_window *aww) {
    if (p_aww(aww)->WM_top_offset == AW_FIX_POS_ON_EXPOSE) {        // very bad hack continued
        // get last position stored in properties
        AW_root *root  = aww->get_root();
        int      oposx = root->awar(aw_awar_name_posx(aww))->read_int();
        int      oposy = root->awar(aw_awar_name_posy(aww))->read_int();

        // get current window position
        short            posy, posx;
        AW_window_Motif *motif = p_aww(aww);
        XtVaGetValues(motif->shell , XmNx, &posx, XmNy, &posy, NULL);

        // calculate offset
        motif->WM_top_offset  = posy-oposy;
        motif->WM_left_offset = posx-oposx;

#if defined(DEBUG) && 0
        printf("aw_calculate_WM_offsets: WM_left_offset=%i WM_top_offset=%i\n", motif->WM_left_offset, motif->WM_top_offset);
#endif // DEBUG
    }

#if defined(DEBUG) && 0
    {
        short posx, posy;
        AW_window_Motif *motif = p_aww(aww);
        XtVaGetValues(motif->shell , XmNx, &posx, XmNy, &posy, NULL);
        printf("[expose] Position reported by motif: %i/%i\n", posx, posy);
    }
#endif // DEBUG
}

/************** standard callback server *********************/

static void macro_message_cb(AW_window *aw, AW_CL);

bool AW_cb_struct::is_equal(const AW_cb_struct& other) const {
    bool equal = false;
    if (f == other.f) {                             // same callback function
        equal = (cd1 == other.cd1) && (cd2 == other.cd2);
        if (equal) {
            if (f == AW_POPUP) {
                equal = aw->get_root() == other.aw->get_root();
            }
            else {
                equal = aw == other.aw;
                if (!equal) {
                    equal = aw->get_root() == other.aw->get_root();
#if defined(DEBUG) && 0
                    if (equal) {
                        fprintf(stderr,
                                "callback '%s' instantiated twice with different windows (w1='%s' w2='%s') -- assuming the callbacks are equal\n",
                                id, aw->get_window_id(), other.aw->get_window_id());
                    }
#endif // DEBUG
                }
            }
        }
    }
    return equal;
}

#if defined(DEBUG)
// #define TRACE_CALLBACKS
#endif // DEBUG


AW_cb_struct_guard AW_cb_struct::guard_before = NULL;
AW_cb_struct_guard AW_cb_struct::guard_after  = NULL;

void AW_cb_struct::run_callback(void) {
    if (next) next->run_callback();                 // callback the whole list
    if (!f) return;                                 // run no callback

    AW_root *root = aw->get_root();
    if (root->disable_callbacks) {
        // some functions (namely aw_message, aw_input, aw_string_selection and aw_file_selection)
        // have to disable most callbacks, because they are often called from inside these callbacks
        // (e.g. because some exceptional condition occurred which needs user interaction) and if
        // callbacks weren't disabled, a recursive deadlock occurs.

        // the following callbacks are allowed even if disable_callbacks is true

        bool isModalCallback = (f == AW_CB(message_cb) ||
                                f == AW_CB(macro_message_cb) ||
                                f == AW_CB(input_history_cb) ||
                                f == AW_CB(input_cb) ||
                                f == AW_CB(file_selection_cb));

        bool isPopdown = (f == AW_CB(AW_POPDOWN));
        bool isHelp    = (f == AW_CB(AW_POPUP_HELP));
        bool allow     = isModalCallback || isHelp || isPopdown;

        bool isInfoResizeExpose = false;

        if (!allow) {
            isInfoResizeExpose = aw->is_expose_callback(AW_INFO_AREA, f) || aw->is_resize_callback(AW_INFO_AREA, f);
            allow              = isInfoResizeExpose;
        }

        if (!allow) {
            // don't warn about the following callbacks, just silently ignore them
            bool silentlyIgnore = 
                aw->is_expose_callback(AW_MIDDLE_AREA, f) ||
                aw->is_resize_callback(AW_MIDDLE_AREA, f);

            if (!silentlyIgnore) { // otherwise remind the user to answer the prompt:
                aw_message("That has been ignored. Answer the prompt first!");
            }
#if defined(TRACE_CALLBACKS)
            printf("suppressing callback %p\n", f);
#endif // TRACE_CALLBACKS
            return; // suppress the callback!
        }
#if defined(TRACE_CALLBACKS)
        else {
            if (isModalCallback) printf("allowed modal callback %p\n", f);
            else if (isPopdown) printf("allowed AW_POPDOWN\n");
            else if (isHelp) printf("allowed AW_POPUP_HELP\n");
            else if (isInfoResizeExpose) printf("allowed expose/resize infoarea\n");
            else printf("allowed other (unknown) callback %p\n", f);
        }
#endif // TRACE_CALLBACKS
    }
    else {
#if defined(TRACE_CALLBACKS)
        printf("Callbacks are allowed (executing %p)\n", f);
#endif // TRACE_CALLBACKS
    }

    if (guard_before) guard_before();

    if (f == AW_POPUP) {
        if (pop_up_window) { // already exists
            pop_up_window->activate();
        }
        else {
            AW_PPP g = (AW_PPP)cd1;
            if (g) {
                pop_up_window = g(aw->get_root(), cd2, 0);
                pop_up_window->show();
            } else {
                aw_message("not implemented -- please report to devel@arb-home.de");
            }
        }
        if (pop_up_window && p_aww(pop_up_window)->popup_cb)
            p_aww(pop_up_window)->popup_cb->run_callback();
    } else {
        f(aw, cd1, cd2);
    }
    
    if (guard_after) guard_after();
}

bool AW_cb_struct::contains(void (*g)(AW_window*,AW_CL ,AW_CL)) {
    return (f == g) || (next && next->contains(g));
}

void AW_root_Motif::set_cursor(Display *d, Window w, Cursor c) {
    XSetWindowAttributes attrs;
    old_cursor_display = d;
    old_cursor_window = w;

    if (c)
        attrs.cursor = c;
    else
        attrs.cursor = None;

    if (d && w) {
        XChangeWindowAttributes(d, w, CWCursor, &attrs);
    }
    XChangeWindowAttributes(XtDisplay(main_widget), XtWindow(main_widget),
            CWCursor, &attrs);
    XFlush(XtDisplay(main_widget));
}

void AW_root_Motif::normal_cursor(void) {
    set_cursor(old_cursor_display, old_cursor_window, 0);
}

void AW_server_callback(Widget wgt, XtPointer aw_cb_struct, XtPointer call_data) {
    AWUSE(wgt);
    AWUSE(call_data);
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;

    AW_root *root = cbs->aw->get_root();
    if (p_global->help_active) {
        p_global->help_active = 0;
        p_global->normal_cursor();

        if (cbs->help_text && ((GBS_string_matches(cbs->help_text, "*.ps", GB_IGNORE_CASE)) ||
                               (GBS_string_matches(cbs->help_text, "*.hlp", GB_IGNORE_CASE)) ||
                               (GBS_string_matches(cbs->help_text, "*.help", GB_IGNORE_CASE))))
        {
            AW_POPUP_HELP(cbs->aw, (AW_CL)cbs->help_text);
        }
        else {
            aw_message("Sorry no help available");
        }
        return;
    }

    if (root->prvt->recording_macro_file) {
        if (cbs->id && strcmp(cbs->id, root->prvt->stop_action_name)) {
            fprintf(root->prvt->recording_macro_file,
                    "BIO::remote_action($gb_main,\"%s\",",
                    root->prvt->application_name_for_macros);
            GBS_fwrite_string(cbs->id, root->prvt->recording_macro_file);
            fprintf(root->prvt->recording_macro_file, ");\n");
        }
    }

    if (cbs->f == AW_POPUP) {
        cbs->run_callback();
    } else {
        p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
                XtWindow(p_aww(cbs->aw)->shell),
                p_global->clock_cursor);
        cbs->run_callback();

        XEvent event; // destroy all old events !!!
        while (XCheckMaskEvent(XtDisplay(p_global->toplevel_widget), 
        ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
        KeyPressMask|KeyReleaseMask|PointerMotionMask, &event)) {
        }

        if (p_global->help_active) {
            p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
                    XtWindow(p_aww(cbs->aw)->shell),
                    p_global->question_cursor);
        } else {
            p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
                    XtWindow(p_aww(cbs->aw)->shell),
                    0);
        }
    }

}

void AW_clock_cursor(AW_root *root) {
    p_global->set_cursor(0, 0, p_global->clock_cursor);
}

void AW_normal_cursor(AW_root *root) {
    p_global->set_cursor(0, 0, 0);
}

/***********************************************************************/
static void AW_root_focusCB(Widget wgt, XtPointer awrp, XEvent*, Boolean*) {
    AWUSE(wgt);
    AW_root *aw_root = (AW_root *)awrp;
    if (aw_root->focus_callback_list) {
        aw_root->focus_callback_list->run_callback(aw_root);
    }
}

void AW_root::set_focus_callback(void (*f)(class AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    focus_callback_list = new AW_var_callback(f,cd1,cd2,focus_callback_list);
}

static void AW_focusCB(Widget wgt, XtPointer aw_cb_struct, XEvent*, Boolean*) {
    AWUSE(wgt);
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    cbs->run_callback();
}

void AW_window::set_popup_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    p_w->popup_cb = new AW_cb_struct(this, f, cd1, cd2, 0, p_w->popup_cb);
}

void AW_window::set_focus_callback(void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAddEventHandler(MIDDLE_WIDGET, EnterWindowMask, FALSE,
    AW_focusCB, (XtPointer) new AW_cb_struct(this, f, cd1, cd2, 0));
}

/*******************************    expose  ****************************************/

static void AW_exposeCB(Widget wgt, XtPointer aw_cb_struct, XmDrawingAreaCallbackStruct *call_data) {
    XEvent *ev = call_data->event;
    AWUSE(wgt);
    AW_area_management *aram = (AW_area_management *) aw_cb_struct;
    if (ev->xexpose.count == 0) { // last expose cb
        if (aram->expose_cb)
            aram->expose_cb->run_callback();
    }
}

void AW_area_management::set_expose_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    // insert expose callback for draw_area
    if (!expose_cb) {
        XtAddCallback(area, XmNexposeCallback, (XtCallbackProc) AW_exposeCB,
                (XtPointer) this );
    }
    expose_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, expose_cb);
}

void AW_window::set_expose_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram= MAP_ARAM(area);
    if (aram) aram->set_expose_callback(this, f, cd1, cd2);
}

bool AW_area_management::is_expose_callback(AW_window */*aww*/, void (*f)(AW_window*,AW_CL,AW_CL)) {
    return expose_cb && expose_cb->contains(f);
}

bool AW_window::is_expose_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL)) {
    AW_area_management *aram = MAP_ARAM(area);
    return aram && aram->is_expose_callback(this, f);
}

void AW_window::force_expose() {
    XmDrawingAreaCallbackStruct da_struct;

    da_struct.reason = XmCR_EXPOSE;
    da_struct.event = (XEvent *) NULL;
    da_struct.window = XtWindow(p_w->shell);

    XtCallCallbacks(p_w->shell, XmNexposeCallback, (XtPointer) &da_struct);
}

bool AW_area_management::is_resize_callback(AW_window */*aww*/, void (*f)(AW_window*,AW_CL,AW_CL)) {
    return resize_cb && resize_cb->contains(f);
}

bool AW_window::is_resize_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL)) {
    AW_area_management *aram = MAP_ARAM(area);
    return aram && aram->is_resize_callback(this, f);
}

void AW_window::set_window_size(int width, int height) {
    XtVaSetValues(p_w->shell, XmNwidth, (int)width, XmNheight, (int)height, NULL);
}
void AW_window::get_window_size(int &width, int &height) {
    unsigned short hoffset = 0;
    if (p_w->menu_bar[0]) XtVaGetValues(p_w->menu_bar[0], XmNheight, &hoffset, NULL);
    width = _at->max_x_size;
    height = hoffset + _at->max_y_size;
}

void AW_window::set_window_pos(int x, int y) {
    XtVaSetValues(p_w->shell, XmNx, (int)x, XmNy, (int)y, NULL);
}
void AW_window::get_window_pos(int& xpos, int& ypos) {
    unsigned short x, y;
    XtVaGetValues(p_w->shell, XmNx, &x, XmNy, &y, NULL);
    xpos = x;
    ypos = y;
}

void AW_window::window_fit(void) {
    int width, height;
    get_window_size(width, height);
    set_window_size(width, height);
}

void AW_window::get_screen_size(int &width, int &height) {
    Screen *screen = XtScreen(p_w->shell);

    width  = WidthOfScreen(screen);
    height = HeightOfScreen(screen);
}

bool AW_window::get_mouse_pos(int& x, int& y) {
    Display      *d  = XtDisplay(p_w->shell);
    Window        w1 = XtWindow(p_w->shell);
    Window        w2;
    Window        w3;
    int           rx, ry;
    int           wx, wy;
    unsigned int  mask;

    Bool ok = XQueryPointer(d, w1, &w2, &w3, &rx, &ry, &wx, &wy, &mask);

    if (ok) {
#if defined(DEBUG) && 0
        printf("get_mouse_pos: rx/ry=%i/%i wx/wy=%i/%i\n", rx, ry, wx, wy);
#endif // DEBUG
        x = rx;
        y = ry;
    }
    return ok;
}

/*******************************    resize  ****************************************/

// Predicate function: checks, if the given event is a ResizeEvent
int is_resize_event(Display *display, XEvent *event, XPointer) {
    if (event && (event->type == ResizeRequest || event->type
            == ConfigureNotify) && event->xany.display == display) {
        return 1;
    }
    return 0;
}

// Removes redundant resize events from the x-event queue
void cleanupResizeEvents(Display *display) {
    if (display) {
        XLockDisplay(display);
        XEvent event;
        if (XCheckIfEvent(display, &event, is_resize_event, 0)) {
            // Some magic happens here... ;-) (removing redundant events from queue)
            while (XCheckIfEvent(display, &event, is_resize_event, 0))
                ;
            // Keep last Event in queue
            XPutBackEvent(display, &event);
        }
        XUnlockDisplay(display);
    }
}

static void AW_resizeCB_draw_area(Widget wgt, XtPointer aw_cb_struct,
        XtPointer call_data) {
    AWUSE(wgt);
    AWUSE(call_data);
    AW_area_management *aram = (AW_area_management *) aw_cb_struct;
    cleanupResizeEvents(aram->common->display);
    if (aram->resize_cb)
        aram->resize_cb->run_callback();
}

void AW_area_management::set_resize_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    // insert resize callback for draw_area
    if (!resize_cb) {
        XtAddCallback(area, XmNresizeCallback,
                (XtCallbackProc) AW_resizeCB_draw_area, (XtPointer) this );
    }
    resize_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, resize_cb);
}

void AW_window::set_resize_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return;
    aram->set_resize_callback(this, f, cd1, cd2);
}

/***********************************************************************/
static void AW_inputCB_draw_area(Widget wgt, XtPointer aw_cb_struct, XmDrawingAreaCallbackStruct *call_data) {
    AWUSE(wgt);
    XEvent             *ev                        = call_data->event;
    AW_cb_struct       *cbs                       = (AW_cb_struct *) aw_cb_struct;
    AW_window          *aww                       = cbs->aw;
    bool                run_callback              = false;
    bool                run_double_click_callback = false;
    AW_area_management *area                      = 0;
    {
        int i;
        for (i=0; i<AW_MAX_AREA; i++) {
            if (p_aww(aww)->areas[i]->area == wgt) {
                area = p_aww(aww)->areas[i];
                break;
            }
        }
    }

    if (ev->xbutton.type == ButtonPress) {
        aww->event.type = AW_Mouse_Press;
        aww->event.button = ev->xbutton.button;
        aww->event.x = ev->xbutton.x;
        aww->event.y = ev->xbutton.y;
        aww->event.keycode = AW_KEY_NONE;
        aww->event.keymodifier = AW_KEYMODE_NONE;
        aww->event.character = '\0';

        if (area && area->double_click_cb) {
            if ( (ev->xbutton.time - area->click_time ) < 200) {
                run_double_click_callback = true;
            } else {
                run_callback = true;
            }
            area->click_time = ev->xbutton.time;
        } else {
            run_callback = true;
        }

        aww->event.time = ev->xbutton.time;
    } else if (ev->xbutton.type == ButtonRelease) {
        aww->event.type = AW_Mouse_Release;
        aww->event.button = ev->xbutton.button;
        aww->event.x = ev->xbutton.x;
        aww->event.y = ev->xbutton.y;
        aww->event.keycode = AW_KEY_NONE;
        aww->event.keymodifier = AW_KEYMODE_NONE;
        aww->event.character = '\0';
        //  aww->event.time     use old time

        run_callback = true;
    } else if (ev->xkey.type == KeyPress || ev->xkey.type == KeyRelease) {
        aww->event.time = ev->xbutton.time;

        const awXKeymap *mykey = aw_xkey_2_awkey(&(ev->xkey));

        aww->event.keycode = mykey->awkey;
        aww->event.keymodifier = mykey->awmod;

        if (mykey->awstr) {
            aww->event.character = mykey->awstr[0];
        } else {
            aww->event.character = 0;
        }

        if (ev->xkey.type == KeyPress) {
            aww->event.type = AW_Keyboard_Press;
        } else {
            aww->event.type = AW_Keyboard_Release;
        }
        aww->event.button = 0;
        aww->event.x = ev->xbutton.x;
        aww->event.y = ev->xbutton.y;

        if (!mykey->awmod && mykey->awkey >= AW_KEY_F1 && mykey->awkey
                <= AW_KEY_F12 && p_aww(aww)->modes_f_callbacks && p_aww(aww)->modes_f_callbacks[mykey->awkey-AW_KEY_F1]
                && aww->event.type == AW_Keyboard_Press) {
            p_aww(aww)->modes_f_callbacks[mykey->awkey-AW_KEY_F1]->run_callback();
        } else {
            run_callback = true;
        }
    }

    //  this is done above :
    //     else if (ev->xkey.type == KeyRelease) { // added Jan 98 to fetch multiple keystrokes in EDIT4 (may cause side effects)
    //         aww->event.time = ev->xbutton.time;
    //         aww->event.type = AW_Keyboard_Release;
    //         run_callback = true;
    //     }

    if (run_double_click_callback) {
        if (cbs->help_text == (char*)1) {
            cbs->run_callback();
        } else {
            if (area)
                area->double_click_cb->run_callback();
        }
    }

    if (run_callback && (cbs->help_text == (char*)0)) {
        cbs->run_callback();
    }
}

void AW_area_management::set_input_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAddCallback(area, XmNinputCallback,
            (XtCallbackProc) AW_inputCB_draw_area,
            (XtPointer) new AW_cb_struct(aww, f, cd1, cd2, (char*)0) );
}

void AW_window::set_input_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return;
    aram->set_input_callback(this, f, cd1, cd2);
}

/***********************************************************************/
void AW_area_management::set_double_click_callback(AW_window *aww, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    double_click_cb = new AW_cb_struct(aww, f, cd1, cd2, (char*)0, double_click_cb);
}

void AW_window::set_double_click_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return;
    aram->set_double_click_callback(this, f, cd1, cd2);
}

void AW_window::get_event(AW_event *eventi) {
    *eventi = event;
}

/***********************************************************************/

static void AW_motionCB(Widget w, XtPointer aw_cb_struct, XEvent *ev, Boolean*) {
    AWUSE(w);
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;

    cbs->aw->event.type = AW_Mouse_Drag;
    //  cbs->aw->event.button   = cbs->aw->event.button;
    cbs->aw->event.x = ev->xmotion.x;
    cbs->aw->event.y = ev->xmotion.y;
    cbs->aw->event.keycode = AW_KEY_NONE;

    cbs->run_callback();
}
void AW_area_management::set_motion_callback(AW_window *aww, void (*f)(AW_window *,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    XtAddEventHandler(area, ButtonMotionMask, False,
                      AW_motionCB, (XtPointer) new AW_cb_struct(aww, f, cd1, cd2, "") );
}
void AW_window::set_motion_callback(AW_area area, void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return;
    aram->set_motion_callback(this, f, cd1, cd2);
}

struct fallbacks {
    const char *fb;
    const char *awar;
    const char *init;
};

static struct fallbacks aw_fb[] = {
// fallback awarname    init

        { "FontList", "window/font", "8x13bold" }, { "background",
                "window/background", "grey" }, { "foreground",
                "window/foreground", "Black", }
, {0, "window/color_1", "red",}
, {0, "window/color_2", "green",}
, {0, "window/color_3", "blue",}
, {0, 0, 0}
};

static const char *aw_awar_2_color[] = { "window/background",
        "window/foreground", "window/color_1", "window/color_2",
        "window/color_3", 0 };

void AW_root::init_variables(AW_default database) {

    application_database = database;

    hash_table_for_variables = GBS_create_hash(1000, GB_MIND_CASE);
    hash_for_windows = GBS_create_hash(100, GB_MIND_CASE);
    prvt->action_hash = GBS_create_hash(1000, GB_MIND_CASE);
    int i;
    for (i=0; i<1000; i++) {
        if (aw_fb[i].awar == 0)
            break;
        awar_string(aw_fb[i].awar, aw_fb[i].init, application_database);
    }
    // PJ temporary site for vectorfont stuff
    vectorfont_lines =NULL; // font data not yet loaded

    awar_float("vectorfont/userscale", 1.0, application_database); // ratio font point size to pixels
    awar_string("vectorfont/name", "lib/pictures/fontgfx.vfont", application_database); // name for default font in lib/pictures
    // from the filerequester
    awar_int("vectorfont/active", 1, application_database); // zoomtext-calls: call text or use vectorfont (1)

    // this MIGHT lead to inconsistencies, as the validated data is in /name ---> worst case: reset
    aw_create_selection_box_awars(this, "vectorfont",
                                  GB_path_in_ARBLIB("pictures", NULL), 
                                  ".vfont", vectorfont_name, application_database, true);
    awar("vectorfont/file_name")->add_callback( (AW_RCB0) aw_xfig_font_changefont_cb);
}

void *AW_root::get_aw_var_struct(char *awar_name) {
    long vs;
    vs = (long)GBS_read_hash(hash_table_for_variables, awar_name);
    if (!vs) {
        AW_ERROR("AW_root::get_aw_var_struct: Variable %s not defined", awar_name);
    }
    return (void *)vs;
}
void *AW_root::get_aw_var_struct_no_error(char *awar_name) {
    long vs;
    vs = (long)GBS_read_hash(hash_table_for_variables, awar_name);
    return (void*)vs;
}

static void aw_root_create_color_map(AW_root *root) {
    int i;
    XColor xcolor_returned, xcolor_exakt;
    GBDATA *gbd = (GBDATA*)root->application_database;
    p_global->color_table = (unsigned long *)GB_calloc(sizeof(unsigned long),AW_COLOR_MAX);

    if (p_global->screen_depth == 1) { //Black and White Monitor
        unsigned long white= WhitePixelOfScreen( XtScreen(p_global->toplevel_widget) );
        unsigned long black= BlackPixelOfScreen( XtScreen(p_global->toplevel_widget) );
        p_global->foreground = black;
        p_global->background = white;
        for (i=0; i< AW_COLOR_MAX; i++) {
            p_global->color_table[i] = black;
        }
        p_global->color_table[AW_WINDOW_FG] = white;
        p_global->color_table[AW_WINDOW_C1] = white;
        p_global->color_table[AW_WINDOW_C2] = white;
        p_global->color_table[AW_WINDOW_C3] = white;
    } else { // Color monitor
        const char **awar_2_color;
        int color;
        for (color = 0, awar_2_color = aw_awar_2_color;
             *awar_2_color;
             awar_2_color++, color++)
        {
            const char *name_of_color = GB_read_char_pntr(GB_search(gbd, *awar_2_color, GB_FIND));
            if (XAllocNamedColor(p_global->display,p_global->colormap,name_of_color, &xcolor_returned,&xcolor_exakt) == 0) {
                fprintf(stderr,"XAllocColor failed: %s\n",name_of_color);
            }
            else {
                p_global->color_table[color] = xcolor_returned.pixel;
            }
        }
        p_global->foreground= BlackPixelOfScreen( XtScreen(p_global->toplevel_widget) );
        XtVaGetValues(p_global->toplevel_widget,XmNbackground,
        &p_global->background, NULL);
    }
    // AW_WINDOW_DRAG see init_devices

}

static void aw_message_forwarder(const char *msg) {
    aw_message(msg);
}
static void aw_message_forwarder_verbose(const char *msg) {
    fprintf(stderr, "ARB: %s\n", msg); // print to console as well
    aw_message(msg);
}

static int aw_status_dummy(double val) {
    return aw_status(val);
}

static int aw_status_dummy2(const char *val) {
    return aw_status((char *)val);
}

void AW_root::init_root(const char *programname, bool no_exit) {
    // Initialisiert eine gesamte X-Anwendung
    int a = 0;
    int i;
    XFontStruct *fontstruct;
    char buffer[256];
    char *fallback_resources[100];

    p_r-> no_exit = no_exit;
    program_name  = strdup(programname);

    for (i=0; i<1000; i++) {
        if (aw_fb[i].fb == 0)
            break;
        sprintf(buffer, "*%s: %s", aw_fb[i].fb, GB_read_char_pntr(GB_search(
                (GBDATA*)application_database, aw_fb[i].awar, GB_FIND)));
        fallback_resources[i] = strdup(buffer);
    }
    fallback_resources[i] = 0;
    GB_install_error_handler((gb_warning_func_type)aw_message_forwarder_verbose);
    GB_install_warning((gb_warning_func_type)aw_message_forwarder);
    GB_install_information((gb_information_func_type)0); // 0 means -> write to stdout only

    GB_install_status((gb_status_func_type)aw_status_dummy);
    GB_install_status2((gb_status_func2_type)aw_status_dummy2);

    // @@@ FIXME: the next line hangs if program runs inside debugger
    p_r->toplevel_widget = XtOpenApplication(&(p_r->context), programname, 
            NULL, 0, // XrmOptionDescRec+numOpts
            &a, /*&argc*/
            NULL, /*argv*/
            fallback_resources, 
            applicationShellWidgetClass, // widget class
            NULL, 0);

    for (i=0; i<1000 && fallback_resources[i]; i++) {
        free(fallback_resources[i]);
    }

    p_r->display = XtDisplay(p_r->toplevel_widget);

    if (p_r->display == NULL) {
        printf("cannot open display\n");
        exit(-1);
    }
    {
        GBDATA *gbd = (GBDATA*)application_database;
        const char *font = GB_read_char_pntr(GB_search(gbd, "window/font", GB_FIND));
        if ( !(fontstruct = XLoadQueryFont( p_r->display, font))) {
            if ( !(fontstruct = XLoadQueryFont( p_r->display, "fixed"))) {
                printf("can not load font\n");
                exit( -1);
            }
        }
    }

    if (fontstruct->max_bounds.width == fontstruct->min_bounds.width) {
        font_width = fontstruct->max_bounds.width;
    } else {
        font_width = (fontstruct->min_bounds.width
                + fontstruct->max_bounds.width) / 2;
    }

    font_height = fontstruct->max_bounds.ascent
            + fontstruct->max_bounds.descent;
    font_ascent = fontstruct->max_bounds.ascent;

    p_r->fontlist = XmFontListCreate(fontstruct,XmSTRING_DEFAULT_CHARSET);

    p_r->button_list = 0;

    p_r->config_list = new AW_config_struct( "", AWM_ALL, NULL, "Programmer Name", "SH", NULL );
    p_r->last_config = p_r->config_list;

    p_r->last_option_menu = p_r->current_option_menu = p_r->option_menu_list = NULL;
    p_r->last_toggle_field = p_r->toggle_field_list = NULL;
    p_r->last_selection_list = p_r->selection_list = NULL;

    value_changed = false;
    y_correction_for_input_labels = 5;
    global_mask = AWM_ALL;

    p_r->screen_depth = PlanesOfScreen( XtScreen(p_r->toplevel_widget) );
    if (p_r->screen_depth == 1) {
        color_mode = AW_MONO_COLOR;
    } else {
        color_mode = AW_RGB_COLOR;
    }
    p_r->colormap = DefaultColormapOfScreen( XtScreen(p_r->toplevel_widget) );
    p_r->clock_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget),XC_watch);
    p_r->question_cursor = XCreateFontCursor(XtDisplay(p_r->toplevel_widget),XC_question_arrow);

    aw_root_create_color_map(this);
    aw_root_init_font(XtDisplay(p_r->toplevel_widget));
    aw_install_xkeys(XtDisplay(p_r->toplevel_widget));

}

/***********************************************************************/
void AW_window::_get_area_size(AW_area area, AW_rectangle *square) {
    AW_area_management *aram= MAP_ARAM(area);
    *square = aram->common->screen;
}

/***********************************************************************/

static void horizontal_scrollbar_redefinition_cb(class AW_root *aw_root,
        AW_CL cd1, AW_CL cd2) {
    AW_rectangle screen;
    AWUSE(aw_root);

    char buffer[200];
    AW_window *aw = (AW_window *)cd1;
    Widget w = (Widget)cd2;

    aw->_get_area_size(AW_MIDDLE_AREA, &screen);

    sprintf(buffer, "window/%s/horizontal_page_increment",
            aw->window_defaults_name);
    XtVaSetValues(w, XmNpageIncrement, (int)((screen.r
            -aw->left_indent_of_horizontal_scrollbar)*(aw->get_root()->awar( buffer )->read_int()*0.01)), NULL);

    sprintf(buffer, "window/%s/scroll_width_horizontal",
            aw->window_defaults_name);
    XtVaSetValues(w, XmNincrement, (int)(aw->get_root()->awar( buffer )->read_int()), NULL);

    sprintf(buffer, "window/%s/scroll_delay_horizontal",
            aw->window_defaults_name);
    XtVaSetValues(w, XmNrepeatDelay, (int)(aw->get_root()->awar( buffer )->read_int()), NULL);

}

static void vertical_scrollbar_redefinition_cb(class AW_root *aw_root,
        AW_CL cd1, AW_CL cd2) {
    AW_rectangle screen;
    AWUSE(aw_root);

    char buffer[200];
    AW_window *aw = (AW_window *)cd1;
    Widget w = (Widget)cd2;

    aw->_get_area_size(AW_MIDDLE_AREA, &screen);

    sprintf(buffer, "window/%s/vertical_page_increment",
            aw->window_defaults_name);
    XtVaSetValues(w, XmNpageIncrement, (int)((screen.b
            -aw->top_indent_of_vertical_scrollbar
            -aw->bottom_indent_of_vertical_scrollbar)*(aw->get_root()->awar( buffer )->read_int()*0.01)), NULL);

    sprintf(buffer, "window/%s/scroll_width_vertical", aw->window_defaults_name);
    XtVaSetValues(w, XmNincrement, (int)(aw->get_root()->awar( buffer )->read_int()), NULL);

    sprintf(buffer, "window/%s/scroll_delay_vertical", aw->window_defaults_name);
    XtVaSetValues(w, XmNrepeatDelay, (int)(aw->get_root()->awar( buffer )->read_int()), NULL);
}

void AW_window::create_window_variables(void) {

    char buffer[200];
    memset(buffer, 0, 200);
    sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name);
    get_root()->awar_int(buffer, 50, get_root()->application_database);
    get_root()->awar( buffer)->add_callback(
            (AW_RCB)horizontal_scrollbar_redefinition_cb, (AW_CL)this,
            (AW_CL)p_w->scroll_bar_horizontal );

    sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);
    get_root()->awar_int(buffer, 50, get_root()->application_database);
    get_root()->awar( buffer)->add_callback( (AW_RCB)vertical_scrollbar_redefinition_cb,
            (AW_CL)this, (AW_CL)p_w->scroll_bar_vertical );

    sprintf(buffer, "window/%s/scroll_delay_vertical", window_defaults_name);
    get_root()->awar_int(buffer, 20, get_root()->application_database);
    get_root()->awar( buffer)->add_callback( (AW_RCB)vertical_scrollbar_redefinition_cb,
            (AW_CL)this, (AW_CL)p_w->scroll_bar_vertical );

    sprintf(buffer, "window/%s/scroll_delay_horizontal", window_defaults_name);
    get_root()->awar_int(buffer, 20, get_root()->application_database);
    get_root()->awar( buffer)->add_callback(
            (AW_RCB)horizontal_scrollbar_redefinition_cb, (AW_CL)this,
            (AW_CL)p_w->scroll_bar_horizontal );

    sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
    get_root()->awar_int(buffer, 9, get_root()->application_database);
    get_root()->awar( buffer)->add_callback(
            (AW_RCB)horizontal_scrollbar_redefinition_cb, (AW_CL)this,
            (AW_CL)p_w->scroll_bar_horizontal );

    sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
    get_root()->awar_int(buffer, 20, get_root()->application_database);
    get_root()->awar( buffer)->add_callback( (AW_RCB)vertical_scrollbar_redefinition_cb,
            (AW_CL)this, (AW_CL)p_w->scroll_bar_vertical );

}

/***********************************************************************/
void AW_area_management::create_devices(AW_window *aww, AW_area ar) {
    AW_root *root =aww->get_root();
    common = new AW_common(aww,ar,XtDisplay(area),XtWindow(area),p_global->color_table,
            (unsigned int **)&aww->color_table, &aww->color_table_size );
}

const char *AW_window::GC_to_RGB(AW_device *device, int gc, int& red,
        int& green, int& blue) {
    AW_common *common = device->common;
    AW_GC_Xm *gcm= AW_MAP_GC(gc);
    aw_assert(gcm);
    unsigned pixel = (unsigned short)(gcm->color);
    GB_ERROR error = 0;
    XColor query_color;

    query_color.pixel = pixel;
    XQueryColor(p_global->display, p_global->colormap, &query_color);
    // @@@ FIXME: error handling!

    red = query_color.red;
    green = query_color.green;
    blue = query_color.blue;

    if (error) {
        red = green = blue = -1;
    }
    return error;
}

// Converts GC to RGB float values to the range (0 - 1.0)
const char *AW_window::GC_to_RGB_float(AW_device *device, int gc, float& red,
        float& green, float& blue) {
    AW_common *common = device->common;
    AW_GC_Xm *gcm= AW_MAP_GC(gc);
    aw_assert(gcm);
    unsigned pixel = (unsigned short)(gcm->color);
    GB_ERROR error = 0;
    XColor query_color;

    query_color.pixel = pixel;

    XQueryColor(p_global->display, p_global->colormap, &query_color);
    // @@@ FIXME: error handling!

    red = query_color.red/65535.0;
    green = query_color.green/65535.0;
    blue = query_color.blue/65535.0;

    if (error) {
        red = green = blue = 1.0f;
    }
    return error;
}

AW_color AW_window::alloc_named_data_color(int colnum, char *colorname) {
    if (!color_table_size) {
        color_table_size = AW_COLOR_MAX + colnum;
        color_table = (unsigned long *)malloc(sizeof(unsigned long)
                *color_table_size);
        memset((char *)color_table, -1, (size_t)(color_table_size
                *sizeof(unsigned long)));
    } else {
        if (colnum>=color_table_size) {
            color_table = (unsigned long *)realloc((char *)color_table, (8
                    + colnum)*sizeof(long)); // valgrinders : never freed because AW_window never is freed
            memset( (char *)(color_table+color_table_size), -1, (int)(8
                    + colnum - color_table_size) * sizeof(long));
            color_table_size = 8+colnum;
        }
    }
    XColor xcolor_returned, xcolor_exakt;

    if (p_global->screen_depth == 1) { //Black and White Monitor
        static int col = 1;
        if (colnum == AW_DATA_BG) {
            col =1;
            if (strcmp(colorname, "white"))
                col *=-1;
        }
        if (col==1) {
            color_table[colnum] = WhitePixelOfScreen( XtScreen(p_global->toplevel_widget) );
        } else {
            color_table[colnum] = BlackPixelOfScreen( XtScreen(p_global->toplevel_widget) );
        }
        if (colnum == AW_DATA_BG)
            col *=-1;
    } else { // Color monitor
        if (color_table[colnum] !=(unsigned long)-1 ) {
            XFreeColors(p_global->display, p_global->colormap, &color_table[colnum],1,0);
        }
        if (XAllocNamedColor(p_global->display,p_global->colormap,colorname,
        &xcolor_returned,&xcolor_exakt) == 0) {
            sprintf(AW_ERROR_BUFFER, "XAllocColor failed: %s\n", colorname);
            aw_message();
            color_table[colnum] = (unsigned long)-1;
        } else {
            color_table[colnum] = xcolor_returned.pixel;
        }
    }
    if (colnum == AW_DATA_BG) {
        XtVaSetValues(p_w->areas[AW_MIDDLE_AREA]->area, 
        XmNbackground, color_table[colnum], NULL);
    }
    return (AW_color)colnum;
}

void AW_window::create_devices(void) {
    unsigned long background_color;
    if (p_w->areas[AW_INFO_AREA]) {
        p_w->areas[AW_INFO_AREA]->create_devices(this, AW_INFO_AREA);
        XtVaGetValues(p_w->areas[AW_INFO_AREA]->area, 
        XmNbackground, &background_color, NULL);
        p_global->color_table[AW_WINDOW_DRAG] =
        background_color ^ p_global->color_table[AW_WINDOW_FG];
    }
    if (p_w->areas[AW_MIDDLE_AREA]) {
        p_w->areas[AW_MIDDLE_AREA]->create_devices(this, AW_MIDDLE_AREA);
    }
    if (p_w->areas[AW_BOTTOM_AREA]) {
        p_w->areas[AW_BOTTOM_AREA]->create_devices(this, AW_BOTTOM_AREA);
    }
}

void AW_help_entry_pressed(AW_window *aww) {
    AW_root *root = aww->get_root();
    p_global->help_active = 1;
}

void aw_create_help_entry(AW_window *aww) {
    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "P", 0,
                           AWM_ALL, (AW_CB)AW_help_entry_pressed, 0, 0);
}

/****************************************************************************************************************************/
/****************************************************************************************************************************/
/****************************************************************************************************************************/

const char *aw_str_2_label(const char *str, AW_window *aww) {
    aw_assert(str);
    
    static const char *last_label = 0;
    static const char *last_str   = 0;
    static AW_window  *last_aww   = 0;

    const char *label;
    if (str == last_str && aww == last_aww) { // reuse result ? 
        label = last_label;
    }
    else {
        if (str[0] == '#') {
            label = GB_path_in_ARBLIB("pixmaps", str+1);
        }
        else {
            AW_awar *is_awar = aww->get_root()->label_is_awar(str);

            if (is_awar) { // for labels displaying awar values, insert dummy text here
                int wanted_len = aww->_at->length_of_buttons - 2;
                if (wanted_len < 1) wanted_len = 1;

                char *labelbuf       = GB_give_buffer(wanted_len+1);
                memset(labelbuf, 'y', wanted_len);
                labelbuf[wanted_len] = 0;

                label = labelbuf;
            }
            else {
                label = str;
            }
        }

        // store results locally, cause aw_str_2_label is nearly always called twice with same arguments
        // (see RES_LABEL_CONVERT)
        last_label = label;
        last_str   = str;
        last_aww   = aww;
    }
    return label;
}

void AW_label_in_awar_list(AW_window *aww, Widget widget, const char *str) {
    AW_awar *is_awar = aww->get_root()->label_is_awar(str);
    if (is_awar) {
        char *var_value = is_awar->read_as_string();
        if (var_value) {
            aww->update_label((int*)widget, var_value);
        }
        else {
            AW_ERROR("AW_label_in_awar_list:: AWAR %s not found\n", str);
            aww->update_label((int*)widget, str);
        }
        free(var_value);
        AW_INSERT_BUTTON_IN_AWAR_LIST(is_awar, 0, widget, AW_WIDGET_LABEL_FIELD, aww);
    }
}
/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/

static void aw_window_avoid_destroy_cb(Widget, AW_window *, XmAnyCallbackStruct *) {
    aw_message("If YOU do not know what to answer, how should ARB know?\nPlease think again and answer the prompt!");
}

static void aw_window_noexit_destroy_cb(Widget , AW_window *aww, XmAnyCallbackStruct *) {
    aww->hide();
    // don't exit, when using destroy callback
}

static void aw_window_destroy_cb(Widget , AW_window *aww, XmAnyCallbackStruct *) {
    AW_root *root = aww->get_root();
    if ( (p_global->main_aww == aww) || !p_global->main_aww->is_shown()) {
#ifdef NDEBUG
        if (aw_question("Are you sure to quit?","YES,NO") ) return;
#endif
        exit(0);
    }
    aww->hide();
}

static long aw_loop_get_window_geometry(const char *, long val, void *) {
    AW_window *aww = (AW_window *)val;
    short posx, posy;

    AW_root *root = aww->get_root();
    unsigned short width, height, borderwidth;

    XtVaGetValues(p_aww(aww)->shell,                // bad hack
                  XmNborderWidth, &borderwidth,
                  XmNwidth, &width,
                  XmNheight, &height,
                  XmNx, &posx,
                  XmNy, &posy,
                  NULL);

    if ( p_aww(aww)->WM_top_offset != AW_FIX_POS_ON_EXPOSE) {
        posy -= p_aww(aww)->WM_top_offset;
    }
    posx -= p_aww(aww)->WM_left_offset;

    if (posx<0) posx = 0;
    if (posy<0) posy = 0;

    root->awar(aw_awar_name_width (aww))->write_int(width);
    root->awar(aw_awar_name_height(aww))->write_int(height);
    root->awar(aw_awar_name_posx (aww))->write_int(posx);
    root->awar(aw_awar_name_posy (aww))->write_int(posy);

    return val;
}

void aw_update_awar_window_geometry(AW_root *awr) {
    GBS_hash_do_loop(awr->hash_for_windows, aw_loop_get_window_geometry, NULL);
}

static const char *existingPixmap(const char *iconpath, const char *name) {
    const char *icon = GBS_global_string("%s/%s.xpm", iconpath, name);

    if (!GB_is_regularfile(icon)) {
        icon = GBS_global_string("%s/%s.bitmap", iconpath, name);
        if (!GB_is_regularfile(icon)) icon = NULL;
    }

    return icon;
}

static Pixmap getIcon(Screen *screen, const char *iconName, Pixel foreground, Pixel background) {
    static GB_HASH *icon_hash = 0;
    if (!icon_hash) icon_hash = GBS_create_hash(100, GB_MIND_CASE);

    Pixmap pixmap = GBS_read_hash(icon_hash, iconName);

    if (!pixmap && iconName) {
        const char *iconpath = GB_path_in_ARBLIB("pixmaps/icons", NULL);
        const char *iconFile = existingPixmap(iconpath, iconName);

        if (iconFile) {
            char *ico = strdup(iconFile);
            pixmap    = XmGetPixmap(screen, ico, foreground, background);
            GBS_write_hash(icon_hash, iconName, pixmap);
            free(ico);
        }
    }

    return pixmap;
}

void aw_set_delete_window_cb(AW_window *aww, Widget shell, bool allow_close) {
    Atom WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell), (char*)"WM_DELETE_WINDOW", False);

    // remove any previous callbacks
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_avoid_destroy_cb,  (caddr_t)aww);
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_noexit_destroy_cb, (caddr_t)aww);
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_destroy_cb,        (caddr_t)aww);

    if (allow_close == false) {
        XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_avoid_destroy_cb,(caddr_t)aww);
    }
    else {
        AW_root *root = aww->get_root();
        if (p_global->no_exit) {
            XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_noexit_destroy_cb,(caddr_t)aww);
        }
        else {
            XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_destroy_cb,(caddr_t)aww);
        }
    }
}

void AW_window::allow_delete_window(bool allow_close) {
    aw_set_delete_window_cb(this, p_w->shell, allow_close);
}

Widget aw_create_shell(AW_window *aww, bool allow_resize, bool allow_close, int width, int height, int posx, int posy) {
    AW_root *root = aww->get_root();
    Widget shell;

    // set minimum window size to size provided by init
    if (width >aww->_at->max_x_size) aww->_at->max_x_size = width;
    if (height>aww->_at->max_y_size) aww->_at->max_y_size = height;

    if ( !GBS_read_hash(root->hash_for_windows, aww->get_window_id())) {
        GBS_write_hash(root->hash_for_windows, aww->get_window_id(), (long)aww);
        bool has_user_geometry = false;

        const char *temp= aw_awar_name_width(aww);
        root->awar_int(temp, width);
        if (allow_resize) {
            int found_width = (int)root->awar(temp)->read_int();
            if (width != found_width) {
                has_user_geometry = true;
                width = found_width;
            }
        }

        temp = aw_awar_name_height(aww);
        root->awar_int(temp, height);
        if (allow_resize) {
            int found_height = (int)root->awar(temp)->read_int();
            if (height != found_height) {
                has_user_geometry = true;
                height = found_height;
            }
        }

        temp = aw_awar_name_posx(aww);
        root->awar_int(temp,posx)->set_minmax(0, 4000);
        // @@@ FIXME:  maximum should be set to current screen size minus some offset
        // to ensure that windows do not appear outside screen
        // same for posy below!

        int found_posx = (int)root->awar(temp)->read_int();
        if (posx != found_posx) {
            has_user_geometry = true;
            posx = found_posx;
        }

        temp = aw_awar_name_posy(aww);
        root->awar_int(temp,posy)->set_minmax(0, 3000);
        int found_posy = (int)root->awar(temp)->read_int();
        if (posy != found_posy) {
            has_user_geometry = true;
            posy = found_posy;
        }

        if (has_user_geometry) {
            aww->recalc_size_atShow(AW_RESIZE_USER); // keep user geometry (only if user size is smaller than default size, the latter is used)
        }
        else { // no geometry yet
            aww->recalc_pos_atShow(AW_REPOS_TO_MOUSE_ONCE); // popup the window at current mouse position
        }
    }

    if (allow_resize) {
        // create the window big enough to ensure that all widgets
        // are created in visible area (otherwise widget are crippled).
        // window will be resized later (on show)

        width = 4000;
        height = 3000;

        aww->recalc_size_atShow(AW_RESIZE_ANY);
    }

    Widget  father      = p_global->toplevel_widget;
    Screen *screen      = XtScreen(father);
    Pixmap  icon_pixmap = getIcon(screen, aww->window_defaults_name, p_global->foreground, p_global->background);

    if (!icon_pixmap) {
        icon_pixmap = getIcon(screen, root->program_name, p_global->foreground, p_global->background);
    }

    if (!icon_pixmap) {
        AW_ERROR("Error: Missing icon pixmap for window '%s'\n", aww->window_defaults_name);
    }
    else if (icon_pixmap == XmUNSPECIFIED_PIXMAP) {
        AW_ERROR("Error: Failed to load icon pixmap for window '%s'\n", aww->window_defaults_name);
    }

#if defined(DEBUG) && 0
    printf("aw_create_shell: pos=%i/%i size=%i%i\n", posx, posy, width, height);
#endif // DEBUG
    if (!p_global->main_widget || !p_global->main_aww->is_shown()) {
        shell = XtVaCreatePopupShell("editor", applicationShellWidgetClass,
                                     father,
                                     XmNwidth, width,
                                     XmNheight, height, 
                                     XmNx, posx, 
                                     XmNy, posy, 
                                     XmNtitle, aww->window_name, 
                                     XmNiconName, aww->window_name, 
                                     XmNkeyboardFocusPolicy, XmEXPLICIT,
                                     XmNdeleteResponse, XmDO_NOTHING,
                                     XtNiconPixmap, icon_pixmap, 
                                     NULL);
    } else {
        shell = XtVaCreatePopupShell("transient", transientShellWidgetClass,
                                     father, 
                                     XmNwidth, width, 
                                     XmNheight, height, 
                                     XmNx, posx, 
                                     XmNy, posy, 
                                     XmNtitle, aww->window_name, 
                                     XmNiconName, aww->window_name, 
                                     XmNkeyboardFocusPolicy, XmEXPLICIT, 
                                     XmNdeleteResponse, XmDO_NOTHING, 
                                     XtNiconPixmap, icon_pixmap, 
                                     NULL);
    }
    XtAddEventHandler(shell, EnterWindowMask, FALSE, AW_root_focusCB, (XtPointer) aww->get_root());

    if (!p_global->main_widget) {
        p_global->main_widget = shell;
        p_global->main_aww    = aww;
    }
    else {
        if ( !p_global->main_aww->is_shown()) { // now i am the root window
            p_global->main_widget = shell;
            p_global->main_aww    = aww;
        }
    }

    aw_set_delete_window_cb(aww, shell, allow_close);

    // set icon window (for window managers where iconified applications are dropped onto desktop or similar)
    {
        Window icon_window;
        XtVaGetValues(shell, XmNiconWindow, &icon_window, NULL);

        Display *dpy = XtDisplay(shell);
        if (!icon_window) {
            XSetWindowAttributes attr;
            attr.background_pixmap = icon_pixmap;

            int          xpos, ypos;
            unsigned int xsize, ysize, borderwidth, depth;
            Window       wroot;

            if (XGetGeometry(dpy, icon_pixmap, &wroot, &xpos, &ypos, &xsize, &ysize, &borderwidth, &depth)) {
                icon_window = XCreateWindow(dpy, wroot, 0, 0, xsize, ysize, 0, depth, CopyFromParent, CopyFromParent, CWBackPixmap, &attr);
            }
        }
        if (!icon_window) {
            XtVaSetValues(shell, XmNiconPixmap, icon_pixmap, NULL);
        }
        else {
            XtVaSetValues(shell, XmNiconWindow, icon_window, NULL);
            XSetWindowBackgroundPixmap(dpy, icon_window, icon_pixmap);
            XClearWindow(dpy, icon_window);
        }
    }

    return shell;
}


void aw_realize_widget(AW_window *aww) {
    if ( p_aww(aww)->areas[AW_INFO_AREA] && p_aww(aww)->areas[AW_INFO_AREA]->form) {
        XtManageChild(p_aww(aww)->areas[AW_INFO_AREA]->form);
    }
    if ( p_aww(aww)->areas[AW_MIDDLE_AREA] && p_aww(aww)->areas[AW_MIDDLE_AREA]->form) {
        XtManageChild(p_aww(aww)->areas[AW_MIDDLE_AREA]->form);
    }
    if ( p_aww(aww)->areas[AW_BOTTOM_AREA] && p_aww(aww)->areas[AW_BOTTOM_AREA]->form) {
        XtManageChild(p_aww(aww)->areas[AW_BOTTOM_AREA]->form);
    }
    XtRealizeWidget( p_aww(aww)->shell);
    p_aww(aww)->WM_top_offset = AW_FIX_POS_ON_EXPOSE;
}

void AW_window_menu_modes::init(AW_root *root_in, const char *wid,
        const char *windowname, int width, int height) {
    Widget main_window;
    Widget help_popup;
    Widget help_label;
    Widget separator;
    Widget form1;
    Widget form2;
    //Widget frame;
    const char *help_button = "HELP";
    const char *help_mnemonic = "H";

#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int posx = 50;
    int posy = 50;

    p_w->shell= aw_create_shell(this, true, true, width, height, posx, posy);

    main_window = XtVaCreateManagedWidget("mainWindow1",
            xmMainWindowWidgetClass, p_w->shell, 
            NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
            main_window, 
            XmNrowColumnType, XmMENU_BAR, 
            NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
            p_w->menu_bar[0], 
            XmNwidth, 1, 
            XmNheight, 1, 
            XmNallowShellResize, true, 
            XmNoverrideRedirect, true, 
            NULL);

    //create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
            xmRowColumnWidgetClass, help_popup, 
            XmNrowColumnType, XmMENU_PULLDOWN, 
            NULL);

    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
            xmCascadeButtonWidgetClass, p_w->menu_bar[0], 
            RES_CONVERT( XmNlabelString, help_button ),
                                          RES_CONVERT( XmNmnemonic, help_mnemonic ), 
                                          XmNsubMenuId, p_w->help_pull_down, NULL );
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    root->make_sensitive(help_label, AWM_ALL);
    
    form1 = XtVaCreateManagedWidget( "form1",
    xmFormWidgetClass,
    main_window,
    // XmNwidth, width,
    // XmNheight, height,
    XmNresizePolicy, XmRESIZE_NONE,
    // XmNx, 0,
    // XmNy, 0,
    NULL);

    p_w->mode_area = XtVaCreateManagedWidget( "mode area",
    xmDrawingAreaWidgetClass,
    form1,
    XmNresizePolicy, XmRESIZE_NONE,
    XmNwidth, 38,
    XmNheight, height,
    XmNx, 0,
    XmNy, 0,
    XmNleftOffset, 0,
    XmNtopOffset, 0,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_POSITION,
    XmNtopAttachment, XmATTACH_POSITION,
    XmNmarginHeight, 2,
    XmNmarginWidth, 1,
    NULL);

    separator = XtVaCreateManagedWidget( "separator",
    xmSeparatorWidgetClass,
    form1,
    XmNx, 37,
    XmNshadowThickness, 4,
    XmNorientation, XmVERTICAL,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNtopAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_NONE,
    XmNleftWidget, NULL,
    XmNrightAttachment, XmATTACH_NONE,
    XmNleftOffset, 70,
    XmNleftPosition, 0,
    NULL);

    form2 = XtVaCreateManagedWidget( "form2",
    xmFormWidgetClass,
    form1,
    XmNwidth, width,
    XmNheight, height,
    XmNtopOffset, 0,
    XmNbottomOffset, 0,
    XmNleftOffset, 0,
    XmNrightOffset, 0,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_WIDGET,
    XmNleftWidget, separator,
    XmNtopAttachment, XmATTACH_POSITION,
    XmNresizePolicy, XmRESIZE_NONE,
    XmNx, 0,
    XmNy, 0,
    NULL);
    p_w->areas[AW_INFO_AREA] =
    new AW_area_management(root, form2, XtVaCreateManagedWidget( "info_area",
            xmDrawingAreaWidgetClass,
            form2,
            XmNheight, 0,
            XmNbottomAttachment, XmATTACH_NONE,
            XmNtopAttachment, XmATTACH_FORM,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            XmNmarginHeight, 2,
            XmNmarginWidth, 2,
            NULL));

    p_w->areas[AW_BOTTOM_AREA] =
    new AW_area_management(root, form2, XtVaCreateManagedWidget( "bottom_area",
            xmDrawingAreaWidgetClass,
            form2,
            XmNheight, 0,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNtopAttachment, XmATTACH_NONE,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            NULL));

    p_w->scroll_bar_horizontal = XtVaCreateManagedWidget( "scroll_bar_horizontal",
    xmScrollBarWidgetClass,
    form2,
    XmNheight, 15,
    XmNminimum, 0,
    XmNmaximum, AW_SCROLL_MAX,
    XmNincrement, 10,
    XmNsliderSize, AW_SCROLL_MAX,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 0,
    XmNleftAttachment, XmATTACH_FORM,
    XmNtopAttachment, XmATTACH_NONE,
    XmNorientation, XmHORIZONTAL,
    XmNrightOffset, 18,
    NULL );

    p_w->scroll_bar_vertical = XtVaCreateManagedWidget( "scroll_bar_vertical",
    xmScrollBarWidgetClass,
    form2,
    XmNwidth, 15,
    XmNminimum, 0,
    XmNmaximum, AW_SCROLL_MAX,
    XmNincrement, 10,
    XmNsliderSize, AW_SCROLL_MAX,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_WIDGET,
    XmNbottomWidget, p_w->scroll_bar_horizontal,
    XmNbottomOffset, 3,
    XmNleftOffset, 3,
    XmNrightOffset, 3,
    XmNleftAttachment, XmATTACH_NONE,
    XmNtopAttachment, XmATTACH_WIDGET,
    XmNtopWidget, INFO_WIDGET,
    NULL );

    p_w->frame = XtVaCreateManagedWidget( "draw_area",
    xmFrameWidgetClass,
    form2,
    XmNshadowType, XmSHADOW_IN,
    XmNshadowThickness, 2,
    XmNleftOffset, 3,
    XmNtopOffset, 3,
    XmNbottomOffset, 3,
    XmNrightOffset, 3,
    XmNbottomAttachment, XmATTACH_WIDGET,
    XmNbottomWidget, p_w->scroll_bar_horizontal,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 0,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_WIDGET,
    XmNrightWidget, p_w->scroll_bar_vertical,
    NULL);

    p_w->areas[AW_MIDDLE_AREA] =
    new AW_area_management(root,p_w->frame, XtVaCreateManagedWidget( "draw area",
            xmDrawingAreaWidgetClass,
            p_w->frame,
            XmNmarginHeight, 0,
            XmNmarginWidth, 0,
            NULL));

    XmMainWindowSetAreas( main_window, p_w->menu_bar[0], (Widget) NULL, (Widget) NULL, (Widget) NULL, form1 );

    aw_realize_widget(this);

    create_devices();
    aw_create_help_entry(this);
    create_window_variables();
}

void AW_window_menu::init(AW_root *root_in, const char *wid,
        const char *windowname, int width, int height) {
    Widget main_window;
    Widget help_popup;
    Widget help_label;
    Widget separator;
    Widget form1;
    Widget form2;
    //Widget frame;
    const char *help_button = "HELP";
    const char *help_mnemonic = "H";

#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int posx = 50;
    int posy = 50;

    p_w->shell= aw_create_shell(this, true, true, width, height, posx, posy);

    main_window = XtVaCreateManagedWidget("mainWindow1",
            xmMainWindowWidgetClass, p_w->shell, 
            NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
            main_window, 
            XmNrowColumnType, XmMENU_BAR, 
            NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
            p_w->menu_bar[0], 
            XmNwidth, 1, 
            XmNheight, 1, 
            XmNallowShellResize, true, 
            XmNoverrideRedirect, true, 
            NULL);

    //create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
            xmRowColumnWidgetClass, help_popup, 
            XmNrowColumnType, XmMENU_PULLDOWN, 
            NULL);

    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
            xmCascadeButtonWidgetClass, p_w->menu_bar[0], 
            RES_CONVERT( XmNlabelString, help_button ),
                                          RES_CONVERT( XmNmnemonic, help_mnemonic ), 
                                          XmNsubMenuId, p_w->help_pull_down, NULL );
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    root->make_sensitive(help_label, AWM_ALL);
    
    form1 = XtVaCreateManagedWidget( "form1",
    xmFormWidgetClass,
    main_window,
    // XmNwidth, width,
    // XmNheight, height,
    XmNresizePolicy, XmRESIZE_NONE,
    // XmNx, 0,
    // XmNy, 0,
    NULL);

    p_w->mode_area = XtVaCreateManagedWidget( "mode area",
    xmDrawingAreaWidgetClass,
    form1,
    XmNresizePolicy, XmRESIZE_NONE,
    XmNwidth, 17,
    XmNheight, height,
    XmNx, 0,
    XmNy, 0,
    XmNleftOffset, 0,
    XmNtopOffset, 0,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_POSITION,
    XmNtopAttachment, XmATTACH_POSITION,
    XmNmarginHeight, 2,
    XmNmarginWidth, 1,
    NULL);

    separator = p_w->mode_area;

    form2 = XtVaCreateManagedWidget( "form2",
    xmFormWidgetClass,
    form1,
    XmNwidth, width,
    XmNheight, height,
    XmNtopOffset, 0,
    XmNbottomOffset, 0,
    XmNleftOffset, 0,
    XmNrightOffset, 0,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_WIDGET,
    XmNleftWidget, separator,
    XmNtopAttachment, XmATTACH_POSITION,
    XmNresizePolicy, XmRESIZE_NONE,
    XmNx, 0,
    XmNy, 0,
    NULL);
    p_w->areas[AW_INFO_AREA] =
    new AW_area_management(root,form2, XtVaCreateManagedWidget( "info_area",
            xmDrawingAreaWidgetClass,
            form2,
            XmNheight, 0,
            XmNbottomAttachment, XmATTACH_NONE,
            XmNtopAttachment, XmATTACH_FORM,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            XmNmarginHeight, 2,
            XmNmarginWidth, 2,
            NULL));

    p_w->areas[AW_BOTTOM_AREA] =
    new AW_area_management(root,form2, XtVaCreateManagedWidget( "bottom_area",
            xmDrawingAreaWidgetClass,
            form2,
            XmNheight, 0,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNtopAttachment, XmATTACH_NONE,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            NULL));

    p_w->scroll_bar_horizontal = XtVaCreateManagedWidget( "scroll_bar_horizontal",
    xmScrollBarWidgetClass,
    form2,
    XmNheight, 15,
    XmNminimum, 0,
    XmNmaximum, AW_SCROLL_MAX,
    XmNincrement, 10,
    XmNsliderSize, AW_SCROLL_MAX,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 0,
    XmNleftAttachment, XmATTACH_FORM,
    XmNtopAttachment, XmATTACH_NONE,
    XmNorientation, XmHORIZONTAL,
    XmNrightOffset, 18,
    NULL );

    p_w->scroll_bar_vertical = XtVaCreateManagedWidget( "scroll_bar_vertical",
    xmScrollBarWidgetClass,
    form2,
    XmNwidth, 15,
    XmNminimum, 0,
    XmNmaximum, AW_SCROLL_MAX,
    XmNincrement, 10,
    XmNsliderSize, AW_SCROLL_MAX,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_WIDGET,
    XmNbottomWidget, p_w->scroll_bar_horizontal,
    XmNbottomOffset, 3,
    XmNleftOffset, 3,
    XmNrightOffset, 3,
    XmNleftAttachment, XmATTACH_NONE,
    XmNtopAttachment, XmATTACH_WIDGET,
    XmNtopWidget, INFO_WIDGET,
    NULL );

    p_w->frame = XtVaCreateManagedWidget( "draw_area",
    xmFrameWidgetClass,
    form2,
    XmNshadowType, XmSHADOW_IN,
    XmNshadowThickness, 2,
    XmNleftOffset, 3,
    XmNtopOffset, 3,
    XmNbottomOffset, 3,
    XmNrightOffset, 3,
    XmNbottomAttachment, XmATTACH_WIDGET,
    XmNbottomWidget, p_w->scroll_bar_horizontal,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 0,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_WIDGET,
    XmNrightWidget, p_w->scroll_bar_vertical,
    NULL);

    p_w->areas[AW_MIDDLE_AREA] =
    new AW_area_management(root,p_w->frame, XtVaCreateManagedWidget( "draw area",
            xmDrawingAreaWidgetClass,
            p_w->frame,
            XmNmarginHeight, 0,
            XmNmarginWidth, 0,
            NULL));

    XmMainWindowSetAreas( main_window, p_w->menu_bar[0], (Widget) NULL,
    (Widget) NULL, (Widget) NULL, form1 );

    aw_realize_widget(this);

    create_devices();
    aw_create_help_entry(this);
    create_window_variables();
}

void AW_window_simple::init(AW_root *root_in, const char *wid,
        const char *windowname) {
    //Arg args[10];
    root = root_in; // for macro

    int width = 100; // this is only the minimum size!
    int height = 100;
    int posx = 50;
    int posy = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    p_w->shell= aw_create_shell(this, true, true, width, height, posx, posy);

    // add this to disable resize or maximize in simple dialogs (avoids broken layouts)
    // XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
    //         NULL);

    Widget form1 = XtVaCreateManagedWidget("forms", xmFormWidgetClass,
            p_w->shell, 
            NULL);

    p_w->areas[AW_INFO_AREA] = new AW_area_management(root,form1, XtVaCreateManagedWidget( "info_area",
                    xmDrawingAreaWidgetClass,
                    form1,
                    XmNbottomAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_FORM,
                    XmNleftAttachment, XmATTACH_FORM,
                    XmNrightAttachment, XmATTACH_FORM,
                    XmNmarginHeight, 2,
                    XmNmarginWidth, 2,
                    NULL));

    aw_realize_widget(this);
    create_devices();
}

void AW_window_simple_menu::init(AW_root *root_in, const char *wid,
        const char *windowname) {
    //  Arg args[10];

    root = root_in; // for macro

    const char *help_button = "HELP";
    const char *help_mnemonic = "H";
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int width = 100;
    int height = 100;
    int posx = 50;
    int posy = 50;

    p_w->shell= aw_create_shell(this, true, true, width, height, posx, posy);

    Widget main_window;
    Widget help_popup;
    Widget help_label;
    Widget form1;

    main_window = XtVaCreateManagedWidget("mainWindow1",
            xmMainWindowWidgetClass, p_w->shell, 
            NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
            main_window, 
            XmNrowColumnType, XmMENU_BAR, 
            NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
            p_w->menu_bar[0], 
            XmNwidth, 1, 
            XmNheight, 1, 
            XmNallowShellResize, true, 
            XmNoverrideRedirect, true, 
            NULL);

    //create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
            xmRowColumnWidgetClass, help_popup, 
            XmNrowColumnType, XmMENU_PULLDOWN, 
            NULL);

    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
            xmCascadeButtonWidgetClass, p_w->menu_bar[0], 
            RES_CONVERT( XmNlabelString, help_button ),
                                          RES_CONVERT( XmNmnemonic, help_mnemonic ), 
                                          XmNsubMenuId, p_w->help_pull_down, NULL );
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    root->make_sensitive(help_label, AWM_ALL);
    
    form1 = XtVaCreateManagedWidget( "form1",
    xmFormWidgetClass,
    main_window,
    XmNtopOffset, 10,
    XmNresizePolicy, XmRESIZE_NONE,
    NULL);

    p_w->areas[AW_INFO_AREA] =
    new AW_area_management(root, form1, XtVaCreateManagedWidget( "info_area",
            xmDrawingAreaWidgetClass,
            form1,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNtopAttachment, XmATTACH_FORM,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            XmNmarginHeight, 2,
            XmNmarginWidth, 2,
            NULL));

    aw_realize_widget(this );

    aw_create_help_entry(this);
    create_devices();
}

void AW_window_message::init(AW_root *root_in, const char *windowname,
        bool allow_close) {
    //  Arg args[10];

    root = root_in; // for macro

    int width = 100;
    int height = 100;
    int posx = 50;
    int posy = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(window_name);

    // create shell for message box
    p_w->shell= aw_create_shell(this, true, allow_close, width, height, posx, posy);

    // disable resize or maximize in simple dialogs (avoids broken layouts)
    XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
            NULL);

    p_w->areas[AW_INFO_AREA] = new AW_area_management(root,p_w->shell, XtVaCreateManagedWidget( "info_area",
                    xmDrawingAreaWidgetClass,
                    p_w->shell,
                    XmNheight, 0,
                    XmNbottomAttachment, XmATTACH_NONE,
                    XmNtopAttachment, XmATTACH_FORM,
                    XmNleftAttachment, XmATTACH_FORM,
                    XmNrightAttachment, XmATTACH_FORM,
                    NULL));

    aw_realize_widget(this);
    //  create_devices();
}

void AW_window::set_info_area_height(int height) {
    XtVaSetValues(INFO_WIDGET, XmNheight, height, NULL);
    XtVaSetValues(p_w->frame, XmNtopOffset, height, NULL);
}

void AW_window::set_bottom_area_height(int height) {
    XtVaSetValues(BOTTOM_WIDGET, XmNheight, height, NULL);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNbottomOffset, (int)height,
            NULL);
}

void AW_window::set_vertical_scrollbar_top_indent(int indent) {
    XtVaSetValues(p_w->scroll_bar_vertical, XmNtopOffset, (int)indent, NULL);
    top_indent_of_vertical_scrollbar = indent;
}

void AW_window::set_vertical_scrollbar_bottom_indent(int indent) {
    XtVaSetValues(p_w->scroll_bar_vertical, XmNbottomOffset, (int)(3+indent),
            NULL);
    bottom_indent_of_vertical_scrollbar = indent;
}

void AW_root::apply_sensitivity(AW_active mask) {
    aw_assert(legal_mask(mask));
    AW_buttons_struct *list;

    global_mask = mask;
    for (list = p_r->button_list; list; list = list->next) {
        XtSetSensitive(list->button, (list->mask & mask) ? True : False);
    }
}

void AW_window::select_mode(int mode) {
    if (mode >= p_w->number_of_modes)
        return;

    Widget oldwidget = p_w->modes_widgets[p_w->selected_mode];
    p_w->selected_mode = mode;
    Widget widget = p_w->modes_widgets[p_w->selected_mode];
    XtVaSetValues(oldwidget, XmNbackground, p_global->background, NULL);
    XtVaSetValues(widget, XmNbackground, p_global->foreground, NULL);
}

static void aw_mode_callback(AW_window *aww, long mode, AW_cb_struct *cbs) {
    aww->select_mode((int)mode);
    cbs->run_callback();
}

#define MODE_BUTTON_OFFSET 34
inline int yoffset_for_mode_button(int button_number) {
    return button_number*MODE_BUTTON_OFFSET + (button_number/4)*8 + 2;
}

int AW_window::create_mode(const char *pixmap, const char *helpText, AW_active Mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    aw_assert(legal_mask(Mask));
    Widget button;

    TuneBackground(p_w->mode_area, TUNE_BUTTON); // set background color for mode-buttons

    const char *path = GB_path_in_ARBLIB("pixmaps", pixmap);

    int y = yoffset_for_mode_button(p_w->number_of_modes);
    button = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, p_w->mode_area,
                                     XmNx,               0,
                                     XmNy,               y,
                                     XmNlabelType,       XmPIXMAP,
                                     XmNshadowThickness, 1,
                                     XmNbackground,      _at->background_color,
                                     NULL);
    XtVaSetValues(button, RES_CONVERT( XmNlabelPixmap, path ),NULL );
    XtVaGetValues(button,XmNforeground, &p_global->foreground, NULL);

    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, 0);
    AW_cb_struct *cb2 = new AW_cb_struct(this, (AW_CB)aw_mode_callback, (AW_CL)p_w->number_of_modes, (AW_CL)cbs, helpText, cbs);
    XtAddCallback(button, XmNactivateCallback,
    (XtCallbackProc) AW_server_callback,
    (XtPointer) cb2);

    if (!p_w->modes_f_callbacks) {
        p_w->modes_f_callbacks = (AW_cb_struct **)GB_calloc(sizeof(AW_cb_struct*),AW_NUMBER_OF_F_KEYS); // valgrinders : never freed because AW_window never is freed
    }
    if (!p_w->modes_widgets) {
        p_w->modes_widgets = (Widget *)GB_calloc(sizeof(Widget),AW_NUMBER_OF_F_KEYS);
    }
    if (p_w->number_of_modes<AW_NUMBER_OF_F_KEYS) {
        p_w->modes_f_callbacks[p_w->number_of_modes] = cb2;
        p_w->modes_widgets[p_w->number_of_modes] = button;
    }

    root->make_sensitive(button, Mask);
    p_w->number_of_modes++;

    int ynext = yoffset_for_mode_button(p_w->number_of_modes);
    if (ynext> _at->max_y_size) _at->max_y_size = ynext;

    return p_w->number_of_modes;
}

// ------------------------
//      Hotkey Checking
// ------------------------

#ifdef DEBUG

#define MAX_DEEP_TO_TEST       10
#define MAX_MENU_ITEMS_TO_TEST 50

static char *TD_menu_name = 0;
static char  TD_mnemonics[MAX_DEEP_TO_TEST][MAX_MENU_ITEMS_TO_TEST];
static int   TD_topics[MAX_DEEP_TO_TEST];

struct SearchPossibilities {
    char *menu_topic;
    SearchPossibilities *next;

    SearchPossibilities(const char* menu_topic_, SearchPossibilities *next_) {
        menu_topic = strdup(menu_topic_);
        next = next_;
    }

    ~SearchPossibilities() {
        free(menu_topic);
        delete next;
    }
};

typedef SearchPossibilities *SearchPossibilitiesPtr;
static SearchPossibilitiesPtr TD_poss[MAX_DEEP_TO_TEST] = { 0 };

inline void addToPoss(int menu_deep, const char *topic_name) {
    TD_poss[menu_deep] = new SearchPossibilities(topic_name, TD_poss[menu_deep]);
}

inline char oppositeCase(char c) {
    return isupper(c) ? tolower(c) : toupper(c);
}

static void strcpy_overlapping(char *dest, char *src) {
    int src_len = strlen(src);
    memmove(dest, src, src_len+1);
}

static const char *possible_mnemonics(int menu_deep, const char *topic_name) {
    int t;
    static char *unused;

    freedup(unused, topic_name);

    for (t = 0; unused[t]; ++t) {
        bool remove = false;
        if (!isalnum(unused[t])) { // remove useless chars
            remove = true;
        } else {
            char *dup = strchr(unused, unused[t]);
            if (dup && (dup-unused)<t) { // remove duplicated chars
                remove = true;
            } else {
                dup = strchr(unused, oppositeCase(unused[t]));
                if (dup && (dup-unused)<t) { // char is duplicated with opposite case
                    dup[0] = toupper(dup[0]); // prefer upper case
                    remove = true;
                }
            }
        }
        if (remove) {
            strcpy_overlapping(unused+t, unused+t+1);
            --t;
        }
    }

    int topics = TD_topics[menu_deep];
    for (t = 0; t<topics; ++t) {
        char c = TD_mnemonics[menu_deep][t]; // upper case!
        char *u = strchr(unused, c);
        if (u)
            strcpy_overlapping(u, u+1); // remove char
        u = strchr(unused, tolower(c));
        if (u)
            strcpy_overlapping(u, u+1); // remove char
    }

    return unused;
}

static void printPossibilities(int menu_deep) {
    SearchPossibilities *sp = TD_poss[menu_deep];
    while (sp) {
        const char *poss = possible_mnemonics(menu_deep, sp->menu_topic);
        fprintf(stderr, "          - Possibilities for '%s': '%s'\n", sp->menu_topic, poss);

        sp = sp->next;
    }

    delete TD_poss[menu_deep];
    TD_poss[menu_deep] = 0;
}

static int menu_deep_check = 0;

static void test_duplicate_mnemonics(int menu_deep, const char *topic_name, const char *mnemonic) {
    if (mnemonic && mnemonic[0] != 0) {
        if (mnemonic[1]) { // longer than 1 char -> wrong
            fprintf(stderr, "Warning: Hotkey '%s' is too long; only 1 character allowed (%s|%s)\n", mnemonic, TD_menu_name, topic_name);
        }
        if (topic_name[0] == '#') { // graphical menu
            if (mnemonic[0]) {
                fprintf(stderr, "Warning: Hotkey '%s' is useless for graphical menu entry (%s|%s)\n", mnemonic, TD_menu_name, topic_name);
            }
        }
        else {
            // fprintf(stderr, "- menu_deep=%i, TD_menu_name='%s', topic_name='%s' mnemonic='%s'\n", menu_deep, TD_menu_name, topic_name, mnemonic);
            if (strchr(topic_name, mnemonic[0])) {  // occurs in menu text
                int topics = TD_topics[menu_deep];
                int t;
                char hotkey = toupper(mnemonic[0]); // store hotkeys case-less (case does not matter when pressing the hotkey)

                TD_mnemonics[menu_deep][topics] = hotkey;

                for (t=0; t<topics; t++) {
                    if (TD_mnemonics[menu_deep][t]==hotkey) {
                        fprintf(stderr, "Warning: Hotkey '%c' used twice (%s|%s)\n", hotkey, TD_menu_name, topic_name);
                        addToPoss(menu_deep, topic_name);
                        break;
                    }
                }

                TD_topics[menu_deep] = topics+1;
            }
            else {
                fprintf(stderr, "Warning: Hotkey '%c' is useless; does not occur in text (%s|%s)\n", mnemonic[0], TD_menu_name, topic_name);
                addToPoss(menu_deep, topic_name);
            }
        }
    }
#if defined(DEVEL_RALF)
    else {
        if (topic_name[0] != '#') { // not a graphical menu
            fprintf(stderr, "Warning: Missing hotkey for (%s|%s)\n", TD_menu_name, topic_name);
            addToPoss(menu_deep, topic_name);
        }
    }
#endif // DEVEL_RALF
}

static void open_test_duplicate_mnemonics(int menu_deep, const char *sub_menu_name, const char *mnemonic) {
    aw_assert(menu_deep == menu_deep_check+1);
    menu_deep_check = menu_deep;

    int len = strlen(TD_menu_name)+1+strlen(sub_menu_name)+1;
    char *buf = (char*)malloc(len);

    memset(buf, 0, len);
    sprintf(buf, "%s|%s", TD_menu_name, sub_menu_name);

    test_duplicate_mnemonics(menu_deep-1, sub_menu_name, mnemonic);

    freeset(TD_menu_name, buf);
    TD_poss[menu_deep] = 0;
}

static void close_test_duplicate_mnemonics(int menu_deep) {
    aw_assert(menu_deep == menu_deep_check);
    menu_deep_check = menu_deep-1;

    printPossibilities(menu_deep);
    TD_topics[menu_deep] = 0;

    aw_assert(TD_menu_name);
    // otherwise no menu was opened

    char *slash = strrchr(TD_menu_name, '|');
    if (slash) {
        slash[0] = 0;
    } else {
        TD_menu_name[0] = 0;
    }
}

static void init_duplicate_mnemonic() {
    int i;

    if (TD_menu_name) close_test_duplicate_mnemonics(1); // close last menu
    freedup(TD_menu_name, "");

    for (i=0; i<MAX_DEEP_TO_TEST; i++) {
        TD_topics[i] = 0;
    }
    aw_assert(menu_deep_check == 0);
}
static void exit_duplicate_mnemonic() {
    close_test_duplicate_mnemonics(1); // close last menu
    aw_assert(TD_menu_name);
    freeset(TD_menu_name, 0);
    aw_assert(menu_deep_check == 0);
}
#endif

// --------------------------------------------------------------------------------

void AW_window::create_menu(AW_label name, const char *mnemonic, AW_active Mask) {
    aw_assert(legal_mask(Mask));
    p_w->menu_deep = 0;
#ifdef DEBUG
    init_duplicate_mnemonic();
#endif
#if defined(DUMP_MENU_LIST)
    dumpCloseAllSubMenus();
#endif // DUMP_MENU_LIST
    insert_sub_menu(name, mnemonic, Mask);
}

void AW_window::all_menus_created() { // this is called by AW_window::show() (i.e. after all menus have been created)
#if defined(DEBUG)
    if (p_w->menu_deep>0) { // window had menu
        aw_assert(p_w->menu_deep == 1);
        // some unclosed sub-menus ?
        if (menu_deep_check == 1) { // otherwise the window is just re-shown (already has been checked!)
            exit_duplicate_mnemonic();
        }
    }
#endif // DEBUG
}

void AW_window::insert_sub_menu(AW_label name, const char *mnemonic, AW_active Mask) {
    aw_assert(legal_mask(Mask));
    Widget shell, Label;

    TuneBackground(p_w->menu_bar[p_w->menu_deep], TUNE_SUBMENU); // set background color for submenus
    // (Note: This must even be called if TUNE_SUBMENU is 0!
    //        Otherwise several submenus get the TUNE_MENUTOPIC color)

#if defined(DUMP_MENU_LIST)
    dumpOpenSubMenu(name);
#endif // DUMP_MENU_LIST
#ifdef DEBUG
    open_test_duplicate_mnemonics(p_w->menu_deep+1, name, mnemonic);
#endif

    // create shell for Pull-Down
    shell = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
            p_w->menu_bar[p_w->menu_deep], 
            XmNwidth, 1, 
            XmNheight, 1, 
            XmNallowShellResize, true, 
            XmNoverrideRedirect, true, 
            NULL);

    //create row column in Pull-Down shell

    p_w->menu_bar[p_w->menu_deep+1] = XtVaCreateWidget("menu_row_column",
            xmRowColumnWidgetClass, shell, 
            XmNrowColumnType, XmMENU_PULLDOWN, 
            XmNtearOffModel, XmTEAR_OFF_ENABLED, 
            NULL);

    // create label in menu bar
    if (mnemonic && *mnemonic && strchr(name, mnemonic[0])) {
        // if mnemonic is "" -> Cannot convert string "" to type KeySym
        Label = XtVaCreateManagedWidget("menu1_top_b1",
                xmCascadeButtonWidgetClass, p_w->menu_bar[p_w->menu_deep], 
                RES_CONVERT( XmNlabelString, name ),
                                         RES_CONVERT( XmNmnemonic, mnemonic ), 
                                         XmNsubMenuId, p_w->menu_bar[p_w->menu_deep+1], 
                                         XmNbackground, _at->background_color, NULL );
    }
    else {
        Label = XtVaCreateManagedWidget( "menu1_top_b1",
        xmCascadeButtonWidgetClass,
        p_w->menu_bar[p_w->menu_deep],
        RES_CONVERT( XmNlabelString, name ),
        XmNsubMenuId, p_w->menu_bar[p_w->menu_deep+1],
        XmNbackground, _at->background_color,
        NULL);
    }

    if (p_w->menu_deep < AW_MAX_MENU_DEEP-1) p_w->menu_deep++;

    root->make_sensitive(Label, Mask);
}

void AW_window::close_sub_menu(void) {
#ifdef DEBUG
    close_test_duplicate_mnemonics(p_w->menu_deep);
#endif
#if defined(DUMP_MENU_LIST)
    dumpCloseSubMenu();
#endif // DUMP_MENU_LIST
    if (p_w->menu_deep>0)
        p_w->menu_deep--;
}

void AW_window::insert_menu_topic(const char *topic_id, AW_label name,
                                  const char *mnemonic, const char *helpText, AW_active Mask,
                                  void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2)
{
    aw_assert(legal_mask(Mask));
    Widget button;
    
    if (!topic_id) topic_id = name; // hmm, due to this we cannot insert_menu_topic w/o id. Change? @@@

    TuneBackground(p_w->menu_bar[p_w->menu_deep], TUNE_MENUTOPIC); // set background color for normal menu topics

#if defined(DUMP_MENU_LIST)
    dumpMenuEntry(name);
#endif // DUMP_MENU_LIST
#ifdef DEBUG
    test_duplicate_mnemonics(p_w->menu_deep, name, mnemonic);
#endif
    if (mnemonic && *mnemonic && strchr(name, mnemonic[0])) {
        // create one sub-menu-point
        button = XtVaCreateManagedWidget("", xmPushButtonWidgetClass,
                p_w->menu_bar[p_w->menu_deep], 
                RES_LABEL_CONVERT( name ),
                                          RES_CONVERT( XmNmnemonic, mnemonic ), 
                                          XmNbackground, _at->background_color, NULL );
    } else {
        button = XtVaCreateManagedWidget( "",
        xmPushButtonWidgetClass,
        p_w->menu_bar[p_w->menu_deep],
        RES_LABEL_CONVERT( name ),
        XmNbackground, _at->background_color,
        NULL);
    }

    AW_label_in_awar_list(this,button,name);
    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, helpText);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) AW_server_callback,
                  (XtPointer) cbs);

    cbs->id = strdup(topic_id);
    root->define_remote_command(cbs);
    root->make_sensitive(button, Mask);
}

void AW_window::insert_help_topic(AW_label name, const char *mnemonic, const char *helpText, AW_active Mask,
                                  void (*f)(AW_window*, AW_CL , AW_CL ), AW_CL cd1, AW_CL cd2)
{
    aw_assert(legal_mask(Mask));
    Widget button;

    // create one help-sub-menu-point
    button = XtVaCreateManagedWidget("", xmPushButtonWidgetClass,
            p_w->help_pull_down, 
            RES_CONVERT( XmNlabelString, name ),
                                      RES_CONVERT( XmNmnemonic, mnemonic ), NULL );
    XtAddCallback(button, XmNactivateCallback,
    (XtCallbackProc) AW_server_callback,
    (XtPointer) new AW_cb_struct(this, f, cd1, cd2, helpText));

    root->make_sensitive(button, Mask);
}

void AW_window::insert_separator(void) {
    Widget separator;

    // create one help-sub-menu-point
    separator = XtVaCreateManagedWidget("", xmSeparatorWidgetClass,
            p_w->menu_bar[p_w->menu_deep], 
            NULL);
}

void AW_window::insert_separator_help(void) {
    Widget separator;

    // create one help-sub-menu-point
    separator = XtVaCreateManagedWidget("", xmSeparatorWidgetClass,
            p_w->help_pull_down, 
            NULL);
}

AW_area_management::AW_area_management(AW_root *awr, Widget formi, Widget widget) {
    memset((char *)this, 0, sizeof(AW_area_management));
    form = formi;
    area = widget;
    XtAddEventHandler(area, EnterWindowMask, FALSE, AW_root_focusCB, (XtPointer)awr);
}

AW_device *AW_window::get_device(AW_area area) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return 0;
    if (!aram->device)
        aram->device = new AW_device_Xm(aram->common);
    aram->device->init();
    return (AW_device *)(aram->device);
}

AW_device *AW_window::get_size_device(AW_area area) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return 0;
    if (!aram->size_device)
        aram->size_device = new AW_device_size(aram->common);
    aram->size_device->init();
    aram->size_device->reset();
    return (AW_device *)(aram->size_device);
}

AW_device *AW_window::get_print_device(AW_area area) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return 0;
    if (!aram->print_device)
        aram->print_device = new AW_device_print(aram->common);
    aram->print_device->init();
    return (AW_device *)(aram->print_device);
}

AW_device *AW_window::get_click_device(AW_area area, int mousex, int mousey,
        AW_pos max_distance_linei, AW_pos max_distance_texti, AW_pos radi) {
    AW_area_management *aram= MAP_ARAM(area);
    if (!aram)
        return 0;
    if (!aram->click_device)
        aram->click_device = new AW_device_click(aram->common);
    aram->click_device->init(mousex, mousey, max_distance_linei,
                             max_distance_texti, radi, (AW_bitset)-1);
    return (AW_device *)(aram->click_device);
}

void AW_window::wm_activate() {
    {
        Boolean iconic = False;
        XtVaGetValues(p_w->shell, XmNiconic, &iconic, NULL) ;

        if (iconic == True) {
            XtVaSetValues(p_w->shell, XmNiconic, False, NULL) ;

            XtMapWidget(p_w->shell) ;
            XRaiseWindow(XtDisplay(p_w->shell), XtWindow(p_w->shell)) ;
        }
    }

    {
        Display *xdpy            = XtDisplay(p_w->shell);
        Window   window          = XtWindow(p_w->shell);
        Atom     netactivewindow = XInternAtom(xdpy, "_NET_ACTIVE_WINDOW", False);

        if (netactivewindow) {

            XClientMessageEvent ce;
            ce.type         = ClientMessage;
            ce.display      = xdpy;
            ce.window       = window;
            ce.message_type = netactivewindow;
            ce.format       = 32;
            ce.data.l[0]    = 2;
            ce.data.l[1]    = None;
            ce.data.l[2]    = Above;
            ce.data.l[3]    = 0;
            ce.data.l[4]    = 0;

#if defined(DEBUG)
            Status ret = 
#endif // DEBUG
                XSendEvent(xdpy, XDefaultRootWindow(xdpy),
                           False,
                           SubstructureRedirectMask | SubstructureNotifyMask,
                           (XEvent *) &ce);

#if defined(DEBUG)
            if (!ret) { fprintf(stderr, "Failed to send _NET_ACTIVE_WINDOW to WM (XSendEvent returns %i)\n", ret); }
#endif // DEBUG
            XSync(xdpy, False);
        }
#if defined(DEBUG)
        else {
            fputs("No such atom '_NET_ACTIVE_WINDOW'\n", stderr);
        }
#endif // DEBUG
    }
}

void AW_window::show_internal(void *cl_grab) {
    XtGrabKind grab = *(XtGrabKind*)cl_grab;

    if (!window_is_shown) {
        all_menus_created();
        get_root()->window_show();
        window_is_shown = true;
    }

    if (recalc_size_at_show != AW_KEEP_SIZE) {
        if (recalc_size_at_show == AW_RESIZE_DEFAULT) {
            window_fit();
        }
        else {
            aw_assert(recalc_size_at_show == AW_RESIZE_USER);
            // check whether user size is too small and increase to minimum (aka default)
            int default_width, default_height;
            get_window_size(default_width, default_height);
            AW_root *tmp_root = get_root();
            int user_width = tmp_root->awar(aw_awar_name_width(this))->read_int();
            int user_height = tmp_root->awar(aw_awar_name_height(this))->read_int();

#if defined(DEBUG)
            // printf("default size = %i/%i  user size = %i/%i\n", default_width, default_height, user_width, user_height);
#endif // DEBUG
            if (user_width<default_width)
                user_width = default_width;
            if (user_height<default_height)
                user_height = default_height;

#if defined(DEBUG)
            // printf("using size = %i/%i\n", user_width, user_height);
#endif // DEBUG
            set_window_size(user_width, user_height);
        }
        recalc_size_at_show = AW_KEEP_SIZE;
    }

    if (recalc_pos_at_show != AW_KEEP_POS) {
        int  posx, posy;
        bool setPos = false;

        switch (recalc_pos_at_show) {
            case AW_REPOS_TO_MOUSE_ONCE:
                recalc_pos_at_show = AW_KEEP_POS;
                // fallthrough
            case AW_REPOS_TO_MOUSE: {
                int mx, my; if (!get_mouse_pos(mx, my)) goto FALLBACK_CENTER;
                int width, height; get_window_size(width, height);
                int wx, wy; get_window_pos(wx, wy);

                int wx2 = wx+width-1;
                int wy2 = wy+height-1;

                if (mx<wx || mx>wx2 || my<wy || my>wy2) { // mouse is outside window
                    setPos = true;
                    posx   = mx-width/2; // center window on mouse position
                    posy   = my-height/2;
                }
                break;
            }
            case AW_REPOS_TO_CENTER: {
            FALLBACK_CENTER: 
                int width, height; get_window_size(width, height);
                int swidth, sheight; get_screen_size(swidth, sheight);
                
                setPos = true;
                posx   = (swidth-width)/2;
                posy   = (sheight-height)/4;
                break;
            }

            case AW_KEEP_POS:
                aw_assert(0);
                break;
        }

        if (setPos) {
            int currx, curry;
            get_window_pos(currx, curry);
            if (currx != posx || curry != posy) {
#if defined(DEBUG) && 0
                printf("force position at show_internal: %i/%i\n", posx, posy);
#endif // DEBUG
                set_window_pos(posx, posy);
            }
        }
    }

    XtPopup(p_w->shell, grab);
    if (p_w->WM_top_offset == AW_FIX_POS_ON_EXPOSE) {              // very bad hack
        set_expose_callback(AW_INFO_AREA, (AW_CB)aw_calculate_WM_offsets, 0, 0);
    }
}

void AW_window::show(void) {
    XtGrabKind grab = XtGrabNone;
    show_internal(&grab);
}

void AW_window::show_grabbed(void) {
    XtGrabKind grab = XtGrabExclusive;
    show_internal(&grab);
}

void AW_window::hide(void) {
    if (window_is_shown) {
        get_root()->window_hide();
        window_is_shown = false;
    }
    XtPopdown(p_w->shell);
}

bool AW_window::is_shown(void) {
    // return true if window is shown ( = not invisible and already created)
    // Note: does return TRUE!, if window is only minimized by WM
    return window_is_shown;
}

void AW_root::window_show() {
    active_windows++;
}

void AW_root::window_hide() {
    active_windows--;
    if (active_windows<0) {
        exit(0);
    }
}

void AW_root::main_loop(void) {
    XtAppMainLoop(p_r->context);
}

void AW_root::process_events(void) {
    XtAppProcessEvent(p_r->context,XtIMAll);
}
void AW_root::process_pending_events(void) {
    XtInputMask pending = XtAppPending(p_r->context);
    while (pending) {
        XtAppProcessEvent(p_r->context, pending);
        pending = XtAppPending(p_r->context);
    }
}

/** Returns type if key event follows, else 0 */

AW_ProcessEventType AW_root::peek_key_event(AW_window */*aww*/) {
    XEvent xevent;
    Boolean result = XtAppPeekEvent(p_r->context,&xevent);

#if defined(DEBUG) && 0
    printf("peek_key_event\n");
#endif // DEBUG
    if (!result)
        return NO_EVENT;
    if ( (xevent.type != KeyPress) && (xevent.type != KeyRelease))
        return NO_EVENT;
    //XKeyEvent *kev = &xevent.xkey;
    return (AW_ProcessEventType)xevent.type;
}

static void timed_window_title_cb(class AW_root* aw_root, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw_root);
    char *title = (char *)cd1;
    AW_window *aw = (AW_window *)cd2;

    aw->number_of_timed_title_changes--;
    if ( !aw->number_of_timed_title_changes) {
        aw->set_window_title_intern(title);
    }

    delete title;
}
void AW_window::message(char *title, int ms) {
    char *old_title= NULL;

    number_of_timed_title_changes++;

    old_title = strdup(window_name);

    XtVaSetValues(p_w->shell, XmNtitle, title, NULL);

    get_root()->add_timed_callback(ms, timed_window_title_cb, (AW_CL)old_title,
            (AW_CL)this );

}

void AW_window::set_window_title_intern(char *title) {
    XtVaSetValues(p_w->shell, XmNtitle, title, NULL);
}

void AW_window::set_window_title(const char *title) {
    XtVaSetValues(p_w->shell, XmNtitle, title, NULL);
    freedup(window_name, title);
}

const char *AW_window::get_window_title(void) {
    char *title;

    XtVaGetValues(p_w->shell, XmNtitle, &title, NULL);

    return title;
}

const char *AW_window::local_id(const char *id) const {
    static char *last_local_id = 0;
    freeset(last_local_id, GBS_global_string_copy("%s/%s", get_window_id(), id));
    return last_local_id;
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/
static void AW_xfigCB_info_area(AW_window *aww, AW_xfig *xfig) {

    AW_device *device = aww->get_device(AW_INFO_AREA);
    device->reset();
    if (aww->get_root()->color_mode == 0) { // mono colr
        device->clear(-1);
    }
    device->set_offset(AW::Vector(-xfig->minx, -xfig->miny));
    xfig->print(device);
}

void AW_window::load_xfig(const char *file, bool resize) {
    AW_xfig *xfig;

    if (file)   xfig = new AW_xfig(file, get_root()->font_width, get_root()->font_height);
    else        xfig = new AW_xfig(get_root()->font_width, get_root()->font_height); // create an empty xfig

    xfig_data = (void*)xfig;

    set_expose_callback(AW_INFO_AREA, (AW_CB)AW_xfigCB_info_area, (AW_CL)xfig_data, 0);
    xfig->create_gcs(get_device(AW_INFO_AREA), get_root()->color_mode ? 8 : 1);

    int xsize = xfig->maxx - xfig->minx;
    int ysize = xfig->maxy - xfig->miny;

    if (xsize>_at->max_x_size) _at->max_x_size = xsize;
    if (ysize>_at->max_y_size) _at->max_y_size = ysize;

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at->max_x_size+1000, _at->max_y_size+1000);
    }
}

void AW_window::draw_line(int x1, int y1, int x2, int y2, int width, bool resize) {
    AW_xfig *xfig = (AW_xfig*)xfig_data;
    aw_assert(xfig);
    // forgot to call load_xfig ?

    xfig->add_line(x1, y1, x2, y2, width);

    class x {
public:
        static inline int max(int i1, int i2) {
            return i1>i2 ? i1 : i2;
        }
    };

    _at->max_x_size = x::max(_at->max_x_size, xfig->maxx - xfig->minx);
    _at->max_y_size = x::max(_at->max_y_size, xfig->maxy - xfig->miny);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at->max_x_size+1000, _at->max_y_size+1000);
    }
}

void AW_window::_set_activate_callback(void *widget) {
    if (_callback && (long)_callback != 1) {
        if (!_callback->help_text && _at->helptext_for_next_button) {
            _callback->help_text = _at->helptext_for_next_button;
            _at->helptext_for_next_button = 0;
        }

        XtAddCallback((Widget) widget, XmNactivateCallback,
                (XtCallbackProc) AW_server_callback, (XtPointer) _callback );
    }
    _callback = NULL;
}

/***********************************************************************/
/*****************      AW_MACRO_MESSAGE     *******************/
/***********************************************************************/

#define AW_MESSAGE_AWAR "tmp/message/macro"

static void macro_message_cb(AW_window *aw, AW_CL) {
    AW_root *root = aw->get_root();
    aw->hide();

    if (root->prvt->recording_macro_file) {
        char *s = root->awar(AW_MESSAGE_AWAR)->read_string();
        fprintf(root->prvt->recording_macro_file, "MESSAGE\t");
        GBS_fwrite_string(s, root->prvt->recording_macro_file);
        fprintf(root->prvt->recording_macro_file, "\n");
        delete s;
    }

    if (root->prvt->executing_macro_file) {
        //root->enable_execute_macro(); @@@@
    }

    return;
}

static void aw_clear_macro_message_cb(AW_window *aww) {
    aww->get_root()->awar(AW_MESSAGE_AWAR)->write_string("");
}

void aw_macro_message(const char *templat, ...)
// @@@ this function is unused.
{

    AW_root *root = AW_root::THIS;
    char buffer[10000];
    {
        va_list parg;
        va_start(parg,templat);
        vsprintf(buffer, templat, parg);
    }
    static AW_window_message *aw_msg = 0;

    root->awar_string(AW_MESSAGE_AWAR)->write_string(buffer);

    if (!aw_msg) {
        aw_msg = new AW_window_message;

        aw_msg->init(root, "MESSAGE", false);
        aw_msg->load_xfig("macro_message.fig");

        aw_msg->at("clear");
        aw_msg->callback(aw_clear_macro_message_cb);
        aw_msg->create_button("OK", "OK", "O");

        aw_msg->at("Message");
        aw_msg->create_text_field(AW_MESSAGE_AWAR);

        aw_msg->at("hide");
        aw_msg->callback(macro_message_cb, 0);
        aw_msg->create_button("OK", "OK", "O");
    }

    aw_msg->show();
    if (root->prvt->executing_macro_file) {
        root->stop_execute_macro();
    }
}

GB_ERROR AW_root::start_macro_recording(const char *file,
        const char *application_id, const char *stop_action_name) {
    if (prvt->recording_macro_file) {
        return GB_export_error("Already Recording Macro");
    }
    char *path = 0;
    if (file[0] == '/') {
        path = strdup(file);
    } else {
        path = GBS_global_string_copy("%s/%s", GB_getenvARBMACROHOME(), file);
    }
    char *macro_header = GB_read_file("$(ARBHOME)/lib/macro.head");
    if (!macro_header) {
        return GB_export_errorf("Cannot open file '%s'", "$(ARBHOME)/lib/macro.head");
    }

    prvt->recording_macro_file = fopen(path, "w");
    prvt->recording_macro_path = path;
    if (!prvt->recording_macro_file) {
        delete macro_header;
        return GB_export_errorf("Cannot open file '%s' for writing", file);
    }
    prvt->stop_action_name = strdup(stop_action_name);
    prvt->application_name_for_macros = strdup(application_id);

    fprintf(prvt->recording_macro_file, "%s", macro_header);
    free(macro_header);
    return 0;
}

GB_ERROR AW_root::stop_macro_recording() {
    if (!prvt->recording_macro_file) {
        return GB_export_error("Not recording macro");
    }
    fprintf(prvt->recording_macro_file, "ARB::close($gb_main);");
    fclose(prvt->recording_macro_file);

    long mode = GB_mode_of_file(prvt->recording_macro_path);

    GB_set_mode_of_file(prvt->recording_macro_path, mode | ((mode >> 2)& 0111));
    prvt->recording_macro_file = 0;

    freeset(prvt->recording_macro_path, 0);
    freeset(prvt->stop_action_name, 0);
    freeset(prvt->application_name_for_macros, 0);

    return 0;
}

GB_ERROR AW_root::execute_macro(const char *file) {
    char *path = 0;
    if (file[0] == '/') {
        path = strdup(file);
    } else {
        path = GBS_global_string_copy("%s/%s", GB_getenvARBMACROHOME(), file);
    }
    const char *com = GBS_global_string("perl %s &", path);
    printf("[Action '%s']\n", com);
    if (system(com)) {
        aw_message(GBS_global_string("Calling '%s' failed", com));
    }
    free(path);
    return 0;
}

void AW_root::stop_execute_macro() {

}

void AW_root::define_remote_command(AW_cb_struct *cbs) {
    if (cbs->f == (AW_CB)AW_POPDOWN) {
        aw_assert(!cbs->get_cd1() && !cbs->get_cd2()); // popdown takes no parameters (please pass ", 0, 0"!)
    }

    AW_cb_struct *old_cbs = (AW_cb_struct*)GBS_write_hash(prvt->action_hash, cbs->id, (long)cbs);
    if (old_cbs) {
        if (!old_cbs->is_equal(*cbs)) {                  // existing remote command replaced by different callback
#if defined(DEBUG)
            fputs(GBS_global_string("Warning: reused callback id '%s' for different callback\n", old_cbs->id), stderr);
#if defined(DEVEL_RALF)
            gb_assert(0);
#endif // DEVEL_RALF
#endif // DEBUG
        }
        // do not free old_cbs, cause it's still reachable from first widget that defined this remote command
    }
}

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#define DUMP_REMOTE_ACTIONS
#endif // DEVEL_RALF
#endif // DEBUG
GB_ERROR AW_root::check_for_remote_command(AW_default gb_maind, const char *rm_base) {
    GBDATA *gb_main = (GBDATA *)gb_maind;

    char *awar_action = GBS_global_string_copy("%s/action", rm_base);
    char *awar_value  = GBS_global_string_copy("%s/value", rm_base);
    char *awar_awar   = GBS_global_string_copy("%s/awar", rm_base);
    char *awar_result = GBS_global_string_copy("%s/result", rm_base);

    GB_push_transaction(gb_main);

    char *action   = GBT_readOrCreate_string(gb_main, awar_action, "");
    char *value    = GBT_readOrCreate_string(gb_main, awar_value, "");
    char *tmp_awar = GBT_readOrCreate_string(gb_main, awar_awar, "");
    
    if (tmp_awar[0]) {
        GB_ERROR error = 0;
        if (strcmp(action, "AWAR_REMOTE_READ") == 0) {
            char *read_value = this->awar(tmp_awar)->read_as_string();
            GBT_write_string(gb_main, awar_value, read_value);
#if defined(DUMP_REMOTE_ACTIONS)
            printf("remote command 'AWAR_REMOTE_READ' awar='%s' value='%s'\n", tmp_awar, read_value);
#endif // DUMP_REMOTE_ACTIONS
            free(read_value);
            // clear action (AWAR_REMOTE_READ is just a pseudo-action) :
            action[0] = 0;
            GBT_write_string(gb_main, awar_action, "");
        }
        else if (strcmp(action, "AWAR_REMOTE_TOUCH") == 0) {
            this->awar(tmp_awar)->touch();
#if defined(DUMP_REMOTE_ACTIONS)
            printf("remote command 'AWAR_REMOTE_TOUCH' awar='%s'\n", tmp_awar);
#endif // DUMP_REMOTE_ACTIONS
            // clear action (AWAR_REMOTE_TOUCH is just a pseudo-action) :
            action[0] = 0;
            GBT_write_string(gb_main, awar_action, "");
        }
        else {
#if defined(DUMP_REMOTE_ACTIONS)
            printf("remote command (write awar) awar='%s' value='%s'\n", tmp_awar, value);
#endif // DUMP_REMOTE_ACTIONS
            error = this->awar(tmp_awar)->write_as_string(value);
        }
        GBT_write_string(gb_main, awar_result, error ? error : "");
        GBT_write_string(gb_main, awar_awar, ""); // this works as READY-signal for perl-client (BIO::remote_awar and BIO:remote_read_awar)
    }
    GB_pop_transaction(gb_main);

    if (action[0]) {
        AW_cb_struct *cbs = (AW_cb_struct *)GBS_read_hash(prvt->action_hash, action);

#if defined(DUMP_REMOTE_ACTIONS)
        printf("remote command (%s) exists=%i\n", action, int(cbs != 0));
#endif                          // DUMP_REMOTE_ACTIONS
        if (cbs) {
            cbs->run_callback();
            GBT_write_string(gb_main, awar_result, "");
        } else {
            aw_message(GB_export_errorf("Unknown action '%s' in macro", action));
            GBT_write_string(gb_main, awar_result, GB_await_error());
        }
        GBT_write_string(gb_main, awar_action, ""); // this works as READY-signal for perl-client (remote_action)
    }
    
    free(tmp_awar);
    free(value);
    free(action);

    free(awar_result);
    free(awar_awar);
    free(awar_value);
    free(awar_action);
    
    return 0;
}

void AW_window::set_background(const char *colorname, Widget parentWidget) {
    bool colorSet = false;

    if (colorname) {
        XColor unused, color;

        if (XAllocNamedColor(p_global->display, p_global->colormap, colorname, &color, &unused)
                == 0) {
            fprintf(stderr,"XAllocColor failed: %s\n", colorname);
        } else {
            _at->background_color = color.pixel;
            colorSet = true;
        }
    }

    if (!colorSet) {
        XtVaGetValues(parentWidget, XmNbackground, &(_at->background_color),
                NULL); // fallback to background color
    }
}

void AW_window::TuneOrSetBackground(Widget w, const char *color, int modStrength) {
    // Sets the background for the next created widget.
    //
    // If 'color' is specified, it may contain one of the following values:
    //      "+"    means: slightly increase color of parent widget 'w' 
    //      "-"    means: slightly decrease color of parent widget 'w' 
    //      otherwise it contains a specific color ('name' or '#RGB')
    // 
    // If color is not specified, the color of the parent widget 'w' is modified
    // by 'modStrength' (increased if positive,  decreased if negative)
    // 
    // If it's not possible to modify the color (e.g. we cannot increase 'white'),
    // the color will be modified in the opposite direction. For details see TuneBackground()

    if (color) {
        switch (color[0]) {
        case '+':
            TuneBackground(w, TUNE_BRIGHT);
            break;
        case '-':
            TuneBackground(w, TUNE_DARK);
            break;
        default:
            set_background(color, w); // use explicit color
        }
    } else {
        TuneBackground(w, modStrength);
    }
}

void AW_window::TuneBackground(Widget w, int modStrength) {
    // Gets the Background Color, modifies the rgb values slightly and sets new background color
    // Intended to give buttons a nicer 3D-look.
    //
    // possible values for modStrength:
    //
    //    0        = do not modify (i.e. set to background color of parent widget)
    //    1 .. 127 = increase if background is bright, decrease if background is dark
    //   -1 ..-127 = opposite behavior than above
    //  256 .. 383 = always increase
    // -256 ..-383 = always decrease 
    //
    // if it's impossible to decrease or increase -> opposite direction is used.

    int col[3];
    {
        Pixel bg;
        XtVaGetValues(w, XmNbackground, &bg, NULL);

        XColor xc;
        xc.pixel = bg;
        XQueryColor(XtDisplay(w), p_global->colormap, &xc);

        col[0] = xc.red >> 8; // take MSB
        col[1] = xc.green >> 8;
        col[2] = xc.blue >> 8;
    }

    int mod = modStrength;
    int preferredDir = 0;
    bool invertedMod = false;

    if (modStrength>0) {
        if (modStrength>255) {
            mod -= 256;
            preferredDir = 1; // increase preferred
        }
    } else {
        if (modStrength<-255) {
            mod = -modStrength-256;
            preferredDir = -1; // decrease preferred
        } else {
            invertedMod = true;
            mod = -mod;
        }
    }

    aw_assert(mod >= 0 && mod < 128);
    // illegal modification

    bool incPossible[3]; // increment possible for color
    bool decPossible[3]; // decrement possible for color
    int incs = 0; // count possible increments
    int decs = 0; // count possible decrements

    for (int i = 0; i<3; ++i) {
        if ((incPossible[i] = ((col[i]+mod) <= 255)))
            incs++;
        if ((decPossible[i] = ((col[i]-mod) >= 0)))
            decs++;
    }

    aw_assert(incs||decs);

    switch (preferredDir) {
    case 0: // no direction preferred yet, need to decide
        if (invertedMod)
            preferredDir = decs ? -1 : 1;
        else
            preferredDir = incs ? 1 : -1;
        break;
    case 1:
        if (!incs)
            preferredDir = -1;
        break;
    case -1:
        if (!decs)
            preferredDir = 1;
        break;
    }

    aw_assert(preferredDir == 1 || preferredDir == -1); // no direction chosen above
    
    if (preferredDir == 1) {
        for (int i=0; i<3; ++i) col[i] += (incPossible[i] ? mod : 0);
    }
    else if (preferredDir == -1) {
        for (int i=0; i<3; ++i) col[i] -= (decPossible[i] ? mod : 0);
    }
    

    char hex_color[50];
    sprintf(hex_color, "#%2.2X%2.2X%2.2X", col[0], col[1], col[2]);
    aw_assert(strlen(hex_color) == 7);
    // otherwise some value overflowed
    set_background(hex_color, w);
}


