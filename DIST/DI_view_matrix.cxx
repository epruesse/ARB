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

#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>

#include <awt_canvas.hxx>

void vertical_change_cb  (AW_window *aww,DI_dmatrix *dis) { dis->monitor_vertical_scroll_cb(aww); }
void horizontal_change_cb(AW_window *aww,DI_dmatrix *dis) { dis->monitor_horizontal_scroll_cb(aww); }

void redisplay_needed(AW_window *,DI_dmatrix *dis) {
    dis->display(true);
}
void resize_needed(AW_window *,DI_dmatrix *dis) {
    dis->init();
    dis->resized();
    dis->display(false);
}

DI_dmatrix::DI_dmatrix() {
    memset((char *) this,0,sizeof(DI_dmatrix));
}

void DI_dmatrix::init (DI_MATRIX *matrix) {
    di_matrix = matrix;
    DI_MATRIX *m = get_matrix();

    // calculate cell width and height
    {
        int max_cell_width = 0;
        int max_cell_height = 0;

        DI_gc gc;
        for (gc=DI_G_STANDARD; gc<=DI_G_LAST; gc = DI_gc(int(gc)+1)) {
            int height = 0;
            int width = 0;

            switch (gc) {
                case DI_G_STANDARD:
                case DI_G_BELOW_DIST:
                case DI_G_ABOVE_DIST: {
                    const AW_font_information *aw_fi = device->get_font_information(DI_G_STANDARD, 0);

                    width  = aw_fi->max_letter.width*6; // normal cell contain 6 characters (e.g.: '0.0162')
                    height = aw_fi->max_letter.height*2;
                    break;
                }
                case DI_G_NAMES: {
                    const AW_font_information *aw_fi = device->get_font_information(DI_G_STANDARD, 0);

                    width  = aw_fi->max_letter.width*SPECIES_NAME_LEN; // normal cell contain 6 characters (e.g.: '0.0162')
                    height = aw_fi->max_letter.height*2;
                    break;
                }
                default: {
                    break;
                }
            }

            if (height>max_cell_height) max_cell_height = height;
            if (width>max_cell_width) max_cell_width = width;
        }

        cell_width = max_cell_width;
        cell_height = max_cell_height;
    }

    cell_offset = 10;  // draw cell_offset pixels above cell base_line

    off_dx = cell_width + 2*cell_offset;
    off_dy = 3*cell_height;

    if (m){
        total_cells_horiz=m->nentries;
        total_cells_vert=m->nentries;
    }
    set_scrollbar_steps( cell_width,cell_height,50,50);
    resized();  // initialize window_size dependent parameters
}

DI_MATRIX *DI_dmatrix::get_matrix(){
    if (di_matrix) return di_matrix;
    return DI_MATRIX::ROOT;
}

void DI_dmatrix::resized(void)
{
    AW_rectangle               squ;
    AW_rectangle               rect;
    long                       horiz_paint_size,vert_paint_size;
    const AW_font_information *aw_fi = device->get_font_information(DI_G_STANDARD,0);
    DI_MATRIX                  *m     = get_matrix();
    long                       n     = 0;

    if (m) n = m->nentries;
    device->get_area_size(&squ);

    screen_width  = squ.r-squ.l;
    screen_height = squ.b-squ.t;

    if (m) {
        horiz_paint_size = (squ.r-aw_fi->max_letter.width-off_dx)/cell_width;
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
    aww->get_event( &event );

    DI_dmatrix *dmatrix = reinterpret_cast<DI_dmatrix*>(cl_dmatrix);
    if (event.button == AWT_M_MIDDLE) {
        dmatrix->handle_move(event);
    }
}

static void input_cb(AW_window *aww, AW_CL cl_dmatrix, AW_CL) {
    AW_event event;
    aww->get_event( &event );

    DI_dmatrix *dmatrix = reinterpret_cast<DI_dmatrix*>(cl_dmatrix);
    if (event.button == AWT_M_MIDDLE) {
        dmatrix->handle_move(event);
    }
    else {
        AW_device *click_device = aww->get_click_device(AW_MIDDLE_AREA, event.x, event.y, 20, 20, 0);

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

            AW_CL        cd1, cd2;
            AW::Position clickPos(event.x, event.y);

            if (AW_getBestClick(clickPos, &clicked_line, &clicked_text, &cd1, &cd2)) {
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
                        case AWT_M_LEFT:  awar_bound = aw_root->awar(AWAR_DIST_MIN_DIST); break;
                        case AWT_M_RIGHT: awar_bound = aw_root->awar(AWAR_DIST_MAX_DIST); break;
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

void DI_dmatrix::display(bool clear)   // draw area
{
#define BUFLEN 200
    char           buf[BUFLEN];
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

    int name_display_width; {
        const AW_font_information *aw_fi = device->get_font_information(DI_G_NAMES,0);
        name_display_width = cell_width/aw_fi->max_letter.width;
    }
    di_assert(name_display_width<BUFLEN);

    int sel_x_pos = -1;

    for (x = horiz_page_start;
         x < (horiz_page_start + horiz_page_size) && (x < total_cells_horiz);
         x++)
    {
        ypos = 0;
        for (y = vert_page_start;
             y < (vert_page_start + vert_page_size) && (y < total_cells_vert);
             y++)
        {
            double val2   = m->matrix->get(x, y);
            AW_CL  cd_val = AW_CL(val2*MINMAX_GRANULARITY+1);

            if (val2>=min_view_dist && val2<=max_view_dist && val2>0.0) { // display ruler
                int maxw = (int)(cell_width * .75);
                int h = cell_height /2 ;
                int y2 = ypos * cell_height - cell_offset - 10;
                int x2 = xpos * cell_width;
                double len = ((val2-min_view_dist)/(max_view_dist-min_view_dist)) * maxw;
                if (len >= 0) {
                    device->box(DI_G_RULER_DISPLAY, true, x2, y2,len, h*.8,-1,AW_CL(CLICK_SET_MINMAX), cd_val);
                }
                else {
                    device->text(DI_G_STANDARD, "????", xpos * cell_width, ypos * cell_height - cell_offset, 0.0, -1, AW_CL(CLICK_SET_MINMAX), cd_val);
                }
                double v;
                for (v = x2; v < x2 + maxw; v += maxw * .1999) {
                    device->line (DI_G_RULER, v, y2+h*.5, v, y2 + h, -1,AW_CL(CLICK_SET_MINMAX), cd_val);
                }
                device->line(DI_G_RULER, x2, y2+h, x2+maxw-1, y2+h, -1, AW_CL(CLICK_SET_MINMAX), cd_val);
            }
            else {
                DI_gc gc = val2<min_view_dist ? DI_G_BELOW_DIST : (val2>max_view_dist ? DI_G_ABOVE_DIST : DI_G_STANDARD);

                if (val2 == 0.0 || val2 == 1.0) {
                    sprintf(buf, "%3.1f", val2);
                    gc = DI_G_STANDARD;
                }
                else {
                    sprintf(buf, "%6.4f", val2);
                }
                device->text(gc, buf, xpos * cell_width, ypos * cell_height - cell_offset, 0.0, -1, AW_CL(CLICK_SET_MINMAX), cd_val);
            }

            ypos++;
        }
        //display horizontal speciesnames:

        strcpy(buf, m->entries[x]->name);
        if (selSpecies && strcmp(buf, selSpecies) == 0) sel_x_pos = xpos; // remember x-position for selected species
        buf[name_display_width] = 0; // cut group-names if too long
        device->text(DI_G_NAMES, buf, xpos * cell_width, cell_height - off_dy - cell_offset, 0.0, -1, AW_CL(CLICK_SELECT_SPECIES), AW_CL(x));
        xpos++;
    }

    device->set_offset(AW::Vector(off_dx, 0));

    AW::Rectangle area = device->get_area_size();

    // highlight selected species (vertically)
    if (sel_x_pos != -1) {
        AW_pos linex1 = sel_x_pos*cell_width - cell_offset;
        AW_pos linex2 = linex1+cell_width;
        AW_pos height = area.height();
        device->line(DI_G_STANDARD, linex1, 0, linex1, height, -1, 0, 0);
        device->line(DI_G_STANDARD, linex2, 0, linex2, height, -1, 0, 0);
    }

    device->set_offset(AW::Vector(0, off_dy));

    // display vertical speciesnames
    ypos          = 0;
    int sel_y_pos = -1;
    for (y = vert_page_start; y < vert_page_start + vert_page_size; y++) {
        strcpy(buf, m->entries[y]->name);
        if (selSpecies && strcmp(buf, selSpecies) == 0) sel_y_pos = ypos; // remember x-position for selected species
        buf[name_display_width] = 0; // cut group-names if too long
        device->text(DI_G_NAMES, buf, 0, ypos * cell_height - cell_offset, 0.0, -1, AW_CL(CLICK_SELECT_SPECIES), AW_CL(y));
        ypos++;
    }

    // highlight selected species
    if (sel_y_pos != -1) {
        AW_pos liney1 = (sel_y_pos-1)*cell_height;
        AW_pos liney2 = liney1+cell_height;
        AW_pos width = area.width();
        device->line(DI_G_STANDARD, 0, liney1, width, liney1, -1, 0, 0);
        device->line(DI_G_STANDARD, 0, liney2, width, liney2, -1, 0, 0);
    }

    device->set_offset(AW::Vector(0, 0));
#undef BUFLEN
}

void DI_dmatrix::set_scrollbar_steps(long width_h,long width_v,long page_h,long page_v)
{
    char buffer[200];

    sprintf(buffer,"window/%s/scroll_width_horizontal",awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(width_h);
    sprintf(buffer,"window/%s/scroll_width_vertical",awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(width_v);
    sprintf( buffer,"window/%s/horizontal_page_increment",awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(page_h);
    sprintf(buffer,"window/%s/vertical_page_increment",awm->window_defaults_name);
    awm->get_root()->awar(buffer)->write_int(page_v);
}


void DI_dmatrix::monitor_vertical_scroll_cb(AW_window *aww) { // draw area
    if (!device) return;

    long old_vert_page_start = vert_page_start;

    vert_last_view_start = aww->slider_pos_vertical;
    vert_page_start      = aww->slider_pos_vertical/cell_height;

    long diff = vert_page_start-old_vert_page_start;
    if (diff) {
        int  top_y = off_dy-cell_height;
        bool clear = false;

        if (diff>0 && diff<vert_page_size) { // scroll some positions up
            int keep_cells = vert_page_size-diff;
            
            device->move_region(0, top_y+diff*cell_height, screen_width, keep_cells*cell_height, 0, top_y);
            device->clear_part(0, top_y+keep_cells*cell_height, screen_width, diff*cell_height, -1);
            device->push_clip_scale();
            device->set_top_clip_border((int)(top_y+keep_cells*cell_height));
        }
        else if (diff>-vert_page_size && diff<0) { // scroll some positions down
            int keep_cells = vert_page_size+diff;
            
            device->move_region(0, top_y, screen_width, keep_cells*cell_height, 0, top_y+(-diff*cell_height));
            device->clear_part(0, top_y, screen_width, cell_height*-diff, -1);
            device->push_clip_scale();
            device->set_bottom_clip_border((int)(top_y+(-diff*cell_height)));
        }
        else {
            device->push_clip_scale();
            clear = true;
        }

        display(clear);
        device->pop_clip_scale();
    }
}

void DI_dmatrix::monitor_horizontal_scroll_cb(AW_window *aww) { // draw area 
    if (!device) return;

    long old_horiz_page_start = horiz_page_start;
    
    horiz_last_view_start = aww->slider_pos_horizontal;
    horiz_page_start      = aww->slider_pos_horizontal/cell_width;

    long diff = horiz_page_start-old_horiz_page_start;

    if (diff) {
        bool clear = false;
        
        if (diff>0 && diff<horiz_page_size) {      // scroll some positions left
            int keep_cells = horiz_page_size-diff;

            device->move_region(off_dx+diff*cell_width, 0, keep_cells*cell_width, screen_height, off_dx, 0);
            device->clear_part(off_dx+keep_cells*cell_width, 0, diff*cell_width, screen_height, -1);
            device->push_clip_scale();
            device->set_left_clip_border((int)(keep_cells*cell_width));
        }
        else if (diff>-horiz_page_size && diff<0) { // scroll some positions right
            int keep_cells = horiz_page_size+diff;
            
            device->move_region(off_dx, 0, keep_cells*cell_width, screen_height, off_dx+cell_width*-diff, 0);
            device->clear_part(off_dx, 0, cell_width*-diff, screen_height, -1);
            device->push_clip_scale();
            device->set_right_clip_border((int)(off_dx+cell_width*-diff));
        }
        else {
            device->push_clip_scale();
            clear = true;
        }

        display(clear);
        device->pop_clip_scale();
    }
}

static int update_display_on_dist_change = 1;

void di_view_set_max_d(AW_window *aww, AW_CL cl_max_d, AW_CL /*clmatr*/){
    AWUSE(aww);
    double   max_d   = cl_max_d*0.01;
    AW_root *aw_root = aww->get_root();

    update_display_on_dist_change = 0;
    aw_root->awar(AWAR_DIST_MIN_DIST)->write_float(0.0);
    update_display_on_dist_change = 1;
    aw_root->awar(AWAR_DIST_MAX_DIST)->write_float(max_d);
}

void di_view_set_distances(AW_root *awr, AW_CL cl_setmax, AW_CL cl_dmatrix) {
    // cl_dmatrix: 0 -> set min and fix max, 1 -> set max and fix min, 2 -> set both
    DI_dmatrix *dmatrix  = (DI_dmatrix *)cl_dmatrix;
    double      max_dist = awr->awar(AWAR_DIST_MAX_DIST)->read_float();
    double      min_dist = awr->awar(AWAR_DIST_MIN_DIST)->read_float();
    int         old      = update_display_on_dist_change;

    update_display_on_dist_change = 0;

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

    update_display_on_dist_change = old;
    if (update_display_on_dist_change) dmatrix->display(true);
}

void di_change_dist(AW_window *aww, AW_CL cl_mode) {
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

AW_window *DI_create_view_matrix_window(AW_root *awr, DI_dmatrix *dmatrix, save_matrix_params *sparam) {
    di_bind_dist_awars(awr, dmatrix);
    AW_window_menu *awm = new AW_window_menu();
    awm->init(awr,"SHOW_MATRIX", "ARB_SHOW_MATRIX", 800,600);

    dmatrix->device = awm->get_device(AW_MIDDLE_AREA);
    dmatrix->awm    = awm;

    awm->set_vertical_change_callback  ((AW_CB2)vertical_change_cb,   (AW_CL)dmatrix, 0);
    awm->set_horizontal_change_callback((AW_CB2)horizontal_change_cb, (AW_CL)dmatrix, 0);
    awm->set_focus_callback            ((AW_CB) redisplay_needed,     (AW_CL)dmatrix, 0);
    
    awm->set_resize_callback(AW_MIDDLE_AREA, (AW_CB2)resize_needed,    (AW_CL)dmatrix, 0);
    awm->set_expose_callback(AW_MIDDLE_AREA, (AW_CB2)redisplay_needed, (AW_CL)dmatrix, 0);
    awm->set_input_callback (AW_MIDDLE_AREA, (AW_CB) input_cb,         (AW_CL)dmatrix, 0);
    awm->set_motion_callback(AW_MIDDLE_AREA, (AW_CB) motion_cb,        (AW_CL)dmatrix, 0);

    AW_gc_manager preset_window =
        AW_manage_GC (awm,dmatrix->device, DI_G_STANDARD, DI_G_LAST, AW_GCM_DATA_AREA,
                      (AW_CB)resize_needed,(AW_CL)dmatrix,0,
                      false,
                      "#D0D0D0",
                      "#Standard$#000000",
                      "#Names$#000000",
                      "+-Ruler$#555", "-Display$#00AA55",
                      "#BelowDist$#008732",
                      "#AboveDist$#DB008D",
                      NULL);

    awm->create_menu("File","F");
    awm->insert_menu_topic("save_matrix",   "Save Matrix to File",  "S","save_matrix.hlp",  AWM_ALL, AW_POPUP, (AW_CL)DI_create_save_matrix_window, (AW_CL)sparam);
    awm->insert_menu_topic("close",     "Close",    "C",0,  AWM_ALL,    (AW_CB)AW_POPDOWN, (AW_CL)0, 0 );

    awm->create_menu("Props","P");
    awm->insert_menu_topic("props_matrix",      "Matrix: Colors and Fonts ...", "C","neprops_data.hlp"  ,   AWM_ALL,    AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)preset_window );
    awm->insert_menu_topic("show_dist_as_ascii",    "Show Dist in Ascii",           "A",0       ,   AWM_ALL,    di_view_set_max_d, 0, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_010",     "Show Dist [0,0.1]",            "1",0       ,   AWM_ALL,    di_view_set_max_d, 10, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_025",     "Show Dist [0,0.25]",           "3",0       ,   AWM_ALL,    di_view_set_max_d, 25, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_050",     "Show Dist [0,0.5]",            "5",0       ,   AWM_ALL,    di_view_set_max_d, 50, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_100",     "Show Dist [0,1.0]",            "0",0       ,   AWM_ALL,    di_view_set_max_d, 100, (AW_CL)dmatrix );

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

    return (AW_window *)awm;
}
