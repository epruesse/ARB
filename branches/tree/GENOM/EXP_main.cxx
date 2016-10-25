//  ==================================================================== //
//                                                                       //
//    File      : EXP_main.cxx                                           //
//    Purpose   :                                                        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2001        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "EXP_local.hxx"

#include <awt.hxx>
#include <awt_input_mask.hxx>
#include <aw_awars.hxx>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <db_query.h>
#include <rootAsWin.h>

using namespace std;

static void EXP_update_combined_cb(AW_root *awr) {
    char       *organism   = awr->awar(AWAR_ORGANISM_NAME)->read_string();
    char       *experiment = awr->awar(AWAR_EXPERIMENT_NAME)->read_string();
    const char *combined   = GBS_global_string("%s/%s", organism, experiment);
    awr->awar(AWAR_COMBINED_EXPERIMENT_NAME)->write_string(combined);
    free(experiment);
    free(organism);
}

void EXP_create_awars(AW_root *aw_root, AW_default /* aw_def */, GBDATA *gb_main) {
    aw_root->awar_string(AWAR_EXPERIMENT_NAME,          "", gb_main)->add_callback(EXP_update_combined_cb);
    aw_root->awar_string(AWAR_PROTEOM_NAME,             "", gb_main);
    aw_root->awar_string(AWAR_PROTEIN_NAME,             "", gb_main);
    aw_root->awar_string(AWAR_ORGANISM_NAME,            "", gb_main)->add_callback(EXP_update_combined_cb);
    aw_root->awar_string(AWAR_COMBINED_EXPERIMENT_NAME, "", gb_main);
    aw_root->awar_string(AWAR_SPECIES_NAME,             "", gb_main);
    aw_root->awar_string(AWAR_EXPERIMENT_DEST,          "", gb_main);
}

// ---------------------------------------
//      EXP_item_type_species_selector

struct EXP_item_type_species_selector : public awt_item_type_selector {
    EXP_item_type_species_selector() : awt_item_type_selector(AWT_IT_EXPERIMENT) {}
    ~EXP_item_type_species_selector() OVERRIDE {}

    const char *get_self_awar() const OVERRIDE {
        return AWAR_COMBINED_EXPERIMENT_NAME;
    }
    size_t get_self_awar_content_length() const OVERRIDE {
        return 12 + 1 + 40; // species-name+'/'+experiment_name
    }
    GBDATA *current(AW_root *root, GBDATA *gb_main) const OVERRIDE { // give the current item
        char   *species_name    = root->awar(AWAR_ORGANISM_NAME)->read_string();
        char   *experiment_name = root->awar(AWAR_EXPERIMENT_NAME)->read_string();
        GBDATA *gb_experiment   = 0;

        if (species_name[0] && experiment_name[0]) {
            GB_transaction ta(gb_main);
            GBDATA *gb_species = GBT_find_species(gb_main, species_name);
            if (gb_species) {
                gb_experiment = EXP_find_experiment(gb_species, experiment_name);
            }
        }

        free(experiment_name);
        free(species_name);

        return gb_experiment;
    }
    const char *getKeyPath() const OVERRIDE { // give the keypath for items
        return CHANGE_KEY_PATH_EXPERIMENTS;
    }
};

static EXP_item_type_species_selector item_type_experiment;

static void EXP_open_mask_window(AW_window *aww, int id, GBDATA *gb_main) {
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    exp_assert(descriptor);
    if (descriptor) {
        AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_experiment, descriptor->get_internal_maskname(), descriptor->is_local_mask());
    }
}

static void EXP_create_mask_submenu(AW_window_menu_modes *awm, GBDATA *gb_main) {
    AWT_create_mask_submenu(awm, AWT_IT_EXPERIMENT, EXP_open_mask_window, gb_main);
}

static AW_window *create_colorize_experiments_window(AW_root *aw_root, GBDATA *gb_main) {
    return QUERY::create_colorize_items_window(aw_root, gb_main, EXP_get_selector());
}

void EXP_create_experiments_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, bool submenu) {
    const char *title  = "Experiment";
    const char *hotkey = "x";

    if (submenu) awm->insert_sub_menu(title, hotkey);
    else awm->create_menu(title, hotkey, AWM_ALL);

    {
        awm->insert_menu_topic("experiment_info",                  "Experiment information", "i", "experiment_info.hlp",   AWM_ALL, RootAsWindowCallback::simple(EXP_popup_experiment_window,        gb_main));
        awm->insert_menu_topic(awm->local_id("experiment_search"), "Search and query",       "q", "experiment_search.hlp", AWM_ALL, makeCreateWindowCallback    (EXP_create_experiment_query_window, gb_main));

        EXP_create_mask_submenu(awm, gb_main);

        awm->sep______________();
        awm->insert_menu_topic(awm->local_id("experiment_colors"), "Colors ...", "C", "colorize.hlp", AWM_ALL, makeCreateWindowCallback(create_colorize_experiments_window, gb_main));
    }
    if (submenu) awm->close_sub_menu();
}

