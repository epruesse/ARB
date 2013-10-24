/* 
 * File:   AW_area_management.hxx
 * Author: aboeckma
 *
 * Created on October 10, 2012, 12:06 PM
 */

#pragma once

#include "aw_base.hxx"
#include "aw_gtk_forward_declarations.hxx"
#include "aw_keysym.hxx"
#include "aw_signal.hxx"
#include <gdk/gdk.h>

class AW_window;
class AW_root;
class AW_common_gtk;
class AW_device_gtk;
class AW_device_size;
class AW_device_print;
class AW_device_click;
class AW_common;

enum AW_key_mod {
    AW_KEYMODE_NONE    = 0,
    AW_KEYMODE_SHIFT   = GDK_SHIFT_MASK,
    AW_KEYMODE_CONTROL = GDK_CONTROL_MASK,
    AW_KEYMODE_ALT     = GDK_MOD1_MASK,
};

enum AW_event_type {
    AW_No_Event         = GDK_NOTHING,
    AW_Keyboard_Press   = GDK_KEY_PRESS,
    AW_Keyboard_Release = GDK_KEY_RELEASE,
    AW_Mouse_Press      = GDK_BUTTON_PRESS,
    AW_Mouse_Release    = GDK_BUTTON_RELEASE,
    AW_Mouse_Drag       = GDK_MOTION_NOTIFY,
    
    NO_EVENT = AW_No_Event,
    KEY_PRESSED  = AW_Keyboard_Press,
    KEY_RELEASED = AW_Keyboard_Release,
};

typedef AW_event_type AW_ProcessEventType;

enum AW_MouseButton {
    AW_BUTTON_NONE   = 0,
    AW_BUTTON_LEFT   = 1,
    AW_BUTTON_MIDDLE = 2,
    AW_BUTTON_RIGHT  = 3,
    AW_WHEEL_UP      = 4,
    AW_WHEEL_DOWN    = 5,
};


struct AW_event {
    // fields always valid
    AW_event_type type;             // AW_Keyboard or AW_Mouse
    AW_key_mod    keymodifier;      // control, alt/meta (shift only for type == AW_Mouse)

    // fields valid for type == AW_Mouse
    AW_MouseButton button;
    int            x, y, width, height;    // pointer x,y coordinates in the event window

    // fields valid for type == AW_Keyboard
    AW_key_code keycode;            // which key type was pressed
    char        character;          // the c character
};





/**
 * Contains information about one area inside a window.
 */
class AW_area_management : virtual Noncopyable {
public:
    class Pimpl;
    Pimpl* prvt; /* < Contains all private attributes and gtk dependencies */  

    AW_signal resize; //! issued when the area size changes
    AW_signal expose; //! issued when parts of the area need to be redrawn
    AW_signal input;  //! issued when mouse button or key presses happened
    AW_signal motion; //! issued when the mouse is moved with a button depressed

    AW_area_management(AW_window*, AW_area, GtkWidget*);
    ~AW_area_management();

    GtkWidget *get_area() const;
    AW_common *get_common() const;

    AW_device_gtk *get_screen_device();
    AW_device_size *get_size_device();
    AW_device_print *get_print_device();
    AW_device_click *get_click_device();

    /**
     * Adds a new callback.
     * @param wcb The callback.
     * @param aww TODO
     */
    void set_expose_callback(AW_window *aww, const WindowCallback& wcb);
    void set_resize_callback(AW_window *aww, const WindowCallback& wcb);
    void set_input_callback(AW_window *aww, const WindowCallback& wcb);
    void set_motion_callback(AW_window *aww, const WindowCallback& wcb);
};


