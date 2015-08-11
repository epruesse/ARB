// =============================================================== //
//                                                                 //
//   File      : AW_device_click.cxx                               //
//   Purpose   : Detect which graphical element is "nearby"        //
//               a given mouse position                            //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_device_click.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_common.hxx"
#include <algorithm>

using namespace AW;

// ------------------------
//      AW_device_click

AW_device_click::AW_device_click(AW_common *common_)
        : AW_simple_device(common_) {
    init_click(0, 0, AWT_NO_CATCH, AW_ALL_DEVICES);
}

void AW_device_click::init_click(AW_pos mousex, AW_pos mousey, int max_distance, AW_bitset filteri) {
    mouse_x = mousex;
    mouse_y = mousey;

    filter = filteri;

    max_distance_line = max_distance;
    max_distance_text = max_distance;

    opt_line = AW_clicked_line();
    opt_text = AW_clicked_text();
}

AW_DEVICE_TYPE AW_device_click::type() {
    return AW_DEVICE_CLICK;
}

bool AW_device_click::line_impl(int /*gc*/, const AW::LineVector& Line, AW_bitset /*filteri*/) {
    LineVector transLine = transform(Line);
    LineVector clippedLine;
    bool       drawflag  = clip(transLine, clippedLine);
    if (drawflag) {
        Position mouse(mouse_x, mouse_y);
        double   nearest_rel_pos;
        Position nearest  = nearest_linepoint(mouse, clippedLine, nearest_rel_pos);
        double   distance = Distance(mouse, nearest);

        if (distance < max_distance_line) {
            max_distance_line = distance;
            opt_line.assign(Line, distance, nearest_rel_pos, click_cd);
        }
    }
    return drawflag;
}


bool AW_device_click::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset /*filteri*/, long opt_strlen) {
    AW_pos X0, Y0;          // Transformed pos
    this->transform(pos.xpos(), pos.ypos(), X0, Y0);

    const AW_GC           *gcm         = get_common()->map_gc(gc);
    const AW_font_limits&  font_limits = gcm->get_font_limits();

    AW_pos Y1 = Y0+font_limits.descent;
    Y0        = Y0-font_limits.ascent;

    // Fast check text against top/bottom clip
    const AW_screen_area& clipRect = get_cliprect();
    if (clipRect.t == 0) {
        if (Y1 < clipRect.t) return false;
    }
    else {
        if (Y0 < clipRect.t) return false;
    }

    if (clipRect.b == get_common()->get_screen().b) {
        if (Y0 > clipRect.b) return false;
    }
    else {
        if (Y1 > clipRect.b) return false;
    }

    // vertical check mouse against textsurrounding
    int  dist2text = 0; // exact hit -> distance == 0

    // vertical check against textborders
    if (mouse_y > Y1) { // above text
        int ydist = mouse_y-Y1;
        if (ydist > max_distance_text) return false; // too far above
        dist2text = ydist;
    }
    else if (mouse_y < Y0) { // below text
        int ydist = Y0-mouse_y;
        if (ydist > max_distance_text) return false; // too far below
        dist2text = ydist;
    }

    // align text
    int len        = opt_strlen ? opt_strlen : strlen(str);
    int text_width = (int)get_string_size(gc, str, len);

    X0        = x_alignment(X0, text_width, alignment);
    AW_pos X1 = X0+text_width;

    // check against left/right clipping areas
    if (X1 < clipRect.l) return false;
    if (X0 > clipRect.r) return false;

    // horizontal check against textborders
    if (mouse_x > X1) { // right of text
        int xdist = mouse_x-X1;
        if (xdist > max_distance_text) return false; // too far right
        dist2text = std::max(xdist, dist2text);
    }
    else if (mouse_x < X0) { // left of text
        int xdist = X0-mouse_x;
        if (xdist > max_distance_text) return false; // too far left
        dist2text = std::max(xdist, dist2text);
    }

    max_distance_text = dist2text; // exact hit -> distance = 0

    if (!opt_text.does_exist() ||            // first candidate
        (opt_text.get_distance()>dist2text)) // previous candidate had greater distance to click
    {
        Rectangle textArea(LineVector(X0, Y0, X1, Y1));

        LineVector orientation = textArea.bigger_extent();
        LineVector clippedOrientation;

        bool visible = clip(orientation, clippedOrientation);
        if (visible) {
            Position mouse(mouse_x, mouse_y);
            double   nearest_rel_pos;
            Position nearest = nearest_linepoint(mouse, clippedOrientation, nearest_rel_pos);

            opt_text.assign(rtransform(textArea), max_distance_text, nearest_rel_pos, click_cd);
        }
    }
    return true;
}

int AW_clicked_line::indicate_selected(AW_device *d, int gc) const {
    return d->line(gc, line);
}
int AW_clicked_text::indicate_selected(AW_device *d, int gc) const {
    return d->box(gc, AW::FillStyle::SOLID, textArea);
}

/**
 * Calculate the precise size of the string to be rendered 
 */
int AW_device_click::get_string_size(int gc, const char *str, long textlen) const { // @@@ check usage!
    return get_common()->map_gc(gc)->get_string_size(str, textlen);
}
