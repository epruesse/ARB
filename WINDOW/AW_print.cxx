// =============================================================== //
//                                                                 //
//   File      : AW_print.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_root.hxx"
#include "aw_common.hxx"

#include <arb_msg.h>

using namespace AW;

const double dpi_screen2printer = double(DPI_PRINTER)/DPI_SCREEN;

inline double screen2printer(double val) { return val*dpi_screen2printer; }
inline int print_pos(AW_pos screen_pos) { return AW_INT(screen2printer(screen_pos)); }

AW_DEVICE_TYPE AW_device_print::type() { return AW_DEVICE_PRINTER; }

bool AW_device_print::line_impl(int gc, const LineVector& Line, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        LineVector transLine = transform(Line);
        LineVector clippedLine;
        drawflag = clip(transLine, clippedLine);

        if (drawflag) {
            const AW_GC *gcm        = get_common()->map_gc(gc);
            int          line_width = gcm->get_line_width();

            int    line_mode = 0;
            double gap_ratio = 0.0;
            switch (gcm->get_line_style()) {
                case AW_SOLID: /* use defaults from above*/ break;
                case AW_DASHED:  line_mode = 1; gap_ratio = 4.0; break;
                case AW_DOTTED:  line_mode = 2; gap_ratio = 2.0; break;
            }

            aw_assert(out);     // file has to be good!

            // type, subtype, style, thickness, pen_color,
            // fill_color(new), depth, pen_style, area_fill, style_val,
            // join_style(new), cap_style(new), radius, forward_arrow,
            // backward_arrow, npoints
            fprintf(out, "2 1 %d %d %d 0 0 0 0 %5.3f 0 1 0 0 0 2\n\t%d %d %d %d\n",
                    line_mode,
                    AW_INT(line_width),
                    find_color_idx(gcm->get_last_fg_color()),
                    gap_ratio, 
                    print_pos(clippedLine.xpos()),
                    print_pos(clippedLine.ypos()),
                    print_pos(clippedLine.head().xpos()),
                    print_pos(clippedLine.head().ypos()));
        }
    }
    return drawflag;
}

bool AW_device_print::invisible_impl(const AW::Position& pos, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        Position trans = transform(pos);

        drawflag = !is_outside_clip(trans);
        if (drawflag) {
            fprintf(out, "2 1 0 1 7 7 50 -1 -1 0.000 0 0 -1 0 0 1\n\t%d %d\n",
                    print_pos(trans.xpos()),
                    print_pos(trans.ypos()));
        }
    }
    return drawflag;
}

static bool AW_draw_string_on_printer(AW_device *devicei, int gc, const char *str, size_t /* opt_strlen */, size_t start, size_t size,
                                      AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/,
                                      AW_CL /*cduser*/)
{
    AW_pos           X, Y;
    AW_device_print *device = (AW_device_print *)devicei;
    AW_common       *common = device->get_common();
    const AW_GC     *gcm    = common->map_gc(gc);

    device->transform(x, y, X, Y);
    char *pstr = strdup(str+start);
    if (size < strlen(pstr)) pstr[size] = 0;
    else size  = strlen(pstr);
    size_t i;
    for (i=0; i<size; i++) {
        if (pstr[i] < ' ') pstr[i] = '?';
    }
    int fontnr = AW_font_2_xfig(gcm->get_fontnr());
    if (fontnr<0) fontnr = - fontnr;
    if (str[0]) {
        // 4=string 0=left color depth penstyle font font_size angle
        // font_flags height length x y string
        // (font/fontsize and color/depth have been switched from format
        // 2.1 to 3.2
        fprintf(device->get_FILE(), "4 0 %d 0 0 %d %d 0.000 4 %d %d %d %d ",
                device->find_color_idx(gcm->get_last_fg_color()),
                fontnr,
                gcm->get_fontsize(),
                (int)gcm->get_font_limits().height,
                device->get_string_size(gc, str, 0),
                print_pos(X),
                print_pos(Y));
        char *p;
        for (p = pstr; *p; p++) {
            if (*p >= 32) putc(*p, device->get_FILE());
        }
        fprintf(device->get_FILE(), "\\001\n");
    }
    free(pstr);
    return true;
}

#define DATA_COLOR_OFFSET 32

GB_ERROR AW_device_print::open(const char *path) {
    if (out) return "You cannot reopen a device";

    out = fopen(path, "w");
    if (!out) return GB_IO_error("writing", path);

    fprintf(out,
            "#FIG 3.2\n"   // version
            "Landscape\n"  // "Portrait"
            "Center\n"     // "Flush Left"
            "Metric\n"     // "Inches"
            "A4\n"         // papersize
            "100.0\n"      // export&print magnification %
            "Single\n"     // Single/Multiple Pages
            "-3\n"         // background = transparent for gif export
            "%i 2\n"       // dpi, 2  = origin in upper left corner
            , DPI_PRINTER);

    if (color_mode) {
        for (int i=0; i<get_common()->get_data_color_size(); i++) {
            unsigned long col = get_common()->get_data_color(i);
            if (col != (unsigned long)AW_NO_COLOR) fprintf(out, "0 %d #%06lx\n", i+DATA_COLOR_OFFSET, col);
        }
    }

    return 0;
}

int AW_common::find_data_color_idx(AW_rgb color) const {
    for (int i=0; i<data_colors_size; i++) {
        if (color == data_colors[i]) {
            return i;
        }
    }
    return -1;
}

int AW_device_print::find_color_idx(AW_rgb color) {
    int idx = -1;
    if (color_mode) {
        idx = get_common()->find_data_color_idx(color);
        if (idx >= 0) idx += DATA_COLOR_OFFSET;
    }
    return idx;
}

void AW_device_print::set_color_mode(bool mode) {
    color_mode=mode;
}

void AW_device_print::close() {
    if (out) fclose(out);
    out = 0;
}


bool AW_device_print::text_impl(int gc, const char *str, const Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, (AW_CL)this, 0.0, 0.0, AW_draw_string_on_printer);
}

bool AW_device_print::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
    bool drawflag = false;
    if (filter & filteri) {
        if (filled) {
            Position q[4];
            q[0] = rect.upper_left_corner();
            q[1] = rect.upper_right_corner();
            q[2] = rect.lower_right_corner();
            q[3] = rect.lower_left_corner();

            drawflag = filled_area(gc, 4, q, filteri);
        }
        else {
            drawflag = generic_box(gc, false, rect, filteri);
        }
    }
    return drawflag;
}

bool AW_device_print::circle_impl(int gc, bool filled, const Position& center, const AW::Vector& radius, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        aw_assert(radius.x()>0 && radius.y()>0);
        Rectangle Box(center-radius, center+radius);
        Rectangle screen_box = transform(Box);
        Rectangle clipped_box;
        drawflag          = box_clip(screen_box, clipped_box);
        bool half_visible = (clipped_box.surface()*2) > screen_box.surface();

        drawflag = drawflag && half_visible;
        // @@@ correct behavior would be to draw an arc if only partly visible

        if (drawflag) {
            const AW_GC *gcm = get_common()->map_gc(gc);

            // force into clipped_box (hack):
            Position Center        = clipped_box.centroid();
            Vector   screen_radius = clipped_box.diagonal()/2;

            int cx = print_pos(Center.xpos());
            int cy = print_pos(Center.ypos());
            int rx = print_pos(screen_radius.x());
            int ry = print_pos(screen_radius.y());

            {
                int subtype = (rx == ry) ? 3 : 1; // 3(circle) 1(ellipse)
                subtype     = 3; // @@@ remove after refactoring
                fprintf(out, "1 %d  ", subtype);  // type + subtype:
            }

            {
                int colorIdx = find_color_idx(gcm->get_last_fg_color());
                int fill_color, area_fill;
                if (filled) {
                    fill_color = colorIdx;
                    area_fill  = AW_INT(20+20*gcm->get_grey_level());    // 20 = full saturation; 40 = white;
                }
                else {
                    fill_color = area_fill = -1;
                }
                int line_width = gcm->get_line_width();

                fprintf(out, "%d %d ", AW_SOLID, line_width);   // line_style + line_width
                fprintf(out, "%d %d 0 ", colorIdx, fill_color); // pen_color + fill_color + depth
                fprintf(out, "0 %d ", area_fill);               // pen_style + area_fill (20 = full color, 40 = white)
                fputs("0.000 1 0.0000 ", out);                  // style_val + direction + angle (x-axis)
            }

            fprintf(out, "%d %d ", cx, cy); // center
            fprintf(out, "%d %d ", rx, ry); // radius
            fprintf(out, "%d %d ", cx, cy); // start 
            fprintf(out, "%d %d\n", print_pos(Center.xpos()+screen_radius.x()), cy); // end 
        }
    }
    return drawflag;
}

bool AW_device_print::arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri && filter) {
        aw_assert(radius.x()>0 && radius.y()>0);
        Rectangle Box(center-radius, center+radius);
        Rectangle screen_box = transform(Box);
        Rectangle clipped_box;
        drawflag          = box_clip(screen_box, clipped_box);
        bool half_visible = (clipped_box.surface()*2) > screen_box.surface();

        drawflag = drawflag && half_visible;
        // @@@ correct behavior would be to draw an arc if only partly visible

        if (drawflag) {
            const AW_GC *gcm = get_common()->map_gc(gc);

            // force into clipped_box (hack):
            Position Center        = clipped_box.centroid();
            Vector   screen_radius = clipped_box.diagonal()/2;

            int cx = print_pos(Center.xpos());
            int cy = print_pos(Center.ypos());
            int rx = print_pos(screen_radius.x());
            int ry = print_pos(screen_radius.y());

            bool use_spline = (rx != ry); // draw interpolated spline for ellipsoid arcs

            // fputs(use_spline ? "3 2 " : "5 1 ", out);  // type + subtype:
            fputs(use_spline ? "3 4 " : "5 1 ", out);  // type + subtype:

            {
                int colorIdx = find_color_idx(gcm->get_last_fg_color());
                int fill_color, area_fill;

                if (filled) {
                    fill_color = colorIdx;
                    area_fill  = AW_INT(20+20*gcm->get_grey_level());    // 20 = full saturation; 40 = white;
                }
                else {
                    fill_color = area_fill = -1;
                }
                int line_width = gcm->get_line_width();

                fprintf(out, "%d %d ", AW_SOLID, line_width);   // line_style + line_width
                fprintf(out, "%d %d 0 ", colorIdx, fill_color); // pen_color + fill_color + depth
                fprintf(out, "0 %d ", area_fill);               // pen_style + area_fill (20 = full color, 40 = white)
                fputs("0.000 1 ", out);                         // style_val + cap_style
                if (!use_spline) fputs("1 ", out);              // direction
                fputs("0 0 ", out);                             // 2 arrows
            }

            Angle a0(Angle::deg2rad*start_degrees);
            Angle a1(Angle::deg2rad*(start_degrees+arc_degrees));

            if (use_spline) {
                const int MAX_ANGLE_STEP = 45; // degrees

                int steps = (abs(arc_degrees)-1)/MAX_ANGLE_STEP+1;
                Angle step(Angle::deg2rad*double(arc_degrees)/steps);

                fprintf(out, "%d\n\t", steps+1); // npoints

                double rmax, x_factor, y_factor;
                if (rx>ry) {
                    rmax     = rx;
                    x_factor = 1.0;
                    y_factor = double(ry)/rx;
                }
                else {
                    rmax     = ry;
                    x_factor = double(rx)/ry;
                    y_factor = 1.0;
                }

                for (int n = 0; n <= steps; ++n) {
                    Vector   toCircle  = a0.normal()*rmax;
                    Vector   toEllipse(toCircle.x()*x_factor, toCircle.y()*y_factor);
                    Position onEllipse = Center+toEllipse;

                    int x = print_pos(onEllipse.xpos());
                    int y = print_pos(onEllipse.ypos());

                    fprintf(out, " %d %d", x, y);

                    if (n<steps) {
                        if (n == steps-1) a0 = a1;
                        else              a0 += step;
                    }
                }
                fputs("\n\t", out);
                for (int n = 0; n <= steps; ++n) {
                    // -1 = interpolate; 0 = discontinuity; 1 = approximate
                    fprintf(out, " %d", (n == 0 || n == steps) ? 0 : -1);
                }
                fputc('\n', out);
            }
            else {
                fprintf(out, "%d %d ", cx, cy); // center

                Angle am(Angle::deg2rad*(start_degrees+arc_degrees*0.5));

                double   r  = screen_radius.x();
                Position p0 = Center+a0.normal()*r;
                Position pm = Center+am.normal()*r;
                Position p1 = Center+a1.normal()*r;

                fprintf(out, "%d %d ",  print_pos(p0.xpos()), print_pos(p0.ypos()));
                fprintf(out, "%d %d ",  print_pos(pm.xpos()), print_pos(pm.ypos()));
                fprintf(out, "%d %d\n", print_pos(p1.xpos()), print_pos(p1.ypos()));
            }
        }
    }
    return drawflag;
}

bool AW_device_print::filled_area_impl(int gc, int npos, const Position *pos, AW_bitset filteri) {
    bool drawflag = false;
    if (filter & filteri) {
        drawflag = generic_filled_area(gc, npos, pos, filteri);
        if (drawflag) { // line visible -> area fill needed
            const AW_GC *gcm = get_common()->map_gc(gc);

            short greylevel             = (short)(gcm->get_grey_level()*22);
            if (greylevel>21) greylevel = 21;

            int line_width = gcm->get_line_width();

            fprintf(out, "2 3 0 %d %d -1 0 0 %d 0.000 0 0 -1 0 0 %d\n",
                    line_width, find_color_idx(gcm->get_last_fg_color()), greylevel, npos+1);

            // @@@ method used here for clipping leads to wrong results,
            // since group border (drawn by generic_filled_area() above) is clipped correctly,
            // but filled content is clipped different.
            //
            // fix: clip the whole polygon before drawing border

            for (int i=0; i <= npos; i++) {
                int j = i == npos ? 0 : i; // print pos[0] after pos[n-1]

                Position transPos = transform(pos[j]);
                Position clippedPos;
                ASSERT_RESULT(bool, true, force_into_clipbox(transPos, clippedPos)); 
                fprintf(out, "   %d %d\n", print_pos(clippedPos.xpos()), print_pos(clippedPos.ypos()));
            }
        }
    }
    return drawflag;
}
