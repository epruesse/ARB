/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>

#include "GEN.hxx"
#include "GEN_map.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"

using namespace std;

extern GBDATA *gb_main;

//  -----------------------------------------------------------------------
//      void GEN_create_genemap_awars(AW_root *aw_root,AW_default def)
//  -----------------------------------------------------------------------
void GEN_create_genemap_awars(AW_root *aw_root,AW_default def) {

}

//  ------------------------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  ------------------------------------------------------------------------
void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    GEN_GRAPHIC->reinit_gen_root();
    ntw->refresh();
}

//  ------------------------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  ------------------------------------------------------------------------
void GEN_gene_name_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    GEN_GRAPHIC->reinit_gen_root();
    ntw->refresh();
}

//  --------------------------------------------------------------------------------------
//      void GEN_add_awar_callbacks(AW_root *awr,AW_default /*def*/, AWT_canvas *ntw)
//  --------------------------------------------------------------------------------------
void GEN_add_awar_callbacks(AW_root *awr,AW_default /*def*/, AWT_canvas *ntw) {
    awr->awar_string(AWAR_SPECIES_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_species_name_changed_cb, (AW_CL)ntw);;
    awr->awar_string(AWAR_GENE_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_gene_name_changed_cb, (AW_CL)ntw);;
    GEN_species_name_changed_cb(awr,ntw);
    GEN_gene_name_changed_cb(awr,ntw);
}

//  -------------------------------------------------------------------------------------
//      void GEN_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode)
//  -------------------------------------------------------------------------------------
void GEN_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode) {

}
//  --------------------------------------------------------------------------
//      void GEN_undo_cb(AW_window *aw, AWT_canvas *ntw, AW_CL undo_type)
//  --------------------------------------------------------------------------
void GEN_undo_cb(AW_window *aw, AWT_canvas *ntw, AW_CL undo_type) {
    AWUSE(aw);
    GB_ERROR error = GB_undo(gb_main,(GB_UNDO_TYPE)undo_type);
    if (error) {
        aw_message(error);
    }
    else{
        GB_begin_transaction(gb_main);
        GB_commit_transaction(gb_main);
        ntw->refresh();
    }
}

void GEN_jump_cb(AW_window *aw, AWT_canvas *ntw) {
}

//  ----------------------------------------------------------
//      AW_window *GEN_create_layout_window(AW_root *awr)
//  ----------------------------------------------------------
AW_window *GEN_create_layout_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "SEC_LAYOUT", "SECEDIT Layout", 100, 100);
    aws->load_xfig("gen_layout.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"gen_layout.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    return aws;
}

//  -----------------------------------------------------------
//      AW_window *GEN_map_create_main_window(AW_root *awr)
//  -----------------------------------------------------------
AW_window *GEN_map_create_main_window(AW_root *awr) {
    GB_transaction        dummy(gb_main);
    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(awr, "ARB_GENE_MAP", "ARB_GENE_MAP", 800, 600, 10, 10);

    GEN_create_genemap_awars(awr, AW_ROOT_DEFAULT);

    AW_gc_manager  aw_gc_manager;
    GEN_GRAPHIC        = new GEN_graphic(awr, gb_main);
    AWT_canvas    *ntw = new AWT_canvas(gb_main, awm, GEN_GRAPHIC, aw_gc_manager, AWAR_SPECIES_NAME);

    GEN_add_awar_callbacks(awr, AW_ROOT_DEFAULT, ntw);

    ntw->recalc_size();
    ntw->set_mode(AWT_MODE_ZOOM); // Default-Mode

    // File Menu
    awm->create_menu( 0, "File", "F", "gene_file.hlp",  AWM_ALL );
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);

    // Properties Menu
    awm->create_menu("props","Properties","r","properties.hlp", AWM_ALL);
    awm->insert_menu_topic("props_menu",	"Menu: Colors and Fonts ...",	"M","props_frame.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    awm->insert_menu_topic("props_secedit",	"GENEMAP: Colors and Fonts ...","C","gene_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    awm->insert_menu_topic("sec_layout", "Layout", "L", "gene_layout.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_layout_window, 0);
    awm->insert_menu_topic("save_props",	"Save Defaults (in ~/.arb_prop/genemap.arb)",	"D","savedef.hlp",	AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

    // Create mode buttons
    awm->create_mode( 0, "zoom.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "info.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_MOD);

    awm->set_info_area_height( 250 );
    awm->at(5,2);
    awm->auto_space(5,-2);

    int db_pathx,db_pathy;
    awm->get_at_position( &db_pathx,&db_pathy );

    awm->button_length(0);
    awm->help_text("quit.hlp");
    awm->callback((AW_CB0)AW_POPDOWN);
    awm->create_button("Close", "Close");

    awm->callback(AW_POPUP_HELP,(AW_CL)"arb_genemap.hlp");
    awm->button_length(0);
    awm->help_text("help.hlp");
    awm->create_button("HELP", "HELP","H");

    awm->callback( AW_help_entry_pressed );
    awm->create_button(0,"?");

    awm->callback((AW_CB)GEN_undo_cb,(AW_CL)ntw,(AW_CL)GB_UNDO_UNDO);
    awm->help_text("undo.hlp");
    awm->create_button("Undo", "Undo");

    awm->callback((AW_CB)GEN_jump_cb,(AW_CL)ntw,(AW_CL)0);
    awm->help_text("gen_jump.hlp");
    awm->create_button("Jump", "Jump");

    awm->button_length(100);
    awm->at_newline();
    awm->create_button(0,"tmp/LeftFooter");
    awm->at_newline();
    awm->get_at_position( &db_pathx,&db_pathy );
    awm->set_info_area_height( db_pathy+6 );

    return awm;
}

