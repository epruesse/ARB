/* -----------------------------------------------------------------
 * Module:                        TESTBED/TB_main.cxx
 *
 * Description: demonstration of the use of AWT and WINDOW libraries, as
 *              a testbed for the vectorfont functions
 *
 * Integration Notes: use the callback in TB_extern.cxx for your own stuff
 *
 * -----------------------------------------------------------------
 */

/*
 * $Header$
 *
 * $Log$
 * Revision 1.4  2005/01/05 11:46:52  westram
 * - uses GBDATA*
 *
 * Revision 1.3  2003/08/14 23:54:06  westram
 * - uses AWAR_FOOTER
 *
 * Revision 1.2  2002/06/20 14:08:06  westram
 * Mac OSX patches from Ben Hines
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
// #include <malloc.h>


#include <arbdb.h>
#include <arbdbt.h>

// WINDOW stuff
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

// AWT stuff, canvas, etc, for use of higher level graphics
#include <awt_canvas.hxx>
#include <awt_nds.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>

// my own stuff
#define EXTERNAL
#include "tb_extern.hxx"

// necessary black magic for X
// required dummy for linker; must be defined in first object to linker
// extern "C" { int XtAppInitialize(); } ; void aw_never_called_main(void) { XtAppInitialize(); }?
AW_HEADER_MAIN;

// required dummy for AWT, even if you don't use trees...
void AD_map_viewer(GBDATA *dummy)
{
    AWUSE(dummy);
}

// necessary for AWT and other stuff, just NULL it
GBDATA *gb_main;

int main(int argc, char **argv)
{
    AW_root *aw_root;
    AW_default aw_default;
    AW_window *aww;

    // db is *not* used, so we don't care about transactions
    // for getting an non-empty db for getting an empty tree for getting the canvas ...
    /*
      gb_main = GB_open(argv[1],"rw");
      if (!gb_main) {
      aw_message(GB_get_error());
      exit(0);
      }
    */

    // keep db stuff happy
    gb_main=NULL;

    // get new root object
    aw_initstatus();
    aw_root = new AW_root;

    // init awars
    aw_default = aw_root->open_default(".arb_prop/testawt.awt");
    aw_root->init_variables(aw_default);
    aw_root->init("ARB_TB");

    // set the parameters for testing zoomtext (as awars)
    aw_root->create_string(AWAR_FOOTER,NULL, aw_default,"");
    aw_root->create_string("tbd_string",&tbd_string,aw_default,"Ursprung");
    aw_root->create_int("tbd_gc",&tbd_gc,aw_default,AWT_GC_CURSOR);
    aw_root->create_int("tbd_function",&tbd_function,aw_default,0);
    aw_root->create_int("tbd_bitset",&tbd_bitset,aw_default,0);
    aw_root->create_int("tbd_cl2",&tbd_cl2,aw_default,0);
    aw_root->create_int("tbd_cl1",&tbd_cl1,aw_default,0);
    aw_root->create_float("tbd_x",&tbd_x,aw_default,0.0);
    aw_root->create_float("tbd_y",&tbd_y,aw_default,0.0);
    aw_root->create_float("tbd_x0",&tbd_x0,aw_default,0.0);
    aw_root->create_float("tbd_y0",&tbd_y0,aw_default,0.0);
    aw_root->create_float("tbd_x1",&tbd_x1,aw_default,1.0);
    aw_root->create_float("tbd_y1",&tbd_y1,aw_default,1.0);
    aw_root->create_float("tbd_height",&tbd_height,aw_default,.1);
    aw_root->create_float("tbd_alignment",&tbd_alignment,aw_default,0.0);
    aw_root->create_float("tbd_alignmentx",&tbd_alignmentx,aw_default,0.0);
    aw_root->create_float("tbd_alignmenty",&tbd_alignmenty,aw_default,0.0);
    aw_root->create_float("tbd_rotation",&tbd_rotation,aw_default,0.0);

    // open the main window
    aww = create_tb_main_window(aw_root);
    aww->show();

    // and go to the loop
    aw_root->main_loop();

    return 0;
}
