/* 
 * File:   AW_area_management.hxx
 * Author: aboeckma
 *
 * Created on October 10, 2012, 12:06 PM
 */

#pragma once

#include "aw_base.hxx"
#include "AW_gtk_forward_declarations.hxx"
class AW_window;
class AW_root;
class AW_common_gtk;
class AW_device_gtk;
class AW_device_size;
class AW_device_print;
class AW_device_click;
class AW_cb_struct;
/**
 * Contains information about one area inside a window.
 */
class AW_area_management {
    
    class Pimpl;
    Pimpl* prvt; /* < Contains all private attributes and gtk dependencies */  
    
public:
    AW_area_management(AW_root *awr, GtkWidget *form, GtkWidget *area);

    GtkWidget *get_form() const;
    GtkWidget *get_area() const;
    AW_common_gtk *get_common() const;

    AW_device_gtk *get_screen_device();
    AW_device_size *get_size_device();
    AW_device_print *get_print_device();
    AW_device_click *get_click_device();

    void create_devices(AW_window *aww, AW_area ar);

    /**
     * Adds a new callback.
     * @param f The callback.
     * @param aww TODO
     * @param cd1 callback parameter 1
     * @param cd2 callback parameter 2
     */
    void set_expose_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_input_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_double_click_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_motion_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);


    bool is_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_input_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_double_click_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_motion_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_expose_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));

    void run_resize_callback();
    void run_expose_callback();

    AW_cb_struct *get_double_click_cb();
    long get_click_time() const;
    void set_click_time(long click_time);
};


