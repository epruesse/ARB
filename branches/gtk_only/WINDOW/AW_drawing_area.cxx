#include "aw_drawing_area.hxx"
#include "aw_gtk_closures.hxx"
#include "aw_assert.hxx"
#include <glib.h>



struct _AW_drawing_area_private {
    GtkAdjustment *horizontalAdjustment;
    GtkAdjustment *verticalAdjustment;
};


//see https://developer.gnome.org/gobject/stable/howto-gobject-destruction.html to 
//learn about gobject finalization
static void aw_drawing_area_finalize(GObject* object) {
    
}

static void aw_drawing_area_dispose(GObject* object) {
    AW_drawing_area *area = AW_DRAWING_AREA(object);
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

GtkAdjustment* aw_drawing_area_get_vertical_adjustment(AW_drawing_area *area) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    return area->prvt->verticalAdjustment;
}
GtkAdjustment* aw_drawing_area_get_horizontal_adjustment(AW_drawing_area *area) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    return area->prvt->horizontalAdjustment;
}

void aw_drawing_area_set_increments(AW_drawing_area *area,
                                    gint horizontal_step_increment,
                                    gint horizontal_page_increment,
                                    gint vertical_step_increment,
                                    gint vertical_page_increment) {
    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    AW_drawing_area_private *prvt = area->prvt;
    
    gtk_adjustment_set_step_increment(prvt->horizontalAdjustment, horizontal_step_increment);
    gtk_adjustment_set_page_increment(prvt->horizontalAdjustment, horizontal_page_increment);
    
    gtk_adjustment_set_step_increment(prvt->verticalAdjustment, vertical_step_increment);
    gtk_adjustment_set_page_increment(prvt->verticalAdjustment, vertical_page_increment);
    
    gtk_adjustment_changed(prvt->horizontalAdjustment);
    gtk_adjustment_changed(prvt->verticalAdjustment);   
}

void aw_drawing_area_set_picture_size(AW_drawing_area *area, gint picture_width,
                                      gint picture_height, gint visible_width,
                                      gint visible_height) {

    aw_assert(NULL != area);
    aw_assert(NULL != area->prvt);
    AW_drawing_area_private *prvt = area->prvt;
    
    gtk_adjustment_set_lower(prvt->horizontalAdjustment, 0);
    gtk_adjustment_set_upper(prvt->horizontalAdjustment, picture_width);
    gtk_adjustment_set_page_size(prvt->horizontalAdjustment, visible_width);

    
    gtk_adjustment_set_lower(prvt->verticalAdjustment, 0);
    gtk_adjustment_set_upper(prvt->verticalAdjustment, picture_height);
    gtk_adjustment_set_page_size(prvt->verticalAdjustment, visible_height);

    
    gtk_adjustment_changed(prvt->horizontalAdjustment);
    gtk_adjustment_changed(prvt->verticalAdjustment);
    
}

static void set_scroll_adjustments(AW_drawing_area *area, GtkAdjustment *hadj,
                                   GtkAdjustment *vadj) {
    if(NULL != area && NULL != area->prvt) {
        AW_drawing_area_private *prvt = area->prvt;
        
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

static void aw_drawing_area_class_init(AW_drawing_area_class *clazz) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(clazz);
    GObjectClass *object_class = G_OBJECT_CLASS(clazz);
    
    //override set_scroll_adjustments
    clazz->set_scroll_adjustments = set_scroll_adjustments;
    widget_class->set_scroll_adjustments_signal =
                g_signal_new("set_scroll_adjustments",
                G_TYPE_FROM_CLASS(clazz),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET(AW_drawing_area_class, set_scroll_adjustments),
                NULL, NULL,
                g_cclosure_arb_marshal_VOID__OBJECT_OBJECT,
                G_TYPE_NONE, 2,
                GTK_TYPE_ADJUSTMENT,
                GTK_TYPE_ADJUSTMENT);  
    
    //register destructor
    object_class->dispose = aw_drawing_area_dispose;
    object_class->finalize = aw_drawing_area_finalize;
    
    //register pimpl. This way it will be freed automatically if the drawing area is destroyed 
    g_type_class_add_private (clazz, sizeof (AW_drawing_area_private));
    
}

static void aw_drawing_area_init(AW_drawing_area *area) {
    //this is done to avoid calling G_TYPE_INSTANCE_GET_PRIVATE every time 
    area->prvt = G_TYPE_INSTANCE_GET_PRIVATE (area, AW_DRAWING_AREA_TYPE, AW_drawing_area_private);
    area->prvt->horizontalAdjustment = NULL;
    area->prvt->verticalAdjustment = NULL;
}

GtkWidget *aw_drawing_area_new() {
    AW_drawing_area *area = AW_DRAWING_AREA(g_object_new(AW_DRAWING_AREA_TYPE, NULL));
    return GTK_WIDGET(area);
}

GType aw_drawing_area_get_type() {
    static GType type = 0;//type singleton
    
    if(0 == type) { //first call
        static const GTypeInfo type_info = {
            sizeof(AW_drawing_area_class),
            NULL,
            NULL,
            (GClassInitFunc) aw_drawing_area_class_init,
            NULL,
            NULL,
            sizeof(AW_drawing_area),
            0,
            (GInstanceInitFunc) aw_drawing_area_init,
        };
        
        type = g_type_register_static(GTK_TYPE_DRAWING_AREA, "aw_drawing_area",
                                      &type_info, (GTypeFlags) 0);
        
    }
    return type;
}
