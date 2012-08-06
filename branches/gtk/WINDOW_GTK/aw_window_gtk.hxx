// ============================================================= //
//                                                               //
//   File      : aw_window_gtk.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 6, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //



#ifndef AW_WINDOW_GTK_HXX_
#define AW_WINDOW_GTK_HXX_

#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
/*
 * Contains all gtk dependent attributs from aw_window.
 */
class AW_window_gtk {
    friend class AW_window;

    GtkWidget *window; /*< The gtk window instance managed by this aw_window */


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


#endif /* AW_WINDOW_GTK_HXX_ */
