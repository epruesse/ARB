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
#include <awtc_rename.hxx>
#include <awt_input_mask.hxx>

#include "GEN_local.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"
#include "GEN_nds.hxx"
#include "EXP.hxx"
#include "../NTREE/ad_spec.hxx" // needed for species query window

using namespace std;

extern GBDATA *gb_main;

//  ---------------------------------------------------------------------------
//      void GEN_NDS_changed(GBDATA *, int *cl_aw_root, gb_call_back_type)
//  ---------------------------------------------------------------------------
void GEN_NDS_changed(GBDATA *, int *, gb_call_back_type) {
    GEN_make_node_text_init(gb_main);
    GEN_GRAPHIC->gen_root->reinit_NDS();
}

//  -----------------------------------------------------------------------
//      void GEN_create_genemap_awars(AW_root *aw_root,AW_default def)
//  -----------------------------------------------------------------------
void GEN_create_genemap_awars(AW_root *aw_root,AW_default def) {

    // layout options:

    aw_root->awar_int(AWAR_GENMAP_DISPLAY_TYPE, GEN_DISPLAY_STYLE_RADIAL);

    aw_root->awar_int(AWAR_GENMAP_ARROW_SIZE, 150);
    aw_root->awar_int(AWAR_GENMAP_SHOW_HIDDEN, 0);
    aw_root->awar_int(AWAR_GENMAP_SHOW_ALL_NDS, 0);

    aw_root->awar_int(AWAR_GENMAP_BOOK_BASES_PER_LINE, 15000);
    aw_root->awar_float(AWAR_GENMAP_BOOK_WIDTH_FACTOR, 0.1);
    aw_root->awar_int(AWAR_GENMAP_BOOK_LINE_HEIGHT, 20);
    aw_root->awar_int(AWAR_GENMAP_BOOK_LINE_SPACE, 5);

    aw_root->awar_float(AWAR_GENMAP_VERTICAL_FACTOR_X, 1.0);
    aw_root->awar_float(AWAR_GENMAP_VERTICAL_FACTOR_Y, 0.3);

    aw_root->awar_float(AWAR_GENMAP_RADIAL_INSIDE, 50);
    aw_root->awar_float(AWAR_GENMAP_RADIAL_OUTSIDE, 4);

    // other options:

    aw_root->awar_int(AWAR_GENMAP_AUTO_JUMP, 1);

    GEN_create_nds_vars(aw_root, def, gb_main, GEN_NDS_changed);
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

//  ------------------------------------------------------------
//      void GEN_jump_cb(AW_window *dummy, AWT_canvas *ntw)
//  ------------------------------------------------------------
void GEN_jump_cb(AW_window */*dummy*/, AWT_canvas *ntw) {
    // @@@ Center on selected gene
    // analog NT_jump_cb
    GEN_GRAPHIC->change_flag = 1;
    ntw->refresh();
}

//  --------------------------------------------------------------
//      void GEN_jump_cb_auto(AW_root *root, AWT_canvas *ntw)
//  --------------------------------------------------------------
void GEN_jump_cb_auto(AW_root *root, AWT_canvas *ntw) {
    int jump = root->awar(AWAR_GENMAP_AUTO_JUMP)->read_int();
    if (jump) {
        AW_window *dummy = 0;
        GEN_jump_cb(dummy, ntw);
    }
}

//  -------------------------------------------------------------------------
//      void GEN_organism_name_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  -------------------------------------------------------------------------
void GEN_organism_name_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    GEN_GRAPHIC->reinit_gen_root(ntw);
    GEN_jump_cb_auto(awr, ntw);
    ntw->refresh();
}

//  ------------------------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw)
//  ------------------------------------------------------------------------
void GEN_gene_name_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    GEN_GRAPHIC->reinit_gen_root(ntw);
    GEN_jump_cb_auto(awr, ntw);
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
    awr->awar_string(AWAR_ORGANISM_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_organism_name_changed_cb, (AW_CL)ntw);
    awr->awar_string(AWAR_GENE_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_gene_name_changed_cb, (AW_CL)ntw);
    awr->awar_string(AWAR_COMBINED_GENE_NAME,"",gb_main);

    awr->awar(AWAR_GENMAP_ARROW_SIZE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_SHOW_HIDDEN)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);
    awr->awar(AWAR_GENMAP_SHOW_ALL_NDS)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)ntw);

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

//  -----------------------------------------------------------
//      AW_window *GEN_create_options_window(AW_root *awr)
//  -----------------------------------------------------------
AW_window *GEN_create_options_window(AW_root *awr) {
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *)aws;

	aws = new AW_window_simple;
	aws->init( awr, "GEN_OPTIONS", "GENE MAP OPTIONS", 10, 10 );
	aws->load_xfig("gene_options.fig");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"gene_options.hlp");
	aws->create_button("HELP","HELP","H");

	aws->at("button");
	aws->auto_space(10,10);
	aws->label_length(30);

    aws->label("Auto jump to selected gene");
    aws->create_toggle(AWAR_GENMAP_AUTO_JUMP);
    aws->at_newline();

	return (AW_window *)aws;
}

//  ----------------------------------------------------------
//      AW_window *GEN_create_layout_window(AW_root *awr)
//  ----------------------------------------------------------
AW_window *GEN_create_layout_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "GENE_LAYOUT", "Gene Map Layout", 100, 100);
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

    aws->at("arrow_size");      aws->create_input_field(AWAR_GENMAP_ARROW_SIZE, 5);

    aws->at("show_hidden");
    aws->label("Show hidden genes");
    aws->create_toggle(AWAR_GENMAP_SHOW_HIDDEN);

    aws->at("show_all");
    aws->label("Show NDS for all genes");
    aws->create_toggle(AWAR_GENMAP_SHOW_ALL_NDS);

    return aws;
}

typedef enum  {
    GEN_PERFORM_ALL_SPECIES,
    GEN_PERFORM_CURRENT_SPECIES,
    GEN_PERFORM_ALL_BUT_CURRENT_SPECIES,
    GEN_PERFORM_MARKED_SPECIES
} GEN_PERFORM_MODE;

typedef enum  {
    GEN_MARK,
    GEN_UNMARK,
    GEN_INVERT_MARKED,
    GEN_COUNT_MARKED,

    GEN_EXTRACT_MARKED,

    GEN_MARK_HIDDEN,
    GEN_MARK_VISIBLE,
    GEN_UNMARK_HIDDEN,
    GEN_UNMARK_VISIBLE
} GEN_MARK_MODE;

typedef enum  {
    GEN_HIDE_ALL,
    GEN_UNHIDE_ALL,
    GEN_INVERT_HIDE_ALL,

    GEN_HIDE_MARKED,
    GEN_UNHIDE_MARKED,
    GEN_INVERT_HIDE_MARKED
} GEN_HIDE_MODE;

//  ----------------------------------------------------------------------------------
//      inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data)
//  ----------------------------------------------------------------------------------
inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
}

//  ----------------------------------------------------------------------------------------------------
//      void GEN_extract_gene_2_pseudoSpecies(GBDATA *gb_species, GBDATA *gb_gene, const char *ali)
//  ----------------------------------------------------------------------------------------------------
void GEN_extract_gene_2_pseudoSpecies(GBDATA *gb_species, GBDATA *gb_gene, const char *ali) {
    GBDATA *gb_sp_name        = GB_find(gb_species,"name",0,down_level);
    GBDATA *gb_sp_fullname    = GB_find(gb_species,"full_name",0,down_level);
    char   *species_name      = gb_sp_name ? GB_read_string(gb_sp_name) : 0;
    char   *full_species_name = gb_sp_fullname ? GB_read_string(gb_sp_fullname) : species_name;
    GBDATA *gb_species_data   = GB_search(gb_main, "species_data",  GB_CREATE_CONTAINER);

    if (!species_name) {
        aw_message("Skipped species w/o name");
        return;
    }

    GBDATA *gb_ge_name = GB_find(gb_gene,"name",0,down_level);
    char   *gene_name  = gb_sp_name ? GB_read_string(gb_ge_name) : 0;

    if (!gene_name) {
        aw_message("Skipped gene w/o name");
        free(species_name);
        return;
    }

    char *full_name = GBS_strdup(GBS_global_string("%s [%s]", full_species_name, gene_name));

    char *sequence = GBT_read_gene_sequence(gb_gene, false);
    if (!sequence) {
        aw_message(GB_get_error());
    }
    else  {
        long id = GBS_checksum(sequence, 1, ".-");
        char acc[100];
        sprintf(acc, "ARB_GENE_%lX", id);

        char     *short_name = 0;
        GB_ERROR  error      = AWTC_generate_one_name(gb_main, full_name, acc, short_name, false);

        if (!error) {           // name was created
            if (!nameIsUnique(short_name, gb_species_data)) {
                char *uniqueName = AWTC_makeUniqueShortName(short_name, gb_species_data);
                free(short_name);
                short_name       = uniqueName;
                if (!short_name) error = "No short name created.";
            }
        }

        if (error) {            // try to make a random name
            error      = 0;
            short_name = AWTC_generate_random_name(gb_species_data);

            if (!short_name) {
                error = GBS_global_string("Failed to create a new name for pseudo gene-species '%s'", full_name);
            }
        }

        if (!error) {
            GBDATA *gb_new_species = GBT_create_species(gb_main, short_name);

            if (!gb_new_species) {
                error = GB_export_error("Failed to create pseudo-species '%s'", short_name);
            }

            if (!error) {
                GBDATA *gb_full_name    = GB_search(gb_new_species, "full_name", GB_STRING);
                if (gb_full_name) error = GB_write_string(gb_full_name, full_name);
                else    error           = GB_export_error("Can't create full_name-entry for %s", full_name);
            }

            if (!error) {
                GBDATA *gb_ali = GB_search(gb_new_species, ali, GB_DB);
                if (gb_ali) {
                    GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
                    error = GB_write_string(gb_data, sequence);
                }
                else {
                    error = GB_export_error("Can't create alignment '%s' for '%s'", ali, full_name);
                }
            }

            // the next two entries are used to identify the origin gene- and species-name:
            if (!error) {
                GBDATA *gb_species_origin    = GB_search(gb_new_species, "ARB_origin_species", GB_STRING);
                if (gb_species_origin) error = GB_write_string(gb_species_origin, species_name);
                else    error                = GB_export_error("Can't create ARB_origin_species-entry for %s", full_name);
            }

            if (!error) {
                GBDATA *gb_gene_origin    = GB_search(gb_new_species, "ARB_origin_gene", GB_STRING);
                if (gb_gene_origin) error = GB_write_string(gb_gene_origin, gene_name);
                else    error             = GB_export_error("Can't create ARB_origin_gene-entry for %s", full_name);
            }


        }

        if (error) aw_message(error);

        free(short_name);
        free(sequence);
    }

    free(full_name);
    free(gene_name);
    free(full_species_name);
    free(species_name);
}

static long gen_count_marked_genes = 0; // used to count marked genes

//  --------------------------------------------------------------------------------------------------
//      static void do_mark_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user)
//  --------------------------------------------------------------------------------------------------
static void do_mark_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user) {
    GEN_MARK_MODE mode = (GEN_MARK_MODE)imode;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool mark_flag     = GB_read_flag(gb_gene) != 0;
        bool org_mark_flag = mark_flag;

        switch (mode) {
            case GEN_MARK:              mark_flag = 1; break;
            case GEN_UNMARK:            mark_flag = 0; break;
            case GEN_INVERT_MARKED:     mark_flag = !mark_flag; break;
            case GEN_COUNT_MARKED:     if (mark_flag) ++gen_count_marked_genes; break;
            case GEN_EXTRACT_MARKED: {
                if (mark_flag) GEN_extract_gene_2_pseudoSpecies(gb_species, gb_gene, (const char *)cl_user);
                break;
            }
            default: {
                GBDATA *gb_hidden = GB_find(gb_gene, "display_hidden", 0, down_level);
                bool    hidden    = gb_hidden ? GB_read_byte(gb_hidden) != 0 : false;

                switch (mode) {
                    case GEN_MARK_HIDDEN:       if (hidden) mark_flag = 1; break;
                    case GEN_UNMARK_HIDDEN:     if (hidden) mark_flag = 0; break;
                    case GEN_MARK_VISIBLE:      if (!hidden) mark_flag = 1; break;
                    case GEN_UNMARK_VISIBLE:    if (!hidden) mark_flag = 0; break;
                    default: gen_assert(0); break;
                }
            }
        }

        if (mark_flag != org_mark_flag) {
            GB_write_flag(gb_gene, mark_flag?1:0);
        }
    }
}

//  --------------------------------------------------------------------------------------------------
//      static void do_hide_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user)
//  --------------------------------------------------------------------------------------------------
static void do_hide_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user) {
    GEN_HIDE_MODE mode = (GEN_HIDE_MODE)imode;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool marked = GB_read_flag(gb_gene) != 0;

        GBDATA *gb_hidden  = GB_find(gb_gene, "display_hidden", 0, down_level);
        bool    hidden     = gb_hidden ? (GB_read_byte(gb_hidden) != 0) : false;
        bool    org_hidden = hidden;

        switch (mode) {
            case GEN_HIDE_ALL:              hidden = true; break;
            case GEN_UNHIDE_ALL:            hidden = false; break;
            case GEN_INVERT_HIDE_ALL:       hidden = !hidden; break;
            case GEN_HIDE_MARKED:           if (marked) hidden = true; break;
            case GEN_UNHIDE_MARKED:         if (marked) hidden = false; break;
            case GEN_INVERT_HIDE_MARKED:    if (marked) hidden = !hidden; break;
            default: gen_assert(0); break;
        }

        if (hidden != org_hidden) {
            if (!gb_hidden) gb_hidden = GB_create(gb_gene, "display_hidden", GB_BYTE);
            GB_write_byte(gb_hidden, hidden ? 1 : 0); // change gene visibility
        }
    }
}

//  --------------------------------------------------------------------------------
//      static void  GEN_perform_command(AW_window *aww, GEN_PERFORM_MODE pmode,
//  --------------------------------------------------------------------------------
static void  GEN_perform_command(AW_window *aww, GEN_PERFORM_MODE pmode,
                                 void (*do_command)(int cmode, GBDATA *gb_species, AW_CL cl_user), int mode, AW_CL cl_user) {
    GB_ERROR error = 0;

    GB_begin_transaction(gb_main);

    switch (pmode) {
        case GEN_PERFORM_ALL_SPECIES: {
            for (GBDATA *gb_species = GBT_first_species(gb_main);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                do_command(mode, gb_species, cl_user);
            }
            break;
        }
        case GEN_PERFORM_MARKED_SPECIES: {
            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 gb_species;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                do_command(mode, gb_species, cl_user);
            }
            break;
        }
        case GEN_PERFORM_ALL_BUT_CURRENT_SPECIES: {
            AW_root *aw_root    = aww->get_root();
            GBDATA  *gb_curr_species = GEN_get_current_organism(gb_main, aw_root);

            for (GBDATA *gb_species = GBT_first_species(gb_main);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                if (gb_species != gb_curr_species) do_command(mode, gb_species, cl_user);
            }
            break;
        }
        case GEN_PERFORM_CURRENT_SPECIES: {
            AW_root *aw_root    = aww->get_root();
            GBDATA  *gb_species = GEN_get_current_organism(gb_main, aw_root);

            if (!gb_species) {
                error = "First you have to select a species.";
            }
            else {
                do_command(mode, gb_species, cl_user);
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
//  -------------------------------------------------------------------------------------
//      static void GEN_hide_command(AW_window *aww, AW_CL cl_pmode, AW_CL cl_hmode)
//  -------------------------------------------------------------------------------------
static void GEN_hide_command(AW_window *aww, AW_CL cl_pmode, AW_CL cl_hmode) {
    GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_hide_command_for_one_species, cl_hmode, 0);
}
//  -------------------------------------------------------------------------------------
//      static void GEN_mark_command(AW_window *aww, AW_CL cl_pmode, AW_CL cl_mmode)
//  -------------------------------------------------------------------------------------
static void GEN_mark_command(AW_window *aww, AW_CL cl_pmode, AW_CL cl_mmode) {
    gen_count_marked_genes = 0;
    GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_mark_command_for_one_species, cl_mmode, 0);

    if ((GEN_MARK_MODE)cl_mmode == GEN_COUNT_MARKED) {
        const char *where = 0;

        switch ((GEN_PERFORM_MODE)cl_pmode) {
            case GEN_PERFORM_CURRENT_SPECIES:           where = "the current species"; break;
            case GEN_PERFORM_MARKED_SPECIES:            where = "all marked species"; break;
            case GEN_PERFORM_ALL_SPECIES:               where = "all species"; break;
            case GEN_PERFORM_ALL_BUT_CURRENT_SPECIES:   where = "all but the current species"; break;
            default: gen_assert(0); break;
        }
        aw_message(GBS_global_string("There are %i marked genes in %s", gen_count_marked_genes, where));
    }
}

//  ------------------------------------------------------------
//      void gene_extract_cb(AW_window *aww, AW_CL cl_pmode)
//  ------------------------------------------------------------
void gene_extract_cb(AW_window *aww, AW_CL cl_pmode){
    char     *ali   = aww->get_root()->awar(AWAR_GENE_EXTRACT_ALI)->read_string();
    GB_ERROR  error = GBT_check_alignment_name(ali);

    if (!error) {
        GB_transaction dummy(gb_main);
        if (!GBT_create_alignment(gb_main,ali,0,0,0,"dna")) {
            error = GB_get_error();
        }
    }

    if (error) {
        aw_message(error);
    }
    else {
        aw_openstatus("Extracting pseudo-species");
        GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_mark_command_for_one_species, GEN_EXTRACT_MARKED, (AW_CL)ali);
        aw_closestatus();
    }
    free(ali);
}

//  ----------------------------------------------------------------------
//      GBDATA *GEN_find_pseudo(GBDATA *gb_organism, GBDATA *gb_gene)
//  ----------------------------------------------------------------------
GBDATA *GEN_find_pseudo(GBDATA *gb_organism, GBDATA *gb_gene) {
    GBDATA *gb_species_data = GB_get_father(gb_organism);
    GBDATA *gb_name         = GB_find(gb_organism, "name", 0, down_level);
    char   *organism_name   = GB_read_string(gb_name);
    gb_name                 = GB_find(gb_gene, "name", 0, down_level);
    char   *gene_name       = GB_read_string(gb_name);
    GBDATA *gb_pseudo       = 0;

    for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        const char *this_organism_name = GEN_origin_organism(gb_species);

        if (this_organism_name && strcmp(this_organism_name, organism_name) == 0)
        {
            if (strcmp(GEN_origin_gene(gb_species), gene_name) == 0)
            {
                gb_pseudo = gb_species;
                break;
            }
        }
    }

    return gb_pseudo;
}

//  -------------------------------------------------------------------------
//      static void mark_organisms(AW_window *aww, AW_CL cl_mark, AW_CL)
//  -------------------------------------------------------------------------
// cl_mark == 0 -> unmark
// cl_mark == 1 -> mark
// cl_mark == 2 -> invert mark
static void mark_organisms(AW_window *aww, AW_CL cl_mark, AW_CL) {
    GB_transaction dummy(gb_main);
    int            mark = (int)cl_mark;

    for (GBDATA *gb_org = GEN_first_organism(gb_main);
         gb_org;
         gb_org = GEN_next_organism(gb_org))
    {
        if (mark == 2) {
            GB_write_flag(gb_org, !GB_read_flag(gb_org)); // invert mark of organism
        }
        else {
            GB_write_flag(gb_org, mark); // mark/unmark organism
        }
    }
}
//  ----------------------------------------------------------------------------
//      static void mark_gene_species(AW_window *aww, AW_CL cl_mark, AW_CL)
//  ----------------------------------------------------------------------------
// cl_mark == 0 -> unmark
// cl_mark == 1 -> mark
// cl_mark == 2 -> invert mark
static void mark_gene_species(AW_window *aww, AW_CL cl_mark, AW_CL) {
    GB_transaction dummy(gb_main);
    int            mark = (int)cl_mark;

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        if (mark == 2) {
            GB_write_flag(gb_pseudo, !GB_read_flag(gb_pseudo)); // invert mark of pseudo-species
        }
        else {
            GB_write_flag(gb_pseudo, mark); // mark/unmark gene-species
        }
    }
}
//  ------------------------------------------------------------------------------------
//      static void mark_gene_species_of_marked_genes(AW_window *aww, AW_CL, AW_CL)
//  ------------------------------------------------------------------------------------
static void mark_gene_species_of_marked_genes(AW_window *aww, AW_CL, AW_CL) {
    GB_transaction dummy(gb_main);

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        for (GBDATA *gb_gene = GEN_first_gene(gb_species);
             gb_gene;
             gb_gene = GEN_next_gene(gb_gene))
        {
            if (GB_read_flag(gb_gene)) {
                GBDATA *gb_pseudo = GEN_find_pseudo(gb_species, gb_gene);
                if (gb_pseudo) GB_write_flag(gb_pseudo, 1);
            }
        }
    }
}
//  ------------------------------------------------------------------------------------
//      static void mark_genes_of_marked_gene_species(AW_window *aww, AW_CL, AW_CL)
//  ------------------------------------------------------------------------------------
static void mark_genes_of_marked_gene_species(AW_window *aww, AW_CL, AW_CL) {
    GB_transaction dummy(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        if (GB_read_flag(gb_pseudo)) {
            GBDATA *gb_gene = GEN_find_origin_gene(gb_pseudo);
            GB_write_flag(gb_gene, 1); // mark gene
        }
    }
}

//  ----------------------------------------------------------------------------
//      AW_window *create_gene_extract_window(AW_root *root, AW_CL cl_pmode)
//  ----------------------------------------------------------------------------
AW_window *create_gene_extract_window(AW_root *root, AW_CL cl_pmode)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "EXTRACT_GENE", "Extract genes to alignment", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_button(0,"Please enter the name\nof the alignment to extract to");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_EXTRACT_ALI,15);

    aws->at("ok");
    aws->callback(gene_extract_cb, cl_pmode);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

#define AWMIMT awm->insert_menu_topic

//  -------------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_extract_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
//  -------------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_extract_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                                const char *help_file) {
//     GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_extract_marked_command, (AW_CL)mark_mode);
    awm->insert_sub_menu(0, submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of current species...", "C", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_CURRENT_SPECIES);

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    AWMIMT(macro_name_buffer, "of marked species...", "M", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_MARKED_SPECIES);

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    AWMIMT(macro_name_buffer, "of all species...", "A", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_ALL_SPECIES);

    awm->close_sub_menu();
}

//  -----------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_multi_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
//  -----------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_multi_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                              const char *help_file, void (*command)(AW_window*, AW_CL, AW_CL), AW_CL command_mode) {
    awm->insert_sub_menu(0, submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of current species", "C", help_file, AWM_ALL, command, GEN_PERFORM_CURRENT_SPECIES, command_mode);

    sprintf(macro_name_buffer, "%s_of_all_but_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of all but current species", "C", help_file, AWM_ALL, command, GEN_PERFORM_ALL_BUT_CURRENT_SPECIES, command_mode);

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    AWMIMT(macro_name_buffer, "of marked species", "M", help_file, AWM_ALL, command, GEN_PERFORM_MARKED_SPECIES, command_mode);

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    AWMIMT(macro_name_buffer, "of all species", "A", help_file, AWM_ALL, command, GEN_PERFORM_ALL_SPECIES, command_mode);

    awm->close_sub_menu();
}
//  ----------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_mark_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
//  ----------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_mark_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                             const char *help_file, GEN_MARK_MODE mark_mode) {
    GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_mark_command, (AW_CL)mark_mode);
}
//  ----------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_hide_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
//  ----------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_hide_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                             const char *help_file, GEN_HIDE_MODE hide_mode) {
    GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_hide_command, (AW_CL)hide_mode);
}

#if defined(DEBUG)
AW_window *GEN_create_awar_debug_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "DEBUG_AWARS", "DEBUG AWARS", 10, 10);
        aws->at(10, 10);
        aws->auto_space(10,10);

        ;                  aws->label("AWAR_SPECIES_NAME      "); aws->create_input_field(AWAR_SPECIES_NAME, 30);
        aws->at_newline(); aws->label("AWAR_ORGANISM_NAME     "); aws->create_input_field(AWAR_ORGANISM_NAME, 30);
        aws->at_newline(); aws->label("AWAR_GENE_NAME         "); aws->create_input_field(AWAR_GENE_NAME, 30);
        aws->at_newline(); aws->label("AWAR_COMBINED_GENE_NAME"); aws->create_input_field(AWAR_COMBINED_GENE_NAME, 30);
        aws->at_newline(); aws->label("AWAR_EXPERIMENT_NAME   "); aws->create_input_field(AWAR_EXPERIMENT_NAME, 30);

        aws->window_fit();
    }
    return aws;
}
#endif // DEBUG
// #########################################
// #########################################
// ###                                   ###
// ##          user mask section          ##
// ###                                   ###
// #########################################
// #########################################

//  -----------------------------------------------------------------------------
//      class GEN_item_type_species_selector : public awt_item_type_selector
//  -----------------------------------------------------------------------------
class GEN_item_type_species_selector : public awt_item_type_selector {
public:
    GEN_item_type_species_selector() : awt_item_type_selector(AWT_IT_GENE) {}
    virtual ~GEN_item_type_species_selector() {}

    virtual const char *get_self_awar() const {
        return AWAR_COMBINED_GENE_NAME;
    }
    virtual size_t get_self_awar_content_length() const {
        return 12 + 1 + 40; // species-name+'/'+gene_name
    }
    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const { // add callbacks to awars
        root->awar(get_self_awar())->add_callback(f, cl_mask);
    }
    virtual void remove_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const  {
        root->awar(get_self_awar())->remove_callback(f, cl_mask);
    }
    virtual GBDATA *current(AW_root *root) const { // give the current item
        char   *species_name = root->awar(AWAR_ORGANISM_NAME)->read_string();
        char   *gene_name   = root->awar(AWAR_GENE_NAME)->read_string();
        GBDATA *gb_gene      = 0;

        if (species_name[0] && gene_name[0]) {
            GB_transaction dummy(gb_main);
            GBDATA *gb_species = GBT_find_species(gb_main,species_name);
            if (gb_species) {
                gb_gene = GEN_find_gene(gb_species, gene_name);
            }
        }

        free(gene_name);
        free(species_name);

        return gb_gene;
    }
    virtual const char *getKeyPath() const { // give the keypath for items
        return CHANGE_KEY_PATH_GENES;
    }
};

static GEN_item_type_species_selector item_type_gene;

//  -----------------------------------------------------------------------------
//      static void GEN_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL)
//  -----------------------------------------------------------------------------
static void GEN_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL) {
    int                              id         = int(cl_id);
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    gen_assert(descriptor);
    if (descriptor) AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_gene, descriptor->get_internal_maskname(), descriptor->is_local_mask());
}

//  -----------------------------------------------------------------------
//      static void GEN_create_mask_submenu(AW_window_menu_modes *awm)
//  -----------------------------------------------------------------------
static void GEN_create_mask_submenu(AW_window_menu_modes *awm) {
    AWT_create_mask_submenu(awm, AWT_IT_GENE, GEN_open_mask_window);
}
//  ----------------------------------------------------------------------------------
//      void GEN_create_organism_submenu(AW_window_menu_modes *awm, bool submenu)
//  ----------------------------------------------------------------------------------
void GEN_create_organism_submenu(AW_window_menu_modes *awm, bool submenu) {
    const char *title  = "Organisms";
    const char *hotkey = "O";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT( "organism_info", 	"Organism Info ...", 	"",	"organism_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)NT_create_organism_window,	0 );

        awm->insert_separator();

        AWMIMT("mark_organisms", "Mark all organisms", "A", "gene_mark.hlp", AWM_ALL, mark_organisms, 1, 0);
        AWMIMT("unmark_organisms", "Unmark all organisms", "U", "gene_mark.hlp", AWM_ALL, mark_organisms, 0, 0);
        AWMIMT("invmark_organisms", "Invert marks of all organisms", "I", "gene_mark.hlp", AWM_ALL, mark_organisms, 2, 0);
    }
    if (submenu) awm->close_sub_menu();
}
//  --------------------------------------------------------------------------------------------
//      void GEN_create_gene_species_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE)
//  --------------------------------------------------------------------------------------------
void GEN_create_gene_species_submenu(AW_window_menu_modes *awm, bool submenu) {
    const char *title  = "Gene-Species";
    const char *hotkey = "E";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT("mark_gene_species", "Mark all gene-species", "A", "gene_mark.hlp", AWM_ALL, mark_gene_species, 1, 0);
        AWMIMT("unmark_gene_species", "Unmark all gene-species", "U", "gene_mark.hlp", AWM_ALL, mark_gene_species, 0, 0);
        AWMIMT("invmark_gene_species", "Invert marks of all gene-species", "U", "gene_mark.hlp", AWM_ALL, mark_gene_species, 2, 0);
        awm->insert_separator();
        AWMIMT("mark_gene_species_of_marked_genes", "Mark gene-species of marked genes", "S", "gene_mark.hlp", AWM_ALL, mark_gene_species_of_marked_genes, 0, 0);
    }

    if (submenu) awm->close_sub_menu();
}


//  -------------------------------------------------------------------------------------
//      void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE)
//  -------------------------------------------------------------------------------------
void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE) {
    awm->create_menu(0,"Genes","G","no.hlp",	AWM_ALL);
    {
#if defined(DEBUG)
        AWMIMT("debug_awars", "Show Main AWARS", "", "no.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_awar_debug_window, 0);
        awm->insert_separator();
#endif // DEBUG

        if (for_ARB_NTREE) {
            AWMIMT( "gene_map",	"Gene Map", "",	"gene_map.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_map, 0 );
            awm->insert_separator();

            GEN_create_gene_species_submenu(awm, true); // Gene-species
            GEN_create_organism_submenu(awm, true); // Organisms
            EXP_create_experiments_submenu(awm, true); // Experiments
            awm->insert_separator();
        }
        AWMIMT( "gene_info", 	"Info (Copy Delete Rename Modify) ...", 	"",	"gene_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_window,	0 );
        AWMIMT( "gene_search",	"Search and Query",			"",	"gene_search.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_query_window, 0 );

        GEN_create_mask_submenu(awm);

        awm->insert_separator();

        GEN_insert_mark_submenu(awm, "gene_mark_all", "Mark all genes", "M", "gene_mark.hlp",  GEN_MARK);
        GEN_insert_mark_submenu(awm, "gene_unmark_all", "Unmark all genes", "U", "gene_mark.hlp", GEN_UNMARK);
        GEN_insert_mark_submenu(awm, "gene_invert_marked", "Invert marked genes", "I", "gene_mark.hlp", GEN_INVERT_MARKED);
        GEN_insert_mark_submenu(awm, "gene_count_marked", "Count marked genes", "I", "gene_mark.hlp", GEN_COUNT_MARKED);

        awm->insert_separator();

        AWMIMT("mark_genes_of_marked_gene_species", "Mark genes of marked gene-species", "G", "gene_mark.hlp", AWM_ALL, mark_genes_of_marked_gene_species, 0, 0);

        awm->insert_separator();

        GEN_insert_extract_submenu(awm, "gene_extract_marked", "Extract marked genes to alignment", "I", "gene_extract.hlp");

        if (!for_ARB_NTREE) {   // only in ARB_GENE_MAP:
            awm->insert_separator();
            GEN_insert_mark_submenu(awm, "gene_mark_hidden", "Mark hidden genes", "", "gene_hide.hlp", GEN_MARK_HIDDEN);
            GEN_insert_mark_submenu(awm, "gene_mark_visible", "Mark visible genes", "", "gene_hide.hlp", GEN_MARK_VISIBLE);

            awm->insert_separator();
            GEN_insert_mark_submenu(awm, "gene_unmark_hidden", "Unmark hidden genes", "", "gene_hide.hlp", GEN_UNMARK_HIDDEN);
            GEN_insert_mark_submenu(awm, "gene_unmark_visible", "Unmark visible genes", "", "gene_hide.hlp", GEN_UNMARK_VISIBLE);
        }
    }
}


//  ----------------------------------------------------------------
//      void GEN_create_hide_submenu(AW_window_menu_modes *awm)
//  ----------------------------------------------------------------
void GEN_create_hide_submenu(AW_window_menu_modes *awm) {
    awm->create_menu(0,"Hide","H","no.hlp",	AWM_ALL);
    {
        GEN_insert_hide_submenu(awm, "gene_hide_marked", "Hide marked genes", "H", "gene_hide.hlp", GEN_HIDE_MARKED);
        GEN_insert_hide_submenu(awm, "gene_unhide_marked", "Unhide marked genes", "U", "gene_hide.hlp", GEN_UNHIDE_MARKED);
        GEN_insert_hide_submenu(awm, "gene_invhide_marked", "Invert-hide marked genes", "I", "gene_hide.hlp", GEN_INVERT_HIDE_MARKED);

        awm->insert_separator();
        GEN_insert_hide_submenu(awm, "gene_hide_all", "Hide all genes", "", "gene_hide.hlp", GEN_HIDE_ALL);
        GEN_insert_hide_submenu(awm, "gene_unhide_all", "Unhide all genes", "", "gene_hide.hlp", GEN_UNHIDE_ALL);
        GEN_insert_hide_submenu(awm, "gene_invhide_all", "Invert-hide all genes", "", "gene_hide.hlp", GEN_INVERT_HIDE_ALL);
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

// ##########################################
// ##########################################
// ###                                    ###
// ##          create main window          ##
// ###                                    ###
// ##########################################
// ##########################################

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
    awm->create_menu( 0, "File", "F", "no.hlp",  AWM_ALL );
    AWMIMT( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);

    GEN_create_genes_submenu(awm, false); // Genes
    GEN_create_gene_species_submenu(awm, false); // Gene-species
    GEN_create_organism_submenu(awm, false); // Organisms

    EXP_create_experiments_submenu(awm, false); // Experiments

    GEN_create_hide_submenu(awm); // Hide Menu

    // Properties Menu
    awm->create_menu("props","Properties","r","no.hlp", AWM_ALL);
    AWMIMT("gene_props_menu",	"Menu: Colors and Fonts ...",	"M","props_frame.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    AWMIMT("gene_props",	"GENEMAP: Colors and Fonts ...","C","gene_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    AWMIMT("gene_layout", "Layout", "L", "gene_layout.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_layout_window, 0);
    AWMIMT("gene_options", "Options", "L", "gene_options.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_options_window, 0);
    AWMIMT(	"gene_nds",		"NDS ( Select Gene Information ) ...",		"N","props_nds.hlp",	AWM_ALL, AW_POPUP, (AW_CL)GEN_open_nds_window, (AW_CL)gb_main );
    AWMIMT("gene_save_props",	"Save Defaults (in ~/.arb_prop/ntree.arb)",	"D","savedef.hlp",	AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

    // Create mode buttons
    awm->create_mode( 0, "zoom.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "info.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_MOD);

    awm->set_info_area_height( 250 );
    awm->at(11,2);
    awm->auto_space(2,-2);
    awm->shadow_width(1);

    // close + undo button, info area, define line y-positions:

    int cur_x, cur_y, start_x, first_line_y, second_line_y, third_line_y;
    awm->get_at_position( &start_x,&first_line_y);
    awm->button_length(6);

    awm->at(start_x, first_line_y);
    awm->help_text("quit.hlp");
    awm->callback((AW_CB0)AW_POPDOWN);
    awm->create_button("Close", "Close");

    awm->get_at_position( &cur_x,&cur_y );

    int gene_x = cur_x;
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
    awm->help_text("organism_search.hlp");
    awm->callback( AW_POPUP, (AW_CL)ad_create_query_window, 0); // @@@ hier sollte eine Art "Organism-Search" verwendet werden (AWT_organism_selector anpassen)
    awm->create_button("SEARCH_ORGANISM", AWAR_ORGANISM_NAME);

    awm->at(gene_x, second_line_y);
    awm->help_text("gene_search.hlp");
    awm->callback( AW_POPUP, (AW_CL)GEN_create_gene_query_window, 0);
    awm->create_button("SEARCH_GENE",AWAR_GENE_NAME);

    awm->get_at_position( &cur_x,&cur_y );
    int dtype_x1 = cur_x;

    // display type buttons:

    awm->button_length(4);

    awm->at(dtype_x1, first_line_y);
    awm->help_text("gen_disp_radial.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)ntw,(AW_CL)GEN_DISPLAY_STYLE_RADIAL);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_radial.bitmap",0);

    awm->help_text("gen_disp_book.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)ntw,(AW_CL)GEN_DISPLAY_STYLE_BOOK);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_book.bitmap",0);

    awm->get_at_position( &cur_x,&cur_y );
    int jump_x = cur_x;

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
    int help_x = cur_x;

    awm->at(help_x, first_line_y);
    awm->help_text("help.hlp");
    awm->callback(AW_POPUP_HELP,(AW_CL)"gene_map.hlp");
    awm->create_button("HELP", "HELP","H");

    awm->at(help_x, second_line_y);
    awm->callback( AW_help_entry_pressed );
    awm->create_button(0,"?");

    return awm;
}

//  ----------------------------------------------
//      AW_window *GEN_map(AW_root *aw_root)
//  ----------------------------------------------
AW_window *GEN_map(AW_root *aw_root) {
    static AW_window *aw_gen = 0;

    if (!aw_gen) {              // do not open window twice
        {
            GB_transaction dummy(gb_main);
            GEN_make_node_text_init(gb_main);
        }

        aw_gen = GEN_map_create_main_window(aw_root);
        if (!aw_gen) {
            aw_message("Couldn't start Gene-Map");
            return 0;
        }

    }

    aw_gen->show();
    return aw_gen;
}

