#include <stdio.h>


#include <arbdb.h>
//#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt.hxx>
#include <awt_canvas.hxx>
#include <aw_preset.hxx>
#include <awt_preset.hxx>


#include "d_classes.hxx"
#include "d_main.hxx"
#include "d_awt_graphic_designer.hxx"

#include "arb_assert.hxx"
#define test_assert(cond) arb_assert(cond)

AW_HEADER_MAIN;

void AD_map_viewer(GBDATA* gbd){return;};
GBDATA *gb_main;


AW_window *create_main_window(AW_root *aw_root) {

    AW_gc_manager aw_gc_manager = 0;
    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(aw_root,"DESIGNER", 800,600,0,0);
    AWT_canvas *ntw = new AWT_canvas(gb_main,awm,(AWT_graphic *)new AWT_graphic_designer(), aw_gc_manager);
    awm->create_menu(       0,  "File", "F",    "xxx.hlp",  0 );
    awm->insert_menu_topic( 0,  "Quit", "Q",    "quit.hlp", 0, (AW_CB)quit_cb, 0, 0);
    awm->insert_separator();
    awm->insert_menu_topic(0,"Colors and Fonts ...", "C","colors.hlp",0,
                           AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    awm->insert_menu_topic(0, "Print View to Printer ...",  "P","print.hlp",0,(AW_CB)AWT_create_print_window, (AW_CL)ntw,   0 );

    awm->create_mode( 0, "pzoom.bitmap", "mode_pzoom.hlp", 0, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_ZOOM
                      );
    awm->create_mode( 0, "lzoom.bitmap", "mode_lzoom.hlp", 0, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_LZOOM
                      );

    awm->set_info_area_height( 0 );
    awm->set_bottom_area_height( 0 );

    return (AW_window *)awm;
}

void quit_cb(AW_window *aww) {
    exit(0);
}



int main(int argc, char **argv) {
    class Element *root;
    class Element *tmp_elem;
    int number = 5;


    AW_root *aw_root;
    AW_default aw_default;

    char *db_server =":";

    //  aw_initstatus();

    aw_root = new AW_root;
    aw_default = aw_root->open_default(".designer");
    aw_root->init_variables(aw_default);
    aw_root->init("DESIGNER");

    //***********************************************************

        root = new Element;

        printf("start\n");

        root->set_contents(1);
        root->print_contents();

        tmp_elem = root->add_son()->set_contents(11)->print_contents();
        tmp_elem->add_son()->set_contents(111)->print_contents();
        tmp_elem->add_son()->set_contents(112)->print_contents();
        tmp_elem->add_son()->set_contents(113)->print_contents();

        tmp_elem = root->add_son()->set_contents(2)->print_contents();
        tmp_elem->add_son()->set_contents(121)->print_contents();
        tmp_elem->add_son()->set_contents(122)->print_contents()->add_son()->set_contents(1221)->print_contents();

        tmp_elem = root->add_son()->set_contents(3)->print_contents();
        tmp_elem->add_son()->set_contents(131)->print_contents();
        tmp_elem->add_son()->set_contents(132)->print_contents();
        tmp_elem->add_son()->set_contents(133)->print_contents();

        root->get_number_of_sons(number);
        printf("Number of sons:%i\n",number);

        //  root->print_contents_recursive(0);

        root->calculate_sizes();

        root->dump_recursive();

        printf("ende\n");

        //***********************************************************

            AW_window_menu_modes *awm = (AW_window_menu_modes*)create_main_window(aw_root);
            awm->show();

            aw_root->awar_string( AWAR_FOOTER, "");

            aw_root->main_loop();


}


void
nt_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode)
{
    AWUSE(aws);
    char *text;
    static int story = 0;

    switch(mode){
        case AWT_MODE_MARK:
            text="MARK MODE    LEFT: mark subtree   RIGHT: unmark subtree";
            break;
        case AWT_MODE_GROUP:
            text="GROUP MODE   LEFT: group/ungroup group  RIGHT: group/ungroup subtree";
            break;
        case AWT_MODE_ZOOM:
            text="ZOOM MODE    LEFT: press and drag to zoom   RIGHT: zoom out one step";
            break;
        case AWT_MODE_LZOOM:
            text="LOGICAL ZOOM MODE   LEFT: show subtree  RIGHT: go up one step";
            break;
        case AWT_MODE_MOD:
            text="MODIFY MODE   LEFT: select node  RIGHT: assign info to internal node";
            break;
        case AWT_MODE_LINE:
            text="LINE MODE    LEFT: reduce linewidth  RIGHT: increase linewidth";
            break;
        case AWT_MODE_ROT:
            text="ROTATE MODE   LEFT: Select branch and drag to rotate  RIGHT: -";
            break;
        case AWT_MODE_SPREAD:
            text="SPREAD MODE   LEFT: decrease angles  RIGHT: increase angles";
            break;
        case AWT_MODE_SWAP:
            text="SWAP MODE    LEFT: swap branches  RIGHT: -";
            break;
        case AWT_MODE_LENGTH:
            text="BRANCH LENGTH MODE   LEFT: Select branch and drag to change length  RIGHT: -";
            break;
        case AWT_MODE_MOVE:
            text="MOVE MODE   LEFT: select subtree to move and drag to new destination  RIGHT: -";
            break;
        case AWT_MODE_NNI:
            text="NEAREST NEIGHBOUR INTERCHANGE OPTIMIZER  LEF: select subtree  RIGHT: whole tree";
            break;
        case AWT_MODE_KERNINGHAN:
            text="KERNIGHAN LIN OPTIMIZER   LEFT: select subtree RIGHT: whole tree";
            break;
        case AWT_MODE_OPTIMIZE:
            text="NNI & KL OPTIMIZER   LEFT: select subtree  RIGHT:whole tree";
            break;
        case AWT_MODE_SETROOT:
            text="SET ROOT MODE   LEFT: set root  RIGHT: -";
            break;
        case AWT_MODE_RESET:
            text="RESET MODE   LEFT: reset rotation  MIDDLE: reset angles  RIGHT: reset linewidth";
            break;

        default:
            text="No help for this mode available";
            break;
    }

    test_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    ntw->awr->awar(AWAR_FOOTER)->write_string( text);
    ntw->set_mode(mode);
}
