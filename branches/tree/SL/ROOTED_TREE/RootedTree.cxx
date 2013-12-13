// ================================================================ //
//                                                                  //
//   File      : RootedTree.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "RootedTree.h"
#include <map>

// ------------------
//      TreeRoot

TreeRoot::~TreeRoot() {
    delete rootNode;
    rt_assert(!rootNode);
    delete nodeMaker;
}

void TreeRoot::change_root(RootedTree *oldroot, RootedTree *newroot) {
    rt_assert(rootNode == oldroot);
    rootNode = newroot;

    if (oldroot && oldroot->get_tree_root() && !oldroot->is_inside(newroot)) oldroot->set_tree_root(0); // unlink from this
    if (newroot && newroot->get_tree_root() != this) newroot->set_tree_root(this); // link to this
}

// --------------------
//      RootedTree

bool RootedTree::is_inside(const RootedTree *subtree) const {
    return this == subtree || (father && get_father()->is_inside(subtree));
}

#if defined(CHECK_TREE_STRUCTURE)

void RootedTree::assert_valid() const {
    rt_assert(this);
    if (!is_leaf) {
        rt_assert(rightson);
        rt_assert(leftson);
        get_rightson()->assert_valid();
        get_leftson()->assert_valid();
    }

    TreeRoot *troot = get_tree_root();
    if (father) {
        rt_assert(is_inside(get_father()));
        if (troot) {
            rt_assert(troot->get_root_node()->is_anchestor_of(this));
        }
        else {
            rt_assert(get_father()->get_tree_root() == NULL); // if this has no root, father as well shouldn't have root
        }
    }
    else {                                          // this is root
        if (troot) {
            rt_assert(troot->get_root_node()  == this);
            rt_assert(!is_leaf);                    // leaf@root (tree has to have at least 2 leafs)
        }
    }
}
#endif // CHECK_TREE_STRUCTURE

void RootedTree::set_tree_root(TreeRoot *new_root) {
    if (tree_root != new_root) {
        tree_root = new_root;
        if (leftson) get_leftson()->set_tree_root(new_root);
        if (rightson) get_rightson()->set_tree_root(new_root);
    }
}

void RootedTree::reorder_subtree(TreeOrder mode) {
    static const char *smallest_leafname; // has to be set to the alphabetically smallest name (when function exits)

    if (is_leaf) {
        smallest_leafname = name;
    }
    else {
        int leftsize  = get_leftson() ->get_leaf_count();
        int rightsize = get_rightson()->get_leaf_count();

        bool swap_branches;
        {
            bool big_at_top    = leftsize>rightsize;
            bool big_at_bottom = leftsize<rightsize;

            swap_branches = (mode&BIG_BRANCHES_TO_BOTTOM) ? big_at_top : big_at_bottom;
        }

        if (swap_branches) swap_sons();

        TreeOrder lmode = mode;
        TreeOrder rmode = mode;

        if (mode & BIG_BRANCHES_TO_CENTER) {
            lmode = BIG_BRANCHES_TO_CENTER;
            rmode = TreeOrder(BIG_BRANCHES_TO_CENTER | BIG_BRANCHES_TO_BOTTOM);
        }

        get_leftson()->reorder_subtree(lmode);
        const char *leftleafname = smallest_leafname;

        get_rightson()->reorder_subtree(rmode);
        const char *rightleafname = smallest_leafname;

        if (leftleafname && rightleafname) {
            int name_cmp = strcmp(leftleafname, rightleafname);
            if (name_cmp <= 0) {
                smallest_leafname = leftleafname;
            }
            else {
                smallest_leafname = rightleafname;
                if (leftsize == rightsize) { // if sizes of subtrees are equal and rightleafname<leftleafname -> swap branches
                    swap_sons();
                }
            }
        }
    }
    rt_assert(smallest_leafname);
}

void RootedTree::reorder_tree(TreeOrder mode) {
    /*! beautify tree (does not change topology, only swaps branches)
     */
    compute_tree();
    reorder_subtree(mode);
}

void RootedTree::rotate_subtree() {
    if (!is_leaf) {
        swap_sons();
        get_leftson()->rotate_subtree();
        get_rightson()->rotate_subtree();
    }
}

// ----------------------------
//      find_innermost_edge

class NodeLeafDistance {
    GBT_LEN downdist, updist;
    enum { NLD_NODIST = 0, NLD_DOWNDIST, NLD_BOTHDIST } state;

public:

    NodeLeafDistance()
        : downdist(-1.0),
          updist(-1.0),
          state(NLD_NODIST)
    {}

    GBT_LEN get_downdist() const { rt_assert(state >= NLD_DOWNDIST); return downdist; }
    void set_downdist(GBT_LEN DownDist) {
        if (state < NLD_DOWNDIST) state = NLD_DOWNDIST;
        downdist = DownDist;
    }

    GBT_LEN get_updist() const { rt_assert(state >= NLD_BOTHDIST); return updist; }
    void set_updist(GBT_LEN UpDist) {
        if (state < NLD_BOTHDIST) state = NLD_BOTHDIST;
        updist = UpDist;
    }

};

class EdgeFinder {
    std::map<RootedTree*, NodeLeafDistance> data;

    ARB_edge innermost;
    GBT_LEN  min_maxDist;

    void insert_tree(RootedTree *node) {
        if (node->is_leaf) {
            data[node].set_downdist(0.0);
        }
        else {
            insert_tree(node->get_leftson());
            insert_tree(node->get_rightson());

            data[node].set_downdist(std::max(data[node->get_leftson()].get_downdist()+node->leftlen,
                                             data[node->get_rightson()].get_downdist()+node->rightlen));
        }
    }

    void findBetterEdge_sub(RootedTree *node) {
        RootedTree *father  = node->get_father();
        RootedTree *brother = node->get_brother();

        GBT_LEN len      = node->get_branchlength();
        GBT_LEN brothLen = brother->get_branchlength();

        GBT_LEN upDist   = std::max(data[father].get_updist(), data[brother].get_downdist()+brothLen);
        GBT_LEN downDist = data[node].get_downdist();

        GBT_LEN edgedist = std::max(upDist, downDist)+len/2;

        if (edgedist<min_maxDist) { // found better edge
            innermost   = ARB_edge(node, father);
            min_maxDist = edgedist;
        }

        data[node].set_updist(upDist+len);

        if (!node->is_leaf) {
            findBetterEdge_sub(node->get_leftson());
            findBetterEdge_sub(node->get_rightson());
        }
    }

    void findBetterEdge(RootedTree *node) {
        if (!node->is_leaf) {
            findBetterEdge_sub(node->get_leftson());
            findBetterEdge_sub(node->get_rightson());
        }
    }

public:
    EdgeFinder(RootedTree *rootNode)
        : innermost(rootNode->get_leftson(), rootNode->get_rightson()) // root-edge
    {
        insert_tree(rootNode);

        RootedTree *lson = rootNode->get_leftson();
        RootedTree *rson = rootNode->get_rightson();

        GBT_LEN rootEdgeLen = rootNode->leftlen + rootNode->rightlen;

        GBT_LEN lddist = data[lson].get_downdist();
        GBT_LEN rddist = data[rson].get_downdist();

        data[lson].set_updist(rddist+rootEdgeLen);
        data[rson].set_updist(lddist+rootEdgeLen);

        min_maxDist = std::max(lddist, rddist)+rootEdgeLen/2;

        findBetterEdge(lson);
        findBetterEdge(rson);
    }

    const ARB_edge& innermost_edge() const { return innermost; }
};

ARB_edge TreeRoot::find_innermost_edge() {
    EdgeFinder edgeFinder(get_root_node());
    return edgeFinder.innermost_edge();
}

