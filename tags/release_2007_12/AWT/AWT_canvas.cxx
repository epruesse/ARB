#include <stdio.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>

#include <algorithm>

using namespace std;
using namespace AW;

void AWT_motion_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2);
void AWT_clip_expose(AW_window *aww,AWT_canvas *ntw,
                     int left_border, int right_border,
                     int top_border, int bottom_border,
                     int hor_overlap, int ver_overlap);
void AWT_expose_cb(AW_window *dummy,AWT_canvas *ntw, AW_CL cl2);
void AWT_resize_cb(AW_window *dummy,AWT_canvas *ntw, AW_CL cl2);
void AWT_focus_cb(AW_window *dummy,AWT_canvas *ntw);
void AWT_input_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2);
void AWT_motion_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2);
void AWT_scroll_vert_cb( AW_window *aww, AWT_canvas* ntw, AW_CL cl1);
void AWT_scroll_hor_cb( AW_window *aww, AWT_canvas* ntw, AW_CL cl1);


void AWT_graphic_exports::clear(){
    zoom_reset       = 0;
    resize           = 0;
    refresh          = 0;
    save             = 0;
    structure_change = 0;
}

void AWT_graphic_exports::init() {
    clear();
    dont_fit_x       = 0;
    dont_fit_y       = 0;
    dont_fit_larger  = 0;
    dont_scroll      = 0;
}

void
AWT_canvas::set_horizontal_scrollbar_position(AW_window *dummy, int pos)
{
    AWUSE(dummy);
    if(pos>worldsize.r - rect.r - 1.0) pos = (int)(worldsize.r - rect.r - 2.0);
    if(pos<0) pos = 0;
    aww->set_horizontal_scrollbar_position(pos);
}

void
AWT_canvas::set_vertical_scrollbar_position(AW_window *dummy, int pos)
{
    AWUSE(dummy);
    if(pos>worldsize.b - rect.b - 1.0 ) pos = (int)(worldsize.b - rect.b - 2.0);
    if(pos<0) pos = 0;
    aww->set_vertical_scrollbar_position(pos);
}

void
AWT_canvas::set_scrollbars( )
    //
{
    AW_pos width = this->worldinfo.r - this->worldinfo.l;
    AW_pos height = this->worldinfo.b - this->worldinfo.t;

    worldsize.l = 0;
    worldsize.r = width*this->trans_to_fit +
        tree_disp->exports.left_offset + tree_disp->exports.right_offset;
    worldsize.t = 0;
    AW_pos scale = this->trans_to_fit;
    if (tree_disp->exports.dont_fit_y) {
        scale = 1.0;
    }
    worldsize.b = height*scale + tree_disp->exports.top_offset + tree_disp->exports.bottom_offset;

    aww->tell_scrolled_picture_size(worldsize);

    aww->calculate_scrollbars();

    this->old_hor_scroll_pos = (int)((-this->worldinfo.l -
                                      this->shift_x_to_fit)*
                                     this->trans_to_fit +
                                     tree_disp->exports.left_offset);
    this->set_horizontal_scrollbar_position(this->aww, old_hor_scroll_pos);

    this->old_vert_scroll_pos = (int)((-this->worldinfo.t -
                                       this->shift_y_to_fit)*
                                      this->trans_to_fit+
                                      tree_disp->exports.top_offset);

    this->set_vertical_scrollbar_position(this->aww, old_vert_scroll_pos);
}

void AWT_canvas::init_device(AW_device *device) {
    device->reset();
    device->shift(AW::Vector(shift_x_to_fit, shift_y_to_fit));
    device->zoom(this->trans_to_fit);
}

void AWT_canvas::zoom_reset( void )
{
    GB_transaction dummy(this->gb_main);

    AW_device *device = aww->get_size_device (AW_MIDDLE_AREA);
    device->set_filter(AW_SIZE);
    device->reset();
    this->tree_disp->show(device);
    device->get_size_information(&(this->worldinfo));

    AW_pos width  = this->worldinfo.r - this->worldinfo.l;
    AW_pos height = this->worldinfo.b - this->worldinfo.t;

    device->get_area_size(&(this->rect));   // real world size (no offset)

    AW_pos net_window_width  = rect.r - rect.l - (tree_disp->exports.left_offset + tree_disp->exports.right_offset);
    AW_pos net_window_height = rect.b - rect.t - (tree_disp->exports.top_offset + tree_disp->exports.bottom_offset);

    if (net_window_width<AWT_MIN_WIDTH) net_window_width   = AWT_MIN_WIDTH;
    if (net_window_height<AWT_MIN_WIDTH) net_window_height = AWT_MIN_WIDTH;

    if (width <EPS) width   = EPS;
    AW_pos x_scale          = net_window_width/width;
    if (height <EPS) height = EPS;
    AW_pos y_scale          = net_window_height/height;

    if (tree_disp->exports.dont_fit_larger) {
        if (width>height) {     // like dont_fit_x = 1; dont_fit_y = 0;
            x_scale = y_scale;
        }
        else {                  // like dont_fit_y = 1; dont_fit_x = 0;
            y_scale = x_scale;
        }
    }
    else {
        if (tree_disp->exports.dont_fit_x) {
            if (tree_disp->exports.dont_fit_y) {
                x_scale = y_scale = 1.0;
            }
            else {
                x_scale = y_scale;
            }
        }
        else {
            if (tree_disp->exports.dont_fit_y) {
                y_scale = x_scale;
            }
            else {
                ;
            }
            //             if (tree_disp->exports.dont_fit_y) { // Ralf: old version (IMHO wrong)
            //                 ;
            //             }else{
            //                 if (y_scale < x_scale) x_scale = y_scale;
            //             }
        }
    }

    this->trans_to_fit = x_scale;

    // complete, upper left corner
    this->shift_x_to_fit = - this->worldinfo.l + tree_disp->exports.left_offset/x_scale;
    this->shift_y_to_fit = - this->worldinfo.t + tree_disp->exports.top_offset/x_scale;

    this->old_hor_scroll_pos  = 0;
    this->old_vert_scroll_pos = 0;

    // scale

    this->set_scrollbars();
}

void
AWT_canvas::recalc_size( void ){
    GB_transaction dummy(this->gb_main);
    AW_device *device = aww->get_size_device (AW_MIDDLE_AREA);
    device->set_filter(AW_SIZE);
    device->reset();

    this->tree_disp->show(device);
    device->get_size_information(&(this->worldinfo));

    device->get_area_size(&(this->rect));   // real world size (no offset)
    this->set_scrollbars();
}

void AWT_canvas::zoom(AW_device *device, bool zoomIn, const Rectangle& wanted_part, const Rectangle& current_part) {
    // zooms the device.
    //
    // zoomIn == true -> wanted_part is zoomed to current_part
    // zoomIn == false -> current_part is zoomed to wanted_part
    //
    // If wanted_part is very small -> assume mistake (act like single click)
    // Single click zooms by 10% centering on click position

    init_device(device);

    AW_pos width  = worldinfo.r-worldinfo.l;
    AW_pos height = worldinfo.b-worldinfo.t;

    if (width<EPS) width = EPS;
    if (height<EPS) height = EPS;

    bool takex = true;
    bool takey = true;

    if (tree_disp) {
        if (tree_disp->exports.dont_fit_y) takey = false;
        if (tree_disp->exports.dont_fit_x) takex = false;
        if (tree_disp->exports.dont_fit_larger) {
            if (width>height)   takey = false;
            else                takex = false;
        }
    }

    awt_assert(takex||takey); // illegal fit combination ?
    
    Rectangle current(device->rtransform(current_part));
    Rectangle wanted;

    bool isClick = false;
    if (takex) {
        if (takey) {
            if (wanted_part.line_vector().length()<40.0) isClick = true;
        }
        else {
            if (wanted_part.width()<30.0) isClick = true;
        }
    }
    else {
        if (wanted_part.height()<30.0) isClick = true;
    }

    if (isClick) { // very small part or single click
        // -> zomm by 10 % on click position
        Vector wanted_diagonal = current.diagonal()*0.45;

        Position clickPos     = device->rtransform(wanted_part.centroid());
        Position screenCenter = current.centroid();
        
        Vector center2click(screenCenter, clickPos);
        Vector center2click_zoomed = center2click / 0.9;

        Position clickPos_zoomed = screenCenter+center2click_zoomed;
        Vector   to_zoomed(clickPos, clickPos_zoomed);

        Position zoomedCenter = screenCenter+to_zoomed;

        // wanted = Rectangle(clickPos-wanted_diagonal, 2*wanted_diagonal);
        // zoom-rectangle around center
        wanted = Rectangle(zoomedCenter-wanted_diagonal, 2*wanted_diagonal);

        
    }
    else {
        wanted = Rectangle(device->rtransform(wanted_part));
    }

    if (!zoomIn) {
        // calculate big rectangle (outside of viewport), which is zoomed into viewport
        double    factor = current.diagonal().length()/wanted.diagonal().length();
        Vector    curr2wanted(current.start(), wanted.start());
        Rectangle big(current.start()+(curr2wanted*-factor), current.diagonal()*factor);

        wanted = big;
    }

    // scroll
    
    shift_x_to_fit = takex ? -wanted.start().xpos() : 0;
    shift_y_to_fit = takey ? -wanted.start().ypos() : 0;

    // scale
    if ((rect.r-rect.l)<EPS) rect.r = rect.l+1;
    if ((rect.b-rect.t)<EPS) rect.b = rect.t+1;

    AW_pos max_trans_to_fit;
    if (takex) {
        if (takey) { // take both
            trans_to_fit     = max((rect.r-rect.l)/wanted.width(), (rect.b-rect.t)/wanted.height());
            max_trans_to_fit = 32000.0/max(width, height);
        }
        else { // takex
            trans_to_fit = (rect.r-rect.l)/wanted.width();
            max_trans_to_fit = 32000.0/width;
        }
    }
    else { // takey
        trans_to_fit = (rect.b-rect.t)/wanted.height();
        max_trans_to_fit = 32000.0/height;
    }
    if (trans_to_fit > max_trans_to_fit) {
        trans_to_fit = max_trans_to_fit;
    }

    set_scrollbars();
}

inline void nt_draw_zoom_box(AW_device *device, int gc, AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2 ) {
    device->box(gc, AW_FALSE, x1, y1, x2-x1, y2-y1, AWT_F_ALL, 0, 0);
}
inline void nt_draw_zoom_box(AW_device *device, AWT_canvas *ntw) {
    nt_draw_zoom_box(device, ntw->drag_gc,
                     ntw->zoom_drag_sx, ntw->zoom_drag_sy,
                     ntw->zoom_drag_ex, ntw->zoom_drag_ey);
}

void AWT_clip_expose(AW_window *aww,AWT_canvas *ntw,
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

    device->clear_part(left_border,top_border,right_border-left_border,
                       bottom_border-top_border, -1);

    GB_transaction dummy(ntw->gb_main);

    if (ntw->tree_disp->check_update(ntw->gb_main)>0){
        ntw->zoom_reset();
    }

    ntw->init_device(device);

    if ( hor_overlap> 0.0) {
        device->set_right_clip_border(right_border + hor_overlap);
    }
    if ( hor_overlap< 0.0) {
        device->set_left_clip_border(left_border + hor_overlap);
    }
    if ( ver_overlap> 0.0) {
        device->set_bottom_clip_border(bottom_border + ver_overlap);
    }
    if ( ver_overlap< 0.0) {
        device->set_top_clip_border(top_border + ver_overlap);
    }
    ntw->tree_disp->show(device);
}

void AWT_expose_cb(AW_window *dummy,AWT_canvas *ntw, AW_CL){
    AWUSE(dummy);
    ntw->refresh();
}

void AWT_canvas::refresh( void )
{
    AW_device *device = this->aww->get_device (AW_MIDDLE_AREA);
    device->clear(-1);
    AWT_clip_expose(this->aww, this, this->rect.l, this->rect.r,
                    this->rect.t, this->rect.b,0,0);
}

void AWT_resize_cb(AW_window *dummy,AWT_canvas *ntw, AW_CL)
{
    AWUSE(dummy);
    ntw->zoom_reset();
    AWT_expose_cb(ntw->aww, ntw, 0);
}



void AWT_focus_cb(AW_window *dummy,AWT_canvas *ntw){
    AWUSE(dummy);
    if (!ntw->gb_main) return;
    ntw->tree_disp->push_transaction(ntw->gb_main);

    int flags = ntw->tree_disp->check_update(ntw->gb_main);
    if (flags){
        ntw->recalc_size();
        ntw->refresh();
    }
    ntw->tree_disp->pop_transaction(ntw->gb_main);
}

static bool handleZoomEvent(AW_window *aww, AWT_canvas *ntw, AW_device *device, const AW_event& event) {
    bool handled = false;
    bool zoomIn  = true;

    if      (event.button == AWT_M_LEFT)  { handled = true; }
    else if (event.button == AWT_M_RIGHT) { handled = true; zoomIn  = false; }

    if (handled) {
        if (event.type == AW_Mouse_Press) {
            ntw->drag = 1;
            ntw->zoom_drag_sx = ntw->zoom_drag_ex = event.x;
            ntw->zoom_drag_sy = ntw->zoom_drag_ey = event.y;
        }
        else {
            /* delete last box */
            nt_draw_zoom_box(device, ntw);
            ntw->drag = 0;

            Rectangle screen(ntw->rect);
            Rectangle drag(ntw->zoom_drag_sx, ntw->zoom_drag_sy, ntw->zoom_drag_ex, ntw->zoom_drag_ey);

            ntw->zoom(device, zoomIn, drag, screen);
            AWT_expose_cb(aww, ntw, 0);
        }
    }
    return handled;
}

void AWT_input_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2)
{
    AWUSE(cd2);
    AW_event event;
    AW_device *device, *click_device;

    aww->get_event( &event );
    device = aww->get_device (AW_MIDDLE_AREA  );

    device->set_filter(AW_SCREEN);
    device->reset();

    ntw->tree_disp->exports.clear();
    if (ntw->gb_main) ntw->tree_disp->push_transaction(ntw->gb_main);

    ntw->tree_disp->check_update(ntw->gb_main);

    /*** here appear all modes which must be handled right here ***/

    bool event_handled = false;

    if (ntw->mode == AWT_MODE_ZOOM) { // zoom mode is identical for all applications, so handle it here
        event_handled = handleZoomEvent(aww, ntw, device, event);
    }

    if (!event_handled) {
        click_device = aww->get_click_device (AW_MIDDLE_AREA,event.x, event.y, AWT_CATCH_LINE, AWT_CATCH_TEXT, 0);
        click_device->set_filter(AW_CLICK);
        device->set_filter(AW_SCREEN);

        ntw->init_device(click_device);
        ntw->init_device(device);

        ntw->tree_disp->show(click_device);
        click_device->get_clicked_line(&ntw->clicked_line);
        click_device->get_clicked_text(&ntw->clicked_text);

        ntw->tree_disp->command(device, ntw->mode,
                                event.button, event.keymodifier, event.keycode, event.character,
                                event.type, event.x,
                                event.y, &ntw->clicked_line,
                                &ntw->clicked_text );
        if (ntw->tree_disp->exports.save ) {
            // save it
            GB_ERROR error = ntw->tree_disp->save(ntw->gb_main, 0,0,0);
            if (error) {
                aw_message(error);
                ntw->tree_disp->load(ntw->gb_main, 0,0,0);
            }
        }
        if (ntw->gb_main) {
            ntw->tree_disp->update(ntw->gb_main);
        }
        if (ntw->tree_disp->exports.zoom_reset) {
            ntw->zoom_reset();
            ntw->refresh();
        }
        else if (ntw->tree_disp->exports.resize) {
            ntw->recalc_size();
            ntw->refresh();
        }
        else if (ntw->tree_disp->exports.refresh) {
            ntw->refresh();
        }
    }

    ntw->zoom_drag_ex = event.x;
    ntw->zoom_drag_ey = event.y;
    if (ntw->gb_main) {
        ntw->tree_disp->pop_transaction(ntw->gb_main);
    }
}


void AWT_canvas::set_dragEndpoint(int dragx, int dragy) {
    bool fit_proportional = false;
    if (tree_disp) {
        bool dont_fit_x = tree_disp->exports.dont_fit_x;
        bool dont_fit_y = tree_disp->exports.dont_fit_y;

        if (tree_disp->exports.dont_fit_larger) {
            AW_pos width  = worldinfo.r-worldinfo.l;
            AW_pos height = worldinfo.b-worldinfo.t;

            if (width>height) {     // like dont_fit_x = 1; dont_fit_y = 0;
                dont_fit_x = true;
            }
            else { // like dont_fit_y = 1; dont_fit_x = 0;
                dont_fit_y = true;
            }
        }

        if (dont_fit_y) {
            zoom_drag_sy = rect.t;
            zoom_drag_ey = rect.b;
            zoom_drag_ex = dragx;
        }
        else if (dont_fit_x) {
            zoom_drag_sx = rect.l;
            zoom_drag_ex = rect.r;
            zoom_drag_ey = dragy;
        }
        else {
            fit_proportional = true;
        }
    }
    else {
        fit_proportional = true;
    }

    if (fit_proportional) {
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
            int width    = int(scr_sx*factor);
            zoom_drag_ex = zoom_drag_sx+width;
        }
        else if (correct_y) {
            int height   = int(scr_sy*factor);
            zoom_drag_ey = zoom_drag_sy+height;
        }
    }
}

void AWT_motion_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2)
{
    AWUSE(cd2);
    AW_event event;
    AW_device *device,*click_device;
    int dx, dy;

    device = aww->get_device (AW_MIDDLE_AREA  );
    device->reset();
    device->set_filter(AW_SCREEN);

    if (ntw->gb_main) ntw->tree_disp->push_transaction(ntw->gb_main);
    aww->get_event( &event );

    if (event.button == AWT_M_MIDDLE) {
        // shift display in ALL modes
        dx = event.x - ntw->zoom_drag_ex;
        dy = event.y - ntw->zoom_drag_ey;

        ntw->zoom_drag_ex = event.x;
        ntw->zoom_drag_ey = event.y;


        /* display */
        ntw->scroll(aww, -dx *3, -dy *3);
    }
    else {
        bool run_command = true;

        if (event.button == AWT_M_LEFT || event.button == AWT_M_RIGHT) {
            switch (ntw->mode) {
                case AWT_MODE_ZOOM:
                    nt_draw_zoom_box(device, ntw);
                    ntw->set_dragEndpoint(event.x, event.y);
                    nt_draw_zoom_box(device, ntw);
                    run_command = false;
                    break;

                case AWT_MODE_SWAP2:
                    if (event.button == AWT_M_RIGHT) break;
                    // fall-through
                case AWT_MODE_MOVE:
                    ntw->init_device(device);
                    click_device = aww->get_click_device (AW_MIDDLE_AREA,
                                                          event.x, event.y, AWT_CATCH_LINE,
                                                          AWT_CATCH_TEXT, 0);
                    click_device->set_filter(AW_CLICK_DRAG);
                    ntw->init_device(click_device);
                    ntw->tree_disp->show(click_device);
                    click_device->get_clicked_line(&ntw->clicked_line);
                    click_device->get_clicked_text(&ntw->clicked_text);
                    run_command  = false;
                    break;
                    
                default :
                    break;
            }
        }

        if (run_command) {
            ntw->init_device(device);
            ntw->tree_disp->command(device, ntw->mode,
                                    event.button, event.keymodifier, event.keycode, event.character, AW_Mouse_Drag, event.x,
                                    event.y, &ntw->clicked_line,
                                    &ntw->clicked_text );
            if (ntw->gb_main) {
                ntw->tree_disp->update(ntw->gb_main);
            }
        }
    }
    
    if (ntw->tree_disp->exports.zoom_reset) {
        ntw->zoom_reset();
        ntw->refresh();
    }
    else if (ntw->tree_disp->exports.resize) {
        ntw->recalc_size();
        ntw->refresh();
    }
    else if (ntw->tree_disp->exports.refresh) {
        ntw->refresh();
    }

    if (ntw->gb_main) ntw->tree_disp->pop_transaction(ntw->gb_main);
}

void
AWT_canvas::scroll( AW_window *dummy, int dx, int dy,AW_BOOL dont_update_scrollbars)
{
    AWUSE(dummy);

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

    /* compute move area params */

    if(dx>0){
        csx = dx;
        cdx = 0;
        cwidth = screenwidth-dx;
    }else{
        csx = 0;
        cdx = -dx;
        cwidth = screenwidth+dx;
    }
    if(dy>0){
        csy = dy;
        cdy = 0;
        cheight = screenheight-dy;
    }else{
        csy = 0;
        cdy = -dy;
        cheight = screenheight+dy;
    }

    /* move area */
    if (!tree_disp->exports.dont_scroll){
        device->move_region( csx, csy, cwidth, cheight, cdx, cdy);
        /* redraw stripes */
        this->shift_x_to_fit -= dx/this->trans_to_fit;
        this->shift_y_to_fit -= dy/this->trans_to_fit;

        // x-stripe
        if((int)dx>0){
            AWT_clip_expose(aww, this, screenwidth-dx, screenwidth,
                            0, screenheight,
                            -CLIP_OVERLAP , 0);
        }
        if((int)dx<0){
            AWT_clip_expose(aww, this,  0, -dx,
                            0, screenheight,
                            CLIP_OVERLAP,0);
        }

        // y-stripe
        if((int)dy>0){
            AWT_clip_expose(aww, this, 0, screenwidth,
                            screenheight-dy, screenheight,
                            0,-CLIP_OVERLAP);
        }
        if((int)dy<0){
            AWT_clip_expose(aww, this,  0, screenwidth,
                            0,  -dy,
                            0,  CLIP_OVERLAP);
        }
    }else{          // redraw everything
        /* redraw stripes */
        this->shift_x_to_fit -= dx/this->trans_to_fit;
        this->shift_y_to_fit -= dy/this->trans_to_fit;
        AWT_expose_cb(aww, this,  0);
    }
    this->refresh();
}

void
AWT_scroll_vert_cb( AW_window *aww, AWT_canvas* ntw, AW_CL cl1)
{
    AWUSE(cl1);
    int delta_screen_y;

    int new_vert = aww->slider_pos_vertical;
    delta_screen_y = (new_vert - ntw->old_vert_scroll_pos) ;


    ntw->scroll(aww, 0, delta_screen_y, AW_TRUE);

    ntw->old_vert_scroll_pos = (int)new_vert;

}

void
AWT_scroll_hor_cb( AW_window *aww, AWT_canvas* ntw, AW_CL cl1)
{
    AWUSE(cl1);
    int delta_screen_x;

    int new_hor = aww->slider_pos_horizontal;
    delta_screen_x = (new_hor - ntw->old_hor_scroll_pos) ;

    ntw->scroll(aww, delta_screen_x, 0, AW_TRUE);

    ntw->old_hor_scroll_pos = new_hor;
}


AWT_canvas::AWT_canvas(GBDATA *gb_maini, AW_window *awwi, AWT_graphic *awd, AW_gc_manager &set_gc_manager, const char *user_awari)
    : user_awar(strdup(user_awari))
    , shift_x_to_fit(0)
    , shift_y_to_fit(0)
    , gb_main(gb_maini)
    , aww(awwi)
    , awr(aww->get_root())
    , tree_disp(awd)
    , gc_manager(tree_disp->init_devices(aww, aww->get_device (AW_MIDDLE_AREA), this, (AW_CL)0))
    , drag_gc(aww->main_drag_gc)
    , mode(AWT_MODE_NONE)
{
    tree_disp->drag_gc  = drag_gc;
    set_gc_manager      = gc_manager;

    memset((char *)&clicked_line,0,sizeof(clicked_line));
    memset((char *)&clicked_text,0,sizeof(clicked_text));

    AWT_resize_cb(aww, this, 0);

    aww->set_expose_callback (AW_MIDDLE_AREA, (AW_CB)AWT_expose_cb, (AW_CL)this, 0);
    aww->set_resize_callback (AW_MIDDLE_AREA,(AW_CB)AWT_resize_cb, (AW_CL)this, 0);
    aww->set_input_callback (AW_MIDDLE_AREA,(AW_CB)AWT_input_event,(AW_CL)this, 0 );
    aww->set_focus_callback ((AW_CB)AWT_focus_cb,(AW_CL)this, 0 );

    aww->set_motion_callback (AW_MIDDLE_AREA,(AW_CB)AWT_motion_event,(AW_CL)this, 0 );
    aww->set_horizontal_change_callback((AW_CB)AWT_scroll_hor_cb,(AW_CL)this, 0 );
    aww->set_vertical_change_callback((AW_CB)AWT_scroll_vert_cb,(AW_CL)this, 0 );
}

// --------------------
//      AWT_graphic
// --------------------

AWT_graphic::AWT_graphic(void) {
    exports.init();
}
AWT_graphic::~AWT_graphic(void) {
}

void AWT_graphic::pop_transaction(GBDATA *gb_main)  {
    GB_pop_transaction(gb_main);
}
void AWT_graphic::push_transaction(GBDATA *gb_main) {
    GB_push_transaction(gb_main);
}

void AWT_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char, AW_event_type type,AW_pos x, AW_pos y,
                          AW_clicked_line *cl, AW_clicked_text *ct){
    AWUSE(device);
    AWUSE(type);
    AWUSE(button);
    AWUSE(key_modifier);
    AWUSE(key_char);
    AWUSE(cmd);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);
}

void AWT_graphic::text(AW_device *device, char *text){
    AWUSE(device);
    AWUSE(text);
}

// --------------------------
//      AWT_nonDB_graphic
// --------------------------

AWT_nonDB_graphic::~AWT_nonDB_graphic() {}

GB_ERROR AWT_nonDB_graphic::load(GBDATA *, const char *, AW_CL, AW_CL) {
    return "AWT_nonDB_graphic cannot be loaded";
}

GB_ERROR AWT_nonDB_graphic::save(GBDATA *, const char *, AW_CL, AW_CL) {
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


