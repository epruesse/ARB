// =============================================================== //
//                                                                 //
//   File      : AW_Xm.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_commn.hxx"
#include <aw_Xm.hxx>

#if defined(WARN_TODO)
#warning change implementation of draw functions (read more)
// * filter has to be checked early (in AW_device)
// * functions shall use Position/LineVector/Rectangle only
#endif

using namespace AW;

// ---------------------
//      AW_device_Xm

AW_device_Xm::AW_device_Xm(AW_common *commoni) : AW_device(commoni) {
;
}

void AW_device_Xm::init() {
;
}


AW_DEVICE_TYPE AW_device_Xm::type() { return AW_DEVICE_SCREEN; }

#define XDRAW_PARAM2(common)    (common)->get_display(), (common)->get_window_id()
#define XDRAW_PARAM3(common,gc) XDRAW_PARAM2(common), (common)->get_GC(gc)

int AW_device_Xm::line_impl(int gc, const LineVector& Line, AW_bitset filteri) {
    int drawflag = 0;
    if (filteri & filter) {
        LineVector transLine = transform(Line);
        LineVector clippedLine;
        drawflag = clip(transLine, clippedLine);
        if (drawflag) {
            XDrawLine(XDRAW_PARAM3(common, gc), 
                      AW_INT(clippedLine.start().xpos()), AW_INT(clippedLine.start().ypos()),
                      AW_INT(clippedLine.head().xpos()), AW_INT(clippedLine.head().ypos()));
            AUTO_FLUSH(this);
        }
    }

    return drawflag;
}

static int AW_draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t start, size_t size,
                                    AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/)
{
    AW_pos     X, Y;
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));
    AW_common *common = device->common;
    XDrawString(XDRAW_PARAM3(common, gc), AW_INT(X), AW_INT(Y), str + start,  (int)size);
    AUTO_FLUSH(device);

    return 1;
}


int AW_device_Xm::text_impl(int gc, const char *str, const Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, (AW_CL)this, 0.0, 0.0, AW_draw_string_on_screen);
}

int AW_device_Xm::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
    int drawflag = 0;
    if (filteri & filter) {
        if (filled) {
            Rectangle transRect = transform(rect);
            Rectangle clippedRect;
            drawflag = box_clip(transRect, clippedRect);
            if (drawflag) {
                XFillRectangle(XDRAW_PARAM3(common, gc), 
                               AW_INT(clippedRect.left()), 
                               AW_INT(clippedRect.top()), 
                               AW_INT(clippedRect.width()), 
                               AW_INT(clippedRect.height()) 
                               );
                AUTO_FLUSH(this);
            }
        }
        else {
            drawflag = generic_box(gc, false, rect, filteri);
        }
    }

    return drawflag;
}

int AW_device_Xm::circle_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri) {
    return arc(gc, filled, center.xpos(), center.ypos(), xradius, yradius, 0, 360, filteri);
}

int AW_device_Xm::arc_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri) {
    AW_pos X0, Y0, X1, Y1;                          // Transformed pos
    AW_pos XL, YL;                                  // Left edge of circle pos
    AW_pos CX0, CY0, CX1, CY1;                      // Clipped line
    int    drawflag = 0;

    if (filteri & filter) {
        AW_pos x0 = center.xpos();
        AW_pos y0 = center.ypos();

        this->transform(x0, y0, X0, Y0); // center

        x0 -= xradius;
        y0 -= yradius;
        this->transform(x0, y0, XL, YL);

        X1 = X0 + 2.0; Y1 = Y0 + 2.0;
        X0 -= 2.0; Y0 -= 2.0;
        drawflag = this->box_clip(X0, Y0, X1, Y1, CX0, CY0, CX1, CY1);
        if (drawflag) {
            AW_pos width  = xradius*2.0 * this->get_scale();
            AW_pos height = yradius*2.0 * this->get_scale();

            start_degrees = -start_degrees;
            while (start_degrees<0) start_degrees += 360;

            if (!filled) {
                XDrawArc(XDRAW_PARAM3(common, gc), AW_INT(XL), AW_INT(YL), AW_INT(width), AW_INT(height), 64*start_degrees, 64*arc_degrees);
            }
            else {
                XFillArc(XDRAW_PARAM3(common, gc), AW_INT(XL), AW_INT(YL), AW_INT(width), AW_INT(height), 64*start_degrees, 64*arc_degrees);
            }
            AUTO_FLUSH(this);
        }
    }

    return 0;
}

void AW_device_Xm::clear(AW_bitset filteri) {
    if (filteri & filter) {
        XClearWindow(XDRAW_PARAM2(common));
        AUTO_FLUSH(this);
    }
}

void AW_device_Xm::clear_part(AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, AW_bitset filteri)
{
    if (filteri & filter) {
        AW_pos x1 = x0+width;
        AW_pos y1 = y0+height;

        AW_pos X0, Y0, X1, Y1;  // Transformed pos
        this->transform(x0, y0, X0, Y0);
        this->transform(x1, y1, X1, Y1);

        AW_pos CX0, CY0, CX1, CY1; // Clipped line
        int    drawflag = this->box_clip(X0, Y0, X1, Y1, CX0, CY0, CX1, CY1);
        if (drawflag) {
            int cx0 = AW_INT(CX0);
            int cx1 = AW_INT(CX1);
            int cy0 = AW_INT(CY0);
            int cy1 = AW_INT(CY1);

            XClearArea(XDRAW_PARAM2(common), cx0, cy0, cx1-cx0+1, cy1-cy0+1, False);
            AUTO_FLUSH(this);
        }
    }
}


void AW_device_Xm::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset /*filteri*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    const AW_GC_Xm    *gcm     = common->map_gc(gc);
    const XFontStruct *xfs     = &gcm->curfont;
    AW_pos             X, Y;    // Transformed pos
    AW_pos             width, height;
    long               textlen = strlen(string);

    this->transform(x, y, X, Y);
    width  = get_string_size(gc, string, textlen);
    height = xfs->max_bounds.ascent + xfs->max_bounds.descent;
    X      = x_alignment(X, width, alignment);

    if (X > this->clip_rect.r) return;
    if (X < this->clip_rect.l) {
        width = width + X - this->clip_rect.l;
        X = this->clip_rect.l;
    }

    if (X + width > this->clip_rect.r) {
        width = this->clip_rect.r - X;
    }

    if (Y < this->clip_rect.t) return;
    if (Y > this->clip_rect.b) return;
    if (width <= 0 || height <= 0) return;

    XClearArea(XDRAW_PARAM2(common),
               AW_INT(X), AW_INT(Y)-AW_INT(xfs->max_bounds.ascent), AW_INT(width), AW_INT(height), False);

    AUTO_FLUSH(this);
}

void AW_device_Xm::fast() {
    fastflag = 1;
}

void AW_device_Xm::slow() {
    fastflag = 0;
}

void AW_device_Xm::flush() {
    XFlush(common->get_display());
}

void AW_device_Xm::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y) {
    int gc = 0;
    XCopyArea(common->get_display(), common->get_window_id(), common->get_window_id(), common->get_GC(gc),
              AW_INT(src_x), AW_INT(src_y), AW_INT(width), AW_INT(height),
              AW_INT(dest_x), AW_INT(dest_y));
    AUTO_FLUSH(this);
}
