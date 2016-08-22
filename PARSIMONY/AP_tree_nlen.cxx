// =============================================================== //
//                                                                 //
//   File      : AP_tree_nlen.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in Summer 1995    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ap_tree_nlen.hxx"
#include "pars_debug.hxx"

#include <AP_seq_dna.hxx>
#include <aw_root.hxx>

using namespace std;

// ---------------------------------
//      Section base operations:
// ---------------------------------

AP_UPDATE_FLAGS AP_tree_nlen::check_update() {
    AP_UPDATE_FLAGS res = AP_tree::check_update();

    return res == AP_UPDATE_RELOADED ? AP_UPDATE_OK : res;
}

void AP_tree_nlen::copy(AP_tree_nlen *tree) {
    // like = operator
    // but copies sequence if is leaf

    this->is_leaf = tree->is_leaf;
    this->leftlen = tree->leftlen;
    this->rightlen = tree->rightlen;
    this->gb_node = tree->gb_node;

    if (tree->name != NULL) {
        this->name = strdup(tree->name);
    }
    else {
        this->name = NULL;
    }

    if (is_leaf) {
        ap_assert(tree->get_seq()); /* oops - AP_tree_nlen expects to have sequences at leafs!
                                     * did you forget to remove_leafs() ? */

        set_seq(tree->get_seq());
        // dangerous - no copy, just moves pointer
        // will result in undefined behavior

        ap_assert(0); //  this will not work, but is only used in GA_genetic.
                      //  Use some kind of SmartPtr there!
    }
}

ostream& operator<<(ostream& out, const AP_tree_nlen& node) {
    out << ' ';

    if (&node==NULL) {
        out << "NULL";
    }
    if (node.is_leaf) {
        out << ((void *)&node) << '(' << node.name << ')';
    }
    else {
        static int notTooDeep;

        if (notTooDeep) {
            out << ((void *)&node);
            if (!node.father) out << " (ROOT)";
        }
        else {
            notTooDeep = 1;

            out << "NODE(" << ((void *)&node);

            if (!node.father) {
                out << " (ROOT)";
            }
            else {
                out << ", father=" << node.father;
            }

            out << ", leftson=" << node.leftson
                << ", rightson=" << node.rightson
                << ", edge[0]=" << *(node.edge[0])
                << ", edge[1]=" << *(node.edge[1])
                << ", edge[2]=" << *(node.edge[2])
                << ")";

            notTooDeep = 0;
        }
    }

    return out << ' ';
}

int AP_tree_nlen::unusedEdgeIndex() const {
    for (int e=0; e<3; e++) if (edge[e]==NULL) return e;
    return -1;
}

AP_tree_edge* AP_tree_nlen::edgeTo(const AP_tree_nlen *neighbour) const {
    for (int e=0; e<3; e++) {
        if (edge[e]!=NULL && edge[e]->node[1-index[e]]==neighbour) {
            return edge[e];
        }
    }
    return NULL;
}

AP_tree_edge* AP_tree_nlen::nextEdge(const AP_tree_edge *afterThatEdge) const {
    /*! @return one edge of 'this'
     *
     * @param afterThatEdge
     * - if == NULL -> returns the "first" edge (edge[0])
     * - otherwise -> returns the next edge following 'afterThatEdge' in the array edge[]
     */
    return edge[afterThatEdge ? ((indexOf(afterThatEdge)+1) % 3) : 0];
}

void AP_tree_nlen::unlinkAllEdges(AP_tree_edge **edgePtr1, AP_tree_edge **edgePtr2, AP_tree_edge **edgePtr3)
{
    ap_assert(edge[0]!=NULL);
    ap_assert(edge[1]!=NULL);
    ap_assert(edge[2]!=NULL);

    *edgePtr1 = edge[0]->unlink();
    *edgePtr2 = edge[1]->unlink();
    *edgePtr3 = edge[2]->unlink();
}

void AP_tree_nlen::linkAllEdges(AP_tree_edge *edge1, AP_tree_edge *edge2, AP_tree_edge *edge3)
{
    ap_assert(edge[0]==NULL);
    ap_assert(edge[1]==NULL);
    ap_assert(edge[2]==NULL);

    edge1->relink(this, get_father()->get_father() ? get_father() : get_brother());
    edge2->relink(this, get_leftson());
    edge3->relink(this, get_rightson());
}

// -----------------------------
//      Check tree structure

#if defined(PROVIDE_TREE_STRUCTURE_TESTS)

inline const AP_tree_edge *edge_between(const AP_tree_nlen *node1, const AP_tree_nlen *node2) {
    AP_tree_edge *edge_12 = node1->edgeTo(node2);

#if defined(ASSERTION_USED)
    AP_tree_edge *edge_21 = node2->edgeTo(node1);
    ap_assert(edge_12 == edge_21); // nodes should agree about their edge
#endif

    return edge_12;
}

void AP_tree_nlen::assert_edges_valid() const {
    if (get_father()) {                     // root has no edges
        if (get_father()->is_root_node()) { // sons of root have one edge between them
            ap_assert(edge_between(this, get_brother()));
        }
        else {
            ap_assert(edge_between(this, get_father()));
            if (!is_leaf) {
                ap_assert(edge_between(this, get_leftson()));
                ap_assert(edge_between(this, get_rightson()));
            }
        }
    }

    if (!is_leaf) {
        get_leftson()->assert_edges_valid();
        get_rightson()->assert_edges_valid();
    }
}

void AP_tree_nlen::assert_valid() const {
    ap_assert(this);
    assert_edges_valid();
    AP_tree::assert_valid();
}

#endif // PROVIDE_TREE_STRUCTURE_TESTS

// -------------------------
//      Tree operations:
//
// insert
// remove
// swap
// set_root
// move
// costs


inline void push_all_upnode_sequences(AP_tree_nlen *nodeBelow) {
    for  (AP_tree_nlen *upnode = nodeBelow->get_father();
          upnode;
          upnode = upnode->get_father())
    {
        ap_main->push_node(upnode, SEQUENCE);
    }
}

inline void sortOldestFirst(AP_tree_edge **e1, AP_tree_edge **e2) {
    if ((*e1)->Age() > (*e2)->Age()) {
        swap(*e1, *e2);
    }
}

inline void sortOldestFirst(AP_tree_edge **e1, AP_tree_edge **e2, AP_tree_edge **e3) {
    sortOldestFirst(e1, e2);
    sortOldestFirst(e2, e3);
    sortOldestFirst(e1, e2);
}

void AP_tree_nlen::initial_insert(AP_tree_nlen *newBrother, AP_tree_root *troot) {
    // construct initial tree from 'this' and 'newBrother'
    // (both have to be leafs)

    ap_assert(newBrother);
    ap_assert(is_leaf);
    ap_assert(newBrother->is_leaf);

    AP_tree::initial_insert(newBrother, troot);
    new AP_tree_edge(newBrother, this); // build the root edge

    ASSERT_VALID_TREE(this->get_father());
}

void AP_tree_nlen::insert(AP_tree_nlen *newBrother) {
    //  inserts 'this' (a new node) at the father-edge of 'newBrother'
    ap_assert(newBrother);

    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(newBrother);

    ap_main->push_node(newBrother, STRUCTURE);

    AP_tree_nlen *brothersFather = newBrother->get_father();
    if (brothersFather) {
        ap_main->push_node(brothersFather, BOTH);
        push_all_upnode_sequences(brothersFather);

        if (brothersFather->get_father()) {
            AP_tree_edge *oldEdge = newBrother->edgeTo(newBrother->get_father())->unlink();
            AP_tree::insert(newBrother);
            oldEdge->relink(get_father(), get_father()->get_father());
        }
        else { // insert to son of root
            AP_tree_edge *oldEdge = newBrother->edgeTo(newBrother->get_brother())->unlink();
            AP_tree::insert(newBrother);
            oldEdge->relink(get_father(), get_father()->get_brother());
        }

        new AP_tree_edge(this, get_father());
        new AP_tree_edge(get_father(), newBrother);

        ASSERT_VALID_TREE(get_father()->get_father());
    }
    else { // insert at root
        ap_assert(!newBrother->is_leaf); // either swap 'this' and 'newBrother' or use initial_insert() to construct the initial tree

        AP_tree_nlen *lson = newBrother->get_leftson();
        AP_tree_nlen *rson = newBrother->get_rightson();

        ap_main->push_node(lson, STRUCTURE);
        ap_main->push_node(rson, STRUCTURE);

        AP_tree_edge *oldEdge = lson->edgeTo(rson)->unlink();

        AP_tree::insert(newBrother);

        oldEdge->relink(this, newBrother);
        new AP_tree_edge(newBrother, rson);
        new AP_tree_edge(newBrother, lson);

        ASSERT_VALID_TREE(get_father());
    }
}

void AP_tree_nlen::remove() {
    // Removes the node and its father from the tree:
    //
    //       grandpa                grandpa
    //           /                    /
    //          /                    /
    //    father        =>        brother
    //       /     \                                            .
    //      /       \                                           .
    //   this       brother
    //
    // One of the edges is relinked between brother and grandpa.
    // The other two edges are lost. This is not very relevant in respect to
    // memory usage because very few remove()s are really performed - the majority
    // is undone by a pop().
    // In the last case the two unlinked edges will be re-used, cause their
    // memory location was stored in the tree-stack.

    AP_tree_nlen *oldBrother = get_brother();

    ASSERT_VALID_TREE(this);

    ap_assert(father); // can't remove complete tree,

    ap_main->push_node(this, STRUCTURE);
    ap_main->push_node(oldBrother, STRUCTURE);
    push_all_upnode_sequences(get_father());

    AP_tree_edge *oldEdge;
    AP_tree_nlen *grandPa = get_father()->get_father();
    if (grandPa) {
        ASSERT_VALID_TREE(grandPa);

        ap_main->push_node(get_father(), BOTH);
        ap_main->push_node(grandPa, STRUCTURE);

        edgeTo(get_father())->unlink();                 // LOST_EDGE
        get_father()->edgeTo(oldBrother)->unlink();     // LOST_EDGE

        if (grandPa->father) {
            oldEdge = get_father()->edgeTo(grandPa)->unlink();
            AP_tree::remove();
            oldEdge->relink(oldBrother, grandPa);
        }
        else { // remove grandson of root
            AP_tree_nlen *uncle = get_father()->get_brother();
            ap_main->push_node(uncle, STRUCTURE);

            oldEdge = get_father()->edgeTo(uncle)->unlink();
            AP_tree::remove();
            oldEdge->relink(oldBrother, uncle);
        }
        ASSERT_VALID_TREE(grandPa);
    }
    else {                                          // remove son of root
        AP_tree_nlen *oldRoot = get_father();
        ASSERT_VALID_TREE(oldRoot);

        if (oldBrother->is_leaf) {
            //           root
            //            oo
            //           o  o
            //          o    o
            // oldBrother --- this         ----->   NULL
            //
            ap_main->push_node(oldRoot, ROOT);

            edgeTo(oldBrother)->unlink();           // LOST_EDGE

#if defined(ASSERTION_USED)
            AP_tree_root *troot = get_tree_root();
#endif // ASSERTION_USED
            AP_tree::remove();
            ap_assert(!troot->get_root_node()); // tree should have been removed
        }
        else {
            //
            //           root
            //            oo                                                              .
            //           o  o                                     root (=oldBrother)
            //          o    o                                     oo                      .
            // oldBrother --- this          ----->                o  o                     .
            //       /\                                          o    o                    .
            //      /  \                                     lson ----- rson
            //     /    \                                                                .
            //    lson  rson
            //
            AP_tree_nlen *lson = oldBrother->get_leftson();
            AP_tree_nlen *rson = oldBrother->get_rightson();

            ap_assert(lson && rson);

            ap_main->push_node(lson, STRUCTURE);
            ap_main->push_node(rson, STRUCTURE);
            ap_main->push_node(oldRoot, ROOT);

            edgeTo(oldBrother)->unlink();           // LOST_EDGE
            oldBrother->edgeTo(lson)->unlink();     // LOST_EDGE

            oldEdge = oldBrother->edgeTo(rson)->unlink();
            AP_tree::remove();
            oldEdge->relink(lson, rson);

            ap_assert(lson->get_tree_root()->get_root_node() == oldBrother);
            ASSERT_VALID_TREE(oldBrother);
        }
    }

    father = NULL;
    set_tree_root(NULL);

    ASSERT_VALID_TREE(this);
}

void AP_tree_nlen::swap_sons() {
    ap_assert(!is_leaf); // cannot swap leafs

    ap_main->push_node(this, STRUCTURE);
    AP_tree::swap_sons();
}

void AP_tree_nlen::swap_assymetric(AP_TREE_SIDE mode) {
    // mode AP_LEFT exchanges leftson with brother
    // mode AP_RIGHT exchanges rightson with brother

    ap_assert(!is_leaf);                            // cannot swap leafs
    ap_assert(father);                              // cannot swap root (has no brother)
    ap_assert(mode == AP_LEFT || mode == AP_RIGHT); // illegal mode

    AP_tree_nlen *oldBrother = get_brother();
    AP_tree_nlen *movedSon   = mode == AP_LEFT ? get_leftson() : get_rightson();

    if (!father->father) {
        // son of root case
        // take leftson of brother to exchange with

        if (!oldBrother->is_leaf) { // swap needed ?
            AP_tree_nlen *nephew = oldBrother->get_leftson();

            ap_main->push_node(this, BOTH);
            ap_main->push_node(movedSon, STRUCTURE);
            ap_main->push_node(get_father(), SEQUENCE);
            ap_main->push_node(nephew, STRUCTURE);
            ap_main->push_node(oldBrother, BOTH);

            AP_tree_edge *edge1 = edgeTo(movedSon)->unlink();
            AP_tree_edge *edge2 = oldBrother->edgeTo(nephew)->unlink();

            AP_tree::swap_assymetric(mode);

            edge1->relink(this, nephew);
            edge2->relink(oldBrother, movedSon);
        }
    }
    else {
        ap_main->push_node(this, BOTH);
        ap_main->push_node(get_father(), BOTH);
        ap_main->push_node(oldBrother, STRUCTURE);
        ap_main->push_node(movedSon, STRUCTURE);

        push_all_upnode_sequences(get_father());

        AP_tree_edge *edge1 = edgeTo(movedSon)->unlink();
        AP_tree_edge *edge2 = get_father()->edgeTo(oldBrother)->unlink();

        AP_tree::swap_assymetric(mode);

        edge1->relink(this, oldBrother);
        edge2->relink(get_father(), movedSon);
    }
}

void AP_tree_nlen::set_root() {
    if (at_root()) return; // already root

    // from this to root buffer the nodes
    ap_main->push_node(this,  STRUCTURE);

    AP_tree_nlen *old_brother = 0;
    AP_tree_nlen *old_root    = 0;
    {
        AP_tree_nlen *pntr;
        for (pntr = get_father(); pntr->father; pntr = pntr->get_father()) {
            ap_main->push_node(pntr, BOTH);
            old_brother = pntr;
        }
        old_root = pntr;
    }

    if (old_brother) {
        old_brother = old_brother->get_brother();
        ap_main->push_node(old_brother,  STRUCTURE);
    }

    ap_main->push_node(old_root, ROOT);
    AP_tree::set_root();
}

void AP_tree_nlen::moveNextTo(AP_tree_nlen *newBrother, AP_FLOAT rel_pos) {
    ap_assert(father);
    ap_assert(newBrother->father);

    ASSERT_VALID_TREE(rootNode());

    // push everything that will be modified onto stack
    ap_main->push_node(this,  STRUCTURE);
    ap_main->push_node(get_brother(), STRUCTURE);

    if (father->father) {
        AP_tree_nlen *grandpa = get_father()->get_father();

        ap_main->push_node(get_father(), BOTH);

        if (grandpa->father) {
            ap_main->push_node(grandpa, BOTH);
            push_all_upnode_sequences(grandpa);
        }
        else { // grandson of root
            ap_main->push_node(grandpa, ROOT);
            ap_main->push_node(get_father()->get_brother(), STRUCTURE);
        }
    }
    else { // son of root
        ap_main->push_node(get_father(), ROOT);

        if (!get_brother()->is_leaf) {
            ap_main->push_node(get_brother()->get_leftson(), STRUCTURE);
            ap_main->push_node(get_brother()->get_rightson(), STRUCTURE);
        }
    }

    ap_main->push_node(newBrother,  STRUCTURE);
    if (newBrother->father) {
        if (newBrother->father->father) {
            ap_main->push_node(newBrother->get_father(), BOTH);
        }
        else { // move to son of root
            ap_main->push_node(newBrother->get_father(), BOTH);
            ap_main->push_node(newBrother->get_brother(), STRUCTURE);
        }
        push_all_upnode_sequences(newBrother->get_father());
    }

    AP_tree_nlen *thisFather        = get_father();
    AP_tree_nlen *grandFather       = thisFather->get_father();
    AP_tree_nlen *oldBrother        = get_brother();
    AP_tree_nlen *newBrothersFather = newBrother->get_father();
    int           edgesChange       = ! (father==newBrother || newBrother->father==father);
    AP_tree_edge *e1, *e2, *e3;

    if (edgesChange) {
        if (thisFather==newBrothersFather->get_father()) { // son -> son of brother
            if (grandFather) {
                if (grandFather->get_father()) {
                    thisFather->unlinkAllEdges(&e1, &e2, &e3);
                    AP_tree_edge *e4 = newBrother->edgeTo(oldBrother)->unlink();

                    AP_tree::moveNextTo(newBrother, rel_pos);

                    sortOldestFirst(&e1, &e2, &e3);
                    e1->relink(oldBrother, grandFather); // use oldest edge at remove position
                    thisFather->linkAllEdges(e2, e3, e4);
                }
                else { // grandson of root -> son of brother
                    AP_tree_nlen *uncle = thisFather->get_brother();

                    thisFather->unlinkAllEdges(&e1, &e2, &e3);
                    AP_tree_edge *e4 = newBrother->edgeTo(oldBrother)->unlink();

                    AP_tree::moveNextTo(newBrother, rel_pos);

                    sortOldestFirst(&e1, &e2, &e3);
                    e1->relink(oldBrother, uncle);
                    thisFather->linkAllEdges(e2, e3, e4);
                }
            }
            else { // son of root -> grandson of root
                oldBrother->unlinkAllEdges(&e1, &e2, &e3);
                AP_tree::moveNextTo(newBrother, rel_pos);
                thisFather->linkAllEdges(e1, e2, e3);
            }
        }
        else if (grandFather==newBrothersFather) { // son -> brother of father
            if (grandFather->father) {
                thisFather->unlinkAllEdges(&e1, &e2, &e3);
                AP_tree_edge *e4 = grandFather->edgeTo(newBrother)->unlink();

                AP_tree::moveNextTo(newBrother, rel_pos);

                sortOldestFirst(&e1, &e2, &e3);
                e1->relink(oldBrother, grandFather);
                thisFather->linkAllEdges(e2, e3, e4);
            }
            else { // no edges change if we move grandson of root -> son of root
                AP_tree::moveNextTo(newBrother, rel_pos);
            }
        }
        else {
            //  now we are sure, the minimal distance
            //  between 'this' and 'newBrother' is 4 edges
            //  or if the root-edge is between them, the
            //  minimal distance is 3 edges

            if (!grandFather) { // son of root
                oldBrother->unlinkAllEdges(&e1, &e2, &e3);
                AP_tree_edge *e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                AP_tree::moveNextTo(newBrother, rel_pos);

                sortOldestFirst(&e1, &e2, &e3);
                e1->relink(oldBrother->get_leftson(), oldBrother->get_rightson()); // new root-edge
                thisFather->linkAllEdges(e2, e3, e4);   // old root
            }
            else if (!grandFather->get_father()) { // grandson of root
                if (newBrothersFather->get_father()->get_father()==NULL) { // grandson of root -> grandson of root
                    thisFather->unlinkAllEdges(&e1, &e2, &e3);
                    AP_tree_edge *e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                    AP_tree::moveNextTo(newBrother, rel_pos);

                    sortOldestFirst(&e1, &e2, &e3);
                    e1->relink(oldBrother, newBrothersFather);  // new root-edge
                    thisFather->linkAllEdges(e2, e3, e4);
                }
                else {
                    AP_tree_nlen *uncle = thisFather->get_brother();

                    thisFather->unlinkAllEdges(&e1, &e2, &e3);
                    AP_tree_edge *e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                    AP_tree::moveNextTo(newBrother, rel_pos);

                    sortOldestFirst(&e1, &e2, &e3);
                    e1->relink(oldBrother, uncle);
                    thisFather->linkAllEdges(e2, e3, e4);
                }
            }
            else {
                if (newBrothersFather->get_father()==NULL) { // move to son of root
                    AP_tree_nlen *newBrothersBrother = newBrother->get_brother();

                    thisFather->unlinkAllEdges(&e1, &e2, &e3);
                    AP_tree_edge *e4 = newBrother->edgeTo(newBrothersBrother)->unlink();

                    AP_tree::moveNextTo(newBrother, rel_pos);

                    sortOldestFirst(&e1, &e2, &e3);
                    e1->relink(oldBrother, grandFather);
                    thisFather->linkAllEdges(e2, e3, e4);
                }
                else { // simple independent move
                    thisFather->unlinkAllEdges(&e1, &e2, &e3);
                    AP_tree_edge *e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                    AP_tree::moveNextTo(newBrother, rel_pos);

                    sortOldestFirst(&e1, &e2, &e3);
                    e1->relink(oldBrother, grandFather);
                    thisFather->linkAllEdges(e2, e3, e4);
                }
            }
        }
    }
    else { // edgesChange==0
        AP_tree::moveNextTo(newBrother, rel_pos);
    }

    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(rootNode());
}

void AP_tree_nlen::unhash_sequence() {
    /*! removes the parsimony sequence from an inner node
     * (has no effect for leafs)
     */

    AP_sequence *sequence = get_seq();
    if (sequence && !is_leaf) sequence->forget_sequence();
}

bool AP_tree_nlen::clear(unsigned long datum, unsigned long user_buffer_count) {
    // returns
    // - true           if the first element is removed
    // - false          if it is copied into the previous level
    // according if user_buffer is greater than datum (wot?)

    if (stack_level != datum) {
        ap_assert(0); // internal control number check failed
        return false;
    }

    AP_tree_buffer * buff = stack.pop();
    bool             result;

    if (buff->controll == datum - 1 || user_buffer_count >= datum) {
        // previous node is buffered

        if (buff->mode & SEQUENCE) delete buff->sequence;

        stack_level = buff->controll;
        delete  buff;
        result      = true;
    }
    else {
        stack_level = datum - 1;
        stack.push(buff);
        result      = false;
    }

    return result;
}


bool AP_tree_nlen::push(AP_STACK_MODE mode, unsigned long datum) {
    // according to mode
    // tree_structure or sequence is buffered in the node

    AP_tree_buffer *new_buff;
    bool            ret;

    if (is_leaf && !(STRUCTURE & mode)) return false;    // tips push only structure

    if (this->stack_level == datum) {
        AP_tree_buffer *last_buffer = stack.get_first();
        AP_sequence    *sequence    = get_seq();

        if (sequence && (mode & SEQUENCE)) sequence->forget_sequence();
        if (0 == (mode & ~last_buffer->mode)) { // already buffered
            return false;
        }
        new_buff = last_buffer;
        ret = false;
    }
    else {
        new_buff           = new AP_tree_buffer;
        new_buff->count    = 1;
        new_buff->controll = stack_level;
        new_buff->mode     = NOTHING;

        stack.push(new_buff);
        this->stack_level = datum;
        ret = true;
    }

    if ((mode & STRUCTURE) && !(new_buff->mode & STRUCTURE)) {
        new_buff->father   = get_father();
        new_buff->leftson  = get_leftson();
        new_buff->rightson = get_rightson();
        new_buff->leftlen  = leftlen;
        new_buff->rightlen = rightlen;
        new_buff->root     = get_tree_root();
        new_buff->gb_node  = gb_node;
        new_buff->distance = distance;

        for (int e=0; e<3; e++) {
            new_buff->edge[e]      = edge[e];
            new_buff->edgeIndex[e] = index[e];
            if (edge[e]) {
                new_buff->edgeData[e]  = edge[e]->data;
            }
        }
    }

    if ((mode & SEQUENCE) && !(new_buff->mode & SEQUENCE)) {
        AP_sequence *sequence   = take_seq();
        new_buff->sequence      = sequence;
        new_buff->mutation_rate = mutation_rate;
        mutation_rate           = 0.0;
    }

    new_buff->mode = (AP_STACK_MODE)(new_buff->mode|mode);

    return ret;
}

void AP_tree_nlen::pop(unsigned long IF_ASSERTION_USED(datum)) { // pop old tree costs
    ap_assert(stack_level == datum); // error in node stack

    AP_tree_buffer *buff = stack.pop();
    AP_STACK_MODE   mode = buff->mode;

    if (mode&STRUCTURE) {
        father   = buff->father;
        leftson  = buff->leftson;
        rightson = buff->rightson;
        leftlen  = buff->leftlen;
        rightlen = buff->rightlen;
        set_tree_root(buff->root);
        gb_node  = buff->gb_node;
        distance = buff->distance;

        for (int e=0; e<3; e++) {
            edge[e] = buff->edge[e];

            if (edge[e]) {
                index[e] = buff->edgeIndex[e];

                edge[e]->index[index[e]] = e;
                edge[e]->node[index[e]]  = this;
                edge[e]->data            = buff->edgeData[e];
            }
        }
    }

    if (mode&SEQUENCE) {
        replace_seq(buff->sequence);
        mutation_rate = buff->mutation_rate;
    }

    if (ROOT==mode) {
        buff->root->change_root(buff->root->get_root_node(), this);
    }

    stack_level = buff->controll;
    delete buff;
}

void AP_tree_nlen::parsimony_rek(char *mutPerSite) {
    AP_sequence *sequence = get_seq();

    if (is_leaf) {
        ap_assert(sequence); // tree w/o aliview?
        sequence->ensure_sequence_loaded();
    }
    else {
        if (!sequence) {
            sequence = set_seq(get_tree_root()->get_seqTemplate()->dup());
            ap_assert(sequence);
        }

        if (!sequence->got_sequence()) {
            AP_tree_nlen *lson = get_leftson();
            AP_tree_nlen *rson = get_rightson();

            ap_assert(lson);
            ap_assert(rson);

            lson->parsimony_rek(mutPerSite);
            rson->parsimony_rek(mutPerSite);

            AP_sequence *lseq = lson->get_seq();
            AP_sequence *rseq = rson->get_seq();

            ap_assert(lseq);
            ap_assert(rseq);

            AP_FLOAT mutations_for_combine = sequence->combine(lseq, rseq, mutPerSite);
            mutation_rate                  = lson->mutation_rate + rson->mutation_rate + mutations_for_combine;
        }
    }
}

AP_FLOAT AP_tree_nlen::costs(char *mutPerSite) {
    // returns costs of a tree ( = number of mutations)

    ap_assert(get_tree_root()->get_seqTemplate());  // forgot to set_seqTemplate() ?  (previously returned 0.0 in this case)
    parsimony_rek(mutPerSite);
    return mutation_rate;
}

AP_FLOAT AP_tree_nlen::nn_interchange_rek(int deep, AP_BL_MODE mode, bool skip_hidden)
{
    if (!father)
    {
        return rootEdge()->nni_rek(deep, skip_hidden, mode, NULL);
    }

    if (!father->father)
    {
        AP_tree_edge *e = rootEdge();

        return e->nni_rek(deep, skip_hidden, mode, e->otherNode(this));
    }

    return edgeTo(get_father())->nni_rek(deep, skip_hidden, mode, get_father());
}


void AP_tree_nlen::kernighan_rek(int rek_deep, int *rek_2_width, int rek_2_width_max, const int rek_deep_max,
                                 double(*function) (double, double *, int), double *param_liste, int param_anz,
                                 AP_FLOAT pars_best, AP_FLOAT pars_start, AP_FLOAT pars_prev,
                                 AP_KL_FLAG rek_width_type, bool *abort_flag)
{
    //
    // rek_deep         Rekursionstiefe
    // rek_2_width      Verzweigungsgrad
    // neg_counter      zaehler fuer die Aeste in denen Kerninghan Lin schon angewendet wurde
    // function         Funktion fuer den dynamischen Schwellwert
    // pars_            Verschiedene Parsimonywerte

    AP_FLOAT help, pars[8];
    // acht parsimony werte
    AP_tree_nlen * pars_refpntr[8];
    // zeiger auf die naechsten Aeste
    int             help_ref, pars_ref[8];
    // referenzen auf die vertauschten parsimonies
    static AP_TREE_SIDE pars_side_ref[8];
    // linker oder rechter ast
    int             i, t, bubblesort_change = 0;
    //
    int             rek_width, rek_width_static = 0, rek_width_dynamic = 0;
    AP_FLOAT        schwellwert = function(rek_deep, param_liste, param_anz) + pars_start;

    // parameterausgabe

    if (rek_deep >= rek_deep_max || is_leaf || *abort_flag)   return;

    // Referenzzeiger auf die vier Kanten und zwei swapmoeglichkeiten initialisieren
    AP_tree_nlen *this_brother = this->get_brother();
    if (rek_deep == 0) {
        for (i = 0; i < 8; i+=2) {
            pars_side_ref[i] = AP_LEFT;
            pars_side_ref[i+1] = AP_RIGHT;
        }
        pars_refpntr[0] = pars_refpntr[1] = this;
        pars_refpntr[2] = pars_refpntr[3] = 0;
        pars_refpntr[4] = pars_refpntr[5] = 0;
        pars_refpntr[6] = pars_refpntr[7] = 0;
    }
    else {
        pars_refpntr[0] = pars_refpntr[1] = this->get_leftson();
        pars_refpntr[2] = pars_refpntr[3] = this->get_rightson();
        if (father->father != 0) {
            // Referenzzeiger falls nicht an der Wurzel
            pars_refpntr[4] = pars_refpntr[5] = this->get_father();
            pars_refpntr[6] = pars_refpntr[7] = this_brother;
        }
        else {
            // an der Wurzel nehme linken und rechten Sohns des Bruders
            if (!get_brother()->is_leaf) {
                pars_refpntr[4] = pars_refpntr[5] = this_brother->get_leftson();
                pars_refpntr[6] = pars_refpntr[7] = this_brother->get_rightson();
            }
            else {
                pars_refpntr[4] = pars_refpntr[5] = 0;
                pars_refpntr[6] = pars_refpntr[7] = 0;
            }
        }
    }


    if (!father) return;        // no kl at root

    //
    // parsimony werte bestimmen
    //

    // Wurzel setzen

    ap_main->push();
    this->set_root();
    rootNode()->costs();

    int visited_subtrees = 0;
    int better_subtrees  = 0;
    for (i = 0; i < 8; i++) {
        pars_ref[i] = i;
        pars[i] = -1;

        if (!pars_refpntr[i])   continue;
        if (pars_refpntr[i]->is_leaf) continue;

        // KL recursion was broken (see changeset [11010] for how)
        // - IMO it should only descent into AP_NONE branches (see setters of 'kernighan'-flag)
        // - quick test shows calculation is much faster and results seem to be better.
        if (pars_refpntr[i]->kernighan != AP_NONE) continue;

        if (pars_refpntr[i]->gr.hidden) continue;
        if (pars_refpntr[i]->get_father()->gr.hidden) continue;

        // nur wenn kein Blatt ist
        ap_main->push();
        pars_refpntr[i]->swap_assymetric(pars_side_ref[i]);
        pars[i] = rootNode()->costs();
        if (pars[i] < pars_best) {
            better_subtrees++;
            pars_best      = pars[i];
            rek_width_type = AP_BETTER;
        }
        if (pars[i] < schwellwert) {
            rek_width_dynamic++;
        }
        ap_main->pop();
        visited_subtrees ++;

    }
    // Bubblesort, in pars[0] steht kleinstes element
    //
    // CAUTION! The original parsimonies will be exchanged


    for (i=7, t=0; t<i; t++) {
        if (pars[t] <0) {
            pars[t]     = pars[i];
            pars_ref[t] = i;
            t--;
            i--;
        }
    }

    bubblesort_change = 0;
    for (t = visited_subtrees - 1; t > 0; t--) {
        for (i = 0; i < t; i++) {
            if (pars[i] > pars[i+1]) {
                bubblesort_change = 1;
                help_ref          = pars_ref[i];
                pars_ref[i]       = pars_ref[i + 1];
                pars_ref[i + 1]   = help_ref;
                help              = pars[i];
                pars[i]           = pars[i + 1];
                pars[i + 1]       = help;
            }
        }
        if (bubblesort_change == 0)
            break;
    }

    display_out(pars, visited_subtrees, pars_prev, pars_start, rek_deep);
    // Darstellen

    if (rek_deep < rek_2_width_max) rek_width_static = rek_2_width[rek_deep];
    else rek_width_static                            = 1;

    rek_width = visited_subtrees;
    if (rek_width_type == AP_BETTER) {
        rek_width =  better_subtrees;
    }
    else {
        if (rek_width_type & AP_STATIC) {
            if (rek_width> rek_width_static) rek_width = rek_width_static;
        }
        if (rek_width_type & AP_DYNAMIK) {
            if (rek_width> rek_width_dynamic) rek_width = rek_width_dynamic;
        }
        else if (!(rek_width_type & AP_STATIC)) {
            if (rek_width> 1) rek_width = 1;
        }

    }

    if (rek_width > visited_subtrees)   rek_width = visited_subtrees;

    for (i=0; i < rek_width; i++) {
        ap_main->push();
        pars_refpntr[pars_ref[i]]->kernighan = pars_side_ref[pars_ref[i]];
        // Markieren
        pars_refpntr[pars_ref[i]]->swap_assymetric(pars_side_ref[pars_ref[i]]);
        // vertausche seite
        rootNode()->parsimony_rek();
        switch (rek_width_type) {
            case AP_BETTER: {
                // starte kerninghan_rek mit rekursionstiefe 3, statisch
                bool flag = false;
                cout << "found better !\n";
                pars_refpntr[pars_ref[i]]->kernighan_rek(rek_deep + 1, rek_2_width,
                                                         rek_2_width_max, rek_deep_max + 4,
                                                         function, param_liste, param_anz,
                                                         pars_best, pars_start, pars[i],
                                                         AP_STATIC, &flag);
                *abort_flag = true;
                break;
            }
            default:
                pars_refpntr[pars_ref[i]]->kernighan_rek(rek_deep + 1, rek_2_width,
                                                         rek_2_width_max, rek_deep_max,
                                                         function, param_liste, param_anz,
                                                         pars_best, pars_start, pars[i],
                                                         rek_width_type, abort_flag);
                break;
        }
        pars_refpntr[pars_ref[i]]->kernighan = AP_NONE;
        // Demarkieren
        if (*abort_flag) {
            cout << "   parsimony:  " << pars_best << "took: " << i << "\n";
            for (i=0; i<visited_subtrees; i++) cout << "  " << pars[i];
            cout << "\n";
            if (!rek_deep) {
                cout << "NEW RECURSION\n\n";
            }
            cout.flush();

            ap_main->clear();
            break;
        }
        else {
            ap_main->pop();
        }
    }
    if (*abort_flag) {       // pop/clear wegen set_root
        ap_main->clear();
    }
    else {
        ap_main->pop();
    }
    return;
}


const char* AP_tree_nlen::sortByName()
{
    if (name) return name;  // leafs

    const char *n1 = get_leftson()->sortByName();
    const char *n2 = get_rightson()->sortByName();

    if (strcmp(n1, n2)<0) return n1;

    AP_tree::swap_sons();

    return n2;
}

const char *AP_tree_nlen::fullname() const
{
    if (!name) {
        static char *buffer;
        char        *lName = strdup(get_leftson()->fullname());
        char        *rName = strdup(get_rightson()->fullname());
        int          len   = strlen(lName)+strlen(rName)+4;

        if (buffer) free(buffer);

        buffer = (char*)malloc(len);

        strcpy(buffer, "[");
        strcat(buffer, lName);
        strcat(buffer, ",");
        strcat(buffer, rName);
        strcat(buffer, "]");

        free(lName);
        free(rName);

        return buffer;
    }

    return name;
}


char* AP_tree_nlen::getSequenceCopy() {
    costs();

    AP_sequence_parsimony *pseq = DOWNCAST(AP_sequence_parsimony*, get_seq());
    ap_assert(pseq->got_sequence());

    size_t  len = pseq->get_sequence_length();
    char   *s   = new char[len];
    memcpy(s, pseq->get_sequence(), len);

    return s;
}

