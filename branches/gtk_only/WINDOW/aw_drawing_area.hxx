/* 
 * File:   aw_drawing_area.hxx
 * Author: Arne Boeckmann
 * Created on March 20, 2013, 10:43 AM
 * 
 * This file defines the aw_drawing_area.
 * 
 * The aw_drawing_area is a simple extension of the gtk_drawing_area.
 * It adds scrolling functionality to the drawing area without the need
 * to resize the drawing area.
 * 
 * This area is always just as big as the visible part of the drawing.
 * It simulates a bigger size for correct scrollbar appearance.
 * 
 * The area emits a signal every time the scrolled position changes.
 * This
 *  
 */

#pragma once
#include <gtk/gtk.h>

#include "aw_gtk_forward_declarations.hxx"

class AW_window;

//gtk boilerplate 
G_BEGIN_DECLS

#define AW_DRAWING_AREA_TYPE (aw_drawing_area_get_type())
#define AW_DRAWING_AREA(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), AW_DRAWING_AREA_TYPE, AW_drawing_area))
#define AW_DRAWING_AREA_CLASS(clazz) (G_TYPE_CHECK_CLASS_CAST(clazz), AW_DRAWING_AREA_TYPE, AW_drawing_area_class)
#define IS_AW_DRAWING_AREA(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), AW_drawing_area))
#define IS_AW_DRAWING_AREA_CLASS (clazz) (G_TYPE_CHECK_CLASS_TYPE((clazz), AW_drawing_area))

typedef struct _AW_drawing_area AW_drawing_area;
typedef struct _AW_drawing_area_class AW_drawing_area_class;
typedef struct _AW_drawing_area_private AW_drawing_area_private;

struct _AW_drawing_area
{
    GtkDrawingArea parent; /**< Parent instance has to be the first attribute */
    AW_drawing_area_private *prvt; /**< Internal attributes */
};

struct _AW_drawing_area_class
{
    GtkDrawingAreaClass parent_class; /**< Parent class has to be first attribute */
    
    /**Is called whenever the value of one of the scrollbars changes */
    void (*scroll_value_changed) (AW_drawing_area *da);
    
    void (*set_scroll_adjustments) (AW_drawing_area *da, GtkAdjustment *hadjustment,
                                    GtkAdjustment *vadjustment);
    
};
    
GType aw_drawing_area_get_type() G_GNUC_CONST;

GtkWidget *aw_drawing_area_new();

/**Sets the size of the complete picture and the currently visible area.*/
void aw_drawing_area_set_picture_size(AW_drawing_area *area, gint picture_width,
                                      gint picture_height, gint visible_width,
                                      gint visible_height);
/** set the step_increment and page_increment values for the scrollbars*/
void aw_drawing_area_set_increments(AW_drawing_area *area,
                                    gint horizontal_step_increment,
                                    gint horizontal_page_increment,
                                    gint vertical_step_increment,
                                    gint vertical_page_increment);

GtkAdjustment* aw_drawing_area_get_vertical_adjustment(AW_drawing_area *area);
GtkAdjustment* aw_drawing_area_get_horizontal_adjustment(AW_drawing_area *area);

G_END_DECLS
