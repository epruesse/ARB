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
#include "aw_common_impl.hxx"
#include "aw_device_gtk.hxx"
#include "aw_root.hxx"
#include "aw_device_size.hxx"
#include "aw_device_click.hxx"
#include "aw_position.hxx"

#include <iostream>

#if defined(DEBUG)
// #define DUMP_EVENTS
#endif

#if defined(DUMP_EVENTS)
#  define DUMP_EVENT(type)                                              \
    printf("event %s: x=%4i y=%4i w=%4i h=%4i  "                        \
           "b=%i m=%i(%s%s%s) k=%i(%c) n=%zu\n",                        \
           type, aww->event.x, aww->event.y, aww->event.width,          \
           aww->event.height, aww->event.button,                        \
           aww->event.keymodifier,                                      \
           aww->event.keymodifier & AW_KEYMODE_SHIFT ? "S" : "",        \
           aww->event.keymodifier & AW_KEYMODE_CONTROL ? "C" : "",      \
           aww->event.keymodifier & AW_KEYMODE_ALT ? "A" : "",          \
           aww->event.keycode,                                          \
           aww->event.character, area->expose.size()                    \
           );                                       
    
#else
#  define DUMP_EVENT(type)
#endif


class AW_area_management::Pimpl : virtual Noncopyable {
public:
    GtkWidget          *widget; // the drawing area
    AW_window          *aww;    // parent window
    AW_area             area;   // which area (INFO, MIDDLE, BOTTOM)
    AW_common_gtk      *common; // holds GCs

    AW_device_gtk      *screen_device;
    AW_device_size     *size_device;
    AW_device_print    *print_device;
    AW_device_click    *click_device;

    Pimpl(AW_window* aww_, AW_area area_, GtkWidget* widget_) : 
        widget(widget_),
        aww(aww_),
        area(area_),
        common(new AW_common_gtk(widget_, aww_, area_)),
        screen_device(new AW_device_gtk(common, widget_)),
        size_device(NULL),
        print_device(NULL),
        click_device(NULL)
    {
    }
    ~Pimpl() {
        delete screen_device;
        delete common;
    }
};

AW_common *AW_area_management::get_common() const {
    return prvt->common; 
}

static void aw_event_clear(AW_window* aww) { // @@@ looks like an AW_event method
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

extern "C" gboolean aw_handle_configure_event(GtkWidget *, GdkEventConfigure *event, gpointer self) {
    AW_area_management* area = (AW_area_management*) self;
    AW_window*          aww  = area->prvt->aww;
    AW_dialog_disabled  now;

    aw_event_clear(aww);

    aww->event.type   = (AW_event_type) event->type;
    aww->event.x      = event->x;
    aww->event.y      = event->y;
    aww->event.width  = event->width;
    aww->event.height = event->height;

    area->prvt->common->set_screen_size(event->width, event->height);
    area->prvt->screen_device->set_cr(0);
    area->resize.emit();
    DUMP_EVENT("resize");
    return true; // event handled
}

extern "C" gboolean aw_handle_expose_event(GtkWidget *, GdkEventExpose *event, gpointer self)  {
    AW_area_management* area = (AW_area_management*) self;
    AW_window*          aww  = area->prvt->aww;
    AW_dialog_disabled  now;

    aw_event_clear(aww);

    aww->event.type   = (AW_event_type) event->type;
    aww->event.x      = event->area.x;
    aww->event.y      = event->area.y;
    aww->event.width  = event->area.width;
    aww->event.height = event->area.height;

    area->prvt->screen_device->set_cr(0);
    area->expose.emit();
    area->prvt->screen_device->set_cr(0);
    DUMP_EVENT("expose");
    return false;
}

extern "C" gboolean aw_handle_button_event(GtkWidget *, GdkEventButton *event, gpointer self) {
    AW_area_management* area = (AW_area_management*) self;
    AW_window*          aww  = area->prvt->aww;

    aw_event_clear(aww);
    aww->event.type        = (AW_event_type) event->type;
    aww->event.button      = (AW_MouseButton) event->button;
    aww->event.x           = event->x;
    aww->event.y           = event->y; 
    aww->event.keymodifier = (AW_key_mod) event->state;

    area->input.emit();
    DUMP_EVENT("input/button");
    return false;
}

extern "C" gboolean aw_handle_key_event(GtkWidget *, GdkEventKey *event, gpointer self) {
    AW_area_management* area = (AW_area_management*) self;
    AW_window*          aww  = area->prvt->aww;

    aw_event_clear(aww);

    unsigned modifiers = gtk_accelerator_get_default_mod_mask();
    // see https://developer.gnome.org/gtk2/2.24/checklist-modifiers.html
    aww->event.type        = (AW_event_type) event->type;    
    aww->event.keycode     = (AW_key_code) event->keyval;
    aww->event.keymodifier = (AW_key_mod) (event->state & modifiers); // &modifiers filters NumLock and CapsLock

    guint32 ucode = gdk_keyval_to_unicode(event->keyval);
    if (ucode>=' ' && ucode<=127) { // real ascii
        aww->event.character = ucode;
        aww->event.keycode   = AW_KEY_ASCII;

        if ((aww->event.keymodifier & GDK_CONTROL_MASK) && (ucode>='a' && ucode<='z')) {
            // Workaround for Motif reporting ctrl-a as 1, ctrl-b as 2, ...
            // FIXME: This should be fixed after the merge in client code
            // @@@ED4_edit_string.cxx:699
            aww->event.character -= 'a' - 1;
        }
    }
    else if (event->is_modifier) {
        // Workaround for Motif reporting modifier press/release as keycode == 0
        // Not doing so caused misbehavior decribed in http://bugs.arb-home.de/ticket/427#comment:4
        // @@@ should be fixed aftermerge (either not pass modifier events at all or fix all clients)
        aww->event.keycode = AW_KEY_NONE;
    }

    area->input.emit();
    DUMP_EVENT("input/key");
    return true;
}

extern "C" gboolean aw_handle_motion_event(GtkWidget *, GdkEventMotion *event, gpointer self) {
    AW_area_management* area = (AW_area_management*) self;
    AW_window*          aww  = area->prvt->aww;
    AW_dialog_disabled  now;

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
  
    area->expose.emit(); // work around for XOR drawing
    area->motion.emit();
    DUMP_EVENT("motion");
    // done processing, allow receiving next motion event:
    gdk_event_request_motions(event);
    return true;
}

extern "C" gboolean aw_handle_scroll_event(GtkWidget*, GdkEventScroll *event, gpointer self) {
    AW_area_management* area = (AW_area_management*) self;
    AW_window*          aww  = area->prvt->aww;
    AW_dialog_disabled  now;

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

    if (aww->event.button) {
        area->input.emit();
        DUMP_EVENT("input/scroll");
        return true;
    }
    return false;
}


void AW_area_management::set_expose_callback(AW_window *aww, const WindowCallback& wcb) {
    expose.connect(wcb, aww);
}

void AW_area_management::set_input_callback(AW_window *aww, const WindowCallback& wcb) {
    input.connect(wcb, aww);
}

void AW_area_management::set_motion_callback(AW_window *aww, const WindowCallback& wcb) {
    motion.connect(wcb, aww);
}

void AW_area_management::set_resize_callback(AW_window *aww, const WindowCallback& wcb) {
    resize.connect(wcb, aww);
}


static const char* AW_area_labels[] = {
    "info_area",
    "middle_area",
    "bottom_area"
};

AW_area_management::AW_area_management(AW_window* window, AW_area area, GtkWidget* widget) 
    : prvt(new AW_area_management::Pimpl(window, area, widget)) 
{
    aw_assert(area >= 0 && area < AW_MAX_AREA);
    
    // assign name to this area (useful for styling and debugging)
    gtk_widget_set_name(widget, AW_area_labels[area]);

    gtk_widget_add_events(prvt->widget, 
                          GDK_BUTTON_RELEASE_MASK | 
                          GDK_BUTTON_PRESS_MASK | 
                          GDK_KEY_PRESS_MASK | 
                          GDK_KEY_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK | 
                          GDK_POINTER_MOTION_HINT_MASK
                          );

    g_signal_connect(prvt->widget, "expose_event",
                     G_CALLBACK(aw_handle_expose_event), (gpointer) this);
    g_signal_connect(prvt->widget, "key_press_event", 
                     G_CALLBACK(aw_handle_key_event),    (gpointer) this);
    g_signal_connect(prvt->widget, "key_release_event", 
                     G_CALLBACK(aw_handle_key_event),    (gpointer) this);
    g_signal_connect(prvt->widget, "button-press-event",
                     G_CALLBACK(aw_handle_button_event), (gpointer) this);
    g_signal_connect(prvt->widget, "button-release-event",
                     G_CALLBACK(aw_handle_button_event), (gpointer) this);
    g_signal_connect(prvt->widget, "scroll-event",
                     G_CALLBACK(aw_handle_scroll_event), (gpointer) this);
    g_signal_connect(prvt->widget, "motion-notify-event", 
                     G_CALLBACK(aw_handle_motion_event), (gpointer) this);
    g_signal_connect(prvt->widget, "configure-event",
                     G_CALLBACK(aw_handle_configure_event), (gpointer) this);
}

AW_area_management::~AW_area_management() {
    delete prvt;
    // fixme, disconnect signals
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
