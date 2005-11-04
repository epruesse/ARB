//  ==================================================================== //
//                                                                       //
//    File      : EXP_main.cxx                                           //
//    Purpose   :                                                        //
//    Time-stamp: <Fri Nov/04/2005 17:30 MET Coder@ReallySoft.de>        //
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
#include <awt_input_mask.hxx>
#include <aw_awars.hxx>
#include <arbdbt.h>
#include <ntree.hxx>

class AWT_canvas;
#include <../NTREE/nt_cb.hxx>

#include "EXP.hxx"
#include "EXP_local.hxx"
#include "EXP_interface.hxx"

using namespace std;

//  -------------------------------------------------------
//      void EXP_species_name_changed_cb(AW_root *awr)
//  -------------------------------------------------------
void EXP_species_name_changed_cb(AW_root */*awr*/) {
}

//  --------------------------------------------------
//      void EXP_update_combined_cb(AW_root *awr)
//  --------------------------------------------------
void EXP_update_combined_cb(AW_root *awr) {
    char       *organism   = awr->awar(AWAR_ORGANISM_NAME)->read_string();
    char       *experiment = awr->awar(AWAR_EXPERIMENT_NAME)->read_string();
    const char *combined   = GBS_global_string("%s/%s", organism, experiment);
    awr->awar(AWAR_COMBINED_EXPERIMENT_NAME)->write_string(combined);
    free(experiment);
    free(organism);
}

//  -------------------------------------------------------------------
//      void EXP_create_awars(AW_root *aw_root, AW_default aw_def)
//  -------------------------------------------------------------------
void EXP_create_awars(AW_root *aw_root, AW_default /*aw_def*/) {
    aw_root->awar_string(AWAR_EXPERIMENT_NAME, "" , gb_main)->add_callback((AW_RCB0)EXP_update_combined_cb);
    aw_root->awar_string(AWAR_PROTEOM_NAME, "" , gb_main);
    aw_root->awar_string(AWAR_PROTEIN_NAME, "" , gb_main);
    aw_root->awar_string(AWAR_ORGANISM_NAME, "" ,   gb_main)->add_callback((AW_RCB0)EXP_update_combined_cb);
    aw_root->awar_string(AWAR_COMBINED_EXPERIMENT_NAME, "" , gb_main);

    aw_root->awar_string(AWAR_SPECIES_NAME,"",gb_main)->add_callback((AW_RCB0)EXP_species_name_changed_cb);
    aw_root->awar_string(AWAR_EXPERIMENT_DEST, "" , gb_main);
}

//  -----------------------------------------------------------------------------
//      class EXP_item_type_species_selector : public awt_item_type_selector
//  -----------------------------------------------------------------------------
class EXP_item_type_species_selector : public awt_item_type_selector {
public:
    EXP_item_type_species_selector() : awt_item_type_selector(AWT_IT_EXPERIMENT) {}
    virtual ~EXP_item_type_species_selector() {}

    virtual const char *get_self_awar() const {
        return AWAR_COMBINED_EXPERIMENT_NAME;
    }
    virtual size_t get_self_awar_content_length() const {
        return 12 + 1 + 40; // species-name+'/'+experiment_name
    }
    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const { // add callbacks to awars
        root->awar(get_self_awar())->add_callback(f, cl_mask);
    }
    virtual void remove_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const { // add callbacks to awars
        root->awar(get_self_awar())->remove_callback(f, cl_mask);
    }
    virtual GBDATA *current(AW_root *root) const { // give the current item
        char   *species_name    = root->awar(AWAR_ORGANISM_NAME)->read_string();
        char   *experiment_name = root->awar(AWAR_EXPERIMENT_NAME)->read_string();
        GBDATA *gb_experiment   = 0;

        if (species_name[0] && experiment_name[0]) {
            GB_transaction dummy(gb_main);
            GBDATA *gb_species = GBT_find_species(gb_main,species_name);
            if (gb_species) {
                gb_experiment = EXP_find_experiment(gb_species, experiment_name);
            }
        }

        free(experiment_name);
        free(species_name);

        return gb_experiment;
    }
    virtual const char *getKeyPath() const { // give the keypath for items
        return CHANGE_KEY_PATH_EXPERIMENTS;
    }
};

static EXP_item_type_species_selector item_type_experiment;

//  -----------------------------------------------------------------------------
//      static void EXP_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL)
//  -----------------------------------------------------------------------------
static void EXP_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL) {
    int                              id         = int(cl_id);
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    exp_assert(descriptor);
    if (descriptor) AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_experiment, descriptor->get_internal_maskname(), descriptor->is_local_mask());
}

//  -----------------------------------------------------------------------
//      static void GEN_create_mask_submenu(AW_window_menu_modes *awm)
//  -----------------------------------------------------------------------
static void EXP_create_mask_submenu(AW_window_menu_modes *awm) {
    AWT_create_mask_submenu(awm, AWT_IT_EXPERIMENT, EXP_open_mask_window);
}

static AW_window *EXP_create_experiment_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, gb_main, &EXP_item_selector);
}

#define AWMIMT awm->insert_menu_topic
//  -------------------------------------------------------------------------------------
//      void EXP_create_experiments_submenu(AW_window_menu_modes *awm, bool submenu)
//  -------------------------------------------------------------------------------------
void EXP_create_experiments_submenu(AW_window_menu_modes *awm, bool submenu) {
    const char *title  = "Experiment";
    const char *hotkey = "x";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT( "experiment_info",   "Experiment information", "i", "experiment_info.hlp", AWM_ALL,EXP_popup_experiment_window, 0, 0);
//         AWMIMT( "experiment_info",   "Experiment information", "i", "experiment_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)EXP_create_experiment_window, 0 );
        AWMIMT( "experiment_search", "Search and query",       "q", "experiment_search.hlp", AWM_ALL,AW_POPUP,   (AW_CL)EXP_create_experiment_query_window, 0 );

        EXP_create_mask_submenu(awm);

        awm->insert_separator();
        AWMIMT( "experiment_colors",    "Colors ...",           "C",    "mark_colors.hlp", AWM_ALL,AW_POPUP,   (AW_CL)EXP_create_experiment_colorize_window, 0);
        awm->insert_separator();
        AWMIMT("pgt", "Proteom Genome Toolkit (PGT)", "P", "pgt.hlp", AWM_ALL, NT_system_cb, (AW_CL)"arb_pgt &", 0);
    }
    if (submenu) awm->close_sub_menu();
}

