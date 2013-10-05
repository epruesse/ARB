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

//#define DUMP_EVENTS

#if defined(DUMP_EVENTS)
#  define DUMP_EVENT(type)                                              \
    printf("event %s: x=%4i y=%4i w=%4i h=%4i  "                        \
           "b=%i m=%i(%s%s%s) k=%i(%c)\n", \
           type, aww->event.x, aww->event.y, aww->event.width,          \
           aww->event.height, aww->event.button,                        \
           aww->event.keymodifier,                                      \
           aww->event.keymodifier & AW_KEYMODE_SHIFT ? "S" : "",        \
           aww->event.keymodifier & AW_KEYMODE_CONTROL ? "C" : "",      \
           aww->event.keymodifier & AW_KEYMODE_ALT ? "A" : "",          \
           aww->event.keycode,                                          \
           aww->event.character);                                       
    
#else
#  define DUMP_EVENT(type)
#endif // DEBUG

static void aw_event_clear(AW_window* aww) {
    aww->event.type        = AW_No_Event;
    aww->event.button      = AW_BUTTON_NONE;
    aww->event.x           = -1;
    aww->event.y           = -1;
    aww->event.width       = -1;
    aww->event.height      = -1;
    aww->event.keymodifier = AW_KEYMODE_NONE;
    aww->event.keycode     = AW_KEY_NONE;
    aww->event.character   = '\0';
}

class AW_area_management::Pimpl {
public:
    AW_area_management *self;
    GtkWidget *area; /** < the drawing area */
    AW_window *aww; /* parent window */
    AW_common_gtk *common;

    AW_device_gtk   *screen_device;
    AW_device_size  *size_device;
    AW_device_print *print_device;
    AW_device_click *click_device;

    Pimpl(AW_area_management* self_, GtkWidget* widget, AW_window* window) : 
        self(self_),
        area(widget),
        aww(window),
        common(NULL),
        screen_device(NULL),
        size_device(NULL),
        print_device(NULL),
        click_device(NULL)
    {}

    gboolean handle_event(GdkEventConfigure*);
    gboolean handle_event(GdkEventExpose*);
    gboolean handle_event(GdkEventButton*);
    gboolean handle_event(GdkEventKey*);
    gboolean handle_event(GdkEventMotion*);
    gboolean handle_event(GdkEventScroll*);
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
extern "C"  gboolean scroll_event_cbproxy(GtkWidget*, GdkEventScroll *ev, gpointer self) {
    return ((AW_area_management::Pimpl*)self)->handle_event(ev);
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventExpose* event) {
    aw_event_clear(aww);
    aww->event.type        = (AW_event_type) event->type;
    aww->event.x           = event->area.x;
    aww->event.y           = event->area.y;
    aww->event.width       = event->area.width;
    aww->event.height      = event->area.height;

    DUMP_EVENT("expose");
    self->expose.emit();
    return false;
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventConfigure* event) {
    aw_event_clear(aww);
    aww->event.type   = (AW_event_type) event->type;
    aww->event.x      = event->x;
    aww->event.y      = event->y;
    aww->event.width  = event->width;
    aww->event.height = event->height;

    DUMP_EVENT("resize");
    self->resize.emit();
    return true; // event handled
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventButton *event) {
    aw_event_clear(aww);
    aww->event.type        = (AW_event_type) event->type;    
    aww->event.button      = (AW_MouseButton) event->button;
    aww->event.x           = event->x;
    aww->event.y           = event->y; 
    aww->event.keymodifier = (AW_key_mod) event->state;

    DUMP_EVENT("input/button");
    self->input.emit();
    return false;
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventScroll *event) {
    aw_event_clear(aww);
    aww->event.type        = AW_Mouse_Press;
    aww->event.x           = event->x;
    aww->event.y           = event->y;
    aww->event.keymodifier = (AW_key_mod) event->state;

    if (event->direction == GDK_SCROLL_UP) {
        aww->event.button = AW_WHEEL_UP;
    }
    else if (event->direction == GDK_SCROLL_DOWN) {
        aww->event.button = AW_WHEEL_DOWN;
    }

    DUMP_EVENT("input/scroll");
    if (aww->event.button) {
        self->input.emit();
        return true;
    }
    return false;
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventKey *event) {
    aw_event_clear(aww);
    unsigned modifiers = gtk_accelerator_get_default_mod_mask (); //see https://developer.gnome.org/gtk2/2.24/checklist-modifiers.html
    aww->event.type        = (AW_event_type) event->type;    
    aww->event.keycode     = (AW_key_code) event->keyval;
    aww->event.keymodifier = (AW_key_mod) (event->state & modifiers); // &modifiers filters NumLock and CapsLock

    gchar* str = gdk_keyval_name(event->keyval);
    if (strlen(str) == 1) {
        aww->event.character = str[0];
        aww->event.keycode = AW_KEY_ASCII;
    }
 
    DUMP_EVENT("input/key");
    self->input.emit();
    return true;
}

gboolean AW_area_management::Pimpl::handle_event(GdkEventMotion *event) {
    aw_event_clear(aww);
    aww->event.type        = (AW_event_type) event->type;
    aww->event.x           = event->x;
    aww->event.y           = event->y;
    aww->event.keymodifier = (AW_key_mod) event->state;

    if (event->state & GDK_BUTTON1_MASK) {
        aww->event.button = AW_BUTTON_LEFT;
    } else if (event->state & GDK_BUTTON2_MASK) {
        aww->event.button = AW_BUTTON_MIDDLE;
    } else if (event->state & GDK_BUTTON3_MASK) {
        aww->event.button = AW_BUTTON_RIGHT;
    } else {
        return false;
    }
  
    DUMP_EVENT("motion");
    self->expose.emit(); // work around for XOR drawing
    self->motion.emit();

    // done processing, allow receiving next motion event:
    gdk_event_request_motions(event);
    return true;
}


void AW_area_management::set_expose_callback(AW_window *aww, 
                                             void (*f)(AW_window*, AW_CL, AW_CL),
                                             AW_CL cd1, AW_CL cd2) {
    expose.connect(makeWindowCallback(f, cd1, cd2), aww);
}

void AW_area_management::set_input_callback(AW_window *aww, const WindowCallback& wcb) {
    input.connect(wcb, aww);
}

void AW_area_management::set_motion_callback(AW_window *aww, const WindowCallback& wcb) {
    motion.connect(wcb, aww);
}

void AW_area_management::set_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), 
                                             AW_CL cd1, AW_CL cd2) {
    resize.connect(makeWindowCallback(f, cd1, cd2), aww);
}


AW_area_management::AW_area_management(GtkWidget* area, AW_window* window) 
    : prvt(new AW_area_management::Pimpl(this, area, window)) 
{
    g_signal_connect(prvt->area, "expose_event",
                      G_CALLBACK (expose_event_cbproxy), (gpointer) prvt);
    gtk_widget_add_events(prvt->area, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | 
                          GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    g_signal_connect(prvt->area, "key_press_event", 
                     G_CALLBACK (key_event_cbproxy), (gpointer) prvt);
    g_signal_connect(prvt->area, "key_release_event", 
                     G_CALLBACK (key_event_cbproxy), (gpointer) prvt);
    g_signal_connect(prvt->area, "button-press-event",
                     G_CALLBACK (button_event_cbproxy), (gpointer) prvt);
    g_signal_connect(prvt->area, "button-release-event",
                     G_CALLBACK (button_event_cbproxy), (gpointer) prvt);
    g_signal_connect(prvt->area, "scroll-event",
                     G_CALLBACK (scroll_event_cbproxy), (gpointer) prvt);
    gtk_widget_add_events(prvt->area, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
    g_signal_connect(prvt->area, "motion-notify-event", 
                     G_CALLBACK (motion_event_cbproxy), (gpointer) prvt);
    g_signal_connect(prvt->area, "configure-event",
                     G_CALLBACK (configure_event_cbproxy), (gpointer) prvt);
}

AW_area_management::~AW_area_management() {
    delete prvt;
    // fixme, disconnect signals
}

void AW_area_management::create_devices(AW_window *aww, AW_area area) {
    prvt->common = new AW_common_gtk(prvt->area, aww, area);
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

AW_device_print *AW_area_management::get_print_device() {
    if (!prvt->print_device) prvt->print_device = new AW_device_print(prvt->common);
    return prvt->print_device;
}
