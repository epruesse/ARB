#include "include.hxx"
#include "PH_display.hxx"

extern void display_status(AW_window *,AW_CL,AW_CL);
GB_ERROR    ph_check_initialized();

void vertical_change_cb(AW_window *aww,void *cb1,void *cb2)
{
    AWUSE(cb1); AWUSE(cb2);
    AP_display::apdisplay->monitor_vertical_scroll_cb(aww);
}

void horizontal_change_cb(AW_window *aww,void *cb1,void *cb2)
{
    AWUSE(cb1); AWUSE(cb2);
    AP_display::apdisplay->monitor_horizontal_scroll_cb(aww);
}

void ph_view_matrix_cb(AW_window *aww)
{
    AW_window *main_win = PH_used_windows::windowList->phylo_main_window;
    AWUSE(aww);

    AP_display::apdisplay->initialize(matrix_dpy);
    AP_display::apdisplay->display();
    main_win->set_vertical_change_callback((AW_CB2)vertical_change_cb,0,0);
    main_win->set_horizontal_change_callback((AW_CB2)horizontal_change_cb,0,0);
}

void ph_view_species_cb(AW_window *aww,AW_CL cb1,AW_CL cb2)
{
    AWUSE(aww); AWUSE(cb1); AWUSE(cb2);
    AW_window *main_win = PH_used_windows::windowList->phylo_main_window;

    AP_display::apdisplay->initialize(species_dpy);
    AP_display::apdisplay->display();
    main_win->set_vertical_change_callback((AW_CB2)vertical_change_cb,0,0);
    main_win->set_horizontal_change_callback((AW_CB2)horizontal_change_cb,0,0);
}

void ph_view_filter_cb(AW_window *aww,AW_CL ,AW_CL )
{
    GB_ERROR err = ph_check_initialized();
    if (err) {
        aw_message(err);
    }
    else {
        AW_window *main_win  = PH_used_windows::windowList->phylo_main_window;
        AWUSE(aww);
        PH_filter *ap_filter = new PH_filter;

        ap_filter->init(PHDATA::ROOT->get_seq_len());
        PHDATA::ROOT->markerline=ap_filter->calculate_column_homology();
        AP_display::apdisplay->initialize(filter_dpy);
        AP_display::apdisplay->display();
        main_win->set_vertical_change_callback((AW_CB2)vertical_change_cb,0,0);
        main_win->set_horizontal_change_callback((AW_CB2)horizontal_change_cb,0,0);
    }
}


AP_display::AP_display()
{
    memset((char *) this,0,sizeof(AP_display));
    this->display_what = NONE;
}


void AP_display::initialize (display_type dpyt)
{
    display_what = dpyt;
    device=PH_used_windows::windowList->phylo_main_window->get_device(AW_MIDDLE_AREA);
    if(!device)
    {
        aw_message("could not get device !!");
        return;
    }
    const AW_font_information *aw_fi=device->get_font_information(0,0);
    switch(display_what)
    {
        case NONE:
            return;
            
        case species_dpy:
        case filter_dpy:
            cell_width  = aw_fi->max_letter.width;
            cell_height = aw_fi->max_letter.height+5;
            cell_offset = 3;

            off_dx = SPECIES_NAME_LEN*aw_fi->max_letter.width+20;
            off_dy = cell_height*3;
            
            total_cells_horiz = PHDATA::ROOT->get_seq_len();
            total_cells_vert  = PHDATA::ROOT->nentries;
            set_scrollbar_steps(PH_used_windows::windowList->phylo_main_window, cell_width,cell_height,50,50);
            break;

        case matrix_dpy:
            cell_width  = aw_fi->max_letter.width*SPECIES_NAME_LEN;
            cell_height = aw_fi->max_letter.height*2;
            cell_offset = 10;   // draw cell_offset pixels above cell base_line

            off_dx = SPECIES_NAME_LEN*aw_fi->max_letter.width+20;
            off_dy = 3*cell_height;
            
            total_cells_horiz = PHDATA::ROOT->nentries;
            total_cells_vert  = PHDATA::ROOT->nentries;
            set_scrollbar_steps(PH_used_windows::windowList->phylo_main_window, cell_width,cell_height,50,50);
            break;

        default:
            aw_message("init: unknown display type (maybe not implemented yet)");
            break;
    }  // switch
    resized();  // initalize window_size dependend parameters
}


void AP_display::resized(void)
{
    AW_rectangle squ;
    AW_rectangle rect;
    long horiz_paint_size,vert_paint_size;

    PH_used_windows::windowList->phylo_main_window->get_device(AW_MIDDLE_AREA)-> get_area_size(&squ);
    screen_width  = squ.r-squ.l;
    screen_height = squ.b-squ.t;

    switch(display_what) {
        case NONE:
            return;

        case species_dpy:
            horiz_paint_size = (squ.r-off_dx)/cell_width;
            vert_paint_size  = (squ.b-off_dy)/cell_height;
            horiz_page_size  = (PHDATA::ROOT->get_seq_len() > horiz_paint_size) ? horiz_paint_size : PHDATA::ROOT->get_seq_len();
            vert_page_size   = (long(PHDATA::ROOT->nentries) > vert_paint_size) ? vert_paint_size : PHDATA::ROOT->nentries;
            rect.l           = 0;
            rect.t           = 0;
            rect.r           = (int) ((PHDATA::ROOT->get_seq_len()-horiz_page_size)*cell_width+squ.r);
            rect.b           = (int) ((PHDATA::ROOT->nentries-vert_page_size)*cell_height+squ.b);
            break;

        case matrix_dpy: {
            const AW_font_information *aw_fi = device->get_font_information(0,0);

            horiz_paint_size = (squ.r-aw_fi->max_letter.width-off_dx)/cell_width;
            vert_paint_size  = (squ.b-off_dy)/cell_height;
            horiz_page_size  = (long(PHDATA::ROOT->nentries) > horiz_paint_size) ? horiz_paint_size : PHDATA::ROOT->nentries;
            vert_page_size   = (long(PHDATA::ROOT->nentries) > vert_paint_size) ? vert_paint_size : PHDATA::ROOT->nentries;
            rect.l           = 0;
            rect.t           = 0;
            rect.r           = (int) ((PHDATA::ROOT->nentries-horiz_page_size)*cell_width+squ.r);
            rect.b           = (int) ((PHDATA::ROOT->nentries-vert_page_size)*cell_height+squ.b);
            break;
        }
        case filter_dpy:
            horiz_paint_size  = (squ.r-off_dx)/cell_width;
            vert_paint_size   = (squ.b-off_dy)/cell_height;
            vert_paint_size  -= (3/8)/cell_height +2;
            horiz_page_size   = (PHDATA::ROOT->get_seq_len() > horiz_paint_size) ? horiz_paint_size : PHDATA::ROOT->get_seq_len();
            vert_page_size    = (long(PHDATA::ROOT->nentries) > vert_paint_size) ? vert_paint_size : PHDATA::ROOT->nentries;
            rect.l            = 0;
            rect.t            = 0;
            rect.r            = (int) ((PHDATA::ROOT->get_seq_len()-horiz_page_size)*cell_width+squ.r);
            rect.b            = (int) ((PHDATA::ROOT->nentries-vert_page_size)*cell_height+squ.b);
            break;

        default:
            aw_message("resized: unknown display type (maybe not implemented yet)");
            break;
    }

    horiz_page_start = 0; horiz_last_view_start=0;
    vert_page_start  = 0; vert_last_view_start=0;

    device->reset();            // clip_size == device_size
    device->clear();
    device->set_right_clip_border((int)(off_dx+cell_width*horiz_page_size));
    device->reset();            // reset shift_x and shift_y

    PH_used_windows::windowList->phylo_main_window->set_vertical_scrollbar_position(0);
    PH_used_windows::windowList->phylo_main_window->set_horizontal_scrollbar_position(0);
    PH_used_windows::windowList->phylo_main_window->tell_scrolled_picture_size(rect);
    PH_used_windows::windowList->phylo_main_window->calculate_scrollbars();
}



void AP_display::display(void)   // draw area
{
    char buf[50],cbuf[2];
    long x,y,xpos,ypos;
    AW_window *main_win = PH_used_windows::windowList->phylo_main_window;
    // AW_font_information *aw_fi=0;
    long minhom;
    long maxhom;
    long startcol,stopcol;

    if (!PHDATA::ROOT) return; // not correctly initialized yet

    float *markerline = PHDATA::ROOT->markerline;

    if(!device) return;
    GB_transaction dummy(PHDATA::ROOT->gb_main);
    switch(display_what)  // be careful: text origin is lower left
    {
        case NONE: return;
        case species_dpy: device->shift_dy(off_dy);
            device->shift_dx(off_dx);
            ypos=0;
            for(y=vert_page_start;y<(vert_page_start+vert_page_size) &&
                    (y<total_cells_vert);y++)
            {
                device->text(0,PHDATA::ROOT->hash_elements[y]->name,-off_dx,
                             ypos*cell_height-cell_offset,
                             0.0,-1,0,0);                          // species names

                device->text(0,GB_read_char_pntr(
                                                 PHDATA::ROOT->hash_elements[y]->gb_species_data_ptr)+horiz_page_start,
                             0,ypos*cell_height-cell_offset,
                             0.0,-1,0,0);                          // alignment
                ypos++;
            }
            device->shift_dy(-off_dy);
            device->shift_dx(-off_dx);
            break;

        case matrix_dpy: device->shift_dx(off_dx);
            device->shift_dy(off_dy);
            xpos=0;
            for(x=horiz_page_start;x<(horiz_page_start+horiz_page_size) &&
                    (x<total_cells_horiz);x++)
            {
                ypos=0;
                for(y=vert_page_start;y<(vert_page_start+vert_page_size) &&
                        (y<total_cells_vert);y++)
                {
                    sprintf(buf,"%3.4f",PHDATA::ROOT->matrix->get(x,y));
                    device->text(0,buf,xpos*cell_width,ypos*cell_height-cell_offset,
                                 0.0,-1,0,0);
                    ypos++;
                }
                // display horizontal speciesnames :
                device->text(0,PHDATA::ROOT->hash_elements[x]->name,
                             xpos*cell_width,cell_height-off_dy-cell_offset,0.0,-1,0,0);
                xpos++;
            }
            device->shift_dx(-off_dx);
            // display vertical speciesnames
            ypos=0;
            for(y=vert_page_start;y<vert_page_start+vert_page_size;y++)
            {
                device->text(0,PHDATA::ROOT->hash_elements[y]->name,
                             0,ypos*cell_height-cell_offset,0.0,-1,0,0);
                ypos++;
            }
            device->shift_dy(-off_dy);
            break;




        case filter_dpy: {
            device->shift_dy(off_dy);
            device->shift_dx(off_dx);
            ypos=0;
            for(y=vert_page_start;y<(vert_page_start+vert_page_size) &&
                    (y<total_cells_vert);y++)
            {
                device->text(0,PHDATA::ROOT->hash_elements[y]->name,-off_dx,
                             ypos*cell_height-cell_offset,
                             0.0,-1,0,0);                          // species names

                device->text(0,GB_read_char_pntr(
                                                 PHDATA::ROOT->hash_elements[y]->gb_species_data_ptr)+horiz_page_start,
                             0,ypos*cell_height-cell_offset,
                             0.0,-1,0,0);                          // alignment
                ypos++;
            }
            xpos=0;
            cbuf[0]='\0'; cbuf[1]='\0';
            const AW_font_information *aw_fi=device->get_font_information(0,0);
            minhom = main_win->get_root()->awar("phyl/filter/minhom")->read_int();
            maxhom = main_win->get_root()->awar("phyl/filter/maxhom")->read_int();
            startcol = main_win->get_root()->awar("phyl/filter/startcol")->read_int();
            stopcol = main_win->get_root()->awar("phyl/filter/stopcol")->read_int();
            for (x = horiz_page_start; x < horiz_page_start + horiz_page_size; x++) {
                int             gc = 1;
                float       ml = markerline[x];
                if (x < startcol || x>stopcol){
                    gc = 2;
                }
                if (markerline[x] >= 0.0) {
                    if (    ml < minhom ||
                            ml > maxhom){
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
                    device->text(gc, cbuf, xpos * cell_width + 1,
                                 vert_page_size * cell_height +
                                 y * aw_fi->max_letter.height,
                                 0.0, -1, 0, 0);
                }
                xpos++;
            }
            device->shift_dy(-off_dy);
            device->shift_dx(-off_dx);
            break;
        }

        default:
            printf("\ndisplay: unknown display type (maybe not implemented yet)\n");
    }  // switch
    //  display_status(0,(AW_CL) main_win->get_root(),0);
}


void AP_display::print(void)
{
    printf("\nContents of class AP_display:\n");
    printf("display_what: %d\n",display_what);
    printf("screen_width:          %f  screen_height:        %f\n",screen_width,screen_height);
    printf("cell_width:            %ld  cell_height:          %ld\n",cell_width,cell_height);
    printf("cell_offset:           %ld\n",cell_offset);
    printf("horiz_page_size:       %ld  vert_page_size:       %ld\n",horiz_page_size,vert_page_size);
    printf("horiz_page_start:      %ld  vert_page_start:      %ld\n",horiz_page_start,vert_page_start);
    printf("off_dx:                %ld  off_dy:               %ld\n",off_dx,off_dy);
    printf("horiz_last_view_start: %ld  vert_last_view_start: %ld\n" ,horiz_last_view_start,vert_last_view_start);
}


void AP_display::set_scrollbar_steps(AW_window *aww,long width_h,long width_v,long page_h,long page_v)
{
    char buffer[200];

    sprintf(buffer,"window/%s/scroll_width_horizontal",aww->window_defaults_name);
    aww->get_root()->awar(buffer)->write_int(width_h);
    sprintf(buffer,"window/%s/scroll_width_vertical",aww->window_defaults_name);
    aww->get_root()->awar(buffer)->write_int(width_v);
    sprintf( buffer,"window/%s/horizontal_page_increment",aww->window_defaults_name);
    aww->get_root()->awar(buffer)->write_int(page_h);
    sprintf(buffer,"window/%s/vertical_page_increment",aww->window_defaults_name);
    aww->get_root()->awar(buffer)->write_int(page_v);
}


void AP_display::monitor_vertical_scroll_cb(AW_window *aww)    // draw area
{
    long diff;

    if(!device) return;
    if(vert_last_view_start==aww->slider_pos_vertical) return;
    diff=(aww->slider_pos_vertical-vert_last_view_start)/cell_height;
    // fast scroll: be careful: no transformation in move_region
    if(diff==1) // scroll one position up (== \/ arrow pressed)
    {
        device->move_region(0,off_dy,screen_width,vert_page_size*cell_height,0,off_dy-cell_height);

        // device->line(0,0,off_dy+1,100,off_dy+1,-1,0,0);  // source top
        // device->line(0,0,(vert_page_size-1)*cell_height+off_dy+1,100,
        //  (vert_page_size-1)*cell_height+off_dy+1,-1,0,0);  // source bottom
        // device->line(0,0,off_dy-cell_height,50,off_dy-cell_height,-1,0,0); // target top

        device->clear_part(0,off_dy-cell_height+(vert_page_size-1)*cell_height+1,
                           screen_width,cell_height);

        // device->line(0,6,off_dy-cell_height+(vert_page_size-1)*cell_height+1,
        // 6,off_dy-cell_height+(vert_page_size)*cell_height,-1,0,0);

        device->push_clip_scale();
        device->set_top_clip_border((int)(off_dy+(vert_page_size-2)*cell_height));
    }
    else if(diff==-1) // scroll one position down (== /\ arrow pressed)
    {
        device->move_region(0,off_dy-cell_height,screen_width,(vert_page_size-1)*cell_height+1,0,
                            off_dy);
        // device->line(0,0,off_dy-cell_height,50,off_dy-cell_height,-1,0,0);
        // device->line(0,0,(vert_page_size-1)*cell_height+1+(off_dy-cell_height),
        //                50,(vert_page_size-1)*cell_height+1+(off_dy-cell_height),-1,0,0);
        device->clear_part(0,off_dy-cell_height,screen_width,cell_height);
        // device->line(0,0,off_dy-cell_height,50,off_dy-cell_height,-1,0,0);
        // device->line(0,0,off_dy,50,off_dy,-1,0,0);
        device->push_clip_scale();
        device->set_bottom_clip_border((int)off_dy);
    }
    else  device->clear();

    vert_last_view_start=aww->slider_pos_vertical;
    vert_page_start=aww->slider_pos_vertical/cell_height;
    display();
    if((diff==1) || (diff==-1))  device->pop_clip_scale();
}

void AP_display::monitor_horizontal_scroll_cb(AW_window *aww)  // draw area
{
    long diff;

    if(!device) return;
    if( horiz_last_view_start==aww->slider_pos_horizontal) return;
    diff=(aww->slider_pos_horizontal- horiz_last_view_start)/cell_width;
    // fast scroll
    if(diff==1)   // scroll one position left ( > arrow pressed)
    {
        device->move_region(off_dx+cell_width,0,
                            horiz_page_size*cell_width,screen_height,
                            off_dx,0);

        // device->line(0,off_dx+cell_width,0,off_dx+cell_width,screen_height,-1,0,0);   // left border
        // device->line(0,horiz_page_size*cell_width+off_dx+cell_width,0,
        //            horiz_page_size*cell_width+off_dx+cell_width,screen_height,-1,0,0); // right border
        // device->line(0,off_dx,0,off_dx,screen_height,-1,0,0);  // target

        device->clear_part(off_dx+(horiz_page_size-1)*cell_width,0,cell_width,screen_height);

        // device->line(0,off_dx+(horiz_page_size-1)*cell_width,0,
        //             off_dx+(horiz_page_size-1)*cell_width,screen_height,-1,0,0);
        // device->line(0,off_dx+(horiz_page_size-1)*cell_width+cell_width,0,
        //              off_dx+(horiz_page_size-1)*cell_width+cell_width,screen_height,-1,0,0);

        device->push_clip_scale();
        device->set_left_clip_border((int)((horiz_page_size-1)*cell_width));
    }
    else if(diff==-1) // scroll one position right ( < arrow pressed)
    {
        device->move_region(off_dx,0,(horiz_page_size-1)*cell_width,screen_height,off_dx+cell_width,
                            0);
        device->clear_part(off_dx,0,cell_width,screen_height);
        device->push_clip_scale();
        device->set_right_clip_border((int)(off_dx+cell_width));
    }
    else device->clear();

    horiz_last_view_start=aww->slider_pos_horizontal;
    horiz_page_start=aww->slider_pos_horizontal/cell_width;
    display();
    if((diff==1) || (diff==-1))  device->pop_clip_scale();
}

AP_display_status::AP_display_status(AW_device *awd)
{
    AW_rectangle rect;
    device=awd;

    if(!device) return;
    const AW_font_information *aw_fi=device->get_font_information(0,0);
    font_width=aw_fi->max_letter.width;
    font_height=aw_fi->max_letter.height;
    device->reset();
    device->get_area_size(&rect);
    device->set_foreground_color(0,AW_WINDOW_FG);
    max_x=(rect.r-rect.l)/font_width;
    max_y=(rect.b-rect.t)/font_height;
    x_pos=0.0;
    y_pos=0.0;
    tab_pos=x_pos;
}

void AP_display_status::write(const char *text)
{
    device->text(0,text,x_pos*font_width,y_pos*font_height,0.0,-1,0,0);
    x_pos+=strlen(text);
}

void AP_display_status::write(long numl)
{
    char buf[20];

    sprintf(buf,"%ld",numl);
    write(buf);
}

void AP_display_status::write(AW_pos numA)
{
    char buf[20];

    sprintf(buf,"%3.3G",numA);
    write(buf);
}

void AP_display_status::clear(void){
    device->clear();
}

void display_status(AW_window *dummy,AW_CL cl_awroot,AW_CL cd2)    // bottom area
{
    AWUSE(dummy); AWUSE(cd2);
    AW_root *aw_root = (AW_root *) cl_awroot;

    if(!AP_display::apdisplay) return;
    if(!PH_used_windows::windowList) return;

    {
        static AP_display_status apds(PH_used_windows::windowList->phylo_main_window->get_device (AW_BOTTOM_AREA));
        apds.clear();
        switch(AP_display::apdisplay->displayed())
        {
            case NONE: return;
            case filter_dpy:
            case species_dpy: apds.set_origin();
                apds.set_cursor((apds.get_size('x')/2)-10,0);
                apds.write("STATUS REPORT FILTER");
                apds.newline();
                apds.newline();
                apds.write("start at column:     ");
                apds.write((long)aw_root->awar("phyl/filter/startcol")->read_int());
                apds.move_x(40);
                apds.set_tab();
                apds.write("stop at column:      ");
                apds.write((long)aw_root->awar("phyl/filter/stopcol")->read_int());
                apds.newline();
                apds.write("minimal similarity:  ");
                apds.write((long)aw_root->awar("phyl/filter/minhom")->read_int());
                apds.set_cursor_x(apds.get_tab());
                apds.write("maximal similarity:  ");
                apds.write((long)aw_root->awar("phyl/filter/maxhom")->read_int());
                apds.newline();
                apds.write("'.' in column:     ");
                apds.write(filter_text[aw_root->awar("phyl/filter/point")->read_int()]);
                apds.newline();
                apds.write("'-' in column:     ");
                apds.write(filter_text[aw_root->awar("phyl/filter/minus")->read_int()]);
                apds.newline();
                apds.write("'rest' in column:  ");
                apds.write(filter_text[aw_root->awar("phyl/filter/rest")->read_int()]);
                apds.newline();
                apds.write("'acgtu' in column: ");
                apds.write(filter_text[aw_root->awar("phyl/filter/lower")->read_int()]);
                break;
            case matrix_dpy: apds.set_origin();
                apds.set_cursor((apds.get_size('x')/2)-10,0);
                apds.write("STATUS REPORT MATRIX");
                break;
            default: printf("\nstatus: unknown display type (maybe not implemented yet)\n");
        }
    }
}

