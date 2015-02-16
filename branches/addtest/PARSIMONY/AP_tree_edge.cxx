// =============================================================== //
//                                                                 //
//   File      : AP_tree_edge.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in Summer 1995    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ap_tree_nlen.hxx"
#include "ap_main.hxx"

#include <AP_filter.hxx>
#include <arb_progress.h>

#include <cmath>
#include <iomanip>

using namespace std;

long AP_tree_edge::timeStamp = 0;

AP_tree_edge::AP_tree_edge(AP_tree_nlen *node1, AP_tree_nlen *node2)
{
    // not really necessary, but why not clear all:
    memset((char *)this, 0, sizeof(AP_tree_edge));
    age  = timeStamp++;

    // link the nodes:

    relink(node1, node2);
}

AP_tree_edge::~AP_tree_edge() {
    if (is_linked()) unlink();
}

static void buildSonEdges(AP_tree_nlen *node) {
    /*! Builds edges between a node and his two sons.
     * We assume there is already an edge to node's father and there are
     * no edges to his sons.
     */

    if (!node->is_leaf) {
        buildSonEdges(node->get_leftson());
        buildSonEdges(node->get_rightson());

        // to ensure the nodes contain the correct distance to the border
        // we MUST build all son edges before creating the father edge

        new AP_tree_edge(node, node->get_leftson());
        new AP_tree_edge(node, node->get_rightson());
    }
}

void AP_tree_edge::initialize(AP_tree_nlen *tree) {
    /*! Builds all edges in the whole tree.
     * The root node is skipped - instead his two sons are connected with an edge
     */
    while (tree->get_father()) tree = tree->get_father(); // go up to root
    buildSonEdges(tree->get_leftson());             // link left subtree
    buildSonEdges(tree->get_rightson());            // link right subtree

    // to ensure the nodes contain the correct distance to the border
    // we MUST build all son edges before creating the root edge

    new AP_tree_edge(tree->get_leftson(), tree->get_rightson()); // link brothers
}

void AP_tree_edge::destroy(AP_tree_nlen *tree) {
    /*! Destroys all edges in the whole tree */
    AP_tree_edge *edge = tree->nextEdge(NULL);
    if (!edge) {
        ap_assert(tree->is_root_node());
        edge = tree->get_leftson()->edgeTo(tree->get_rightson());
    }
    ap_assert(edge); // got no edges?

    EdgeChain chain(edge, UNLIMITED, ANY_EDGE, false);
    while (chain) {
        AP_tree_edge *curr = *chain;
        ++chain;
        delete curr;
    }
}

AP_tree_edge* AP_tree_edge::unlink() {
    ap_assert(this!=0);
    ap_assert(is_linked());

    node[0]->edge[index[0]] = NULL;
    node[1]->edge[index[1]] = NULL;

    node[0] = NULL;
    node[1] = NULL;

    return this;
}

void AP_tree_edge::relink(AP_tree_nlen *node1, AP_tree_nlen *node2) {
    ap_assert(!is_linked());

    node[0] = node1;
    node[1] = node2;

    node1->edge[index[0] = node1->unusedEdgeIndex()] = this;
    node2->edge[index[1] = node2->unusedEdgeIndex()] = this;

    node1->index[index[0]] = 0;
    node2->index[index[1]] = 1;
}

size_t AP_tree_edge::buildChainInternal(int depth, EdgeSpec whichEdges, bool depthFirst, const AP_tree_nlen *skip, AP_tree_edge **&prevNextPtr) {
    size_t added = 0;

    ap_assert(prevNextPtr);
    ap_assert(*prevNextPtr == NULL);

    bool descend = true;
    bool use     = true;

    if (use && (whichEdges&SKIP_UNMARKED_EDGES)) {
        use = descend = has_marked(); // Note: root edge is chained if ANY son of root has marked children
    }
    if (use && (whichEdges&SKIP_FOLDED_EDGES)) {
        // do not chain edges leading to root of group
        // (doing an NNI there will swap branches across group-borders)
        use = !next_to_folded_group();
    }
    if (use && (whichEdges&(SKIP_LEAF_EDGES|SKIP_INNER_EDGES))) {
        use = !(whichEdges&(is_leaf_edge() ? SKIP_LEAF_EDGES : SKIP_INNER_EDGES));
    }

    if (use && !depthFirst) {
        *prevNextPtr  = this;
        next_in_chain = NULL;
        prevNextPtr   = &next_in_chain;
        added++;
    }
    if (descend && depth) {
        for (int n=0; n<2; n++) {
            if (node[n]!=skip && !node[n]->is_leaf) {
                for (int e=0; e<3; e++) {
                    AP_tree_edge * Edge = node[n]->edge[e];
                    if (Edge != this) {
                        descend = true;
                        if (descend && (whichEdges&SKIP_UNMARKED_EDGES)) descend = has_marked();
                        if (descend && (whichEdges&SKIP_FOLDED_EDGES))   descend = !next_to_folded_group();
                        if (descend) {
                            added += Edge->buildChainInternal(depth-1, whichEdges, depthFirst, node[n], prevNextPtr);
                        }
                    }
                }
            }
        }
    }
    if (use && depthFirst) {
        ap_assert(*prevNextPtr == NULL);

        *prevNextPtr  = this;
        next_in_chain = NULL;
        prevNextPtr   = &next_in_chain;
        added++;
    }

    return added;
}

bool EdgeChain::exists = false;

EdgeChain::EdgeChain(AP_tree_edge *startEgde, int depth, EdgeSpec whichEdges, bool depthFirst, const AP_tree_nlen *skip)
    : start(NULL),
      curr(NULL)
{
    /*! build a chain of edges for further processing
     * @param startEgde        start edge
     * @param depth            specifies how far to recurse into tree (-1 = recurse whole tree; 0 = this edge only)
     * @param whichEdges       if MARKED_VISIBLE_EDGES -> do not descend into folded subtrees and subtrees w/o marked species
     * @param depthFirst       true -> insert leafs before inner nodes (but whole son-subtree before other-son-subtree)
     * @param skip             previous node (will not recurse beyond)
     */

    ap_assert(!exists); // only one existing chain is allowed!
    exists = true;

    AP_tree_edge **prev = &start;

    len  = startEgde->buildChainInternal(depth, whichEdges, depthFirst, skip, prev);
    curr = start;

    ap_assert(correlated(len, start));
    ap_assert(implicated(!depthFirst, curr == startEgde));
}

class MutationsPerSite : virtual Noncopyable {
    char   *Data;
    size_t  length;

public:
    MutationsPerSite(size_t len)
        : Data((char*)GB_calloc(sizeof(char), len*3))
        , length(len)
    {}
    ~MutationsPerSite() {
        free(Data);
    }

    char *data(int block) {
        ap_assert(block >= 0 && block<3);
        return Data+block*length;
    }
    const char *data(int block) const {
        return const_cast<MutationsPerSite*>(this)->data(block);
    }
};

static double ap_calc_bootstrap_remark_sub(int seq_len, const char *old, const char *ne) {
    int sum[3] = { 0, 0, 0 };
    for (int i=0; i<seq_len; i++) {
        int diff = ne[i] - old[i];
        if (diff > 1 || diff < -1) {
#if defined(DEBUG)
            fprintf(stderr, "diff by nni at one position not in [-1,1]: %i:%i - %i", diff, old[i], ne[i]);
#endif // DEBUG
            continue;
        }
        sum[diff+1] ++;
    }

    double prob = 0;
    {
        int asum = 0;
        for (int i=0; i<3; i++) asum += sum[i];

        double freq[3];
        double log_freq[3];
        for (int i=0; i<3; i++) {
            freq[i] = sum[i] / double(asum); // relative frequencies of -1, 0, 1
            if (sum[i] >0) {
                log_freq[i] = log(freq[i]);
            }
            else {
                log_freq[i] = -1e100; // minus infinit
            }
        }

        int max = seq_len;  // bootstrap can select seq_len ones maximum
        double log_fak_seq_len = GB_log_fak(seq_len);
        double log_eps = log(1e-11);

        // loop over all delta_mutations, begin in the middle
        for (int tsum_add = 1; tsum_add >= -1; tsum_add -= 2) {
            int tsum = sum[2]-sum[0];
            if (tsum <= 0) tsum = 1;
            for (; tsum < max && tsum > 0; tsum += tsum_add) {      // sum of mutations in bootstrap sample, loop over all possibilities
                if (tsum_add < 0 && tsum == sum[2]-sum[0]) continue; // don't double count tsum



                int max_minus = max;        // we need tsum + n_minus ones, we cannot have more than max_minux minus, reduce also
                for (int minus_add = 1; minus_add>=-1; minus_add-=2) {
                    int first_minus = 1;
                    for (int n_minus = sum[0]; n_minus<max_minus && n_minus>=0; first_minus = 0, n_minus+=minus_add) {
                        if (minus_add < 0 && first_minus) continue; // don't double count center
                        int n_zeros = seq_len - n_minus * 2 - tsum; // number of minus
                        int n_plus = tsum + n_minus; // number of plus ones  (n_ones + n_minus + n_zeros = seq_len)

                        double log_a =
                            n_minus * log_freq[0] +
                            n_zeros * log_freq[1] +
                            n_plus  * log_freq[2] +
                            log_fak_seq_len - GB_log_fak(n_minus) - GB_log_fak(n_zeros) - GB_log_fak(n_plus);

                        if (log_a < log_eps) {
                            if (first_minus && minus_add>0) goto end;
                            break; // cannot go with so many minus, try next
                        }
                        double a = exp(log_a);
                        prob += a;
                    }
                }
            }
        end :;
        }
    }
    return prob;
}

static void ap_calc_bootstrap_remark(AP_tree_nlen *son_node, AP_BL_MODE mode, const MutationsPerSite& mps) {
    if (!son_node->is_leaf) {
        size_t seq_len = son_node->get_seq()->get_sequence_length();
        float  one     = ap_calc_bootstrap_remark_sub(seq_len, mps.data(0), mps.data(1));
        float  two     = ap_calc_bootstrap_remark_sub(seq_len, mps.data(0), mps.data(2));

        if ((mode & AP_BL_BOOTSTRAP_ESTIMATE) == AP_BL_BOOTSTRAP_ESTIMATE) {
            one = one * two;    // assume independent bootstrap values for both nnis
        }
        else {
            if (two<one) one = two; // dependent bootstrap values, take minimum (safe)
        }

        double bootstrap = one<1.0 ? 100.0 * one : 100.0;
        son_node->set_bootstrap(bootstrap);
        son_node->get_brother()->set_remark(son_node->get_remark());
    }
}


static void ap_calc_leaf_branch_length(AP_tree_nlen *leaf) {
    // calculates the branchlength for leafs

    AP_FLOAT Seq_len = leaf->get_seq()->weighted_base_count();
    if (Seq_len <= 1.0) Seq_len = 1.0;

    ap_assert(leaf->is_leaf);

    AP_FLOAT parsbest = rootNode()->costs();

    ap_main->remember();
    leaf->REMOVE();
    AP_FLOAT mutations = parsbest - rootNode()->costs(); // number of min. mutations caused by adding 'leaf' to tree
    ap_main->revert();

    GBT_LEN blen = mutations/Seq_len; // scale with Seq_len (=> max branchlength == 1.0)
    leaf->set_branchlength_unrooted(blen);
}

const GBT_LEN AP_UNDEF_BL = 10.5;

static void set_inner_branch_length_and_calc_adj_leaf_lengths(AP_tree_nlen *son, GBT_LEN blen) {
    ap_assert(!son->is_leaf);

    AP_FLOAT seq_len             = son->get_seq()->weighted_base_count();
    if (seq_len <= 1.0) seq_len  = 1.0;
    blen                        *= 0.5 / seq_len * 2.0; // doubled counted sum * corr

    son->set_branchlength_unrooted(blen);

    // calculate adjacent leaf branchlengths early
    // (calculating them at end of nni_rec, produces much more combines)
    if (son->leftson->is_leaf) {
        ap_assert(son->get_leftson()->get_branchlength_unrooted() == AP_UNDEF_BL);
        ap_calc_leaf_branch_length(son->get_leftson());
    }

    if (son->rightson->is_leaf) {
        ap_assert(son->get_rightson()->get_branchlength_unrooted() == AP_UNDEF_BL);
        ap_calc_leaf_branch_length(son->get_rightson());
    }
}

#if defined(ASSERTION_USED) || defined(UNIT_TESTS)
bool allBranchlengthsAreDefined(AP_tree_nlen *tree) {
    if (tree->father) {
        if (tree->get_branchlength_unrooted() == AP_UNDEF_BL) return false;
    }
    if (tree->is_leaf) return true;
    return
        allBranchlengthsAreDefined(tree->get_leftson()) &&
        allBranchlengthsAreDefined(tree->get_rightson());
}
#endif

inline void update_undefined_leaf_branchlength(AP_tree_nlen *node) {
    if (node->is_leaf &&
        node->get_branchlength_unrooted() == AP_UNDEF_BL)
    {
        ap_calc_leaf_branch_length(node);
    }
}

inline void undefine_branchlengths(AP_tree_nlen *node) {
    // undefine branchlengths of node (triggers recalculation)
    ap_main->push_node(node, STRUCTURE); // store branchlengths for revert
    node->leftlen  = AP_UNDEF_BL;
    node->rightlen = AP_UNDEF_BL;
}

AP_FLOAT AP_tree_edge::nni_rec(int depth, EdgeSpec whichEdges, AP_BL_MODE mode, AP_tree_nlen *skipNode) {
    ap_assert(depth == UNLIMITED); // @@@ elim param 'depth' if always UNLIMITED

    if (!rootNode())         return 0.0;
    if (rootNode()->is_leaf) return rootNode()->costs();

    AP_tree_edge *oldRootEdge = rootEdge();

    if (depth>=0) set_root();

    AP_FLOAT old_parsimony = rootNode()->costs();
    AP_FLOAT new_parsimony = old_parsimony;

    ap_assert(allBranchlengthsAreDefined(rootNode()));

    bool         recalc_lengths = mode & AP_BL_BL_ONLY;
    EdgeChain    chain(this, depth, whichEdges, !recalc_lengths, skipNode);
    arb_progress progress(chain.size());

    if (recalc_lengths) { // set all branchlengths to undef
        ap_main->push_node(rootNode(), STRUCTURE);
        while (chain) {
            AP_tree_edge *edge = *chain; ++chain;
            undefine_branchlengths(edge->node[0]);
            undefine_branchlengths(edge->node[1]);
            undefine_branchlengths(edge->node[0]->get_father());
        }
        rootNode()->leftlen  = AP_UNDEF_BL *.5;
        rootNode()->rightlen = AP_UNDEF_BL *.5;
    }

    chain.restart();
    while (chain && (recalc_lengths || !progress.aborted())) { // never abort while calculating branchlengths
        AP_tree_edge *edge   = *chain; ++chain;
        AP_tree_nlen *son    = edge->sonNode();
        AP_tree_nlen *notSon = edge->otherNode(son);

        if (notSon->father) {
            if (mode & AP_BL_BOOTSTRAP_LIMIT) {
                son->set_root();
                new_parsimony = rootNode()->costs();
            }
            else {
                if (notSon->father->father) {
                    notSon->set_root();
                    new_parsimony = rootNode()->costs();
                }
            }
        }
        if (mode & AP_BL_BOOTSTRAP_LIMIT) {
            MutationsPerSite mps(son->get_seq()->get_sequence_length());
            new_parsimony = edge->nni_mutPerSite(new_parsimony, mode, &mps);
            ap_calc_bootstrap_remark(son, mode, mps);
        }
        else {
            new_parsimony = edge->nni(new_parsimony, mode);
        }

        progress.inc();
    }

    if (recalc_lengths) {
// #define DUMP_FINAL_RECALCS
#if defined(DUMP_FINAL_RECALCS)
        int final_recalc = 0;
#endif

        chain.restart();
        while (chain) {
            // inner branchlengths have been caclculated by nni_mutPerSite (called above directly or indirectly via nni())
            // most leaf branchlengths have already been calculated (via set_inner_branch_length_and_calc_adj_leaf_lengths)
            // (calls from unit tests only update 1 or max. 2 lengths in this loop)

            AP_tree_edge *edge = *chain; ++chain;

#if defined(DUMP_FINAL_RECALCS)
            if (edge->node[0]->get_branchlength_unrooted() == AP_UNDEF_BL) ++final_recalc;
            if (edge->node[1]->get_branchlength_unrooted() == AP_UNDEF_BL) ++final_recalc;
#endif

            update_undefined_leaf_branchlength(edge->node[0]);
            update_undefined_leaf_branchlength(edge->node[1]);
        }

#if defined(DUMP_FINAL_RECALCS)
        fprintf(stderr, "Final leaf branches calculated: %i leafs for a chain of size=%li\n", final_recalc, cs);
#endif
    }

    ap_assert(allBranchlengthsAreDefined(rootNode()));

    oldRootEdge->set_root();
    new_parsimony = rootNode()->costs();

    return new_parsimony;
}

AP_FLOAT AP_tree_edge::nni_mutPerSite(AP_FLOAT pars_one, AP_BL_MODE mode, MutationsPerSite *mps) {
    if (is_leaf_edge()) return pars_one;

    AP_tree_nlen *root     = rootNode();
    AP_FLOAT      parsbest = pars_one;
    AP_tree_nlen *son      = sonNode();

    {               // ******** original tree
        if ((mode & AP_BL_BOOTSTRAP_LIMIT)) {
            root->costs();
            son->unhash_sequence();
            son->get_father()->unhash_sequence();
            ap_assert(!son->father->father);
            AP_tree_nlen *brother = son->get_brother();
            brother->unhash_sequence();

            ap_assert(mps);
            pars_one = root->costs(mps->data(0));
        }
        else if (pars_one==0.0) {
            pars_one = root->costs();
        }
    }

    AP_FLOAT pars_two;
    {               // ********* first nni
        ap_main->remember();
        son->swap_assymetric(AP_LEFT);
        pars_two = root->costs(mps ? mps->data(1) : NULL);

        if (pars_two <= parsbest) {
            ap_main->accept_if(mode & AP_BL_NNI_ONLY);
            parsbest = pars_two;
        }
        else {
            ap_main->revert();
        }
    }

    AP_FLOAT pars_three;
    {               // ********** second nni
        ap_main->remember();
        son->swap_assymetric(AP_RIGHT);
        pars_three = root->costs(mps ? mps->data(2) : NULL);

        if (pars_three <= parsbest) {
            ap_main->accept_if(mode & AP_BL_NNI_ONLY);
            parsbest = pars_three;
        }
        else {
            ap_main->revert();
        }
    }

    if (mode & AP_BL_BL_ONLY) { // ************* calculate branch lengths **************
        GBT_LEN blen = (pars_one + pars_two + pars_three) - (3.0 * parsbest);
        if (blen <0) blen = 0;
        set_inner_branch_length_and_calc_adj_leaf_lengths(son, blen);
    }

    return parsbest;
}

ostream& operator<<(ostream& out, const AP_tree_edge& e)
{
    static int notTooDeep;

    out << ' ';

    if (notTooDeep || &e==NULL) {
        out << e;
    }
    else {
        notTooDeep = 1;
        out << "AP_tree_edge(" << e
            << ", node[0]=" << *(e.node[0])
            << ", node[1]=" << *(e.node[1])
            << ")";
        notTooDeep = 0; // cppcheck-suppress redundantAssignment
    }

    return out << ' ';
}

void AP_tree_edge::mixTree(int repeat, int percent) {
    EdgeChain chain(this, UNLIMITED, SKIP_LEAF_EDGES, false); // @@@ unwanted hardcoded edge selection (should operate as configured)
    long      edges = chain.size();

    arb_progress progress(repeat*edges);
    while (repeat-- && !progress.aborted()) {
        chain.restart();
        while (chain) {
            AP_tree_nlen *son = (*chain)->sonNode();
            ap_assert(!son->is_leaf);
            if (percent>=100 || GB_random(100)<percent) {
                son->swap_assymetric(GB_random(2) ? AP_LEFT : AP_RIGHT);
            }
            ++chain;
            ++progress;
        }
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arb_defs.h>
#include "pars_main.hxx"
#include <AP_seq_dna.hxx>
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif
#include <test_env.h>

void TEST_edgeChain() {
    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    AP_tree_edge *root  = rootEdge();
    AP_tree_nlen *rootN = root->sonNode()->get_father();

    ap_assert(!rootN->father);
    AP_tree_nlen *leftSon  = rootN->get_leftson();
    AP_tree_nlen *rightSon = rootN->get_rightson();

    TEST_EXPECT_EQUAL(EdgeChain(root, -1, ANY_EDGE,                                     true).size(), 27);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(ANY_EDGE|SKIP_INNER_EDGES),          true).size(), 15);    // 15 leafs
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(SKIP_FOLDED_EDGES|SKIP_INNER_EDGES), true).size(), 15-4);  // 4 leafs are inside folded group
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(ANY_EDGE|SKIP_LEAF_EDGES),           true).size(), 27-15);

    TEST_EXPECT_EQUAL(EdgeChain(root,  0, ANY_EDGE, true).size(),  1); // root-edge
    TEST_EXPECT_EQUAL(EdgeChain(root,  1, ANY_EDGE, true).size(),  5); // plus 4 adjacent edges
    TEST_EXPECT_EQUAL(EdgeChain(root,  2, ANY_EDGE, true).size(), 11); // < max (=5+8)

    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true).size(), 13);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(MARKED_VISIBLE_EDGES|SKIP_INNER_EDGES), true).size(), 6); // 6 marked leafs
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(MARKED_VISIBLE_EDGES|SKIP_LEAF_EDGES),  true).size(), 13-6);

    // skip left/right subtree
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, ANY_EDGE, true, leftSon) .size(),  9);  // right subtree plus rootEdge
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, ANY_EDGE, true, rightSon).size(), 19);  // left  subtree plus rootEdge

    TEST_EXPECT_EQUAL(EdgeChain(root,  0, ANY_EDGE, true, leftSon) .size(),  1); // rootEdge
    TEST_EXPECT_EQUAL(EdgeChain(root,  1, ANY_EDGE, true, leftSon) .size(),  3); // plus 2 right son-edges
    TEST_EXPECT_EQUAL(EdgeChain(root,  2, ANY_EDGE, true, leftSon) .size(),  5); // plus two more sons (i.e. one of the above edges must be a leaf edge)

    TEST_EXPECT_EQUAL(EdgeChain(root,  0, ANY_EDGE, true, rightSon).size(),  1); // rootEdge
    TEST_EXPECT_EQUAL(EdgeChain(root,  1, ANY_EDGE, true, rightSon).size(),  3); // plus 2 left son-edges
    TEST_EXPECT_EQUAL(EdgeChain(root,  2, ANY_EDGE, true, rightSon).size(),  7); // plus 4 grandson-edges

    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES, true, leftSon) .size(),  8); // one leaf edge is unmarked
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES, true, rightSon).size(),  6);

    // mark only two species: CorGluta (unfolded) + CloTyro2 (folded)
    {
        GB_transaction ta(env.gbmain());
        GBT_restore_marked_species(env.gbmain(), "CloTyro2;CorGluta");
        env.compute_tree(); // species marks affect node-chain
    }

    TEST_EXPECT_EQUAL(EdgeChain(root, -1, ANY_EDGE,                                        true)          .size(), 27);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true)          .size(), 6);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(MARKED_VISIBLE_EDGES|SKIP_INNER_EDGES), true)          .size(), 1);   // one visible marked leaf (the other is hidden)
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(MARKED_VISIBLE_EDGES|SKIP_LEAF_EDGES),  true)          .size(), 6-1);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(SKIP_UNMARKED_EDGES|SKIP_INNER_EDGES),  true)          .size(), 2);   // two marked leaf
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true, rightSon).size(), 3);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true, leftSon) .size(), 4);

    // test trees with marks in ONE subtree (of root) only
    {
        GB_transaction ta(env.gbmain());
        GBT_restore_marked_species(env.gbmain(), "CloTyro2");
        env.compute_tree(); // species marks affect node-chain
    }
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true)          .size(), 3);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, EdgeSpec(MARKED_VISIBLE_EDGES|SKIP_INNER_EDGES), true)          .size(), 0); // the only marked leaf is folded
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true, rightSon).size(), 3);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES,                            true, leftSon) .size(), 1);

    {
        GB_transaction ta(env.gbmain());
        GBT_restore_marked_species(env.gbmain(), "CorGluta");
        env.compute_tree(); // species marks affect node-chain
    }
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES, true)          .size(), 4);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES, true, rightSon).size(), 1);
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES, true, leftSon) .size(), 4);

    // unmark all
    {
        GB_transaction ta(env.gbmain());
        GBT_mark_all(env.gbmain(), 0);
        env.compute_tree(); // species marks affect node-chain
    }
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, MARKED_VISIBLE_EDGES, true).size(), 0);

    TEST_EXPECT_EQUAL(EdgeChain(root, -1, SKIP_FOLDED_EDGES, true)          .size(), 20); // folded group contains 6 edges + edge leading to group (20=27-7)
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, SKIP_FOLDED_EDGES, true, rightSon).size(), 12); // (12=19-7)
    TEST_EXPECT_EQUAL(EdgeChain(root, -1, SKIP_FOLDED_EDGES, true, leftSon) .size(), 9);
}

void TEST_tree_flags_needed_by_EdgeChain() {
    // EdgeChain depends on correctly set marked flags in AP_tree
    // (i.e. on gr.has_marked_children)
    //
    // These flags get destroyed by tree operations
    // -> chains created after tree modifications are wrong
    // -> optimization operates on wrong part of the tree
    // (esp. add+NNI and NNI/KL)

    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    TEST_EXPECT(rootNode()->has_correct_mark_flags());

    // mark only two species: CorGluta (unfolded) + CloTyro2 (folded)
    {
        GB_transaction ta(env.gbmain());
        GBT_restore_marked_species(env.gbmain(), "CloTyro2;CorGluta");
        env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
    }

    TEST_EXPECT(rootNode()->has_correct_mark_flags());

    AP_tree_nlen *CorGluta = rootNode()->findLeafNamed("CorGluta"); // marked
    AP_tree_nlen *CelBiazo = rootNode()->findLeafNamed("CelBiazo"); // not marked (marked parent, marked brother)
    AP_tree_nlen *CurCitre = rootNode()->findLeafNamed("CurCitre"); // not marked (unmarked parent, unmarked brother)
    AP_tree_nlen *CloTyro2 = rootNode()->findLeafNamed("CloTyro2"); // marked, inside folded group!
    AP_tree_nlen *CloCarni = rootNode()->findLeafNamed("CloCarni"); // in the mid of unmarked subtree of 4

    AP_tree_nlen *CurCitre_father      = CurCitre->get_father();
    AP_tree_nlen *CurCitre_grandfather = CurCitre_father->get_father();

    // test moving root
    // (fails whenever moved to an edge w/o marks)
    {
        env.push();

        CorGluta->set_root();
        TEST_EXPECT(rootNode()->has_correct_mark_flags());

        CelBiazo->set_root();
        TEST_EXPECT(rootNode()->has_correct_mark_flags());

        CloTyro2->set_root();
        TEST_EXPECT(rootNode()->has_correct_mark_flags());

        // CurCitre and its brother form an unmarked subtree
        CurCitre->set_root();
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

        CurCitre_father->set_root();
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

        CurCitre_grandfather->set_root(); // grandfather has 1 marked child
        TEST_EXPECT(rootNode()->has_correct_mark_flags());

        env.pop(); TEST_EXPECT(rootNode()->has_correct_mark_flags());
    }

    // test moving nodes/subtrees
    {
        env.push();

        // move marked node into unmarked subtree of 2 species:
        CorGluta->moveNextTo(CurCitre, 0.5);
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        // move unmarked subtree of two species (brother is marked)
        CurCitre_father->moveNextTo(CelBiazo, 0.5); // move to unmarked uncle of brother
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        // move same subtree into unmarked subtree
        CurCitre_father->moveNextTo(CloCarni, 0.5);
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        // move unmarked -> unmarked (both parents are unmarked as well)
        CurCitre->moveNextTo(CloCarni, 0.5);
        TEST_EXPECT(rootNode()->has_correct_mark_flags()); // works (but moving CurCitre_father doesnt)

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        // move marked -> marked
        CorGluta->moveNextTo(CloTyro2, 0.5);
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags()); // subtree losts the only marked species (should unmark up to root)

        // --------------------------------------------------------------------------------
        // now mark flags are broken -> test whether revert restores them
        ap_assert(!rootNode()->has_correct_mark_flags());
        rootNode()->compute_tree(); // fixes the flags (i.e. changes hidded AND marked flags)

        env.pop(); TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags()); // shows that flags are not restored

        rootNode()->compute_tree(); // fix flags again
        TEST_EXPECT(rootNode()->has_correct_mark_flags());
    }

    // test swap_assymetric
    {
        env.push();

        rootNode()->get_leftson()->swap_assymetric(AP_LEFT);
        TEST_EXPECT(rootNode()->has_correct_mark_flags());

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        CorGluta->get_father()->swap_assymetric(AP_RIGHT);
        TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        CorGluta->get_father()->swap_assymetric(AP_LEFT);
        TEST_EXPECT(rootNode()->has_correct_mark_flags()); // (maybe swaps two unmarked subtrees?!)

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        {
            // swap inside folded group
            AP_tree_nlen *CloTyro2_father = CloTyro2->get_father();

            CloTyro2_father->swap_assymetric(AP_LEFT);
            TEST_EXPECT(rootNode()->has_correct_mark_flags());

            env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

            AP_tree_nlen *CloTyro2_grandfather = CloTyro2_father->get_father();
            ap_assert(CloTyro2_grandfather->gr.grouped); // this is the group-root

            CloTyro2_grandfather->swap_assymetric(AP_LEFT);
            TEST_EXPECT__BROKEN(rootNode()->has_correct_mark_flags());

            env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

            CloTyro2_grandfather->swap_assymetric(AP_RIGHT); // (i guess) this swaps CloTyrob <-> CloInnoc (both unmarked)
            TEST_EXPECT(rootNode()->has_correct_mark_flags());
        }

        env.pop(); env.push(); TEST_EXPECT(rootNode()->has_correct_mark_flags());

        // swap in unmarked subtree
        CloCarni->get_father()->swap_assymetric(AP_LEFT);
        TEST_EXPECT(rootNode()->has_correct_mark_flags());

        env.pop(); TEST_EXPECT(rootNode()->has_correct_mark_flags());
    }
}

void TEST_undefined_branchlength() {
    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    AP_tree_nlen *root     = env.root_node();
    AP_tree_nlen *CorAquat = root->findLeafNamed("CorAquat");
    AP_tree_nlen *inner    = CorAquat->get_father()->get_father();

    AP_tree_nlen *sonOfRoot = root->get_leftson();
    ap_assert(!sonOfRoot->is_leaf);

    TEST_EXPECT(root && CorAquat && inner);

    GBT_LEN length[] = {
        0.0,
        0.8,
        AP_UNDEF_BL,
    };
    AP_tree_nlen *nodes[] = {
        sonOfRoot,
        CorAquat,
        inner,
    };

    for (size_t i = 0; i<ARRAY_ELEMS(length); ++i) {
        GBT_LEN testLen = length[i];
        for (size_t n = 0; n<ARRAY_ELEMS(nodes); ++n) {
            TEST_ANNOTATE(GBS_global_string("length=%.2f node=%zu", testLen, n));

            AP_tree_nlen *node   = nodes[n];
            GBT_LEN      oldLen = node->get_branchlength_unrooted();

            node->set_branchlength_unrooted(testLen);
            TEST_EXPECT_EQUAL(node->get_branchlength_unrooted(), testLen);

            node->set_branchlength_unrooted(oldLen);
            TEST_EXPECT(node->get_branchlength_unrooted() == oldLen);
        }
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
