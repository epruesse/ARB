// ============================================================= //
//                                                               //
//   File      : aw_window_gtk.hxx                                    //
//   Purpose   : Contains gtk dependent helper classes for aw_window //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 6, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#pragma once

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include "aw_base.hxx"

/*
 * Contains all gtk dependent attributs from aw_window.
 */
class AW_window_gtk : public Noncopyable {
    friend class AW_window;

    GtkWidget *window; /*< The gtk window instance managed by this aw_window */
    AW_area_management *areas[AW_MAX_AREA]; /*< Managers for the areas that make up this window */

    AW_window_gtk() :
        window(NULL) {}

    ~AW_window_gtk() {
        destroy(window);
    }

    /*
     * Destroys the specified widget.
     */
    void destroy(GtkWidget *widget) {
        if(NULL != widget) {
            gtk_widget_destroy(widget);
            widget = NULL;
        }
    }
};

/**
 * FIXME
 */
class AW_area_management {
    GtkWidget *form; //FIXME what is this?
    GtkWidget *area;

    AW_common_gtk *common;

    AW_device_gtk   *device;
    AW_device_size  *size_device;
    AW_device_print *print_device;
    AW_device_click *click_device;

    AW_cb_struct *expose_cb;
    AW_cb_struct *resize_cb;
    AW_cb_struct *double_click_cb;

    long click_time;

public:
    AW_area_management(AW_root *awr, GtkWidget *form, GtkWidget *widget);

    Widget get_form() const { return form; }
    Widget get_area() const { return area; }

    AW_common_gtk *get_common() const { return common; }

    AW_device_gtk *get_screen_device();
    AW_device_size *get_size_device();
    AW_device_print *get_print_device();
    AW_device_click *get_click_device();

    void create_devices(AW_window *aww, AW_area ar);

    void set_expose_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_input_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_double_click_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);
    void set_motion_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2);

    bool is_expose_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_input_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_double_click_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));
    bool is_motion_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL));

    void run_expose_callback();
    void run_resize_callback();

    AW_cb_struct *get_double_click_cb() { return double_click_cb; }
    long get_click_time() const { return click_time; }
    void set_click_time(long click_time_) { click_time = click_time_; }
};


