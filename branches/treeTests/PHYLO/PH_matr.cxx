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

static AP_smatrix *global_ratematrix = 0;

static void set_globel_r_m_value(AW_root *aw_root, long i, long j) {
    char buffer[256];
    sprintf(buffer, "phyl/ratematrix/val_%li_%li", i, j);
    global_ratematrix->set(i, j, aw_root->awar(buffer)->read_float());
}

void PH_create_matrix_variables(AW_root *aw_root, AW_default def)
{
    aw_root->awar_string("tmp/dummy_string", "0", def);


    aw_root->awar_string("phyl/which_species", "marked", def);
    aw_root->awar_string("phyl/alignment", "", def);

    aw_root->awar_string("phyl/filter/alignment", "none", def);
    aw_root->awar_string("phyl/filter/name", "none", def);
    aw_root->awar_string("phyl/filter/filter", "", def);

    aw_root->awar_string("phyl/weights/name", "none", def);
    aw_root->awar_string("phyl/weights/alignment", "none", def);

    aw_root->awar_string("phyl/rates/name", "none", def);
    aw_root->awar_string("phyl/cancel/chars", ".", def);
    aw_root->awar_string("phyl/correction/transformation", "none", def);

    aw_root->awar("phyl/filter/alignment")->map("phyl/alignment");
    aw_root->awar("phyl/weights/alignment")->map("phyl/alignment");

    AW_create_fileselection_awars(aw_root, "tmp/phyl/save_matrix", ".", "", "infile", def);

    aw_root->awar_string("phyl/tree/tree_name", "tree_temp", def);

    aw_root->awar_float("phyl/alpha", 1.0, def);

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
            sprintf(buffer, "phyl/ratematrix/val_%li_%li", i, j);
            aw_root->awar_float(buffer, 1.0, def);
            aw_root->awar(buffer)->add_callback((AW_RCB)set_globel_r_m_value, (AW_CL)i, (AW_CL)j);
        }
    }
}

