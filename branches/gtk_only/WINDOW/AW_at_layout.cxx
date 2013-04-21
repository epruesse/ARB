#include "aw_at_layout.hxx"
#include "aw_assert.hxx"

enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ARIGHT
};

typedef struct _AwAtLayoutPrivate AwAtLayoutPrivate;

struct _AwAtLayoutPrivate {
    gint width;
    gint height;
};

#define AW_AT_LAYOUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AW_AT_LAYOUT_TYPE, AwAtLayoutPrivate))

G_DEFINE_TYPE(AwAtLayout, aw_at_layout, GTK_TYPE_FIXED);

static void aw_at_layout_size_allocate(GtkWidget*, GtkAllocation*);
#if GTK_MAJOR_VERSION > 2
static void aw_at_layout_get_preferred_width(GtkWidget*, gint*, gint*);
static void aw_at_layout_get_preferred_height(GtkWidget*, gint*, gint*);
#endif
static void aw_at_layout_size_request(GtkWidget*, GtkRequisition*);
// static void aw_at_layout_size_realize(GtkWidget*);
static GType aw_at_layout_child_type(GtkContainer*) {return GTK_TYPE_WIDGET;}
static void aw_at_layout_forall(GtkContainer*, gboolean internal, GtkCallback cb, gpointer cd);

static void aw_at_layout_class_init(AwAtLayoutClass *klass) {
    GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
    GtkContainerClass *container_class = (GtkContainerClass*) klass;

    // widget_class->realize = aw_at_layout_reqlize;
    widget_class->size_allocate = aw_at_layout_size_allocate;
#if GTK_MAJOR_VERSION > 2
    widget_class->get_preferred_width = aw_at_layout_get_preferred_width;
    widget_class->get_preferred_height = aw_at_layout_get_preferred_height;
#else
    widget_class->size_request = aw_at_layout_size_request;
#endif

    container_class->child_type = aw_at_layout_child_type;
    container_class->forall = aw_at_layout_forall;

    g_type_class_add_private(G_OBJECT_CLASS (klass), sizeof(AwAtLayoutPrivate));
}

static void aw_at_layout_init(AwAtLayout *self) {
}

GtkWidget *aw_at_layout_new() {
    AwAtLayout *self = AW_AT_LAYOUT(g_object_new(AW_AT_LAYOUT_TYPE, NULL));
    return GTK_WIDGET(self);
}

void aw_at_layout_put(AwAtLayout* self,
                      GtkWidget* widget, 
                      AW_at* at) 
{
  //    g_return_if_fail(IS_AW_AT_LAYOUT(layout));
    g_return_if_fail(GTK_IS_WIDGET(widget));
    AwAtLayoutPrivate *prvt = AW_AT_LAYOUT_GET_PRIVATE(self);
    prvt->width = at->max_x_size;
    prvt->height = at->max_y_size;

    AwAtLayoutChild *child = g_new(AwAtLayoutChild, 1);
    child->widget = widget;
    child->x = at->get_at_xposition();
    child->y = at->get_at_yposition();
    child->xalign = at->correct_for_at_center / 2.f;
    child->yalign = 0.f;
    
    if (at->attach_x) {
        if (at->attach_lx) {
            child->xmove = 1.f;
            child->xscale = 0.f;
        } else {
            child->xmove = 0.f;
            child->xscale = 1.f;
        }
    } else {
        if (at->attach_lx) {
            child->xmove = 1.f;
            child->xscale = 0.f;
        } else {
            child->xmove = 0.f;
            child->xscale = 0.f;
        }
    }

    if (at->attach_y) {
        if (at->attach_ly) {
            child->ymove = 1.f;
            child->yscale = 0.f;
        } else {
            child->ymove = 0.f;
            child->yscale = 1.f;
        }
    } else {
        if (at->attach_ly) {
            child->ymove = 1.f;
            child->yscale = 0.f;
        } else {
            child->ymove = 0.f;
            child->yscale = 0.f;
        }
    }


    gtk_widget_set_parent(widget, GTK_WIDGET(self));

    self->children = g_list_append(self->children, child);

    
}

static void aw_at_layout_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    AwAtLayout *self = AW_AT_LAYOUT(widget);
    GList *children = self->children;
    requisition->width = requisition->height = 0;
    while (children) {
        AwAtLayoutChild *child = (AwAtLayoutChild*) children->data;
        children = children->next;
        if (gtk_widget_get_visible(child->widget)) {
            GtkRequisition child_req;
            gtk_widget_size_request(child->widget, &child_req);
            requisition->width = MAX(requisition->width, child->x + child_req.width * (1.-child->xalign));
            requisition->height = MAX(requisition->height, child->y + child_req.height * (1.-child->yalign));
        }
    }
    //prvt->width = requisition->width;
    //prvt->height = requisition->height;
}

#if GTK_MAJOR_VERSION > 2
static void aw_at_layout_get_preferred_width(GtkWidget* widget, gint *minimal_width, gint *natural_width) {
    GtkRequisition req;
    aw_at_layout_size_request(widget, &req);
    *minimal_width = *natural_width = req.width;
}
static void aw_at_layout_get_preferred_height(GtkWidget* widget, gint *minimal_height, gint *natural_height) {
    GtkRequisition req;
    aw_at_layout_size_request(widget, &req);
    *minimal_height = *natural_height = req.height;
}
#endif

static void aw_at_layout_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    AwAtLayout *self = AW_AT_LAYOUT(widget);
    AwAtLayoutPrivate *prvt = AW_AT_LAYOUT_GET_PRIVATE(self);
    
    gtk_widget_set_allocation(widget, allocation);

    if (gtk_widget_get_has_window(widget)
        &&  gtk_widget_get_realized(widget)) {
        gdk_window_move_resize(gtk_widget_get_window(widget),
                               allocation->x, allocation->y,
                               allocation->width, allocation->height);
    }

    GList *children = self->children;
    while(children) {
        AwAtLayoutChild *child = (AwAtLayoutChild*) children->data;
        children = children->next;
        if (gtk_widget_get_visible(child->widget)) {
            GtkRequisition child_req;
            gtk_widget_get_child_requisition(child->widget, &child_req);
            GtkAllocation child_alloc;
            child_alloc.x = child->x - child->xalign * child_req.width
              + child->xmove * (allocation->width - prvt->width);
            child_alloc.y = child->y - child->yalign * child_req.height
              + child->ymove * (allocation->height - prvt->height);

            child_alloc.width = child_req.width + (allocation->width - prvt->width) * child->xscale;
            child_alloc.height =child_req.height + (allocation->height - prvt->height) * child->yscale;

            if (!gtk_widget_get_has_window(widget)) {
              child_alloc.x += allocation->x;
              child_alloc.y += allocation->y;
            }
            
            gtk_widget_size_allocate(child->widget, &child_alloc);
        }
    }

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

static void aw_at_layout_forall(GtkContainer *self, gboolean intern, GtkCallback cb, gpointer cd) {
    GList *children = AW_AT_LAYOUT(self)->children;
    while(children) {
        AwAtLayoutChild *child = (AwAtLayoutChild*) children->data;
        children = children->next;
        (*cb)(child->widget, cd);
    }
}
