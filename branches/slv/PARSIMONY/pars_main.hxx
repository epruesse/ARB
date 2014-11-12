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

#ifndef TREEDISPLAY_HXX
#include <TreeDisplay.hxx>
#endif

#define MIN_SEQUENCE_LENGTH 20

class AW_root;
class AWT_graphic_tree;
class WeightedFilter;
class AP_tree_nlen;
class arb_progress;

class ArbParsimony {
    AW_root          *awr;
    AWT_graphic_tree *tree;

public:
    ArbParsimony(AW_root *awroot) : awr(awroot), tree(NULL) {}

    AW_root *get_awroot() const { return awr; }
    AWT_graphic_tree *get_tree() const { return tree; }

    DEFINE_DOWNCAST_ACCESSORS(AP_tree_nlen, get_root_node, get_tree()->get_root_node());

    void generate_tree(WeightedFilter *pars_weighted_filter);
    void optimize_tree(AP_tree *tree, arb_progress& progress);
    void kernighan_optimize_tree(AP_tree *at);
};

void PARS_map_viewer(GBDATA *gb_species, AD_MAP_VIEWER_TYPE vtype);

#else
#error pars_main.hxx included twice
#endif // PARS_MAIN_HXX
