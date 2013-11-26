// =============================================================== //
//                                                                 //
//   File      : PH_matr.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "phylo.hxx"

#include <aw_file.hxx>
#include <aw_window.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <AP_pro_a_nucs.hxx>

void PH_create_matrix_variables(AW_root *aw_root, AW_default def) {
    aw_root->awar_string(AWAR_PHYLO_ALIGNMENT, "",       def);
    aw_root->awar_string(AWAR_PHYLO_FILTER_FILTER,    "",     def);
}

