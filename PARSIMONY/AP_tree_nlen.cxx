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
#include "ap_main.hxx"

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

ostream& operator<<(ostream& out, const AP_tree_nlen *node) {
    out << ' ';

    if (node==NULL) {
        out << "NULL";
    }
    if (node->is_leaf) {
        out << ((void *)node) << '(' << node->name << ')';
    }
    else {
        static int notTooDeep;

        if (notTooDeep) {
            out << ((void *)node);
            if (!node->father) out << " (ROOT)";
        }
        else {
            notTooDeep = 1;

            out << "NODE(" << ((void *)node);

            if (!node->father) {
                out << " (ROOT)";
            }
            else {
                out << ", father=" << node->father;
            }

            out << ", leftson=" << node->leftson
                << ", rightson=" << node->rightson
                << ", edge[0]=" << node->edge[0]
                << ", edge[1]=" << node->edge[1]
                << ", edge[2]=" << node->edge[2]
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

#if defined(DEBUG)
#define DUMP_INVALID_SUBTREES
#endif

#if defined(DEVEL_RALF)
#define CHECK_CORRECT_INVALIDATION // recombines all up-to-date nodes to find missing invalidations (VERY slow)
#endif


#if defined(DUMP_INVALID_SUBTREES)
inline void dumpSubtree(const char *msg, const AP_tree_nlen *node) {
    fprintf(stderr, "%s:\n", msg);
    char *printable = GBT_tree_2_newick(node, NewickFormat(nSIMPLE|nWRAP), true);
    fputs(printable, stderr);
    fputc('\n', stderr);
    free(printable);
}
#endif

inline const AP_tree_edge *edge_between(const AP_tree_nlen *node1, const AP_tree_nlen *node2) {
    AP_tree_edge *edge_12 = node1->edgeTo(node2);

#if defined(ASSERTION_USED)
    AP_tree_edge *edge_21 = node2->edgeTo(node1);
    ap_assert(edge_12 == edge_21); // nodes should agree about their edge
#endif

    return edge_12;
}

inline const char *no_valid_edge_between(const AP_tree_nlen *node1, const AP_tree_nlen *node2) {
    AP_tree_edge *edge_12 = node1->edgeTo(node2);
    AP_tree_edge *edge_21 = node2->edgeTo(node1);

    if (edge_12 == edge_21) {
        return edge_12 ? NULL : "edge missing";
    }
    return "edge inconsistent";
}

#if defined(DUMP_INVALID_SUBTREES)
#define PRINT_BAD_EDGE(msg,node) dumpSubtree(msg,node)
#else // !defined(DUMP_INVALID_SUBTREES)
#define PRINT_BAD_EDGE(msg,node) fprintf(stderr, "Warning: %s (at node=%p)\n", (msg), (node))
#endif

#define SHOW_BAD_EDGE(format,str,node) do{              \
        char *msg = GBS_global_string_copy(format,str); \
        PRINT_BAD_EDGE(msg, node);                      \
        free(msg);                                      \
    }while(0)

Validity AP_tree_nlen::has_valid_edges() const {
    Validity valid;
    if (get_father()) {                     // root has no edges
        if (get_father()->is_root_node()) { // sons of root have one edge between them
            if (is_leftson()) { // test root-edge only from one son
                const char *invalid = no_valid_edge_between(this, get_brother());
                if (invalid) {
                    SHOW_BAD_EDGE("root-%s. root", invalid, get_father());
                    valid = Validity(false, "no valid edge between sons of root");
                }
            }
            const char *invalid = no_valid_edge_between(this, get_father());
            if (!invalid || !strstr(invalid, "missing")) {
                SHOW_BAD_EDGE("unexpected edge (%s) between root and son", invalid ? invalid : "valid", this);
                valid = Validity(false, "unexpected edge between son-of-root and root");
            }
        }
        else {
            const char *invalid = no_valid_edge_between(this, get_father());
            if (invalid) {
                SHOW_BAD_EDGE("son-%s. father", invalid, get_father());
                SHOW_BAD_EDGE("parent-%s. son", invalid, this);
                valid = Validity(false, "invalid edge between son and father");
            }
        }
    }

    if (!is_leaf) {
        if (valid) valid = get_leftson()->has_valid_edges();
        if (valid) valid = get_rightson()->has_valid_edges();
    }
    return valid;
}

Validity AP_tree_nlen::sequence_state_valid() const {
    // if some node has a sequence, all son-nodes have to have sequences!

    Validity valid;

    const AP_sequence *sequence = get_seq();
    if (sequence) {
        if (sequence->hasSequence()) {
            if (!is_leaf) {
                bool leftson_hasSequence  = get_leftson()->hasSequence();
                bool rightson_hasSequence = get_rightson()->hasSequence();

#if defined(DUMP_INVALID_SUBTREES)
                if (!leftson_hasSequence) dumpSubtree("left subtree has no sequence", get_leftson());
                if (!rightson_hasSequence) dumpSubtree("right subtree has no sequence", get_rightson());
                if (!(leftson_hasSequence && rightson_hasSequence)) {
                    dumpSubtree("while father HAS sequence", this);
                }
#endif

                valid = Validity(leftson_hasSequence && rightson_hasSequence, "node has sequence and son w/o sequence");

#if defined(CHECK_CORRECT_INVALIDATION)
                if (valid) {
                    // check for missing invalidations
                    // (if recalculating a node (via combine) does not reproduce the current sequence, it should have been invalidated)

                    AP_sequence *recombined             = sequence->dup();
                    AP_FLOAT     mutations_from_combine = recombined->noncounting_combine(get_leftson()->get_seq(), get_rightson()->get_seq());

                    valid = Validity(recombined->equals(sequence), "recombining changed existing sequence (missing invalidation?)");
                    if (valid) {
                        AP_FLOAT expected_mutrate = mutations_from_combine + get_leftson()->mutation_rate + get_rightson()->mutation_rate;
                        valid = Validity(expected_mutrate == mutation_rate, "invalid mutation_rate");
                    }

                    delete recombined;

#if defined(DUMP_INVALID_SUBTREES)
                    if (!valid) {
                        dumpSubtree(valid.why_not(), this);
                    }
#endif
                }
#endif
            }
        }
#if defined(ASSERTION_USED)
        else {
            if (is_leaf) ap_assert(sequence->is_bound_to_species()); // can do lazy load if needed
        }
#endif
    }

    if (!is_leaf) {
        if (valid) valid = get_leftson()->sequence_state_valid();
        if (valid) valid = get_rightson()->sequence_state_valid();
    }

    return valid;
}

Validity AP_tree_nlen::is_valid() const {
    ap_assert(knownNonNull(this));

    Validity valid   = AP_tree::is_valid();
    if (valid) valid = has_valid_edges();
    if (valid) valid = sequence_state_valid();

    return valid;
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

void AP_tree_nlen::initial_insert(AP_tree_nlen *newBrother, AP_pars_root *troot) {
    // construct initial tree from 'this' and 'newBrother'
    // (both have to be leafs)

    ap_assert(newBrother);
    ap_assert(is_leaf);
    ap_assert(newBrother->is_leaf);

    AP_tree::initial_insert(newBrother, troot);
    makeEdge(newBrother, this); // build the root edge

    ASSERT_VALID_TREE(this->get_father());
}

void AP_tree_nlen::insert(AP_tree_nlen *newBrother) {
    //  inserts 'this' (a new node) at the father-edge of 'newBrother'
    ap_assert(newBrother);

    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(newBrother);

    ap_main->push_node(this, STRUCTURE);
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
            AP_tree_nlen *brothersOldBrother = newBrother->get_brother();
            ap_main->push_node(brothersOldBrother, STRUCTURE);

            AP_tree_edge *oldEdge = newBrother->edgeTo(brothersOldBrother)->unlink();
            AP_tree::insert(newBrother);
            oldEdge->relink(get_father(), get_father()->get_brother());
        }

        makeEdge(this, get_father());
        makeEdge(get_father(), newBrother);

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
        makeEdge(newBrother, rson);
        makeEdge(newBrother, lson);

        ASSERT_VALID_TREE(get_father());
    }
}

AP_tree_nlen *AP_tree_nlen::REMOVE() {
    // Removes 'this' and its father from the tree:
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
    // 'father' is destroyed, 'this' is returned.

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

        destroyEdge(edgeTo(get_father())->unlink());
        destroyEdge(get_father()->edgeTo(oldBrother)->unlink());

        if (grandPa->father) {
            oldEdge = get_father()->edgeTo(grandPa)->unlink();
            AP_tree::REMOVE();
            oldEdge->relink(oldBrother, grandPa);
        }
        else { // remove grandson of root
            AP_tree_nlen *uncle = get_father()->get_brother();
            ap_main->push_node(uncle, STRUCTURE);

            oldEdge = get_father()->edgeTo(uncle)->unlink();
            AP_tree::REMOVE();
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

            destroyEdge(edgeTo(oldBrother)->unlink());

#if defined(ASSERTION_USED)
            AP_pars_root *troot = get_tree_root();
#endif // ASSERTION_USED
            AP_tree::REMOVE();
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

            destroyEdge(edgeTo(oldBrother)->unlink());
            destroyEdge(oldBrother->edgeTo(lson)->unlink());

            oldEdge = oldBrother->edgeTo(rson)->unlink();
            AP_tree::REMOVE();
            oldEdge->relink(lson, rson);

            ap_assert(lson->get_tree_root()->get_root_node() == oldBrother);
            ASSERT_VALID_TREE(oldBrother);
        }
    }

    father = NULL;
    set_tree_root(NULL);

    ASSERT_VALID_TREE(this);
    return this;
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

            if (mode == AP_LEFT) {
                swap(leftson->father, nephew->father);
                swap(leftson, oldBrother->leftson);
            }
            else {
                swap(rightson->father, nephew->father);
                swap(rightson, oldBrother->leftson);
            }

            edge2->relink(this, nephew);
            edge1->relink(oldBrother, movedSon);

            if (nephew->gr.mark_sum != movedSon->gr.mark_sum) {
                get_brother()->recalc_marked_from_sons();
                this->recalc_marked_from_sons_and_forward_upwards();
            }
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

        if (mode == AP_LEFT) { // swap leftson with brother
            swap(leftson->father, oldBrother->father);
            if (father->leftson == this) {
                swap(leftson, father->rightson);
            }
            else {
                swap(leftson, father->leftson);
            }
        }
        else { // swap rightson with brother
            swap(rightson->father, oldBrother->father);
            if (father->leftson == this) {
                swap(rightson, father->rightson);
            }
            else {
                swap(rightson, father->leftson);
            }
        }

        edge2->relink(this, oldBrother);
        edge1->relink(get_father(), movedSon);

        if (oldBrother->gr.mark_sum != movedSon->gr.mark_sum) {
            recalc_marked_from_sons_and_forward_upwards(); // father is done implicit
        }
    }
}

void AP_tree_nlen::set_root() {
    if (at_root()) return; // already root

    // from this to root buffer the nodes
    ap_main->push_node(this,  STRUCTURE);

    AP_tree_nlen *son_of_root = 0; // in previous topology 'this' was contained inside 'son_of_root'
    AP_tree_nlen *old_root    = 0;
    {
        AP_tree_nlen *pntr;
        for (pntr = get_father(); pntr->father; pntr = pntr->get_father()) {
            ap_main->push_node(pntr, BOTH);
            son_of_root = pntr;
        }
        old_root = pntr;
    }

    ap_assert(son_of_root); // always true

    {
        AP_tree_nlen *other_son_of_root = son_of_root->get_brother();
        ap_main->push_node(other_son_of_root, STRUCTURE);
    }

    ap_main->push_node(old_root, ROOT);
    AP_tree::set_root();

    for (AP_tree_nlen *node = son_of_root; node ; node = node->get_father()) {
        node->recalc_marked_from_sons();
    }
}

void AP_tree_nlen::moveNextTo(AP_tree_nlen *newBrother, AP_FLOAT rel_pos) {
    // Note: see http://bugs.arb-home.de/ticket/627#comment:8 for an experimental
    // replacement of moveNextTo with REMOVE() + insert()

    ap_assert(father);
    ap_assert(newBrother);
    ap_assert(newBrother->father);
    ap_assert(newBrother->father != father); // already there
    ap_assert(newBrother != father);         // already there

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
        else { // 'this' is grandson of root
            ap_main->push_node(grandpa, ROOT);
            ap_main->push_node(get_father()->get_brother(), STRUCTURE);
        }
    }
    else { // 'this' is son of root
        ap_main->push_node(get_father(), ROOT);

        if (!get_brother()->is_leaf) {
            ap_main->push_node(get_brother()->get_leftson(), STRUCTURE);
            ap_main->push_node(get_brother()->get_rightson(), STRUCTURE);
        }
    }

    ap_main->push_node(newBrother,  STRUCTURE);
    if (newBrother->father) {
        AP_tree_nlen *newBrothersFather = newBrother->get_father();
        ap_main->push_node(newBrothersFather, BOTH);
        if (!newBrothersFather->father) { // move to son of root
            ap_main->push_node(newBrother->get_brother(), STRUCTURE);
        }
        push_all_upnode_sequences(newBrothersFather);
    }

    AP_tree_nlen *thisFather        = get_father();
    AP_tree_nlen *grandFather       = thisFather->get_father();
    AP_tree_nlen *oldBrother        = get_brother();
    AP_tree_nlen *newBrothersFather = newBrother->get_father();
    AP_tree_edge *e1, *e2, *e3;

    if (thisFather==newBrothersFather->get_father()) { // son -> son of brother
        if (grandFather) {
            if (grandFather->get_father()) {
                // covered by test at PARS_main.cxx@COVER3
                thisFather->unlinkAllEdges(&e1, &e2, &e3);
                AP_tree_edge *e4 = newBrother->edgeTo(oldBrother)->unlink();

                AP_tree::moveNextTo(newBrother, rel_pos);

                sortOldestFirst(&e1, &e2, &e3);
                e1->relink(oldBrother, grandFather); // use oldest edge at remove position
                thisFather->linkAllEdges(e2, e3, e4);
            }
            else { // grandson of root -> son of brother
                // covered by test at PARS_main.cxx@COVER2
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
            // covered by test at PARS_main.cxx@COVER1
            oldBrother->unlinkAllEdges(&e1, &e2, &e3);
            AP_tree::moveNextTo(newBrother, rel_pos);
            thisFather->linkAllEdges(e1, e2, e3);
        }
    }
    else if (grandFather==newBrothersFather) { // son -> brother of father
        if (grandFather->father) {
            // covered by test at PARS_main.cxx@COVER4
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

    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(rootNode());

    ap_assert(is_leftson());
    ap_assert(get_brother() == newBrother);
}

void AP_tree_nlen::unhash_sequence() {
    /*! removes the parsimony sequence from an inner node
     * (has no effect for leafs)
     */

    AP_sequence *sequence = get_seq();
    if (sequence && !is_leaf) sequence->forget_sequence();
}

bool AP_tree_nlen::acceptCurrentState(Level frame_level) {
    // returns
    // - true           if the top state has been removed
    // - false          if the top state was kept/extended for possible revert at lower frame_level

    if (remembered_for_frame != frame_level) {
        ap_assert(0); // internal control number check failed
        return false;
    }

    NodeState *state   = states.pop();
    bool       removed = true;

    Level next_frame_level   = frame_level-1;
    Level stored_frame_level = state->frameNr;

    if (!next_frame_level) { // accept() called at top-level
        delete state;
    }
    else if (stored_frame_level == next_frame_level) {
        // node already is buffered for next_frame_level

        // if the currently accepted state->mode is not completely covered by previous state->mode
        // => a future revert() would only restore partially
        // To avoid that, move missing state information to previous NodeState
        {
            NodeState     *prev_state = states.top();
            AP_STACK_MODE  prev_mode  = prev_state->mode;
            AP_STACK_MODE  common     = AP_STACK_MODE(prev_mode & state->mode);

            if (common != state->mode) {
                AP_STACK_MODE missing = AP_STACK_MODE(state->mode & ~common); // previous is missing this state information

                ap_assert((prev_mode&missing) == NOTHING);
                state->move_info_to(*prev_state, missing);
            }
        }

        delete state;
    }
    else {
        // keep state for future revert
        states.push(state);
        removed = false;
    }
    remembered_for_frame = next_frame_level;

    return removed;
}


bool AP_tree_nlen::rememberState(AP_STACK_MODE mode, Level frame_level) {
    // according to mode
    // tree_structure or sequence is buffered in the node

    NodeState *store;
    bool       ret;

    if (is_leaf && !(STRUCTURE & mode)) return false;    // tips push only structure

    if (remembered_for_frame == frame_level) { // node already has a push (at current frame_level)
        NodeState *is_stored = states.top();

        if (0 == (mode & ~is_stored->mode)) { // already buffered
            AP_sequence *sequence = get_seq();
            if (sequence && (mode & SEQUENCE)) sequence->forget_sequence();
            return false;
        }
        store = is_stored;
        ret = false;
    }
    else { // first push for this node (in current stack frame)
        store = new NodeState(remembered_for_frame);
        states.push(store);

        remembered_for_frame = frame_level;
        ret                  = true;
    }

    if ((mode & (STRUCTURE|SEQUENCE)) && !(store->mode & (STRUCTURE|SEQUENCE))) {
        store->mark_sum = gr.mark_sum;
    }
    if ((mode & STRUCTURE) && !(store->mode & STRUCTURE)) {
        store->father   = get_father();
        store->leftson  = get_leftson();
        store->rightson = get_rightson();
        store->leftlen  = leftlen;
        store->rightlen = rightlen;
        store->root     = get_tree_root();
        store->gb_node  = gb_node;

        for (int e=0; e<3; e++) {
            store->edge[e]      = edge[e];
            store->edgeIndex[e] = index[e];
        }
    }

    if (mode & SEQUENCE) {
        ap_assert(!is_leaf); // only allowed to push SEQUENCE for inner nodes
        if (!(store->mode & SEQUENCE)) {
            AP_sequence *sequence   = take_seq();
            store->sequence      = sequence;
            store->mutation_rate = mutation_rate;
            mutation_rate           = 0.0;
        }
        else {
            AP_sequence *sequence = get_seq();
            if (sequence) sequence->forget_sequence();
        }
    }

    store->mode = (AP_STACK_MODE)(store->mode|mode);

    return ret;
}

void NodeState::move_info_to(NodeState& target, AP_STACK_MODE what) {
    // rescue partial NodeState information

    ap_assert((mode&what)        == what); // this has to contain 'what' is moved
    ap_assert((target.mode&what) == NOTHING); // target shall not already contain 'what' is moved

    if ((what & (STRUCTURE|SEQUENCE)) && !(target.mode & (STRUCTURE|SEQUENCE))) {
        target.mark_sum = mark_sum;
    }
    if (what & STRUCTURE) {
        target.father     = father;
        target.leftson    = leftson;
        target.rightson   = rightson;
        target.leftlen    = leftlen;
        target.rightlen   = rightlen;
        target.root       = root;
        target.gb_node    = gb_node;

        for (int e=0; e<3; e++) {
            target.edge[e]      = edge[e];
            target.edgeIndex[e] = edgeIndex[e];
        }
    }
    if (what & SEQUENCE) {
        target.sequence      = sequence;
        target.mutation_rate = mutation_rate;
        sequence             = NULL;

    }
    // nothing needs to be done for ROOT
    target.mode = AP_STACK_MODE(target.mode|what);
}

void AP_tree_nlen::restore_structure(const NodeState& state) {
    father   = state.father;
    leftson  = state.leftson;
    rightson = state.rightson;
    leftlen  = state.leftlen;
    rightlen = state.rightlen;
    set_tree_root(state.root);
    gb_node  = state.gb_node;

    gr.mark_sum = state.mark_sum;

    for (int e=0; e<3; e++) {
        edge[e]  = state.edge[e];
        index[e] = state.edgeIndex[e];
        if (edge[e]) {
            edge[e]->index[index[e]] = e;
            edge[e]->node[index[e]]  = this;
        }
    }
}
void AP_tree_nlen::restore_sequence(NodeState& state) {
    replace_seq(state.sequence);
    state.sequence = NULL;
    mutation_rate  = state.mutation_rate;
    gr.mark_sum    = state.mark_sum;
}
void AP_tree_nlen::restore_sequence_nondestructive(const NodeState& state) {
    replace_seq(state.sequence ? state.sequence->dup() : NULL);
    mutation_rate = state.mutation_rate;
}
void AP_tree_nlen::restore_root(const NodeState& state) {
    state.root->change_root(state.root->get_root_node(), this);
}

void AP_tree_nlen::restore(NodeState& state) {
    //! restore 'this' from NodeState (cheap; only call once for each 'state')
    AP_STACK_MODE mode = state.mode;
    if (mode&STRUCTURE) restore_structure(state);
    if (mode&SEQUENCE) restore_sequence(state);
    if (ROOT==mode) restore_root(state);
}
void AP_tree_nlen::restore_nondestructive(const NodeState& state) {
    //! restore 'this' from NodeState (expensive; may be called multiple times for each 'state')
    AP_STACK_MODE mode = state.mode;
    if (mode&STRUCTURE) restore_structure(state);
    if (mode&SEQUENCE) restore_sequence_nondestructive(state);
    if (ROOT==mode) restore_root(state);
}

void AP_tree_nlen::revertToPreviousState(Level IF_ASSERTION_USED(curr_frameLevel), bool& IF_ASSERTION_USED(rootPopped)) { // pop old tree costs
    ap_assert(remembered_for_frame == curr_frameLevel); // error in node stack (node wasnt remembered in current frame!)

    NodeState *previous = states.pop();
#if defined(ASSERTION_USED)
    if (previous->mode == ROOT) { // @@@ remove test code later
        ap_assert(!rootPopped); // only allowed once
        rootPopped = true;
    }
#endif
    restore(*previous);

    remembered_for_frame = previous->frameNr;
    delete previous;
}

void AP_tree_nlen::parsimony_rec(char *mutPerSite) {
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

        if (!sequence->hasSequence()) {
            AP_tree_nlen *lson = get_leftson();
            AP_tree_nlen *rson = get_rightson();

            ap_assert(lson);
            ap_assert(rson);

            lson->parsimony_rec(mutPerSite);
            rson->parsimony_rec(mutPerSite);

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
    ap_assert(sequence_state_valid());

    parsimony_rec(mutPerSite);
    return mutation_rate;
}

AP_FLOAT AP_tree_nlen::nn_interchange_rec(EdgeSpec whichEdges, AP_BL_MODE mode) {
    if (!father) {
        return rootEdge()->nni_rec(whichEdges, mode, NULL, true);
    }
    if (!father->father) {
        AP_tree_edge *e = rootEdge();
        return e->nni_rec(whichEdges, mode, e->otherNode(this), false);
    }
    return edgeTo(get_father())->nni_rec(whichEdges, mode, get_father(), false);
}

inline CONSTEXPR_RETURN AP_TREE_SIDE idx2side(const int idx) {
    return idx&1 ? AP_RIGHT : AP_LEFT;
}

bool AP_tree_edge::kl_rec(const KL_params& KL, const int rec_depth, AP_FLOAT pars_best) {
    /*! does K.L. recursion
     * @param KL                parameters defining how recursion is done
     * @param rec_depth         current recursion depth (starts with 0)
     * @param pars_best         current parsimony value of topology
     */

    ap_assert(!is_leaf_edge());
    if (rec_depth >= KL.max_rec_depth) return false;

    ap_assert(implicated(rec_depth>0, kl_visited));

    int           order[8];
    AP_tree_edge *descend[8];

    {
        if (rec_depth == 0) {
            descend[0] = this;
            descend[2] = NULL;
            descend[4] = NULL;
            descend[6] = NULL;
        }
        else {
            AP_tree_nlen *son    = sonNode();
            AP_tree_nlen *notSon = otherNode(son); // brother or father

            descend[0] = notSon->nextEdge(this);
            descend[2] = notSon->nextEdge(descend[0]);

            ap_assert(descend[2] != this);

            descend[4] = son->nextEdge(this);
            descend[6] = son->nextEdge(descend[4]);

            ap_assert(descend[6] != this);
        }

        descend[1] = descend[0];
        descend[3] = descend[2];
        descend[5] = descend[4];
        descend[7] = descend[6];
    }

    // ---------------------------------
    //      detect parsimony values

    ap_main->remember(); // @@@ i think this is unneeded. better reset root after all done in caller
    set_root();
    rootNode()->costs();

    int rec_width_dynamic = 0;
    int visited_subtrees  = 0;
    int better_subtrees   = 0;

    AP_FLOAT pars[8]; // eight parsimony values (produced by 2*swap_assymetric at each adjacent edge)

#if defined(ASSERTION_USED)
    int forbidden_descends = 0;
#endif
    {
        AP_FLOAT schwellwert = KL.thresFunctor.calculate(rec_depth); // @@@ skip if not needed
        for (int i = 0; i < 8; i++) {
            order[i] = i;
            AP_tree_edge * const subedge = descend[i];

            if (subedge                  &&
                !subedge->is_leaf_edge() &&
                !subedge->kl_visited     &&
                (!KL.stopAtFoldedGroups || !subedge->next_to_folded_group())
                )
            {
                ap_main->remember();
                subedge->sonNode()->swap_assymetric(idx2side(i));
                pars[i] = rootNode()->costs();
                if (pars[i] < pars_best) {
                    better_subtrees++;
                    pars_best = pars[i]; // @@@ do not overwrite yet; store and overwrite when done with this loop
                }
                if (pars[i] < schwellwert) {
                    rec_width_dynamic++;
                }
                ap_main->revert();
                visited_subtrees++;
            }
            else {
                pars[i] = -1;
#if defined(ASSERTION_USED)
                if (subedge && subedge->kl_visited) {
                    forbidden_descends++;
                }
#endif
            }
        }
    }

    // bubblesort pars[]+order[], such that pars[0] contains best (=smallest) parsimony value
    {
        for (int i=7, t=0; t<i; t++) { // move negative (=unused) parsimony values to the end
            if (pars[t] <0) {
                pars[t]  = pars[i];
                order[t] = i;
                t--;
                i--;
            }
        }

        for (int t = visited_subtrees - 1; t > 0; t--) {
            bool bubbled = false;
            for (int i = 0; i < t; i++) {
                if (pars[i] > pars[i+1]) {
                    std::swap(order[i], order[i+1]);
                    std::swap(pars[i],  pars[i+1]);
                    bubbled = true;
                }
            }
            if (!bubbled) break;
        }
    }

#if defined(ASSERTION_USED)
    // rec_depth == 0 (called with start-node)
    // rec_depth == 1 (called twice with start-node (swap_assymetric AP_LEFT + AP_RIGHT))
    // rec_depth == 2 (called twice with each adjacent node -> 8 calls)
    // rec_depth == 3 (called twice with each adjacent node, but not with those were recursion came from -> 6 calls)

    if (!is_root_edge()) {
        switch (rec_depth) {
            case 0:
                ap_assert(visited_subtrees == 2);
                ap_assert(forbidden_descends == 0);
                break;
            case 1:
                ap_assert(visited_subtrees <= 8);
                ap_assert(forbidden_descends == 0);
                break;
            default:
                ap_assert(visited_subtrees <= 6);
                ap_assert(forbidden_descends == 2);
                break;
        }
    }
    else { // at root
        switch (rec_depth) {
            case 0:
                ap_assert(visited_subtrees <= 2);
                break;
            case 1:
                ap_assert(visited_subtrees <= 8);
                ap_assert(forbidden_descends <= 2); // in case of subtree-optimization, 2 descends may be forbidden
                break;
            default:
                ap_assert(visited_subtrees <= 8);
                break;
        }
    }
#endif

    int rec_width;
    if (better_subtrees) {
        rec_width = better_subtrees; // @@@ wrong if static/dynamic reduction would allow more

        // @@@ IMO the whole concept of incrementing depth when a better topology was found has no positive effect
        // (the better topology is kept anyway and next recursive KL will do a full optimization starting from that edge as well)

    }
    else {
        rec_width = visited_subtrees;
        if (KL.rec_type & AP_STATIC) {
            int rec_width_static = (rec_depth < CUSTOM_DEPTHS) ? KL.rec_width[rec_depth] : 1;
            rec_width            = std::min(rec_width, rec_width_static);
        }
        if (KL.rec_type & AP_DYNAMIK) {
            rec_width = std::min(rec_width, rec_width_dynamic);
        }
    }
    ap_assert(rec_width<=visited_subtrees);

    bool found_better = false;
    for (int i=0; i<rec_width && !found_better; i++) {
        AP_tree_edge * const subedge = descend[order[i]];

        ap_main->remember();
        subedge->kl_visited = true; // mark
        subedge->sonNode()->swap_assymetric(idx2side(order[i])); // swap
        rootNode()->parsimony_rec();

        if (better_subtrees) {
            KL_params modified      = KL;
            modified.rec_type       = AP_STATIC;
            modified.max_rec_depth += KL.inc_rec_depth;

            subedge->kl_rec(modified, rec_depth+1, pars_best);
            found_better = true;
        }
        else {
            found_better = subedge->kl_rec(KL, rec_depth+1, pars_best);
        }

        subedge->kl_visited = false;      // unmark
        ap_main->accept_if(found_better); // revert
    }

    ap_main->accept_if(found_better); // undo set_root otherwise
    return found_better;
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
    ap_assert(pseq->hasSequence());

    size_t  len = pseq->get_sequence_length();
    char   *s   = new char[len];
    memcpy(s, pseq->get_sequence(), len);

    return s;
}


GB_ERROR AP_pars_root::saveToDB() {
    has_been_saved = true;
    return AP_tree_root::saveToDB();
}

void AP_tree_nlen::buildNodeList_rec(AP_tree_nlen **list, long& num) {
    // builds a list of all inner nodes (w/o root node)
    if (!is_leaf) {
        if (father) list[num++] = this;
        get_leftson()->buildNodeList_rec(list, num);
        get_rightson()->buildNodeList_rec(list, num);
    }
}

void AP_tree_nlen::buildNodeList(AP_tree_nlen **&list, long &num) {
    num = this->count_leafs()-1;
    list = new AP_tree_nlen *[num+1];
    list[num] = 0;
    num  = 0;
    buildNodeList_rec(list, num);
}

void AP_tree_nlen::buildBranchList_rec(AP_tree_nlen **list, long& num, bool create_terminal_branches, int deep) {
    // builds a list of all species
    // (returns pairs of leafs/father and nodes/father)

    if (deep) {
        if (father && (create_terminal_branches || !is_leaf)) {
            if (father->father) {
                list[num++] = this;
                list[num++] = get_father();
            }
            else {                  // root
                if (father->leftson == this) {
                    list[num++] = this;
                    list[num++] = get_brother();
                }
            }
        }
        if (!is_leaf) {
            get_leftson() ->buildBranchList_rec(list, num, create_terminal_branches, deep-1);
            get_rightson()->buildBranchList_rec(list, num, create_terminal_branches, deep-1);
        }
    }
}

void AP_tree_nlen::buildBranchList(AP_tree_nlen **&list, long &num, bool create_terminal_branches, int deep) {
    if (deep>=0) {
        num = 2;
        for (int i=0; i<deep; i++) num *= 2;
    }
    else {
        num = count_leafs() * (create_terminal_branches ? 2 : 1);
    }

    ap_assert(num >= 0);

    list = new AP_tree_nlen *[num*2+4];

    if (num) {
        long count = 0;

        buildBranchList_rec(list, count, create_terminal_branches, deep);
        list[count] = 0;
        num         = count/2;
    }
}
