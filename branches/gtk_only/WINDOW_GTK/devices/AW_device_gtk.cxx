// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_gtk_migration_helpers.hxx"
#include <gdk/gdk.h>
#include "aw_position.hxx"
#include "aw_device.hxx"
#include "gtk/gtkwidget.h"
#include "gtk/gtkstyle.h"
#include "aw_device_gtk.hxx"
#include <gdk/gdkgc.h>


#if defined(WARN_TODO)
#warning change implementation of draw functions (read more)
// * filter has to be checked early (in AW_device)
// * functions shall use Position/LineVector/Rectangle only
#endif

using namespace AW;

AW_DEVICE_TYPE AW_device_gtk::type() { return AW_DEVICE_SCREEN; }

#define XDRAW_PARAM2(common)    (common)->get_display(), (common)->get_window_id()
#define XDRAW_PARAM3(common,gc) XDRAW_PARAM2(common), (common)->get_GC(gc)





AW_device_gtk::AW_device_gtk(AW_common *commoni, GtkWidget *gtk_drawingArea) :
        AW_device(commoni),
        drawingArea(gtk_drawingArea)
{

    aw_assert(gtk_drawingArea != NULL);
    aw_assert(commoni != NULL);
    gtk_widget_set_app_paintable(gtk_drawingArea, true);

}

bool AW_device_gtk::line_impl(int gc, const LineVector& Line, AW_bitset filteri) {

    bool drawflag = false;
    

    arb_assert(get_common()->get_GC(gc) != NULL);

    if (filteri & filter) {
        LineVector transLine = transform(Line);
        LineVector clippedLine;
        drawflag = true;

        drawflag = clip(transLine, clippedLine); 
        
         
        if (drawflag) {

            aw_assert(clippedLine.valid());
              //this is the version that should be used if clipping is active
            gdk_draw_line(GDK_DRAWABLE(gtk_widget_get_window(GTK_WIDGET(drawingArea))),
                         get_common()->get_GC(gc),
                         int(clippedLine.start().xpos()),
                         int(clippedLine.start().ypos()),
                         int(clippedLine.head().xpos()),
                         int(clippedLine.head().ypos()));

            AUTO_FLUSH(this);
        }
    }

    return true;
}


bool AW_device_gtk::draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t /*start*/, size_t size,
                                    AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/)
{
    AW_pos X, Y;
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));
    AW_device_gtk *device_gtk = DOWNCAST(AW_device_gtk*, device);

    GdkGCValues values;
    GdkGC *gdkGc = device_gtk->get_common()->get_GC(gc);

    gdk_gc_get_values(gdkGc, &values);
    ASSERT_FALSE(values.font == NULL);
    FIXME("Use NULL font");
    //TODO according to the gtk documentation it should be possible to use NULL as font.
    //      NULL means: use the gc font. However that does not work. Maybe it will in a newer gtk version.
    gdk_draw_string(GDK_DRAWABLE(gtk_widget_get_window(device_gtk->drawingArea)),
                    values.font,
                    gdkGc,
                    AW_INT(X),
                    AW_INT(Y),
                    str);

    return true;
}


bool AW_device_gtk::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {

    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, (AW_CL)this, 0.0, 0.0, draw_string_on_screen);
}

bool AW_device_gtk::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        if (filled) {
            Rectangle transRect = transform(rect);
            Rectangle clippedRect;
            drawflag = box_clip(transRect, clippedRect);
            if (drawflag) {
                
                gdk_draw_rectangle(drawingArea->window,
                                   get_common()->get_GC(gc),
                                   filled, 
                                   clippedRect.left(),
                                   clippedRect.top(),
                                   clippedRect.width() + 1,
                                   clippedRect.height() +1); // see aw_device.hxx@WORLD_vs_PIXEL
                AUTO_FLUSH(this);
            }
        }
        else {
            drawflag = generic_box(gc, false, rect, filteri);
        }
    }
    return drawflag;
}

bool AW_device_gtk::circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) {
    aw_assert(radius.x()>0 && radius.y()>0);
    return arc_impl(gc, filled, center, radius, 0, 360, filteri);
}

bool AW_device_gtk::arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) {
    // degrees start at east side of unit circle,
    // but orientation is clockwise (because ARBs y-coordinate grows downwards)

    bool drawflag = false;
    if (filteri & filter) {
        Rectangle Box(center-radius, center+radius);
        Rectangle screen_box = transform(Box);

        drawflag = !is_outside_clip(screen_box);
        if (drawflag) {
            int             width  = AW_INT(screen_box.width());
            int             height = AW_INT(screen_box.height());
            const Position& ulc    = screen_box.upper_left_corner();
            int             xl     = AW_INT(ulc.xpos());
            int             yl     = AW_INT(ulc.ypos());

            aw_assert(arc_degrees >= -360 && arc_degrees <= 360);

            // ARB -> X
            start_degrees = -start_degrees;
            arc_degrees   = -arc_degrees;

            while (start_degrees<0) start_degrees += 360;
            
            gdk_draw_arc(GDK_DRAWABLE(drawingArea->window), get_common()->get_GC(gc), filled, xl, yl, width, height, 64*start_degrees, 64*arc_degrees );
            AUTO_FLUSH(this);
        }
    }
    return drawflag;

}

void AW_device_gtk::clear(AW_bitset filteri) {
    if (filteri & filter) {
        gdk_window_clear(drawingArea->window);
    }
}

void AW_device_gtk::clear_part(const Rectangle& rect, AW_bitset filteri) {
    if (filteri & filter) {
        Rectangle transRect = transform(rect);
        Rectangle clippedRect;
        bool drawflag = box_clip(transRect, clippedRect);
        if (drawflag) {
            
            gdk_window_clear_area(drawingArea->window,
                                  AW_INT(clippedRect.left()),
                                  AW_INT(clippedRect.top()),
                                  AW_INT(clippedRect.width())+1, // see aw_device.hxx@WORLD_vs_PIXEL
                                  AW_INT(clippedRect.height())+1);
            //TODO the old motif code did not generate expose events on clear area
            AUTO_FLUSH(this);
        }
    }
}


void AW_device_gtk::flush() {
    gdk_flush();
}

void AW_device_gtk::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height,
                                AW_pos dest_x, AW_pos dest_y) {
  GdkRectangle rect;
  rect.x = AW_INT(src_x), rect.y = AW_INT(src_y), rect.width=AW_INT(width), rect.height=AW_INT(height);
  gdk_window_move_region(drawingArea->window, gdk_region_rectangle(&rect), AW_INT(dest_x), AW_INT(dest_y));
  AUTO_FLUSH(this);
}
