// =============================================================== //
//                                                                 //
//   File      : pars_main.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PARS_MAIN_HXX
#define PARS_MAIN_HXX

#ifndef PARS_DTREE_HXX
#include "pars_dtree.hxx"
#endif

#define MIN_SEQUENCE_LENGTH 20

class WeightedFilter;
class AP_tree_nlen;

struct KL_Settings;

class ArbParsimony {
    AWT_graphic_parsimony *tree;

public:
    ArbParsimony() : tree(NULL) {}

    AWT_graphic_parsimony *get_tree() const { return tree; }

    DEFINE_READ_ACCESSORS(AP_tree_nlen*, get_root_node, get_tree()->get_root_node());

    void generate_tree(WeightedFilter *pars_weighted_filter);
    void set_tree(AWT_graphic_parsimony *tree_);

    void optimize_tree(AP_tree_nlen *at, const KL_Settings& settings, arb_progress& progress);
    void kernighan_optimize_tree(AP_tree_nlen *at, const KL_Settings& settings, const AP_FLOAT *pars_global_start, bool dumpPerf);
};

void PARS_map_viewer(GBDATA *gb_species, AD_MAP_VIEWER_TYPE vtype);

#else
#error pars_main.hxx included twice
#endif // PARS_MAIN_HXX
