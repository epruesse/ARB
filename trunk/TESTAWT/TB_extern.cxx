// see awdemo for a (slightly out of date) demonstration of direct graphics without
// canvas support

/* -----------------------------------------------------------------
 * Module:                        TESTBED/TB_extern.cxx
 *
 * Description:  the meat of this demonstration of AWT and WINDOW
 *               use - careful, as we had to start tons of stuff
 *               to get here, even if we'll never need most of it
 *		 (trees, db, ...)
 *
 *  See AWT, PH_dtree, ...  for a larger example
 * -----------------------------------------------------------------
 */

/*
 * $Header$
 *
 * $Log$
 * Revision 1.4  2004/06/30 17:34:01  westram
 * - zoom.bitmap -> pzoom.bitmap
 *
 * Revision 1.3  2002/12/17 10:36:40  westram
 * valgrinded
 *
 * Revision 1.2  2001/06/06 18:34:19  westram
 * changes for color groups (working for gene map)
 *
 * Revision 1.1.1.1  2000/11/23 09:41:16  westram
 * Erster Import
 *
 * Revision 1.2  1995/03/13  16:53:19  jakobi
 * *** empty log message ***
 *
 */

#include <stdio.h>
#include <stdlib.h>

// string functions and db stuff
#include <arbdb.h>
#include <arbdbt.h>

// window stuff
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
// here we set some stuff for zoomtext and other defaults
#include <aw_preset.hxx>
#include <awt_preset.hxx>

// ton's of stuff for canvas support (which simply should allow
// us to concentrate on our core functions instead of doing
// lowlevel display games)
#include <awt_canvas.hxx>
#include "awt_tree.hxx"
#include "awt_dtree.hxx"

// more stuff, just needed to startup the filerequester
// awt.hxx is req'd for filereq
#include <awt.hxx>
//#include <awt_nds.hxx>
#include <awt_tree_cb.hxx>

#include "tb_extern.hxx"


/* -----------------------------------------------------------------
 * Class:                        AWT_graphic_testbed
 *
 * Description:                  derived class to interface with AWT.
 *                               these routines display our graphics
 * -----------------------------------------------------------------
 */

AWT_graphic_testbed::AWT_graphic_testbed(void):AWT_graphic() {
        exports.dont_fit_x = 0;
        exports.dont_fit_y = 0;
        exports.left_offset = 100;
        exports.right_offset = 100;
        exports.top_offset = 30;
        exports.bottom_offset = 30;
}

// this is used for evaluating commands/actions in the canvas,
// necessary dummies (input in the canvas, ...)
// see AWT_dtree,cxx:AWT_graphic_tree
void AWT_graphic_testbed::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, char key_char, AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) {
	AWUSE(device); AWUSE(cmd); AWUSE(button); AWUSE(type);
	AWUSE(x); AWUSE(y); AWUSE(cl); AWUSE(ct);
}

// called by canvas to display/create the objects to be displayed
// canvas simply calls us, it would be our job here to box larger subparts and
// ask clip, whether we really have to look into that box (see WINDOW/AW_xfigfont.hxx/zoomtext1 as example)
void AWT_graphic_testbed::show(AW_device *device) {
	// central cross
	device->line(AWT_GC_CURSOR,-1.0,0.0,1.0,0.0,AW_ALL,0,0);
	device->line(AWT_GC_CURSOR,0.0,1.0,0.0,-1.0,AW_ALL,0,0);
	// title
	device->text(AWT_GC_CURSOR, "testforme", -1.0, -1.0, 0.0, AW_SCREEN, 0, 0);
	// set devices for this text (just don't move fixed sized text (like standard text) into the size device...)
	AW_bitset bitset=AW_SCREEN;
	if (tbd_bitset) bitset=tbd_bitset;
        // run the test function
	if(tbd_function)
		device->zoomtext((int)tbd_gc,tbd_string,tbd_x,tbd_y,tbd_height,
                         tbd_alignment,tbd_rotation,bitset,
			 tbd_cl1,tbd_cl2);
	else
		device->zoomtext4line((int)tbd_gc,tbd_string,tbd_height,
			 tbd_x0,tbd_y0,tbd_x1,tbd_y1,
                         tbd_alignmentx,tbd_alignmenty,bitset,
                         tbd_cl1,tbd_cl2);
}

// necessary dummy, otherwise cc would theatralically cry 'virtual class' and die
void AWT_graphic_testbed::info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) {
        aw_message("INFO MESSAGE");
        AWUSE(device); AWUSE(x); AWUSE(y); AWUSE(cl); AWUSE(ct);
}


//
// presets
//


// a small gc manager is needed to tell AW_presets what we like to offer in the userinterface ...
AW_gc_manager AWT_graphic_testbed::init_devices(AW_window *window, AW_device * device, AWT_canvas *canvas ,AW_CL cl) {
        AW_gc_manager preset_window =
                AW_manage_GC(window, device, AWT_GC_CURSOR, AWT_GC_CURSOR+5, AW_GCM_DATA_AREA,
                             (AW_CB)AWT_resize_cb, (AW_CL)canvas, cl,
                             false, // no color groups
                             "",
                             "CURSOR",
                             "SELECTED",
                             "SEL. & NOT",
                             "NOT SEL.",
                             "ETC",
                             0 );
        return preset_window;
}


//
// testbed stuff: userinterface related routines - callback, etc
//

void tb_exit(AW_window *aw_window) {
	AWUSE(aw_window);
	exit(0);
}

// generate and show current gfx - the "TEST" button
void tb_action(AW_window *aw_window,AW_CL canvas) {
	AW_device *device=aw_window->get_device(AW_MIDDLE_AREA);
	tb_reload(aw_window->get_root(), (AWT_canvas *)canvas);
}

// generate current gfx
AWT_graphic *tb_gen_gfx(void) {
	AWT_graphic_testbed *gfx = new AWT_graphic_testbed;
        return (AWT_graphic *)gfx;
}

// (re)display gfx (canvas then calls ->show)
void tb_reload(AW_root *awr, AWT_canvas *canvas) {
	AWUSE(awr);
	canvas->zoom_reset();
        AWT_expose_cb(0,canvas,0);
}

// here we create the main window, menues, ... and a physical zoom mode
AW_window * create_tb_main_window(AW_root *awr) {
        AW_gc_manager aw_gc_manager; // not used?
        AWT_graphic *gfx=tb_gen_gfx();

	AW_window_menu_modes *awm = new AW_window_menu_modes();
	awm->init(awr,"ARB_TB", 800,600,0,0);

	awm->button_length(5);

	// dummy tree
	AWT_canvas *canvas = new AWT_canvas(gb_main,(AW_window *)awm,gfx,aw_gc_manager) ;
	tb_reload(awr,canvas);// load it for the first time...

    awm->create_menu(       0,   "File",     "F", "nt_file.hlp",  F_ALL );
	awm->insert_menu_topic(0,"Save Defaults (in ~/.arb_prop/testawt.awt)","D","savedef.hlp",F_ALL, (AW_CB) AW_save_defaults, 0, 0 );
	awm->insert_menu_topic(0,"Reset Physical Zoom", "P","rst_phys_zoom.hlp",F_ALL, (AW_CB)NT_reset_pzoom_cb, (AW_CL) canvas, 0 );
	awm->insert_menu_topic( 0, "Quit",				"Q","quit.hlp",	F_ALL, (AW_CB)tb_exit, 	0, 0 );

// PJ restructured preferences
    awm->create_menu(0,"Properties","r","properties.hlp", F_ALL);
	awm->insert_menu_topic(0,"Application: Fonts, etc",		"F","props_frame.hlp",	F_ALL, AW_POPUP, (AW_CL)AWT_preset_window, 0 );
	awm->insert_menu_topic(0,"TestSetup",                           "T","nt_props_tree.hlp",F_ALL, AW_POPUP, (AW_CL)tb_preset_tree_window, 0);
        awm->insert_menu_topic(0,"DataDisplay: Fonts, etc",             "D","nt_props_data.hlp",F_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );

	awm->create_mode( 0, "pzoom.bitmap", "mode_pzoom.hlp", F_ALL, (AW_CB)nt_mode_event,
                      (AW_CL)canvas,
                      (AW_CL)AWT_MODE_ZOOM
                      );


	awm->at(5,2);
	awm->auto_space(0,-2);
	awm->shadow_width(1);

	int db_actx,db_acty;
	awm->get_at_position( &db_actx,&db_acty );
	awm->callback(tb_action,(AW_CL) canvas);
	awm->button_length(20);
	awm->create_button("Test","Test");

	awm->at_newline();
	awm->auto_space(-2,-2);

	awm->at_newline();
	int db_specx,db_specy;
	awm->get_at_position( &db_specx,&db_specy );
	awm->set_info_area_height( db_specy+6 );

	awm->set_bottom_area_height( 0 );

	return (AW_window *)awm;

}

// parameter setup panel for the user
AW_window *tb_preset_tree_window( AW_root *root )  {

	AW_window_simple *aws = new AW_window_simple;
	const int	tabstop = 400;

	// window sizes

	aws->init( root, "Test Options", 600, 600, 400, 300 );
	aws->label_length( 25 );
	aws->button_length( 20 );

	aws->at           ( 10,10 );
	aws->auto_space(10,10);
	aws->callback     ( AW_POPDOWN );
	aws->create_button( "CLOSE", "CLOSE", "C" );

 	// aws->callback     ( (AW_CB0)AW_save_defaults );
	// aws->create_button( "SAVE", "SAVE", "S" );
	aws->at_newline();

        // No Help, as we usually do not know the associated help file from within presets of AW Lib

       	aws->create_option_menu( "vectorfont/active", "Data font type", "1" );
       	aws->insert_option        ( "Use vectorfont",     " ", (int) 1);
       	aws->insert_option        ( "Use standard font",  " ", (int) 0);
       	aws->update_option_menu();
       	aws->at_newline();

     	AW_preset_create_scale_chooser(aws,"vectorfont/userscale","VF: absolute scaling");
       	aws->create_input_field( "vectorfont/userscale",6);
       	aws->at_newline();

	aws->create_button("Testdataset","Testdataset");
	aws->at_newline();

	aws->create_option_menu("tbd_function","Vector font type","1");
	aws->insert_option("Zoomtext"," ", (int) 1);
	aws->insert_option("Zoomtext4Line"," ", (int) 0);
	aws->update_option_menu();
        aws->at_newline();

	// WARNING: labels are just labels for other elements, so don't tab here...

        aws->label("GC");
        aws->create_input_field("tbd_gc",10);
        aws->at_newline();

	aws->label("String");
	aws->create_input_field("tbd_string",10);
	aws->at_newline();

	aws->label("Height");
        aws->create_input_field("tbd_height",10);
        aws->at_newline();

        aws->label("Bitset");
        aws->create_input_field("tbd_bitset",10);
        aws->at_newline();

        aws->label("CL1");
        aws->create_input_field("tbd_cl1",10);
        aws->at_newline();

        aws->label("CL2");
        aws->create_input_field("tbd_cl2",10);
        aws->at_newline();

	aws->create_button(0,"Zoomtext Parameters");

	aws->at_newline();

	aws->label("x/y");
	aws->create_input_field("tbd_x",10);
	aws->at_x(tabstop);
	aws->create_input_field("tbd_y",10);
	aws->at_newline();

	aws->label("alignment");
        aws->create_input_field("tbd_alignment",10);
	aws->at_newline();

	aws->label("rotation");
        aws->create_input_field("tbd_rotation",10);
        aws->at_newline();

	aws->create_button(0,"Zoomtext4line Parameters");
	aws->at_newline();

	aws->label("x0/y0 x1/y1");
        aws->create_input_field("tbd_x0",10);
        aws->at_x(tabstop);
        aws->create_input_field("tbd_y0",10);
        aws->at_x(tabstop);
        aws->at_newline();
	aws->label(" ");
	aws->create_input_field("tbd_x1",10);
        aws->at_x(tabstop);
        aws->create_input_field("tbd_y1",10);
        aws->at_newline();

	aws->label("alignment x/y");
        aws->create_input_field("tbd_alignmentx",10);
        aws->at_x(tabstop);
        aws->create_input_field("tbd_alignmenty",10);
        aws->at_newline();

	aws->window_fit();
	return (AW_window *)aws;

}

