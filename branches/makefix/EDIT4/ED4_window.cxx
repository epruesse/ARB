// =============================================================== //
//                                                                 //
//   File      : ED4_window.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdb.h>
#include <ed4_extern.hxx>
#include "ed4_class.hxx"
#include "ed4_tools.hxx"
#include <aw_awar.hxx>
#include <aw_root.hxx>

int ED4_window::no_of_windows = 0;                  // static variable has to be initialized only once

void ED4_window::reset_all_for_new_config() {
    scrolled_rect.reset(*this);
    e4_assert(ED4_foldable::is_reset());

    slider_pos_horizontal = 0;
    slider_pos_vertical   = 0;

    coords.top_area_x      = 0;
    coords.top_area_y      = 0;
    coords.top_area_height = 0;

    coords.middle_area_x = 0;
    coords.middle_area_y = 0;

    coords.window_width  = 0;
    coords.window_height = 0;

    coords.window_upper_clip_point = 0;
    coords.window_lower_clip_point = 0;
    coords.window_left_clip_point  = 0;
    coords.window_right_clip_point = 0;

    ED4_ROOT->aw_root->awar(awar_path_for_cursor)->write_int(0);
    ED4_ROOT->aw_root->awar(awar_path_for_Ecoli)->write_int(0);
    ED4_ROOT->aw_root->awar(awar_path_for_basePos)->write_int(0);
    ED4_ROOT->aw_root->awar(awar_path_for_IUPAC)->write_string(ED4_IUPAC_EMPTY);
    ED4_ROOT->aw_root->awar(awar_path_for_helixNr)->write_string("");

    if (next) next->reset_all_for_new_config();
}


void ED4_window::update_window_coords()
{
    AW_pos       x, y;
    AW_screen_area area_size;

    ED4_ROOT->top_area_man->calc_world_coords(&x, &y);

    coords.top_area_x      = (long) x;
    coords.top_area_y      = (long) y;
    coords.top_area_height = (long) ED4_ROOT->top_area_man->extension.size[HEIGHT];

    ED4_ROOT->middle_area_man->calc_world_coords(&x, &y);

    coords.middle_area_x = (long) x;
    coords.middle_area_y = (long) y;

    aww->_get_area_size (AW_MIDDLE_AREA, &area_size);

    coords.window_width  = area_size.r;
    coords.window_height = area_size.b;

    // world coordinates
    coords.window_upper_clip_point = coords.middle_area_y + aww->slider_pos_vertical; // coordinate of upper clipping point of middle area
    coords.window_lower_clip_point = coords.window_upper_clip_point + coords.window_height - coords.middle_area_y;

    if (ED4_ROOT->scroll_links.link_for_hor_slider) {
        ED4_ROOT->scroll_links.link_for_hor_slider->calc_world_coords(&x, &y);
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



ED4_folding_line* ED4_foldable::insert_folding_line(AW_pos world_pos, AW_pos dimension, ED4_properties prop) {
    ED4_folding_line *fl = NULL;

    if (prop == ED4_P_VERTICAL || prop == ED4_P_HORIZONTAL) {
        fl = new ED4_folding_line(world_pos, dimension);
        fl->insertAs(prop == ED4_P_VERTICAL ? vertical_fl : horizontal_fl);
    }
    return fl;
}

ED4_window *ED4_window::get_matching_ed4w(AW_window *aw) {
    e4_assert(aw);
    
    ED4_window *window = ED4_ROOT->first_window;
    while (window && window->aww != aw) {
        window = window->next;
    }
    
    e4_assert(window); // aw is not an edit-window
    return window;
}



void ED4_foldable::delete_folding_line(ED4_folding_line *fl, ED4_properties prop) {
    if (prop == ED4_P_HORIZONTAL) {
        horizontal_fl = horizontal_fl->delete_member(fl);
    }
    else {
        e4_assert(prop == ED4_P_VERTICAL);
        vertical_fl = vertical_fl->delete_member(fl);
    }
}

AW::Rectangle ED4_scrolled_rectangle::get_world_rect() const {
    e4_assert(is_linked());
    
    AW_pos x, y, dummy;
    x_link->calc_world_coords(&x, &dummy);
    y_link->calc_world_coords(&dummy, &y);

    AW::Vector size(width_link->extension.size[WIDTH],
                    height_link->extension.size[HEIGHT]);

    return AW::Rectangle(AW::Position(x, y), size);
}

void ED4_window::update_scrolled_rectangle() {
    if (scrolled_rect.exists()) {
        e4_assert(scrolled_rect.is_linked());

        AW::Rectangle srect = scrolled_rect.get_world_rect();
        scrolled_rect.set_rect_and_update_folding_line_positions(srect);

        // if slider positions stored in AW_window and ED4_window differ,
        // we correct folding line dimensions:
        
        {
            // update dimension and window position of folding lines at the borders of scrolled rectangle
            int dx = aww->slider_pos_horizontal - slider_pos_horizontal;
            int dy = aww->slider_pos_vertical - slider_pos_vertical;

            scrolled_rect.add_to_top_left_dimension(dx, dy);
        }

        AW_world rect = { 0, srect.height(), 0, srect.width() };
        aww->tell_scrolled_picture_size(rect);

        const AW_screen_area& area_size = get_device()->get_area_size();
        scrolled_rect.calc_bottomRight_folding_dimensions(area_size.r, area_size.b);

        update_window_coords(); // @@@ do at end of this function? (since it uses aww-slider_pos_horizontal,
                                // which might get modified by calculate_scrollbars below);

        // update window scrollbars
        set_scrollbar_indents();
        aww->calculate_scrollbars();

        check_valid_scrollbar_values(); // test that AW_window slider positions and folding line dimensions are in sync
    }

    // store synced slider positions in ED4_window
    slider_pos_vertical   = aww->slider_pos_vertical;
    slider_pos_horizontal = aww->slider_pos_horizontal;
}

ED4_returncode ED4_window::set_scrolled_rectangle(ED4_base *x_link, ED4_base *y_link, ED4_base *width_link, ED4_base *height_link) {
    e4_assert(x_link);
    e4_assert(y_link);
    e4_assert(width_link);
    e4_assert(height_link);

    scrolled_rect.destroy_folding_lines(*this); // first remove existing scrolled rectangle

    x_link->update_info.linked_to_scrolled_rectangle      = 1;
    y_link->update_info.linked_to_scrolled_rectangle      = 1;
    width_link->update_info.linked_to_scrolled_rectangle  = 1;
    height_link->update_info.linked_to_scrolled_rectangle = 1;

    scrolled_rect.link(x_link, y_link, width_link, height_link);

    const AW_screen_area& area_size = get_device()->get_area_size();

    AW::Rectangle rect = scrolled_rect.get_world_rect();
    scrolled_rect.create_folding_lines(*this, rect, area_size.r, area_size.b);

    scrolled_rect.set_rect(rect);

    return ED4_R_OK;
}

static inline void clear_and_update_rectangle(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2)
// clears and updates any range of the screen (in win coordinates)
// clipping range should be set correctly
{
    AW_screen_area rect;

    rect.t = int(y1);
    rect.b = int(y2);
    rect.l = int(x1);
    rect.r = int(x2);
    ED4_set_clipping_rectangle(&rect);

#if defined(DEBUG) && 0 
    static int toggle = 0;
    current_device()->box(ED4_G_COLOR_2+toggle, true, x1, y1, x2-x1, y2-y1, AW_ALL_DEVICES_SCALED);    // fill range with color (for testing)
    toggle = (toggle+1)&7;
#else
    current_device()->clear_part(x1, y1, x2-x1, y2-y1, AW_ALL_DEVICES);
#endif

    ED4_ROOT->main_manager->Show(1, 1); // direct call to Show (critical)
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
        AW_screen_area rect;
        rect.t = int(ty);
        rect.b = int(ty+ys-1);
        rect.l = int(tx);
        rect.r = int(tx+xs-1);
        ED4_set_clipping_rectangle(&rect);
    }

    AW_device *device = current_device();
    device->move_region(fx, fy, xs, ys, tx, ty);

    if (dy<0) { // scroll to the top
        device->set_top_font_overlap(true);
        clear_and_update_rectangle(x1, y2+dy, x2, y2);
        device->set_top_font_overlap(false);
    }
    else if (dy>0) { // scroll to the bottom
        device->set_bottom_font_overlap(true);
        clear_and_update_rectangle(x1, y1, x2, y1+dy);
        device->set_bottom_font_overlap(false);
    }

    int char_width = ED4_ROOT->font_group.get_max_width() * 2;
    if (dx<0) { // scroll left
        device->set_left_font_overlap(true);
        clear_and_update_rectangle(x2+dx-char_width, y1, x2, y2);
        device->set_left_font_overlap(false);
    }
    else if (dx>0) { // scroll right
        device->set_right_font_overlap(true);
        clear_and_update_rectangle(x1, y1, x1+dx+char_width, y2);
        device->set_right_font_overlap(false);
    }
}

static inline void update_rectangle(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2) // x1/y1=upper-left-corner x2/y2=lower-right-corner
{
    AW_screen_area rect;
    rect.t = int(y1);
    rect.b = int(y2);
    rect.l = int(x1);
    rect.r = int(x2);

    ED4_set_clipping_rectangle(&rect);
    clear_and_update_rectangle(x1, y1, x2, y2);
}


ED4_returncode ED4_window::scroll_rectangle(int dx, int dy)
{
    int skip_move;

    if (!dx && !dy) return ED4_R_OK; // scroll not

    AW::Rectangle rect = scrolled_rect.get_window_rect();

    AW::Position ul = rect.upper_left_corner();
    AW::Position lr = rect.lower_right_corner();

    AW_pos left_x   = ul.xpos();
    AW_pos top_y    = ul.ypos();
    AW_pos right_x  = lr.xpos();
    AW_pos bottom_y = lr.ypos();

    scrolled_rect.scroll(dx, dy);

    skip_move = (abs(int(dy)) > (bottom_y - top_y - 20)) || (abs(int(dx)) > (right_x - left_x - 20));

    AW_pos leftmost_x = coords.middle_area_x;
    AW_pos toptop_y = coords.top_area_y;
    AW_pos topbottom_y = toptop_y + coords.top_area_height - 1;

    get_device()->push_clip_scale();

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

    get_device()->pop_clip_scale();

    return (ED4_R_OK);
}

void ED4_window::set_scrollbar_indents() {
    if (scrolled_rect.exists()) {
        AW::Rectangle rect = scrolled_rect.get_window_rect();
        aww->set_vertical_scrollbar_top_indent(rect.top() + SLIDER_OFFSET);
        aww->set_horizontal_scrollbar_left_indent(rect.left() + SLIDER_OFFSET);
    }
}


void ED4_window::delete_window(ED4_window *window) {
    // delete from window list
    ED4_window *temp, *temp2;

    if (window == ED4_ROOT->first_window) {
        temp = ED4_ROOT->first_window;                  // delete temp afterwards
        ED4_ROOT->first_window = ED4_ROOT->first_window->next;
    }
    else {
        temp = temp2 = ED4_ROOT->first_window;

        while (temp != window) {
            temp2 = temp;
            temp = temp->next;
        }

        temp2->next = temp->next;
    }

    ED4_ROOT->aw_root->awar(temp->awar_path_for_cursor)->write_int(0);              // save in database
    ED4_ROOT->aw_root->awar(temp->awar_path_for_Ecoli)->write_int(0);
    ED4_ROOT->aw_root->awar(temp->awar_path_for_basePos)->write_int(0);
    ED4_ROOT->aw_root->awar(temp->awar_path_for_IUPAC)->write_string(ED4_IUPAC_EMPTY);
    ED4_ROOT->aw_root->awar(temp->awar_path_for_helixNr)->write_string("");
    delete temp;
}

static void ED4_expose_cb(AW_window *aww) {
    ED4_LocalWinContext uses(aww);
    GB_transaction      ta(GLOBAL_gb_main);

    ED4_expose_recalculations();
    current_ed4w()->update_scrolled_rectangle();

    current_device()->reset();
    ED4_ROOT->special_window_refresh(true);
}

static void ED4_resize_cb(AW_window *aww) {
    ED4_LocalWinContext uses(aww);
    GB_transaction      ta(GLOBAL_gb_main);

    current_device()->reset();
    current_ed4w()->update_scrolled_rectangle();
}


ED4_window *ED4_window::insert_window(AW_window_menu_modes *new_aww) {
    ED4_window *last, *temp;

    temp = ED4_ROOT->first_window;          // append at end of window list
    last = temp;
    while (temp) {
        last = temp;
        temp = temp->next;
    }

    temp = new ED4_window (new_aww);

    if (!ED4_ROOT->first_window) {       // this is the first window
        ED4_ROOT->first_window = temp;
    }
    else if (last != NULL) {
        last->next = temp;
    }

    // treat devices
    new_aww->set_expose_callback(AW_MIDDLE_AREA, makeWindowCallback(ED4_expose_cb));
    new_aww->set_resize_callback(AW_MIDDLE_AREA, makeWindowCallback(ED4_resize_cb));
    new_aww->set_input_callback (AW_MIDDLE_AREA, makeWindowCallback(ED4_input_cb));
    new_aww->set_motion_callback(AW_MIDDLE_AREA, makeWindowCallback(ED4_motion_cb));

    new_aww->set_horizontal_change_callback(makeWindowCallback(ED4_horizontal_change_cb));
    new_aww->set_vertical_change_callback  (makeWindowCallback(ED4_vertical_change_cb));

    ED4_ROOT->temp_gc = ED4_G_STANDARD;

    return temp;
}

ED4_window::ED4_window(AW_window_menu_modes *window)
    : aww(window),
      next(NULL),
      slider_pos_horizontal(0),
      slider_pos_vertical(0),
      id(++no_of_windows),
      is_hidden(false),
      cursor(this)
{
    coords.clear();

    sprintf(awar_path_for_cursor, AWAR_EDIT_SEQ_POSITION, id);
    ED4_ROOT->aw_root->awar_int(awar_path_for_cursor, 0, AW_ROOT_DEFAULT);

    sprintf(awar_path_for_Ecoli, AWAR_EDIT_ECOLI_POSITION, id);
    ED4_ROOT->aw_root->awar_int(awar_path_for_Ecoli, 0, AW_ROOT_DEFAULT);

    sprintf(awar_path_for_basePos, AWAR_EDIT_BASE_POSITION, id);
    ED4_ROOT->aw_root->awar_int(awar_path_for_basePos, 0, AW_ROOT_DEFAULT);

    sprintf(awar_path_for_IUPAC, AWAR_EDIT_IUPAC, id);
    ED4_ROOT->aw_root->awar_string(awar_path_for_IUPAC, ED4_IUPAC_EMPTY, AW_ROOT_DEFAULT);

    sprintf(awar_path_for_helixNr, AWAR_EDIT_HELIXNR, id);
    ED4_ROOT->aw_root->awar_string(awar_path_for_helixNr, "", AW_ROOT_DEFAULT);
}


ED4_window::~ED4_window() {
    delete aww;
    no_of_windows --;
}

