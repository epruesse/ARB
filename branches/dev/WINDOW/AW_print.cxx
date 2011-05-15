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

AW_DEVICE_TYPE AW_device_print::type() { return AW_DEVICE_PRINTER; }

int AW_device_print::line_impl(int gc, const LineVector& Line, AW_bitset filteri) {
    int drawflag = 0;
    if (filteri & filter) {
        LineVector transLine = transform(Line);
        LineVector clippedLine;
        drawflag = clip(transLine, clippedLine);

        if (drawflag) {
            const AW_GC *gcm        = get_common()->map_gc(gc);
            int          line_width = gcm->get_line_width();

            aw_assert(out);     // file has to be good!

            // type, subtype, style, thickness, pen_color,
            // fill_color(new), depth, pen_style, area_fill, style_val,
            // join_style(new), cap_style(new), radius, forward_arrow,
            // backward_arrow, npoints
            fprintf(out, "2 1 0 %d %d 0 0 0 0 0.000 0 1 0 0 0 2\n\t%d %d %d %d\n",
                    AW_INT(line_width), find_color_idx(gcm->get_last_fg_color()),
                    AW_INT(clippedLine.xpos()),
                    AW_INT(clippedLine.ypos()),
                    AW_INT(clippedLine.head().xpos()),
                    AW_INT(clippedLine.head().ypos()));
        }
    }
    return drawflag;
}

static int AW_draw_string_on_printer(AW_device *devicei, int gc, const char *str, size_t /* opt_strlen */, size_t start, size_t size,
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
                AW_INT(X), AW_INT(Y));
        char *p;
        for (p = pstr; *p; p++) {
            if (*p >= 32) putc(*p, device->get_FILE());
        }
        fprintf(device->get_FILE(), "\\001\n");
    }
    free(pstr);
    return 1;
}

#define DATA_COLOR_OFFSET 32

GB_ERROR AW_device_print::open(const char *path) {
    if (out) return "You cannot reopen a device";

    out = fopen(path, "w");
    if (!out) return GB_IO_error("writing", path);

    fputs("#FIG 3.2\n"   // version
          "Landscape\n"  // "Portrait"
          "Center\n"     // "Flush Left"
          "Metric\n"     // "Inches"
          "A4\n"         // papersize
          "100.0\n"      // export&print magnification %
          "Single\n"     // Single/Multiple Pages
          "-3\n"         // background = transparent for gif export
          "80 2\n"       // 80 dpi, 2  = origin in upper left corner
          , out);

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


int AW_device_print::text_impl(int gc, const char *str, const Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, (AW_CL)this, 0.0, 0.0, AW_draw_string_on_printer);
}

int AW_device_print::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
    int    drawflag;
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
    return drawflag;
}

int AW_device_print::circle_impl(int gc, bool filled, const Position& center, const AW::Vector& radius, AW_bitset filteri) {
    if (filteri & filter) {
        aw_assert(radius.x()>0 && radius.y()>0);
        Rectangle Box(center-radius, center+radius);
        Rectangle screen_box = transform(Box);
        Rectangle clipped_box;
        bool      drawflag   = box_clip(screen_box, clipped_box);

        if (drawflag && (clipped_box.surface()*2) > screen_box.surface()) { // clip away if half of circle is visible
            // @@@ correct behavior would be to draw an arc

            // Don't know how to use greylevel --ralf
            // short greylevel             = (short)(gcm->grey_level*22);
            // if (greylevel>21) greylevel = 21;

            const AW_GC *gcm        = get_common()->map_gc(gc);
            int          line_width = gcm->get_line_width();
            int          colorIdx   = find_color_idx(gcm->get_last_fg_color());

            Position Center        = clipped_box.centroid();
            Vector   screen_radius = clipped_box.diagonal()/2;

            int cx = AW_INT(Center.xpos());
            int cy = AW_INT(Center.ypos());

            // 1, 3, 0?, line_width?, pencolor, fill_color, 0?, 0?, fill_style(-1 = none, 20 = filled),
            // ?, ?, ?, coordinates+size (8 entries)
            fprintf(out, "1 3  0 %d %d %d 0 0 %d 0.000 1 0.0000 %d %d %d %d %d %d %d %d\n",
                    line_width,
                    colorIdx, // before greylevel has been used here
                    filled ? colorIdx : -1,
                    filled ? 20 : -1,
                    cx, cy, 
                    AW_INT(screen_radius.x()), AW_INT(screen_radius.y()),
                    cx, cy,
                    AW_INT(Center.xpos()+screen_radius.x()), cy);
        }
    }
    return 0;
}

int AW_device_print::filled_area_impl(int gc, int npos, const Position *pos, AW_bitset filteri) {
    if (!(filteri & this->filter)) return 0;

    int erg = generic_filled_area(gc, npos, pos, filteri);
    if (!erg) return 0;                         // no line visible -> no area fill needed

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
        ASSERT_RESULT(int, 1, force_into_clipbox(transPos, clippedPos)); 
        fprintf(out, "   %d %d\n", AW_INT(clippedPos.xpos()), AW_INT(clippedPos.ypos()));
    }
    return 1;
}
