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

#include "GEN_local.hxx"
#include "GEN_map.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"
#include "../NTREE/ad_spec.hxx" // needed for species query window

using namespace std;

extern GBDATA *gb_main;

//  -----------------------------------------------------------------------
//      void GEN_create_genemap_awars(AW_root *aw_root,AW_default def)
//  -----------------------------------------------------------------------
void GEN_create_genemap_awars(AW_root *aw_root,AW_default def) {
    aw_root->awar_int(AWAR_GENMAP_DISPLAY_TYPE, GEN_DISPLAY_STYLE_RADIAL);

    aw_root->awar_int(AWAR_GENMAP_BOOK_BASES_PER_LINE, 20000);
    aw_root->awar_float(AWAR_GENMAP_BOOK_WIDTH_FACTOR, 0.1);
    aw_root->awar_int(AWAR_GENMAP_BOOK_LINE_HEIGHT, 20);
    aw_root->awar_int(AWAR_GENMAP_BOOK_LINE_SPACE, 5);

    aw_root->awar_float(AWAR_GENMAP_VERTICAL_FACTOR_X, 1.0);
    aw_root->awar_float(AWAR_GENMAP_VERTICAL_FACTOR_Y, 0.3);

    aw_root->awar_float(AWAR_GENMAP_RADIAL_INSIDE, 100);
    aw_root->awar_float(AWAR_GENMAP_RADIAL_OUTSIDE, 10);
}

//  --------------------------------------------------------------------------------------------
//      void GEN_gene_container_changed_cb(GBDATA *gb_gene_data, int *, GB_CB_TYPE gb_type)
//  --------------------------------------------------------------------------------------------
void GEN_gene_container_changed_cb(GBDATA *gb_gene_data, int *cl_ntw, GB_CB_TYPE gb_type) {
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;

    GEN_GRAPHIC->delete_gen_root(ntw);
    GEN_GRAPHIC->reinit_gen_root(ntw);

    ntw->refresh();
}

//  ----------------------------------------------------------------------------
//      void GEN_gene_container_cb_installer(bool install, AWT_canvas *ntw)
//  ----------------------------------------------------------------------------
void GEN_gene_container_cb_installer(bool install, AWT_canvas *ntw) {
    static GBDATA *last_added_gb_gene_data = 0;

    if (install) {
        GBDATA *gb_gene_data = GEN_get_current_gene_data(GEN_GRAPHIC->gb_main, GEN_GRAPHIC->aw_root);
        if (gb_gene_data) {
            GB_add_callback(gb_gene_data, (GB_CB_TYPE)(GB_CB_DELETE|GB_CB_CHANGED), GEN_gene_container_changed_cb, (int*)ntw);
            last_added_gb_gene_data = gb_gene_data;
        }
    }
    else {
        if (last_added_gb_gene_data) {
            GB_remove_callback(last_added_gb_gene_data, (GB_CB_TYPE)(GB_CB_DELETE|GB_CB_CHANGED), GEN_gene_container_changed_cb, (int*)ntw);
            last_added_gb_gene_data = 0;
        }
    }
}

//  ------------------------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  ------------------------------------------------------------------------
void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    GEN_GRAPHIC->reinit_gen_root(ntw);
    ntw->refresh();
}

//  ------------------------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  ------------------------------------------------------------------------
void GEN_gene_name_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    GEN_GRAPHIC->reinit_gen_root(ntw);
    ntw->refresh();
}

//  -------------------------------------------------------------------------
//      void GEN_display_param_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  -------------------------------------------------------------------------
void GEN_display_param_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    ntw->zoom_reset();
    ntw->refresh();
}

//  --------------------------------------------------------------------------------------
//      void GEN_add_awar_callbacks(AW_root *awr,AW_default /*def*/, AWT_canvas *ntw)
//  --------------------------------------------------------------------------------------
void GEN_add_awar_callbacks(AW_root *awr,AW_default /*def*/, AWT_canvas *ntw) {
    awr->awar_string(AWAR_SPECIES_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_species_name_changed_cb, (AW_CL)ntw);
    awr->awar_string(AWAR_GENE_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_gene_name_changed_cb, (AW_CL)ntw);

    awr->awar(AWAR_GENMAP_BOOK_BASES_PER_LINE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_BOOK_WIDTH_FACTOR)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_BOOK_LINE_HEIGHT)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_BOOK_LINE_SPACE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);

    awr->awar(AWAR_GENMAP_VERTICAL_FACTOR_X)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_VERTICAL_FACTOR_Y)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);

    awr->awar(AWAR_GENMAP_RADIAL_INSIDE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_RADIAL_OUTSIDE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
}

//  -------------------------------------------------------------------------------------
//      void GEN_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode)
//  -------------------------------------------------------------------------------------
void GEN_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode) {
    const char *text = 0;
    switch (mode) {
        case AWT_MODE_ZOOM: {
            text="ZOOM MODE    LEFT: drag to zoom   RIGHT: zoom out";
            break;
        }
        case AWT_MODE_MOD: {
            text="INFO MODE    LEFT: click for info";
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    aws->get_root()->awar("tmp/LeftFooter")->write_string( text);
    ntw->set_mode(mode);
    ntw->refresh();
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

//  ---------------------------------------------------------
//      void GEN_jump_cb(AW_window *aw, AWT_canvas *ntw)
//  ---------------------------------------------------------
void GEN_jump_cb(AW_window *aw, AWT_canvas *ntw) {
    // @@@ Center on selected gene
}

//  ----------------------------------------------------------
//      AW_window *GEN_create_layout_window(AW_root *awr)
//  ----------------------------------------------------------
AW_window *GEN_create_layout_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "SEC_LAYOUT", "SECEDIT Layout", 100, 100);
    aws->load_xfig("gene_layout.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"gen_layout.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("base_pos");        aws->create_input_field(AWAR_GENMAP_BOOK_BASES_PER_LINE, 15);
    aws->at("width_factor");    aws->create_input_field(AWAR_GENMAP_BOOK_WIDTH_FACTOR, 7);
    aws->at("line_height");     aws->create_input_field(AWAR_GENMAP_BOOK_LINE_HEIGHT, 5);
    aws->at("line_space");      aws->create_input_field(AWAR_GENMAP_BOOK_LINE_SPACE, 5);

    aws->at("factor_x");        aws->create_input_field(AWAR_GENMAP_VERTICAL_FACTOR_X, 5);
    aws->at("factor_y");        aws->create_input_field(AWAR_GENMAP_VERTICAL_FACTOR_Y, 5);

    aws->at("inside");          aws->create_input_field(AWAR_GENMAP_RADIAL_INSIDE, 5);
    aws->at("outside");         aws->create_input_field(AWAR_GENMAP_RADIAL_OUTSIDE, 5);

    return aws;
}

typedef enum  {
    GEN_MARK   = 1,
    GEN_UNMARK = 2,
    GEN_INVERT_MARKED = 3,

    GEN_PERFORM_ALL_SPECIES     = 4,
    GEN_PERFORM_CURRENT_SPECIES = 8,
    GEN_PERFORM_MARKED_SPECIES  = 12

} GEN_MARK_MODE;


//  --------------------------------------------------------------------------------------------
//      static void do_mark_command_for_one_species(GEN_MARK_MODE mode, GBDATA *gb_species)
//  --------------------------------------------------------------------------------------------
static void do_mark_command_for_one_species(GEN_MARK_MODE mode, GBDATA *gb_species) {
    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool mark_flag     = GB_read_flag(gb_gene) != 0;
        bool org_mark_flag = mark_flag;

        switch (mode&3) {
            case GEN_MARK:              mark_flag = 1; break;
            case GEN_UNMARK:            mark_flag = 0; break;
            case GEN_INVERT_MARKED:     mark_flag = !mark_flag; break;
            default: gen_assert(0); break;
        }

        if (mark_flag != org_mark_flag) {
            GB_write_flag(gb_gene, mark_flag?1:0);
        }
    }
}
//  ---------------------------------------------------------------------------
//      static void GEN_mark_command(AW_window *aww, AW_CL cl_mode, AW_CL)
//  ---------------------------------------------------------------------------
static void GEN_mark_command(AW_window *aww, AW_CL cl_mode, AW_CL) {
    GEN_MARK_MODE mode    = (GEN_MARK_MODE)cl_mode;
    GB_ERROR      error   = 0;

    GB_begin_transaction(gb_main);

    switch (mode&12) {
        case GEN_PERFORM_ALL_SPECIES: {
            for (GBDATA *gb_species = GBT_first_species(gb_main);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                do_mark_command_for_one_species(mode, gb_species);
            }
            break;
        }
        case GEN_PERFORM_MARKED_SPECIES: {
            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 gb_species;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                do_mark_command_for_one_species(mode, gb_species);
            }
            break;
        }
        case GEN_PERFORM_CURRENT_SPECIES: {
            AW_root *aw_root    = aww->get_root();
            GBDATA  *gb_species = GEN_get_current_species(gb_main, aw_root);

            if (!gb_species) {
                error = "First you have to select a species.";
            }
            else {
                do_mark_command_for_one_species(mode, gb_species);
            }
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    if (!error) GB_commit_transaction(gb_main);
    else GB_abort_transaction(gb_main);

    if (error) aw_message(error);
}

#define AWMIMT awm->insert_menu_topic

//  -------------------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_mark_menu(AW_window_menu_modes *awm, const char *macro_postfix, const char *menu_text_postfix, GEN_MARK_MODE range_mode)
//  -------------------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_mark_menu(AW_window_menu_modes *awm, const char *macro_postfix, const char *menu_text_postfix, GEN_MARK_MODE range_mode) {
    char macro_name_buffer[50];
    char menu_text_buffer[70];

    sprintf(macro_name_buffer, "gene_mark_%s", macro_postfix);
    sprintf(menu_text_buffer,   "Mark all genes    %s", menu_text_postfix);
    AWMIMT(macro_name_buffer, menu_text_buffer, "",	"gene_mark.hlp", AWM_ALL,GEN_mark_command, GEN_MARK|range_mode, 0);

    sprintf(macro_name_buffer, "gene_unmark_%s", macro_postfix);
    sprintf(menu_text_buffer,   "Unmark all genes  %s", menu_text_postfix);
    AWMIMT(macro_name_buffer, menu_text_buffer, "",	"gene_mark.hlp", AWM_ALL,GEN_mark_command, GEN_UNMARK|range_mode, 0);

    sprintf(macro_name_buffer, "gene_mark_%s", macro_postfix);
    sprintf(menu_text_buffer,   "Swap marked genes %s", menu_text_postfix);
    AWMIMT(macro_name_buffer, menu_text_buffer, "",	"gene_mark.hlp", AWM_ALL, GEN_mark_command, GEN_INVERT_MARKED|range_mode, 0);

    awm->insert_separator();
}

//  -------------------------------------------------------------------------------------
//      void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE)
//  -------------------------------------------------------------------------------------
void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE) {
    awm->create_menu(0,"Genes","G","genes.hlp",	AWM_ALL);
    {
        if (for_ARB_NTREE) {
            AWMIMT( "gene_map",	"Gene Map", "",	"gene_map.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_map, 0 );
            awm->insert_separator();
        }
        AWMIMT( "gene_info", 	"Info (Copy Delete Rename Modify) ...", 	"",	"gene_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_window,	0 );
        AWMIMT( "gene_search",	"Search and Query",			"",	"gene_search.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_query_window, 0 );

        awm->insert_separator();

        GEN_insert_mark_menu(awm, "curr",   "of current species", GEN_PERFORM_CURRENT_SPECIES);
        GEN_insert_mark_menu(awm, "marked", "of marked species", GEN_PERFORM_MARKED_SPECIES);
        GEN_insert_mark_menu(awm, "all",    "of all species", GEN_PERFORM_ALL_SPECIES);
    }
}

//  ----------------------------------------------------------------------------------------
//      void GEN_set_display_style(void *dummy, AWT_canvas *ntw, GEN_DisplayStyle type)
//  ----------------------------------------------------------------------------------------
void GEN_set_display_style(void *dummy, AWT_canvas *ntw, GEN_DisplayStyle type) {
    GEN_GRAPHIC->aw_root->awar(AWAR_GENMAP_DISPLAY_TYPE)->write_int(type);
    GEN_GRAPHIC->set_display_style(type);
    ntw->zoom_reset();
    ntw->refresh();
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
    GEN_GRAPHIC        = new GEN_graphic(awr, gb_main, GEN_gene_container_cb_installer);
    AWT_canvas    *ntw = new AWT_canvas(gb_main, awm, GEN_GRAPHIC, aw_gc_manager, AWAR_SPECIES_NAME);

    GEN_add_awar_callbacks(awr, AW_ROOT_DEFAULT, ntw);

    GEN_GRAPHIC->reinit_gen_root(ntw);

    ntw->recalc_size();
    ntw->refresh();
    ntw->set_mode(AWT_MODE_ZOOM); // Default-Mode

    // File Menu
    awm->create_menu( 0, "File", "F", "gene_file.hlp",  AWM_ALL );
    AWMIMT( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);

    // Genes
    GEN_create_genes_submenu(awm, false);

    // Properties Menu
    awm->create_menu("props","Properties","r","properties.hlp", AWM_ALL);
    AWMIMT("props_menu",	"Menu: Colors and Fonts ...",	"M","props_frame.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    AWMIMT("props_secedit",	"GENEMAP: Colors and Fonts ...","C","gene_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    AWMIMT("sec_layout", "Layout", "L", "gene_layout.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_layout_window, 0);
    AWMIMT("save_props",	"Save Defaults (in ~/.arb_prop/genemap.arb)",	"D","savedef.hlp",	AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

    // Create mode buttons
    awm->create_mode( 0, "zoom.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "info.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_MOD);

    awm->set_info_area_height( 250 );
    awm->at(5,2);
    awm->auto_space(-2,-2);
//     awm->auto_space(5,-2);
    awm->shadow_width(1);

    // close + undo button, info area, define line y-positions:

    int cur_x, cur_y, start_x, first_line_y, second_line_y, third_line_y;
    awm->get_at_position( &start_x,&first_line_y);

    awm->button_length(0);

    awm->at(cur_x, first_line_y);
    awm->help_text("quit.hlp");
    awm->callback((AW_CB0)AW_POPDOWN);
    awm->create_button("Close", "Close");

    awm->get_at_position( &cur_x,&cur_y );
    int gene_x = cur_x+10;

    awm->at_newline();
    awm->get_at_position( &cur_x,&second_line_y);

    awm->at(start_x, second_line_y);
    awm->help_text("undo.hlp");
    awm->callback((AW_CB)GEN_undo_cb,(AW_CL)ntw,(AW_CL)GB_UNDO_UNDO);
    awm->create_button("Undo", "Undo");

    awm->at_newline();
    awm->get_at_position( &cur_x,&third_line_y);

    awm->button_length(100);
    awm->create_button(0,"tmp/LeftFooter");

    awm->at_newline();
    awm->get_at_position( &cur_x,&cur_y );
    awm->set_info_area_height( cur_y+6 );

    // gene+species buttons:

    awm->button_length(20);

    awm->at(gene_x, first_line_y);
    awm->help_text("sp_search.hlp");
    awm->callback( AW_POPUP, (AW_CL)ad_create_query_window, 0);
    awm->create_button("SEARCH_SPECIES", AWAR_SPECIES_NAME);

    awm->at(gene_x, second_line_y);
    awm->help_text("gene_search.hlp");
    awm->callback( AW_POPUP, (AW_CL)GEN_create_gene_query_window, 0);
    awm->create_button("SEARCH_GENE",AWAR_GENE_NAME);

    awm->get_at_position( &cur_x,&cur_y );
    int dtype_x1 = cur_x+10;

    // display type buttons:

    awm->button_length(7);

    awm->at(dtype_x1, first_line_y);
    awm->help_text("gen_disp_radial.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)ntw,(AW_CL)GEN_DISPLAY_STYLE_RADIAL);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_radial.bitmap",0);

    awm->help_text("gen_disp_book.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)ntw,(AW_CL)GEN_DISPLAY_STYLE_BOOK);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_book.bitmap",0);

    awm->get_at_position( &cur_x,&cur_y );
    int jump_x = cur_x+10;

    awm->at(dtype_x1, second_line_y);
    awm->help_text("gen_disp_vertical.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)ntw,(AW_CL)GEN_DISPLAY_STYLE_VERTICAL);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_vertical.bitmap",0);

    // jump button:

    awm->button_length(0);

    awm->at(jump_x, first_line_y);
    awm->help_text("gen_jump.hlp");
    awm->callback((AW_CB)GEN_jump_cb,(AW_CL)ntw,(AW_CL)0);
    awm->create_button("Jump", "Jump");

    // help buttons:

    awm->get_at_position( &cur_x,&cur_y );
    int help_x = cur_x+10;

    awm->at(help_x, first_line_y);
    awm->help_text("help.hlp");
    awm->callback(AW_POPUP_HELP,(AW_CL)"gene_map.hlp");
    awm->create_button("HELP", "HELP","H");

    awm->at(help_x, second_line_y);
    awm->callback( AW_help_entry_pressed );
    awm->create_button(0,"?");

    return awm;
}

