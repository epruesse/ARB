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
        : AW_simple_device(common_)
{
    init_click(Origin, AWT_NO_CATCH, AW_ALL_DEVICES);
}

void AW_device_click::init_click(const AW::Position& click, int max_distance, AW_bitset filteri) {
    mouse  = click;
    filter = filteri;

    max_distance_line = max_distance;
    max_distance_text = max_distance;

    opt_line = AW_clicked_line();
    opt_text = AW_clicked_text();
    opt_box  = AW_clicked_box();
}

AW_DEVICE_TYPE AW_device_click::type() {
    return AW_DEVICE_CLICK;
}

bool AW_device_click::line_impl(int /*gc*/, const AW::LineVector& Line, AW_bitset /*filteri*/) {
    LineVector transLine = transform(Line);
    LineVector clippedLine;
    bool       drawflag  = clip(transLine, clippedLine);
    if (drawflag) {
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

bool AW_device_click::box_impl(int gc, AW::FillStyle filled, const AW::Rectangle& rect, AW_bitset filteri) {
    if (!(filteri & filter)) return false; // motif-only

    int dist = -1;
    if (filled.is_empty()) {
        LocallyModify<AW_clicked_line> saveLine(opt_line, AW_clicked_line());
        LocallyModify<int>             saveDist(max_distance_line, max_distance_line);

        if (!generic_box(gc, rect, filteri)) return false;
        if (!opt_line.does_exist()) return true; // no click near any borderline detected

        dist = opt_line.get_distance();
    }
    else {
        Rectangle transRect = transform(rect);
        if (!transRect.contains(mouse)) {
            return box_impl(gc, FillStyle::EMPTY, rect, filteri); // otherwise use min. dist to box frame
        }
        dist = 0; // if inside rect -> use zero distance
    }

    aw_assert(dist != -1);
    if (!opt_box.does_exist() || dist<opt_box.get_distance()) {
        opt_box.assign(rect, dist, 0.0, click_cd);
    }
    return true;
}

bool AW_device_click::polygon_impl(int gc, AW::FillStyle filled, int npos, const AW::Position *pos, AW_bitset filteri) {
    return generic_polygon(gc, npos, pos, filteri); // stores single lines - shall store polygon
}

bool AW_device_click::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (!(filteri & filter)) return false; // motif only

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
    if (mouse.ypos() > Y1) { // above text
        int ydist = mouse.ypos()-Y1;
        if (ydist > max_distance_text) return false; // too far above
        dist2text = ydist;
    }
    else if (mouse.ypos() < Y0) { // below text
        int ydist = Y0-mouse.ypos();
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
    if (mouse.xpos() > X1) { // right of text
        int xdist = mouse.xpos()-X1;
        if (xdist > max_distance_text) return false; // too far right
        dist2text = std::max(xdist, dist2text);
    }
    else if (mouse.xpos() < X0) { // left of text
        int xdist = X0-mouse.xpos();
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
            double   nearest_rel_pos;
            Position nearest = nearest_linepoint(mouse, clippedOrientation, nearest_rel_pos);

            opt_text.assign(rtransform(textArea), max_distance_text, nearest_rel_pos, click_cd);
        }
    }
    return true;
}

const AW_clicked_element *AW_device_click::best_click(ClickPreference prefer) {
    // returns the element with lower distance (to mouse-click- or key-"click"-position).
    // or NULL (if no element was found inside catch-distance)
    //
    // Note: during drag/drop a target element is only available in AWT_MODE_MOVE!
    // see ../AWT/AWT_canvas.cxx@motion_event

    const AW_clicked_element *bestClick = 0;

    if (prefer == PREFER_LINE && opt_line.does_exist()) bestClick = &opt_line;
    if (prefer == PREFER_TEXT && opt_text.does_exist()) bestClick = &opt_text;

    if (!bestClick) {
        const AW_clicked_element *maybeClicked[] = {
            &opt_line,
            &opt_text,
            &opt_box,
        };

        for (size_t i = 0; i<ARRAY_ELEMS(maybeClicked); ++i) {
            if (maybeClicked[i]->does_exist()) {
                if (!bestClick || maybeClicked[i]->get_distance()<bestClick->get_distance()) {
                    bestClick = maybeClicked[i];
                }
            }
        }
    }

    return bestClick;
}

int AW_clicked_line::indicate_selected(AW_device *d, int gc) const {
    return d->line(gc, line);
}
int AW_clicked_text::indicate_selected(AW_device *d, int gc) const {
#if defined(ARB_MOTIF)
    return d->box(gc, AW::FillStyle::SOLID, textArea);
#else // !defined(ARB_MOTIF)
    d->set_grey_level(gc, 0.2);
    return d->box(gc, AW::FillStyle::SHADED_WITH_BORDER, textArea);
#endif
}
int AW_clicked_box::indicate_selected(AW_device *d, int gc) const {
    fprintf(stderr, "AW_clicked_box::indicate_selected\n");
    return d->box(gc, AW::FillStyle::SOLID, box);
}

/**
 * Calculate the precise size of the string to be rendered 
 */
int AW_device_click::get_string_size(int gc, const char *str, long textlen) const { // @@@ check usage!
    return get_common()->map_gc(gc)->get_string_size(str, textlen);
}
