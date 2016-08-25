// =============================================================== //
//                                                                 //
//   File      : ED4_consensus.cxx                                 //
//   Purpose   : consensus control functions                       //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_seq_colors.hxx"

#include <aw_root.hxx>
#include <aw_awar.hxx>

#define CONSENSUS_AWAR_SOURCE CAS_EDIT4
#include <consensus.h>

// ------------------------
//      Build consensus

const ConsensusBuildParams& ED4_root::get_consensus_params() {
    if (!cons_param) cons_param = new ConsensusBuildParams(aw_root);
    return *cons_param;
}

void ED4_root::reset_consensus_params() {
    if (cons_param) {
        delete cons_param;
        cons_param = NULL;
    }
}

void ED4_consensus_definition_changed(AW_root*) {
    ED4_ROOT->reset_consensus_params();

    ED4_reference *ref = ED4_ROOT->reference;
    if (ref->reference_is_a_consensus()) {
        ref->data_changed_cb(NULL);
        ED4_ROOT->request_refresh_for_specific_terminals(LEV_SEQUENCE_STRING); // refresh all sequences
    }
    else {
        ED4_ROOT->request_refresh_for_consensus_terminals();
    }
}

static ARB_ERROR toggle_consensus_display(ED4_base *base, bool show) {
    if (base->is_consensus_manager()) {
        ED4_manager         *consensus_man = base->to_manager();
        ED4_spacer_terminal *spacer        = consensus_man->parent->get_defined_level(LEV_SPACER)->to_spacer_terminal();

        if (show) {
            consensus_man->unhide_children();
        }
        else {
            consensus_man->hide_children();
        }
        spacer->resize_dynamic();
    }

    return NULL;
}

void ED4_consensus_display_changed(AW_root *root) {
    bool show = root->awar(ED4_AWAR_CONSENSUS_SHOW)->read_int();
    ED4_ROOT->root_group_man->route_down_hierarchy(makeED4_route_cb(toggle_consensus_display, show)).expect_no_error();
}

