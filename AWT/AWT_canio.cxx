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


const char *AWT_print_tree_to_file(AW_window *aww, AWT_canvas * ntw)
{
    GB_transaction  dummy(ntw->gb_main);
    AW_root        *awr  = aww->get_root();
    // 	char           *dest = awr->awar(AWAR_PRINT_TREE_2_FILE_NAME)->read_string();
    char           *dest = awt_get_selected_fullname(awr, AWAR_PRINT_TREE_2_FILE_BASE);
    if (!strlen(dest)) {
        delete(dest);
        sprintf(AW_ERROR_BUFFER,"Please enter a file name first");
        aw_message();
        return AW_ERROR_BUFFER;
    }

    long what = awr->awar(AWAR_PRINT_TREE_2_FILE_WHAT)->read_int();
    long handles = awr->awar(AWAR_PRINT_TREE_2_FILE_HANDLES)->read_int();

    char tmp[1024];
    const char *error;


    sprintf(tmp,"%s",dest);


    AW_device *device= ntw->aww->get_print_device(AW_MIDDLE_AREA);
    AW_device *size_device = ntw->aww->get_size_device(AW_MIDDLE_AREA);

    device->reset();
    error = device->open(tmp);
    device->line(0,0,0,1,-1); // dummy point upper left corner

    if (what) {				// draw all
        AW_world size;
        size_device->reset();
        size_device->zoom(ntw->trans_to_fit);
        size_device->set_filter(AW_SCREEN);
        ntw->tree_disp->show(size_device);
        size_device->get_size_information(&size);
        size.l -= 50;
        size.t -= 40;		    // expand pic
        size.r += 20;
        size.b += 20;
        device->shift_x(-size.l/ntw->trans_to_fit);
        device->shift_y(-size.t/ntw->trans_to_fit);
        device->set_bottom_clip_border((int)(size.b-size.t),AW_TRUE);
        device->set_right_clip_border((int)(size.r-size.l), AW_TRUE);
        // 		device->set_bottom_font_overlap(AW_TRUE);
        // 		device->set_right_font_overlap(AW_TRUE);
        device->zoom(ntw->trans_to_fit);
    }else{
        ntw->init_device(device);	// draw screen
    }
    if (!error) {
        if (handles) {
            device->set_filter(AW_PRINTER | AW_PRINTER_EXT);
        }else{
            device->set_filter(AW_PRINTER);
        }
        ntw->tree_disp->show(device);
        device->close();
        awr->awar(AWAR_PRINT_TREE_2_FILE_DIR)->touch();	// reload dir !!!
    }


    if (error) aw_message(error);
    else	aww->get_root()->awar(AWAR_PRINT_TREE_2_FILE_DIR)->touch();
    delete dest;
    return error;
}

void AWT_print_tree_to_file_xfig(AW_window *aww, AW_CL cl_ntw){
    AWT_canvas * ntw = (AWT_canvas*)cl_ntw;
    AW_root *awr = aww->get_root();
    const char *error = AWT_print_tree_to_file(aww,ntw);
    if (!error) {
        // char *dest = awr->awar(AWAR_PRINT_TREE_2_FILE_NAME)->read_string();
        char *dest = awt_get_selected_fullname(awr, AWAR_PRINT_TREE_2_FILE_BASE);
        char  com[1024];
        sprintf(com,"xfig %s &",dest);
        system(com);
        delete dest;
    }
}

AW_window * AWT_create_export_window(AW_root *awr, AWT_canvas *ntw){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    AW_default def = AW_ROOT_DEFAULT;
    awr->awar_string( AWAR_PRINT_TREE_2_FILE_ORIENTATION, "", def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_MAGNIFICATION, 100, def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_WHAT, 0, def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_HANDLES, 1, def);

    awr->awar_string( AWAR_PRINT_TREE_2_FILE_NAME, "print.fig", def);
    awr->awar_string( AWAR_PRINT_TREE_2_FILE_DIR, "", def);
    awr->awar_string( AWAR_PRINT_TREE_2_FILE_FILTER, "fig", def);

    aws = new AW_window_simple;
    aws->init( awr, "EXPORT_TREE_AS_XFIG" ,"EXPORT TREE TO XFIG");
    aws->load_xfig("awt/export.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"tree2file.hlp");
    aws->create_button("HELP", "HELP","H");

    aws->label_length(15);

    awt_create_selection_box((AW_window *)aws,AWAR_PRINT_TREE_2_FILE_BASE);


    aws->at("what");
    aws->label("Clip at Screen");
    aws->create_toggle_field(AWAR_PRINT_TREE_2_FILE_WHAT,1);
    aws->insert_toggle("#print/clipscreen.bitmap","S",0);
    aws->insert_toggle("#print/clipall.bitmap","A",1);
    aws->update_toggle_field();

    aws->at("remove_root");
    aws->label("Show Handles");
    aws->create_toggle_field(AWAR_PRINT_TREE_2_FILE_HANDLES,1);
    aws->insert_toggle("#print/nohandles.bitmap","S",0);
    aws->insert_toggle("#print/handles.bitmap","A",1);
    aws->update_toggle_field();



    aws->at("xfig");aws->callback(AWT_print_tree_to_file_xfig,(AW_CL)ntw);
    aws->create_button("START_XFIG", "GO XFIG","X");

    aws->at("cancel");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CANCEL","C");

    return (AW_window *)aws;
}
/*------------------------------------- to export secondary structure to XFIG ---------------------------------------------*/

AW_window * AWT_create_sec_export_window(AW_root *awr, AWT_canvas *ntw){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    AW_default def = AW_ROOT_DEFAULT;
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_WHAT, 0, def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_HANDLES, 1, def);

    awr->awar_string( AWAR_PRINT_TREE_2_FILE_NAME, "print.fig", def);
    awr->awar_string( AWAR_PRINT_TREE_2_FILE_DIR, "", def);
    awr->awar_string( AWAR_PRINT_TREE_2_FILE_FILTER, "fig", def);

    aws = new AW_window_simple;
    aws->init( awr, "EXPORT_TREE_AS_XFIG" ,"EXPORT STRUCTURE TO XFIG");
    aws->load_xfig("awt/secExport.fig");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"tree2file.hlp");
    aws->create_button("HELP", "HELP","H");

    aws->label_length(15);
    awt_create_selection_box((AW_window *)aws,AWAR_PRINT_TREE_2_FILE_BASE);

    aws->at("what");
    aws->label("Clip Options");
    aws->create_toggle_field(AWAR_PRINT_TREE_2_FILE_WHAT,1);
    aws->insert_toggle("#print/clipscreen.bitmap","S",0);
    aws->insert_toggle("#print/clipall.bitmap","A",1);
    aws->update_toggle_field();

    aws->at("xfig");aws->callback(AWT_print_tree_to_file_xfig,(AW_CL)ntw);
    aws->create_button("START_XFIG", "EXPORT to XFIG","X");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("cancel");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CANCEL","C");

    return (AW_window *)aws;
}
/*------------------------------------------------------------------------------------------------------------------------------------------*/

void AWT_print_tree_to_printer(AW_window *aww, AW_CL cl_ntw)
{
    AWT_canvas *    ntw = (AWT_canvas*)cl_ntw;
    GB_transaction  dummy(ntw->gb_main);
    AW_root        *awr = aww->get_root();

    long print2file = awr->awar(AWAR_PRINT_TREE_PRINT "dest")->read_int();
    char *dest;
    GB_ERROR error = 0;


    char *orientation = awr->awar(AWAR_PRINT_TREE_2_FILE_ORIENTATION)->read_string();
    double magnification = awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)->read_int() * 0.01;
    long what = awr->awar(AWAR_PRINT_TREE_2_FILE_WHAT)->read_int();
    long handles = awr->awar(AWAR_PRINT_TREE_2_FILE_HANDLES)->read_int();

    char sys[2400];
    char *xfig = GBS_eval_env("/tmp/arb_print_$(USER)_$(ARB_PID).xfig");

    switch(print2file) {
        case 1:
            // dest = awr->awar(AWAR_PRINT_TREE_2_FILE_NAME)->read_string();
            dest = awt_get_selected_fullname(awr, AWAR_PRINT_TREE_2_FILE_BASE);
            break;
        default:
            dest = GBS_eval_env("/tmp/arb_print_$(USER)_$(ARB_PID).ps");
            break;
    }
    {
        FILE *out;
        out = fopen(dest,"w");
        if (!out) {
            error = (char *)GB_export_error("Cannot open file '%s'",dest);
        }
        else {
            fclose(out);
            // 			sprintf(sys,"fig2dev -L ps -P -m %f %s %s %s", magnification,orientation, xfig, dest);
            sprintf(sys,"fig2dev -L ps -M -m %f %s %s %s", magnification, orientation, xfig, dest);
        }
    }

    AW_device *device= ntw->aww->get_print_device(AW_MIDDLE_AREA);

    aw_openstatus("Printing");

    device->reset();
    ntw->init_device(device);	// draw screen

    aw_status("Get Picture Size");
    device->reset();
    error = device->open(xfig);
    device->line(0,0,0,1,-1); // dummy point upper left corner
    if (what) {				// draw all
        AW_world size;
        AW_device *size_device = ntw->aww->get_size_device(AW_MIDDLE_AREA);
        size_device->reset();
        size_device->zoom(ntw->trans_to_fit);
        size_device->set_filter(AW_SCREEN);
        ntw->tree_disp->show(size_device);
        size_device->get_size_information(&size);
        size.l -= 50;
        size.t -= 40;		// expand pic
        size.r += 20;
        size.b += 20;
        device->shift_x(-size.l/ntw->trans_to_fit);
        device->shift_y(-size.t/ntw->trans_to_fit);
        device->set_bottom_clip_border((int)(size.b-size.t),AW_TRUE);
        device->set_right_clip_border((int)(size.r-size.l),AW_TRUE);
        device->zoom(ntw->trans_to_fit);
    }else{
        ntw->init_device(device);	// draw screen
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
        if (system(sys)){
            error = GB_export_error("System error occured while running '%s'",sys);
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
            GB_CSTR command      = GBS_global_string("(%s %s;rm -f %s) &",gs,dest,dest);
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
                char *prt = awr->awar(AWAR_PRINT_TREE_PRINT "printer")->read_string();
                system(GBS_global_string("%s %s",prt,dest));
                delete prt;
                GB_unlink(dest);
            }
            break;
    }

    free(xfig);
    free(dest);
    free(orientation);
    aw_closestatus();

    if (error) aw_message(error);
}


void awt_print_tree_check_size(AW_window *, AW_CL cl_ntw) {
    AWT_canvas     *ntw  = (AWT_canvas*)cl_ntw;
    GB_transaction  dummy2(ntw->gb_main);
    AW_world        size;
    long            what = ntw->aww->get_root()->awar(AWAR_PRINT_TREE_2_FILE_WHAT)->read_int();

    AW_device *size_device = ntw->aww->get_size_device(AW_MIDDLE_AREA);

    if (what){
        size_device->reset();
        size_device->zoom(ntw->trans_to_fit);
        size_device->set_filter(AW_SCREEN);
        ntw->tree_disp->show(size_device);
        size_device->get_size_information(&size);
    }else{
        size_device->get_area_size( &size ) ;
    }

    ntw->aww->get_root()->awar(AWAR_PRINT_TREE_PRINT "gsizex")->write_float( (size.r-size.l + 30)/80);
    ntw->aww->get_root()->awar(AWAR_PRINT_TREE_PRINT "gsizey")->write_float( (size.b-size.t + 30)/80);
}

void awt_page_size_check_cb(void *dummy,AW_root *awr) {
    AWUSE(dummy);
    char *orientation	= awr->awar(AWAR_PRINT_TREE_2_FILE_ORIENTATION)->read_string();
    double px		= awr->awar(AWAR_PRINT_TREE_PRINT "psizex")->read_float();
    double py		= awr->awar(AWAR_PRINT_TREE_PRINT "psizey")->read_float();
    int swap = 0;
    if (strlen(orientation)){	// Landscape
        if ( px < py ) swap = 1;
    }else{
        if ( px > py ) swap = 1;
    }
    if (swap) {
        awr->awar(AWAR_PRINT_TREE_PRINT "psizex")->write_float(py);	// recalls this function
        awr->awar(AWAR_PRINT_TREE_PRINT "psizey")->write_float(px);
        return;
    }
    long magnification = awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)->read_int();
    double gx		= awr->awar(AWAR_PRINT_TREE_PRINT "gsizex")->read_float();
    double gy		= awr->awar(AWAR_PRINT_TREE_PRINT "gsizey")->read_float();

    awr->awar( AWAR_PRINT_TREE_PRINT "sizex")->write_float( gx * magnification / 100 / px);
    awr->awar( AWAR_PRINT_TREE_PRINT "sizey")->write_float( gy * magnification / 100 / py);

    free(orientation);
}

// called when resulting pages (x) were changed by users
void awt_calc_mag_from_psizex(AW_window *aww) {
    AW_root *awr = aww->get_root();
    double   p	 = awr->awar(AWAR_PRINT_TREE_PRINT "psizex")->read_float();
    double   g	 = awr->awar(AWAR_PRINT_TREE_PRINT "gsizex")->read_float();
    double   s	 = awr->awar(AWAR_PRINT_TREE_PRINT "sizex")->read_float();

    awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)->write_int( (long)( s * p * 100 / g) );
}

// called when resulting pages (y) were changed by users
void awt_calc_mag_from_psizey(AW_window *aww) {
    AW_root *awr = aww->get_root();
    double   p	 = awr->awar(AWAR_PRINT_TREE_PRINT "psizey")->read_float();
    double   g	 = awr->awar(AWAR_PRINT_TREE_PRINT "gsizey")->read_float();
    double   s	 = awr->awar(AWAR_PRINT_TREE_PRINT "sizey")->read_float();

    awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)->write_int( (long)( s * p * 100 / g) );
}



void awt_calc_best_fit(AW_window *aww) {
    const char *best_orientation    = 0;
    const char *best_zoom_awar_name = 0;
    int         best_magnification  = 0;
    AW_root    *awr                 = aww->get_root();

    for (int o = 0; o <= 1; ++o) {
        const char *orientation = o ? "-l 0" : ""; // Landscape or Portrait
        awr->awar(AWAR_PRINT_TREE_2_FILE_ORIENTATION)->write_string(orientation); // set orientation (calls awt_page_size_check_cb)

        for (int xy = 0; xy <= 1; ++xy) {
            const char *awar_name;
            if (xy == 0) awar_name = AWAR_PRINT_TREE_PRINT "sizex";
            else        awar_name  = AWAR_PRINT_TREE_PRINT "sizey";

            awr->awar(awar_name)->write_float(1.0); // set zoom (x or y)

            // calculate magnification :
            if (xy == 0) awt_calc_mag_from_psizex(aww);
            else awt_calc_mag_from_psizey(aww);

            int    magnification = awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)->read_int(); // read calculated magnification
            double sx		     = awr->awar(AWAR_PRINT_TREE_PRINT "sizex")->read_float();
            double sy		     = awr->awar(AWAR_PRINT_TREE_PRINT "sizey")->read_float();

#if defined(DEBUG)
            fprintf(stderr, "sx=%f sy=%f mag=%i awar_name=%s orientation='%s'\n", sx, sy, magnification, awar_name, orientation);
#endif // DEBUG

            if (sx <= 1.0 && sy <= 1.0 && magnification>best_magnification) { // yes -- fits on 1 page and is best result yet
                best_magnification  = magnification;
                best_orientation    = orientation;
                best_zoom_awar_name = awar_name;
            }
        }
    }

    if (best_orientation) {
        awt_assert(best_zoom_awar_name);

        // take the best found values :
        awr->awar(AWAR_PRINT_TREE_2_FILE_ORIENTATION)->write_string(best_orientation);
        awr->awar(best_zoom_awar_name)->write_float(1.0);
    }
    else {
        aw_message("That won't fit on 1 page -- whyever?!");
    }
}

void AWT_create_print_window(AW_window *parent_win, AWT_canvas *ntw){
    AW_root *awr = parent_win->get_root();
    static AW_window_simple *aws = 0;

    if (aws) {
        awt_print_tree_check_size(0,(AW_CL)ntw);
        aws->show();
        return;
    }

    AW_default def = AW_ROOT_DEFAULT;
    awr->awar_string( AWAR_PRINT_TREE_2_FILE_ORIENTATION, "", def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_MAGNIFICATION, 100, def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_WHAT, 0, def);
    awr->awar_int( AWAR_PRINT_TREE_2_FILE_HANDLES, 1, def);

    awr->awar_string( AWAR_PRINT_TREE_2_FILE_NAME, "print.ps", def);

    awr->awar_float( AWAR_PRINT_TREE_PRINT "gsizex");
    awr->awar_float( AWAR_PRINT_TREE_PRINT "gsizey");

    awr->awar_float( AWAR_PRINT_TREE_PRINT "psizex",7.5);
    awr->awar_float( AWAR_PRINT_TREE_PRINT "psizey", 9.5);


    awr->awar_float( AWAR_PRINT_TREE_PRINT "sizex");
    awr->awar_float( AWAR_PRINT_TREE_PRINT "sizey");

    awr->awar_int( AWAR_PRINT_TREE_PRINT "dest");
    {
        char *print_command;
        if (getenv("PRINTER")){
            print_command = GBS_eval_env("lpr -h -P$(PRINTER)");
        }else	print_command = strdup("lpr -h");

        awr->awar_string( AWAR_PRINT_TREE_PRINT "printer",print_command,def);
        free(print_command);
    }

    awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)->set_minmax( 1, 10000);
    awr->awar(AWAR_PRINT_TREE_PRINT "psizex")->set_minmax( 0.1, 100);
    awr->awar(AWAR_PRINT_TREE_PRINT "psizey")->set_minmax( 0.1, 100);

    awt_print_tree_check_size(0,(AW_CL)ntw);

    awr->awar(AWAR_PRINT_TREE_2_FILE_WHAT)		->add_callback((AW_RCB1)awt_print_tree_check_size,(AW_CL)ntw);
    awr->awar(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION)	->add_callback((AW_RCB1)awt_page_size_check_cb,(AW_CL)awr);
    awr->awar(AWAR_PRINT_TREE_2_FILE_ORIENTATION)	->add_callback((AW_RCB1)awt_page_size_check_cb,(AW_CL)awr);
    awr->awar(AWAR_PRINT_TREE_PRINT "psizex")	->add_callback((AW_RCB1)awt_page_size_check_cb,(AW_CL)awr);
    awr->awar(AWAR_PRINT_TREE_PRINT "psizey")	->add_callback((AW_RCB1)awt_page_size_check_cb,(AW_CL)awr);
    awr->awar(AWAR_PRINT_TREE_PRINT "gsizex")	->add_callback((AW_RCB1)awt_page_size_check_cb,(AW_CL)awr);
    awr->awar(AWAR_PRINT_TREE_PRINT "gsizey")	->add_callback((AW_RCB1)awt_page_size_check_cb,(AW_CL)awr);
    awt_page_size_check_cb(0,awr);

    aws = new AW_window_simple;
    aws->init( awr, "PRINT_CANVAS", "PRINT GRAPHIC");
    aws->load_xfig("awt/print.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"tree2prt.hlp");
    aws->create_button("HELP", "HELP","H");


    aws->at("orientation");
    aws->create_toggle_field(AWAR_PRINT_TREE_2_FILE_ORIENTATION,1);
    aws->insert_toggle("#print/landscape.bitmap","L","-l 0");
    aws->insert_toggle("#print/portrait.bitmap","P","");
    aws->update_toggle_field();
    aws->label_length(15);

    aws->at("magnification");
    aws->create_input_field(AWAR_PRINT_TREE_2_FILE_MAGNIFICATION,4);

    aws->at("what");
    aws->label("Clip at Screen");
    aws->create_toggle_field(AWAR_PRINT_TREE_2_FILE_WHAT,1);
    aws->insert_toggle("#print/clipscreen.bitmap","S",0);
    aws->insert_toggle("#print/clipall.bitmap","A",1);
    aws->update_toggle_field();

    aws->at("remove_root");
    aws->label("Show Handles");
    aws->create_toggle_field(AWAR_PRINT_TREE_2_FILE_HANDLES,1);
    aws->insert_toggle("#print/nohandles.bitmap","S",0);
    aws->insert_toggle("#print/handles.bitmap","A",1);
    aws->update_toggle_field();

    aws->button_length(7);
    aws->at("gsizex");
    aws->create_button(0, AWAR_PRINT_TREE_PRINT "gsizex");
    aws->at("gsizey");
    aws->create_button(0, AWAR_PRINT_TREE_PRINT "gsizey");
    aws->button_length(8);

    aws->at("psizex");
    aws->create_input_field(AWAR_PRINT_TREE_PRINT "psizex",4);
    aws->at("psizey");
    aws->create_input_field(AWAR_PRINT_TREE_PRINT "psizey",4);

    aws->at("sizex");
    aws->callback(awt_calc_mag_from_psizex);
    aws->create_input_field(AWAR_PRINT_TREE_PRINT "sizex",4);
    aws->at("sizey");
    aws->callback(awt_calc_mag_from_psizey);
    aws->create_input_field(AWAR_PRINT_TREE_PRINT "sizey",4);

    aws->at("best_fit");
    aws->callback(awt_calc_best_fit);
    aws->create_autosize_button(0, "Fit on page");

    aws->at("printto");
    aws->label_length(12);
    aws->label("Destination");
    aws->create_toggle_field(AWAR_PRINT_TREE_PRINT "dest");
    aws->insert_toggle("Printer","P",0);
    aws->insert_toggle("File (Postscript)","F",1);
    aws->insert_toggle("Preview","V",2);
    aws->update_toggle_field();

    aws->at("printer");
    //aws->label("Print Command");
    aws->create_input_field(AWAR_PRINT_TREE_PRINT "printer", 16);

    aws->at("filename");
    //aws->label("File Name");
    aws->create_input_field(AWAR_PRINT_TREE_2_FILE_NAME, 16);

    aws->at("go");
    aws->highlight();
    aws->callback(AWT_print_tree_to_printer, (AW_CL)ntw);
    aws->create_button("PRINT", "PRINT","P");

    aws->button_length(0);

    aws->at("getsize");
    aws->callback(awt_print_tree_check_size, (AW_CL)ntw);
    aws->create_button(0, "Get Graphic Size");


    aws->show();
}
