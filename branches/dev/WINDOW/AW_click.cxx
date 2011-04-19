// =============================================================== //
//                                                                 //
//   File      : AW_click.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_commn.hxx"

using namespace AW;

// ------------------------
//      AW_device_click

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


int AW_device_click::line_impl(int /*gc*/, const LineVector& Line, AW_bitset filteri) {
    if (!(filteri & filter)) return false;

    LineVector transLine = transform(Line);
    LineVector clippedLine;
    int        drawflag = clip(transLine, clippedLine);

    if (drawflag) {
        Position mouse(mouse_x, mouse_y);
        double   skalar;
        Position nearest  = nearest_linepoint(mouse, clippedLine, skalar);
        double   distance = Distance(mouse, nearest);

        if (distance < max_distance_line) {
            max_distance_line = distance;
            
            opt_line.x0 = Line.xpos();
            opt_line.y0 = Line.ypos();
            opt_line.x1 = Line.head().xpos();
            opt_line.y1 = Line.head().ypos();

            opt_line.height = distance;
            opt_line.length = skalar;

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
    return false;
}


int AW_device_click::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (filteri & filter) {
        AW_pos X0, Y0;          // Transformed pos
        this->transform(pos.xpos(), pos.ypos(), X0, Y0);

        XFontStruct *xfs = &common->gcs[gc]->curfont;

        AW_pos Y1 = Y0+(AW_pos)(xfs->max_bounds.descent);
        Y0        = Y0-(AW_pos)(xfs->max_bounds.ascent);

        // Fast check text against top/bottom clip

        if (this->clip_rect.t == 0) {
            if (Y1 < this->clip_rect.t) return 0;
        }
        else {
            if (Y0 < this->clip_rect.t) return 0;
        }

        if (this->clip_rect.b == common->screen.b) {
            if (Y0 > this->clip_rect.b) return 0;
        }
        else {
            if (Y1 > this->clip_rect.b) return 0;
        }

        // vertical check mouse against textsurrounding
        bool   exact     = true;
        double best_dist = 0;

        if (mouse_y > Y1) {     // outside text
            if (mouse_y > Y1+max_distance_text) return 0; // too far above
            exact = false;
            best_dist = mouse_y - Y1;
        }
        else if (mouse_y < Y0) {
            if (mouse_y < Y0-max_distance_text) return 0; // too far below
            exact = false;
            best_dist = Y0 - mouse_y;
        }

        // align text
        int len        = opt_strlen ? opt_strlen : strlen(str);
        int text_width = (int)get_string_size(gc, str, len);

        X0        = common->x_alignment(X0, text_width, alignment);
        AW_pos X1 = X0+text_width;

        // check against left right clipping areas
        if (X1 < this->clip_rect.l) return 0;
        if (X0 > this->clip_rect.r) return 0;

        if (mouse_x < X0) return 0; // left of text
        if (mouse_x > X1) return 0; // right of text

        max_distance_text = best_dist; // exact hit -> distance = 0;

        int position;
        if (xfs->max_bounds.width == xfs->min_bounds.width) {           // monospaced font
            short letter_width = xfs->max_bounds.width;
            position = (int)((mouse_x-X0)/letter_width);
            if (position<0) position = 0;
            if (position>(len-1)) position = len-1;
        }
        else {                                 // non-monospaced font
            AW_GC_Xm *gcm = AW_MAP_GC(gc);
            position   = 0;
            int tmp_offset = 0;
            while (position<=len) {
                tmp_offset += gcm->width_of_chars[(unsigned char)str[position]];
                if (mouse_x <= X0+tmp_offset) break;
                position++;
            }
        }

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
            opt_text.cursor       = position;
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
    }
    return 1;
}


void AW_device_click::get_clicked_line(class AW_clicked_line *ptr) {
    *ptr = opt_line;
}


void AW_device_click::get_clicked_text(class AW_clicked_text *ptr) {
    *ptr = opt_text;
}

double AW_clicked_line::distanceTo(const AW::Position& pos) {
    // @@@ change (member 'height'[sic] already contains distance)
    AW::LineVector cl(x0, y0, x1, y1);
    if (cl.length() == 0) {
        return AW::Distance(pos, cl.start());
    }
    return AW::Distance(pos, cl);
}

bool AW_getBestClick(const AW::Position& click, AW_clicked_line *cl, AW_clicked_text *ct, AW_CL *cd1, AW_CL *cd2) {
    // detect the nearest item next to 'click'
    // and return that items callback params.
    // returns false, if nothing has been clicked

    AW_clicked_element *bestClick = 0;

    if (cl->exists) {
        if (ct->exists) {
            if (cl->distanceTo(click) < ct->distance) {
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

