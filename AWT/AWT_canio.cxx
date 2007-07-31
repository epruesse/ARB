#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include "awt_canvas.hxx"
#include "awt.hxx"

// --------------------------------------------------------------------------------

static void awt_print_tree_check_size(void *, AW_CL cl_ntw) {
    AWT_canvas     *ntw  = (AWT_canvas*)cl_ntw;
    GB_transaction  dummy2(ntw->gb_main);
    AW_world        size;
    long            what = ntw->aww->get_root()->awar(AWAR_PRINT_TREE_CLIP)->read_int();

    AW_device *size_device = ntw->aww->get_size_device(AW_MIDDLE_AREA);

    if (what){
        size_device->reset();
        size_device->zoom(ntw->trans_to_fit);
        size_device->set_filter(AW_SCREEN);
        ntw->tree_disp->show(size_device);
        size_device->get_size_information(&size);
    }else{
        size_device->get_area_size(&size);
    }

    ntw->aww->get_root()->awar(AWAR_PRINT_TREE_GSIZEX)->write_float((size.r-size.l + 30)/80);
    ntw->aww->get_root()->awar(AWAR_PRINT_TREE_GSIZEY)->write_float((size.b-size.t + 30)/80);
}

inline int xy2pages(double sx, double sy) {
    return (int(sx+0.99)*int(sy+0.99));
}

void awt_page_size_check_cb(AW_root *awr) {
    bool   landscape = awr->awar(AWAR_PRINT_TREE_LANDSCAPE)->read_int();
    double px        = awr->awar(AWAR_PRINT_TREE_PSIZEX)->read_float();
    double py        = awr->awar(AWAR_PRINT_TREE_PSIZEY)->read_float();

    if (landscape != (px>py)) {
        awr->awar(AWAR_PRINT_TREE_PSIZEX)->write_float(py);   // recalls this function
        awr->awar(AWAR_PRINT_TREE_PSIZEY)->write_float(px);
        return;
    }

    long   magnification = awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->read_int();
    double gx            = awr->awar(AWAR_PRINT_TREE_GSIZEX)->read_float();
    double gy            = awr->awar(AWAR_PRINT_TREE_GSIZEY)->read_float();

    double ox = (gx*magnification)/100; // resulting size of output in inches
    double oy = (gy*magnification)/100;

    double sx = 0;              // resulting pages
    double sy = 0;

    bool useOverlap = awr->awar(AWAR_PRINT_TREE_OVERLAP)->read_int();
    if (useOverlap) {
        double overlapAmount = awr->awar(AWAR_PRINT_TREE_OVERLAP_AMOUNT)->read_float();

        if (overlapAmount >= px || overlapAmount >= py) {
            aw_message("Overlap amount bigger than paper size. Please fix!");
        }
        else {
            while (ox>px) { ox  = ox-px+overlapAmount; sx += 1.0; }
            while (oy>py) { oy = oy-py+overlapAmount; sy += 1.0; }
            sx += ox/px;
            sy += oy/py;
        }
    }
    else {
        sx = ox/px;
        sy = oy/py;
    }

    awr->awar(AWAR_PRINT_TREE_SIZEX)->write_float(sx);
    awr->awar(AWAR_PRINT_TREE_SIZEY)->write_float(sy);

    awr->awar(AWAR_PRINT_TREE_PAGES)->write_int(xy2pages(sx, sy));
}

// --------------------------------------------------------------------------------

static bool export_awars_created = false;
static bool print_awars_created = false;

static void create_export_awars(AW_root *awr) {
    if (!export_awars_created) {
        AW_default def = AW_ROOT_DEFAULT;
    
        awr->awar_int(AWAR_PRINT_TREE_CLIP, 0, def);
        awr->awar_int(AWAR_PRINT_TREE_HANDLES, 1, def);
        awr->awar_int(AWAR_PRINT_TREE_COLOR, 1, def);

        awr->awar_string(AWAR_PRINT_TREE_FILE_NAME, "print.fig", def);
        awr->awar_string(AWAR_PRINT_TREE_FILE_DIR, "", def);
        awr->awar_string(AWAR_PRINT_TREE_FILE_FILTER, "fig", def);

        awr->awar_int(AWAR_PRINT_TREE_LANDSCAPE, 0, def);
        awr->awar_int(AWAR_PRINT_TREE_MAGNIFICATION, 100, def);

        // constraints: 
        
        awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->set_minmax(1, 10000);
        
        export_awars_created = true;
    }
}

static void resetFiletype(AW_root *awr, const char *filter, const char *defaultFilename) {
    AW_awar *awar_filter    = awr->awar(AWAR_PRINT_TREE_FILE_FILTER);
    char    *current_filter = awar_filter->read_string();

    if (strcmp(current_filter, filter) != 0) {
        awar_filter->write_string(filter);
        awr->awar(AWAR_PRINT_TREE_FILE_NAME)->write_string(defaultFilename);
    }
    free(current_filter);
}

static void create_print_awars(AW_root *awr, AWT_canvas *ntw) {
    create_export_awars(awr);

    if (!print_awars_created) {
        AW_default def = AW_ROOT_DEFAULT;

        awr->awar_int(AWAR_PRINT_TREE_PAGES, 1, def);
        awr->awar_int(AWAR_PRINT_TREE_OVERLAP, 1, def);
        awr->awar_float(AWAR_PRINT_TREE_OVERLAP_AMOUNT, 0.82, def);

        awr->awar_float(AWAR_PRINT_TREE_GSIZEX);
        awr->awar_float(AWAR_PRINT_TREE_GSIZEY);
        
        // default paper size (A4 =  8.27*11.69)
        // using 'preview' determined the following values (fitting 1 page!):
        awr->awar_float(AWAR_PRINT_TREE_PSIZEX, 7.5);
        awr->awar_float(AWAR_PRINT_TREE_PSIZEY, 10.5);

        awr->awar_float(AWAR_PRINT_TREE_SIZEX);
        awr->awar_float(AWAR_PRINT_TREE_SIZEY);

        awr->awar_int(AWAR_PRINT_TREE_DEST);
    
        {
            char *print_command;
            if (getenv("PRINTER")){
                print_command = GBS_eval_env("lpr -h -P$(PRINTER)");
            }else   print_command = strdup("lpr -h");

            awr->awar_string(AWAR_PRINT_TREE_PRINTER, print_command, def);
            free(print_command);
        }

        // contraints and automatica: 
    
        awr->awar(AWAR_PRINT_TREE_PSIZEX)->set_minmax(0.1, 100);
        awr->awar(AWAR_PRINT_TREE_PSIZEY)->set_minmax(0.1, 100);

        awt_print_tree_check_size(0, (AW_CL)ntw);
    
        awr->awar(AWAR_PRINT_TREE_CLIP)->add_callback((AW_RCB1)awt_print_tree_check_size, (AW_CL)ntw);

        { // add callbacks for page recalculation
            const char *checked_awars[] = {
                AWAR_PRINT_TREE_MAGNIFICATION, AWAR_PRINT_TREE_LANDSCAPE,
                AWAR_PRINT_TREE_OVERLAP,       AWAR_PRINT_TREE_OVERLAP_AMOUNT,
                AWAR_PRINT_TREE_PSIZEX,        AWAR_PRINT_TREE_PSIZEY,
                AWAR_PRINT_TREE_GSIZEX,        AWAR_PRINT_TREE_GSIZEY,
                0
            };
            for (int ca = 0; checked_awars[ca]; ca++) {
                awr->awar(checked_awars[ca])->add_callback(awt_page_size_check_cb);
            }
        }

        awt_page_size_check_cb(awr);
        
        print_awars_created = true;
    }
}

// --------------------------------------------------------------------------------

const char *AWT_print_tree_to_file(AW_window *aww, AWT_canvas * ntw)
{
    // export to xfig

    GB_transaction  dummy(ntw->gb_main);
    AW_root        *awr  = aww->get_root();
    //  char           *dest = awr->awar(AWAR_PRINT_TREE_FILE_NAME)->read_string();
    char           *dest = awt_get_selected_fullname(awr, AWAR_PRINT_TREE_FILE_BASE);
    if (!strlen(dest)) {
        delete(dest);
        sprintf(AW_ERROR_BUFFER, "Please enter a file name first");
        aw_message();
        return AW_ERROR_BUFFER;
    }

    long what      = awr->awar(AWAR_PRINT_TREE_CLIP)->read_int();
    long handles   = awr->awar(AWAR_PRINT_TREE_HANDLES)->read_int();
    int  use_color = awr->awar(AWAR_PRINT_TREE_COLOR)->read_int();

    char tmp[1024];
    const char *error;


    sprintf(tmp, "%s", dest);


    AW_device *device      = ntw->aww->get_print_device(AW_MIDDLE_AREA);
    AW_device *size_device = ntw->aww->get_size_device(AW_MIDDLE_AREA);

    device->reset();
    device->set_color_mode((use_color==1));
    error = device->open(tmp);
    device->line(0, 0, 0, 1, -1); // dummy point upper left corner

    if (what) {             // draw all
        AW_world size;
        size_device->reset();
        size_device->zoom(ntw->trans_to_fit);
        size_device->set_filter(AW_SCREEN);
        ntw->tree_disp->show(size_device);
        size_device->get_size_information(&size);
        size.l -= 50;
        size.t -= 40;           // expand pic
        size.r += 20;
        size.b += 20;
        device->shift_x(-size.l/ntw->trans_to_fit);
        device->shift_y(-size.t/ntw->trans_to_fit);
        device->set_bottom_clip_border((int)(size.b-size.t), AW_TRUE);
        device->set_right_clip_border((int)(size.r-size.l), AW_TRUE);
        //      device->set_bottom_font_overlap(AW_TRUE);
        //      device->set_right_font_overlap(AW_TRUE);
        device->zoom(ntw->trans_to_fit);
    }else{
        ntw->init_device(device);   // draw screen
    }
    if (!error) {
        if (handles) {
            device->set_filter(AW_PRINTER | AW_PRINTER_EXT);
        }else{
            device->set_filter(AW_PRINTER);
        }
        ntw->tree_disp->show(device);
        device->close();
        awr->awar(AWAR_PRINT_TREE_FILE_DIR)->touch();   // reload dir !!!
    }


    if (error) aw_message(error);
    else    aww->get_root()->awar(AWAR_PRINT_TREE_FILE_DIR)->touch();
    delete dest;
    return error;
}

void AWT_print_tree_to_file_xfig(AW_window *aww, AW_CL cl_ntw){
    AWT_canvas * ntw = (AWT_canvas*)cl_ntw;
    AW_root *awr = aww->get_root();
    const char *error = AWT_print_tree_to_file(aww, ntw);
    if (!error) {
        // char *dest = awr->awar(AWAR_PRINT_TREE_FILE_NAME)->read_string();
        char *dest = awt_get_selected_fullname(awr, AWAR_PRINT_TREE_FILE_BASE);
        char  com[1024];
        sprintf(com, "xfig %s &", dest);
        system(com);
        delete dest;
    }
}

void AWT_popup_tree_export_window(AW_window *parent_win, AW_CL cl_canvas, AW_CL) {
    static AW_window_simple *aws = 0;

    AW_root *awr = parent_win->get_root();
    create_export_awars(awr);
    resetFiletype(awr, "fig", "print.fig");

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "EXPORT_TREE_AS_XFIG", "EXPORT TREE TO XFIG");
        aws->load_xfig("awt/export.fig");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");aws->callback(AW_POPUP_HELP, (AW_CL)"tree2file.hlp");
        aws->create_button("HELP", "HELP", "H");

        aws->label_length(15);

        awt_create_selection_box((AW_window *)aws, AWAR_PRINT_TREE_FILE_BASE);

        aws->at("what");
        aws->label("Clip at Screen");
        aws->create_toggle_field(AWAR_PRINT_TREE_CLIP, 1);
        aws->insert_toggle("#print/clipscreen.bitmap", "S", 0);
        aws->insert_toggle("#print/clipall.bitmap", "A", 1);
        aws->update_toggle_field();

        aws->at("remove_root");
        aws->label("Show Handles");
        aws->create_toggle_field(AWAR_PRINT_TREE_HANDLES, 1);
        aws->insert_toggle("#print/nohandles.bitmap", "S", 0);
        aws->insert_toggle("#print/handles.bitmap", "A", 1);
        aws->update_toggle_field();

        aws->at("color");
        aws->label("Export colors");
        aws->create_toggle(AWAR_PRINT_TREE_COLOR);
    

        aws->at("xfig");aws->callback(AWT_print_tree_to_file_xfig, cl_canvas);
        aws->create_button("START_XFIG", "GO XFIG", "X");

        aws->at("cancel");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CANCEL", "C");
    }

    aws->show();
}
/*------------------------------------- to export secondary structure to XFIG ---------------------------------------------*/

// AW_window * AWT_create_sec_export_window(AW_root *awr, AWT_canvas *ntw){
void AWT_popup_sec_export_window(AW_window *parent_win, AW_CL cl_canvas, AW_CL) {
    static AW_window_simple *aws = 0;

    AW_root *awr = parent_win->get_root();
    create_export_awars(awr);
    resetFiletype(awr, "fig", "print.fig");

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "EXPORT_TREE_AS_XFIG", "EXPORT STRUCTURE TO XFIG");
        aws->load_xfig("awt/secExport.fig");

        aws->at("help");aws->callback(AW_POPUP_HELP, (AW_CL)"tree2file.hlp");
        aws->create_button("HELP", "HELP", "H");

        aws->label_length(15);
        awt_create_selection_box((AW_window *)aws, AWAR_PRINT_TREE_FILE_BASE);

        aws->at("what");
        aws->label("Clip Options");
        aws->create_option_menu(AWAR_PRINT_TREE_CLIP);
        aws->insert_option("Export screen only", "s", 0);
        aws->insert_default_option("Export complete structure", "c", 1);
        aws->update_option_menu();

        aws->at("color");
        aws->label("Export colors");
        aws->create_toggle(AWAR_PRINT_TREE_COLOR);

        aws->at("xfig"); aws->callback(AWT_print_tree_to_file_xfig, cl_canvas);
        aws->create_button("START_XFIG", "EXPORT to XFIG", "X");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("cancel");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CANCEL", "C");
    }
    
    aws->show();
}
/*------------------------------------------------------------------------------------------------------------------------------------------*/

void AWT_print_tree_to_printer(AW_window *aww, AW_CL cl_ntw)
{
    AWT_canvas *    ntw = (AWT_canvas*)cl_ntw;
    GB_transaction  dummy(ntw->gb_main);
    AW_root        *awr = aww->get_root();

    long print2file = awr->awar(AWAR_PRINT_TREE_DEST)->read_int();
    char *dest;
    GB_ERROR error = 0;


    bool   landscape     = awr->awar(AWAR_PRINT_TREE_LANDSCAPE)->read_int();
    bool   useOverlap    = awr->awar(AWAR_PRINT_TREE_OVERLAP)->read_int();
    double magnification = awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->read_int() * 0.01;
    long   what          = awr->awar(AWAR_PRINT_TREE_CLIP)->read_int();
    long   handles       = awr->awar(AWAR_PRINT_TREE_HANDLES)->read_int();
    int    use_color     = awr->awar(AWAR_PRINT_TREE_COLOR)->read_int();

    char sys[2400];
    char *xfig = GBS_eval_env("/tmp/arb_print_$(USER)_$(ARB_PID).xfig");

    switch(print2file) {
        case 1:
            // dest = awr->awar(AWAR_PRINT_TREE_FILE_NAME)->read_string();
            dest = awt_get_selected_fullname(awr, AWAR_PRINT_TREE_FILE_BASE);
            break;
        default:
            dest = GBS_eval_env("/tmp/arb_print_$(USER)_$(ARB_PID).ps");
            break;
    }
    {
        FILE *out;
        out = fopen(dest, "w");
        if (!out) {
            error = (char *)GB_export_error("Cannot open file '%s'", dest);
        }
        else {
            fclose(out);
            sprintf(sys,
                    "fig2dev -L ps -M %s -m %f %s %s %s",
                    (useOverlap ? "-O" : ""), 
                    magnification,
                    (landscape ? "-l 0" : "-p 0"),
                    xfig,
                    dest);

        }
    }

    AW_device *device= ntw->aww->get_print_device(AW_MIDDLE_AREA);

    aw_openstatus("Printing");

    device->reset();
    ntw->init_device(device);   // draw screen

    aw_status("Get Picture Size");
    device->reset();
    device->set_color_mode((use_color==1));
    error = device->open(xfig);
    device->line(0, 0, 0, 1, -1); // dummy point upper left corner
    if (what) {             // draw all
        AW_world size;
        AW_device *size_device = ntw->aww->get_size_device(AW_MIDDLE_AREA);
        size_device->reset();
        size_device->zoom(ntw->trans_to_fit);
        size_device->set_filter(AW_SCREEN);
        ntw->tree_disp->show(size_device);
        size_device->get_size_information(&size);
        size.l -= 50;
        size.t -= 40;       // expand pic
        size.r += 20;
        size.b += 20;
        device->shift_x(-size.l/ntw->trans_to_fit);
        device->shift_y(-size.t/ntw->trans_to_fit);
        device->set_bottom_clip_border((int)(size.b-size.t), AW_TRUE);
        device->set_right_clip_border((int)(size.r-size.l), AW_TRUE);
        device->zoom(ntw->trans_to_fit);
    }else{
        ntw->init_device(device);   // draw screen
    }

    aw_status("Exporting Data");

    if (!error) {
        if (handles) {
            device->set_filter(AW_PRINTER | AW_PRINTER_EXT);
        }else{
            device->set_filter(AW_PRINTER);
        }
        ntw->tree_disp->show(device);
        device->close();
        aw_status("Converting to Postscript");
#if defined(DEBUG)
        printf("convert command: '%s'\n", sys);
#endif // DEBUG
        if (system(sys)){
            error = GB_export_error("System error occured while running '%s'", sys);
        }
        if (GB_unlink(xfig)) {
            error = GB_get_error();
        }
    }

    aw_status("Printing");
    if (error) aw_message(error);
    else switch(print2file) {
        case 2:{
            GB_CSTR gs           = GB_getenvARB_GS();
            GB_CSTR command      = GBS_global_string("(%s %s;rm -f %s) &", gs, dest, dest);
            GB_information("executing '%s'", command);
            if (system(command) != 0) {
                GB_warning("error running '%s'", command);
            }
            break;
        }
        case 1:
            break;
        case 0:
            {
                char *prt = awr->awar(AWAR_PRINT_TREE_PRINTER)->read_string();
                system(GBS_global_string("%s %s", prt, dest));
                delete prt;
                GB_unlink(dest);
            }
            break;
    }

    free(xfig);
    free(dest);
    aw_closestatus();

    if (error) aw_message(error);
}


static long calc_mag_from_psize(AW_root *awr, double papersize, double gfxsize, double wantedpages) {
    bool   useOverlap = awr->awar(AWAR_PRINT_TREE_OVERLAP)->read_int();
    long   mag        = -1;
    double usableSize = 0;

    if (useOverlap) {
        double overlapAmount = awr->awar(AWAR_PRINT_TREE_OVERLAP_AMOUNT)->read_float();
        if (wantedpages>1.0) {
            while (wantedpages>1.0) {
                usableSize  += papersize-overlapAmount;
                wantedpages -= 1.0;
            }
            if (usableSize<0.1) aw_message("Usable size very low. Wrong overlap amount?");
        }
        usableSize += papersize * wantedpages; // add (partial) page (dont subtract overlapAmount)
    }
    else {
        usableSize = wantedpages*papersize;
    }

    mag = (long)(usableSize*100/gfxsize);

#if defined(DEBUG)
    fprintf(stderr, "usableSize=%f mag=%li\n", usableSize, mag);
#endif // DEBUG

    return mag;
}

// called when resulting pages x-factor was changed by users
void awt_calc_mag_from_psizex(AW_window *aww) {
    AW_root *awr         = aww->get_root();
    double   papersize   = awr->awar(AWAR_PRINT_TREE_PSIZEX)->read_float();
    double   gfxsize     = awr->awar(AWAR_PRINT_TREE_GSIZEX)->read_float();
    double   wantedpages = awr->awar(AWAR_PRINT_TREE_SIZEX)->read_float();
    long     mag         = calc_mag_from_psize(awr, papersize, gfxsize, wantedpages);

    awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->write_int(mag);
}

// called when resulting pages y-factor was changed by users
void awt_calc_mag_from_psizey(AW_window *aww) {
    AW_root *awr         = aww->get_root();
    double   papersize   = awr->awar(AWAR_PRINT_TREE_PSIZEY)->read_float();
    double   gfxsize     = awr->awar(AWAR_PRINT_TREE_GSIZEY)->read_float();
    double   wantedpages = awr->awar(AWAR_PRINT_TREE_SIZEY)->read_float();
    long     mag         = calc_mag_from_psize(awr, papersize, gfxsize, wantedpages);

    awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->write_int(mag);
}

void awt_calc_best_fit(AW_window *aww) {
    int         best_orientation    = -1;
    const char *best_zoom_awar_name = 0;
    float       best_zoom           = 0;
    int         best_magnification  = 0;
    int         best_pages          = 0;
    AW_root    *awr                 = aww->get_root();
    int         wanted_pages        = awr->awar(AWAR_PRINT_TREE_PAGES)->read_int();

    for (int o = 0; o <= 1; ++o) {
        awr->awar(AWAR_PRINT_TREE_LANDSCAPE)->write_int(o); // set orientation (calls awt_page_size_check_cb)

        for (int xy = 0; xy <= 1; ++xy) {
            const char *awar_name = xy == 0 ? AWAR_PRINT_TREE_SIZEX : AWAR_PRINT_TREE_SIZEY;

            for (int pcount = 1; pcount <= wanted_pages; pcount++) {
                double zoom = pcount*1.0;
                awr->awar(awar_name)->write_float(zoom); // set zoom (x or y)

                // calculate magnification :
                if (xy == 0) awt_calc_mag_from_psizex(aww);
                else awt_calc_mag_from_psizey(aww);

                int    magnification = awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->read_int(); // read calculated magnification
                double sx            = awr->awar(AWAR_PRINT_TREE_SIZEX)->read_float();
                double sy            = awr->awar(AWAR_PRINT_TREE_SIZEY)->read_float();
                int    pages         = xy2pages(sx, sy);

                if (pages>wanted_pages) break; // pcount-loop

#if defined(DEBUG)
                fprintf(stderr, "pages=%i sx=%f sy=%f mag=%i awar_name=%s landscape='%i'",
                        pages, sx, sy, magnification, awar_name, o);
#endif // DEBUG

                if (pages <= wanted_pages && pages >= best_pages && magnification>best_magnification) {
                    // fits on wanted_pages and is best result yet
                    best_magnification  = magnification;
                    best_orientation    = o;
                    best_zoom_awar_name = awar_name;
                    best_zoom           = zoom;
                    best_pages          = pages;
#if defined(DEBUG)
                    fprintf(stderr, " [best yet]");
#endif // DEBUG
                }
#if defined(DEBUG)
                fprintf(stderr, "\n");
#endif // DEBUG
            }
        }
    }

    if (best_orientation != -1) {
        awt_assert(best_zoom_awar_name);

        // take the best found values :
        awr->awar(AWAR_PRINT_TREE_LANDSCAPE)->write_int(best_orientation);
        awr->awar(best_zoom_awar_name)->write_float(best_zoom);
        awr->awar(AWAR_PRINT_TREE_PAGES)->write_int(best_pages);
        awr->awar(AWAR_PRINT_TREE_MAGNIFICATION)->write_int(best_magnification);

        if (best_pages != wanted_pages) {
            aw_message(GBS_global_string("That didn't fit on %i page(s) - using %i page(s)",
                                         wanted_pages, best_pages));
        }
    }
    else {
        aw_message(GBS_global_string("That didn't fit on %i page(s)", wanted_pages));
    }
}

void AWT_popup_print_window(AW_window *parent_win, AW_CL cl_canvas, AW_CL) {
    AW_root                 *awr = parent_win->get_root();
    AWT_canvas              *ntw = (AWT_canvas*)cl_canvas;
    static AW_window_simple *aws = 0;

    create_print_awars(awr, ntw);
    resetFiletype(awr, "ps", "print.ps");

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "PRINT_CANVAS", "PRINT GRAPHIC");
        aws->load_xfig("awt/print.fig");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");aws->callback(AW_POPUP_HELP, (AW_CL)"tree2prt.hlp");
        aws->create_button("HELP", "HELP", "H");


        aws->at("orientation");
        aws->create_toggle_field(AWAR_PRINT_TREE_LANDSCAPE, 1);
        aws->insert_toggle("#print/landscape.bitmap", "L", 1);
        aws->insert_toggle("#print/portrait.bitmap",  "P", 0);
        aws->update_toggle_field();
        aws->label_length(15);

        aws->at("magnification");
        aws->create_input_field(AWAR_PRINT_TREE_MAGNIFICATION, 4);

        aws->at("what");
        aws->label("Clip at Screen");
        aws->create_toggle_field(AWAR_PRINT_TREE_CLIP, 1);
        aws->insert_toggle("#print/clipscreen.bitmap", "S", 0);
        aws->insert_toggle("#print/clipall.bitmap", "A", 1);
        aws->update_toggle_field();

        aws->at("remove_root");
        aws->label("Show Handles");
        aws->create_toggle_field(AWAR_PRINT_TREE_HANDLES, 1);
        aws->insert_toggle("#print/nohandles.bitmap", "S", 0);
        aws->insert_toggle("#print/handles.bitmap", "A", 1);
        aws->update_toggle_field();

        aws->at("color");
        aws->label("Export colors");
        aws->create_toggle(AWAR_PRINT_TREE_COLOR);

        aws->button_length(6);
        aws->at("gsizex"); aws->create_button(0, AWAR_PRINT_TREE_GSIZEX);
        aws->at("gsizey"); aws->create_button(0, AWAR_PRINT_TREE_GSIZEY);
    
        aws->button_length(8);
        aws->at("psizex"); aws->create_input_field(AWAR_PRINT_TREE_PSIZEX, 4);
        aws->at("psizey"); aws->create_input_field(AWAR_PRINT_TREE_PSIZEY, 4);

        aws->at("sizex");
        aws->callback(awt_calc_mag_from_psizex);
        aws->create_input_field(AWAR_PRINT_TREE_SIZEX, 4);
        aws->at("sizey");
        aws->callback(awt_calc_mag_from_psizey);
        aws->create_input_field(AWAR_PRINT_TREE_SIZEY, 4);

        aws->at("best_fit");
        aws->callback(awt_calc_best_fit);
        aws->create_autosize_button(0, "Fit on");

        aws->at("pages");
        aws->create_input_field(AWAR_PRINT_TREE_PAGES, 3);

        aws->at("overlap");
        aws->create_toggle(AWAR_PRINT_TREE_OVERLAP);

        aws->at("amount");
        aws->create_input_field(AWAR_PRINT_TREE_OVERLAP_AMOUNT, 4);

        aws->at("printto");
        aws->label_length(12);
        aws->label("Destination");
        aws->create_toggle_field(AWAR_PRINT_TREE_DEST);
        aws->insert_toggle("Printer", "P", 0);
        aws->insert_toggle("File (Postscript)", "F", 1);
        aws->insert_toggle("Preview", "V", 2);
        aws->update_toggle_field();

        aws->at("printer");
        aws->create_input_field(AWAR_PRINT_TREE_PRINTER, 16);

        aws->at("filename");
        aws->create_input_field(AWAR_PRINT_TREE_FILE_NAME, 16);

        aws->at("go");
        aws->highlight();
        aws->callback(AWT_print_tree_to_printer, (AW_CL)ntw);
        aws->create_button("PRINT", "PRINT", "P");

        aws->button_length(0);

        aws->at("getsize");
        aws->callback((AW_CB1)awt_print_tree_check_size, (AW_CL)ntw);
        aws->create_button(0, "Get Graphic Size");
    }

    awt_print_tree_check_size(0, (AW_CL)ntw);
    aws->show();
}
