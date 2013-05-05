#include "aw_drawing_area.hxx"
#include "aw_gtk_closures.hxx"
#include "aw_assert.hxx"
#include <glib.h>

struct _AwDrawingAreaPrivate {
    GtkAdjustment *horizontalAdjustment;
    GtkAdjustment *verticalAdjustment;
};

G_DEFINE_TYPE(AwDrawingArea, aw_drawing_area, GTK_TYPE_DRAWING_AREA);

static void aw_drawing_area_finalize(GObject* object);
static void aw_drawing_area_dispose(GObject* object);
static void aw_drawing_area_set_scroll_adjustments(AwDrawingArea *area, 
                                                   GtkAdjustment *hadj, 
                                                   GtkAdjustment *vadj);
static gint aw_drawing_area_button_press(GtkWidget *widget, 
                                         GdkEventButton *event);

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
}


GtkWidget *aw_drawing_area_new() {
    AwDrawingArea *area = AW_DRAWING_AREA(g_object_new(AW_DRAWING_AREA_TYPE, NULL));
    return GTK_WIDGET(area);
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

static gint aw_drawing_area_button_press(GtkWidget *widget, GdkEventButton *event) {
    if (!gtk_widget_has_focus(widget)) 
        gtk_widget_grab_focus(widget);
    return false;
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
    
    gtk_adjustment_set_step_increment(prvt->horizontalAdjustment, horizontal_step_increment);
    gtk_adjustment_set_page_increment(prvt->horizontalAdjustment, horizontal_page_increment);
    
    gtk_adjustment_set_step_increment(prvt->verticalAdjustment, vertical_step_increment);
    gtk_adjustment_set_page_increment(prvt->verticalAdjustment, vertical_page_increment);
    
    gtk_adjustment_changed(prvt->horizontalAdjustment);
    gtk_adjustment_changed(prvt->verticalAdjustment);   
}

void aw_drawing_area_set_horizontal_slider(AwDrawingArea *area, gdouble pos) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    AwDrawingAreaPrivate *prvt = area->prvt; 
    gtk_adjustment_set_value(prvt->horizontalAdjustment, pos);
    gtk_adjustment_value_changed(prvt->horizontalAdjustment);
}

void aw_drawing_area_set_vertical_slider(AwDrawingArea *area, gdouble pos) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    AwDrawingAreaPrivate *prvt = area->prvt; 
    gtk_adjustment_set_value(prvt->verticalAdjustment, pos);
    gtk_adjustment_value_changed(prvt->verticalAdjustment);
}


void aw_drawing_area_set_picture_size(AwDrawingArea *area, gint picture_width,
                                      gint picture_height, gint visible_width,
                                      gint visible_height) {

    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    AwDrawingAreaPrivate *prvt = area->prvt;
    
    gtk_adjustment_set_lower(prvt->horizontalAdjustment, 0);
    gtk_adjustment_set_upper(prvt->horizontalAdjustment, picture_width);
    gtk_adjustment_set_page_size(prvt->horizontalAdjustment, visible_width);

    
    gtk_adjustment_set_lower(prvt->verticalAdjustment, 0);
    gtk_adjustment_set_upper(prvt->verticalAdjustment, picture_height);
    gtk_adjustment_set_page_size(prvt->verticalAdjustment, visible_height);

    
    gtk_adjustment_changed(prvt->horizontalAdjustment);
    gtk_adjustment_changed(prvt->verticalAdjustment);
    
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
        
        //ref new adjustments if they exist
        if(NULL != hadj) {
            prvt->horizontalAdjustment = hadj;
            g_object_ref(prvt->horizontalAdjustment);
        }
        if(NULL != vadj) {
            prvt->verticalAdjustment = vadj;
            g_object_ref(prvt->verticalAdjustment);
            
        }
    }
}


