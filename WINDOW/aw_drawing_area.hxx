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
 *  
 */

#pragma once
#include <gtk/gtk.h>

#include "aw_gtk_forward_declarations.hxx"

//gtk boilerplate 
G_BEGIN_DECLS

#define AW_DRAWING_AREA_TYPE (aw_drawing_area_get_type())
#define AW_DRAWING_AREA(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), AW_DRAWING_AREA_TYPE, AwDrawingArea))
#define AW_DRAWING_AREA_CLASS(clazz) (G_TYPE_CHECK_CLASS_CAST(clazz), AW_DRAWING_AREA_TYPE, AwDrawingAreaClass)
#define IS_AW_DRAWING_AREA(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), AwDrawingArea))
#define IS_AW_DRAWING_AREA_CLASS (clazz) (G_TYPE_CHECK_CLASS_TYPE((clazz), AwDrawingArea))

typedef struct _AwDrawingArea AwDrawingArea;
typedef struct _AwDrawingAreaClass AwDrawingAreaClass;
typedef struct _AwDrawingAreaPrivate AwDrawingAreaPrivate;

struct _AwDrawingArea
{
    GtkDrawingArea parent; /**< Parent instance has to be the first attribute */
    AwDrawingAreaPrivate *prvt;
};

struct _AwDrawingAreaClass
{
    GtkDrawingAreaClass parent_class; /**< Parent class has to be first attribute */
    
    /**Is called whenever the value of one of the scrollbars changes */
    void (*scroll_value_changed)   (AwDrawingArea *da);
    void (*set_scroll_adjustments) (AwDrawingArea *da, GtkAdjustment *hadjustment,
                                    GtkAdjustment *vadjustment);
};
    
GType aw_drawing_area_get_type() G_GNUC_CONST;

GtkWidget *aw_drawing_area_new();

void aw_drawing_area_set_size(AwDrawingArea *area, gint width, gint height);
gint aw_drawing_area_get_width(AwDrawingArea *area);
gint aw_drawing_area_get_height(AwDrawingArea *area);
void aw_drawing_area_set_unscrolled_width(AwDrawingArea *area, gint height);
void aw_drawing_area_set_unscrolled_height(AwDrawingArea *area, gint width);
void aw_drawing_area_get_scrolled_size(AwDrawingArea *area, gint* width, gint* height);

/** set the step_increment and page_increment values for the scrollbars*/
void aw_drawing_area_set_increments         (AwDrawingArea *area,
                                             gint horizontal_step_increment,
                                             gint horizontal_page_increment,
                                             gint vertical_step_increment,
                                             gint vertical_page_increment);
void aw_drawing_area_set_horizontal_slider  (AwDrawingArea *area, gdouble pos);
void aw_drawing_area_set_vertical_slider    (AwDrawingArea *area, gdouble pos);

GtkAdjustment* aw_drawing_area_get_vertical_adjustment   (AwDrawingArea *area);
GtkAdjustment* aw_drawing_area_get_horizontal_adjustment (AwDrawingArea *area);

G_END_DECLS
