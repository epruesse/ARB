// =============================================================== //
//                                                                 //
//   File      : PH_display.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdb.h>
#include "phylo.hxx"
#include "phwin.hxx"
#include "PH_display.hxx"
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

static void vertical_change_cb(AW_window *aww) {
    PH_display::ph_display->monitor_vertical_scroll_cb(aww);
}

static void horizontal_change_cb(AW_window *aww) {
    PH_display::ph_display->monitor_horizontal_scroll_cb(aww);
}

void ph_view_species_cb() {
    AW_window *main_win = PH_used_windows::windowList->phylo_main_window;

    PH_display::ph_display->initialize_display(DISP_SPECIES);
    PH_display::ph_display->display();
    main_win->set_vertical_change_callback(makeWindowCallback(vertical_change_cb));
    main_win->set_horizontal_change_callback(makeWindowCallback(horizontal_change_cb));
}

GB_ERROR ph_check_initialized() {
    if (!PHDATA::ROOT) return "Please select alignment and press DONE";
    return 0;
}

void ph_view_filter_cb() {
    GB_ERROR err = ph_check_initialized();
    if (err) {
        aw_message(err);
    }
    else {
        AW_window *main_win  = PH_used_windows::windowList->phylo_main_window;
        PH_filter *ph_filter = new PH_filter;

        ph_filter->init(PHDATA::ROOT->get_seq_len());
        PHDATA::ROOT->markerline=ph_filter->calculate_column_homology();
        PH_display::ph_display->initialize_display(DISP_FILTER);
        PH_display::ph_display->display();
        main_win->set_vertical_change_callback(makeWindowCallback(vertical_change_cb));
        main_win->set_horizontal_change_callback(makeWindowCallback(horizontal_change_cb));
    }
}


PH_display::PH_display() {
    memset((char *) this, 0, sizeof(PH_display));
    this->display_what = DISP_NONE;
}


void PH_display::initialize_display(display_type dpyt) {
    display_what = dpyt;
    device=PH_used_windows::windowList->phylo_main_window->get_device(AW_MIDDLE_AREA);
    if (!device)
    {
        aw_message("could not get device !!");
        return;
    }
    const AW_font_limits& lim = device->get_font_limits(0, 0);
    switch (display_what) {
        case DISP_NONE:
            return;

        case DISP_SPECIES:
        case DISP_FILTER:
            cell_width  = lim.width;
            cell_height = lim.height+5;
            cell_offset = 3;

            off_dx = SPECIES_NAME_LEN*lim.width+20;
            off_dy = cell_height*3;

            total_cells_vert  = PHDATA::ROOT->nentries;
            set_scrollbar_steps(PH_used_windows::windowList->phylo_main_window, cell_width, cell_height, 50, 50);
            break;
    }
    resized();  // initialize window_size dependent parameters
}


void PH_display::resized() {
    const AW_screen_area& squ = PH_used_windows::windowList->phylo_main_window->get_device(AW_MIDDLE_AREA)-> get_area_size();
    screen_width              = squ.r-squ.l;
    screen_height             = squ.b-squ.t;

    AW_screen_area rect =  { 0, 0, 0, 0 };
    long         horiz_paint_size, vert_paint_size;
    switch (display_what) {
        case DISP_NONE:
            return;

        case DISP_SPECIES:
            horiz_paint_size = (squ.r-off_dx)/cell_width;
            vert_paint_size  = (squ.b-off_dy)/cell_height;
            horiz_page_size  = (PHDATA::ROOT->get_seq_len() > horiz_paint_size) ? horiz_paint_size : PHDATA::ROOT->get_seq_len();
            vert_page_size   = (long(PHDATA::ROOT->nentries) > vert_paint_size) ? vert_paint_size : PHDATA::ROOT->nentries;
            rect.l           = 0;
            rect.t           = 0;
            rect.r           = (int) ((PHDATA::ROOT->get_seq_len()-horiz_page_size)*cell_width+squ.r);
            rect.b           = (int) ((PHDATA::ROOT->nentries-vert_page_size)*cell_height+squ.b);
            break;

        case DISP_FILTER:
            horiz_paint_size  = (squ.r-off_dx)/cell_width;
            vert_paint_size   = (squ.b-off_dy)/cell_height;
            vert_paint_size  -= (3/8)/cell_height + 2;
            horiz_page_size   = (PHDATA::ROOT->get_seq_len() > horiz_paint_size) ? horiz_paint_size : PHDATA::ROOT->get_seq_len();
            vert_page_size    = (long(PHDATA::ROOT->nentries) > vert_paint_size) ? vert_paint_size : PHDATA::ROOT->nentries;
            rect.l            = 0;
            rect.t            = 0;
            rect.r            = (int) ((PHDATA::ROOT->get_seq_len()-horiz_page_size)*cell_width+squ.r);
            rect.b            = (int) ((PHDATA::ROOT->nentries-vert_page_size)*cell_height+squ.b);
            break;
    }

    horiz_page_start = 0; horiz_last_view_start = 0;
    vert_page_start  = 0; vert_last_view_start  = 0;

    device->reset();            // clip_size == device_size
    device->clear(-1);
    device->set_right_clip_border((int)(off_dx+cell_width*horiz_page_size));
    device->reset();            // reset shift_x and shift_y

    AW_window *pmw = PH_used_windows::windowList->phylo_main_window;
    pmw->set_vertical_scrollbar_position(0);
    pmw->set_horizontal_scrollbar_position(0);
    pmw->tell_scrolled_picture_size(rect);
    pmw->calculate_scrollbars();
}



void PH_display::display()       // draw area
{
    char buf[50], cbuf[2];
    long x, y, xpos, ypos;
    AW_window *main_win = PH_used_windows::windowList->phylo_main_window;
    long minhom;
    long maxhom;
    long startcol, stopcol;

    if (!PHDATA::ROOT) return; // not correctly initialized yet

    float *markerline = PHDATA::ROOT->markerline;

    if (!device) return;
    
    GB_transaction ta(PHDATA::ROOT->get_gb_main());
    // be careful: text origin is lower left
    switch (display_what) {
        case DISP_NONE:
            break;

        case DISP_SPECIES: {
            device->shift(AW::Vector(off_dx, off_dy));
            ypos      = 0;
            long ymax = std::min(vert_page_start+vert_page_size, total_cells_vert);
            for (y=vert_page_start; y<ymax; y++) {
                // species names
                device->text(0, PHDATA::ROOT->hash_elements[y]->name, -off_dx, ypos*cell_height-cell_offset);

                // alignment
                GBDATA     *gb_seq_data = PHDATA::ROOT->hash_elements[y]->gb_species_data_ptr;
                const char *seq_data    = GB_read_char_pntr(gb_seq_data);
                long        seq_len     = GB_read_count(gb_seq_data);

                device->text(0, (horiz_page_start >= seq_len) ? "" : (seq_data+horiz_page_start), 0, ypos*cell_height-cell_offset);
                ypos++;
            }
            device->shift(-AW::Vector(off_dx, off_dy));
            break;
        }
        case DISP_FILTER: {
            device->shift(AW::Vector(off_dx, off_dy));
            ypos=0;
            long ymax = std::min(vert_page_start+vert_page_size, total_cells_vert);
            for (y=vert_page_start; y<ymax; y++) {
                // species names
                device->text(0, PHDATA::ROOT->hash_elements[y]->name, -off_dx, ypos*cell_height-cell_offset);

                // alignment
                GBDATA     *gb_seq_data = PHDATA::ROOT->hash_elements[y]->gb_species_data_ptr;
                const char *seq_data    = GB_read_char_pntr(gb_seq_data);
                long        seq_len     = GB_read_count(gb_seq_data);

                device->text(0, (horiz_page_start >= seq_len) ? "" : (seq_data+horiz_page_start), 0, ypos*cell_height-cell_offset);
                ypos++;
            }
            xpos=0;
            cbuf[0]='\0'; cbuf[1]='\0';

            const AW_font_limits& lim = device->get_font_limits(0, 0);

            minhom   = main_win->get_root()->awar(AWAR_PHYLO_FILTER_MINHOM)->read_int();
            maxhom   = main_win->get_root()->awar(AWAR_PHYLO_FILTER_MAXHOM)->read_int();
            startcol = main_win->get_root()->awar(AWAR_PHYLO_FILTER_STARTCOL)->read_int();
            stopcol  = main_win->get_root()->awar(AWAR_PHYLO_FILTER_STOPCOL)->read_int();

            for (x = horiz_page_start; x < horiz_page_start + horiz_page_size; x++) {
                int             gc = 1;
                float       ml = markerline[x];
                if (x < startcol || x>stopcol) {
                    gc = 2;
                }
                if (markerline[x] >= 0.0) {
                    if (ml < minhom ||
                            ml > maxhom) {
                        gc = 2;
                    }
                    sprintf(buf, "%3.0f", ml);
                }
                else {
                    gc = 2;
                    sprintf(buf, "XXX");
                }

                for (y = 0; y < 3; y++) {
                    strncpy(cbuf, buf + y, 1);
                    device->text(gc, cbuf, xpos * cell_width + 1, vert_page_size * cell_height + y * lim.height);
                }
                xpos++;
            }
            device->shift(-AW::Vector(off_dx, off_dy));
            break;
        }
    }
}


void PH_display::set_scrollbar_steps(AW_window *aww, long width_h, long width_v, long page_h, long page_v) {
    aww->window_local_awar("scroll_width_horizontal")  ->write_int(width_h);
    aww->window_local_awar("scroll_width_vertical")    ->write_int(width_v);
    aww->window_local_awar("horizontal_page_increment")->write_int(page_h);
    aww->window_local_awar("vertical_page_increment")  ->write_int(page_v);
}


void PH_display::monitor_vertical_scroll_cb(AW_window *aww)    // draw area
{
    long diff;

    if (!device) return;
    if (vert_last_view_start==aww->slider_pos_vertical) return;

    diff = (aww->slider_pos_vertical-vert_last_view_start)/cell_height;
    // fast scroll: be careful: no transformation in move_region
    if (diff==1) // scroll one position up (== \/ arrow pressed)
    {
        device->move_region(0, off_dy, screen_width, vert_page_size*cell_height, 0, off_dy-cell_height);
        device->clear_part(0, off_dy-cell_height+(vert_page_size-1)*cell_height+1, screen_width, cell_height, -1);
        device->push_clip_scale();
        device->set_top_clip_border((int)(off_dy+(vert_page_size-2)*cell_height));
    }
    else if (diff==-1) // scroll one position down (== /\ arrow pressed)
    {
        device->move_region(0, off_dy-cell_height, screen_width, (vert_page_size-1)*cell_height+1, 0, off_dy);
        device->clear_part(0, off_dy-cell_height, screen_width, cell_height, -1);
        device->push_clip_scale();
        device->set_bottom_clip_border((int)off_dy);
    }
    else  device->clear(-1);

    vert_last_view_start = aww->slider_pos_vertical;
    vert_page_start      = aww->slider_pos_vertical/cell_height;

    display();
    if ((diff==1) || (diff==-1)) device->pop_clip_scale();
}

void PH_display::monitor_horizontal_scroll_cb(AW_window *aww)  // draw area
{
    long diff;

    if (!device) return;
    if (horiz_last_view_start==aww->slider_pos_horizontal) return;
    diff=(aww->slider_pos_horizontal- horiz_last_view_start)/cell_width;
    // fast scroll
    if (diff==1)  // scroll one position left ( > arrow pressed)
    {
        device->move_region(off_dx+cell_width, 0, horiz_page_size*cell_width, screen_height, off_dx, 0);
        device->clear_part(off_dx+(horiz_page_size-1)*cell_width, 0, cell_width, screen_height, -1);
        device->push_clip_scale();
        device->set_left_clip_border((int)((horiz_page_size-1)*cell_width));
    }
    else if (diff==-1) // scroll one position right ( < arrow pressed)
    {
        device->move_region(off_dx, 0, (horiz_page_size-1)*cell_width, screen_height, off_dx+cell_width,
                            0);
        device->clear_part(off_dx, 0, cell_width, screen_height, -1);
        device->push_clip_scale();
        device->set_right_clip_border((int)(off_dx+cell_width));
    }
    else device->clear(-1);

    horiz_last_view_start=aww->slider_pos_horizontal;
    horiz_page_start=aww->slider_pos_horizontal/cell_width;
    display();
    if ((diff==1) || (diff==-1)) device->pop_clip_scale();
}

PH_display_status::PH_display_status(AW_device *awd) {
    device = awd;
    if (!device) return;

    const AW_font_limits& lim = device->get_font_limits(0, 0);

    font_width  = lim.width;
    font_height = lim.height;

    device->reset();

    const AW_screen_area& rect = device->get_area_size();

    device->set_foreground_color(0, AW_WINDOW_FG);
    max_x   = (rect.r-rect.l)/font_width;
    max_y   = (rect.b-rect.t)/font_height;
    x_pos   = 0.0;
    y_pos   = 0.0;
    tab_pos = x_pos;
}

void PH_display_status::write(const char *text)
{
    device->text(0, text, x_pos*font_width, y_pos*font_height);
    x_pos+=strlen(text);
}

void PH_display_status::writePadded(const char *text, size_t len)
{
    device->text(0, text, x_pos*font_width, y_pos*font_height);
    x_pos += len;
}

void PH_display_status::write(long numl)
{
    char buf[20];

    sprintf(buf, "%ld", numl);
    write(buf);
}

void PH_display_status::write(AW_pos numA)
{
    char buf[20];

    sprintf(buf, "%3.3G", numA);
    write(buf);
}

void PH_display_status::clear() {
    device->clear(-1);
}

void display_status_cb() {
    // bottom area
    if (!PH_display::ph_display) return;
    if (!PH_used_windows::windowList) return;

    AW_root *aw_root = AW_root::SINGLETON;

    {
        static PH_display_status phds(PH_used_windows::windowList->phylo_main_window->get_device (AW_BOTTOM_AREA));
        phds.clear();

        const int LABEL_LEN = 21;

        switch (PH_display::ph_display->displayed()) {
            case DISP_NONE:
                break;

            case DISP_FILTER:
            case DISP_SPECIES:
                phds.set_origin();
                phds.set_cursor((phds.get_size('x')/2)-10, 0);
                phds.write("STATUS REPORT FILTER");
                phds.newline();

                phds.writePadded("Start at column:", LABEL_LEN);
                phds.write((long)aw_root->awar(AWAR_PHYLO_FILTER_STARTCOL)->read_int());
                phds.move_x(15);
                phds.set_tab();
                phds.writePadded("Stop at column:", LABEL_LEN);
                phds.write((long)aw_root->awar(AWAR_PHYLO_FILTER_STOPCOL)->read_int());
                phds.newline();

                phds.writePadded("Minimal similarity:", LABEL_LEN);
                phds.write((long)aw_root->awar(AWAR_PHYLO_FILTER_MINHOM)->read_int());
                phds.set_cursor_x(phds.get_tab());
                phds.writePadded("Maximal similarity:", LABEL_LEN);
                phds.write((long)aw_root->awar(AWAR_PHYLO_FILTER_MAXHOM)->read_int());
                phds.newline();
                phds.newline();

                phds.writePadded("'.':", LABEL_LEN);
                phds.write(filter_text[aw_root->awar(AWAR_PHYLO_FILTER_POINT)->read_int()]);
                phds.newline();

                phds.writePadded("'-':", LABEL_LEN);
                phds.write(filter_text[aw_root->awar(AWAR_PHYLO_FILTER_MINUS)->read_int()]);
                phds.newline();

                phds.writePadded("ambiguity codes:", LABEL_LEN);
                phds.write(filter_text[aw_root->awar(AWAR_PHYLO_FILTER_REST)->read_int()]);
                phds.newline();

                phds.writePadded("lowercase chars:", LABEL_LEN);
                phds.write(filter_text[aw_root->awar(AWAR_PHYLO_FILTER_LOWER)->read_int()]);
                break;
        }
    }
}

