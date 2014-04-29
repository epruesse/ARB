// =============================================================== //
//                                                                 //
//   File      : AW_Xm.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <aw_Xm.hxx>

#if defined(WARN_TODO)
#warning change implementation of draw functions (read more)
// * filter has to be checked early (in AW_device)
// * functions shall use Position/LineVector/Rectangle only
#endif

using namespace AW;

// ---------------------
//      AW_device_Xm

AW_DEVICE_TYPE AW_device_Xm::type() { return AW_DEVICE_SCREEN; }

#define XDRAW_PARAM2(common)    (common)->get_display(), (common)->get_window_id()
#define XDRAW_PARAM3(common,gc) XDRAW_PARAM2(common), (common)->get_GC(gc)

bool AW_device_Xm::line_impl(int gc, const LineVector& Line, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        LineVector transLine = transform(Line);
        LineVector clippedLine;
        drawflag = clip(transLine, clippedLine);
        if (drawflag) {
            XDrawLine(XDRAW_PARAM3(get_common(), gc),
                      AW_INT(clippedLine.start().xpos()), AW_INT(clippedLine.start().ypos()),
                      AW_INT(clippedLine.head().xpos()), AW_INT(clippedLine.head().ypos()));
            AUTO_FLUSH(this);
        }
    }

    return drawflag;
}

static bool AW_draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t start, size_t size,
                                    AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/)
{
    AW_pos X, Y;
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));
    AW_device_Xm *device_xm = DOWNCAST(AW_device_Xm*, device);
    XDrawString(XDRAW_PARAM3(device_xm->get_common(), gc), AW_INT(X), AW_INT(Y), str + start,  (int)size);
    AUTO_FLUSH(device);
    return true;
}


bool AW_device_Xm::text_impl(int gc, const char *str, const Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, (AW_CL)this, 0.0, 0.0, AW_draw_string_on_screen);
}

bool AW_device_Xm::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        if (filled) {
            Rectangle transRect = transform(rect);
            Rectangle clippedRect;
            drawflag = box_clip(transRect, clippedRect);
            if (drawflag) {
                XFillRectangle(XDRAW_PARAM3(get_common(), gc),
                               AW_INT(clippedRect.left()),
                               AW_INT(clippedRect.top()),
                               AW_INT(clippedRect.width())+1, // see aw_device.hxx@WORLD_vs_PIXEL
                               AW_INT(clippedRect.height())+1);
                AUTO_FLUSH(this);
            }
        }
        else {
            drawflag = generic_box(gc, false, rect, filteri);
        }
    }
    return drawflag;
}

bool AW_device_Xm::circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) {
    aw_assert(radius.x()>0 && radius.y()>0);
    return arc_impl(gc, filled, center, radius, 0, 360, filteri);
}

bool AW_device_Xm::arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) {
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

            if (!filled) {
                XDrawArc(XDRAW_PARAM3(get_common(), gc), xl, yl, width, height, 64*start_degrees, 64*arc_degrees);
            }
            else {
                XFillArc(XDRAW_PARAM3(get_common(), gc), xl, yl, width, height, 64*start_degrees, 64*arc_degrees);
            }
            AUTO_FLUSH(this);
        }
    }
    return drawflag;
}

void AW_device_Xm::clear(AW_bitset filteri) {
    if (filteri & filter) {
        XClearWindow(XDRAW_PARAM2(get_common()));
        AUTO_FLUSH(this);
    }
}

void AW_device_Xm::clear_part(const Rectangle& rect, AW_bitset filteri) {
    if (filteri & filter) {
        Rectangle transRect = transform(rect);
        Rectangle clippedRect;
        bool drawflag = box_clip(transRect, clippedRect);
        if (drawflag) {
            XClearArea(XDRAW_PARAM2(get_common()),
                       AW_INT(clippedRect.left()),
                       AW_INT(clippedRect.top()),
                       AW_INT(clippedRect.width())+1, // see aw_device.hxx@WORLD_vs_PIXEL
                       AW_INT(clippedRect.height())+1,
                       False);
            AUTO_FLUSH(this);
        }
    }
}

void AW_device_Xm::flush() {
    XFlush(get_common()->get_display());
}

void AW_device_Xm::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y) {
    int gc = 0;
    XCopyArea(get_common()->get_display(), get_common()->get_window_id(), get_common()->get_window_id(), get_common()->get_GC(gc),
              AW_INT(src_x), AW_INT(src_y), AW_INT(width), AW_INT(height),
              AW_INT(dest_x), AW_INT(dest_y));
    AUTO_FLUSH(this);
}
