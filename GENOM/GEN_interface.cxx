#include <cstdlib>
#include <cstring>
#include <awt.hxx>
#include <awt_canvas.hxx>
#include <aw_awars.hxx>
#include <arbdbt.h>
#include <probe_design.hxx>

#include <ntree.hxx>
#include <../NTREE/ad_spec.hxx>
#include <../NTREE/nt_internal.h>

#ifndef GEN_LOCAL_HXX
#include "GEN_local.hxx"
#endif
#ifndef GEN_NDS_HXX
#include "GEN_nds.hxx"
#endif

using namespace std;

// --------------------------------------------------------------------------------

#define AD_F_ALL (AW_active)(-1)

// --------------------------------------------------------------------------------

void GEN_select_gene(GBDATA* /*gb_main*/, AW_root *aw_root, const char *item_name) {
    char *organism  = GB_strdup(item_name);
    char *gene = strchr(organism, '/');

    if (gene) {
        *gene++ = 0;
        aw_root->awar(AWAR_ORGANISM_NAME)->write_string(organism);
        aw_root->awar(AWAR_GENE_NAME)->write_string(gene);
    }
    else {
        aw_message(GBS_global_string("Illegal item_name '%s' in GEN_select_gene()", item_name));
    }
    free(organism);
}

static char *gen_get_gene_id(GBDATA */*gb_main*/, GBDATA *gb_gene) {
    GBDATA *gb_name = GB_find(gb_gene, "name", 0, down_level);
    if (!gb_name) return 0;     // gene w/o name -> skip

    GBDATA *gb_species = GB_get_father(GB_get_father(gb_gene));
    GBDATA *gb_sp_name = GB_find(gb_species, "name", 0, down_level);
    if (!gb_sp_name) return 0;  // species w/o name -> skip

    char *species_name = GB_read_as_string(gb_sp_name);
    char *gene_name = GB_read_as_string(gb_name);

    char *result = (char*)malloc(strlen(species_name)+1+strlen(gene_name)+1);
    strcpy(result, species_name);
    strcat(result, "/");
    strcat(result, gene_name);

    free(gene_name);
    free(species_name);

    return result;
}

static GBDATA *gen_find_gene_by_id(GBDATA *gb_main, const char *id) {
    char   *organism = GB_strdup(id);
    char   *gene     = strchr(organism, '/');
    GBDATA *result   = 0;

    if (gene) {
        *gene++ = 0;
        GBDATA *gb_organism = GEN_find_organism(gb_main, organism);
        if (gb_organism) {
            result = GEN_find_gene(gb_organism, gene);
        }
    }

    free(organism);
    return result;
}


static char *old_species_marks = 0; // configuration storing marked species

//  --------------------------------------------------------------------------------------------------------------
//      extern "C" GB_ERROR GEN_mark_organism_or_corresponding_organism(GBDATA *gb_species, int *client_data)
//  --------------------------------------------------------------------------------------------------------------
extern "C" GB_ERROR GEN_mark_organism_or_corresponding_organism(GBDATA *gb_species, int *client_data) {
    AWUSE(client_data);
    GB_ERROR error = 0;

    if (GEN_is_pseudo_gene_species(gb_species)) {
        GBDATA *gb_organism = GEN_find_origin_organism(gb_species);
        if (gb_organism) {
            GB_write_flag(gb_organism, 1);
        }
        else {
            error = GEN_organism_not_found(gb_species);
        }
    }
    else if (GEN_is_organism(gb_species)) {
        GB_write_flag(gb_species, 1);
    }

    return error;
}

//  ---------------------------------------------------------------------------------------------------------
//      static GBDATA *GEN_get_first_gene_data(GBDATA *gb_main, AW_root *aw_root, AWT_QUERY_RANGE range)
//  ---------------------------------------------------------------------------------------------------------
static GBDATA *GEN_get_first_gene_data(GBDATA *gb_main, AW_root *aw_root, AWT_QUERY_RANGE range) {
    GBDATA   *gb_species = 0;
    GB_ERROR  error      = 0;

    switch (range) {
        case AWT_QUERY_CURRENT_SPECIES: {
            char *species_name = aw_root->awar(AWAR_ORGANISM_NAME)->read_string();
            gb_species         = GBT_find_species(gb_main, species_name);
            free(species_name);
            break;
        }
        case AWT_QUERY_MARKED_SPECIES: {
            GBDATA *gb_organism = GEN_first_marked_organism(gb_main);
            GBDATA *gb_pseudo   = GEN_first_marked_pseudo_species(gb_main);

            gb_assert(old_species_marks == 0); // this occurs in case of recursive calls (not possible)

            if (gb_pseudo) {    // there are marked pseudo-species..
                old_species_marks = GBT_store_marked_species(gb_main, 1); // store and unmark marked species

                error                  = GBT_with_stored_species(gb_main, old_species_marks, GEN_mark_organism_or_corresponding_organism, 0); // mark organisms related with stored
                if (!error) gb_species = GEN_first_marked_organism(gb_main);
            }
            else {
                gb_species = gb_organism;
            }

            break;
        }
        case AWT_QUERY_ALL_SPECIES: {
            gb_species = GBT_first_species(gb_main);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    if (error) GB_export_error(error);
    return gb_species ? GEN_expect_gene_data(gb_species) : 0;
}
//  -------------------------------------------------------------------------------------------
//      static GBDATA *GEN_get_next_gene_data(GBDATA *gb_gene_data, AWT_QUERY_RANGE range)
//  -------------------------------------------------------------------------------------------
static GBDATA *GEN_get_next_gene_data(GBDATA *gb_gene_data, AWT_QUERY_RANGE range) {
    GBDATA *gb_species = 0;
    switch (range) {
        case AWT_QUERY_CURRENT_SPECIES: {
            break;
        }
        case AWT_QUERY_MARKED_SPECIES: {
            GBDATA *gb_last_species = GB_get_father(gb_gene_data);
            gb_species              = GEN_next_marked_organism(gb_last_species);

            if (!gb_species && old_species_marks) { // got all -> clean up
                GBT_restore_marked_species(gb_main, old_species_marks);
                free(old_species_marks);
                old_species_marks = 0;
            }

            break;
        }
        case AWT_QUERY_ALL_SPECIES: {
            GBDATA *gb_last_species = GB_get_father(gb_gene_data);
            gb_species              = GBT_next_species(gb_last_species);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    return gb_species ? GEN_expect_gene_data(gb_species) : 0;
}

//  ----------------------------------------------------
//      struct ad_item_selector GEN_item_selector =
//  ----------------------------------------------------
struct ad_item_selector GEN_item_selector         = {
    AWT_QUERY_ITEM_GENES,
    GEN_select_gene,
    gen_get_gene_id,
    gen_find_gene_by_id,
    (AW_CB)awt_gene_field_selection_list_update_cb,
    25,
    CHANGE_KEY_PATH_GENES,
    "gene",
    "genes",
    GEN_get_first_gene_data,
    GEN_get_next_gene_data,
    GEN_first_gene_rel_gene_data,
    GEN_next_gene,
    GEN_get_current_gene
};

//  -------------------------------------------------------
//      void GEN_species_name_changed_cb(AW_root *awr)
//  -------------------------------------------------------
void GEN_species_name_changed_cb(AW_root *awr) {
    char   *species_name = awr->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species   = GBT_find_species(gb_main, species_name);

    if (gb_species) {
        if (GEN_is_pseudo_gene_species(gb_species)) {
            awr->awar(AWAR_ORGANISM_NAME)->write_string(GEN_origin_organism(gb_species));
            awr->awar(AWAR_GENE_NAME)->write_string(GEN_origin_gene(gb_species));
        }
        else {
            awr->awar(AWAR_ORGANISM_NAME)->write_string(species_name);
        }
    }
    free(species_name);
}

//  ----------------------------------------------------------------------------------------------------------------------------
//      static void GB_auto_select_pseudo_species_cb(AW_root *awr, GBDATA *gb_main, const char *organism, const char *gene)
//  ----------------------------------------------------------------------------------------------------------------------------
static void auto_select_pseudo_species(AW_root *awr, GBDATA *gb_main, const char *organism, const char *gene) {
    GB_transaction  dummy(gb_main);
    GBDATA         *gb_pseudo = GEN_find_pseudo_species(gb_main, organism, gene); // search for pseudo species..

    if (gb_pseudo) {
        GBDATA *gb_name     = GB_find(gb_pseudo, "name", 0, down_level);
        char   *pseudo_name = GB_read_string(gb_name);

        awr->awar(AWAR_SPECIES_NAME)->write_string(pseudo_name); // ..and select it if found
        free(pseudo_name);
    }
    else {
        awr->awar(AWAR_SPECIES_NAME)->write_string(organism); // otherwise select organism
    }
}

//  --------------------------------------------------------------------
//      void GEN_update_GENE_CONTENT(GBDATA *gb_main, AW_root *awr)
//  --------------------------------------------------------------------
void GEN_update_GENE_CONTENT(GBDATA *gb_main, AW_root *awr) {
    GB_transaction  dummy(gb_main);
    GBDATA         *gb_gene = GEN_get_current_gene(gb_main, awr);
    bool            clear   = true;

    if (gb_gene) {
        char *gene_content = GBT_read_gene_sequence(gb_gene, false); // ignore complement here (to highlight gene in ARB_EDIT4)

        awr->awar(AWAR_GENE_CONTENT)->write_string(gene_content);
        clear = false;
        free(gene_content);
    }
    else {
        char      *gene_name  = awr->awar(AWAR_GENE_NAME)->read_string();
        const int  prefix_len = 10;

        if (strncmp(gene_name, "intergene_", prefix_len) == 0) { // special case (non-gene result from gene pt server)
            char *start_pos_ptr = gene_name+prefix_len;
            char *end_pos_ptr   = strchr(start_pos_ptr, '_');

            gen_assert(end_pos_ptr);
            if (end_pos_ptr) {
                *end_pos_ptr++ = 0;
                long start_pos = atol(start_pos_ptr);
                long end_pos   = atol(end_pos_ptr);

                gen_assert(end_pos >= start_pos);

                GBDATA *gb_organism = GEN_get_current_organism(gb_main, awr);
                if (gb_organism) {
                    GBDATA     *gb_seq   = GBT_read_sequence(gb_organism, GENOM_ALIGNMENT);
                    const char *seq_data = GB_read_char_pntr(gb_seq);

                    long  len    = end_pos-start_pos+1;
                    char *buffer = (char*)malloc(len+1);
                    memcpy(buffer, seq_data+start_pos, len);
                    buffer[len]  = 0;

                    awr->awar(AWAR_GENE_CONTENT)->write_string(buffer);
                    clear = false;

                    free(buffer);
                }
            }
        }
        free(gene_name);
    }

    if (clear) {
        awr->awar(AWAR_GENE_CONTENT)->write_string(""); // if we did not detect any gene sequence -> clear
    }
}

//  --------------------------------------------------
//      void GEN_update_combined_cb(AW_root *awr)
//  --------------------------------------------------
void GEN_update_combined_cb(AW_root *awr) {
    char       *organism     = awr->awar(AWAR_ORGANISM_NAME)->read_string();
    char       *gene         = awr->awar(AWAR_GENE_NAME)->read_string();
    char       *old_combined = awr->awar(AWAR_COMBINED_GENE_NAME)->read_string();
    const char *combined     = GBS_global_string("%s/%s", organism, gene);

    if (strcmp(combined, old_combined) != 0) {
        awr->awar(AWAR_COMBINED_GENE_NAME)->write_string(combined);
        auto_select_pseudo_species(awr, gb_main, organism, gene);
        GEN_update_GENE_CONTENT(gb_main, awr);
    }

    free(old_combined);
    free(gene);
    free(organism);
}

//  -----------------------------------------------------------------
//      bool GEN_is_genome_db(GBDATA *gb_main, int default_value)
//  -----------------------------------------------------------------
// default_value == 0 -> default to normal database
//               == 1 -> default to GENOM database
//               == -1 -> assume that type is already defined

bool GEN_is_genome_db(GBDATA *gb_main, int default_value) {
    GB_transaction  dummy(gb_main);
    GBDATA         *gb_genom_db = GB_find(gb_main, GENOM_DB_TYPE, 0, down_level);

    if (!gb_genom_db) {         // no DB-type entry -> create on with default
        if (default_value == -1) {
            GB_CORE;
        }

        gb_genom_db = GB_create(gb_main, GENOM_DB_TYPE, GB_INT);
        GB_write_int(gb_genom_db, default_value);
    }

    return GB_read_int(gb_genom_db) != 0;
}


//  -------------------------------------------------------------------
//      void GEN_create_awars(AW_root *aw_root, AW_default aw_def)
//  -------------------------------------------------------------------
void GEN_create_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_COMBINED_GENE_NAME,"",gb_main);
    aw_root->awar_string(AWAR_GENE_CONTENT,"",gb_main);

    aw_root->awar_string(AWAR_GENE_NAME, "" ,   gb_main)->add_callback((AW_RCB0)GEN_update_combined_cb);
    aw_root->awar_string(AWAR_ORGANISM_NAME, "" ,   gb_main)->add_callback((AW_RCB0)GEN_update_combined_cb);

    aw_root->awar_string(AWAR_SPECIES_NAME,"",gb_main)->add_callback((AW_RCB0)GEN_species_name_changed_cb);

    aw_root->awar_string(AWAR_GENE_DEST, "" ,   aw_def);
    aw_root->awar_string(AWAR_GENE_POS1, "" ,   aw_def);
    aw_root->awar_string(AWAR_GENE_POS2, "" ,   aw_def);
    aw_root->awar_int(AWAR_GENE_COMPLEMENT, 0,   aw_def);

    aw_root->awar_string(AWAR_GENE_EXTRACT_ALI, "ali_gene_" ,   aw_def);
}


//  ---------------------------------------------------------------------------
//      GBDATA *GEN_get_current_organism(GBDATA *gb_main, AW_root *aw_root)
//  ---------------------------------------------------------------------------
GBDATA *GEN_get_current_organism(GBDATA *gb_main, AW_root *aw_root) {
    char   *species_name = aw_root->awar(AWAR_ORGANISM_NAME)->read_string();
    GBDATA *gb_species   = GBT_find_species(gb_main,species_name);
    free(species_name);
    return gb_species;
}

//  -----------------------------------------------------------------------------
//      GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root)
//  -----------------------------------------------------------------------------
GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species      = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_gene_data    = 0;

    if (gb_species) gb_gene_data = GEN_expect_gene_data(gb_species);

    return gb_gene_data;
}

//  ---------------------------------------------------------------------
//      GBDATA *GEN_get_current_gene(GBDATA *gb_main, AW_root *root)
//  ---------------------------------------------------------------------
GBDATA *GEN_get_current_gene(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_gene    = 0;

    if (gb_species) {
        char *gene_name = aw_root->awar(AWAR_GENE_NAME)->read_string();
        gb_gene         = GEN_find_gene(gb_species,gene_name);
        free(gene_name);
    }

    return gb_gene;
}


static AW_CL    ad_global_scannerid   = 0;
static AW_root *ad_global_scannerroot = 0;
AW_CL           gene_query_global_cbs = 0;

//  -------------------------------------------
//      void gene_rename_cb(AW_window *aww)
//  -------------------------------------------
void gene_rename_cb(AW_window *aww){

    GB_ERROR  error   = 0;
    AW_root  *aw_root = aww->get_root();
    char     *source  = aw_root->awar(AWAR_GENE_NAME)->read_string();
    char     *dest    = aw_root->awar(AWAR_GENE_DEST)->read_string();

    if (strcmp(source, dest) != 0) {
        GB_begin_transaction(gb_main);

        GBDATA *gb_gene_data = GEN_get_current_gene_data(gb_main, aww->get_root());

        if (!gb_gene_data) error = "Please select a species first";
        else {
            GBDATA *gb_source = GEN_find_gene_rel_gene_data(gb_gene_data, source);
            GBDATA *gb_dest   = GEN_find_gene_rel_gene_data(gb_gene_data, dest);

            if (!gb_source) error   = "Please select a gene first";
            else if (gb_dest) error = GB_export_error("Sorry: gene '%s' already exists", dest);
            else {
                GBDATA *gb_name = GB_search(gb_source, "name", GB_STRING);
                error           = GB_write_string(gb_name, dest);
                if (!error) aww->get_root()->awar(AWAR_GENE_NAME)->write_string(dest);
            }
        }

        if (!error){
            aww->hide();
            GB_commit_transaction(gb_main);
        }else{
            GB_abort_transaction(gb_main);
        }
    }

    if (error) aw_message(error);
    delete source;
    delete dest;
}


//  -----------------------------------------------------------
//      AW_window *create_gene_rename_window(AW_root *root)
//  -----------------------------------------------------------
AW_window *create_gene_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "RENAME_GENE", "GENE RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST,15);
    aws->at("ok");
    aws->callback(gene_rename_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

//  -----------------------------------------
//      void gene_copy_cb(AW_window *aww)
//  -----------------------------------------
void gene_copy_cb(AW_window *aww){
    GB_begin_transaction(gb_main);

    GB_ERROR  error        = 0;
    char     *source       = aww->get_root()->awar(AWAR_GENE_NAME)->read_string();
    char     *dest         = aww->get_root()->awar(AWAR_GENE_DEST)->read_string();
    GBDATA   *gb_gene_data = GEN_get_current_gene_data(gb_main, aww->get_root());

    if (!gb_gene_data) {
        error = "Please select a species first.";
    }
    else {
        GBDATA *gb_source = GEN_find_gene_rel_gene_data(gb_gene_data, source);
        GBDATA *gb_dest   = GEN_find_gene_rel_gene_data(gb_gene_data, dest);

        if (!gb_source) error   = "Please select a gene first";
        else if (gb_dest) error = GB_export_error("Sorry: gene '%s' already exists", dest);
        else {
            gb_dest = GB_create_container(gb_gene_data,"gene");
            error = GB_copy(gb_dest, gb_source);
            if (!error) {
                GBDATA *gb_name = GB_search(gb_dest,"name",GB_STRING);
                error = GB_write_string(gb_name,dest);
            }
            if (!error) {
                aww->get_root()->awar(AWAR_GENE_NAME)->write_string(dest);
            }
        }
    }

    if (!error){
        aww->hide();
        GB_commit_transaction(gb_main);
    }else{
        GB_abort_transaction(gb_main);
    }
    if (error) aw_message(error);
    delete source;
    delete dest;
}

//  ---------------------------------------------------------
//      AW_window *create_gene_copy_window(AW_root *root)
//  ---------------------------------------------------------
AW_window *create_gene_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_GENE", "GENE COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST,15);

    aws->at("ok");
    aws->callback(gene_copy_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

//  -------------------------------------------
//      void gene_create_cb(AW_window *aww)
//  -------------------------------------------
void gene_create_cb(AW_window *aww){
    GB_begin_transaction(gb_main);

    GB_ERROR  error        = 0;
    AW_root  *aw_root      = aww->get_root();
    char     *dest         = aw_root->awar(AWAR_GENE_DEST)->read_string();
    int       pos1         = atoi(aw_root->awar(AWAR_GENE_POS1)->read_string());
    int       pos2         = atoi(aw_root->awar(AWAR_GENE_POS2)->read_string());
    int       complement   = aw_root->awar(AWAR_GENE_COMPLEMENT)->read_int();
    GBDATA   *gb_gene_data = GEN_get_current_gene_data(gb_main, aw_root);
    GBDATA   *gb_dest      = GEN_find_gene_rel_gene_data(gb_gene_data, dest);

    if (!gb_gene_data) error  = "Please select a species first";
    else if (gb_dest) error   = GBS_global_string("Gene '%s' already exists", dest);
    else {
        GB_ERROR pos_error = 0;
        if (pos1<1 || pos2<1) {
            pos_error = "positions have to be above zero";
        }
        else if (pos2<pos1) {
            pos_error = "endpos has to be greater than startpos";
        }
        else {
            GBDATA *gb_organism   = GB_get_father(gb_gene_data);
            GBDATA *gb_genome     = GBT_read_sequence(gb_organism, GENOM_ALIGNMENT);
            int     genome_length = GB_read_count(gb_genome);

            if (pos2 > genome_length) {
                pos_error = GBS_global_string("endpos is behind sequence end (%i)", genome_length);
            }
        }

        if (pos_error) {
            error = GBS_global_string("Illegal position(s): %s", pos_error);
        }
        else {
            gb_dest = GEN_create_gene_rel_gene_data(gb_gene_data, dest);
            if (gb_dest) {
                GBDATA *gb_pos     = GB_create(gb_dest, "pos_begin", GB_INT);
                if (!gb_pos) error = GB_get_error();
                else    GB_write_int(gb_pos, pos1);

                if (!error) {
                    gb_pos             = GB_create(gb_dest, "pos_end", GB_INT);
                    if (!gb_pos) error = GB_get_error();
                    else GB_write_int(gb_pos, pos2);
                }

                if (!error) {
                    GBDATA *gb_compl     = GB_create(gb_dest, "complement", GB_BYTE);
                    if (!gb_compl) error = GB_get_error();
                    else    GB_write_byte(gb_compl, complement);
                }
            }
            else {
                error = GB_get_error();
            }
            if (!error) aww->get_root()->awar(AWAR_GENE_NAME)->write_string(dest);
        }
    }
    if (!error) GB_commit_transaction(gb_main);
    else    GB_abort_transaction(gb_main);
    if (error) aw_message(error);
    delete dest;
}

//  -----------------------------------------------------------
//      AW_window *create_gene_create_window(AW_root *root)
//  -----------------------------------------------------------
AW_window *create_gene_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_GENE","GENE CREATE");
    aws->load_xfig("ad_al_si3.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label"); aws->create_autosize_button(0,"Please enter the name\nof the new gene");
    aws->at("input"); aws->create_input_field(AWAR_GENE_DEST,15);

    aws->at("label1"); aws->create_autosize_button(0,"Start position");
    aws->at("input1"); aws->create_input_field(AWAR_GENE_POS1,12);

    aws->at("label2"); aws->create_autosize_button(0,"End position");
    aws->at("input2"); aws->create_input_field(AWAR_GENE_POS2,12);

    aws->at("toggle");
    aws->label("Complentary strand");
    aws->create_toggle(AWAR_GENE_COMPLEMENT);

    aws->at("ok");
    aws->callback(gene_create_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}



//  -------------------------------------------
//      void gene_delete_cb(AW_window *aww)
//  -------------------------------------------
void gene_delete_cb(AW_window *aww){
    if (aw_message("Are you sure to delete the gene","OK,CANCEL")) return;

    GB_begin_transaction(gb_main);

    GB_ERROR error    = 0;
    GBDATA   *gb_gene = GEN_get_current_gene(gb_main, aww->get_root()); // aw_root);

    if (gb_gene) error = GB_delete(gb_gene);
    else error         = "Please select a gene first";

    if (!error) GB_commit_transaction(gb_main);
    else GB_abort_transaction(gb_main);

    if (error) aw_message(error);
}



//  ------------------------------------------------------------
//      void GEN_map_gene(AW_root *aw_root, AW_CL scannerid)
//  ------------------------------------------------------------
void GEN_map_gene(AW_root *aw_root, AW_CL scannerid)
{
    GB_transaction  dummy(gb_main);
    GBDATA         *gb_gene = GEN_get_current_gene(gb_main, aw_root);

    if (gb_gene) awt_map_arbdb_scanner(scannerid, gb_gene, 0, CHANGE_KEY_PATH_GENES);
}

//  ----------------------------------------------------
//      void GEN_create_field_items(AW_window *aws)
//  ----------------------------------------------------
void GEN_create_field_items(AW_window *aws) {
    aws->insert_menu_topic("reorder_fields", "Reorder fields ...",    "R", "spaf_reorder.hlp", AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_list_reorder, (AW_CL)&GEN_item_selector); 
    aws->insert_menu_topic("delete_field",   "Delete/Hide field ...", "D", "spaf_delete.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_delete, (AW_CL)&GEN_item_selector); 
    aws->insert_menu_topic("create_field",   "Create fields ...",     "C", "spaf_create.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_create, (AW_CL)&GEN_item_selector); 
    aws->insert_separator();
    aws->insert_menu_topic("unhide_fields", "Show all hidden fields", "S", "scandb.hlp", AD_F_ALL, (AW_CB)awt_gene_field_selection_list_unhide_all_cb, (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_separator();
    aws->insert_menu_topic("scan_unknown_fields", "Scan unknown fields",   "u", "scandb.hlp", AD_F_ALL, (AW_CB)awt_gene_field_selection_list_scan_unknown_cb,  (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_menu_topic("del_unused_fields",   "Remove unused fields",  "e", "scandb.hlp", AD_F_ALL, (AW_CB)awt_gene_field_selection_list_delete_unused_cb, (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_menu_topic("refresh_fields",      "Refresh fields (both)", "f", "scandb.hlp", AD_F_ALL, (AW_CB)awt_gene_field_selection_list_update_cb,        (AW_CL)gb_main, AWT_NDS_FILTER); 
}


//  ------------------------------------------------------------
//      AW_window *GEN_create_gene_window(AW_root *aw_root)
//  ------------------------------------------------------------
AW_window *GEN_create_gene_window(AW_root *aw_root) {
    static AW_window_simple_menu *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple_menu;
    aws->init( aw_root, "GENE_INFORMATION", "GENE INFORMATION");
    aws->load_xfig("ad_spec.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("search");
    aws->callback(AW_POPUP, (AW_CL)GEN_create_gene_query_window, 0);
    aws->create_button("SEARCH","SEARCH","S");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"gene_info.hlp");
    aws->create_button("HELP","HELP","H");


    AW_CL scannerid       = awt_create_arbdb_scanner(gb_main, aws, "box",0,"field","enable",AWT_VIEWER,0,"mark",AWT_NDS_FILTER, &GEN_item_selector);
    ad_global_scannerid   = scannerid;
    ad_global_scannerroot = aws->get_root();

    aws->create_menu(       0,   "GENE",     "G", "spa_gene.hlp",  AD_F_ALL );
    aws->insert_menu_topic("gene_delete",   "Delete",       "D","spa_delete.hlp",       AD_F_ALL,   (AW_CB)gene_delete_cb, 0, 0);
    aws->insert_menu_topic("gene_rename",   "Rename ...",   "R","spa_rename.hlp",   AD_F_ALL,   AW_POPUP, (AW_CL)create_gene_rename_window, 0);
    aws->insert_menu_topic("gene_copy",     "Copy ...",     "y","spa_copy.hlp",         AD_F_ALL,   AW_POPUP, (AW_CL)create_gene_copy_window, 0);
    aws->insert_menu_topic("gene_create",   "Create ...",   "C","spa_create.hlp",   AD_F_ALL,   AW_POPUP, (AW_CL)create_gene_create_window, 0);
    aws->insert_separator();

    aws->create_menu(       0,   "FIELDS",     "F", "gene_fields.hlp",  AD_F_ALL );
    GEN_create_field_items(aws);

    {
        Awar_Callback_Info    *cb_info     = new Awar_Callback_Info(aws->get_root(), AWAR_GENE_NAME, GEN_map_gene, scannerid); // do not delete!
        AW_detach_information *detach_info = new AW_detach_information(cb_info); // do not delete!

        cb_info->add_callback();

        aws->at("detach");
        aws->callback(NT_detach_information_window, (AW_CL)&aws, (AW_CL)detach_info);
        aws->create_button("DETACH", "DETACH", "D");

        detach_info->set_detach_button(aws->get_last_button_widget());
    }

    //     aws->get_root()->awar(AWAR_GENE_NAME)->add_callback(GEN_map_gene,scannerid);

    aws->show();
    GEN_map_gene(aws->get_root(),scannerid);
    return (AW_window *)aws;
}

void GEN_popup_gene_window(AW_window *aww, AW_CL, AW_CL) { // w/o this DETACH does not work
    AW_window *aws = GEN_create_gene_window(aww->get_root());
    aws->show();
}

//  ------------------------------------------------------------------
//      AW_window *GEN_create_gene_query_window(AW_root *aw_root)
//  ------------------------------------------------------------------
AW_window *GEN_create_gene_query_window(AW_root *aw_root) {

    static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "GEN_QUERY", "Gene SEARCH and QUERY");
    aws->create_menu(0,"More functions","f");
    aws->load_xfig("ad_query.fig");

    awt_query_struct awtqs;

    awtqs.gb_main             = gb_main;
    awtqs.species_name        = AWAR_SPECIES_NAME;
    awtqs.tree_name           = AWAR_TREE;
    //     awtqs.query_genes  = true;
    //     awtqs.gene_name    = AWAR_GENE_NAME;
    awtqs.select_bit          = 1;
    awtqs.use_menu            = 1;
    awtqs.ere_pos_fig         = "ere3";
    awtqs.where_pos_fig       = "where3";
    awtqs.by_pos_fig          = "by3";
    awtqs.qbox_pos_fig        = "qbox";
    awtqs.rescan_pos_fig      = 0;
    awtqs.key_pos_fig         = 0;
    awtqs.query_pos_fig       = "content";
    awtqs.result_pos_fig      = "result";
    awtqs.count_pos_fig       = "count";
    awtqs.do_query_pos_fig    = "doquery";
    awtqs.config_pos_fig      = "doconfig";
    awtqs.do_mark_pos_fig     = "domark";
    awtqs.do_unmark_pos_fig   = "dounmark";
    awtqs.do_delete_pos_fig   = "dodelete";
    awtqs.do_set_pos_fig      = "doset";
    awtqs.do_refresh_pos_fig  = "dorefresh";
    awtqs.open_parser_pos_fig = "openparser";
    awtqs.create_view_window  = (AW_CL)GEN_create_gene_window;
    awtqs.selector            = &GEN_item_selector;

    AW_CL cbs             = (AW_CL)awt_create_query_box((AW_window*)aws,&awtqs);
    gene_query_global_cbs = cbs;

    aws->create_menu(       0,   "More search",     "s" );
    aws->insert_menu_topic( "search_equal_fields_within_db","Search For Equal Fields and Mark Duplikates",          "E",0,  -1, (AW_CB)awt_search_equal_entries, cbs, 0 );
    aws->insert_menu_topic( "search_equal_words_within_db", "Search For Equal Words Between Fields and Mark Duplikates",    "W",0,  -1, (AW_CB)awt_search_equal_entries, cbs, 1 );

    aws->button_length(7);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");
    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"gene_search.hlp");
    aws->create_button("HELP","HELP","H");

    return (AW_window *)aws;

}


