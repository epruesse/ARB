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
#include <aw_question.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>
#include <awtc_rename.hxx>
#include <awt_input_mask.hxx>

#include "GEN_local.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"
#include "GEN_nds.hxx"
#include "GEN_interface.hxx"
#include "EXP.hxx"
#include "EXP_interface.hxx"
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

//  --------------------------------------------------------------------------------------------------
//      void GEN_gene_container_changed_cb(GBDATA *gb_gene_data, int *cl_gmw, GB_CB_TYPE gb_type)
//  --------------------------------------------------------------------------------------------------
void GEN_gene_container_changed_cb(GBDATA */*gb_gene_data*/, int *cl_gmw, GB_CB_TYPE /*gb_type*/) {
    AWT_canvas *gmw = (AWT_canvas*)cl_gmw;

    GEN_GRAPHIC->delete_gen_root(gmw);
    GEN_GRAPHIC->reinit_gen_root(gmw);

    gmw->refresh();
}

//  ----------------------------------------------------------------------------
//      void GEN_gene_container_cb_installer(bool install, AWT_canvas *ntw)
//  ----------------------------------------------------------------------------
void GEN_gene_container_cb_installer(bool install, AWT_canvas *gmw) {
    static GBDATA *last_added_gb_gene_data = 0;

    if (install) {
        GBDATA *gb_gene_data = GEN_get_current_gene_data(GEN_GRAPHIC->gb_main, GEN_GRAPHIC->aw_root);
        if (gb_gene_data) {
            GB_add_callback(gb_gene_data, (GB_CB_TYPE)(GB_CB_DELETE|GB_CB_CHANGED), GEN_gene_container_changed_cb, (int*)gmw);
            last_added_gb_gene_data = gb_gene_data;
        }
    }
    else {
        if (last_added_gb_gene_data) {
            GB_remove_callback(last_added_gb_gene_data, (GB_CB_TYPE)(GB_CB_DELETE|GB_CB_CHANGED), GEN_gene_container_changed_cb, (int*)gmw);
            last_added_gb_gene_data = 0;
        }
    }
}

//  ------------------------------------------------------------
//      void GEN_jump_cb(AW_window *dummy, AWT_canvas *gmw)
//  ------------------------------------------------------------
void GEN_jump_cb(AW_window */*dummy*/, AWT_canvas *gmw) {
    // @@@ Center on selected gene
    // analog NT_jump_cb
    GEN_GRAPHIC->change_flag = 1;
    gmw->refresh();
}

//  --------------------------------------------------------------
//      void GEN_jump_cb_auto(AW_root *root, AWT_canvas *gmw)
//  --------------------------------------------------------------
void GEN_jump_cb_auto(AW_root *root, AWT_canvas *gmw) {
    int jump = root->awar(AWAR_GENMAP_AUTO_JUMP)->read_int();
    if (jump) {
        AW_window *dummy = 0;
        GEN_jump_cb(dummy, gmw);
    }
}

//  -------------------------------------------------------------------------
//      void GEN_organism_name_changed_cb(AW_root *awr, AWT_canvas *gmw)
//  -------------------------------------------------------------------------
void GEN_organism_name_changed_cb(AW_root *awr, AWT_canvas *gmw) {
    GEN_GRAPHIC->reinit_gen_root(gmw);
    GEN_jump_cb_auto(awr, gmw);
    gmw->refresh();
}

//  ------------------------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr, AWT_canvas *gmw)
//  ------------------------------------------------------------------------
void GEN_gene_name_changed_cb(AW_root *awr, AWT_canvas *gmw) {
    GEN_GRAPHIC->reinit_gen_root(gmw);
    GEN_jump_cb_auto(awr, gmw);
    gmw->refresh();
}

//  -------------------------------------------------------------------------
//      void GEN_display_param_changed_cb(AW_root *awr, AWT_canvas *gmw)
//  -------------------------------------------------------------------------
void GEN_display_param_changed_cb(AW_root */*awr*/, AWT_canvas *gmw) {
    gmw->zoom_reset();
    gmw->refresh();
}

//  --------------------------------------------------------------------------------------
//      void GEN_add_awar_callbacks(AW_root *awr,AW_default /*def*/, AWT_canvas *gmw)
//  --------------------------------------------------------------------------------------
void GEN_add_awar_callbacks(AW_root *awr,AW_default /*def*/, AWT_canvas *gmw) {
    awr->awar_string(AWAR_ORGANISM_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_organism_name_changed_cb, (AW_CL)gmw);
    awr->awar_string(AWAR_GENE_NAME,"",gb_main)->add_callback((AW_RCB1)GEN_gene_name_changed_cb, (AW_CL)gmw);
    awr->awar_string(AWAR_COMBINED_GENE_NAME,"",gb_main);

    awr->awar(AWAR_GENMAP_ARROW_SIZE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_SHOW_HIDDEN)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_SHOW_ALL_NDS)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);

    awr->awar(AWAR_GENMAP_BOOK_BASES_PER_LINE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_BOOK_WIDTH_FACTOR)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_BOOK_LINE_HEIGHT)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_BOOK_LINE_SPACE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);

    awr->awar(AWAR_GENMAP_VERTICAL_FACTOR_X)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_VERTICAL_FACTOR_Y)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);

    awr->awar(AWAR_GENMAP_RADIAL_INSIDE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
    awr->awar(AWAR_GENMAP_RADIAL_OUTSIDE)->add_callback((AW_RCB1)GEN_display_param_changed_cb, (AW_CL)gmw);
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

    gen_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    aws->get_root()->awar(AWAR_FOOTER)->write_string( text);
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
    aws->init( awr, "GEN_OPTIONS", "GENE MAP OPTIONS");
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

    aws->init(awr, "GENE_LAYOUT", "Gene Map Layout");
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

    //     GEN_MARK_COLORED,              done by awt_colorize_marked now
    //     GEN_UNMARK_COLORED,
    //     GEN_INVERT_COLORED,
    //     GEN_COLORIZE_MARKED,

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

//  -----------------------------------------------------------------------------------------------------
//      static GB_ERROR GEN_species_add_entry(GBDATA *gb_pseudo, const char *key, const char *value)
//  -----------------------------------------------------------------------------------------------------
static GB_ERROR GEN_species_add_entry(GBDATA *gb_pseudo, const char *key, const char *value) {
    GB_ERROR  error = 0;
    GB_clear_error();
    GBDATA   *gbd   = GB_find(gb_pseudo, key, 0, down_level);

    if (!gbd) { // key does not exist yet -> create
        gbd   = GB_create(gb_pseudo, key, GB_STRING);
        error = GB_get_error();
    }
    else { // key exists
        if (GB_read_type(gbd) != GB_STRING) { // test correct key type
            error = GB_export_error("field '%s' exists and has wrong type", key);
        }
    }

    if (!error) error = GB_write_string(gbd, value);

    return error;
}

static AW_repeated_question *ask_about_existing_gene_species = 0;
static AW_repeated_question *ask_to_overwrite_alignment      = 0;

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

    char *sequence = GBT_read_gene_sequence(gb_gene, true);
    if (!sequence) {
        aw_message(GB_get_error());
    }
    else  {
        long id = GBS_checksum(sequence, 1, ".-");
        char acc[100];
        sprintf(acc, "ARB_GENE_%lX", id);

        // test if this gene has been already extracted to a gene-species

        GBDATA   *gb_exist_geneSpec       = GEN_find_pseudo_species(gb_main, species_name, gene_name);
        bool      create_new_gene_species = true;
        char     *short_name              = 0;
        GB_ERROR  error                   = 0;

        if (gb_exist_geneSpec) {
            GBDATA *gb_name       = GB_find(gb_exist_geneSpec, "name", 0, down_level);
            char   *existing_name = GB_read_string(gb_name);

            gen_assert(ask_about_existing_gene_species);
            gen_assert(ask_to_overwrite_alignment);

            char *question = GBS_global_string_copy("Already have a gene-species for %s/%s ('%s')", species_name, gene_name, existing_name);
            int   answer   = ask_about_existing_gene_species->get_answer(question, "Overwrite species,Insert new alignment,Skip,Create new", "all", true);

            create_new_gene_species = false;

            switch (answer) {
                case 0: {   // Overwrite species
                    // @@@ FIXME:  delete species needed here
                    create_new_gene_species = true;
                    short_name              = strdup(existing_name);
                    break;
                }
                case 1: {     // Insert new alignment or overwrite alignment
                    GBDATA *gb_ali = GB_find(gb_exist_geneSpec, ali, 0, down_level);
                    if (gb_ali) { // the alignment already exists
                        char *question2        = GBS_global_string_copy("Gene-species '%s' already has data in '%s'", existing_name, ali);
                        int   overwrite_answer = ask_to_overwrite_alignment->get_answer(question2, "Overwrite data,Skip", "all", true);

                        if (overwrite_answer == 1) error      = GBS_global_string("Skipped gene-species '%s' (already had data in alignment)", existing_name); // Skip
                        else if (overwrite_answer == 2) error = "Aborted."; // Abort
                        // @@@ FIXME: overwrite data is missing

                        free(question2);
                    }
                    break;
                }
                case 2: {       // Skip existing ones
                    error = GBS_global_string("Skipped gene-species '%s'", existing_name);
                    break;
                }
                case 3: {   // Create with new name
                    create_new_gene_species = true;
                    break;
                }
                case 4: {   // Abort
                    error = "Aborted.";
                    break;
                }
                default : gen_assert(0);
            }
            free(question);
            free(existing_name);
        }

        if (!error) {
            if (create_new_gene_species) {
                if (!short_name) { // create a new name
                    error = AWTC_generate_one_name(gb_main, full_name, acc, short_name, false);
                    if (!error) { // name was created
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
                        if (!short_name) error = GBS_global_string("Failed to create a new name for pseudo gene-species '%s'", full_name);
                    }
                }

                if (!error) { // create the species
                    gen_assert(short_name);
                    gb_exist_geneSpec = GBT_create_species(gb_main, short_name);
                    if (!gb_exist_geneSpec) error = GB_export_error("Failed to create pseudo-species '%s'", short_name);
                }
            }
            else {
                gen_assert(gb_exist_geneSpec); // do not generate new or skip -> should only occur when gene-species already existed
            }

            if (!error) { // write sequence data
                GBDATA *gb_data = GBT_add_data(gb_exist_geneSpec, ali, "data", GB_STRING);
                if (gb_data) {
                    size_t sequence_length = strlen(sequence);
                    error                  = GBT_write_sequence(gb_data, ali, sequence_length, sequence);
                }
                else {
                    error = GB_get_error();
                }

                //                 GBDATA *gb_ali = GB_search(gb_exist_geneSpec, ali, GB_DB);
                //                 if (gb_ali) {
                //                     GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
                //                     error           = GB_write_string(gb_data, sequence);
                //                     GBT_write_sequence(...);
                //                 }
                //                 else {
                //                     error = GB_export_error("Can't create alignment '%s' for '%s'", ali, short_name);
                //                 }
            }



            // write other entries:
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "full_name", full_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "ARB_origin_species", species_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "ARB_origin_gene", gene_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "acc", acc);

            // copy codon_start and transl_table :
            const char *codon_start  = 0;
            const char *transl_table = 0;

            {
                GBDATA *gb_codon_start  = GB_find(gb_gene, "codon_start", 0, down_level);
                GBDATA *gb_transl_table = GB_find(gb_gene, "transl_table", 0, down_level);

                if (gb_codon_start)  codon_start  = GB_read_char_pntr(gb_codon_start);
                if (gb_transl_table) transl_table = GB_read_char_pntr(gb_transl_table);
            }

            if (!error && codon_start)  error = GEN_species_add_entry(gb_exist_geneSpec, "codon_start", codon_start);
            if (!error && transl_table) error = GEN_species_add_entry(gb_exist_geneSpec, "transl_table", transl_table);
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
    GEN_MARK_MODE mode  = (GEN_MARK_MODE)imode;
    GB_ERROR      error = 0;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         !error && gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool mark_flag     = GB_read_flag(gb_gene) != 0;
        bool org_mark_flag = mark_flag;
        //         int wantedColor;

        switch (mode) {
            case GEN_MARK:              mark_flag = 1; break;
            case GEN_UNMARK:            mark_flag = 0; break;
            case GEN_INVERT_MARKED:     mark_flag = !mark_flag; break;

            case GEN_COUNT_MARKED:     {
                if (mark_flag) ++gen_count_marked_genes;
                break;
            }
            case GEN_EXTRACT_MARKED: {
                if (mark_flag) GEN_extract_gene_2_pseudoSpecies(gb_species, gb_gene, (const char *)cl_user);
                break;
            }
                //             case GEN_COLORIZE_MARKED:  {
                //                 if (mark_flag) error = AW_set_color_group(gb_gene, wantedColor);
                //                 break;
                //             }
            default: {
                GBDATA *gb_hidden = GB_find(gb_gene, ARB_HIDDEN, 0, down_level);
                bool    hidden    = gb_hidden ? GB_read_byte(gb_hidden) != 0 : false;
                //                 long    myColor   = AW_find_color_group(gb_gene, true);

                switch (mode) {
                    //                     case GEN_MARK_COLORED:      if (myColor == wantedColor) mark_flag = 1; break;
                    //                     case GEN_UNMARK_COLORED:    if (myColor == wantedColor) mark_flag = 0; break;
                    //                     case GEN_INVERT_COLORED:    if (myColor == wantedColor) mark_flag = !mark_flag; break;

                    case GEN_MARK_HIDDEN:       if (hidden) mark_flag = 1; break;
                    case GEN_UNMARK_HIDDEN:     if (hidden) mark_flag = 0; break;
                    case GEN_MARK_VISIBLE:      if (!hidden) mark_flag = 1; break;
                    case GEN_UNMARK_VISIBLE:    if (!hidden) mark_flag = 0; break;
                    default: gen_assert(0); break;
                }
            }
        }

        if (mark_flag != org_mark_flag) {
            error = GB_write_flag(gb_gene, mark_flag?1:0);
        }
    }

    if (error) aw_message(error);
}

//  --------------------------------------------------------------------------------------------------
//      static void do_hide_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user)
//  --------------------------------------------------------------------------------------------------
static void do_hide_command_for_one_species(int imode, GBDATA *gb_species, AW_CL /*cl_user*/) {
    GEN_HIDE_MODE mode = (GEN_HIDE_MODE)imode;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool marked = GB_read_flag(gb_gene) != 0;

        GBDATA *gb_hidden  = GB_find(gb_gene, ARB_HIDDEN, 0, down_level);
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
            if (!gb_hidden) gb_hidden = GB_create(gb_gene, ARB_HIDDEN, GB_BYTE);
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
        aw_message(GBS_global_string("There are %li marked genes in %s", gen_count_marked_genes, where));
    }
}

//  ------------------------------------------------------------
//      void gene_extract_cb(AW_window *aww, AW_CL cl_pmode)
//  ------------------------------------------------------------
void gene_extract_cb(AW_window *aww, AW_CL cl_pmode){
    char     *ali   = aww->get_root()->awar(AWAR_GENE_EXTRACT_ALI)->read_string();
    GB_ERROR  error = GBT_check_alignment_name(ali);

    if (!error) {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_ali = GBT_get_alignment(gb_main, ali);

        if (!gb_ali && !GBT_create_alignment(gb_main,ali,0,0,0,"dna")) {
            error = GB_get_error();
        }
    }

    if (error) {
        aw_message(error);
    }
    else {
        ask_about_existing_gene_species = new AW_repeated_question();
        ask_to_overwrite_alignment      = new AW_repeated_question();

        aw_openstatus("Extracting pseudo-species");
        GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_mark_command_for_one_species, GEN_EXTRACT_MARKED, (AW_CL)ali);
        aw_closestatus();

        delete ask_to_overwrite_alignment;
        delete ask_about_existing_gene_species;
        ask_to_overwrite_alignment      = 0;
        ask_about_existing_gene_species = 0;
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
// cl_mark == 3 -> mark organisms, unmark rest
static void mark_organisms(AW_window */*aww*/, AW_CL cl_mark, AW_CL cl_canvas) {
    GB_transaction dummy(gb_main);
    int            mark = (int)cl_mark;

    if (mark == 3) {
        GBT_mark_all(gb_main, 0); // unmark all species
        mark = 1;
    }

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

    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}

//  --------------------------------------------------------------------------------------
//      static void mark_gene_species(AW_window *aww, AW_CL cl_mark, AW_CL cl_canvas)
//  --------------------------------------------------------------------------------------
//  cl_mark == 0 -> unmark
//  cl_mark == 1 -> mark
//  cl_mark == 2 -> invert mark
//  cl_mark == 3 -> mark gene-species, unmark rest

static void mark_gene_species(AW_window */*aww*/, AW_CL cl_mark, AW_CL cl_canvas) {
    GB_transaction dummy(gb_main);
    int            mark = (int)cl_mark;

    if (mark == 3) {
        GBT_mark_all(gb_main, 0); // unmark all species
        mark = 1;
    }

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

    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}
//  ----------------------------------------------------------------------------------------------
//      static void mark_gene_species_of_marked_genes(AW_window *aww, AW_CL cl_canvas, AW_CL)
//  ----------------------------------------------------------------------------------------------
static void mark_gene_species_of_marked_genes(AW_window */*aww*/, AW_CL cl_canvas, AW_CL) {
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
    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}
//  ---------------------------------------------------------------------------------------------
//      static void mark_organisms_with_marked_genes(AW_window *aww, AW_CL cl_canvas, AW_CL)
//  ---------------------------------------------------------------------------------------------
static void mark_organisms_with_marked_genes(AW_window */*aww*/, AW_CL cl_canvas, AW_CL) {
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
                GB_write_flag(gb_species, 1);
                break; // continue with next organism
            }
        }
    }
    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}
//  ------------------------------------------------------------------------------------------------------
//      static void mark_gene_species_using_current_alignment(AW_window *aww, AW_CL cl_canvas, AW_CL)
//  ------------------------------------------------------------------------------------------------------
static void mark_gene_species_using_current_alignment(AW_window */*aww*/, AW_CL cl_canvas, AW_CL) {
    GB_transaction  dummy(gb_main);
    char           *ali = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        GBDATA *gb_ali = GB_find(gb_pseudo, ali, 0, down_level);
        if (gb_ali) {
            GBDATA *gb_data = GB_find(gb_ali, "data", 0, down_level);
            if (gb_data) {
                GB_write_flag(gb_pseudo, 1);
            }
        }
    }
    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}
//  ------------------------------------------------------------------------------------
//      static void mark_genes_of_marked_gene_species(AW_window *aww, AW_CL, AW_CL)
//  ------------------------------------------------------------------------------------
static void mark_genes_of_marked_gene_species(AW_window */*aww*/, AW_CL, AW_CL) {
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
    aws->init( root, "EXTRACT_GENE", "Extract genes to alignment");
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
    AWMIMT(macro_name_buffer, "of current species...", "c", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_CURRENT_SPECIES);

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    AWMIMT(macro_name_buffer, "of marked species...", "m", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_MARKED_SPECIES);

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    AWMIMT(macro_name_buffer, "of all species...", "a", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_ALL_SPECIES);

    awm->close_sub_menu();
}

//  -----------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_multi_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
//  -----------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_multi_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                              const char *help_file, void (*command)(AW_window*, AW_CL, AW_CL), AW_CL command_mode)
{
    awm->insert_sub_menu(0, submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of current species", "c", help_file, AWM_ALL, command, GEN_PERFORM_CURRENT_SPECIES, command_mode);

    sprintf(macro_name_buffer, "%s_of_all_but_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of all but current species", "b", help_file, AWM_ALL, command, GEN_PERFORM_ALL_BUT_CURRENT_SPECIES, command_mode);

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    AWMIMT(macro_name_buffer, "of marked species", "m", help_file, AWM_ALL, command, GEN_PERFORM_MARKED_SPECIES, command_mode);

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    AWMIMT(macro_name_buffer, "of all species", "a", help_file, AWM_ALL, command, GEN_PERFORM_ALL_SPECIES, command_mode);

    awm->close_sub_menu();
}
//  ----------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_insert_mark_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
//  ----------------------------------------------------------------------------------------------------------------------------------------
void GEN_insert_mark_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                             const char *help_file, GEN_MARK_MODE mark_mode)
{
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

        aws->init(aw_root, "DEBUG_AWARS", "DEBUG AWARS");
        aws->at(10, 10);
        aws->auto_space(10,10);

        const int width = 50;

        ;                  aws->label("AWAR_SPECIES_NAME      "); aws->create_input_field(AWAR_SPECIES_NAME, width);
        aws->at_newline(); aws->label("AWAR_ORGANISM_NAME     "); aws->create_input_field(AWAR_ORGANISM_NAME, width);
        aws->at_newline(); aws->label("AWAR_GENE_NAME         "); aws->create_input_field(AWAR_GENE_NAME, width);
        aws->at_newline(); aws->label("AWAR_COMBINED_GENE_NAME"); aws->create_input_field(AWAR_COMBINED_GENE_NAME, width);
        aws->at_newline(); aws->label("AWAR_EXPERIMENT_NAME   "); aws->create_input_field(AWAR_EXPERIMENT_NAME, width);

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

static AW_window *GEN_create_gene_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, gb_main, &GEN_item_selector);
}

static AW_window *GEN_create_organism_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, gb_main, &AWT_organism_selector);
}

// used to avoid that the organisms info window is stored in a menu (or with a button)
void GEN_popup_organism_window(AW_window *aww, AW_CL, AW_CL) {
    AW_window *aws = NT_create_organism_window(aww->get_root());
    aws->show();
}
//  ------------------------------------------------------------------------------------------------------------
//      void GEN_create_organism_submenu(AW_window_menu_modes *awm, bool submenu, AWT_canvas *ntree_canvas)
//  ------------------------------------------------------------------------------------------------------------
void GEN_create_organism_submenu(AW_window_menu_modes *awm, bool submenu, AWT_canvas *ntree_canvas) {
    const char *title  = "Organisms";
    const char *hotkey = "O";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT( "organism_info", "Organism information", "i", "organism_info.hlp", AWM_ALL,GEN_popup_organism_window,  0, 0);
//         AWMIMT( "organism_info", "Organism information", "i", "organism_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)NT_create_organism_window,  0 );

        awm->insert_separator();

        AWMIMT("mark_organisms", "Mark All organisms", "A", "organism_mark.hlp", AWM_ALL, mark_organisms, 1, (AW_CL)ntree_canvas);
        AWMIMT("mark_organisms_unmark_rest", "Mark all organisms, unmark Rest", "R", "organism_mark.hlp", AWM_ALL, mark_organisms, 3, (AW_CL)ntree_canvas);
        AWMIMT("unmark_organisms", "Unmark all organisms", "U", "organism_mark.hlp", AWM_ALL, mark_organisms, 0, (AW_CL)ntree_canvas);
        AWMIMT("invmark_organisms", "Invert marks of all organisms", "v", "organism_mark.hlp", AWM_ALL, mark_organisms, 2, (AW_CL)ntree_canvas);
        awm->insert_separator();
        AWMIMT("mark_organisms_with_marked_genes", "Mark organisms with marked Genes", "G", "organism_mark.hlp", AWM_ALL, mark_organisms_with_marked_genes, (AW_CL)ntree_canvas, 0);
        awm->insert_separator();
        AWMIMT( "organism_colors",  "Colors ...",           "C",    "mark_colors.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_organism_colorize_window, 0);
    }
    if (submenu) awm->close_sub_menu();
}
//  ----------------------------------------------------------------------------------------------------------------
//      void GEN_create_gene_species_submenu(AW_window_menu_modes *awm, bool submenu, AWT_canvas *ntree_canvas)
//  ----------------------------------------------------------------------------------------------------------------
void GEN_create_gene_species_submenu(AW_window_menu_modes *awm, bool submenu, AWT_canvas *ntree_canvas) {
    const char *title  = "Gene-Species";
    const char *hotkey = "S";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT("mark_gene_species", "Mark All gene-species", "A", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 1, (AW_CL)ntree_canvas);
        AWMIMT("mark_gene_species_unmark_rest", "Mark all gene-species, unmark Rest", "R", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 3, (AW_CL)ntree_canvas);
        AWMIMT("unmark_gene_species", "Unmark all gene-species", "U", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 0, (AW_CL)ntree_canvas);
        AWMIMT("invmark_gene_species", "Invert marks of all gene-species", "I", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 2, (AW_CL)ntree_canvas);
        awm->insert_separator();
        AWMIMT("mark_gene_species_of_marked_genes", "Mark gene-species of marked genes", "M", "gene_species_mark.hlp", AWM_ALL, mark_gene_species_of_marked_genes, (AW_CL)ntree_canvas, 0);
        AWMIMT("mark_gene_species", "Mark all gene-species using Current alignment", "C", "gene_species_mark.hlp", AWM_ALL, mark_gene_species_using_current_alignment, (AW_CL)ntree_canvas, 0);
    }

    if (submenu) awm->close_sub_menu();
}

struct GEN_update_info {
    AWT_canvas *canvas1;        // just canvasses of different windows (needed for updates)
    AWT_canvas *canvas2;
};

//  ---------------------------------------------------------------------------------------------------------------
//      void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE, AWT_canvas *ntree_canvas)
//  ---------------------------------------------------------------------------------------------------------------
void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE, AWT_canvas *ntree_canvas) {
    gen_assert(ntree_canvas != 0);

    awm->create_menu(0,"Genome", "G", "no.hlp",    AWM_ALL);
    {
#if defined(DEBUG)
        AWMIMT("debug_awars", "[DEBUG] Show main AWARs", "", "no.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_awar_debug_window, 0);
        awm->insert_separator();
#endif // DEBUG

        if (for_ARB_NTREE) {
            AWMIMT( "gene_map", "Gene Map", "", "gene_map.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_map, (AW_CL)ntree_canvas );
            awm->insert_separator();

            GEN_create_gene_species_submenu(awm, true, ntree_canvas); // Gene-species
            GEN_create_organism_submenu(awm, true, ntree_canvas); // Organisms
            EXP_create_experiments_submenu(awm, true); // Experiments
            awm->insert_separator();
        }
        AWMIMT( "gene_info",    "Gene information", "", "gene_info.hlp", AWM_ALL,GEN_popup_gene_window, (AW_CL)awm, 0);
//         AWMIMT( "gene_info",    "Gene information", "", "gene_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_window, 0 );
        AWMIMT( "gene_search",  "Search and Query", "", "gene_search.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_query_window, 0 );

        GEN_create_mask_submenu(awm);

        awm->insert_separator();

        GEN_insert_mark_submenu(awm, "gene_mark_all", "Mark all genes", "M", "gene_mark.hlp",  GEN_MARK);
        GEN_insert_mark_submenu(awm, "gene_unmark_all", "Unmark all genes", "U", "gene_mark.hlp", GEN_UNMARK);
        GEN_insert_mark_submenu(awm, "gene_invert_marked", "Invert marked genes", "v", "gene_mark.hlp", GEN_INVERT_MARKED);
        GEN_insert_mark_submenu(awm, "gene_count_marked", "Count marked genes", "C", "gene_mark.hlp", GEN_COUNT_MARKED);

        AWMIMT( "gene_colors",  "Colors ...", "l",    "mark_colors.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_colorize_window, 0);

        awm->insert_separator();

        AWMIMT("mark_genes_of_marked_gene_species", "Mark genes of marked gene-species", "g", "gene_mark.hlp", AWM_ALL, mark_genes_of_marked_gene_species, 0, 0);

        awm->insert_separator();

        GEN_insert_extract_submenu(awm, "gene_extract_marked", "Extract marked genes", "E", "gene_extract.hlp");

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
    awm->create_menu(0,"Hide","H","no.hlp", AWM_ALL);
    {
        GEN_insert_hide_submenu(awm, "gene_hide_marked", "Hide marked genes", "H", "gene_hide.hlp", GEN_HIDE_MARKED);
        GEN_insert_hide_submenu(awm, "gene_unhide_marked", "Unhide marked genes", "U", "gene_hide.hlp", GEN_UNHIDE_MARKED);
        GEN_insert_hide_submenu(awm, "gene_invhide_marked", "Invert-hide marked genes", "v", "gene_hide.hlp", GEN_INVERT_HIDE_MARKED);

        awm->insert_separator();
        GEN_insert_hide_submenu(awm, "gene_hide_all", "Hide all genes", "", "gene_hide.hlp", GEN_HIDE_ALL);
        GEN_insert_hide_submenu(awm, "gene_unhide_all", "Unhide all genes", "", "gene_hide.hlp", GEN_UNHIDE_ALL);
        GEN_insert_hide_submenu(awm, "gene_invhide_all", "Invert-hide all genes", "", "gene_hide.hlp", GEN_INVERT_HIDE_ALL);
    }
}

//  ----------------------------------------------------------------------------------------
//      void GEN_set_display_style(void *dummy, AWT_canvas *ntw, GEN_DisplayStyle type)
//  ----------------------------------------------------------------------------------------
void GEN_set_display_style(void */*dummy*/, AWT_canvas *ntw, GEN_DisplayStyle type) {
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

//  --------------------------------------------------------------------------------------
//      AW_window *GEN_map_create_main_window(AW_root *awr, AWT_canvas *ntree_canvas)
//  --------------------------------------------------------------------------------------
AW_window *GEN_map_create_main_window(AW_root *awr, AWT_canvas *ntree_canvas) {
    GB_transaction        dummy(gb_main);
    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(awr, "ARB_GENE_MAP", "ARB_GENE_MAP", 300, 300);

    GEN_create_genemap_awars(awr, AW_ROOT_DEFAULT);

    AW_gc_manager  aw_gc_manager;
    GEN_GRAPHIC        = new GEN_graphic(awr, gb_main, GEN_gene_container_cb_installer);
    AWT_canvas    *gmw = new AWT_canvas(gb_main, awm, GEN_GRAPHIC, aw_gc_manager, AWAR_SPECIES_NAME);

    GEN_add_awar_callbacks(awr, AW_ROOT_DEFAULT, gmw);

    GEN_GRAPHIC->reinit_gen_root(gmw);

    gmw->recalc_size();
    gmw->refresh();
    gmw->set_mode(AWT_MODE_ZOOM); // Default-Mode

    // File Menu
    awm->create_menu( 0, "File", "F", "no.hlp",  AWM_ALL );
    AWMIMT( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);

    GEN_create_genes_submenu(awm, false, ntree_canvas); // Genes
    GEN_create_gene_species_submenu(awm, false, ntree_canvas); // Gene-species
    GEN_create_organism_submenu(awm, false, ntree_canvas); // Organisms

    EXP_create_experiments_submenu(awm, false); // Experiments

    GEN_create_hide_submenu(awm); // Hide Menu

    // Properties Menu
    awm->create_menu("props","Properties","r","no.hlp", AWM_ALL);
    AWMIMT("gene_props_menu",   "Menu: Colors and Fonts ...",   "M","props_frame.hlp",  AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    AWMIMT("gene_props",        "GENEMAP: Colors and Fonts ...","C","gene_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    AWMIMT("gene_layout",       "Layout",   "L", "gene_layout.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_layout_window, 0);
    AWMIMT("gene_options",      "Options",  "O", "gene_options.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_options_window, 0);
    AWMIMT("gene_nds",          "NDS ( Select Gene Information ) ...",      "N","props_nds.hlp",    AWM_ALL, AW_POPUP, (AW_CL)GEN_open_nds_window, (AW_CL)gb_main );
    AWMIMT("gene_save_props",   "Save Defaults (in ~/.arb_prop/ntree.arb)", "D","savedef.hlp",  AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

    // Create mode buttons
    awm->create_mode( 0, "zoom.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)gmw,(AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "info.bitmap", "gen_mode.hlp", AWM_ALL, (AW_CB)GEN_mode_event,(AW_CL)gmw,(AW_CL)AWT_MODE_MOD);

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
    awm->callback((AW_CB)GEN_undo_cb,(AW_CL)gmw,(AW_CL)GB_UNDO_UNDO);
    awm->create_button("Undo", "Undo");

    awm->at_newline();
    awm->get_at_position( &cur_x,&third_line_y);

    awm->button_length(AWAR_FOOTER_MAX_LEN);
    awm->create_button(0,AWAR_FOOTER);

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
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)gmw,(AW_CL)GEN_DISPLAY_STYLE_RADIAL);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_radial.bitmap",0);

    awm->help_text("gen_disp_book.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)gmw,(AW_CL)GEN_DISPLAY_STYLE_BOOK);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_book.bitmap",0);

    awm->get_at_position( &cur_x,&cur_y );
    int jump_x = cur_x;

    awm->at(dtype_x1, second_line_y);
    awm->help_text("gen_disp_vertical.hlp");
    awm->callback((AW_CB)GEN_set_display_style,(AW_CL)gmw,(AW_CL)GEN_DISPLAY_STYLE_VERTICAL);
    awm->create_button("RADIAL_DISPLAY_TYPE", "#gen_vertical.bitmap",0);

    // jump button:

    awm->button_length(0);

    awm->at(jump_x, first_line_y);
    awm->help_text("gen_jump.hlp");
    awm->callback((AW_CB)GEN_jump_cb,(AW_CL)gmw,(AW_CL)0);
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

//  -----------------------------------------------------------------
//      AW_window *GEN_map(AW_root *aw_root, AW_CL cl_nt_canvas)
//  -----------------------------------------------------------------
AW_window *GEN_map(AW_root *aw_root, AW_CL cl_nt_canvas) {
    static AW_window *aw_gen = 0;

    if (!aw_gen) {              // do not open window twice
        {
            GB_transaction dummy(gb_main);
            GEN_make_node_text_init(gb_main);
        }

        aw_gen = GEN_map_create_main_window(aw_root, (AWT_canvas*)cl_nt_canvas);
        if (!aw_gen) {
            aw_message("Couldn't start Gene-Map");
            return 0;
        }

    }

    aw_gen->show();
    return aw_gen;
}

