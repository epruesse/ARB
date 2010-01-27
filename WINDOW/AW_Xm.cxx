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

#if defined(DEVEL_RALF)
#warning change implementation of draw functions (read more)
// * cd1 and cd2 shall not be passed to real draw functions (only to click device)
// * filter has to be checked early (in AW_device)
// * functions shall use Position/LineVector/Rectangle only
// way to do :
// AW_device-methods
// * expect parameters 'AW_bitset filteri, AW_CL cd1, AW_CL cd2' (as in the past)
// * they are really implemented, check the filter, save cd's into AW_device and
//   call virtual private methods w/o above parameters
#endif // DEVEL_RALF


// ---------------------
//      AW_device_Xm

AW_device_Xm::AW_device_Xm(AW_common *commoni) : AW_device(commoni) {
;
}

void AW_device_Xm::init() {
;
}


AW_DEVICE_TYPE AW_device_Xm::type() { return AW_DEVICE_SCREEN; }


int AW_device_Xm::line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    class AW_GC_Xm *gcm      = AW_MAP_GC(gc);
    AW_pos          X0, Y0, X1, Y1; // Transformed pos
    AW_pos          CX0, CY0, CX1, CY1; // Clipped line
    int             drawflag = 0;

    if (filteri & filter) {
        this->transform(x0, y0, X0, Y0);
        this->transform(x1, y1, X1, Y1);
        drawflag = this->clip(X0, Y0, X1, Y1, CX0, CY0, CX1, CY1);
        if (drawflag) {
            AWUSE(cd1);
            AWUSE(filter);
            AWUSE(cd2);

            XDrawLine(common->display, common->window_id,
                      gcm->gc, AW_INT(CX0), AW_INT(CY0), AW_INT(CX1), AW_INT(CY1));

            AUTO_FLUSH(this);
        }
    }

    return drawflag;
}

int AW_draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t start, size_t size,
                             AW_pos x, AW_pos y, AW_pos opt_ascent, AW_pos opt_descent,
                             AW_CL cduser, AW_CL cd1, AW_CL cd2)
{
    AW_pos X, Y;
    AWUSE(cd1); AWUSE(cd2); AWUSE(opt_ascent); AWUSE(opt_descent); AWUSE(cduser);
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));
    XDrawString(device->common->display, device->common->window_id, device->common->gcs[gc]->gc,
                AW_INT(X), AW_INT(Y), str + start,  (int)size);

    AUTO_FLUSH(device);

    return 1;
}


int AW_device_Xm::text(int gc, const char *str, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2, long opt_strlen) {
    return text_overlay(gc, str, opt_strlen, x, y, alignment, filteri, (AW_CL)this, cd1, cd2, 0.0, 0.0, AW_draw_string_on_screen);
}

int AW_device_Xm::box(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd1); AWUSE(cd2);
    class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos x1, y1;
    AW_pos X0, Y0, X1, Y1;                          // Transformed pos
    AW_pos CX0, CY0, CX1, CY1;                      // Clipped line
    int    drawflag = 0;

    if (filteri & filter) {
        x1 = x0 + width;
        y1 = y0 + height;
        this->transform(x0, y0, X0, Y0);
        this->transform(x1, y1, X1, Y1);

        if (filled) {
            drawflag = this->box_clip(X0, Y0, X1, Y1, CX0, CY0, CX1, CY1);
            if (drawflag) {
                AWUSE(cd1);
                AWUSE(cd2);

                int cx0 = AW_INT(CX0);
                int cx1 = AW_INT(CX1);
                int cy0 = AW_INT(CY0);
                int cy1 = AW_INT(CY1);

                XFillRectangle(common->display, common->window_id, gcm->gc,
                               cx0, cy0, cx1-cx0+1, cy1-cy0+1);
                AUTO_FLUSH(this);
            }
        }
        else {
            line(gc, x0, y0, x1, y0, filteri, cd1, cd2);
            line(gc, x0, y0, x0, y1, filteri, cd1, cd2);
            line(gc, x0, y1, x1, y1, filteri, cd1, cd2);
            line(gc, x1, y0, x1, y1, filteri, cd1, cd2);
        }
    }

    return 0;
}

int AW_device_Xm::circle(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    return arc(gc, filled, x0, y0, width, height, 0, 360, filteri, cd1, cd2);
}

int AW_device_Xm::arc(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, int start_degrees, int arc_degrees, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd1); AWUSE(cd2);
    class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos X0, Y0, X1, Y1;                          // Transformed pos
    AW_pos XL, YL;                                  // Left edge of circle pos
    AW_pos CX0, CY0, CX1, CY1;                      // Clipped line
    int    drawflag = 0;

    if (filteri & filter) {

        this->transform(x0, y0, X0, Y0); // center

        x0 -= width;
        y0 -= height;
        this->transform(x0, y0, XL, YL);

        X1 = X0 + 2.0; Y1 = Y0 + 2.0;
        X0 -= 2.0; Y0 -= 2.0;
        drawflag = this->box_clip(X0, Y0, X1, Y1, CX0, CY0, CX1, CY1);
        if (drawflag) {
            AWUSE(cd1);
            AWUSE(cd2);
            width  *= 2.0 * this->get_scale();
            height *= 2.0 * this->get_scale();

            start_degrees = -start_degrees;
            while (start_degrees<0) start_degrees += 360;

            if (!filled) {
                XDrawArc(common->display, common->window_id, gcm->gc, AW_INT(XL), AW_INT(YL), AW_INT(width), AW_INT(height), 64*start_degrees, 64*arc_degrees);
            }
            else {
                XFillArc(common->display, common->window_id, gcm->gc, AW_INT(XL), AW_INT(YL), AW_INT(width), AW_INT(height), 64*start_degrees, 64*arc_degrees);
            }
            AUTO_FLUSH(this);
        }
    }

    return 0;
}

void AW_device_Xm::clear(AW_bitset filteri) {
    if (filteri & filter) {
        XClearWindow(common->display, common->window_id);
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

            XClearArea(common->display, common->window_id, cx0, cy0, cx1-cx0+1, cy1-cy0+1, False);
            AUTO_FLUSH(this);
        }
    }
}


void AW_device_Xm::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(filteri); AWUSE(cd1); AWUSE(cd2);
    class AW_GC_Xm *gcm     = AW_MAP_GC(gc);
    XFontStruct    *xfs     = &gcm->curfont;
    AW_pos          X, Y;       // Transformed pos
    AW_pos          width, height;
    long            textlen = strlen(string);

    this->transform(x, y, X, Y);
    width  = get_string_size(gc, string, textlen);
    height = xfs->max_bounds.ascent + xfs->max_bounds.descent;
    X      = common->x_alignment(X, width, alignment);

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

    XClearArea(common->display, common->window_id,
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
    XFlush(common->display);
}


void AW_device_Xm::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y) {
    int             gc  = 0;
    class AW_GC_Xm *gcm = AW_MAP_GC(gc);

    XCopyArea(common->display, common->window_id, common->window_id, gcm->gc,
               AW_INT(src_x), AW_INT(src_y), AW_INT(width), AW_INT(height),
               AW_INT(dest_x), AW_INT(dest_y));
    AUTO_FLUSH(this);
}
