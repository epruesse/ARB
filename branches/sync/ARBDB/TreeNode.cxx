// ================================================================ //
//                                                                  //
//   File      : TreeNode.cxx                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "TreeNode.h"
#include <map>
#include <set>
#include <cmath> // needed with 4.4.3 (but not with 4.7.3)

// ------------------
//      TreeRoot

TreeRoot::~TreeRoot() {
    deleteWithNodes = false; // avoid recursive call of ~TreeRoot (obsolete?)
    rt_assert(!rootNode);    // you have to call TreeRoot::predelete() before dtor! you can do this is dtor of that derived class, which defines makeNode/destroyNode
    // Note: destroying nodes from here is impossible (leads to pure virtual call, as derived class instance of 'this' is already under destruction)
}

void TreeRoot::change_root(TreeNode *oldroot, TreeNode *newroot) {
    rt_assert(rootNode == oldroot);
    rt_assert(implicated(newroot, !newroot->father));
    rootNode = newroot;

    if (oldroot && oldroot->get_tree_root() && !oldroot->is_inside(newroot)) oldroot->set_tree_root(0); // unlink from this
    if (newroot && newroot->get_tree_root() != this) newroot->set_tree_root(this); // link to this
}

// --------------------
//      TreeNode

#if defined(PROVIDE_TREE_STRUCTURE_TESTS)

Validity TreeNode::is_valid() const {
    rt_assert(knownNonNull(this));
    Validity valid;

    TreeRoot *troot = get_tree_root();
    if (troot) {
        if (is_leaf) {
            valid = Validity(!rightson && !leftson, "leaf has son");
        }
        else {
            valid            = Validity(rightson && leftson, "inner node lacks sons");
            if (valid) valid = get_rightson()->is_valid();
            if (valid) valid = get_leftson()->is_valid();
        }
        if (father) {
            if (valid) valid = Validity(is_inside(get_father()), "node not inside father subtree");
            if (valid) valid = Validity(troot->get_root_node()->is_anchestor_of(this), "root is not nodes anchestor");
            if (valid) valid = Validity(get_father()->get_tree_root() == troot, "node and father have different TreeRoot");
        }
        else {
            if (valid) valid = Validity(troot->get_root_node() == this, "node has no father, but isn't root-node");
            if (valid) valid = Validity(!is_leaf, "root-node is leaf"); // leaf@root (tree has to have at least 2 leafs)
        }
    }
    else { // removed node (may be incomplete)
        if (is_leaf) {
            valid = Validity(!rightson && !leftson, "(removed) leaf has son");
        }
        else {
            if (rightson)         valid = get_rightson()->is_valid();
            if (leftson && valid) valid = get_leftson()->is_valid();
        }
        if (father) {
            if (valid) valid = Validity(is_inside(get_father()), "(removed) node not inside father subtree");
            if (valid) valid = Validity(get_father()->get_tree_root() == troot, "(removed) node and father have different TreeRoot");
        }
    }
    return valid;
}
#endif // PROVIDE_TREE_STRUCTURE_TESTS

void TreeNode::set_tree_root(TreeRoot *new_root) {
    if (tree_root != new_root) {
        tree_root = new_root;
        if (leftson) get_leftson()->set_tree_root(new_root);
        if (rightson) get_rightson()->set_tree_root(new_root);
    }
}

void TreeNode::reorder_subtree(TreeOrder mode) {
    static const char *smallest_leafname; // has to be set to the alphabetically smallest name (when function exits)

    if (is_leaf) {
        smallest_leafname = name;
    }
    else {
        int leftsize  = get_leftson() ->get_leaf_count();
        int rightsize = get_rightson()->get_leaf_count();

        {
            bool big_at_top    = leftsize>rightsize;
            bool big_at_bottom = leftsize<rightsize;
            bool swap_branches = (mode&ORDER_BIG_DOWN) ? big_at_top : big_at_bottom;
            if (swap_branches) swap_sons();
        }

        TreeOrder lmode, rmode;
        if (mode & (ORDER_BIG_TO_EDGE|ORDER_BIG_TO_CENTER)) { // symmetric
            if (mode & ORDER_ALTERNATING) mode = TreeOrder(mode^(ORDER_BIG_TO_EDGE|ORDER_BIG_TO_CENTER));

            if (mode & ORDER_BIG_TO_CENTER) {
                lmode = TreeOrder(mode | ORDER_BIG_DOWN);
                rmode = TreeOrder(mode & ~ORDER_BIG_DOWN);
            }
            else {
                lmode = TreeOrder(mode & ~ORDER_BIG_DOWN);
                rmode = TreeOrder(mode | ORDER_BIG_DOWN);
            }
        }
        else { // asymmetric
            if (mode & ORDER_ALTERNATING) mode = TreeOrder(mode^ORDER_BIG_DOWN);

            lmode = mode;
            rmode = mode;
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
                    const char *smallest_leafname_save = smallest_leafname;

                    swap_sons();
                    get_leftson ()->reorder_subtree(lmode); rt_assert(strcmp(smallest_leafname, rightleafname)== 0);
                    get_rightson()->reorder_subtree(rmode); rt_assert(strcmp(smallest_leafname, leftleafname) == 0);

                    smallest_leafname = smallest_leafname_save;
                }
            }
        }
    }
    rt_assert(smallest_leafname);
}

void TreeNode::reorder_tree(TreeOrder mode) {
    /*! beautify tree (does not change topology, only swaps branches)
     */
    compute_tree();
    reorder_subtree(mode);
}

void TreeNode::rotate_subtree() {
    if (!is_leaf) {
        swap_sons();
        get_leftson()->rotate_subtree();
        get_rightson()->rotate_subtree();
    }
}

void TreeNode::set_root() {
    /*! set the root at parent edge of this
     * pointers to tree-nodes remain valid, but all parent-nodes of this change their meaning
     * (afterwards they will point to [father+brother] instead of [this+brother])
     * esp. pointers to the root-node will still point to the root-node (which only changed children)
     */

    if (at_root()) return; // already root

    TreeNode *old_root    = get_root_node();
    TreeNode *old_brother = is_inside(old_root->get_leftson()) ? old_root->get_rightson() : old_root->get_leftson();

    // move remark branches to top
    {
        char *remark = nulldup(get_remark());
        for (TreeNode *node = this; node->father; node = node->get_father()) {
            remark = node->swap_remark(remark);
        }
        free(remark);
    }

    GBT_LEN old_root_len = old_root->leftlen + old_root->rightlen;

    // new node & this init
    old_root->leftson  = this;
    old_root->rightson = father; // will be set later

    if (father->leftson == this) {
        old_root->leftlen = old_root->rightlen = father->leftlen*.5;
    }
    else {
        old_root->leftlen = old_root->rightlen = father->rightlen*.5;
    }

    TreeNode *next = get_father()->get_father();
    TreeNode *prev = old_root;
    TreeNode *pntr = get_father();

    if (father->leftson == this)    father->leftson = old_root; // to set the flag correctly

    // loop from father to son of root, rotate tree
    while  (next->father) {
        double len = (next->leftson == pntr) ? next->leftlen : next->rightlen;

        if (pntr->leftson == prev) {
            pntr->leftson = next;
            pntr->leftlen = len;
        }
        else {
            pntr->rightson = next;
            pntr->rightlen = len;
        }

        pntr->father = prev;
        prev         = pntr;
        pntr         = next;
        next         = next->get_father();
    }
    // now next points to the old root, which has been destroyed above
    //
    // pointer at oldroot
    // pntr == brother before old root == next

    if (pntr->leftson == prev) {
        pntr->leftlen = old_root_len;
        pntr->leftson = old_brother;
    }
    else {
        pntr->rightlen = old_root_len;
        pntr->rightson = old_brother;
    }

    old_brother->father = pntr;
    pntr->father        = prev;
    father              = old_root;

    rt_assert(get_root_node() == old_root);
}

TreeNode *TreeNode::findLeafNamed(const char *wantedName) {
    TreeNode *found = NULL;
    if (is_leaf) {
        if (name && strcmp(name, wantedName) == 0) found = this;
    }
    else {
        found             = get_leftson()->findLeafNamed(wantedName);
        if (!found) found = get_rightson()->findLeafNamed(wantedName);
    }
    return found;
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
    std::map<TreeNode*, NodeLeafDistance> data; // maximum distance to farthest leaf

    ARB_edge innermost;
    double   min_distdiff; // abs diff between up- and downdiff

    GBT_LEN calc_distdiff(GBT_LEN d1, GBT_LEN d2) { return fabs(d1-d2); }

    void insert_tree(TreeNode *node) {
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

    void findBetterEdge_sub(TreeNode *node) {
        TreeNode *father  = node->get_father();
        TreeNode *brother = node->get_brother();

        GBT_LEN len      = node->get_branchlength();
        GBT_LEN brothLen = brother->get_branchlength();

        GBT_LEN upDist   = std::max(data[father].get_updist(), data[brother].get_downdist()+brothLen);
        GBT_LEN downDist = data[node].get_downdist();

        {
            GBT_LEN edge_dd = calc_distdiff(upDist, downDist);
            if (edge_dd<min_distdiff) { // found better edge
                innermost    = ARB_edge(node, father);
                min_distdiff = edge_dd;
            }
        }

        data[node].set_updist(upDist+len);

        if (!node->is_leaf) {
            findBetterEdge_sub(node->get_leftson());
            findBetterEdge_sub(node->get_rightson());
        }
    }

    void findBetterEdge(TreeNode *node) {
        if (!node->is_leaf) {
            findBetterEdge_sub(node->get_leftson());
            findBetterEdge_sub(node->get_rightson());
        }
    }

public:
    EdgeFinder(TreeNode *rootNode)
        : innermost(rootNode->get_leftson(), rootNode->get_rightson()) // root-edge
    {
        insert_tree(rootNode);

        TreeNode *lson = rootNode->get_leftson();
        TreeNode *rson = rootNode->get_rightson();

        GBT_LEN rootEdgeLen = rootNode->leftlen + rootNode->rightlen;

        GBT_LEN lddist = data[lson].get_downdist();
        GBT_LEN rddist = data[rson].get_downdist();

        data[lson].set_updist(rddist+rootEdgeLen);
        data[rson].set_updist(lddist+rootEdgeLen);

        min_distdiff = calc_distdiff(lddist, rddist);

        findBetterEdge(lson);
        findBetterEdge(rson);
    }

    const ARB_edge& innermost_edge() const { return innermost; }
};

ARB_edge TreeRoot::find_innermost_edge() {
    EdgeFinder edgeFinder(get_root_node());
    return edgeFinder.innermost_edge();
}

// ------------------------
//      multifurcation

class TreeNode::LengthCollector {
    typedef std::map<TreeNode*,GBT_LEN> LengthMap;
    typedef std::set<TreeNode*>         NodeSet;

    LengthMap eliminatedParentLength;
    LengthMap addedParentLength;

public:
    void eliminate_parent_edge(TreeNode *node) {
        rt_assert(!node->is_root_node());
        eliminatedParentLength[node] += parentEdge(node).eliminate();
    }

    void add_parent_length(TreeNode *node, GBT_LEN addLen) {
        rt_assert(!node->is_root_node());
        addedParentLength[node] += addLen;
    }

    void independent_distribution() {
        // step 2: (see caller)
        while (!eliminatedParentLength.empty()) { // got eliminated lengths which need to be distributed
            for (LengthMap::iterator from = eliminatedParentLength.begin(); from != eliminatedParentLength.end(); ++from) {
                ARB_edge elimEdge = parentEdge(from->first);
                GBT_LEN  elimLen  = from->second;

                elimEdge.virtually_distribute_length(elimLen, *this);
            }
            eliminatedParentLength.clear(); // all distributed!

            // handle special cases where distributed length is negative and results in negative destination branches.
            // Avoid generating negative dest. branchlengths by
            // - eliminating the dest. branch
            // - redistributing the additional (negative) length (may cause additional negative lengths on other dest. branches)

            NodeSet handled;
            for (LengthMap::iterator to = addedParentLength.begin(); to != addedParentLength.end(); ++to) {
                ARB_edge affectedEdge     = parentEdge(to->first);
                GBT_LEN  additionalLen    = to->second;
                double   effective_length = affectedEdge.length() + additionalLen;

                if (effective_length<=0.0) { // negative or zero
                    affectedEdge.set_length(effective_length);
                    eliminate_parent_edge(to->first); // adds entry to eliminatedParentLength and causes another additional loop
                    handled.insert(to->first);
                }
            }

            // remove all redistributed nodes
            for (NodeSet::iterator del = handled.begin(); del != handled.end(); ++del) {
                addedParentLength.erase(*del);
            }
        }

        // step 3:
        for (LengthMap::iterator to = addedParentLength.begin(); to != addedParentLength.end(); ++to) {
            ARB_edge affectedEdge     = parentEdge(to->first);
            GBT_LEN  additionalLen    = to->second;
            double   effective_length = affectedEdge.length() + additionalLen;

            affectedEdge.set_length(effective_length);
        }
    }
};

GBT_LEN ARB_edge::adjacent_distance() const {
    //! return length of edges starting from this->dest()

    if (at_leaf()) return 0.0;
    return next().length_or_adjacent_distance() + otherNext().length_or_adjacent_distance();
}

void ARB_edge::virtually_add_or_distribute_length_forward(GBT_LEN len, TreeNode::LengthCollector& collect) const {
    rt_assert(!is_nan_or_inf(len));
    if (length() > 0.0) {
        collect.add_parent_length(son(), len);
    }
    else {
        if (len != 0.0) virtually_distribute_length_forward(len, collect);
    }
}


void ARB_edge::virtually_distribute_length_forward(GBT_LEN len, TreeNode::LengthCollector& collect) const {
    /*! distribute length to edges adjacent in edge direction (i.e. edges starting from this->dest())
     * Split 'len' proportional to adjacent edges lengths.
     *
     * Note: length will not be distributed to tree-struction itself (yet), but collected in 'collect'
     */

    rt_assert(is_normal(len));
    rt_assert(!at_leaf()); // cannot forward anything (nothing beyond leafs)

    ARB_edge e1 = next();
    ARB_edge e2 = otherNext();

    GBT_LEN d1 = e1.length_or_adjacent_distance();
    GBT_LEN d2 = e2.length_or_adjacent_distance();

    len = len/(d1+d2);

    e1.virtually_add_or_distribute_length_forward(len*d1, collect);
    e2.virtually_add_or_distribute_length_forward(len*d2, collect);
}

void ARB_edge::virtually_distribute_length(GBT_LEN len, TreeNode::LengthCollector& collect) const {
    /*! distribute length to all adjacent edges.
     * Longer edges receive more than shorter ones.
     *
     * Edges with length zero will not be changed, instead both edges "beyond"
     * the edge will be affected (they will be affected equally to direct edges,
     * because edges at multifurcations are considered to BE direct edges).
     *
     * Note: length will not be distributed to tree-struction itself (yet), but collected in 'collect'
     */

    ARB_edge backEdge = inverse();
    GBT_LEN len_fwd, len_bwd;
    {
        GBT_LEN dist_fwd = adjacent_distance();
        GBT_LEN dist_bwd = backEdge.adjacent_distance();
        GBT_LEN lenW     = len/(dist_fwd+dist_bwd);
        len_fwd          = lenW*dist_fwd;
        len_bwd          = lenW*dist_bwd;

    }

    if (is_normal(len_fwd)) virtually_distribute_length_forward(len_fwd, collect);
    if (is_normal(len_bwd)) backEdge.virtually_distribute_length_forward(len_bwd, collect);
}

void TreeNode::eliminate_and_collect(const multifurc_limits& below, LengthCollector& collect) {
    /*! eliminate edges specified by 'below' and
     * store their length in 'collect' for later distribution.
     */
    rt_assert(!is_root_node());
    if (!is_leaf || below.applyAtLeafs) {
        double value;
        switch (parse_bootstrap(value)) {
            case REMARK_NONE:
                value = 100.0;
                // fall-through
            case REMARK_BOOTSTRAP:
                if (value<below.bootstrap && get_branchlength_unrooted()<below.branchlength) {
                    collect.eliminate_parent_edge(this);
                }
                break;

            case REMARK_OTHER: break;
        }
    }

    if (!is_leaf) {
        get_leftson() ->eliminate_and_collect(below, collect);
        get_rightson()->eliminate_and_collect(below, collect);
    }
}

void ARB_edge::multifurcate() {
    /*! eliminate edge and distribute length to adjacent edges
     * - sets its length to zero
     * - removes remark (bootstrap)
     * - distributes length to neighbour-branches
     */
    TreeNode::LengthCollector collector;
    collector.eliminate_parent_edge(son());
    collector.independent_distribution();
}
void TreeNode::multifurcate() {
    /*! eliminate branch from 'this' to 'father' (or brother @ root)
     * @see ARB_edge::multifurcate()
     */
    rt_assert(father); // cannot multifurcate at root; call with son of root to multifurcate root-edge
    if (father) parentEdge(this).multifurcate();
}

void TreeNode::set_branchlength_preserving(GBT_LEN new_len) {
    /*! set branchlength to 'new_len' while preserving overall distance in tree.
     *
     * Always works on unrooted tree (i.e. lengths @ root are treated correctly).
     * Length is preserved as in multifurcate()
     */

    GBT_LEN old_len = get_branchlength_unrooted();
    GBT_LEN change  = new_len-old_len;

    char *old_remark = nulldup(get_remark());

    // distribute the negative 'change' to neighbours:
    set_branchlength_unrooted(-change);
    multifurcate();

    set_branchlength_unrooted(new_len);
    use_as_remark(old_remark); // restore remark (was removed by multifurcate())
}

void TreeNode::multifurcate_whole_tree(const multifurc_limits& below) {
    /*! multifurcate all branches specified by 'below'
     * - step 1: eliminate all branches, store eliminated lengths
     * - step 2: calculate length distribution for all adjacent branches (proportionally to original length of each branch)
     * - step 3: apply distributed length
     */
    TreeNode      *root = get_root_node();
    LengthCollector  collector;

    // step 1:
    root->get_leftson()->eliminate_and_collect(below, collector);
    root->get_rightson()->eliminate_and_collect(below, collector);
    // root-edge is handled twice by the above calls.
    // Unproblematic: first call will eliminate root-edge, so second call will do nothing.

    // step 2 and 3:
    collector.independent_distribution();
}

TreeNode::bs100_mode TreeNode::toggle_bootstrap100(bs100_mode mode) {
    if (!is_leaf) {
        if (!is_root_node()) {
            double bootstrap;
            switch (parse_bootstrap(bootstrap)) {
                case REMARK_NONE:
                case REMARK_OTHER:
                    switch (mode) {
                        case BS_UNDECIDED: mode = BS_INSERT;
                        case BS_INSERT: set_bootstrap(100);
                        case BS_REMOVE: break;
                    }
                    break;
                case REMARK_BOOTSTRAP:
                    if (bootstrap >= 99.5) {
                        switch (mode) {
                            case BS_UNDECIDED: mode = BS_REMOVE;
                            case BS_REMOVE: remove_remark();
                            case BS_INSERT: break;
                        }
                    }
                    break;
            }
        }

        mode = get_leftson()->toggle_bootstrap100(mode);
        mode = get_rightson()->toggle_bootstrap100(mode);
    }
    return mode;
}
void TreeNode::remove_bootstrap() {
    freenull(remark_branch);
    if (!is_leaf) {
        get_leftson()->remove_bootstrap();
        get_rightson()->remove_bootstrap();
    }
}
void TreeNode::reset_branchlengths() {
    if (!is_leaf) {
        leftlen = rightlen = DEFAULT_BRANCH_LENGTH;

        get_leftson()->reset_branchlengths();
        get_rightson()->reset_branchlengths();
    }
}

void TreeNode::scale_branchlengths(double factor) {
    if (!is_leaf) {
        leftlen  *= factor;
        rightlen *= factor;

        get_leftson()->scale_branchlengths(factor);
        get_rightson()->scale_branchlengths(factor);
    }
}

GBT_LEN TreeNode::sum_child_lengths() const {
    if (is_leaf) return 0.0;
    return
        leftlen +
        rightlen +
        get_leftson()->sum_child_lengths() +
        get_rightson()->sum_child_lengths();
}

void TreeNode::bootstrap2branchlen() {
    //! copy bootstraps to branchlengths
    if (is_leaf) {
        set_branchlength_unrooted(DEFAULT_BRANCH_LENGTH);
    }
    else {
        if (father) {
            double         bootstrap;
            GBT_RemarkType rtype = parse_bootstrap(bootstrap);

            if (rtype == REMARK_BOOTSTRAP) {
                double len = bootstrap/100.0;
                set_branchlength_unrooted(len);
            }
            else {
                set_branchlength_unrooted(1.0); // no bootstrap means "100%"
            }
        }
        get_leftson()->bootstrap2branchlen();
        get_rightson()->bootstrap2branchlen();
    }
}

void TreeNode::branchlen2bootstrap() {
    //! copy branchlengths to bootstraps
    remove_remark();
    if (!is_leaf) {
        if (!is_root_node()) {
            set_bootstrap(get_branchlength_unrooted()*100.0);
        }
        get_leftson()->branchlen2bootstrap();
        get_rightson()->branchlen2bootstrap();
    }
}

TreeNode *TreeNode::fixDeletedSon() {
    // fix node after one son has been deleted
    TreeNode *result = NULL;

    if (leftson) {
        gb_assert(!rightson);
        result  = get_leftson();
        leftson = NULL;
    }
    else {
        gb_assert(!leftson);
        gb_assert(rightson);

        result   = get_rightson();
        rightson = NULL;
    }

    // now 'result' contains the lasting tree
    result->father = father;

    if (remark_branch && !result->remark_branch) { // rescue remarks if possible
        result->remark_branch    = remark_branch;
        remark_branch = NULL;
    }
    if (gb_node && !result->gb_node) { // rescue group if possible
        result->gb_node = gb_node;
        gb_node         = NULL;
    }

    if (!result->father) {
        get_tree_root()->change_root(this, result);
    }

    gb_assert(!is_root_node());

    forget_origin();
    forget_relatives();
    delete this;

    return result;
}

const TreeNode *TreeNode::ancestor_common_with(const TreeNode *other) const {
    if (this == other) return this;
    if (is_anchestor_of(other)) return this;
    if (other->is_anchestor_of(this)) return other;
    return get_father()->ancestor_common_with(other->get_father());
}

