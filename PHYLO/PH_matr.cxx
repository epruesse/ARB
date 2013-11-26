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

#if defined(WARN_TODO)
#warning module completely unused
#endif

#define CHECK_NAN(x) if ((!(x>=0.0)) && (!(x<0.0))) *(int *)0=0;

static AP_smatrix *global_ratematrix = 0; // @@@ unused?

static void set_globel_r_m_value(AW_root *aw_root, long i, long j) {
    char buffer[256];
    sprintf(buffer, AWAR_PHYLO_RATEMATRIX_VAL_TEMPLATE, i, j);
    global_ratematrix->set(i, j, aw_root->awar(buffer)->read_float());
}

void PH_create_matrix_variables(AW_root *aw_root, AW_default def) {
    aw_root->awar_string("phyl/which_species", "marked", def); // @@@ unused?
    aw_root->awar_string(AWAR_PHYLO_ALIGNMENT, "",       def);

    aw_root->awar_string(AWAR_PHYLO_FILTER_ALIGNMENT, "none", def);
    aw_root->awar_string(AWAR_PHYLO_FILTER_NAME,      "none", def); // @@@ unused?
    aw_root->awar_string(AWAR_PHYLO_FILTER_FILTER,    "",     def);

    aw_root->awar_string(AWAR_PHYLO_WEIGHTS_NAME,      "none", def); // @@@ unused?
    aw_root->awar_string(AWAR_PHYLO_WEIGHTS_ALIGNMENT, "none", def);

    aw_root->awar_string(AWAR_PHYLO_RATES_NAME,                "none", def); // @@@ unused?
    aw_root->awar_string(AWAR_PHYLO_CANCEL_CHARS,              ".",    def); // @@@ unused?
    aw_root->awar_string(AWAR_PHYLO_CORRECTION_TRANSFORMATION, "none", def); // @@@ unused?

    aw_root->awar(AWAR_PHYLO_FILTER_ALIGNMENT) ->map(AWAR_PHYLO_ALIGNMENT);
    aw_root->awar(AWAR_PHYLO_WEIGHTS_ALIGNMENT)->map(AWAR_PHYLO_ALIGNMENT);

    AW_create_fileselection_awars(aw_root, AWAR_PHYLO_SAVE_MATRIX, ".", "", "infile", def); // @@@ unused?
    aw_root->awar_string(AWAR_PHYLO_TREE_NAME, "tree_temp", def); // @@@ unused?
    aw_root->awar_float (AWAR_PHYLO_ALPHA,     1.0,         def); // @@@ unused?

    global_ratematrix = new AP_smatrix(AP_MAX);

    long i, j;
    for (i=0; i <AP_MAX; i++) {
        for (j=0; j <AP_MAX; j++) {
            if (i!=j) global_ratematrix->set(i, j, 1.0);
        }
    }
    for (i=AP_A; i <AP_MAX; i*=2) {
        for (j = AP_A; j < i; j*=2) {
            char buffer[256];
            sprintf(buffer, AWAR_PHYLO_RATEMATRIX_VAL_TEMPLATE, i, j);
            aw_root->awar_float(buffer, 1.0, def);
            aw_root->awar(buffer)->add_callback((AW_RCB)set_globel_r_m_value, (AW_CL)i, (AW_CL)j);
        }
    }
}

