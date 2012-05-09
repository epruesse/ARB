// =============================================================== //
//                                                                 //
//   File      : DI_view_matrix.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_view_matrix.hxx"
#include "di_matr.hxx"
#include "dist.hxx"

#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <awt_canvas.hxx>

#include <algorithm>

#define AWAR_MATRIX                "matrix/"
#define AWAR_MATRIX_PADDING        AWAR_MATRIX "padding"
#define AWAR_MATRIX_SHOWZERO       AWAR_MATRIX "show_zero"
#define AWAR_MATRIX_DIGITS         AWAR_MATRIX "show_digits"
#define AWAR_MATRIX_NAMECHARS_TOP  AWAR_MATRIX "namechars_top"
#define AWAR_MATRIX_NAMECHARS_LEFT AWAR_MATRIX "namechars_left"

static void vertical_change_cb  (AW_window *aww, DI_dmatrix *dis) { dis->monitor_vertical_scroll_cb(aww); }
static void horizontal_change_cb(AW_window *aww, DI_dmatrix *dis) { dis->monitor_horizontal_scroll_cb(aww); }

static void redisplay_needed(AW_window *, DI_dmatrix *dis) {
    dis->display(true);
}

static void reinit_needed(AW_root *, AW_CL cl_dis) {
    DI_dmatrix *dis = (DI_dmatrix*)cl_dis;
    dis->init();
    dis->display(false);
}

static void resize_needed(AW_window *, DI_dmatrix *dis) {
    dis->init();
    dis->resized();
    dis->display(false);
}

DI_dmatrix::DI_dmatrix() {
    memset((char *) this, 0, sizeof(DI_dmatrix));
}

void DI_dmatrix::init (DI_MATRIX *matrix) {
    di_matrix = matrix;
    DI_MATRIX *m = get_matrix();

    AW_root *awr = awm->get_root();


    leadZero = awr->awar(AWAR_MATRIX_SHOWZERO)->read_int();
    digits   = awr->awar(AWAR_MATRIX_DIGITS)->read_int();

    sprintf(format_string, "%%%i.%if", digits+2, digits);

    // calculate cell width and height
    {
        cell_width  = 0;
        cell_height = 0;

        int max_chars[DI_G_LAST+1];
        memset(max_chars, 0, sizeof(*max_chars)*(DI_G_LAST+1));
        
        max_chars[DI_G_STANDARD]   = leadZero+2; // standard cell contains "0.0", "1.0" or "---"
        max_chars[DI_G_BELOW_DIST] = leadZero+1+digits; // size of numeric distance
        max_chars[DI_G_ABOVE_DIST] = max_chars[DI_G_BELOW_DIST];
        max_chars[DI_G_NAMES]      = awr->awar(AWAR_MATRIX_NAMECHARS_TOP)->read_int();

        for (DI_gc gc=DI_G_STANDARD; gc<=DI_G_LAST; gc = DI_gc(int(gc)+1)) {
            if (max_chars[gc]) {
                const AW_font_limits& lim = device->get_font_limits(gc, 0);

                cell_width  = std::max(cell_width, lim.width*max_chars[gc]);
                cell_height = std::max(cell_height, int(lim.height));
            }
        }
        // ensure cell-dimensions are > 0
        AW_awar *pad_awar = awr->awar(AWAR_MATRIX_PADDING);
        cell_padd         = pad_awar->read_int();

        if (cell_padd<0) {
            bool update = false;
            if (-cell_padd >= cell_width) {
                cell_padd = -cell_width+1;
                update    = true;
            }
            if (-cell_padd >= cell_height) {
                cell_padd = -cell_height+1;
                update    = true;
            }
            if (update) pad_awar->write_int(cell_padd);
        }

        cell_width  += cell_padd;
        cell_height += cell_padd;
    }

    {
        const AW_font_limits& lim = device->get_font_limits(DI_G_NAMES, 0);
 
        off_dx = awr->awar(AWAR_MATRIX_NAMECHARS_LEFT)->read_int() * lim.width + 1 + cell_padd;
        off_dy = lim.height + cell_height; // off_dy corresponds to "lower" y of cell
    }

    if (m) {
        total_cells_horiz=m->nentries;
        total_cells_vert=m->nentries;
    }
    set_scrollbar_steps(cell_width, cell_height, 50, 50);
    resized();  // initialize window_size dependent parameters
}

DI_MATRIX *DI_dmatrix::get_matrix() {
    if (di_matrix) return di_matrix;
    return GLOBAL_MATRIX.get();
}

void DI_dmatrix::resized() {
    const AW_font_limits& lim = device->get_font_limits(DI_G_STANDARD, 0);

    DI_MATRIX *m = get_matrix();
    long       n = 0;

    if (m) n = m->nentries;

    const AW_screen_area& squ = device->get_area_size();
    screen_width  = squ.r-squ.l;
    screen_height = squ.b-squ.t;

    long horiz_paint_size, vert_paint_size;
    AW_screen_area rect; // @@@ used uninitialized if !m
    if (m) {
        horiz_paint_size = (squ.r-lim.width-off_dx)/cell_width;
        vert_paint_size  = (squ.b-off_dy)/cell_height;
        horiz_page_size  = (n > horiz_paint_size) ?  horiz_paint_size : n;
        vert_page_size   = (n > vert_paint_size) ? vert_paint_size : n;

        rect.l = 0;
        rect.t = 0;
        rect.r = (int)((n-horiz_page_size)*cell_width+squ.r);
        rect.b = (int)((n-vert_page_size)*cell_height+squ.b);
    }

    horiz_page_start      = 0;
    horiz_last_view_start = 0;
    vert_page_start       = 0;
    vert_last_view_start  = 0;

    device->reset();            // clip_size == device_size
    device->clear(-1);
    device->set_right_clip_border((int)(off_dx+cell_width*horiz_page_size));
    device->reset();            // reset shift_x and shift_y
    awm->set_vertical_scrollbar_position(0);
    awm->set_horizontal_scrollbar_position(0);
    awm->tell_scrolled_picture_size(rect);
    awm->calculate_scrollbars();
    if (!awm->is_shown() && m) {
        awm->show();
    }
}

enum ClickAction {
    CLICK_SELECT_SPECIES = 1,
    CLICK_SET_MINMAX,
};

#define MINMAX_GRANULARITY 10000L
#define ROUNDUP            0.00005 // in order to round to 4 digits

void DI_dmatrix::handle_move(AW_event& event) {
    static int clickx, clicky; // original click pos
    static int startx, starty; // original slider position

    if (event.type == AW_Mouse_Press) {
        clickx = event.x;
        clicky = event.y;

        startx = awm->slider_pos_horizontal;
        starty = awm->slider_pos_vertical;
    }
    else if (event.type == AW_Mouse_Drag || event.type == AW_Mouse_Release) {
        int x_screen_diff = clickx - event.x;
        int y_screen_diff = clicky - event.y;

        int sxpos = startx + x_screen_diff;
        int sypos = starty + y_screen_diff;

        AW_pos maxx = awm->get_scrolled_picture_width() - screen_width;
        AW_pos maxy = awm->get_scrolled_picture_height() - screen_height;

        if (sxpos>maxx) sxpos = int(maxx);
        if (sypos>maxy) sypos = int(maxy);
        if (sxpos<0) sxpos    = 0;
        if (sypos<0) sypos    = 0;

        awm->set_horizontal_scrollbar_position(sxpos);
        awm->set_vertical_scrollbar_position(sypos);

        monitor_vertical_scroll_cb(awm);
        monitor_horizontal_scroll_cb(awm);
    }
}

static void motion_cb(AW_window *aww, AW_CL cl_dmatrix, AW_CL) {
    AW_event event;
    aww->get_event(&event);

    DI_dmatrix *dmatrix = reinterpret_cast<DI_dmatrix*>(cl_dmatrix);
    if (event.button == AW_BUTTON_MIDDLE) {
        dmatrix->handle_move(event);
    }
}

static void input_cb(AW_window *aww, AW_CL cl_dmatrix, AW_CL) {
    AW_event event;
    aww->get_event(&event);

    DI_dmatrix *dmatrix = reinterpret_cast<DI_dmatrix*>(cl_dmatrix);
    if (event.button == AW_BUTTON_MIDDLE) {
        dmatrix->handle_move(event);
    }
    else {
        AW_device_click *click_device = aww->get_click_device(AW_MIDDLE_AREA, event.x, event.y, 20, 20, 0);

        click_device->set_filter(AW_CLICK);
        click_device->reset();

        {
            AW_device *oldMatrixDevice = dmatrix->device;

            dmatrix->device = click_device;
            dmatrix->display(false); // detect clicked element
            dmatrix->device = oldMatrixDevice;
        }

        if (event.type == AW_Mouse_Press) {
            AW_clicked_text clicked_text;
            AW_clicked_line clicked_line;
            click_device->get_clicked_text(&clicked_text);
            click_device->get_clicked_line(&clicked_line);

            AW_CL cd1, cd2;
            if (AW_getBestClick(&clicked_line, &clicked_text, &cd1, &cd2)) {
                ClickAction action = static_cast<ClickAction>(cd1);

                if (action == CLICK_SELECT_SPECIES) {
                    long       idx    = long(cd2);
                    DI_MATRIX *matrix = dmatrix->get_matrix();
                    if (idx >= matrix->nentries) {
                        aw_message(GBS_global_string("Illegal idx %li [allowed: 0-%li]", idx, matrix->nentries));
                    }
                    else {
                        DI_ENTRY   *entry        = matrix->entries[idx];
                        const char *species_name = entry->name;

                        aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(species_name);
                    }
                }
                else if (action == CLICK_SET_MINMAX) {
                    AW_root *aw_root    = aww->get_root();
                    AW_awar *awar_bound = 0;

                    switch (event.button) {
                        case AW_BUTTON_LEFT:  awar_bound = aw_root->awar(AWAR_DIST_MIN_DIST); break;
                        case AW_BUTTON_RIGHT: awar_bound = aw_root->awar(AWAR_DIST_MAX_DIST); break;
                        default: break;
                    }

                    if (awar_bound) {
                        double val = double(cd2)/MINMAX_GRANULARITY;
                        awar_bound->write_float(val);
                    }
                }
            }
        }
    }
}

void DI_dmatrix::display(bool clear) {
    // draw matrix

    long           x, y, xpos, ypos;
    GB_transaction dummy(GLOBAL_gb_main);

    if (!device) return;

    DI_MATRIX *m = get_matrix();
    if (!m) {
        if (awm) awm->hide();
        return;
    }

    if (clear) device->clear(-1);
    device->set_offset(AW::Vector(off_dx, off_dy));
    xpos = 0;

    char *selSpecies = 0;
    if (awm) selSpecies = awm->get_root()->awar(AWAR_SPECIES_NAME)->read_string();

    int name_display_width_top;
    int name_display_width_left;
    {
        const AW_font_limits& lim = device->get_font_limits(DI_G_NAMES, 0);

        name_display_width_top  = (cell_width-1)/lim.width;
        name_display_width_left = (off_dx-1)/lim.width;
    }

    int  BUFLEN = std::max(200, std::max(name_display_width_left, name_display_width_top));
    char buf[BUFLEN];

    int sel_x_pos = -1;

    // device->set_line_attributes(DI_G_RULER, 1, AW_SOLID); // @@@ try AW_DOTTED here (need merges from dev!)
    
    for (x = horiz_page_start;
         x < (horiz_page_start + horiz_page_size) && (x < total_cells_horiz);
         x++)
    {
        ypos = 0;
        for (y = vert_page_start;
             y < (vert_page_start + vert_page_size) && (y < total_cells_vert);
             y++)
        {
            bool is_identity = (x == y);

            // lower(!) left corner of cell:
            AW_pos cellx = xpos*cell_width;
            AW_pos celly = ypos*cell_height;

            if (is_identity) {
                device->text(DI_G_STANDARD, "---"+(1-leadZero), cellx, celly);
            }
            else {
                double val2 = m->matrix->get(x, y);
                AW_click_cd cd(device, CLICK_SET_MINMAX, val2*MINMAX_GRANULARITY+1);

                if (val2>=min_view_dist && val2<=max_view_dist) { // display ruler
                    int maxw = cell_width-cell_padd;

                    int h = cell_height - cell_padd-1;

                    int hbox, hruler;
                    if (cell_padd >= 0) {
                        hbox   = h*2/3;
                        hruler = h-hbox;
                    }
                    else {
                        hbox   = h;
                        hruler = 0;
                    }

                    int y1 = celly - h;
                    int y2 = y1+hbox;
                    int y3 = y2+hruler/2;
                    int y4 = y2+hruler;
                    int x2 = cellx;

                    double len = ((val2-min_view_dist)/(max_view_dist-min_view_dist)) * maxw;
                    if (len >= 0) {
                        device->box(DI_G_RULER_DISPLAY, true, x2, y1, int(len+0.5), hbox);
                    }
                    else {
                        device->text(DI_G_STANDARD, "???", cellx, celly);
                    }

                    if (hruler) { // do not paint ruler if cell_padd is negative
                        double v;
                        int    cnt;
                        int    maxx = x2+maxw-1;
                        for (v = x2, cnt = 0; v < x2 + maxw; v += maxw * .24999, ++cnt) {
                            int xr = std::min(int(v+0.5), maxx);
                            device->line(DI_G_RULER, xr, (cnt&1) ? y3 : y2, xr, y4);
                        }
                        device->line(DI_G_RULER, x2, y4, maxx, y4);
                    }
                }
                else {
                    DI_gc gc;
                    if (val2 == 0.0) {
                        strcpy(buf, "0.0");
                        gc = DI_G_STANDARD;
                    }
                    else if (val2 == 1.0) {
                        strcpy(buf, leadZero ? "1.0" : " 1");
                        gc = DI_G_STANDARD;
                    }
                    else {
                        sprintf(buf, format_string, val2);
                        gc = val2<min_view_dist ? DI_G_BELOW_DIST : DI_G_ABOVE_DIST;
                    }
                    device->text(gc, leadZero ? buf : buf+1, cellx, celly);
                }
            }
            ypos++;
        }
        
        // display horizontal (top) speciesnames:
        strcpy(buf, m->entries[x]->name);
        if (selSpecies && strcmp(buf, selSpecies) == 0) sel_x_pos = xpos; // remember x-position for selected species
        buf[name_display_width_top] = 0; // cut group-names if too long

        AW_click_cd cd(device, CLICK_SELECT_SPECIES, x);
        device->text(DI_G_NAMES, buf, xpos * cell_width, cell_height - off_dy);

        xpos++;
    }

    device->set_offset(AW::Vector(off_dx, 0));

    AW::Rectangle area(device->get_area_size(), AW::INCLUSIVE_OUTLINE);

    // highlight selected species (vertically)
    if (sel_x_pos != -1) {
        AW_pos linex1 = sel_x_pos*cell_width - cell_padd/2-1;
        AW_pos linex2 = linex1+cell_width;
        AW_pos height = area.height();
        device->line(DI_G_STANDARD, linex1, 0, linex1, height);
        device->line(DI_G_STANDARD, linex2, 0, linex2, height);
    }

    device->set_offset(AW::Vector(0, off_dy));

    // display vertical (left) speciesnames
    ypos          = 0;
    int sel_y_pos = -1;
    for (y = vert_page_start; y < vert_page_start + vert_page_size; y++) {
        strcpy(buf, m->entries[y]->name);
        if (selSpecies && strcmp(buf, selSpecies) == 0) sel_y_pos = ypos; // remember x-position for selected species
        buf[name_display_width_left] = 0; // cut group-names if too long
        AW_click_cd cd(device, CLICK_SELECT_SPECIES, y);
        device->text(DI_G_NAMES, buf, 0, ypos * cell_height);
        ypos++;
    }

    // highlight selected species (horizontally)
    if (sel_y_pos != -1) {
        AW_pos liney2 = sel_y_pos*cell_height + cell_padd/2+1;
        AW_pos liney1 = liney2-cell_height;
        AW_pos width  = area.width();
        device->line(DI_G_STANDARD, 0, liney1, width, liney1);
        device->line(DI_G_STANDARD, 0, liney2, width, liney2);
    }

    device->set_offset(AW::Vector(0, 0));
#undef BUFLEN
}

void DI_dmatrix::set_scrollbar_steps(long width_h, long width_v, long page_h, long page_v)
{
    char buffer[200];

    sprintf(buffer, "window/%s/scroll_width_horizontal", awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(width_h);
    sprintf(buffer, "window/%s/scroll_width_vertical", awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(width_v);
    sprintf(buffer, "window/%s/horizontal_page_increment", awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(page_h);
    sprintf(buffer, "window/%s/vertical_page_increment", awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(page_v);
}

#if defined(WARN_TODO)
#warning test scrolling again with fixed box_impl()
#endif

#if defined(DEBUG) && 0
#define DEBUG_GC DI_G_STANDARD
#endif

void DI_dmatrix::monitor_vertical_scroll_cb(AW_window *aww) { // draw area
    if (!device) return;

    int old_vert_page_start = vert_page_start;

    vert_last_view_start = aww->slider_pos_vertical;
    vert_page_start      = aww->slider_pos_vertical/cell_height;

    int diff = vert_page_start-old_vert_page_start; // amount of rows to scroll
    if (diff) {
        int  diff_pix   = abs(diff)*cell_height;
        int  top_y      = off_dy-cell_height;
        bool clear      = false;
        int  keep_cells = vert_page_size-abs(diff);
        int  keep_pix   = keep_cells*cell_height;

        if (diff>0 && diff<vert_page_size) { // scroll some positions up
            device->move_region(0, top_y+diff_pix, screen_width, keep_pix, 0, top_y);
            device->clear_part (0, top_y+keep_pix, screen_width, diff_pix, AW_SCREEN);
#if defined(DEBUG_GC)
            device->box (DEBUG_GC, true, 0, top_y+keep_pix, screen_width, diff_pix);
#endif
            device->push_clip_scale();
            device->set_top_clip_border(top_y+keep_pix, true);
        }
        else if (diff>-vert_page_size && diff<0) { // scroll some positions down
            device->move_region(0, top_y, screen_width, keep_pix, 0, top_y+diff_pix);
            device->clear_part (0, top_y, screen_width, diff_pix, AW_SCREEN);
#if defined(DEBUG_GC)
            device->box (DEBUG_GC, true, 0, top_y, screen_width, diff_pix);
#endif
            device->push_clip_scale();
            device->set_bottom_clip_border(top_y+diff_pix, true);
        }
        else { // repaint
            device->push_clip_scale();
            clear = true;
        }

        display(clear);
        device->pop_clip_scale();
    }
}

void DI_dmatrix::monitor_horizontal_scroll_cb(AW_window *aww) { // draw area
    if (!device) return;

    int old_horiz_page_start = horiz_page_start;

    horiz_last_view_start = aww->slider_pos_horizontal;
    horiz_page_start      = aww->slider_pos_horizontal/cell_width;

    int diff = horiz_page_start-old_horiz_page_start; // amount of columns to scroll

    if (diff) {
        bool clear      = false;
        int  diff_pix   = abs(diff)*cell_width;
        int  keep_cells = horiz_page_size-abs(diff);
        int  keep_pix   = keep_cells*cell_width;

        if (diff>0 && diff<horiz_page_size) {      // scroll some positions left
            device->move_region(off_dx+diff_pix, 0, keep_pix, screen_height, off_dx, 0);
            device->clear_part (off_dx+keep_pix, 0, diff_pix, screen_height, AW_SCREEN);
#if defined(DEBUG_GC)
            device->box(DEBUG_GC, true, off_dx+keep_pix, 0, diff_pix, screen_height);
#endif
            device->push_clip_scale();
            device->set_left_clip_border(keep_pix, true);
        }
        else if (diff>-horiz_page_size && diff<0) { // scroll some positions right
            device->move_region(off_dx, 0, keep_pix, screen_height, off_dx+diff_pix, 0);
            device->clear_part (off_dx, 0, diff_pix, screen_height, AW_SCREEN);
#if defined(DEBUG_GC)
            device->box(DEBUG_GC, true, off_dx, 0, diff_pix, screen_height);
#endif
            device->push_clip_scale();
            device->set_right_clip_border(off_dx+diff_pix, true);
        }
        else { // repaint
            device->push_clip_scale();
            clear = true;
        }

        display(clear);
        device->pop_clip_scale();
    }
}

static bool update_display_on_dist_change = true;

static void di_view_set_max_d(AW_window *aww, AW_CL cl_max_d, AW_CL /* clmatr */) {
    double   max_d   = cl_max_d*0.01;
    AW_root *aw_root = aww->get_root();

    {
        LocallyModify<bool> flag(update_display_on_dist_change, false);
        aw_root->awar(AWAR_DIST_MIN_DIST)->write_float(0.0);
    }
    aw_root->awar(AWAR_DIST_MAX_DIST)->write_float(max_d);
}

static void di_view_set_distances(AW_root *awr, AW_CL cl_setmax, AW_CL cl_dmatrix) {
    // cl_dmatrix: 0 -> set min and fix max, 1 -> set max and fix min, 2 -> set both
    DI_dmatrix *dmatrix  = (DI_dmatrix *)cl_dmatrix;
    double      max_dist = awr->awar(AWAR_DIST_MAX_DIST)->read_float();
    double      min_dist = awr->awar(AWAR_DIST_MIN_DIST)->read_float();

    {
        LocallyModify<bool> flag(update_display_on_dist_change, false);

        switch (cl_setmax) {
            case 2:                 // both
                dmatrix->set_slider_max(max_dist);
                // fall-through
            case 0:                 // set min and fix max
                dmatrix->set_slider_min(min_dist);
                if (min_dist>max_dist) awr->awar(AWAR_DIST_MAX_DIST)->write_float(min_dist);
                break;
            case 1:                 // set max and fix min
                dmatrix->set_slider_max(max_dist);
                if (min_dist>max_dist) awr->awar(AWAR_DIST_MIN_DIST)->write_float(max_dist);
                break;

            default:
                di_assert(0);
                break;
        }
    }
    if (update_display_on_dist_change) dmatrix->display(true);
}

static void di_change_dist(AW_window *aww, AW_CL cl_mode) {
    AW_root *awr = aww->get_root();
    const char *awar_name;

    di_assert(cl_mode>=0 && cl_mode<=3);

    if (cl_mode<2) { // change min
        awar_name = AWAR_DIST_MIN_DIST;
    }
    else { // change max
        awar_name = AWAR_DIST_MAX_DIST;
    }

    double dist = awr->awar(awar_name)->read_float();
    double increment = 0.01;

    if (cl_mode%2) increment = -increment; // decrement value
    dist += increment;
    if (!(dist<0)) awr->awar(awar_name)->write_float(dist);
}

static void di_bind_dist_awars(AW_root *aw_root, DI_dmatrix *dmatrix) {
    aw_root->awar_float(AWAR_DIST_MIN_DIST)->add_callback(di_view_set_distances, 0, (AW_CL)dmatrix);
    aw_root->awar_float(AWAR_DIST_MAX_DIST)->add_callback(di_view_set_distances, 1, (AW_CL)dmatrix);
}

static void create_matrix_awars(AW_root *awr, DI_dmatrix *dmatrix) {
    awr->awar_int(AWAR_MATRIX_PADDING, 4)->add_callback(reinit_needed, (AW_CL)dmatrix);
    awr->awar_int(AWAR_MATRIX_SHOWZERO, 1)->add_callback(reinit_needed, (AW_CL)dmatrix);
    awr->awar_int(AWAR_MATRIX_DIGITS, 4)->add_callback(reinit_needed, (AW_CL)dmatrix);
    awr->awar_int(AWAR_MATRIX_NAMECHARS_TOP, 8)->add_callback(reinit_needed, (AW_CL)dmatrix);
    awr->awar_int(AWAR_MATRIX_NAMECHARS_LEFT, 10)->add_callback(reinit_needed, (AW_CL)dmatrix);
}

static AW_window *create_matrix_settings_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "MATRIX_SETTINGS", "Matrix settings");

    aws->auto_space(10, 10);

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"matrix_settings.hlp");
    aws->create_button("HELP", "HELP");

    aws->label_length(21);

    aws->at_newline();
    aws->label("Padding (pixels)");
    aws->create_input_field(AWAR_MATRIX_PADDING, 3);

    aws->at_newline();
    aws->label("Show leading zero");
    aws->create_toggle(AWAR_MATRIX_SHOWZERO);

    aws->at_newline();
    aws->label("Precision (digits)");
    aws->create_input_field(AWAR_MATRIX_DIGITS, 2);

    aws->at_newline();
    aws->label("Min. namechars (top)");
    aws->create_input_field(AWAR_MATRIX_NAMECHARS_TOP, 3);

    aws->at_newline();
    aws->label("Min. namechars (left)");
    aws->create_input_field(AWAR_MATRIX_NAMECHARS_LEFT, 3);

    aws->window_fit();
    return aws;
}

static void selected_species_changed_cb(AW_root*, AW_CL cl_maxtrix_window, AW_CL cl_dmatrix) {
    DI_dmatrix *dmatrix = (DI_dmatrix*)cl_dmatrix;
    if (dmatrix) {
        AW_window *awm = reinterpret_cast<AW_window*>(cl_maxtrix_window);
        redisplay_needed(awm, dmatrix);
    }
}

AW_window *DI_create_view_matrix_window(AW_root *awr, DI_dmatrix *dmatrix, save_matrix_params *sparam) {
    di_bind_dist_awars(awr, dmatrix);
    create_matrix_awars(awr, dmatrix);
    
    AW_window_menu *awm = new AW_window_menu();
    awm->init(awr, "SHOW_MATRIX", "ARB_SHOW_MATRIX", 800, 600);

    dmatrix->device = awm->get_device(AW_MIDDLE_AREA);
    dmatrix->awm    = awm;

    AW_awar *awar_sel = awr->awar(AWAR_SPECIES_NAME);
    awar_sel->add_callback(selected_species_changed_cb, (AW_CL)awm, (AW_CL)dmatrix);
    
    awm->set_vertical_change_callback  ((AW_CB2)vertical_change_cb,   (AW_CL)dmatrix, 0);
    awm->set_horizontal_change_callback((AW_CB2)horizontal_change_cb, (AW_CL)dmatrix, 0);

    awm->set_resize_callback(AW_MIDDLE_AREA, (AW_CB2)resize_needed,    (AW_CL)dmatrix, 0);
    awm->set_expose_callback(AW_MIDDLE_AREA, (AW_CB2)redisplay_needed, (AW_CL)dmatrix, 0);
    awm->set_input_callback (AW_MIDDLE_AREA, (AW_CB) input_cb,         (AW_CL)dmatrix, 0);
    awm->set_motion_callback(AW_MIDDLE_AREA, (AW_CB) motion_cb,        (AW_CL)dmatrix, 0);

    AW_gc_manager preset_window =
        AW_manage_GC(awm, dmatrix->device, DI_G_STANDARD, DI_G_LAST, AW_GCM_DATA_AREA,
                     (AW_CB)resize_needed, (AW_CL)dmatrix, 0,
                     false,
                     "#D0D0D0",
                     "#Standard$#000000",
                     "#Names$#000000",
                     "+-Ruler$#555", "-Display$#00AA55",
                     "#BelowDist$#008732",
                     "#AboveDist$#DB008D",
                     NULL);

    awm->create_menu("File", "F");
    awm->insert_menu_topic("save_matrix", "Save Matrix to File", "S", "save_matrix.hlp", AWM_ALL, AW_POPUP,          (AW_CL)DI_create_save_matrix_window, (AW_CL)sparam);
    awm->insert_menu_topic("close",       "Close",               "C", 0,                 AWM_ALL, (AW_CB)AW_POPDOWN, (AW_CL)0, 0);

    awm->create_menu("Range", "R");
    awm->insert_menu_topic("deselect_range",    "Deselect range",           "D", 0, AWM_ALL, di_view_set_max_d, 0,   (AW_CL)dmatrix);
    awm->insert_menu_topic("show_dist_species", "Species range [ <=  2% ]", "S", 0, AWM_ALL, di_view_set_max_d, 2,   (AW_CL)dmatrix);
    awm->insert_menu_topic("show_dist_genus",   "Genus range   [ <=  5% ]", "G", 0, AWM_ALL, di_view_set_max_d, 5,   (AW_CL)dmatrix);
    awm->insert_menu_topic("show_dist_010",     "Range         [ <= 10% ]", "1", 0, AWM_ALL, di_view_set_max_d, 10,  (AW_CL)dmatrix);
    awm->insert_menu_topic("show_dist_025",     "Range         [ <= 25% ]", "2", 0, AWM_ALL, di_view_set_max_d, 25,  (AW_CL)dmatrix);
    awm->insert_menu_topic("show_dist_050",     "Range         [ <= 50% ]", "5", 0, AWM_ALL, di_view_set_max_d, 50,  (AW_CL)dmatrix);
    awm->insert_menu_topic("show_dist_100",     "Whole range   [ 0-100% ]", "0", 0, AWM_ALL, di_view_set_max_d, 100, (AW_CL)dmatrix);

    awm->create_menu("Properties", "P");
    awm->insert_menu_topic("matrix_settings", "Settings ...",         "S", "matrix_settings.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_matrix_settings_window, (AW_CL)0);
    awm->insert_menu_topic("matrix_colors",   "Colors and Fonts ...", "C", "neprops_data.hlp", AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)preset_window);
    
    int x, y;
    awm->get_at_position(&x, &y);
    awm->button_length(3);

#define FIELD_XSIZE 160
#define BUTTON_XSIZE 25

    awm->label("Dist min:");
    awm->create_input_field(AWAR_DIST_MIN_DIST, 7);
    x += FIELD_XSIZE;

    awm->at_x(x);
    awm->callback(di_change_dist, 0);
    awm->create_button("PLUS_MIN", "+");
    x += BUTTON_XSIZE;

    awm->at_x(x);
    awm->callback(di_change_dist, 1);
    awm->create_button("MINUS_MIN", "-");
    x += BUTTON_XSIZE;

    awm->at_x(x);
    awm->label("Dist max:");
    awm->create_input_field(AWAR_DIST_MAX_DIST, 7);
    x += FIELD_XSIZE;

    awm->at_x(x);
    awm->callback(di_change_dist, 2);
    awm->create_button("PLUS_MAX", "+");
    x += BUTTON_XSIZE;

    awm->at_x(x);
    awm->callback(di_change_dist, 3);
    awm->create_button("MINUS_MAX", "-");
    x += BUTTON_XSIZE;

    awm->set_info_area_height(40);

    dmatrix->init(dmatrix->di_matrix);

    di_view_set_distances(awm->get_root(), 2, AW_CL(dmatrix));

    return awm;
}
