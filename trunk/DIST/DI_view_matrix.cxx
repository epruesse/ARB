#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>

#include <awt_tree.hxx>
#include "dist.hxx"
#include <BI_helix.hxx>

#include <di_matr.hxx>
#include <di_view_matrix.hxx>

#define AWAR_DIST_SHOW_PREFIX   AWAR_DIST_PREFIX "show/"
#define AWAR_DIST_SHOW_MIN_DIST AWAR_DIST_SHOW_PREFIX "min_dist"
#define AWAR_DIST_SHOW_MAX_DIST AWAR_DIST_SHOW_PREFIX "max_dist"

void vertical_change_cb(AW_window *aww,PH_dmatrix *dis)
{
    dis->monitor_vertical_scroll_cb(aww);
}

void horizontal_change_cb(AW_window *aww,PH_dmatrix *dis)
{
    dis->monitor_horizontal_scroll_cb(aww);
}

void redisplay_needed(AW_window *,PH_dmatrix *dis)
{
    dis->display();
}

void resize_needed(AW_window *,PH_dmatrix *dis)
{
    dis->init();
    dis->resized();
    dis->display();
}

PH_dmatrix::PH_dmatrix()
{
    memset((char *) this,0,sizeof(PH_dmatrix));
}


void PH_dmatrix::init (PHMATRIX *matrix)
{
    ph_matrix = matrix;
    PHMATRIX *m = get_matrix();

    // calculate cell width and height
    {
        int max_cell_width = 0;
        int max_cell_height = 0;

        PH_gc gc;
        for (gc=PH_G_STANDARD; gc<=PH_G_LAST; gc = PH_gc(int(gc)+1)) {
            int height = 0;
            int width = 0;

            switch (gc) {
                case PH_G_STANDARD:
                case PH_G_BELOW_DIST:
                case PH_G_ABOVE_DIST: {
                    const AW_font_information *aw_fi = device->get_font_information(PH_G_STANDARD, 0);

                    width  = aw_fi->max_letter.width*6; // normal cell contain 6 characters (e.g.: '0.0162')
                    height = aw_fi->max_letter.height*2;
                    break;
                }
                case PH_G_NAMES: {
                    const AW_font_information *aw_fi = device->get_font_information(PH_G_STANDARD, 0);

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

    //     cell_width = aw_fi->max_letter_width*SPECIES_NAME_LEN;
    //     cell_height = aw_fi->max_letter_height*2;

    cell_offset = 10;  // draw cell_offset pixels above cell base_line

    off_dx = cell_width + 2*cell_offset;
    off_dy = 3*cell_height;

    if (m){
        total_cells_horiz=m->nentries;
        total_cells_vert=m->nentries;
    }
    set_scrollbar_steps( cell_width,cell_height,50,50);
    resized();  // initalize window_size dependend parameters
}

PHMATRIX *PH_dmatrix::get_matrix(){
    if (ph_matrix) return ph_matrix;
    return PHMATRIX::ROOT;
}

void PH_dmatrix::resized(void)
{
    AW_rectangle               squ;
    AW_rectangle               rect;
    long                       horiz_paint_size,vert_paint_size;
    const AW_font_information *aw_fi = device->get_font_information(PH_G_STANDARD,0);
    PHMATRIX                  *m     = get_matrix();
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
    device->clear();
    device->set_right_clip_border((int)(off_dx+cell_width*horiz_page_size));
    device->reset();            // reset shift_x and shift_y
    awm->set_vertical_scrollbar_position(0);
    awm->set_horizontal_scrollbar_position(0);
    awm->tell_scrolled_picture_size(rect);
    awm->calculate_scrollbars();
    if (!awm->get_show() && m) {
        awm->show();
    }
}


void PH_dmatrix::display(void)   // draw area
{
#define BUFLEN 200
    char            buf[BUFLEN];
    long            x, y, xpos, ypos;
    GB_transaction  dummy(gb_main);
    if (!device)    return;
    PHMATRIX       *m = get_matrix();
    if (!m) {
        if (awm) awm->hide();
        return;
    }
    device->shift_dx(off_dx);
    device->shift_dy(off_dy);
    xpos = 0;

    int name_display_width; {
        const AW_font_information *aw_fi = device->get_font_information(PH_G_NAMES,0);
        name_display_width = cell_width/aw_fi->max_letter.width;
    }
    gb_assert(name_display_width<BUFLEN);

    for (x = horiz_page_start;
         x < (horiz_page_start + horiz_page_size) && (x < total_cells_horiz);
         x++)
    {
        ypos = 0;
        for (y = vert_page_start;
             y < (vert_page_start + vert_page_size) && (y < total_cells_vert);
             y++)
        {
            double val2 = m->matrix->get(x, y);

            if (val2>=min_view_dist && val2<=max_view_dist && val2>0.0) { // display ruler
                int maxw = (int)(cell_width * .75);
                int h = cell_height /2 ;
                int y2 = ypos * cell_height - cell_offset - 10;
                int x2 = xpos * cell_width;
                double len = ((val2-min_view_dist)/(max_view_dist-min_view_dist)) * maxw;
                if (len >= 0) {
                    device->box(PH_G_RULER_DISPLAY, x2, y2,len, h*.8,-1,0,0);
                }else{
                    device->text(PH_G_STANDARD, "????", xpos * cell_width, ypos * cell_height - cell_offset, 0.0, -1, 0, 0);
                }
                double v;
                for (v = x2; v < x2 + maxw; v += maxw * .1999) {
                    device->line (PH_G_RULER, v, y2+h*.5, v, y2 + h, -1,0,0);
                }
                // device->line ( 0,x2, y2, x2+maxw-1, y2, -1,0,0);
                device->line (PH_G_RULER, x2, y2+h, x2+maxw-1, y2+h, -1,0,0);
            }
            else {
                PH_gc gc = val2<min_view_dist ? PH_G_BELOW_DIST : (val2>max_view_dist ? PH_G_ABOVE_DIST : PH_G_STANDARD);

                if (val2 == 0.0 || val2 == 1.0) {
                    sprintf(buf, "%3.1f", val2);
                    gc = PH_G_STANDARD;
                }
                else {
                    sprintf(buf, "%3.4f", val2);
                }
                device->text(gc, buf, xpos * cell_width, ypos * cell_height - cell_offset, 0.0, -1, 0, 0);
            }

            ypos++;
        }
        //display horizontal speciesnames:

        strcpy(buf, m->entries[x]->name);
        buf[name_display_width] = 0; // cut group-names if too long
        device->text(PH_G_NAMES, buf, xpos * cell_width, cell_height - off_dy - cell_offset, 0.0, -1, 0, 0);
        xpos++;
    }
    device->shift_dx(-off_dx);
    //display vertical speciesnames
    ypos = 0;
    for (y = vert_page_start; y < vert_page_start + vert_page_size; y++) {
        strcpy(buf, m->entries[y]->name);
        buf[name_display_width] = 0; // cut group-names if too long
        device->text(PH_G_NAMES, buf, 0, ypos * cell_height - cell_offset, 0.0, -1, 0, 0);
        ypos++;
    }
    device->shift_dy(-off_dy);
#undef BUFLEN
}

void PH_dmatrix::set_scrollbar_steps(long width_h,long width_v,long page_h,long page_v)
{ char buffer[200];

 sprintf(buffer,"window/%s/scroll_width_horizontal",awm->window_defaults_name);
 awm->get_root()->awar(buffer)->write_int(width_h);
 sprintf(buffer,"window/%s/scroll_width_vertical",awm->window_defaults_name);
 awm->get_root()->awar(buffer)->write_int(width_v);
 sprintf( buffer,"window/%s/horizontal_page_increment",awm->window_defaults_name);
 awm->get_root()->awar(buffer)->write_int(page_h);
 sprintf(buffer,"window/%s/vertical_page_increment",awm->window_defaults_name);
 awm->get_root()->awar(buffer)->write_int(page_v);
}


void PH_dmatrix::monitor_vertical_scroll_cb(AW_window *aww)    // draw area
{ long diff;

 if(!device) return;
 if(vert_last_view_start==aww->slider_pos_vertical) return;
 diff=(aww->slider_pos_vertical-vert_last_view_start)/cell_height;
 // fast scroll: be careful: no transformation in move_region
 if(diff==1){ // scroll one position up (== \/ arrow pressed)

     device->move_region(0,off_dy,
                         screen_width,vert_page_size*cell_height,0,off_dy-cell_height);
     device->clear_part(0,off_dy-cell_height+(vert_page_size-1)*cell_height+1,
                        screen_width,cell_height);

     device->push_clip_scale();
     device->set_top_clip_border((int)(off_dy+(vert_page_size-2)*cell_height));
 }else if(diff==-1){ // scroll one position down (== /\ arrow pressed)
     device->move_region(0,off_dy-cell_height,screen_width,
                         (vert_page_size-1)*cell_height+1,0,off_dy);
     device->clear_part(0,off_dy-cell_height,screen_width,cell_height);
     device->push_clip_scale();
     device->set_bottom_clip_border((int)off_dy);
 }else  device->clear();

 vert_last_view_start=aww->slider_pos_vertical;
 vert_page_start=aww->slider_pos_vertical/cell_height;
 display();
 if((diff==1) || (diff==-1))  device->pop_clip_scale();
}

void PH_dmatrix::monitor_horizontal_scroll_cb(AW_window *aww)  // draw area
{ long diff;

 if(!device) return;
 if( horiz_last_view_start==aww->slider_pos_horizontal) return;
 diff=(aww->slider_pos_horizontal- horiz_last_view_start)/cell_width;
 // fast scroll
 if(diff==1)   // scroll one position left ( > arrow pressed)
 { device->move_region(off_dx+cell_width,0,
                       horiz_page_size*cell_width,screen_height,
                       off_dx,0);

 device->clear_part(off_dx+(horiz_page_size-1)*cell_width,0,cell_width,screen_height);


 device->push_clip_scale();
 device->set_left_clip_border((int)((horiz_page_size-1)*cell_width));
 }
 else if(diff==-1) // scroll one position right ( < arrow pressed)
 { device->move_region(off_dx,0,(horiz_page_size-1)*cell_width,screen_height,off_dx+cell_width,
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

static int update_display_on_dist_change = 1;

void ph_view_set_max_d(AW_window *aww, AW_CL cl_max_d, AW_CL /*clmatr*/){
    AWUSE(aww);
    //PH_dmatrix *dmatrix = (PH_dmatrix *)clmatr;
    double max_d = cl_max_d*0.01;

    AW_root *aw_root = aww->get_root();

    update_display_on_dist_change = 0;
    aw_root->awar(AWAR_DIST_SHOW_MIN_DIST)->write_float(0.0);
    update_display_on_dist_change = 1;
    aw_root->awar(AWAR_DIST_SHOW_MAX_DIST)->write_float(max_d);
}

void ph_view_set_distances(AW_root *awr, AW_CL cl_setmax, AW_CL cl_dmatrix) {
    PH_dmatrix *dmatrix = (PH_dmatrix *)cl_dmatrix;
    double max_dist = awr->awar(AWAR_DIST_SHOW_MAX_DIST)->read_float();
    double min_dist = awr->awar(AWAR_DIST_SHOW_MIN_DIST)->read_float();
    int old = update_display_on_dist_change;

    update_display_on_dist_change = 0;
    if (cl_setmax) { // !=0 -> set max and fix min
        dmatrix->set_slider_max(max_dist);
        if (min_dist>max_dist) awr->awar(AWAR_DIST_SHOW_MIN_DIST)->write_float(max_dist);
    }
    else { // ==0 -> set min and fix max
        dmatrix->set_slider_min(min_dist);
        if (min_dist>max_dist) awr->awar(AWAR_DIST_SHOW_MAX_DIST)->write_float(min_dist);
    }
    update_display_on_dist_change = old;
    if (update_display_on_dist_change) {
        dmatrix->resized();
        dmatrix->display();
    }
}

void ph_change_dist(AW_window *aww, AW_CL cl_mode) {
    AW_root *awr = aww->get_root();
    const char *awar_name;

    gb_assert(cl_mode>=0 && cl_mode<=3);

    if (cl_mode<2) { // change min
        awar_name = AWAR_DIST_SHOW_MIN_DIST;
    }
    else { // change max
        awar_name = AWAR_DIST_SHOW_MAX_DIST;
    }

    double dist = awr->awar(awar_name)->read_float();
    double increment = 0.01;

    if (cl_mode%2) increment = -increment; // decrement value
    dist += increment;
    if (!(dist<0)) awr->awar(awar_name)->write_float(dist);
}

void ph_view_create_awars(AW_root *aw_root, PH_dmatrix *dmatrix) {
    aw_root->awar_float( AWAR_DIST_SHOW_MIN_DIST, 0.0)->add_callback(ph_view_set_distances, (AW_CL)0, (AW_CL)dmatrix);
    aw_root->awar_float( AWAR_DIST_SHOW_MAX_DIST, 0.0)->add_callback(ph_view_set_distances, (AW_CL)1, (AW_CL)dmatrix);
}

AW_window *PH_create_view_matrix_window(AW_root *awr, PH_dmatrix *dmatrix){
    ph_view_create_awars(awr, dmatrix);
    AW_window_menu *awm = new AW_window_menu();
    awm->init(awr,"SHOW_MATRIX", "ARB_SHOW_MATRIX", 800,600);

    dmatrix->device = awm->get_device(AW_MIDDLE_AREA);
    dmatrix->awm = awm;
    //    dmatrix->display();
    awm->set_vertical_change_callback((AW_CB2)vertical_change_cb,(AW_CL)dmatrix,0);
    awm->set_horizontal_change_callback((AW_CB2)horizontal_change_cb,(AW_CL)dmatrix,0);
    awm->set_resize_callback(AW_MIDDLE_AREA,(AW_CB2)resize_needed,(AW_CL)dmatrix,0);
    awm->set_expose_callback(AW_MIDDLE_AREA,(AW_CB2)redisplay_needed,(AW_CL)dmatrix,0);

    AW_gc_manager preset_window =
        AW_manage_GC (awm,dmatrix->device, PH_G_STANDARD, PH_G_LAST, AW_GCM_DATA_AREA,
                      (AW_CB)resize_needed,(AW_CL)dmatrix,0,
                      false,
                      "#D0D0D0",
                      "#Standard$#000000",
                      "#Names$#000000",
                      "+-Ruler$#555", "-Display$#00AA55",
                      "#BelowDist$#008732",
                      "#AboveDist$#DB008D",
                      0);

    awm->create_menu(0,"File","F");
    awm->insert_menu_topic("save_matrix",   "Save Matrix to File",  "S","save_matrix.hlp",  AWM_ALL, AW_POPUP, (AW_CL)create_save_matrix_window, (AW_CL)AWAR_DIST_SAVE_MATRIX_BASE);
    awm->insert_menu_topic("close",     "Close",    "C",0,  AWM_ALL,    (AW_CB)AW_POPDOWN, (AW_CL)0, 0 );

    awm->create_menu(0,"Props","P");
    awm->insert_menu_topic("props_matrix",      "Matrix: Colors and Fonts ...", "C","neprops_data.hlp"  ,   AWM_ALL,    AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)preset_window );
    awm->insert_menu_topic("show_dist_as_ascii",    "Show Dist in Ascii",           "A",0       ,   AWM_ALL,    ph_view_set_max_d, 0, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_010",     "Show Dist [0,0.1]",            "1",0       ,   AWM_ALL,    ph_view_set_max_d, 10, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_025",     "Show Dist [0,0.25]",           "3",0       ,   AWM_ALL,    ph_view_set_max_d, 25, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_050",     "Show Dist [0,0.5]",            "5",0       ,   AWM_ALL,    ph_view_set_max_d, 50, (AW_CL)dmatrix );
    awm->insert_menu_topic("show_dist_100",     "Show Dist [0,1.0]",            "0",0       ,   AWM_ALL,    ph_view_set_max_d, 100, (AW_CL)dmatrix );

    int x, y;
    awm->get_at_position(&x, &y);
    awm->button_length(3);

#define FIELD_XSIZE 160
#define BUTTON_XSIZE 25

    awm->label("Dist min:");
    awm->create_input_field(AWAR_DIST_SHOW_MIN_DIST, 7);
    x += FIELD_XSIZE;

    awm->at_x(x);
    awm->callback(ph_change_dist, 0);
    awm->create_button("PLUS_MIN", "+");
    x += BUTTON_XSIZE;

    awm->at_x(x);
    awm->callback(ph_change_dist, 1);
    awm->create_button("MINUS_MIN", "-");
    x += BUTTON_XSIZE;

    awm->at_x(x);
    awm->label("Dist max:");
    awm->create_input_field(AWAR_DIST_SHOW_MAX_DIST, 7);
    x += FIELD_XSIZE;

    awm->at_x(x);
    awm->callback(ph_change_dist, 2);
    awm->create_button("PLUS_MAX", "+");
    x += BUTTON_XSIZE;

    awm->at_x(x);
    awm->callback(ph_change_dist, 3);
    awm->create_button("MINUS_MAX", "-");
    x += BUTTON_XSIZE;

    awm->set_info_area_height(40);

    dmatrix->init(dmatrix->ph_matrix);

    return (AW_window *)awm;
}
