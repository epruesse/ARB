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

#include "AW_area_management.hxx"
#include "aw_window.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "gdk/gdkscreen.h"
#include "gtk/gtkwidget.h"
#include "aw_common_gtk.hxx"
#include "aw_device_gtk.hxx"
#include "aw_root.hxx"
#include "aw_xkey.hxx"

class AW_area_management::Pimpl {
public:
    GtkWidget *form; /**< the managing widget */
    GtkWidget *area; /**< the drawing area */

    AW_common_gtk *common;

    AW_device_gtk   *screen_device;
    AW_device_size  *size_device;
    AW_device_print *print_device;
    AW_device_click *click_device;
    long click_time;

    AW_cb_struct *resize_cb; /**<A list of callback functions that are called whenever this area is resized. */
    AW_cb_struct *double_click_cb;
    AW_cb_struct *expose_cb;
    AW_cb_struct *input_cb;
    
    Pimpl() : 
        form(NULL),
        area(NULL),
        common(NULL),
        screen_device(NULL),
        size_device(NULL),
        print_device(NULL),
        click_device(NULL),
        click_time(0),
        resize_cb(NULL),
        double_click_cb(NULL),
        expose_cb(NULL),
        input_cb(NULL){}   
};


GtkWidget *AW_area_management::get_form() const {
    return GTK_WIDGET(prvt->form);
}
GtkWidget *AW_area_management::get_area() const {
    return GTK_WIDGET(prvt->area);
}
AW_common_gtk *AW_area_management::get_common() const {
    return prvt->common; 
}


/**
 * Handles the drawing areas expose callback.
 */
static gboolean draw_area_expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer area_management)
{
    AW_area_management *aram = (AW_area_management *) area_management;
    aram->run_resize_callback();
    aram->run_expose_callback();
}

/**
 * Handles the key press event
 */
static gboolean input_event_cb(GtkWidget *widget, GdkEvent *event, gpointer cb_struct)
{
    GTK_PARTLY_IMPLEMENTED;
    GdkEventType type = event->type;
    bool run_callback  = false;
    bool run_double_click_callback = false;
    AW_cb_struct *cbs = (AW_cb_struct*) cb_struct;
    AW_window *aww = cbs->aw;
    AW_area_management *area = NULL;
    
    for (int i=0; i<AW_MAX_AREA; i++) {
        if(aww->get_area(i) && aww->get_area(i)->get_area() == widget) {
            area = aww->get_area(i);
            break;
        }
    }
    aw_assert(NULL != area);
    if(GDK_BUTTON_PRESS == type || GDK_BUTTON_RELEASE == type) {
        aww->event.button      = AW_MouseButton(event->button.button);
        aww->event.x           = event->button.x;
        aww->event.y           = event->button.y; //FIXME keymodifier might not be correct due to missmatched enum values
        aww->event.keymodifier = (AW_key_mod)(event->button.state & (AW_KEYMODE_SHIFT|AW_KEYMODE_CONTROL|AW_KEYMODE_ALT));
        aww->event.keycode     = AW_KEY_NONE;
        aww->event.character   = '\0';

        if (GDK_BUTTON_PRESS == type) {
            aww->event.type = AW_Mouse_Press;
            if (area && area->get_double_click_cb()) {
                if ((event->button.time - area->get_click_time()) < 200) {
                    run_double_click_callback = true;
                }
                else {
                    run_callback = true;
                }
                area->set_click_time(event->button.time);
            }
            else {
                run_callback = true;
            }
            
            aww->event.time = event->button.time;
        }
        else if (GDK_BUTTON_RELEASE == type) {
            aww->event.type = AW_Mouse_Release;
            run_callback = true;
            // keep event.time set in ButtonPress-branch
        }
    }
    else if (GDK_KEY_PRESS == type || GDK_KEY_RELEASE == type) {
        aww->event.time = event->key.time;
        
        
        const awKeymap mykey = gtk_key_2_awkey(event->key);

        aww->event.keycode = mykey.key;
        aww->event.keymodifier = mykey.modifier;

        if (!mykey.str.empty()) {
            aww->event.character = mykey.str[0];
        }
        else {
            aww->event.character = 0;
        }

        if (GDK_KEY_PRESS == type) {
            aww->event.type = AW_Keyboard_Press;
        }
        else {
            aww->event.type = AW_Keyboard_Release;
        }
        aww->event.button = AW_BUTTON_NONE;
        aww->event.x = 0; //FIXME get mouse cursor location
        aww->event.y = 0; //FIXME get mouse cursor location
        
        //FIXME key events modes_f_callbacks not implemented
//        if (!mykey.modifier && mykey.key >= AW_KEY_F1 && mykey.key
//                <= AW_KEY_F12 && p_aww(aww)->modes_f_callbacks && p_aww(aww)->modes_f_callbacks[mykey->awkey-AW_KEY_F1]
//                && aww->event.type == AW_Keyboard_Press) {
//            p_aww(aww)->modes_f_callbacks[mykey->awkey-AW_KEY_F1]->run_callback();
//        }
//        else {
//            run_callback = true;
//        }
    }

    if (run_double_click_callback) {
        if (cbs->help_text == (char*)1) {
            cbs->run_callback();
        }
        else {
            if (area)
                area->get_double_click_cb()->run_callback();
        }
    }

    if (run_callback && (cbs->help_text == (char*)0)) {
        cbs->run_callback();
    }
}




void AW_area_management::run_expose_callback() {
    if (prvt->expose_cb) prvt->expose_cb->run_callback();
}

void AW_area_management::set_expose_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {

    //copy and paste code from set_resize callback. resize and expose use the same callback.
    
    if (!prvt->resize_cb) {//connect the gtk signal upon first call

        //note: we use the expose callback because there is no resize callback in gtk.
        g_signal_connect (prvt->area, "expose_event",
                          G_CALLBACK (draw_area_expose_cb), (gpointer) this);

    }
    prvt->expose_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->expose_cb);
}

void AW_area_management::set_input_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    
    if(!prvt->input_cb) {
        
        //enabled button press and release events
        gtk_widget_add_events(prvt->area, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | 
                               GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
        
        AW_cb_struct* param = new AW_cb_struct(aww, f, cd1, cd2, (char*)0);
        //all events are handled by the same event handler
        g_signal_connect (prvt->area, "key_press_event", G_CALLBACK (input_event_cb), (gpointer) param);
        g_signal_connect (prvt->area, "key_release_event", G_CALLBACK (input_event_cb), (gpointer) param);
        g_signal_connect (prvt->area, "button-press-event", G_CALLBACK (input_event_cb), (gpointer) param);
        g_signal_connect (prvt->area, "button-release-event", G_CALLBACK (input_event_cb), (gpointer) param);

    }
    
    prvt->input_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->input_cb);
}

void AW_area_management::set_motion_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED;
}

void AW_area_management::set_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    // insert resize callback for draw_area

    //FIXME do this when handling the expose_cb.
    if (!prvt->resize_cb) {//connect the gtk signal upon first call

        //note: we use the expose callback because there is no resize callback in gtk.
        g_signal_connect (prvt->area, "expose_event",
                          G_CALLBACK (draw_area_expose_cb), (gpointer) this);

    }
    prvt->resize_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->resize_cb);
}


inline void AW_area_management::run_resize_callback() {
    if (prvt->resize_cb) prvt->resize_cb->run_callback();
}

AW_area_management::AW_area_management(AW_root* awr, GtkWidget* form,
        GtkWidget* area) {
    
    prvt = new AW_area_management::Pimpl();
    prvt->form = form;
    prvt->area = area;
    GTK_PARTLY_IMPLEMENTED;
    
}

bool AW_area_management::is_input_callback(AW_window* aww,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_area_management::is_double_click_callback(AW_window* aww,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    return prvt->double_click_cb && prvt->double_click_cb->contains(f);
}

bool AW_area_management::is_motion_callback(AW_window* aww,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    GTK_NOT_IMPLEMENTED;
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
    //FIXME font stuff
    //FIXME parameter global colortable is wrong.
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


AW_cb_struct *AW_area_management::get_double_click_cb() { return prvt->double_click_cb; }
long AW_area_management::get_click_time() const { return prvt->click_time; }
void AW_area_management::set_click_time(long click_time) { prvt->click_time = click_time; }

AW_device_click *AW_area_management::get_click_device() {
    if (!prvt->click_device) prvt->click_device = new AW_device_click(prvt->common);
    return prvt->click_device;
}