#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_window.hxx>

#include <ed4_extern.hxx>

#include "ed4_class.hxx"
#include "ed4_tools.hxx"

int ED4_window::no_of_windows = 0;                  // static variable has to be initialized only once

//*****************************************
//* ED4_window Methods      beginning *
//*****************************************

// void ED4_window::redraw()
// {
//     if (ED4_ROOT->main_manager) {
//  AW_window *old_aww = ED4_ROOT->temp_aww;
//  AW_device *old_device = ED4_ROOT->temp_device;
//  ED4_window *old_window = ED4_ROOT->temp_ed4w;

//  ED4_ROOT->temp_aww = aww;
//  ED4_ROOT->temp_device = aww->get_device(AW_MIDDLE_AREA);
//  ED4_ROOT->temp_ed4w = this;

//  ED4_ROOT->main_manager->update_info.clear_at_refresh = 1;
//  ED4_ROOT->main_manager->Show(1);

//  ED4_ROOT->temp_aww = old_aww;
//  ED4_ROOT->temp_device = old_device;
//  ED4_ROOT->temp_ed4w = old_window;
//     }
// }

void ED4_window::reset_all_for_new_config()
{
    horizontal_fl           = NULL;
    vertical_fl             = NULL;
    scrolled_rect.scroll_top        = NULL;
    scrolled_rect.scroll_bottom = NULL;
    scrolled_rect.scroll_left   = NULL;
    scrolled_rect.scroll_right  = NULL;
    scrolled_rect.world_x       = 0;
    scrolled_rect.world_y       = 0;
    scrolled_rect.width         = 0;
    scrolled_rect.height        = 0;
    slider_pos_horizontal       = 0;
    slider_pos_vertical         = 0;
    coords.top_area_x           = 0;
    coords.top_area_y           = 0;
    coords.top_area_height      = 0;
    coords.middle_area_x        = 0;
    coords.middle_area_y        = 0;
    coords.window_width     = 0;
    coords.window_height        = 0;
    coords.window_upper_clip_point  = 0;
    coords.window_lower_clip_point  = 0;
    coords.window_left_clip_point   = 0;
    coords.window_right_clip_point  = 0;

    ED4_ROOT->aw_root->awar(awar_path_for_cursor)->write_int(0);
    ED4_ROOT->aw_root->awar(awar_path_for_Ecoli)->write_int(0);
    ED4_ROOT->aw_root->awar(awar_path_for_basePos)->write_int(0);
    ED4_ROOT->aw_root->awar(awar_path_for_IUPAC)->write_string(IUPAC_EMPTY);
    ED4_ROOT->aw_root->awar(awar_path_for_helixNr)->write_string("");

    if (next) next->reset_all_for_new_config();
}


void ED4_window::update_window_coords()
{
    AW_pos  x,y;
    AW_rectangle    area_size;

    ED4_ROOT->top_area_man->calc_world_coords( &x, &y );

    coords.top_area_x       = (long) x;
    coords.top_area_y       = (long) y;
    coords.top_area_height      = (long) ED4_ROOT->top_area_man->extension.size[HEIGHT];

    ED4_ROOT->middle_area_man->calc_world_coords( &x, &y );

    coords.middle_area_x        = (long) x;
    coords.middle_area_y        = (long) y;

    aww->_get_area_size (AW_MIDDLE_AREA, &area_size);

    coords.window_width         = area_size.r;
    coords.window_height        = area_size.b;
                                                            // world coordinates
    coords.window_upper_clip_point  = coords.middle_area_y + (long) aww->slider_pos_vertical; //coordinate of upper clipping point of middle area
    coords.window_lower_clip_point  = coords.window_upper_clip_point + coords.window_height - coords.middle_area_y;

    if (ED4_ROOT->scroll_links.link_for_hor_slider)
    {
        ED4_ROOT->scroll_links.link_for_hor_slider->calc_world_coords( &x, &y );
        coords.window_left_clip_point = (long) x + aww->slider_pos_horizontal;
        coords.window_right_clip_point = coords.window_left_clip_point + coords.window_width - (long) x;
    }

#if defined(DEBUG) && 0
    printf("left %d right %d (xdiff=%d) upper %d lower %d (ydiff=%d) width=%d height=%d\n",
           coords.window_left_clip_point,
           coords.window_right_clip_point,
           coords.window_right_clip_point-coords.window_left_clip_point,
           coords.window_upper_clip_point,
           coords.window_lower_clip_point,
           coords.window_lower_clip_point-coords.window_upper_clip_point,
           coords.window_width,
           coords.window_height);
#endif
}



ED4_folding_line* ED4_window::insert_folding_line( AW_pos world_x,
                           AW_pos world_y,
                           AW_pos length,
                           AW_pos dimension,
                           ED4_base *link,
                           ED4_properties prop )
{
    ED4_folding_line  *fl, *current_fl, *previous_fl = NULL;
    AW_pos             x, y;
    int                rel_pos;

    fl = new ED4_folding_line;
    fl->link = link;

    if ( link != NULL ) {
        link->update_info.linked_to_folding_line = 1;
        link->calc_world_coords( &x, &y );
        fl->world_pos[X_POS] = x;
        fl->world_pos[Y_POS] = y;

        if ( prop == ED4_P_VERTICAL ) {
            fl->length = link->extension.size[HEIGHT];
        }
        else {
            fl->length = link->extension.size[WIDTH];
        }
    }
    else
    {
        fl->world_pos[X_POS] = world_x;
        fl->world_pos[Y_POS] = world_y;
        fl->length = length;
    }

    fl->dimension = dimension;

    if ( prop == ED4_P_VERTICAL ) {
        fl->window_pos[X_POS] = fl->world_pos[X_POS] - dimension;
        fl->window_pos[Y_POS] = fl->world_pos[Y_POS];

        if ( (current_fl = vertical_fl) == NULL ) {
            vertical_fl = fl;
            return ( fl );
        }

        rel_pos = X_POS;
    }
    else {
        if ( prop == ED4_P_HORIZONTAL ) {
            fl->window_pos[Y_POS] = fl->world_pos[Y_POS] - dimension;
            fl->window_pos[X_POS] = fl->world_pos[X_POS];

            if ( (current_fl = horizontal_fl) == NULL ) {
                horizontal_fl = fl;
                return ( fl );
            }

            rel_pos = Y_POS;
        }
        else {
            return ( NULL );
        }
    }

    while (  (current_fl != NULL) && (current_fl->world_pos[rel_pos] <= fl->world_pos[rel_pos]) ) {
        previous_fl = current_fl;
        current_fl  = current_fl->next;
    }

    if (previous_fl){
        previous_fl->next = fl;
    }
    fl->next = current_fl;

    return ( fl );
}

ED4_window *ED4_window::get_matching_ed4w(AW_window *aw) {
    ED4_window *window = ED4_ROOT->first_window;
    while (window && window->aww != aw) window = window->next;
    return window;
}



ED4_returncode ED4_window::delete_folding_line(ED4_folding_line *fl, ED4_properties prop) {
    AWUSE(fl);
    AWUSE(prop);

    return ( ED4_R_OK );
}

ED4_returncode ED4_window::update_scrolled_rectangle( void )
{
    AW_pos world_x,
    world_y,
    width,
    height,
    dummy,
    dim,
    delta;
//      ED4_properties  scroll_prop;
    AW_world        rect;
    AW_rectangle    area_size;

    if ((scrolled_rect.scroll_top == NULL) || (scrolled_rect.scroll_bottom == NULL) ||
    (scrolled_rect.scroll_left == NULL) || (scrolled_rect.scroll_right == NULL) )
    return ED4_R_IMPOSSIBLE;

    world_x = scrolled_rect.world_x;
    world_y = scrolled_rect.world_y;
    width = scrolled_rect.scroll_right->world_pos[X_POS] - scrolled_rect.scroll_left->world_pos[X_POS] + 1;
    height = scrolled_rect.scroll_bottom->world_pos[Y_POS] - scrolled_rect.scroll_top->world_pos[Y_POS] + 1;


    if ( scrolled_rect.x_link != NULL ) // calculate possibly new world coordinates and extensions of existing links
    scrolled_rect.x_link->calc_world_coords( &world_x, &dummy );

    if ( scrolled_rect.y_link != NULL )
    scrolled_rect.y_link->calc_world_coords( &dummy, &world_y );

    if ( scrolled_rect.width_link != NULL )
    width = scrolled_rect.width_link->extension.size[WIDTH];

    if ( scrolled_rect.height_link != NULL )
    height = scrolled_rect.height_link->extension.size[HEIGHT];

    scrolled_rect.scroll_top->world_pos[X_POS] = world_x;           // set new world and window coordinates
    scrolled_rect.scroll_top->world_pos[Y_POS] = world_y;
    scrolled_rect.scroll_top->window_pos[X_POS] = world_x;
    scrolled_rect.scroll_top->window_pos[Y_POS] = world_y;

    if ( scrolled_rect.scroll_top->length != INFINITE )
    scrolled_rect.scroll_top->length = width;

    scrolled_rect.scroll_bottom->world_pos[X_POS] = world_x;
    scrolled_rect.scroll_bottom->world_pos[Y_POS] = world_y + height - 1;

    if ( scrolled_rect.scroll_bottom->length != INFINITE )
    scrolled_rect.scroll_bottom->length = width;

    scrolled_rect.scroll_left->world_pos[X_POS] = world_x;
    scrolled_rect.scroll_left->world_pos[Y_POS] = world_y;
    scrolled_rect.scroll_left->window_pos[X_POS] = world_x;
    scrolled_rect.scroll_left->window_pos[Y_POS] = world_y;

    if ( scrolled_rect.scroll_left->length != INFINITE )
    scrolled_rect.scroll_left->length = height;

    scrolled_rect.scroll_right->world_pos[X_POS] = world_x + width - 1;
    scrolled_rect.scroll_right->world_pos[Y_POS] = world_y;

    if ( scrolled_rect.scroll_right->length != INFINITE )
    scrolled_rect.scroll_right->length = height;

    scrolled_rect.world_x = world_x;
    scrolled_rect.world_y = world_y;
    scrolled_rect.width   = width;
    scrolled_rect.height  = height;

    rect.t = 0;                                 // update window scrollbars
    rect.l = 0;
    rect.r = width - 1;
    rect.b = height - 1;

    set_scrollbar_indents();
    aww->tell_scrolled_picture_size( rect );
    aww->calculate_scrollbars();

    delta = aww->slider_pos_horizontal - slider_pos_horizontal;     // update dimension and window position of folding lines at
    scrolled_rect.scroll_left->dimension += delta;                  // the borders of scrolled rectangle
    delta = aww->slider_pos_vertical - slider_pos_vertical;
    scrolled_rect.scroll_top->dimension += delta;
    ED4_ROOT->get_device()->get_area_size( &area_size );

    if ( scrolled_rect.height_link != NULL )
    slider_pos_vertical = aww->slider_pos_vertical;

    if ( scrolled_rect.width_link != NULL )
    slider_pos_horizontal = aww->slider_pos_horizontal;


    if ( (world_y + height - 1) > area_size.b )                 // our world doesn't fit vertically in our window =>
    {                                       // determine dimension of bottom folding line
    dim = (world_y + height - 1) - area_size.b;
    scrolled_rect.scroll_bottom->dimension = max(0, int(dim - scrolled_rect.scroll_top->dimension));
    }
    else
    {
    dim = 0;
    scrolled_rect.scroll_bottom->dimension = 0;
    scrolled_rect.scroll_top->dimension = 0;
    }

    scrolled_rect.scroll_bottom->window_pos[Y_POS] = world_y + height - 1 - dim;
    scrolled_rect.scroll_bottom->window_pos[X_POS] = world_x;

    if ( (world_x + width - 1) > area_size.r )                  // our world doesn't fit horizontally in our window =>
    {                                       // determine dimension of right folding line
    dim = (world_x + width - 1) - area_size.r;
    scrolled_rect.scroll_right->dimension = max(0, int(dim - scrolled_rect.scroll_left->dimension));
    }
    else
    {
    dim = 0;
    scrolled_rect.scroll_right->dimension = 0;
    scrolled_rect.scroll_left->dimension = 0;
    }

    scrolled_rect.scroll_right->window_pos[X_POS] = world_x + width - 1 - dim;
    scrolled_rect.scroll_right->window_pos[Y_POS] = world_y;

    update_window_coords();

    return ( ED4_R_OK );
}

ED4_returncode ED4_window::set_scrolled_rectangle( AW_pos world_x, AW_pos world_y, AW_pos width, AW_pos height,
                           ED4_base *x_link, ED4_base *y_link, ED4_base *width_link, ED4_base *height_link )
{
    AW_pos         x, y, dim;
    AW_rectangle   area_size;

    if ( scrolled_rect.scroll_top != NULL )                         // first of all remove existing scrolled rectangle
        delete_folding_line( scrolled_rect.scroll_top, ED4_P_HORIZONTAL );

    if ( scrolled_rect.scroll_bottom != NULL )
        delete_folding_line( scrolled_rect.scroll_bottom, ED4_P_HORIZONTAL );

    if ( scrolled_rect.scroll_left != NULL )
        delete_folding_line( scrolled_rect.scroll_left, ED4_P_VERTICAL );

    if ( scrolled_rect.scroll_right != NULL )
        delete_folding_line( scrolled_rect.scroll_right, ED4_P_VERTICAL );

    if ( x_link != NULL )
    {
        x_link->update_info.linked_to_scrolled_rectangle = 1;
        x_link->calc_world_coords( &x, &y );
        world_x = x;
    }

    if ( y_link != NULL )
    {
        y_link->update_info.linked_to_scrolled_rectangle = 1;
        y_link->calc_world_coords( &x, &y );
        world_y = y;
    }

    if ( width_link != NULL )
    {
        width_link->update_info.linked_to_scrolled_rectangle = 1;
        width = width_link->extension.size[WIDTH];
    }

    if ( height_link != NULL )
    {
        height_link->update_info.linked_to_scrolled_rectangle = 1;
        height = height_link->extension.size[HEIGHT];
    }

    ED4_ROOT->get_device()->get_area_size( &area_size );

    if ( (area_size.r <= world_x) || (area_size.b <= world_y) )
        return ( ED4_R_IMPOSSIBLE );

    scrolled_rect.scroll_top = insert_folding_line( world_x, world_y, INFINITE, 0, NULL, ED4_P_HORIZONTAL );
    scrolled_rect.scroll_left = insert_folding_line( world_x, world_y, INFINITE, 0, NULL, ED4_P_VERTICAL );

    dim = 0;
    if ( (world_y + height - 1) > area_size.b )
        dim = (world_y + height - 1) - area_size.b;

    scrolled_rect.scroll_bottom = insert_folding_line( world_x, (world_y + height - 1), INFINITE, dim, NULL, ED4_P_HORIZONTAL );

    dim = 0;
    if ( (world_x + width - 1) > area_size.r )
        dim = (world_x + width - 1) - area_size.r;

    scrolled_rect.scroll_right = insert_folding_line( (world_x + width - 1), world_y, INFINITE, dim, NULL, ED4_P_VERTICAL );
    scrolled_rect.x_link = x_link;
    scrolled_rect.y_link = y_link;
    scrolled_rect.width_link = width_link;
    scrolled_rect.height_link = height_link;
    scrolled_rect.world_x = world_x;
    scrolled_rect.world_y = world_y;
    scrolled_rect.width = width;
    scrolled_rect.height = height;

    return( ED4_R_OK );
}

static inline void clear_and_update_rectangle(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2)
// clears and updates any range of the screen (in win coordinates)
// clipping range should be set correctly
{
    AW_rectangle rect;

    rect.t = int(y1);
    rect.b = int(y2);
    rect.l = int(x1);
    rect.r = int(x2);
    ED4_set_clipping_rectangle(&rect);

#if defined(DEBUG) && 0
    static int toggle = 0;
    ED4_ROOT->get_device()->box(ED4_G_COLOR_2+toggle, x1, y1, x2-x1+1, y2-y1+1, (AW_bitset)-1, 0, 0);    // fill range with color (for testing)
    toggle = (toggle+1)&7;
#else
    ED4_ROOT->get_device()->clear_part(x1, y1, x2-x1+1, y2-y1+1, (AW_bitset)-1);
#endif

    ED4_ROOT->main_manager->to_manager()->Show(1,1);
}

static inline void move_and_update_rectangle(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2, int dx, int dy)
// x1/y1=upper-left-corner (win coordinates)
// x2/y2=lower-right-corner
// dx/dy=movement
{
    AW_pos fx = dx<0 ? x1-dx : x1;  // move from position ..
    AW_pos fy = dy<0 ? y1-dy : y1;
    AW_pos tx = dx>0 ? x1+dx : x1;  // ..to position ..
    AW_pos ty = dy>0 ? y1+dy : y1;
    int xs = int(x2-x1-abs(dx));        // ..size
    int ys = int(y2-y1-abs(dy));

    {
        AW_rectangle rect;
        rect.t = int(ty);
        rect.b = int(ty+ys-1);
        rect.l = int(tx);
        rect.r = int(tx+xs-1);
        ED4_set_clipping_rectangle(&rect);
    }

    AW_device *device = ED4_ROOT->get_device();
    device->move_region(fx, fy, xs, ys, tx, ty);

    if (dy<0) { // scroll to the top
        device->set_top_font_overlap(1);
        clear_and_update_rectangle(x1, y2+dy, x2, y2);
        device->set_top_font_overlap(0);
    }
    else if (dy>0) { // scroll to the bottom
        device->set_bottom_font_overlap(1);
        clear_and_update_rectangle(x1, y1, x2, y1+dy);
        device->set_bottom_font_overlap(0);
    }

    int char_width = ED4_ROOT->font_group.get_max_width() * 2;
    if (dx<0) { // scroll left
        device->set_left_font_overlap(1);
        clear_and_update_rectangle(x2+dx-char_width, y1, x2, y2);
        device->set_left_font_overlap(0);
    }
    else if (dx>0) { // scroll right
        device->set_right_font_overlap(1);
        clear_and_update_rectangle(x1, y1, x1+dx+char_width, y2);
        device->set_right_font_overlap(0);
    }
}

static inline void update_rectangle(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2) // x1/y1=upper-left-corner x2/y2=lower-right-corner
{
    AW_rectangle rect;
    rect.t = int(y1);
    rect.b = int(y2);
    rect.l = int(x1);
    rect.r = int(x2);

    ED4_set_clipping_rectangle(&rect);
    clear_and_update_rectangle(x1, y1, x2, y2);
}


ED4_returncode ED4_window::scroll_rectangle( int dx, int dy )
{
    int skip_move;

    if (!dx && !dy) return ED4_R_OK; // scroll not

    AW_pos left_x   = scrolled_rect.scroll_left->window_pos[X_POS];
    AW_pos top_y    = scrolled_rect.scroll_top->window_pos[Y_POS];
    AW_pos right_x  = scrolled_rect.scroll_right->window_pos[X_POS];
    AW_pos bottom_y = scrolled_rect.scroll_bottom->window_pos[Y_POS];

//     e4_assert((scrolled_rect.scroll_left->dimension-dx)>=0); // if any of these asserts fails, dx or dy is set wrong
//     e4_assert((scrolled_rect.scroll_right->dimension+dx)>=0);
//     e4_assert((scrolled_rect.scroll_top->dimension-dy)>=0);
//     e4_assert((scrolled_rect.scroll_bottom->dimension+dy)>=0);

    scrolled_rect.scroll_left->dimension -= dx;
    scrolled_rect.scroll_top->dimension -= dy;
    scrolled_rect.scroll_right->dimension += dx;
    scrolled_rect.scroll_bottom->dimension += dy;

    skip_move = (ABS(int(dy)) > (bottom_y - top_y - 20)) || (ABS(int(dx)) > (right_x - left_x - 20));

    AW_pos leftmost_x = coords.middle_area_x;
    AW_pos toptop_y = coords.top_area_y;
    AW_pos topbottom_y = toptop_y + coords.top_area_height - 1;

    ED4_ROOT->get_device()->push_clip_scale();

    // main area

    if (skip_move)  update_rectangle(left_x, top_y, right_x, bottom_y); // main area
    else            move_and_update_rectangle(left_x, top_y, right_x, bottom_y, int(dx), int(dy)); // main area

    // name area (scroll only vertically)

    if (dy) {
        if (skip_move)  update_rectangle(leftmost_x, top_y, left_x, bottom_y);
        else            move_and_update_rectangle(leftmost_x, top_y, left_x, bottom_y, 0, int(dy));
    }

    // top area (scroll only horizontally)

    if (dx) {
        if (skip_move)  update_rectangle(left_x, toptop_y, right_x, topbottom_y);
        else            move_and_update_rectangle(left_x, toptop_y, right_x, topbottom_y, int(dx), 0);
    }

    ED4_ROOT->get_device()->pop_clip_scale();

    return ( ED4_R_OK );
}

ED4_returncode ED4_window::set_scrollbar_indents( void ) {
    int indent_y, indent_x;

    if ((scrolled_rect.scroll_top    == NULL) ||
        (scrolled_rect.scroll_bottom == NULL) ||
        (scrolled_rect.scroll_left   == NULL) ||
        (scrolled_rect.scroll_right  == NULL) )
    return ( ED4_R_IMPOSSIBLE );

    indent_y = (int) scrolled_rect.world_y + SLIDER_OFFSET;
    aww->set_vertical_scrollbar_top_indent( indent_y );
    indent_x = (int) scrolled_rect.world_x + SLIDER_OFFSET;
    aww->set_horizontal_scrollbar_left_indent( indent_x );

    return ( ED4_R_OK );
}


void ED4_window::delete_window( ED4_window *window) //delete from window list
{
    ED4_window      *temp, *temp2;


    if (window == ED4_ROOT->first_window)
    {
        temp = ED4_ROOT->first_window;                  //delete temp afterwards
        ED4_ROOT->first_window = ED4_ROOT->first_window->next;

        if (no_of_windows > 1) {
            ED4_ROOT->use_first_window();
        }
    }
    else
    {
        temp = temp2 = ED4_ROOT->first_window;

        while (temp != window)
        {
            temp2 = temp;
            temp = temp->next;
        }

        temp2->next = temp->next;

        ED4_ROOT->use_window(temp2);
    }

    ED4_ROOT->aw_root->awar(temp->awar_path_for_cursor)->write_int(0);              // save in database
    ED4_ROOT->aw_root->awar(temp->awar_path_for_Ecoli)->write_int(0);
    ED4_ROOT->aw_root->awar(temp->awar_path_for_basePos)->write_int(0);
    ED4_ROOT->aw_root->awar(temp->awar_path_for_IUPAC)->write_string(IUPAC_EMPTY);
    ED4_ROOT->aw_root->awar(temp->awar_path_for_helixNr)->write_string("");
    delete temp;
}


ED4_window *ED4_window::insert_window( AW_window *new_aww )
{
    ED4_window *last,*temp;

    temp = ED4_ROOT->first_window;          //append at end of window list
    last = temp;
    while (temp) {
        last = temp;
        temp = temp->next;
    }

    temp = new ED4_window ( new_aww );

    if (!ED4_ROOT->first_window) {       //this is the first window
        ED4_ROOT->first_window = temp;
    }
    else if ( last != NULL) {
        last->next = temp;
    }

    // treat devices
    new_aww->set_expose_callback    ( AW_MIDDLE_AREA, ED4_expose_cb , 0, 0 );
    new_aww->set_resize_callback    ( AW_MIDDLE_AREA, ED4_resize_cb , 0, 0 );
    new_aww->set_input_callback     ( AW_MIDDLE_AREA, ED4_input_cb  , 0, 0 );
    new_aww->set_motion_callback    ( AW_MIDDLE_AREA, ED4_motion_cb , 0, 0 );
    new_aww->set_horizontal_change_callback( ED4_horizontal_change_cb   , 0, 0 );
    new_aww->set_vertical_change_callback  ( ED4_vertical_change_cb , 0, 0 );

    ED4_ROOT->use_window(temp);
    ED4_ROOT->temp_gc = ED4_G_STANDARD;

    return temp;
}

ED4_window::ED4_window( AW_window *window )
{
    aww = window;
    next = 0;
    slider_pos_horizontal = 0;
    slider_pos_vertical = 0;
    horizontal_fl = 0;
    vertical_fl = 0;
    scrolled_rect.clear();
    id = ++no_of_windows;
    coords.clear();
    is_hidden = 0;

    sprintf(awar_path_for_cursor, AWAR_EDIT_SEQ_POSITION, id);
    ED4_ROOT->aw_root->awar_int(awar_path_for_cursor,0,AW_ROOT_DEFAULT)->set_minmax(0,MAX_POSSIBLE_SEQ_LENGTH);

    sprintf(awar_path_for_Ecoli, AWAR_EDIT_ECOLI_POSITION, id);
    ED4_ROOT->aw_root->awar_int(awar_path_for_Ecoli,0,AW_ROOT_DEFAULT)->set_minmax(0,MAX_POSSIBLE_SEQ_LENGTH);

    sprintf(awar_path_for_basePos, AWAR_EDIT_BASE_POSITION, id);
    ED4_ROOT->aw_root->awar_int(awar_path_for_basePos,0,AW_ROOT_DEFAULT)->set_minmax(0,MAX_POSSIBLE_SEQ_LENGTH);

    sprintf(awar_path_for_IUPAC, AWAR_EDIT_IUPAC, id);
    ED4_ROOT->aw_root->awar_string(awar_path_for_IUPAC,IUPAC_EMPTY,AW_ROOT_DEFAULT);

    sprintf(awar_path_for_helixNr, AWAR_EDIT_HELIXNR, id);
    ED4_ROOT->aw_root->awar_string(awar_path_for_helixNr, "", AW_ROOT_DEFAULT); // ->set_minmax(-MAX_POSSIBLE_SEQ_LENGTH,MAX_POSSIBLE_SEQ_LENGTH);
}


ED4_window::~ED4_window()
{
    delete aww;
    delete horizontal_fl;       // be careful, don't delete links to hierarchy  in folding lines!!!
    delete vertical_fl;

/*  if (scrolled_rect.scroll_top)
        delete scrolled_rect.scroll_top;
    if (scrolled_rect.scroll_bottom)
        delete scrolled_rect.scroll_bottom;
    if (scrolled_rect.scroll_left)
        delete scrolled_rect.scroll_left;
    if (scrolled_rect.scroll_right)
        delete scrolled_rect.scroll_right;
*/
    no_of_windows --;
}

//***********************************
//* ED4_window Methods      end *
//***********************************



