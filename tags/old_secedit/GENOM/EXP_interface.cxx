//  ==================================================================== //
//                                                                       //
//    File      : EXP_interface.cxx                                      //
//    Purpose   :                                                        //
//    Time-stamp: <Wed May/25/2005 17:21 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2001        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_changekey.hxx>
#include <awt_sel_boxes.hxx>
#include <arbdbt.h>
#include <aw_awars.hxx>
#include <db_scanner.hxx>

#include "EXP.hxx"
#include "EXP_interface.hxx"
#include "EXP_local.hxx"

#include "GEN.hxx"
#include "GEN_local.hxx"
#include "GEN_interface.hxx"

#include "../NTREE/ad_spec.hxx"
#include <../NTREE/nt_internal.h>

#include <cstdlib>
#include <cstring>

using namespace std;

extern GBDATA *gb_main;

#define AD_F_ALL (AW_active)(-1)

//  -----------------------------------------------------------------------------------
//      GBDATA* EXP_get_current_experiment_data(GBDATA *gb_main, AW_root *aw_root)
//  -----------------------------------------------------------------------------------
GBDATA* EXP_get_current_experiment_data(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species         = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_experiment_data = 0;

    if (gb_species) gb_experiment_data = EXP_get_experiment_data(gb_species);

    return gb_experiment_data;
}
//  --------------------------------------------------------------------------------------------------------
//      static void EXP_select_experiment(GBDATA* /*gb_main*/, AW_root *aw_root, const char *item_name)
//  --------------------------------------------------------------------------------------------------------
static void EXP_select_experiment(GBDATA* /*gb_main*/, AW_root *aw_root, const char *item_name) {
    char *name  = GB_strdup(item_name);
    char *slash = strchr(name, '/');

    if (slash) {
        slash[0] = 0;
        aw_root->awar(AWAR_ORGANISM_NAME)->write_string(name);
        aw_root->awar(AWAR_EXPERIMENT_NAME)->write_string(slash+1);
    }
    free(name);
}

static char *EXP_get_experiment_id(GBDATA */*gb_main*/, GBDATA *gb_experiment) {
    GBDATA *gb_name = GB_find(gb_experiment, "name", 0, down_level);
    if (!gb_name) return 0;     // experiment  w/o name -> skip

    GBDATA *gb_species = GB_get_father(GB_get_father(gb_experiment));
    GBDATA *gb_sp_name = GB_find(gb_species, "name", 0, down_level);
    if (!gb_sp_name) return 0;  // species w/o name -> skip

    char *species_name = GB_read_as_string(gb_sp_name);
    char *experiment_name = GB_read_as_string(gb_name);

    char *result = (char*)malloc(strlen(species_name)+1+strlen(experiment_name)+1);
    strcpy(result, species_name);
    strcat(result, "/");
    strcat(result, experiment_name);

    free(experiment_name);
    free(species_name);

    return result;
}

static GBDATA *EXP_find_experiment_by_id(GBDATA *gb_main, const char *id) {
    char   *organism = GB_strdup(id);
    char   *exp     = strchr(organism, '/');
    GBDATA *result   = 0;

    if (exp) {
        *exp++ = 0;
        GBDATA *gb_organism = GEN_find_organism(gb_main, organism);
        if (gb_organism) {
            result = EXP_find_experiment(gb_organism, exp);
        }
    }

    free(organism);
    return result;    
}

static char *old_species_marks = 0; // configuration storing marked species

// extern "C" {
//     typedef  GB_ERROR (*species_callback)(GBDATA *gb_species, int *clientdata);
//     GB_ERROR GBT_with_stored_species(GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata);
// }

//  ---------------------------------------------------------------------------------------------------------------
//      static GBDATA *EXP_get_first_experiment_data(GBDATA *gb_main, AW_root *aw_root, AWT_QUERY_RANGE range)
//  ---------------------------------------------------------------------------------------------------------------
static GBDATA *EXP_get_first_experiment_data(GBDATA *gb_main, AW_root *aw_root, AWT_QUERY_RANGE range) {
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
    return gb_species ? EXP_get_experiment_data(gb_species) : 0;
}

//  -------------------------------------------------------------------------------------------------------
//      static GBDATA *EXP_get_next_experiment_data(GBDATA *gb_experiment_data, AWT_QUERY_RANGE range)
//  -------------------------------------------------------------------------------------------------------
static GBDATA *EXP_get_next_experiment_data(GBDATA *gb_experiment_data, AWT_QUERY_RANGE range) {
    GBDATA *gb_species = 0;
    switch (range) {
        case AWT_QUERY_CURRENT_SPECIES: {
            break;
        }
        case AWT_QUERY_MARKED_SPECIES: {
            GBDATA *gb_last_species = GB_get_father(gb_experiment_data);
            gb_species              = GEN_next_marked_organism(gb_last_species);

            if (!gb_species && old_species_marks) { // got all -> clean up
                GBT_restore_marked_species(gb_main, old_species_marks);
                free(old_species_marks);
                old_species_marks = 0;
            }

            break;
        }
        case AWT_QUERY_ALL_SPECIES: {
            GBDATA *gb_last_species = GB_get_father(gb_experiment_data);
            gb_species              = GBT_next_species(gb_last_species);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    return gb_species ? EXP_get_experiment_data(gb_species) : 0;
}


//  ------------------------------------------------------------
//      struct ad_item_selector EXP_item_selector =
//  ------------------------------------------------------------
struct ad_item_selector EXP_item_selector =
    {
        AWT_QUERY_ITEM_EXPERIMENTS,
        EXP_select_experiment,
        EXP_get_experiment_id,
        EXP_find_experiment_by_id,
        (AW_CB)awt_experiment_field_selection_list_update_cb,
        25,
        CHANGE_KEY_PATH_EXPERIMENTS,
        "experiment",
        "experiments",
        EXP_get_first_experiment_data,
        EXP_get_next_experiment_data,
        EXP_first_experiment_rel_experiment_data,
        EXP_next_experiment,
        EXP_get_current_experiment,
        &AWT_organism_selector
    };


//  ------------------------------------------------------------------------------
//      GBDATA *EXP_get_current_experiment(GBDATA *gb_main, AW_root *aw_root)
//  ------------------------------------------------------------------------------
GBDATA *EXP_get_current_experiment(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species    = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_experiment = 0;

    if (gb_species) {
        char *experiment_name = aw_root->awar(AWAR_EXPERIMENT_NAME)->read_string();
        gb_experiment         = EXP_find_experiment(gb_species,experiment_name);
        delete experiment_name;
    }

    return gb_experiment;
}

static AW_CL    ad_global_scannerid   = 0;
static AW_root *ad_global_scannerroot = 0;
AW_CL           experiment_query_global_cbs = 0;

//  ------------------------------------------------------------------------
//      AW_window *EXP_create_experiment_query_window(AW_root *aw_root)
//  ------------------------------------------------------------------------
AW_window *EXP_create_experiment_query_window(AW_root *aw_root) {

    static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "EXPERIMENT_QUERY", "Experiment SEARCH and QUERY");
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
    awtqs.create_view_window  = (AW_CL)EXP_create_experiment_window;
    awtqs.selector            = &EXP_item_selector;

    AW_CL cbs                   = (AW_CL)awt_create_query_box((AW_window*)aws,&awtqs);
    experiment_query_global_cbs = cbs;

    aws->create_menu(       0,   "More search",     "s" );
    aws->insert_menu_topic( "search_equal_fields_within_db","Search For Equal Fields and Mark Duplikates",          "E",0,  -1, (AW_CB)awt_search_equal_entries, cbs, 0 );
    aws->insert_menu_topic( "search_equal_words_within_db", "Search For Equal Words Between Fields and Mark Duplikates",    "W",0,  -1, (AW_CB)awt_search_equal_entries, cbs, 1 );

    aws->button_length(7);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");
    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"experiment_search.hlp");
    aws->create_button("HELP","HELP","H");

    return (AW_window *)aws;

}

//  -------------------------------------------------
//      void experiment_delete_cb(AW_window *aww)
//  -------------------------------------------------
void experiment_delete_cb(AW_window *aww){
    if (aw_message("Are you sure to delete the experiment","OK,CANCEL")) return;

    GB_begin_transaction(gb_main);

    GB_ERROR error          = 0;
    GBDATA   *gb_experiment = EXP_get_current_experiment(gb_main, aww->get_root()); // aw_root);

    if (gb_experiment) error = GB_delete(gb_experiment);
    else error               = "Please select a experiment first";

    if (!error) GB_commit_transaction(gb_main);
    else GB_abort_transaction(gb_main);

    if (error) aw_message(error);
}

//  -------------------------------------------------
//      void experiment_create_cb(AW_window *aww)
//  -------------------------------------------------
void experiment_create_cb(AW_window *aww){
    GB_begin_transaction(gb_main);

    GB_ERROR  error              = 0;
    AW_root  *aw_root            = aww->get_root();
    char     *dest               = aw_root->awar(AWAR_EXPERIMENT_DEST)->read_string();
    GBDATA   *gb_experiment_data = EXP_get_current_experiment_data(gb_main, aw_root);
    GBDATA   *gb_dest            = EXP_find_experiment_rel_experiment_data(gb_experiment_data, dest);

    if (!gb_experiment_data) error = "Please select a species first";
    else if (gb_dest) error  = GB_export_error("Sorry: experiment '%s' already exists", dest);
    else {
        gb_dest = EXT_create_experiment_rel_experiment_data(gb_experiment_data, dest);
        if (gb_dest) aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->write_string(dest);
        else error = GB_get_error();
    }
    if (!error) GB_commit_transaction(gb_main);
    else    GB_abort_transaction(gb_main);
    if (error) aw_message(error);
    delete dest;
}

//  -------------------------------------------------
//      void experiment_rename_cb(AW_window *aww)
//  -------------------------------------------------
void experiment_rename_cb(AW_window *aww){

    GB_ERROR  error   = 0;
    AW_root  *aw_root = aww->get_root();
    char     *source  = aw_root->awar(AWAR_EXPERIMENT_NAME)->read_string();
    char     *dest    = aw_root->awar(AWAR_EXPERIMENT_DEST)->read_string();

    if (strcmp(source, dest) != 0) {
        GB_begin_transaction(gb_main);

        GBDATA *gb_experiment_data = EXP_get_current_experiment_data(gb_main, aww->get_root());

        if (!gb_experiment_data) error = "Please select a species first";
        else {
            GBDATA *gb_source = EXP_find_experiment_rel_experiment_data(gb_experiment_data, source);
            GBDATA *gb_dest   = EXP_find_experiment_rel_experiment_data(gb_experiment_data, dest);

            if (!gb_source) error   = "Please select a experiment first";
            else if (gb_dest) error = GB_export_error("Sorry: experiment '%s' already exists", dest);
            else {
                GBDATA *gb_name = GB_search(gb_source, "name", GB_STRING);
                error           = GB_write_string(gb_name, dest);
                if (!error) aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->write_string(dest);
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

//  -----------------------------------------------
//      void experiment_copy_cb(AW_window *aww)
//  -----------------------------------------------
void experiment_copy_cb(AW_window *aww){
    GB_begin_transaction(gb_main);

    GB_ERROR  error        = 0;
    char     *source       = aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->read_string();
    char     *dest         = aww->get_root()->awar(AWAR_EXPERIMENT_DEST)->read_string();
    GBDATA   *gb_experiment_data = EXP_get_current_experiment_data(gb_main, aww->get_root());

    if (!gb_experiment_data) {
        error = "Please select a species first.";
    }
    else {
        GBDATA *gb_source = EXP_find_experiment_rel_experiment_data(gb_experiment_data, source);
        GBDATA *gb_dest   = EXP_find_experiment_rel_experiment_data(gb_experiment_data, dest);

        if (!gb_source) error   = "Please select a experiment first";
        else if (gb_dest) error = GB_export_error("Sorry: experiment '%s' already exists", dest);
        else {
            gb_dest = GB_create_container(gb_experiment_data,"experiment");
            error = GB_copy(gb_dest, gb_source);
            if (!error) {
                GBDATA *gb_name = GB_search(gb_dest,"name",GB_STRING);
                error = GB_write_string(gb_name,dest);
            }
            if (!error) {
                aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->write_string(dest);
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



//  -----------------------------------------------------------------
//      AW_window *create_experiment_rename_window(AW_root *root)
//  -----------------------------------------------------------------
AW_window *create_experiment_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "RENAME_EXPERIMENT", "EXPERIMENT RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the experiment");

    aws->at("input");
    aws->create_input_field(AWAR_EXPERIMENT_DEST,15);
    aws->at("ok");
    aws->callback(experiment_rename_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}
//  ---------------------------------------------------------------
//      AW_window *create_experiment_copy_window(AW_root *root)
//  ---------------------------------------------------------------
AW_window *create_experiment_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_EXPERIMENT", "EXPERIMENT COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new experiment");

    aws->at("input");
    aws->create_input_field(AWAR_EXPERIMENT_DEST,15);

    aws->at("ok");
    aws->callback(experiment_copy_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}
//  -----------------------------------------------------------------
//      AW_window *create_experiment_create_window(AW_root *root)
//  -----------------------------------------------------------------
AW_window *create_experiment_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_EXPERIMENT","EXPERIMENT CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label"); aws->create_autosize_button(0,"Please enter the name\nof the new experiment");
    aws->at("input"); aws->create_input_field(AWAR_EXPERIMENT_DEST,15);

    aws->at("ok");
    aws->callback(experiment_create_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

//  ------------------------------------------------------------------
//      void EXP_map_experiment(AW_root *aw_root, AW_CL scannerid)
//  ------------------------------------------------------------------
void EXP_map_experiment(AW_root *aw_root, AW_CL scannerid)
{
    GB_transaction  dummy(gb_main);
    GBDATA         *gb_experiment = EXP_get_current_experiment(gb_main, aw_root);

    if (gb_experiment) awt_map_arbdb_scanner(scannerid, gb_experiment, 0, CHANGE_KEY_PATH_EXPERIMENTS);
}


//  ----------------------------------------------------
//      void EXP_create_field_items(AW_window *aws)
//  ----------------------------------------------------
void EXP_create_field_items(AW_window *aws) {
    aws->insert_menu_topic("reorder_fields", "Reorder fields ...",    "R", "spaf_reorder.hlp", AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_list_reorder, (AW_CL)&EXP_item_selector); 
    aws->insert_menu_topic("delete_field",   "Delete/Hide Field ...", "D", "spaf_delete.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_delete, (AW_CL)&EXP_item_selector); 
    aws->insert_menu_topic("create_field",   "Create fields ...",     "C", "spaf_create.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_create, (AW_CL)&EXP_item_selector); 
    aws->insert_separator();
    aws->insert_menu_topic("unhide_fields", "Show all hidden fields", "S", "scandb.hlp", AD_F_ALL, (AW_CB)awt_experiment_field_selection_list_unhide_all_cb, (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_separator();
    aws->insert_menu_topic("scan_unknown_fields", "Scan unknown fields",   "u", "scandb.hlp", AD_F_ALL, (AW_CB)awt_experiment_field_selection_list_scan_unknown_cb,  (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_menu_topic("del_unused_fields",   "Remove unused fields",  "e", "scandb.hlp", AD_F_ALL, (AW_CB)awt_experiment_field_selection_list_delete_unused_cb, (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_menu_topic("refresh_fields",      "Refresh fields (both)", "f", "scandb.hlp", AD_F_ALL, (AW_CB)awt_experiment_field_selection_list_update_cb,        (AW_CL)gb_main, AWT_NDS_FILTER); 
}

//  ------------------------------------------------------------
//      void EXP_create_experiment_window(AW_root *aw_root)
//  ------------------------------------------------------------
AW_window *EXP_create_experiment_window(AW_root *aw_root) {
    static AW_window_simple_menu *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple_menu;
    aws->init( aw_root, "EXPERIMENT_INFORMATION", "EXPERIMENT INFORMATION");
    aws->load_xfig("ad_spec.fig");
    //     aws->load_xfig("experiment_info.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("search");
    aws->callback(AW_POPUP, (AW_CL)EXP_create_experiment_query_window, 0);
    aws->create_button("SEARCH","SEARCH","S");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"experiment_info.hlp");
    aws->create_button("HELP","HELP","H");


    AW_CL scannerid       = awt_create_arbdb_scanner(gb_main, aws, "box",0,"field","enable",AWT_VIEWER,0,"mark",AWT_NDS_FILTER, &EXP_item_selector);
    ad_global_scannerid   = scannerid;
    ad_global_scannerroot = aws->get_root();

    aws->create_menu(       0,   "EXPERIMENT",     "E", "spa_experiment.hlp",  AD_F_ALL );
    aws->insert_menu_topic("experiment_delete", "Delete",     "D","spa_delete.hlp",       AD_F_ALL,   (AW_CB)experiment_delete_cb, 0, 0);
    aws->insert_menu_topic("experiment_rename", "Rename ...", "R","spa_rename.hlp",   AD_F_ALL,   AW_POPUP, (AW_CL)create_experiment_rename_window, 0);
    aws->insert_menu_topic("experiment_copy",   "Copy ...",   "y","spa_copy.hlp",         AD_F_ALL,   AW_POPUP, (AW_CL)create_experiment_copy_window, 0);
    aws->insert_menu_topic("experiment_create", "Create ...", "C","spa_create.hlp",   AD_F_ALL,   AW_POPUP, (AW_CL)create_experiment_create_window, 0);
    aws->insert_separator();

    aws->create_menu(       0,   "FIELDS",     "F", "experiment_fields.hlp",  AD_F_ALL );
    EXP_create_field_items(aws);

    {
        Awar_Callback_Info    *cb_info     = new Awar_Callback_Info(aws->get_root(), AWAR_EXPERIMENT_NAME, EXP_map_experiment, scannerid); // do not delete!
        AW_detach_information *detach_info = new AW_detach_information(cb_info); // do not delete!
        cb_info->add_callback();

        aws->at("detach");
        aws->callback(NT_detach_information_window, (AW_CL)&aws, (AW_CL)cb_info);
        aws->create_button("DETACH", "DETACH", "D");

        detach_info->set_detach_button(aws->get_last_button_widget());
    }

    //     aws->get_root()->awar(AWAR_EXPERIMENT_NAME)->add_callback(EXP_map_experiment,scannerid);

    aws->show();
    EXP_map_experiment(aws->get_root(),scannerid);
    return (AW_window *)aws;
}

void EXP_popup_experiment_window(AW_window *aww, AW_CL, AW_CL) {
    AW_window *aws = EXP_create_experiment_window(aww->get_root());
    aws->show();
}

