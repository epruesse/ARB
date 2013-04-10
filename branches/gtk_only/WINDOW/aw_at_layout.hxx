/* 
 * File:   aw_at_layout.hxx
 * Author: Elmar Pruesse
 * Created on April 9, 2013, 9:11 PM
 * 
 * This file defines the aw_at_layout container.
 *
 * The aw_at_layout container is a simple extension of gtk_fixed. It 
 * allows widget placement and scaling via AW_at.
 */

#pragma once
#include <gtk/gtkfixed.h>
#include "aw_at.hxx"

//#include "aw_gtk_foward_declarations.hxx"

G_BEGIN_DECLS

#define AW_AT_LAYOUT_TYPE (aw_at_layout_get_type())
#define AW_AT_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), AW_AT_LAYOUT_TYPE, AwAtLayout))
#define AW_AT_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST(klass), AW_AT_LAYOUT_TYPE, AwAtLayoutClass)
#define IS_AW_AT_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), AwAtLayout))
#define IS_AW_AT_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), AwAtLayout))
#define AW_AT_LAYOUT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), AW_AT_LAYOUT_TYPE, AwAtLayoutClass))

typedef struct _AwAtLayout AwAtLayout;
typedef struct _AwAtLayoutClass AwAtLayoutClass;
typedef struct _AwAtLayoutChild AwAtLayoutChild;

struct _AwAtLayout 
{
    GtkFixed parent;

    GList *children;
};

struct _AwAtLayoutClass
{
    GtkFixedClass parent_class;
  
};

struct _AwAtLayoutChild {
    GtkWidget *widget;
    guint x;
    guint y;
    gfloat xalign;
    gfloat yalign;
    gfloat xscale;
    gfloat yscale;
    gfloat xmove;
    gfloat ymove;
};


GType aw_at_layout_get_type();

GtkWidget *aw_at_layout_new();
void      aw_at_layout_put(AwAtLayout* layout,
                           GtkWidget* widget, 
                           AW_at* at);
                            

G_END_DECLS
