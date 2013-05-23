/* 
 * File:   AW_device_click.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 11:34 AM
 */

#include "aw_device_click.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_common.hxx"

using namespace AW;

AW_device_click::AW_device_click(AW_common *common_)
        : AW_simple_device(common_) {
    init(0, 0, -1, -1, 0, AW_ALL_DEVICES);
}

void AW_device_click::init(AW_pos mousex, AW_pos mousey, AW_pos max_distance_linei, AW_pos max_distance_texti, AW_pos /*radi*/, AW_bitset filteri) {
    mouse_x           = mousex;
    mouse_y           = mousey;
    filter            = filteri;
    max_distance_line = max_distance_linei*max_distance_linei;
    max_distance_text = max_distance_texti;
    memset((char *)&opt_line, 0, sizeof(opt_line));
    memset((char *)&opt_text, 0, sizeof(opt_text));
    opt_line.exists   = false;
    opt_text.exists   = false;
}

AW_DEVICE_TYPE AW_device_click::type() {
    return AW_DEVICE_CLICK;
}

bool AW_device_click::line_impl(int /*gc*/, const AW::LineVector& Line, AW_bitset /*filteri*/) {
    AW::LineVector transLine = transform(Line);
    AW::LineVector clippedLine;
    if (!clip(transLine, clippedLine)) return false;

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

      opt_line.distance        = distance;
      opt_line.nearest_rel_pos = nearest_rel_pos;

      if (click_cd) {
          opt_line.client_data1 = click_cd->get_cd1();
          opt_line.client_data2 = click_cd->get_cd2();
      }
      else {
          opt_line.client_data1 = 0;
          opt_line.client_data2 = 0;
      }
      opt_line.exists = true;
    }
    return true;
}


bool AW_device_click::text_impl(int gc, const char *str, const AW::Position& pos, 
                                AW_pos alignment, AW_bitset /*filteri*/, long opt_strlen) {
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
    bool   exact     = true;
    double best_y_dist = 0;

    // @@@ the following section produces wrong result
    // 
    if (mouse_y > Y1) {     // below text
        if (mouse_y > Y1+max_distance_text) return false; // too far above
        exact = false;
        best_y_dist = mouse_y - Y1;
    }
    else if (mouse_y < Y0) { // above text
        if (mouse_y < Y0-max_distance_text) return false; // too far below
        exact = false;
        best_y_dist = Y0 - mouse_y;
    }

    

    // align text
    int len        = opt_strlen ? opt_strlen : strlen(str);
    int text_width = (int)get_string_size(gc, str, len);

    X0        = AW::x_alignment(X0, text_width, alignment);
    AW_pos X1 = X0+text_width;

    // check against left right clipping areas
    if (X1 < clipRect.l) return false;
    if (X0 > clipRect.r) return false;

    if (mouse_x < X0) return false; // left of text
    if (mouse_x > X1) return false; // right of text

    max_distance_text = best_y_dist; // exact hit -> distance = 0;

    AW_pos dist2center = Distance(Position(mouse_x, mouse_y),
                                  LineVector(X0, Y0, X0+text_width, Y1).centroid());

    if (!opt_text.exists || // first candidate
        (!opt_text.exactHit && exact) || // previous candidate was no exact hit
        (dist2center<opt_text.dist2center)) // distance to text-center is smaller
      {
        opt_text.textArea     = Rectangle(rtransform(LineVector(X0, Y0, X1, Y1)));
        opt_text.alignment    = alignment;
        opt_text.rotation     = 0;
        opt_text.distance     = max_distance_text;
        opt_text.dist2center  = dist2center;
        if (click_cd) {
            opt_text.client_data1 = click_cd->get_cd1();
            opt_text.client_data2 = click_cd->get_cd2();
        }
        else {
            opt_text.client_data1 = 0;
            opt_text.client_data2 = 0;
        }
        // opt_text.client_data1 = clientdata1;
        // opt_text.client_data2 = clientdata2;
        opt_text.exists       = true;
        opt_text.exactHit     = exact;
      }
    return true;
}


void AW_device_click::get_clicked_line(AW_clicked_line *ptr) const {
    *ptr = opt_line;
}


void AW_device_click::get_clicked_text(AW_clicked_text *ptr) const {
    *ptr = opt_text;
}

//void AW_device_click::get_clicked(AW_clicked_element *ptr, int *is_text) const 

bool AW_getBestClick(AW_clicked_line *cl, AW_clicked_text *ct, AW_CL *cd1, AW_CL *cd2) {
    // detect the nearest item next to 'click'
    // and return that items callback params.
    // returns false, if nothing has been clicked

    AW_clicked_element *bestClick = 0;

    if (cl->exists) {
        if (ct->exists) {
            if (cl->distance < ct->distance) {
                bestClick = cl;
            }
            else {
                bestClick = ct;
            }
        }
        else {
            bestClick = cl;
        }
    }
    else if (ct->exists) {
        bestClick = ct;
    }

    if (bestClick) {
        *cd1 = bestClick->client_data1;
        *cd2 = bestClick->client_data2;
    }
    else {
        *cd1 = 0;
        *cd2 = 0;
    }

    return bestClick;
}
