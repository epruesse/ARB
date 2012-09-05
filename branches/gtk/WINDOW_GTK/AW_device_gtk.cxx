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


//FIXME device flushing is not implemented right now.


AW_device_gtk::AW_device_gtk(AW_common *commoni, GtkWidget *drawingArea) :
        AW_device(commoni),
        pixmap(gdk_pixmap_new(drawingArea->window, 3000, 3000, -1)),
        drawingArea(drawingArea) //FIXME get width and height from somewhere
{

    arb_assert(drawingArea != NULL);
    arb_assert(commoni != NULL);

    //subsequent calls only work on a realized widget
    if(!gtk_widget_get_realized(drawingArea)) {
        gtk_widget_realize(drawingArea);
    }

    //set the background of the widget to the pixmap.
    //later we will draw on this pixmap instead of the window.
    //this way we can get around implementing our own expose handler :)
    GtkStyle* style = gtk_widget_get_style (drawingArea); //this call fails if the widget has not been realized
    style->bg_pixmap[0] = pixmap;
    style->bg_pixmap[1] = pixmap;
    style->bg_pixmap[2] = pixmap;
    style->bg_pixmap[3] = pixmap;
    style->bg_pixmap[4] = pixmap;

    gtk_widget_set_style(drawingArea, style);

    //initialy the pixmap is black.
    //set background color to the window background color
    GdkGC* tempGc = gdk_gc_new(pixmap);
    gdk_gc_set_foreground(tempGc, &style->bg[GTK_STATE_NORMAL]);
    gdk_draw_rectangle(GDK_DRAWABLE(pixmap),tempGc, true, 0, 0, 3000, 3000);//fixme use real width and height

    //FIXME the pixmap should be transparent in the beginning.

}

bool AW_device_gtk::line_impl(int gc, const LineVector& Line, AW_bitset filteri) {

    bool drawflag = false;

    arb_assert(get_common()->get_GC(gc) != NULL);

    if (filteri & filter) {
        LineVector transLine = transform(Line);
        LineVector clippedLine;
        drawflag = true;
        //clip(transLine, clippedLine); //FIXME clipping
        if (drawflag) {


              //this is the version that should be used if clipping is active
//            gdk_draw_line(GDK_DRAWABLE(pixmap),
//                         //get_common()->get_GC(gc), // FIXME get real gc instead
//                         drawingArea->style->white_gc,
//                         int(clippedLine.start().xpos()),
//                         int(clippedLine.start().ypos()),
//                         int(clippedLine.head().xpos()),
//                         int(clippedLine.head().ypos()));

            GdkGC* pGc = gdk_gc_new(GDK_DRAWABLE(pixmap));

            gdk_draw_line(GDK_DRAWABLE(pixmap),
                         get_common()->get_GC(gc),
                    //drawingArea->style->white_gc,
                   // pGc,
                         int(transLine.start().xpos()),
                         int(transLine.start().ypos()),
                         int(transLine.head().xpos()),
                         int(transLine.head().ypos()));

            //AUTO_FLUSH(this);
        }
    }

    return true;
}

//FIXME I do not fully understand why this has to be done in a out of class callback. Check later if this can be transformed into a member
bool AW_device_gtk::draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t /*start*/, size_t size,
                                    AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/)
{
    AW_pos X, Y;
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));
    AW_device_gtk *device_gtk = DOWNCAST(AW_device_gtk*, device);

    gdk_draw_string(GDK_DRAWABLE(device_gtk->pixmap),
                    gdk_font_load("-bitstream-courier 10 pitch-bold-i-normal--0-0-0-0-m-0-ascii-0"), //FIXME use real font
                    device_gtk->get_common()->get_GC(gc),
                    AW_INT(X),
                    AW_INT(Y),
                    str);

    return true;
}


bool AW_device_gtk::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {

    printf("impl xy: %f, %f\n", pos.xpos(), pos.ypos());

    GdkGCValues *values;
    gdk_gc_get_values(get_common()->get_GC(gc), values);

    //FIXME what does y coordinate -1 mean?
    //FIXME do not ignore the alignment.
    //FIXME use real gc once the color problem is solved
    gdk_draw_string(GDK_DRAWABLE(pixmap),
                    values->font, //FIXME use real font
                    get_common()->get_GC(gc),
                    pos.xpos(),
                    pos.ypos(),
                    str);
    return true;


    //This is the old & soon the new way with clipping
//    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, (AW_CL)this, 0.0, 0.0, draw_string_on_screen);
}

bool AW_device_gtk::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
//    bool drawflag = false;
//    if (filteri & filter) {
//        if (filled) {
//            Rectangle transRect = transform(rect);
//            Rectangle clippedRect;
//            drawflag = box_clip(transRect, clippedRect);
//            if (drawflag) {
//                XFillRectangle(XDRAW_PARAM3(get_common(), gc),
//                               AW_INT(clippedRect.left()),
//                               AW_INT(clippedRect.top()),
//                               AW_INT(clippedRect.width())+1, // see aw_device.hxx@WORLD_vs_PIXEL
//                               AW_INT(clippedRect.height())+1);
//                AUTO_FLUSH(this);
//            }
//        }
//        else {
//            drawflag = generic_box(gc, false, rect, filteri);
//        }
//    }
//    return drawflag;
    GTK_NOT_IMPLEMENTED;
    return true;
}

bool AW_device_gtk::circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) {
//    aw_assert(radius.x()>0 && radius.y()>0);
//    return arc_impl(gc, filled, center, radius, 0, 360, filteri);

    GTK_NOT_IMPLEMENTED;
    return true;

}

bool AW_device_gtk::arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) {
    // degrees start at east side of unit circle,
    // but orientation is clockwise (because ARBs y-coordinate grows downwards)

//    bool drawflag = false;
//    if (filteri & filter) {
//        Rectangle Box(center-radius, center+radius);
//        Rectangle screen_box = transform(Box);
//
//        drawflag = !is_outside_clip(screen_box);
//        if (drawflag) {
//            int             width  = AW_INT(screen_box.width());
//            int             height = AW_INT(screen_box.height());
//            const Position& ulc    = screen_box.upper_left_corner();
//            int             xl     = AW_INT(ulc.xpos());
//            int             yl     = AW_INT(ulc.ypos());
//
//            aw_assert(arc_degrees >= -360 && arc_degrees <= 360);
//
//            // ARB -> X
//            start_degrees = -start_degrees;
//            arc_degrees   = -arc_degrees;
//
//            while (start_degrees<0) start_degrees += 360;
//
//            if (!filled) {
//                XDrawArc(XDRAW_PARAM3(get_common(), gc), xl, yl, width, height, 64*start_degrees, 64*arc_degrees);
//            }
//            else {
//                XFillArc(XDRAW_PARAM3(get_common(), gc), xl, yl, width, height, 64*start_degrees, 64*arc_degrees);
//            }
//            AUTO_FLUSH(this);
//        }
//    }
//    return drawflag;

    GTK_NOT_IMPLEMENTED;
    return true;
}

void AW_device_gtk::clear(AW_bitset filteri) {
//    if (filteri & filter) {
//        XClearWindow(XDRAW_PARAM2(get_common()));
//        AUTO_FLUSH(this);
//    }
    GTK_NOT_IMPLEMENTED;
}

void AW_device_gtk::clear_part(const Rectangle& rect, AW_bitset filteri) {
//    if (filteri & filter) {
//        Rectangle transRect = transform(rect);
//        Rectangle clippedRect;
//        bool drawflag = box_clip(transRect, clippedRect);
//        if (drawflag) {
//            XClearArea(XDRAW_PARAM2(get_common()),
//                       AW_INT(clippedRect.left()),
//                       AW_INT(clippedRect.top()),
//                       AW_INT(clippedRect.width())+1, // see aw_device.hxx@WORLD_vs_PIXEL
//                       AW_INT(clippedRect.height())+1,
//                       False);
//            AUTO_FLUSH(this);
//        }
//    }
    GTK_NOT_IMPLEMENTED;
}


void AW_device_gtk::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset /*filteri*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
//    const XFontStruct *xfs     = get_common()->get_xfont(gc);
//    AW_pos             X, Y;    // Transformed pos
//    AW_pos             width, height;
//    long               textlen = strlen(string);
//
//    this->transform(x, y, X, Y);
//    width  = get_string_size(gc, string, textlen);
//    height = xfs->max_bounds.ascent + xfs->max_bounds.descent;
//    X      = x_alignment(X, width, alignment);
//
//    const AW_screen_area& clipRect = get_cliprect();
//
//    if (X > clipRect.r) return;
//    if (X < clipRect.l) {
//        width = width + X - clipRect.l;
//        X = clipRect.l;
//    }
//
//    if (X + width > clipRect.r) {
//        width = clipRect.r - X;
//    }
//
//    if (Y < clipRect.t) return;
//    if (Y > clipRect.b) return;
//    if (width <= 0 || height <= 0) return;
//
//    XClearArea(XDRAW_PARAM2(get_common()),
//               AW_INT(X), AW_INT(Y)-AW_INT(xfs->max_bounds.ascent), AW_INT(width), AW_INT(height), False);
//
//    AUTO_FLUSH(this);
    GTK_NOT_IMPLEMENTED;
}

void AW_device_gtk::flush() {
//    XFlush(get_common()->get_display());
    GTK_NOT_IMPLEMENTED;
}

void AW_device_gtk::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y) {
//    int gc = 0;
//    XCopyArea(get_common()->get_display(), get_common()->get_window_id(), get_common()->get_window_id(), get_common()->get_GC(gc),
//              AW_INT(src_x), AW_INT(src_y), AW_INT(width), AW_INT(height),
//              AW_INT(dest_x), AW_INT(dest_y));
//    AUTO_FLUSH(this);
    GTK_NOT_IMPLEMENTED;
}
