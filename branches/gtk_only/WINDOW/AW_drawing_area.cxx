#include "aw_drawing_area.hxx"
#include "aw_gtk_closures.hxx"
#include "aw_assert.hxx"
#include <glib.h>

struct _AwDrawingAreaPrivate {
    GtkAdjustment *horizontalAdjustment;
    GtkAdjustment *verticalAdjustment;
    int left_indent; //! part of visible area not moving with scrollbar (left)
    int top_indent;  //! part of visible area not moving with scrollbar (right)
    int width;
    int height;
};

G_DEFINE_TYPE(AwDrawingArea, aw_drawing_area, GTK_TYPE_DRAWING_AREA);

static void aw_drawing_area_finalize(GObject* object);
static void aw_drawing_area_dispose(GObject* object);
static void aw_drawing_area_set_scroll_adjustments(AwDrawingArea *area, 
                                                   GtkAdjustment *hadj, 
                                                   GtkAdjustment *vadj);
static gint aw_drawing_area_button_press(GtkWidget *widget, 
                                         GdkEventButton *event);
static void aw_drawing_area_size_allocate(GtkWidget*, GtkAllocation*);

static void aw_drawing_area_class_init(AwDrawingAreaClass *clazz) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(clazz);
    GObjectClass *object_class = G_OBJECT_CLASS(clazz);
    
#if GTK_MAJOR_VERSION >2
#else
    //override set_scroll_adjustments
    clazz->set_scroll_adjustments = aw_drawing_area_set_scroll_adjustments;
    widget_class->set_scroll_adjustments_signal =
                g_signal_new("set_scroll_adjustments",
                G_TYPE_FROM_CLASS(clazz),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET(AwDrawingAreaClass, set_scroll_adjustments),
                NULL, NULL,
                g_cclosure_arb_marshal_VOID__OBJECT_OBJECT,
                G_TYPE_NONE, 2,
                GTK_TYPE_ADJUSTMENT,
                GTK_TYPE_ADJUSTMENT);  
#endif
    widget_class->button_press_event = aw_drawing_area_button_press;
    widget_class->size_allocate = aw_drawing_area_size_allocate;

    //register destructor
    object_class->dispose = aw_drawing_area_dispose;
    object_class->finalize = aw_drawing_area_finalize;
    
    //register pimpl. This way it will be freed automatically if the drawing area is destroyed 
    g_type_class_add_private (clazz, sizeof (AwDrawingAreaPrivate));
    
}

static void aw_drawing_area_init(AwDrawingArea *area) {
    gtk_widget_set_can_focus (GTK_WIDGET (area), TRUE);
    //this is done to avoid calling G_TYPE_INSTANCE_GET_PRIVATE every time 
    area->prvt = G_TYPE_INSTANCE_GET_PRIVATE (area, AW_DRAWING_AREA_TYPE, AwDrawingAreaPrivate);
    area->prvt->horizontalAdjustment = NULL;
    area->prvt->verticalAdjustment = NULL;
    area->prvt->left_indent = 0;
    area->prvt->top_indent = 0;
    area->prvt->width = 0;
    area->prvt->height = 0;
}


GtkWidget *aw_drawing_area_new() {
    return GTK_WIDGET(g_object_new(AW_DRAWING_AREA_TYPE, NULL));
}

//see https://developer.gnome.org/gobject/stable/howto-gobject-destruction.html to 
//learn about gobject finalization
static void aw_drawing_area_finalize(GObject* /*object*/) {
}

static void aw_drawing_area_dispose(GObject* object) {
    AwDrawingArea *area = AW_DRAWING_AREA(object);
    if(NULL != area && NULL != area->prvt) { //area might be NULL if the cast fails
        
        if (NULL != area->prvt->horizontalAdjustment) {
            g_object_unref(area->prvt->horizontalAdjustment);
            area->prvt->horizontalAdjustment = NULL;
        }
        
        if (NULL != area->prvt->verticalAdjustment) {
            g_object_unref(area->prvt->verticalAdjustment);
            area->prvt->verticalAdjustment = NULL;
        }
    }
}

static gint aw_drawing_area_button_press(GtkWidget *widget, GdkEventButton *) {
    if (!gtk_widget_has_focus(widget)) 
        gtk_widget_grab_focus(widget);
    return false;
}

static void adjustment_set_safely(GtkAdjustment *adj, gdouble value, gdouble upper, gdouble page_size) {
    if (!adj) return;

    if (value < 0) value = gtk_adjustment_get_value(adj);
    if (upper < 0) upper = gtk_adjustment_get_upper(adj);
    if (page_size < 0) page_size = gtk_adjustment_get_page_size(adj);
    
    if (value > upper - page_size) {
        value = upper - page_size;
    }
    else if (value < 0) {
        value = 0;
    }
    
    printf("value %f  upper %f  page %f\n", value, upper, page_size);

    g_object_freeze_notify(G_OBJECT(adj));
    gtk_adjustment_set_value(adj, value);
    gtk_adjustment_set_upper(adj, upper);
    gtk_adjustment_set_page_size(adj, page_size);
    g_object_thaw_notify(G_OBJECT(adj));
}

static void aw_drawing_area_size_allocate(GtkWidget *widget, GtkAllocation* allocation) {
    AwDrawingArea *area = AW_DRAWING_AREA(widget);
   
    gtk_widget_set_allocation(widget, allocation);

    if (gtk_widget_get_has_window(widget)
        &&  gtk_widget_get_realized(widget)) {
        gdk_window_move_resize(gtk_widget_get_window(widget),
                               allocation->x, allocation->y,
                               allocation->width, allocation->height);
    }

    adjustment_set_safely(area->prvt->horizontalAdjustment, -1, -1, 
                              allocation->width - area->prvt->left_indent);

    adjustment_set_safely(area->prvt->verticalAdjustment, -1, -1, 
                              allocation->height - area->prvt->top_indent);

    if (gtk_widget_get_has_window(widget) &&
        gtk_widget_get_realized(widget)) {
        GdkEvent *event = gdk_event_new(GDK_CONFIGURE);
        event->configure.window = GDK_WINDOW(g_object_ref(gtk_widget_get_window(widget)));
        event->configure.send_event = true;
        event->configure.x = allocation->x;
        event->configure.y = allocation->y;
        event->configure.width = allocation->width;
        event->configure.height = allocation->height;
        gtk_widget_event(widget, event);
        gdk_event_free(event);
    }
}

/**
 * Set the size of the (virtual) drawing area.
 */
void aw_drawing_area_set_size(AwDrawingArea *area, gint width, gint height) {
    aw_return_if_fail(area != NULL);
    aw_return_if_fail(area->prvt != NULL);
    aw_return_if_fail(area->prvt->horizontalAdjustment != 0);
    aw_return_if_fail(area->prvt->verticalAdjustment != 0);

    adjustment_set_safely(area->prvt->horizontalAdjustment, -1, width, -1);
    adjustment_set_safely(area->prvt->verticalAdjustment, -1, height, -1);
}

/**
 * Get the width of the (virtual) drawing area
 */
gint aw_drawing_area_get_width(AwDrawingArea *area) {
    aw_return_val_if_fail(area != NULL, -1);
    aw_return_val_if_fail(area->prvt != NULL, -1);
    aw_return_val_if_fail(area->prvt->horizontalAdjustment != 0, -1);

    return gtk_adjustment_get_upper(area->prvt->horizontalAdjustment);
}

/**
 * Get the height of the (virtual) drawing area
 */
gint aw_drawing_area_get_height(AwDrawingArea *area) {
    aw_return_val_if_fail(area != NULL, -1);
    aw_return_val_if_fail(area->prvt != NULL, -1);
    aw_return_val_if_fail(area->prvt->verticalAdjustment != 0, -1);

    return gtk_adjustment_get_upper(area->prvt->verticalAdjustment);
}

void aw_drawing_area_set_unscrolled_width(AwDrawingArea *area, gint width) {
    aw_return_if_fail(area != NULL);
    aw_return_if_fail(area->prvt != NULL);
    area->prvt->left_indent = width;

    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);

    adjustment_set_safely(area->prvt->horizontalAdjustment, -1, -1, alloc.width - width);
    
    // make sure the widget never get's smaller than the unscrolled width
    // (otherwise the scrolled part would get negative)
    gtk_widget_set_size_request(GTK_WIDGET(area), width, area->prvt->top_indent);
}

void aw_drawing_area_set_unscrolled_height(AwDrawingArea *area, gint height) {
    aw_return_if_fail(area != NULL);
    aw_return_if_fail(area->prvt != NULL);
    area->prvt->top_indent = height;

    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);

    adjustment_set_safely(area->prvt->verticalAdjustment, -1, -1, alloc.height - height);

    // make sure the widget never get's smaller than the unscrolled height
    // (otherwise the scrolled part would get negative)
    gtk_widget_set_size_request(GTK_WIDGET(area), area->prvt->left_indent, height);
}
    
void aw_drawing_area_get_scrolled_size(AwDrawingArea *area, gint* width, gint* height) {
    aw_return_if_fail(area != NULL);
    aw_return_if_fail(area->prvt != NULL);

    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);

    *width  = alloc.width  - area->prvt->left_indent;
    *height = alloc.height - area->prvt->top_indent;
}

GtkAdjustment* aw_drawing_area_get_vertical_adjustment(AwDrawingArea *area) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    return area->prvt->verticalAdjustment;
}

GtkAdjustment* aw_drawing_area_get_horizontal_adjustment(AwDrawingArea *area) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    return area->prvt->horizontalAdjustment;
}

void aw_drawing_area_set_increments(AwDrawingArea *area,
                                    gint horizontal_step_increment,
                                    gint horizontal_page_increment,
                                    gint vertical_step_increment,
                                    gint vertical_page_increment) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    AwDrawingAreaPrivate *prvt = area->prvt;

    g_object_freeze_notify(G_OBJECT(prvt->horizontalAdjustment));
    gtk_adjustment_set_step_increment(prvt->horizontalAdjustment, horizontal_step_increment);
    gtk_adjustment_set_page_increment(prvt->horizontalAdjustment, horizontal_page_increment);
    g_object_thaw_notify(G_OBJECT(prvt->horizontalAdjustment));

    g_object_freeze_notify(G_OBJECT(prvt->verticalAdjustment));    
    gtk_adjustment_set_step_increment(prvt->verticalAdjustment, vertical_step_increment);
    gtk_adjustment_set_page_increment(prvt->verticalAdjustment, vertical_page_increment);
    g_object_thaw_notify(G_OBJECT(prvt->verticalAdjustment));
}

void aw_drawing_area_set_horizontal_slider(AwDrawingArea *area, gdouble pos) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    adjustment_set_safely(area->prvt->horizontalAdjustment, pos, -1, -1);
}

void aw_drawing_area_set_vertical_slider(AwDrawingArea *area, gdouble pos) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    adjustment_set_safely(area->prvt->verticalAdjustment, pos, -1, -1);
}

static void aw_drawing_area_set_scroll_adjustments(AwDrawingArea *area, 
                                                   GtkAdjustment *hadj,
                                                   GtkAdjustment *vadj) {
    if(NULL != area && NULL != area->prvt) {
        AwDrawingAreaPrivate *prvt = area->prvt;
        
        //unref old adjustments
        if(prvt->horizontalAdjustment && prvt->horizontalAdjustment != hadj) {
            g_object_unref(area->prvt->horizontalAdjustment);
            /*changing the adjustment will break the changed callbacks which are 
             managed by aw_window. If you want to be able to change the adjustments
             you have to move set_horizontal_change_callback and set_vertical_change_callback
             from AW_window into this class.*/
            aw_assert(false);
        }
        if(prvt->verticalAdjustment && prvt->verticalAdjustment != hadj) {
            g_object_unref(area->prvt->verticalAdjustment);
            aw_assert(false);
        }
        
        //ref new adjustments if they exist, and set lower bound
        if(NULL != hadj) {
            prvt->horizontalAdjustment = hadj;
            g_object_ref(prvt->horizontalAdjustment);
 
            g_object_freeze_notify(G_OBJECT(prvt->horizontalAdjustment));
            gtk_adjustment_set_lower(prvt->horizontalAdjustment, 0);
            gtk_adjustment_set_value(prvt->horizontalAdjustment, 0);
            g_object_thaw_notify(G_OBJECT(prvt->horizontalAdjustment));
            
        }
        
        if(NULL != vadj) {
            prvt->verticalAdjustment = vadj;
            g_object_ref(prvt->verticalAdjustment);
            g_object_freeze_notify(G_OBJECT(prvt->verticalAdjustment));    
            gtk_adjustment_set_lower(prvt->verticalAdjustment, 0);
            gtk_adjustment_set_value(prvt->horizontalAdjustment, 0);
            g_object_thaw_notify(G_OBJECT(prvt->verticalAdjustment));
        }
    }
}


