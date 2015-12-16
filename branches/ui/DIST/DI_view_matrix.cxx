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
#include "dist.hxx"

#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <awt_canvas.hxx>

#include <arb_algo.h>
#include <awt_config_manager.hxx>

#define AWAR_MATRIX                "matrix/"
#define AWAR_MATRIX_PADDINGX       AWAR_MATRIX "paddingx"
#define AWAR_MATRIX_PADDINGY       AWAR_MATRIX "paddingy"
#define AWAR_MATRIX_SHOWZERO       AWAR_MATRIX "show_zero"
#define AWAR_MATRIX_DIGITS         AWAR_MATRIX "show_digits"
#define AWAR_MATRIX_NAMECHARS_TOP  AWAR_MATRIX "namechars_top"
#define AWAR_MATRIX_NAMECHARS_LEFT AWAR_MATRIX "namechars_left"

static void vertical_change_cb  (AW_window *aww, MatrixDisplay *disp) { disp->monitor_vertical_scroll_cb(aww); }
static void horizontal_change_cb(AW_window *aww, MatrixDisplay *disp) { disp->monitor_horizontal_scroll_cb(aww); }

static void redisplay_needed(UNFIXED, MatrixDisplay *disp) {
    disp->mark(MatrixDisplay::NEED_CLEAR);
    disp->update_display();
}

static void reinit_needed(UNFIXED, MatrixDisplay *disp) {
    disp->mark(MatrixDisplay::NEED_SETUP);
    disp->update_display();
}

static void resize_needed(UNFIXED, MatrixDisplay *disp) {
    disp->mark(MatrixDisplay::NEED_SETUP); // @@@ why not NEED_RESIZE?
    disp->update_display();
}

static void gc_changed_cb(GcChange whatChanged, MatrixDisplay *disp) {
    switch (whatChanged) {
        case GC_COLOR_GROUP_USE_CHANGED:
            di_assert(0); // not used atm -> fall-through
        case GC_COLOR_CHANGED:
            redisplay_needed(NULL, disp);
            break;
        case GC_FONT_CHANGED:
            resize_needed(NULL, disp);
            break;
    }
}

void MatrixDisplay::setup() {
    DI_MATRIX *m   = get_matrix();
    AW_root   *awr = awm->get_root();

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

        for (int igc=DI_G_STANDARD; igc<=DI_G_LAST; ++igc) {
            DI_gc gc = DI_gc(igc);
            if (max_chars[gc]) {
                const AW_font_limits& lim = device->get_font_limits(gc, 0);

                cell_width  = std::max(cell_width, lim.width*max_chars[gc]);
                cell_height = std::max(cell_height, int(lim.height));
            }
        }

        {
            // ensure cell-dimensions are > 0
            AW_awar *pad_awarx = awr->awar(AWAR_MATRIX_PADDINGX);
            AW_awar *pad_awary = awr->awar(AWAR_MATRIX_PADDINGY);

            cell_paddx = pad_awarx->read_int();
            cell_paddy = pad_awary->read_int();

            if (cell_paddx<0 && -cell_paddx >= cell_width) {
                cell_paddx = -cell_width+1;
                pad_awarx->write_int(cell_paddx);
            }
            if (cell_paddy<0 && -cell_paddy >= cell_height) {
                cell_paddy = -cell_height+1;
                pad_awary->write_int(cell_paddy);
            }
        }

        cell_width  += cell_paddx;
        cell_height += cell_paddy;
    }

    {
        const AW_font_limits& lim = device->get_font_limits(DI_G_NAMES, 0);
 
        off_dx = awr->awar(AWAR_MATRIX_NAMECHARS_LEFT)->read_int() * lim.width + 1 + cell_paddx;
        off_dy = lim.height + cell_height; // off_dy corresponds to "lower" y of cell
    }

    if (m) {
        total_cells_horiz=m->nentries;
        total_cells_vert=m->nentries;
    }
    set_scrollbar_steps(cell_width, cell_height, 50, 50);

    mark(NEED_RESIZE);
}

void MatrixDisplay::adapt_to_canvas_size() {
    const AW_font_limits& lim = device->get_font_limits(DI_G_STANDARD, 0);

    DI_MATRIX *m = get_matrix();
    long       n = 0;

    if (m) n = m->nentries;

    const AW_screen_area& squ = device->get_area_size();
    screen_width  = squ.r-squ.l;
    screen_height = squ.b-squ.t;

    AW_screen_area rect; // @@@ used uninitialized if !m
    if (m) {
        long horiz_paint_size = (squ.r-lim.width-off_dx)/cell_width;
        long vert_paint_size  = (squ.b-off_dy)/cell_height;

        horiz_page_size = (n > horiz_paint_size) ?  horiz_paint_size : n;
        vert_page_size  = (n > vert_paint_size) ? vert_paint_size : n;

        rect.l = 0;
        rect.t = 0;
        rect.r = (int)((n-horiz_page_size)*cell_width+squ.r);
        rect.b = (int)((n-vert_page_size)*cell_height+squ.b);
    }
    else {
        horiz_page_size = 0;
        vert_page_size  = 0;
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

    mark(NEED_CLEAR);
}

enum ClickAction {
    CLICK_SELECT_SPECIES = 1,
    CLICK_SET_MINMAX,
};

#define MINMAX_GRANULARITY 10000L

void MatrixDisplay::scroll_to(int sxpos, int sypos) {
    sxpos = force_in_range(0, sxpos, int(awm->get_scrolled_picture_width())-screen_width);
    sypos = force_in_range(0, sypos, int(awm->get_scrolled_picture_height())-screen_height);

    awm->set_horizontal_scrollbar_position(sxpos);
    awm->set_vertical_scrollbar_position(sypos);

    monitor_vertical_scroll_cb(awm);
    monitor_horizontal_scroll_cb(awm);
}

void MatrixDisplay::scroll_cells(int cells_x, int cells_y) {
    scroll_to(awm->slider_pos_horizontal + cells_x*cell_width,
              awm->slider_pos_vertical + cells_y*cell_height);
}

void MatrixDisplay::handle_move(AW_event& event) {
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

        scroll_to(startx + x_screen_diff, starty + y_screen_diff);
    }
}

static void motion_cb(AW_window *aww, MatrixDisplay *disp) {
    AW_event event;
    aww->get_event(&event);

    if (event.button == AW_BUTTON_MIDDLE) {
        disp->handle_move(event);
    }
}

static void input_cb(AW_window *aww, MatrixDisplay *disp) {
    AW_event event;
    aww->get_event(&event);

    if (event.button == AW_WHEEL_UP || event.button == AW_WHEEL_DOWN) {
        if (event.type == AW_Mouse_Press) {
            bool horizontal = event.keymodifier & AW_KEYMODE_ALT;
            int  direction  = event.button == AW_WHEEL_UP ? -1 : 1;
            disp->scroll_cells(horizontal*direction, !horizontal*direction);
        }
    }
    else if (event.button == AW_BUTTON_MIDDLE) {
        disp->handle_move(event);
    }
    else {
        AW_device_click *click_device = aww->get_click_device(AW_MIDDLE_AREA, event.x, event.y, AWT_CATCH);

        click_device->set_filter(AW_CLICK);
        click_device->reset();

        {
            AW_device *oldMatrixDevice = disp->device;

            disp->device = click_device;
            disp->update_display(); // detect clicked element
            disp->device = oldMatrixDevice;
        }

        if (event.type == AW_Mouse_Press) {
            AWT_graphic_event   gevent(AWT_MODE_NONE, event, false, click_device);
            const AW_clicked_element *clicked = gevent.best_click();

            if (clicked) {
                ClickAction action = static_cast<ClickAction>(clicked->cd1());

                if (action == CLICK_SELECT_SPECIES) {
                    size_t     idx    = size_t(clicked->cd2());
                    DI_MATRIX *matrix = disp->get_matrix();
                    if (idx >= matrix->nentries) {
                        aw_message(GBS_global_string("Illegal idx %zi [allowed: 0-%zi]", idx, matrix->nentries));
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
                        double val = double(clicked->cd2())/MINMAX_GRANULARITY;
                        awar_bound->write_float(val);
                    }
                }
            }
        }
    }
}

void MatrixDisplay::draw() {
    // draw matrix

    if (!device) return;

    long x, y, xpos, ypos;

    DI_MATRIX *m = get_matrix();
    if (!autopop(m)) return;

    GB_transaction ta(GLOBAL_gb_main);

    if (beforeUpdate&NEED_CLEAR) device->clear(-1);
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
                    int maxw = cell_width-cell_paddx;

                    int h = cell_height - cell_paddy-1;

                    int hbox, hruler;
                    if (cell_paddy >= 0) {
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
                        device->box(DI_G_RULER_DISPLAY, AW::FillStyle::SOLID, x2, y1, int(len+0.5), hbox);
                    }
                    else {
                        device->text(DI_G_STANDARD, "???", cellx, celly);
                    }

                    if (hruler) { // do not paint ruler if cell_paddy is negative
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
        AW_pos linex1 = sel_x_pos*cell_width - cell_paddx/2-1;
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
        AW_pos liney2 = sel_y_pos*cell_height + cell_paddy/2+1;
        AW_pos liney1 = liney2-cell_height;
        AW_pos width  = area.width();
        device->line(DI_G_STANDARD, 0, liney1, width, liney1);
        device->line(DI_G_STANDARD, 0, liney2, width, liney2);
    }

    free(selSpecies);

    device->set_offset(AW::Vector(0, 0));
#undef BUFLEN
}

void MatrixDisplay::set_scrollbar_steps(long width_h, long width_v, long page_h, long page_v) {
    awm->window_local_awar("scroll_width_horizontal")->write_int(width_h);
    awm->window_local_awar("scroll_width_vertical")->write_int(width_v);
    awm->window_local_awar("horizontal_page_increment")->write_int(page_h);
    awm->window_local_awar("vertical_page_increment")->write_int(page_v);
}

#if defined(WARN_TODO)
#warning test scrolling again with fixed box_impl()
#endif

void MatrixDisplay::monitor_vertical_scroll_cb(AW_window *aww) { // draw area
    if (!device) return;

    int old_vert_page_start = vert_page_start;

    vert_last_view_start = aww->slider_pos_vertical;
    vert_page_start      = aww->slider_pos_vertical/cell_height;

    int diff = vert_page_start-old_vert_page_start; // amount of rows to scroll
    if (diff) {
        int diff_pix   = abs(diff)*cell_height;
        int top_y      = off_dy-cell_height;
        int keep_cells = vert_page_size-abs(diff);
        int keep_pix   = keep_cells*cell_height;

        if (diff>0 && diff<vert_page_size) { // scroll some positions up
            device->move_region(0, top_y+diff_pix, screen_width, keep_pix, 0, top_y);
            device->clear_part (0, top_y+keep_pix, screen_width, diff_pix, AW_SCREEN);
            device->push_clip_scale();
            device->set_top_clip_border(top_y+keep_pix, true);
        }
        else if (diff>-vert_page_size && diff<0) { // scroll some positions down
            device->move_region(0, top_y, screen_width, keep_pix, 0, top_y+diff_pix);
            device->clear_part (0, top_y, screen_width, diff_pix, AW_SCREEN);
            device->push_clip_scale();
            device->set_bottom_clip_border(top_y+diff_pix, true);
        }
        else { // repaint
            device->push_clip_scale();
            mark(NEED_CLEAR);
        }

        update_display();
        device->pop_clip_scale();
    }
}

void MatrixDisplay::monitor_horizontal_scroll_cb(AW_window *aww) { // draw area
    if (!device) return;

    int old_horiz_page_start = horiz_page_start;

    horiz_last_view_start = aww->slider_pos_horizontal;
    horiz_page_start      = aww->slider_pos_horizontal/cell_width;

    int diff = horiz_page_start-old_horiz_page_start; // amount of columns to scroll

    if (diff) {
        int diff_pix   = abs(diff)*cell_width;
        int keep_cells = horiz_page_size-abs(diff);
        int keep_pix   = keep_cells*cell_width;

        if (diff>0 && diff<horiz_page_size) {      // scroll some positions left
            device->move_region(off_dx+diff_pix, 0, keep_pix, screen_height, off_dx, 0);
            device->clear_part (off_dx+keep_pix, 0, diff_pix, screen_height, AW_SCREEN);
            device->push_clip_scale();
            device->set_left_clip_border(keep_pix, true);
        }
        else if (diff>-horiz_page_size && diff<0) { // scroll some positions right
            device->move_region(off_dx, 0, keep_pix, screen_height, off_dx+diff_pix, 0);
            device->clear_part (off_dx, 0, diff_pix, screen_height, AW_SCREEN);
            device->push_clip_scale();
            device->set_right_clip_border(off_dx+diff_pix, true);
        }
        else { // repaint
            device->push_clip_scale();
            mark(NEED_CLEAR);
        }

        update_display();
        device->pop_clip_scale();
    }
}

static bool update_display_on_dist_change = true;

static void di_view_set_max_dist(AW_window *aww, int max_dist) {
    AW_root *aw_root = aww->get_root();
    {
        LocallyModify<bool> flag(update_display_on_dist_change, false);
        aw_root->awar(AWAR_DIST_MIN_DIST)->write_float(0.0);
    }
    aw_root->awar(AWAR_DIST_MAX_DIST)->write_float(max_dist*0.01);
}

static void di_view_set_distances(AW_root *awr, int setmax, MatrixDisplay *disp) {
    // cl_dmatrix: 0 -> set min and fix max, 1 -> set max and fix min, 2 -> set both
    double max_dist = awr->awar(AWAR_DIST_MAX_DIST)->read_float();
    double min_dist = awr->awar(AWAR_DIST_MIN_DIST)->read_float();

    {
        LocallyModify<bool> flag(update_display_on_dist_change, false);

        switch (setmax) {
            case 2:                 // both
                disp->set_slider_max(max_dist);
                // fall-through
            case 0:                 // set min and fix max
                disp->set_slider_min(min_dist);
                if (min_dist>max_dist) awr->awar(AWAR_DIST_MAX_DIST)->write_float(min_dist);
                break;
            case 1:                 // set max and fix min
                disp->set_slider_max(max_dist);
                if (min_dist>max_dist) awr->awar(AWAR_DIST_MIN_DIST)->write_float(max_dist);
                break;

            default:
                di_assert(0);
                break;
        }
    }
    if (update_display_on_dist_change) {
        disp->mark(MatrixDisplay::NEED_CLEAR);
        disp->update_display();
    }
}

static void di_bind_dist_awars(AW_root *aw_root, MatrixDisplay *disp) {
    aw_root->awar_float(AWAR_DIST_MIN_DIST)->add_callback(makeRootCallback(di_view_set_distances, 0, disp));
    aw_root->awar_float(AWAR_DIST_MAX_DIST)->add_callback(makeRootCallback(di_view_set_distances, 1, disp));
}

static void create_matrix_awars(AW_root *awr, MatrixDisplay *disp) {
    RootCallback reinit_needed_cb = makeRootCallback(reinit_needed, disp);

    awr->awar_int(AWAR_MATRIX_SHOWZERO,       1)                      ->add_callback(reinit_needed_cb);
    awr->awar_int(AWAR_MATRIX_PADDINGX,       4) ->set_minmax(-10, 10)->add_callback(reinit_needed_cb);
    awr->awar_int(AWAR_MATRIX_PADDINGY,       4) ->set_minmax(-10, 10)->add_callback(reinit_needed_cb);
    awr->awar_int(AWAR_MATRIX_DIGITS,         4) ->set_minmax(0, 10)  ->add_callback(reinit_needed_cb);
    awr->awar_int(AWAR_MATRIX_NAMECHARS_TOP,  8) ->set_minmax(0, 10)  ->add_callback(reinit_needed_cb);
    awr->awar_int(AWAR_MATRIX_NAMECHARS_LEFT, 10)->set_minmax(0, 10)  ->add_callback(reinit_needed_cb);
}

static AWT_config_mapping_def matrixConfigMapping[] = {
    { AWAR_MATRIX_PADDINGX,       "paddingx" },
    { AWAR_MATRIX_PADDINGY,       "paddingy" },
    { AWAR_MATRIX_SHOWZERO,       "showzero" },
    { AWAR_MATRIX_DIGITS,         "precision" },
    { AWAR_MATRIX_NAMECHARS_TOP,  "namechars_top" },
    { AWAR_MATRIX_NAMECHARS_LEFT, "namechars_left" },

    { 0, 0 }
};

static AW_window *create_matrix_settings_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "MATRIX_SETTINGS", "Matrix settings");

    const int FIELDWIDTH = 3;
    const int SCALERWIDTH = 200;

    aws->auto_space(10, 10);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("matrix_settings.hlp"));
    aws->create_button("HELP", "HELP");

    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "matrix_settings", matrixConfigMapping);

    aws->label_length(21);

    aws->at_newline();
    aws->label("X-padding (pixels)");
    aws->create_input_field_with_scaler(AWAR_MATRIX_PADDINGX, FIELDWIDTH, SCALERWIDTH);

    aws->at_newline();
    aws->label("Y-padding (pixels)");
    aws->create_input_field_with_scaler(AWAR_MATRIX_PADDINGY, FIELDWIDTH, SCALERWIDTH);

    aws->at_newline();
    aws->label("Show leading zero");
    aws->create_toggle(AWAR_MATRIX_SHOWZERO);

    aws->at_newline();
    aws->label("Precision (digits)");
    aws->create_input_field_with_scaler(AWAR_MATRIX_DIGITS, FIELDWIDTH, SCALERWIDTH);

    aws->at_newline();
    aws->label("Min. namechars (top)");
    aws->create_input_field_with_scaler(AWAR_MATRIX_NAMECHARS_TOP, FIELDWIDTH, SCALERWIDTH);

    aws->at_newline();
    aws->label("Min. namechars (left)");
    aws->create_input_field_with_scaler(AWAR_MATRIX_NAMECHARS_LEFT, FIELDWIDTH, SCALERWIDTH);

    aws->window_fit();
    return aws;
}

static void selected_species_changed_cb(AW_root*, MatrixDisplay *disp) {
    if (disp) redisplay_needed(NULL, disp);
}

AW_window *DI_create_view_matrix_window(AW_root *awr, MatrixDisplay *disp, save_matrix_params *sparam) {
    di_bind_dist_awars(awr, disp);
    create_matrix_awars(awr, disp);
    
    AW_window_menu *awm = new AW_window_menu;
    awm->init(awr, "SHOW_MATRIX", "ARB distance matrix", 800, 600);

    disp->device = awm->get_device(AW_MIDDLE_AREA);
    disp->awm    = awm;

    awr->awar(AWAR_SPECIES_NAME)->add_callback(makeRootCallback(selected_species_changed_cb, disp));
    
    awm->set_vertical_change_callback  (makeWindowCallback(vertical_change_cb,   disp));
    awm->set_horizontal_change_callback(makeWindowCallback(horizontal_change_cb, disp));

    awm->set_resize_callback(AW_MIDDLE_AREA, makeWindowCallback(resize_needed,    disp));
    awm->set_expose_callback(AW_MIDDLE_AREA, makeWindowCallback(redisplay_needed, disp));
    awm->set_input_callback (AW_MIDDLE_AREA, makeWindowCallback(input_cb,         disp));
    awm->set_motion_callback(AW_MIDDLE_AREA, makeWindowCallback(motion_cb,        disp));

    AW_gc_manager gc_manager =
        AW_manage_GC(awm,
                     awm->get_window_id(),
                     disp->device, DI_G_STANDARD, DI_G_LAST, AW_GCM_DATA_AREA,
                     makeGcChangedCallback(gc_changed_cb, disp),
                     false,
                     "#D0D0D0",
                     "#Standard$#000000",
                     "#Names$#000000",
                     "+-Ruler$#555", "-Display$#00AA55",
                     "#BelowDist$#008732",
                     "#AboveDist$#DB008D",
                     NULL);

    awm->create_menu("File", "F");
    awm->insert_menu_topic("save_matrix", "Save Matrix to File", "S", "save_matrix.hlp", AWM_ALL, makeCreateWindowCallback(DI_create_save_matrix_window, sparam));
    awm->insert_menu_topic("close",       "Close",               "C", 0,                 AWM_ALL, AW_POPDOWN);

    awm->create_menu("Range", "R");
    awm->insert_menu_topic("deselect_range",    "Deselect range",           "D", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 0));
    awm->insert_menu_topic("show_dist_species", "Species range [ <=  2% ]", "S", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 2));
    awm->insert_menu_topic("show_dist_genus",   "Genus range   [ <=  5% ]", "G", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 5));
    awm->insert_menu_topic("show_dist_010",     "Range         [ <= 10% ]", "1", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 10));
    awm->insert_menu_topic("show_dist_025",     "Range         [ <= 25% ]", "2", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 25));
    awm->insert_menu_topic("show_dist_050",     "Range         [ <= 50% ]", "5", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 50));
    awm->insert_menu_topic("show_dist_100",     "Whole range   [ 0-100% ]", "0", 0, AWM_ALL, makeWindowCallback(di_view_set_max_dist, 100));

    awm->create_menu("Properties", "P");
    awm->insert_menu_topic("matrix_settings", "Settings ...",               "S", "matrix_settings.hlp", AWM_ALL, create_matrix_settings_window);
    awm->insert_menu_topic("matrix_colors",   "Colors and Fonts ...",       "C", "color_props.hlp",     AWM_ALL, makeCreateWindowCallback(AW_create_gc_window, gc_manager));
    awm->insert_menu_topic("save_props",      "Save Properties (dist.arb)", "P", "savedef.hlp",         AWM_ALL, AW_save_properties);

#define FIELD_SIZE  7
#define SCALER_SIZE 200

    awm->auto_space(5, 5);

    awm->label("Dist min:"); awm->create_input_field_with_scaler(AWAR_DIST_MIN_DIST, FIELD_SIZE, SCALER_SIZE, AW_SCALER_EXP_LOWER);
    awm->label("Dist max:"); awm->create_input_field_with_scaler(AWAR_DIST_MAX_DIST, FIELD_SIZE, SCALER_SIZE, AW_SCALER_EXP_LOWER);

    awm->set_info_area_height(35);

    di_view_set_distances(awm->get_root(), 2, disp);

    return awm;
}
