//#define FINDCORR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>

#include <servercntrl.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>

#include <aw_preset.hxx>

AW_HEADER_MAIN

AW_window *create_matrix_window(AW_root *aw_root);
void create_matrix_variables(AW_root *aw_root, AW_default aw_def);
#ifdef FINDCORR
    AW_window *bc_create_main_window( AW_root *awr);
    void bc_create_bc_variables(AW_root *awr, AW_default awd);
#endif

GBDATA *gb_main;


// #if 0
//  awm->insert_menu_topic("base_correlation","base correlation ...","b","no help",AWM_ALL, AW_POPUP, (AW_CL)bc_create_main_window, 0);
// #endif


int main(int argc, char **argv)
{
    AW_root     *aw_root;
    AW_default   aw_default;
    AW_window   *aww;
    AWT_graphic *awd;

    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "Usage: arb_dist\n"
                "Is called from ARB.\n"
                );
        exit(-1);
    }

    aw_initstatus();
    aw_root    = new AW_root;
    aw_default = aw_root->open_default(".arb_prop/dist.arb");
    aw_root->init_variables(aw_default);
    aw_root->init("ARB_DIST");

    struct arb_params *params;
    params  = arb_trace_argv(&argc,argv);
    if (argc==2) {
        params->db_server = argv[1];
    }
    gb_main = GB_open(params->db_server,"rw");
    if (!gb_main) {
        aw_message(GB_get_error());
        fprintf(stderr,"%s\n",GB_get_error());
        exit(-1);
    }

    create_matrix_variables(aw_root,aw_default);
#ifdef FINDCORR
    bc_create_bc_variables(aw_root,aw_default);
#endif
    ARB_init_global_awars(aw_root, aw_default, gb_main);

    awd = (AWT_graphic *)0;
    aww = create_matrix_window(aw_root);


    aww->show();
    aw_root->main_loop();
    return 0;
}

void AD_map_viewer(GBDATA *dummy,AD_MAP_VIEWER_TYPE)
    {
    AWUSE(dummy);
}
