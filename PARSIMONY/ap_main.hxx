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
//      stack-aware resource management

#define REUSE_EDGES

inline AP_tree_nlen *StackFrameData::makeNode(AP_pars_root *proot) {
    return created.put(new AP_tree_nlen(proot));
}
inline AP_tree_edge *StackFrameData::makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2) {
#if defined(REUSE_EDGES)
    if (destroyed.has_edges()) {
        AP_tree_edge *reuse = destroyed.getEdge();
        reuse->relink(n1, n2);
        return reuse;
    }
#endif
    return created.put(new AP_tree_edge(n1, n2));
}

inline void StackFrameData::destroyNode(AP_tree_nlen *node) { destroyed.put(node); }
inline void StackFrameData::destroyEdge(AP_tree_edge *edge) { destroyed.put(edge); }

inline AP_tree_nlen *AP_main::makeNode(AP_pars_root *proot) {
    return currFrame ? frameData->makeNode(proot) : new AP_tree_nlen(proot);
}
inline AP_tree_edge *AP_main::makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2) {
    return currFrame ? frameData->makeEdge(n1, n2) : new AP_tree_edge(n1, n2);
}
inline void AP_main::destroyNode(AP_tree_nlen *node) {
    if (currFrame) {
        ap_assert(!node->rightson && !node->leftson); // node needs to be unlinked from neighbours
        frameData->destroyNode(node);
    }
    else {
        ap_assert(!node->may_be_recollected());
        delete node; // here child nodes are destroyed
    }
}
inline void AP_main::destroyEdge(AP_tree_edge *edge) {
    ap_assert(!edge->is_linked());
    if (currFrame) {
        frameData->destroyEdge(edge);
    }
    else {
        delete edge;
    }
}

inline TreeNode *AP_pars_root::makeNode() const {
    return ap_main->makeNode(const_cast<AP_pars_root*>(this));
}
inline void AP_pars_root::destroyNode(TreeNode *node) const {
    ap_main->destroyNode(DOWNCAST(AP_tree_nlen*, node));
}

inline AP_tree_edge *makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2) {
    return ap_main->makeEdge(n1, n2);
}
inline void destroyEdge(AP_tree_edge *edge) {
    ap_main->destroyEdge(edge);
}


#else
#error ap_main.hxx included twice
#endif // AP_MAIN_HXX
