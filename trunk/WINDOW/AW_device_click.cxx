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

#include "aw_common.hxx"
#include "aw_device_click.hxx"
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

    opt_line.clear();
    opt_text.clear();
}

AW_DEVICE_TYPE AW_device_click::type() {
    return AW_DEVICE_CLICK;
}

bool AW_device_click::line_impl(int /*gc*/, const AW::LineVector& Line, AW_bitset filteri) {
    if (!(filteri & filter)) return false; // motif-only

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
            
            opt_line.x0 = Line.xpos();
            opt_line.y0 = Line.ypos();
            opt_line.x1 = Line.head().xpos();
            opt_line.y1 = Line.head().ypos();

            opt_line.distance = distance;
            opt_line.set_rel_pos(nearest_rel_pos);

            opt_line.copy_cds(click_cd);
            opt_line.exists = true;
        }
    }
    return drawflag;
}


bool AW_device_click::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (!(filteri & filter)) return false; // motif only - gtk should

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
    bool exact     = true;
    int  dist2text = 0; // exact hit -> distance == 0

    // vertical check against textborders
    if (mouse_y > Y1) { // above text
        int ydist = mouse_y-Y1;
        if (ydist > max_distance_text) return false; // too far above
        exact     = false;
        dist2text = ydist;
    }
    else if (mouse_y < Y0) { // below text
        int ydist = Y0-mouse_y;
        if (ydist > max_distance_text) return false; // too far below
        exact     = false;
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
        exact     = false;
        dist2text = std::max(xdist, dist2text);
    }
    else if (mouse_x < X0) { // left of text
        int xdist = X0-mouse_x;
        if (xdist > max_distance_text) return false; // too far left
        exact     = false;
        dist2text = std::max(xdist, dist2text);
    }

    max_distance_text = dist2text; // exact hit -> distance = 0

    int position = -1; // invalid (if not exact)
    if (exact) {
        if (font_limits.is_monospaced()) {
            short letter_width = font_limits.width;
            position = (int)((mouse_x-X0)/letter_width);
            if (position<0) position = 0;
            if (position>(len-1)) position = len-1;
        }
        else { // proportional font
            position   = 0;
            int tmp_offset = 0;
            while (position<=len) {
                tmp_offset += gcm->get_width_of_char(str[position]);
                if (mouse_x <= X0+tmp_offset) break;
                position++;
            }
        }
    }

    if (!opt_text.exists              || // first candidate
        (!opt_text.exactHit && exact) || // previous candidate was no exact hit
        (opt_text.distance>dist2text))   // previous candidate had greater distance to click
    {
        Rectangle textArea(LineVector(X0, Y0, X1, Y1));

        LineVector orientation = textArea.bigger_extent();
        LineVector clippedOrientation;

        bool visible = clip(orientation, clippedOrientation);
        if (visible) {
            Position mouse(mouse_x, mouse_y);
            double   nearest_rel_pos;
            Position nearest = nearest_linepoint(mouse, clippedOrientation, nearest_rel_pos);

            opt_text.textArea = rtransform(textArea);
            opt_text.set_rel_pos(nearest_rel_pos);
            opt_text.cursor   = position;

            opt_text.distance = max_distance_text;
            opt_text.exactHit = exact;

            opt_text.copy_cds(click_cd);
            opt_text.exists   = true;
        }
    }
    return true;
}


void AW_device_click::get_clicked_line(AW_clicked_line *ptr) const {
    *ptr = opt_line;
}


void AW_device_click::get_clicked_text(AW_clicked_text *ptr) const {
    *ptr = opt_text;
}

int AW_clicked_line::indicate_selected(AW_device *d, int gc) const {
    return d->line(gc, x0, y0, x1, y1);
}
int AW_clicked_text::indicate_selected(AW_device *d, int gc) const {
    return d->box(gc, true, textArea);
}

