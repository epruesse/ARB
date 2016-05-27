// ================================================================ //
//                                                                  //
//   File      : AWT_canvas.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt_canvas.hxx"
#include "awt.hxx"

#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_preset.hxx>

#include <arbdbt.h>

#include <algorithm>

using namespace std;
using namespace AW;

void AWT_graphic_exports::clear() {
    zoom_reset       = 0;
    resize           = 0;
    refresh          = 0;
    save             = 0;
    structure_change = 0;
}

void AWT_graphic_exports::init() {
    clear();
    padding.clear();

    zoom_mode = AWT_ZOOM_BOTH;
    fit_mode  = AWT_FIT_LARGER;

    dont_scroll = 0;
}

inline void AWT_canvas::push_transaction() const { if (gb_main) GB_push_transaction(gb_main); }
inline void AWT_canvas::pop_transaction() const { if (gb_main) GB_pop_transaction(gb_main); }

void AWT_canvas::set_horizontal_scrollbar_position(AW_window *, int pos) {
    int maxpos = int(worldsize.r-rect.r)-1;
    if (pos>maxpos) pos = maxpos;
    if (pos<0) pos = 0;
    aww->set_horizontal_scrollbar_position(pos);
}

void AWT_canvas::set_vertical_scrollbar_position(AW_window *, int pos) {
    int maxpos = int(worldsize.b-rect.b)-1;
    if (pos>maxpos) pos = maxpos;
    if (pos<0) pos = 0;
    aww->set_vertical_scrollbar_position(pos);
}

void AWT_canvas::set_scrollbars() {
    AW_pos width  = this->worldinfo.r - this->worldinfo.l;
    AW_pos height = this->worldinfo.b - this->worldinfo.t;

    worldsize.l = 0;
    worldsize.t = 0;

    AW::Vector zv = gfx->exports.zoomVector(trans_to_fit);

    worldsize.r = width *zv.x() + gfx->exports.get_x_padding();
    worldsize.b = height*zv.y() + gfx->exports.get_y_padding();

    aww->tell_scrolled_picture_size(worldsize);

    aww->calculate_scrollbars();

    this->old_hor_scroll_pos = (int)((-this->worldinfo.l -
                                      this->shift_x_to_fit)*
                                     this->trans_to_fit +
                                     gfx->exports.get_left_padding());
    this->set_horizontal_scrollbar_position(this->aww, old_hor_scroll_pos);

    this->old_vert_scroll_pos = (int)((-this->worldinfo.t -
                                       this->shift_y_to_fit)*
                                      this->trans_to_fit+
                                      gfx->exports.get_top_padding());

    this->set_vertical_scrollbar_position(this->aww, old_vert_scroll_pos);
}

void AWT_canvas::init_device(AW_device *device) {
    device->reset();
    device->shift(AW::Vector(shift_x_to_fit, shift_y_to_fit));
    device->zoom(this->trans_to_fit);
}

void AWT_canvas::recalc_size(bool adjust_scrollbars) {
    GB_transaction  ta(this->gb_main);
    AW_device_size *size_device = aww->get_size_device(AW_MIDDLE_AREA);

    size_device->set_filter(AW_SIZE|(consider_text_for_size ? AW_SIZE_UNSCALED : 0));
    size_device->reset();

    gfx->show(size_device);

    if (consider_text_for_size) {
        gfx->exports.set_extra_text_padding(size_device->get_unscaleable_overlap());
    }

    size_device->get_size_information(&(this->worldinfo));
    rect = size_device->get_area_size();   // real world size (no offset)

    if (adjust_scrollbars) set_scrollbars();
}

void AWT_canvas::zoom_reset() {
    recalc_size(false);

    AW_pos width  = this->worldinfo.r - this->worldinfo.l;
    AW_pos height = this->worldinfo.b - this->worldinfo.t;

    AW_pos net_window_width  = rect.r - rect.l - gfx->exports.get_x_padding();
    AW_pos net_window_height = rect.b - rect.t - gfx->exports.get_y_padding();
    
    if (net_window_width<AWT_MIN_WIDTH) net_window_width   = AWT_MIN_WIDTH;
    if (net_window_height<AWT_MIN_WIDTH) net_window_height = AWT_MIN_WIDTH;

    if (width <EPS) width   = EPS;
    if (height <EPS) height = EPS;

    AW_pos x_scale = net_window_width/width;
    AW_pos y_scale = net_window_height/height;

    trans_to_fit = -1;
    switch (gfx->exports.fit_mode) {
        case AWT_FIT_NEVER:   trans_to_fit = 1.0; break;
        case AWT_FIT_X:       trans_to_fit = x_scale; break;
        case AWT_FIT_Y:       trans_to_fit = y_scale; break;
        case AWT_FIT_LARGER:  trans_to_fit = std::min(x_scale, y_scale); break;
        case AWT_FIT_SMALLER: trans_to_fit = std::max(x_scale, y_scale); break;
    }
    aw_assert(trans_to_fit > 0);

    AW_pos center_shift_x = 0;
    AW_pos center_shift_y = 0;

    if (gfx->exports.zoom_mode&AWT_ZOOM_X) center_shift_x = (net_window_width /trans_to_fit - width)/2;
    if (gfx->exports.zoom_mode&AWT_ZOOM_Y) center_shift_y = (net_window_height/trans_to_fit - height)/2;

    // complete, upper left corner
    this->shift_x_to_fit = - this->worldinfo.l + gfx->exports.get_left_padding()/trans_to_fit + center_shift_x;
    this->shift_y_to_fit = - this->worldinfo.t + gfx->exports.get_top_padding()/trans_to_fit  + center_shift_y;

    this->old_hor_scroll_pos  = 0;
    this->old_vert_scroll_pos = 0;

    // scale

    this->set_scrollbars();
}

void AWT_canvas::zoom(AW_device *device, bool zoomIn, const Rectangle& wanted_part, const Rectangle& current_part, int percent) {
    // zooms the device.
    //
    // zoomIn == true -> wanted_part is zoomed to current_part
    // zoomIn == false -> current_part is zoomed to wanted_part
    //
    // If wanted_part is very small -> assume mistake (act like single click)
    // Single click zooms by 'percent' % centering on click position

    init_device(device);

    if (!gfx) {
        awt_assert(0); // we have no display - does this occur?
                       // if yes, pls inform devel@arb-home.de about circumstances
        return;
    }

    AW_pos width  = worldinfo.r-worldinfo.l;
    AW_pos height = worldinfo.b-worldinfo.t;

    if (width<EPS) width = EPS;
    if (height<EPS) height = EPS;

    AWT_zoom_mode zoom_mode = gfx->exports.zoom_mode;
    if (zoom_mode == AWT_ZOOM_NEVER) {
        aw_message("Zoom does not work in this mode");
        return;
    }

    Rectangle current(device->rtransform(current_part));
    Rectangle wanted;

    bool isClick = false;
    switch (zoom_mode) {
        case AWT_ZOOM_BOTH: isClick = wanted_part.line_vector().length()<40.0; break;
        case AWT_ZOOM_X:    isClick = wanted_part.width()<30.0;                break;
        case AWT_ZOOM_Y:    isClick = wanted_part.height()<30.0;               break;

        case AWT_ZOOM_NEVER: awt_assert(0); break;
    }

    if (isClick) { // very small part or single click
        // -> zoom by 'percent' % on click position
        Position clickPos = device->rtransform(wanted_part.centroid());

        Vector click2UpperLeft  = current.upper_left_corner()-clickPos;
        Vector click2LowerRight = current.lower_right_corner()-clickPos;

        double scale = (100-percent)/100.0;

        wanted = Rectangle(clickPos+scale*click2UpperLeft, clickPos+scale*click2LowerRight);
    }
    else {
        wanted = Rectangle(device->rtransform(wanted_part));
    }

    if (!zoomIn) {
        // calculate big rectangle (outside of viewport), which is zoomed into viewport

        if (zoom_mode == AWT_ZOOM_BOTH) {
            double    factor = current.diagonal().length()/wanted.diagonal().length();
            Vector    curr2wanted(current.upper_left_corner(), wanted.upper_left_corner());
            Rectangle big(current.upper_left_corner()+(curr2wanted*-factor), current.diagonal()*factor);

            wanted = big;
        }
        else {
            double factor;
            if (zoom_mode == AWT_ZOOM_X) {
                factor = current.width()/wanted.width();
            }
            else {
                awt_assert(zoom_mode == AWT_ZOOM_Y);
                factor = current.height()/wanted.height();
            }
            Vector    curr2wanted_start(current.upper_left_corner(), wanted.upper_left_corner());
            Vector    curr2wanted_end(current.lower_right_corner(), wanted.lower_right_corner());
            Rectangle big(current.upper_left_corner()+(curr2wanted_start*-factor),
                          current.lower_right_corner()+(curr2wanted_end*-factor));

            wanted = big;
        }
    }

    // scroll
    shift_x_to_fit = (zoom_mode&AWT_ZOOM_X) ? -wanted.start().xpos() : (shift_x_to_fit+worldinfo.l)*trans_to_fit;
    shift_y_to_fit = (zoom_mode&AWT_ZOOM_Y) ? -wanted.start().ypos() : (shift_y_to_fit+worldinfo.t)*trans_to_fit;

    // scale
    if ((rect.r-rect.l)<EPS) rect.r = rect.l+1;
    if ((rect.b-rect.t)<EPS) rect.b = rect.t+1;

    AW_pos max_trans_to_fit = 0;

    switch (zoom_mode) {
        case AWT_ZOOM_BOTH:
            trans_to_fit     = max((rect.r-rect.l)/wanted.width(), (rect.b-rect.t)/wanted.height());
            max_trans_to_fit = 32000.0/max(width, height);
            break;

        case AWT_ZOOM_X:
            trans_to_fit     = (rect.r-rect.l)/wanted.width();
            max_trans_to_fit = 32000.0/width;
            break;

        case AWT_ZOOM_Y:
            trans_to_fit     = (rect.b-rect.t)/wanted.height();
            max_trans_to_fit = 32000.0/height;
            break;

        case AWT_ZOOM_NEVER: awt_assert(0); break;
    }
    trans_to_fit = std::min(trans_to_fit, max_trans_to_fit);

    // correct scrolling for "dont_fit"-direction
    if (zoom_mode == AWT_ZOOM_Y) shift_x_to_fit = (shift_x_to_fit/trans_to_fit)-worldinfo.l;
    if (zoom_mode == AWT_ZOOM_X) shift_y_to_fit = (shift_y_to_fit/trans_to_fit)-worldinfo.t;

    set_scrollbars();
}

inline void nt_draw_zoom_box(AW_device *device, int gc, AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2) {
    device->box(gc, AW::FillStyle::EMPTY, x1, y1, x2-x1, y2-y1);
}
inline void nt_draw_zoom_box(AW_device *device, AWT_canvas *scr) {
    nt_draw_zoom_box(device,
                     scr->gfx->get_drag_gc(),
                     scr->zoom_drag_sx, scr->zoom_drag_sy,
                     scr->zoom_drag_ex, scr->zoom_drag_ey);
}

static void clip_expose(AW_window *aww, AWT_canvas *scr,
                        int left_border, int right_border,
                        int top_border, int bottom_border,
                        int hor_overlap, int ver_overlap)
{
    AW_device *device = aww->get_device (AW_MIDDLE_AREA);
    device->set_filter(AW_SCREEN);
    device->reset();

    device->set_top_clip_border(top_border);
    device->set_bottom_clip_border(bottom_border);
    device->set_left_clip_border(left_border);
    device->set_right_clip_border(right_border);

    device->clear_part(left_border, top_border, right_border-left_border,
                       bottom_border-top_border, -1);

    GB_transaction ta(scr->gb_main);

    if (scr->gfx->check_update(scr->gb_main)>0) {
        scr->zoom_reset();
    }

    scr->init_device(device);

    if (hor_overlap> 0.0) {
        device->set_right_clip_border(right_border + hor_overlap);
    }
    if (hor_overlap< 0.0) {
        device->set_left_clip_border(left_border + hor_overlap);
    }
    if (ver_overlap> 0.0) {
        device->set_bottom_clip_border(bottom_border + ver_overlap);
    }
    if (ver_overlap< 0.0) {
        device->set_top_clip_border(top_border + ver_overlap);
    }
    scr->gfx->show(device);
}

void AWT_expose_cb(UNFIXED, AWT_canvas *scr) {
    clip_expose(scr->aww, scr, scr->rect.l, scr->rect.r,
                scr->rect.t, scr->rect.b, 0, 0);
}

void AWT_canvas::refresh() {
    AW_device *device = this->aww->get_device (AW_MIDDLE_AREA);
    device->queue_draw();
}

void AWT_resize_cb(UNFIXED, AWT_canvas *scr) {
    scr->zoom_reset();
}

void AWT_GC_changed_cb(GcChange whatChanged, AWT_canvas *scr) {
    // standard callback which may be passed to AW_manage_GC
    switch (whatChanged) {
        case GC_COLOR_CHANGED:
        case GC_COLOR_GROUP_USE_CHANGED:
            AWT_expose_cb(NULL, scr);
            break;
        case GC_FONT_CHANGED:
            AWT_resize_cb(NULL, scr);
            break;
    }
}

static void canvas_focus_cb(AW_window *, AWT_canvas *scr) {
    if (scr->gb_main) {
        scr->push_transaction();

        int flags = scr->gfx->check_update(scr->gb_main);
        if (flags) scr->recalc_size_and_refresh();

        scr->pop_transaction();
    }
}

const int ZOOM_SPEED_CLICK = 10;
const int ZOOM_SPEED_WHEEL = 4;

static bool handleZoomEvent(AWT_canvas *scr, AW_device *device, const AW_event& event, int percent) {
    bool handled = false;
    bool zoomIn  = true;

    if      (event.button == AW_BUTTON_LEFT)  { handled = true; }
    else if (event.button == AW_BUTTON_RIGHT) { handled = true; zoomIn  = false; }

    if (handled) {
        if (event.type == AW_Mouse_Press) {
            scr->drag = 1;
            scr->zoom_drag_sx = scr->zoom_drag_ex = event.x;
            scr->zoom_drag_sy = scr->zoom_drag_ey = event.y;
        }
        else {
            // delete last box
            nt_draw_zoom_box(device, scr);
            scr->drag = 0;

            Rectangle screen(scr->rect, INCLUSIVE_OUTLINE);
            Rectangle drag(scr->zoom_drag_sx, scr->zoom_drag_sy, scr->zoom_drag_ex, scr->zoom_drag_ey);

            scr->zoom(device, zoomIn, drag, screen, percent);
            AWT_expose_cb(NULL, scr);
        }
    }
    else if (event.keycode == AW_KEY_ASCII && event.character == '0') { // reset zoom (as promised by MODE_TEXT_STANDARD_ZOOMMODE)
        scr->zoom_reset_and_refresh();
        handled = true;
    }
    return handled;
}

bool AWT_canvas::handleWheelEvent(AW_device *device, const AW_event& event) {
    if (event.button != AW_WHEEL_UP && event.button != AW_WHEEL_DOWN)  {
        return false; // not handled
    }
    if (event.type == AW_Mouse_Press) {
        if (event.keymodifier & AW_KEYMODE_CONTROL) {
            AW_event faked = event;

            faked.button = (event.button == AW_WHEEL_UP) ? AW_BUTTON_LEFT : AW_BUTTON_RIGHT;
            handleZoomEvent(this, device, faked, ZOOM_SPEED_WHEEL);
            faked.type   = AW_Mouse_Release;
            handleZoomEvent(this, device, faked, ZOOM_SPEED_WHEEL);
        }
        else {
            bool horizontal = event.keymodifier & AW_KEYMODE_ALT;

            int viewport_size = horizontal ? (rect.r-rect.l+1) : (rect.b-rect.t+1);
            int gfx_size      = horizontal ? (worldsize.r-worldsize.l) : (worldsize.b-worldsize.t);
            
            // scroll 10% of screen or 10% of graphic size (whichever is smaller):
            int dist      = std::min(viewport_size / 20, gfx_size / 30);
            int direction = event.button == AW_WHEEL_UP ? -dist : dist;

            int dx = horizontal ? direction : 0;
            int dy = horizontal ? 0 : direction;

            scroll(dx, dy);
        }
    }
    return true;
}

void AWT_graphic::postevent_handler(GBDATA *gb_main) {
    // handle AWT_graphic_exports

    if (exports.save) {
        GB_ERROR error = save(gb_main, NULL);
        if (error) {
            aw_message(error);
            load(gb_main, NULL);
        }
        exports.structure_change = 1;
    }
    if (exports.structure_change) {
        update_structure();
        exports.resize = 1;
    }
    if (gb_main) update(gb_main);
}

void AWT_canvas::postevent_handler() {
    gfx->postevent_handler(gb_main);
    gfx->refresh_by_exports(this);
}

static void input_event(AW_window *aww, AWT_canvas *scr) {
    awt_assert(aww = scr->aww);

    AW_event event;
    aww->get_event(&event);
    
    AW_device *device = aww->get_device(AW_MIDDLE_AREA);
    device->set_filter(AW_SCREEN);
    device->reset();

    scr->gfx->exports.clear();
    scr->push_transaction();

    scr->gfx->check_update(scr->gb_main);

    bool event_handled = false;

    if (event.button == AW_BUTTON_MIDDLE) {
        event_handled = true; // only set zoom_drag_e.. below
    }
    else if (scr->mode == AWT_MODE_ZOOM) { // zoom mode is identical for all applications, so handle it here
        event_handled = handleZoomEvent(scr, device, event, ZOOM_SPEED_CLICK);
    }

    if (!event_handled) {
        event_handled = scr->handleWheelEvent(device, event);
    }

    if (!event_handled) {
        AW_device_click *click_device = aww->get_click_device(AW_MIDDLE_AREA, event.x, event.y, AWT_CATCH);
        click_device->set_filter(AW_CLICK);
        device->set_filter(AW_SCREEN);

        scr->init_device(click_device);
        scr->init_device(device);

        scr->gfx->show(click_device);
        if (event.type == AW_Mouse_Press) {
            scr->gfx->drag_target_detection(false);
            // drag_target_detection is off by default.
            // it should be activated in handle_command (by all modes that need it)
        }

        AWT_graphic_event gevent(scr->mode, event, false, click_device);
        scr->gfx->handle_command(device, gevent);

        scr->postevent_handler();
    }

    scr->zoom_drag_ex = event.x;
    scr->zoom_drag_ey = event.y;
    scr->pop_transaction();
}


void AWT_canvas::set_dragEndpoint(int dragx, int dragy) {
    switch (gfx->exports.zoom_mode) {
        case AWT_ZOOM_NEVER: {
            awt_assert(0);
            break;
        }
        case AWT_ZOOM_X: {
            zoom_drag_sy = rect.t;
            zoom_drag_ey = rect.b-1;
            zoom_drag_ex = dragx;
            break;
        }
        case AWT_ZOOM_Y: {
            zoom_drag_sx = rect.l;
            zoom_drag_ex = rect.r-1;
            zoom_drag_ey = dragy;
            break;
        }
        case AWT_ZOOM_BOTH: {
            zoom_drag_ex = dragx;
            zoom_drag_ey = dragy;

            int drag_sx = zoom_drag_ex-zoom_drag_sx;
            int drag_sy = zoom_drag_ey-zoom_drag_sy;

            bool   correct_x = false;
            bool   correct_y = false;
            double factor;

            int scr_sx = rect.r-rect.l;
            int scr_sy = rect.b-rect.t;

            if (drag_sx == 0) {
                if (drag_sy != 0) { factor = double(drag_sy)/scr_sy; correct_x = true; }
            }
            else {
                if (drag_sy == 0) { factor = double(drag_sx)/scr_sx; correct_y = true; }
                else {
                    double facx = double(drag_sx)/scr_sx;
                    double facy = double(drag_sy)/scr_sy;

                    if (fabs(facx)>fabs(facy)) { factor = facx; correct_y = true; }
                    else                       { factor = facy; correct_x = true; }
                }
            }

            if (correct_x) {
                int width    = int(scr_sx*factor) * ((drag_sx*drag_sy) < 0 ? -1 : 1);
                zoom_drag_ex = zoom_drag_sx+width;
            }
            else if (correct_y) {
                int height = int(scr_sy*factor) * ((drag_sx*drag_sy) < 0 ? -1 : 1);
                zoom_drag_ey = zoom_drag_sy+height;
            }
            break;
        }
    }
}

static void motion_event(AW_window *aww, AWT_canvas *scr) {
    AW_device *device = aww->get_device(AW_MIDDLE_AREA);
    device->reset();
    device->set_filter(AW_SCREEN);

    scr->push_transaction();

    AW_event event;
    aww->get_event(&event);

    if (event.button == AW_BUTTON_MIDDLE) {
        // shift display in ALL modes
        int dx = event.x - scr->zoom_drag_ex;
        int dy = event.y - scr->zoom_drag_ey;

        scr->zoom_drag_ex = event.x;
        scr->zoom_drag_ey = event.y;

        // display
        scr->scroll(-dx*3, -dy*3);
    }
    else {
        if (event.button == AW_BUTTON_LEFT || event.button == AW_BUTTON_RIGHT) {
            if (scr->mode == AWT_MODE_ZOOM) {
                if (scr->gfx->exports.zoom_mode != AWT_ZOOM_NEVER) {
                    nt_draw_zoom_box(device, scr);
                    scr->set_dragEndpoint(event.x, event.y);
                    nt_draw_zoom_box(device, scr);
                }
            }
            else {
                AW_device_click *click_device = NULL;

                if (scr->gfx->wants_drag_target()) {
                    // drag/drop-target is only updated if requested via AWT_graphic::drag_target_detection
                    click_device = aww->get_click_device(AW_MIDDLE_AREA, event.x, event.y, AWT_CATCH);
                    click_device->set_filter(AW_CLICK_DROP);

                    scr->init_device(click_device);
                    scr->gfx->show(click_device);
                }

                scr->init_device(device);
                AWT_graphic_event gevent(scr->mode, event, true, click_device);
                scr->gfx->handle_command(device, gevent);
            }
        }
    }

    scr->postevent_handler();
    scr->pop_transaction();
}

void AWT_canvas::scroll(int dx, int dy, bool dont_update_scrollbars) {
    int csx, cdx, cwidth, csy, cdy, cheight;
    AW_device *device;
    if (!dont_update_scrollbars) {
        this->old_hor_scroll_pos += dx;
        this->set_horizontal_scrollbar_position(aww, this->old_hor_scroll_pos);
        this->old_vert_scroll_pos += dy;
        this->set_vertical_scrollbar_position(aww, this->old_vert_scroll_pos);
    }
    device = aww->get_device (AW_MIDDLE_AREA);
    device->set_filter(AW_SCREEN);
    device->reset();
    int screenwidth = this->rect.r-this->rect.l;
    int screenheight = this->rect.b-this->rect.t;

    // compute move area params

    if (dx>0) {
        csx = dx;
        cdx = 0;
        cwidth = screenwidth-dx;
    }
    else {
        csx = 0;
        cdx = -dx;
        cwidth = screenwidth+dx;
    }
    if (dy>0) {
        csy = dy;
        cdy = 0;
        cheight = screenheight-dy;
    }
    else {
        csy = 0;
        cdy = -dy;
        cheight = screenheight+dy;
    }

    // move area
    // FIXME: scrolling with move region does not work
    // (e.g. because of brackets in tree)
    if (!gfx->exports.dont_scroll && false) {
        device->move_region(csx, csy, cwidth, cheight, cdx, cdy);
        // redraw stripes
        this->shift_x_to_fit -= dx/this->trans_to_fit;
        this->shift_y_to_fit -= dy/this->trans_to_fit;

        // x-stripe
        if ((int)dx>0) {
            clip_expose(aww, this,
                        screenwidth-dx, screenwidth, 0, screenheight,
                        -CLIP_OVERLAP,  0);
        }
        if ((int)dx<0) {
            clip_expose(aww, this,
                        0, -dx, 0, screenheight,
                        CLIP_OVERLAP, 0);
        }

        // y-stripe
        if ((int)dy>0) {
            clip_expose(aww, this,
                        0, screenwidth, screenheight-dy, screenheight,
                        0, -CLIP_OVERLAP);
        }
        if ((int)dy<0) {
            clip_expose(aww, this,
                        0, screenwidth, 0,  -dy,
                        0,  CLIP_OVERLAP);
        }
    }
    else {          // redraw everything
        // redraw stripes
        this->shift_x_to_fit -= dx/this->trans_to_fit;
        this->shift_y_to_fit -= dy/this->trans_to_fit;
        AWT_expose_cb(NULL, this);
    }
}

static void scroll_vert_cb(AW_window *aww, AWT_canvas* scr) {
    int new_vert       = aww->slider_pos_vertical;
    int delta_screen_y = (new_vert - scr->old_vert_scroll_pos);

    scr->scroll(0, delta_screen_y, true);
    scr->old_vert_scroll_pos = new_vert;
}

static void scroll_hor_cb(AW_window *aww, AWT_canvas* scr) {
    int new_hor        = aww->slider_pos_horizontal;
    int delta_screen_x = (new_hor - scr->old_hor_scroll_pos);

    scr->scroll(delta_screen_x, 0, true);
    scr->old_hor_scroll_pos = new_hor;
}


AWT_canvas::AWT_canvas(GBDATA *gb_maini, AW_window *awwi, const char *gc_base_name_, AWT_graphic *awd, const char *user_awari)
    : consider_text_for_size(true)
    , gc_base_name(strdup(gc_base_name_))
    , user_awar(strdup(user_awari))
    , shift_x_to_fit(0)
    , shift_y_to_fit(0)
    , gb_main(gb_maini)
    , aww(awwi)
    , awr(aww->get_root())
    , gfx(awd)
    , gc_manager(gfx->init_devices(aww, aww->get_device(AW_MIDDLE_AREA), this))
    , mode(AWT_MODE_NONE)
{
    gfx->drag_gc   = AW_get_drag_gc(gc_manager);

    AWT_resize_cb(NULL, this);

    aww->set_expose_callback(AW_MIDDLE_AREA, makeWindowCallback(AWT_expose_cb, this));
    aww->set_resize_callback(AW_MIDDLE_AREA, makeWindowCallback(AWT_resize_cb, this));
    aww->set_input_callback(AW_MIDDLE_AREA, makeWindowCallback(input_event, this));
    aww->set_focus_callback(makeWindowCallback(canvas_focus_cb, this));

    aww->set_motion_callback(AW_MIDDLE_AREA, makeWindowCallback(motion_event, this));
    aww->set_horizontal_change_callback(makeWindowCallback(scroll_hor_cb, this));
    aww->set_vertical_change_callback(makeWindowCallback(scroll_vert_cb, this));
}

// --------------------------
//      AWT_nonDB_graphic

GB_ERROR AWT_nonDB_graphic::load(GBDATA *, const char *) {
    return "AWT_nonDB_graphic cannot be loaded";
}

GB_ERROR AWT_nonDB_graphic::save(GBDATA *, const char *) {
    return "AWT_nonDB_graphic cannot be saved";
}

int AWT_nonDB_graphic::check_update(GBDATA *) {
#if defined(DEBUG)
    printf("AWT_nonDB_graphic can't be check for update\n");
#endif // DEBUG
    return -1;
}
void AWT_nonDB_graphic::update(GBDATA *) {
#if defined(DEBUG)
    printf("AWT_nonDB_graphic can't be updated\n");
#endif // DEBUG
}



