// ============================================================= //
//                                                               //
//   File      : AW_area_management.cxx                          //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 13, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_window.hxx"
#include "aw_area_management.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "gdk/gdk.h"
#include "gtk/gtk.h"
#include "aw_common_gtk.hxx"
#include "aw_device_gtk.hxx"
#include "aw_root.hxx"
#include "aw_device_size.hxx"
#include "aw_device_click.hxx"
#include "aw_position.hxx"

#include <iostream>



#if defined(DEBUG)
// #define DUMP_KEYEVENTS
#endif // DEBUG


class AW_area_management::Pimpl {
public:
    GtkWidget *area; /** < the drawing area */
    AW_window *aww; /* parent window */
    AW_common_gtk *common;

    AW_device_gtk   *screen_device;
    AW_device_size  *size_device;
    AW_device_print *print_device;
    AW_device_click *click_device;
    long click_time;

    AW_cb_struct *resize_cb; /** < A list of callback functions that are called whenever this area is resized. */
    AW_cb_struct *expose_cb;
    AW_cb_struct *input_cb;
    AW_cb_struct *motion_cb;
    
    /**used to simulate resize events*/
    int old_width;
    int old_height;
    
    Pimpl(GtkWidget* widget, AW_window* window) : 
        area(widget),
        aww(window),
        common(NULL),
        screen_device(NULL),
        size_device(NULL),
        print_device(NULL),
        click_device(NULL),
        click_time(0),
        resize_cb(NULL),
        expose_cb(NULL),
        input_cb(NULL),
        motion_cb(NULL),
        old_width(-1),
        old_height(-1){}   

    gboolean handle_event(GdkEventConfigure*);
    gboolean handle_event(GdkEventExpose*);
    gboolean handle_event(GdkEventButton*);
    gboolean handle_event(GdkEventKey*);
    gboolean handle_event(GdkEventMotion*);
};

GtkWidget *AW_area_management::get_area() const {
    return GTK_WIDGET(prvt->area);
}
AW_common *AW_area_management::get_common() const {
    return prvt->common; 
}

extern "C"  gboolean configure_event_cbproxy(GtkWidget *, GdkEventConfigure *ev, gpointer self) {
    return ((AW_area_management::Pimpl*)self)->handle_event(ev);
}
extern "C"  gboolean expose_event_cbproxy(GtkWidget *, GdkEventExpose *ev, gpointer self) {
    return ((AW_area_management::Pimpl*)self)->handle_event(ev);
}
extern "C"  gboolean button_event_cbproxy(GtkWidget *, GdkEventButton *ev, gpointer self) {
    return ((AW_area_management::Pimpl*)self)->handle_event(ev);
}
extern "C"  gboolean key_event_cbproxy(GtkWidget *, GdkEventKey *ev, gpointer self) {
    return ((AW_area_management::Pimpl*)self)->handle_event(ev);
}
extern "C"  gboolean motion_event_cbproxy(GtkWidget *, GdkEventMotion *ev, gpointer self) {
    return ((AW_area_management::Pimpl*)self)->handle_event(ev);
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventExpose*) {
    expose_cb->run_callback();
    return false;
}
gboolean AW_area_management::Pimpl::handle_event(GdkEventConfigure*) {
    resize_cb->run_callback();
    return false;
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventButton *event) {
    aww->event.type        = (AW_event_type) event->type;    
    aww->event.button      = (AW_MouseButton) event->button;
    aww->event.x           = event->x;
    aww->event.y           = event->y; 
    aww->event.keymodifier = (AW_key_mod) event->state;
    aww->event.keycode     = AW_KEY_NONE;
    aww->event.character   = '\0';

    input_cb->run_callback();
    return false;
}
gboolean AW_area_management::Pimpl::handle_event(GdkEventKey *event) {
    aww->event.type        = (AW_event_type) event->type;    
    aww->event.button      = AW_BUTTON_NONE;
    aww->event.x           = 0; 
    aww->event.y           = 0; 
    aww->event.keycode     = (AW_key_code) event->keyval;
    aww->event.keymodifier = (AW_key_mod) event->state;      
    aww->event.character   = '\0';
  
    gchar* str = gdk_keyval_name(event->keyval);
    if (strlen(str) == 1) {
        aww->event.character = str[0];
        aww->event.keycode = AW_KEY_ASCII;
    }
 
    input_cb->run_callback();
    return true;
}
gboolean AW_area_management::Pimpl::handle_event(GdkEventMotion *event) {
    int x, y;
    GdkModifierType state;
    gdk_window_get_pointer(event->window, &x, &y, &state);
    
    if (state & GDK_BUTTON1_MASK) {
        /*
          aww->event.type = AW_Mouse_Drag;
          aww->event.x = x;
          aww->event.y = y;
          aww->event.keycode = AW_KEY_NONE;
        */
            
        motion_cb->run_callback();
    }
    return false;
}


void AW_area_management::set_expose_callback(AW_window *aww, 
                                             void (*f)(AW_window*, AW_CL, AW_CL),
                                             AW_CL cd1, AW_CL cd2) 
{
    if (!prvt->expose_cb) { 
        g_signal_connect (prvt->area, "expose_event",
                          G_CALLBACK (expose_event_cbproxy), (gpointer) prvt);

    }
    prvt->expose_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->expose_cb);
}

void AW_area_management::set_input_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), 
                                            AW_CL cd1, AW_CL cd2) 
{
    if(!prvt->input_cb) {
        // enable button press and release events
        gtk_widget_add_events(prvt->area, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | 
                               GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
        
        g_signal_connect (prvt->area, "key_press_event", 
                          G_CALLBACK (key_event_cbproxy), (gpointer) prvt);
        g_signal_connect (prvt->area, "key_release_event", 
                          G_CALLBACK (key_event_cbproxy), (gpointer) prvt);
        g_signal_connect (prvt->area, "button-press-event",
                          G_CALLBACK (button_event_cbproxy), (gpointer) prvt);
        g_signal_connect (prvt->area, "button-release-event",
                          G_CALLBACK (button_event_cbproxy), (gpointer) prvt);
    }
    prvt->input_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->input_cb);
}

void AW_area_management::set_motion_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), 
                                             AW_CL cd1, AW_CL cd2) 
{
    if (!prvt->motion_cb) {
        gtk_widget_add_events(prvt->area, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
        g_signal_connect(prvt->area, "motion-notify-event", 
                         G_CALLBACK (motion_event_cbproxy), (gpointer) prvt);
    }
    prvt->motion_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->motion_cb);
}

void AW_area_management::set_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), 
                                             AW_CL cd1, AW_CL cd2) 
{
    if (!prvt->resize_cb) {
        g_signal_connect (prvt->area, "configure-event",
                          G_CALLBACK (configure_event_cbproxy), (gpointer) prvt);

    }
    prvt->resize_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->resize_cb);
}



AW_area_management::AW_area_management(GtkWidget* area, AW_window* window) 
    : prvt(new AW_area_management::Pimpl(area, window))
{
}
AW_area_management::~AW_area_management() {
  delete prvt;
}

bool AW_area_management::is_input_callback(AW_window* /*aww*/,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    return prvt->input_cb && prvt->input_cb->contains(f);
}

bool AW_area_management::is_motion_callback(AW_window* /*aww*/,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    return prvt->motion_cb && prvt->motion_cb->contains(f);
}

bool AW_area_management::is_expose_callback(AW_window */*aww*/,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
     return prvt->expose_cb && prvt->expose_cb->contains(f);
}

bool AW_area_management::is_resize_callback(AW_window * /* aww */, void (*f)(AW_window*, AW_CL, AW_CL)) {
    return prvt->resize_cb && prvt->resize_cb->contains(f);
}

void AW_area_management::create_devices(AW_window *aww, AW_area ar) {
    AW_root *root = aww->get_root();
    GdkScreen* screen = gtk_widget_get_screen (prvt->area);
    GdkDisplay* pDisplay = gdk_screen_get_display(screen);
    FIXME("parameter global colortable is wrong.");
    prvt->common = new AW_common_gtk(pDisplay, prvt->area, root->getColorTable(), aww->color_table, aww->color_table_size, aww, ar);
    prvt->screen_device = new AW_device_gtk(prvt->common, prvt->area);

}

AW_device_gtk *AW_area_management::get_screen_device() {
    return prvt->screen_device;
}

AW_device_size *AW_area_management::get_size_device() {
    if (!prvt->size_device) prvt->size_device = new AW_device_size(prvt->common);
    return prvt->size_device;
}

AW_device_click *AW_area_management::get_click_device() {
    if (!prvt->click_device) prvt->click_device = new AW_device_click(prvt->common);
    return prvt->click_device;
}
