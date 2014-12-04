// =============================================================== //
//                                                                 //
//   File      : ap_main.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_MAIN_HXX
#define AP_MAIN_HXX

#ifndef AP_MAIN_TYPE_HXX
#include "ap_main_type.hxx"
#endif
#ifndef AP_TREE_NLEN_HXX
#include "ap_tree_nlen.hxx"
#endif


#define AWAR_ALIGNMENT        "tmp/pars/alignment"
#define AWAR_FILTER_NAME      "tmp/pars/filter/name"
#define AWAR_FILTER_FILTER    "tmp/pars/filter/filter"
#define AWAR_FILTER_ALIGNMENT "tmp/pars/filter/alignment"
#define AWAR_PARSIMONY        "tmp/pars/parsimony"
#define AWAR_BEST_PARSIMONY   "tmp/pars/best_parsimony"
#define AWAR_STACKPOINTER     "tmp/pars/stackpointer"

#define AWAR_PARS_TYPE      "pars/pars_type"

enum PARS_pars_type {
    PARS_WAGNER,
    PARS_TRANSVERSION
};

struct PARS_commands {
    bool add_marked;
    bool add_selected;
    bool calc_branch_lengths;
    bool calc_bootstrap;
    bool quit;

    PARS_commands()
        : add_marked(false)
        , add_selected(false)
        , calc_branch_lengths(false)
        , calc_bootstrap(false)
        , quit(false)
    {
    }
};

extern AP_main *ap_main; // @@@ elim

inline AP_tree_nlen *rootNode() {
    return ap_main->get_root_node();
}

inline AP_tree_edge *rootEdge() {
    return rootNode()->get_leftson()->edgeTo(rootNode()->get_rightson());
}

// ------------------------------------------
//      stack-aware ressource management

inline AP_tree_nlen *AP_main::makeNode(AP_pars_root *proot) {
    return new AP_tree_nlen(proot);
}
inline void AP_main::destroyNode(AP_tree_nlen *node) {
    delete node;
}

inline TreeNode *AP_pars_root::makeNode() const {
    return ap_main->makeNode(const_cast<AP_pars_root*>(this));
}

inline void AP_pars_root::destroyNode(TreeNode *node) const {
    ap_main->destroyNode(DOWNCAST(AP_tree_nlen*, node));
}

#else
#error ap_main.hxx included twice
#endif // AP_MAIN_HXX
