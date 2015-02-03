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

    EdgeChain chain(edge, UNLIMITED, ANY_EDGE);
    while (chain) {
        AP_tree_edge *curr = *chain;
        ++chain;
        delete curr;
    }
}


void AP_tree_edge::tailDistance(AP_tree_nlen *n)
{
    ap_assert(!n->is_leaf);    // otherwise call was not necessary!

    int i0 = n->indexOf(this);          // index of this
    int i1 = i0==0 ? 1 : 0;             // the indices of the other two nodes (beside n)
    int i2 = i1==1 ? 2 : (i0==1 ? 2 : 1);
    AP_tree_edge *e1 = n->edge[i1];     // edges to the other two nodes
    AP_tree_edge *e2 = n->edge[i2];

    cout << "tail n=" << n << " d(n)=" << n->distance << " ";

    if (e1 && e2)
    {
        AP_tree_nlen *n1 = e1->node[1-n->index[i1]]; // the other two nodes
        AP_tree_nlen *n2 = e2->node[1-n->index[i2]];

        cout <<  "n1=" << n1 << " d(n1)=" << n1->distance <<
            " n2=" << n2 << " d(n2)=" << n2->distance << " ";

        if (n1->distance==n2->distance)
        {
            if (n1->distance>n->distance && n2->distance>n->distance)
            {
                // both distances (of n1 and n2) are greather that distance of n
                // so its possible that the nearest connection the border was
                // via node n and we must recalculate the distance into the other
                // directions

                ap_assert(n1->distance==n2->distance);

                cout << "special tailDistance-case\n";
                e1->tailDistance(n1);
                e2->tailDistance(n2);

                if (n1->distance<n2->distance)
                {
                    n->distance = n1->distance+1;
                    if (!e2->distanceOK()) e2->calcDistance();
                }
                else
                {
                    n->distance = n2->distance+1;
                    if (!e1->distanceOK()) e1->calcDistance();
                }
            }
            else
            {
                cout << "tail case 2\n";
                n->distance = n1->distance+1;
            }
        }
        else
        {
            ap_assert(n1->distance != n2->distance);

            if (n1->distance<n2->distance)
            {
                if (n1->distance<n->distance)
                {
                    // in this case the distance via n1 is the same as
                    // the distance via the cutted edge - so we do nothing

                    cout << "tail case 3\n";
                    ap_assert(n1->distance==(n->distance-1));
                }
                else
                {
                    cout << "tail case 4\n";
                    ap_assert(n1->distance==n->distance);
                    ap_assert(n2->distance==(n->distance+1));

                    n->distance = n1->distance+1;
                    e2->tailDistance(n2);
                }
            }
            else
            {
                ap_assert(n2->distance<n1->distance);

                if (n2->distance<n->distance)
                {
                    // in this case the distance via n2 is the same as
                    // the distance via the cutted edge - so we do nothing

                    cout << "tail case 5\n";
                    ap_assert(n2->distance==(n->distance-1));
                }
                else
                {
                    cout << "tail case 6\n";
                    ap_assert(n2->distance==n->distance);
                    ap_assert(n1->distance==(n->distance+1));

                    n->distance = n2->distance+1;
                    e1->tailDistance(n1);
                }
            }
        }

        cout << "d(n)=" << n->distance <<
            " d(n1)=" << n1->distance <<
            " d(n2)=" << n2->distance <<
            " D(e1)=" << e1->Distance() <<
            " D(e2)=" << e2->Distance() <<
            " dtb(e1)=" << e1->distanceToBorder(INT_MAX, n) <<
            " dtb(e2)=" << e2->distanceToBorder(INT_MAX, n) << endl;
    }
    else
    {
        if (e1)
        {
            AP_tree_nlen *n1 = e1->node[1-n->index[i1]];

            cout << "tail case 7\n";
            ap_assert(n1!=0);
            if (n1->distance>n->distance) e1->tailDistance(n1);
            n->distance = n1->distance+1;

            cout << "d(n)=" << n->distance <<
                " d(n1)=" << n1->distance <<
                " D(e1)=" << e1->Distance() <<
                " dtb(e1)=" << e1->distanceToBorder(INT_MAX, n) << endl;
        }
        else if (e2)
        {
            AP_tree_nlen *n2 = e2->node[1-n->index[i2]];

            cout << "tail case 8\n";
            ap_assert(n2!=0);
            cout << "d(n2)=" << n2->distance << " d(n)=" << n->distance << endl;

            if (n2->distance>n->distance) e2->tailDistance(n2);
            n->distance = n2->distance+1;

            cout << "d(n)=" << n->distance <<
                " d(n2)=" << n2->distance <<
                " D(e2)=" << e2->Distance() <<
                " dtb(e2)=" << e2->distanceToBorder(INT_MAX, n) << endl;
        }
        else
        {
            cout << "tail case 9\n";
            n->distance = INT_MAX;
        }
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

void AP_tree_edge::calcDistance()
{
    ap_assert(!distanceOK());  // otherwise call was not necessary
    ap_assert (node[0]->distance!=node[1]->distance);

    if (node[0]->distance < node[1]->distance)  // node[1] has wrong distance
    {
        cout << "dist(" << node[1] << ") " << node[1]->distance;

        if (node[1]->distance==INT_MAX) // not initialized -> do not recurse
        {
            node[1]->distance = node[0]->distance+1;
            cout  << " -> " << node[1]->distance << endl;
        }
        else
        {
            node[1]->distance = node[0]->distance+1;
            cout  << " -> " << node[1]->distance << endl;

            if (!node[1]->is_leaf)
            {
                for (int e=0; e<3; e++)     // recursively correct distance where necessary
                {
                    AP_tree_edge *ed = node[1]->edge[e];
                    if (ed && ed!=this && !ed->distanceOK()) ed->calcDistance();
                }
            }
        }
    }
    else                        // node[0] has wrong distance
    {
        cout << "dist(" << node[0] << ") " << node[0]->distance;

        if (node[0]->distance==INT_MAX) // not initialized -> do not recurse
        {
            node[0]->distance = node[1]->distance+1;
            cout  << " -> " << node[0]->distance << endl;
        }
        else
        {
            node[0]->distance = node[1]->distance+1;
            cout  << " -> " << node[0]->distance << endl;

            if (!node[0]->is_leaf)
            {
                for (int e=0; e<3; e++)     // recursively correct distance where necessary
                {
                    AP_tree_edge *ed = node[0]->edge[e];
                    if (ed && ed!=this && !ed->distanceOK()) ed->calcDistance();
                }
            }
        }
    }

    ap_assert(distanceOK());   // invariant of AP_tree_edge (if fully constructed)
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

size_t AP_tree_edge::buildChainInternal(int depth, EdgeSpec whichEdges, const AP_tree_nlen *skip, int distanceToStart, AP_tree_edge*& prev) {
    size_t added = 1;

    data.distance                 = distanceToStart;
    if (prev) prev->next_in_chain = this;
    this->next_in_chain           = NULL;
    prev                          = this;

    if (depth) {
        if (whichEdges == MARKED_VISIBLE_EDGES) {
            if (node[0]->gr.hidden ||
                node[1]->gr.hidden ||
                ((!node[0]->gr.has_marked_children) && (!node[1]->gr.has_marked_children))) return added;
        }

        for (int n=0; n<2; n++) {
            if (node[n]!=skip && !node[n]->is_leaf) {
                for (int e=0; e<3; e++) {
                    AP_tree_edge * Edge = node[n]->edge[e];
                    if (Edge != this) {
                        added += Edge->buildChainInternal(depth-1, whichEdges, node[n], distanceToStart+1, prev);
                    }
                }
            }
        }
    }
    return added;
}

bool EdgeChain::exists = false;

EdgeChain::EdgeChain(AP_tree_edge *start_, int depth, EdgeSpec whichEdges, const AP_tree_nlen *skip)
    : start(start_),
      curr(start)
{
    /*! build a chain of edges for further processing
     * @param start_           start edge
     * @param depth            specifies how far to recurse into tree (-1 = recurse whole tree; 0 = this edge only)
     * @param whichEdges       if MARKED_VISIBLE_EDGES -> do not descend into folded subtrees and subtrees w/o marked species
     * @param skip             previous node (will not recurse beyond)
     */

    ap_assert(!exists); // only one existing chain is allowed!
    exists = true;

    AP_tree_edge *prev = NULL;
    len = start->buildChainInternal(depth, whichEdges, skip, 0, prev);
}

int AP_tree_edge::distanceToBorder(int maxsearch, AP_tree_nlen *skipNode) const {
    /*! @return the minimal distance to the borders of the tree (aka leafs).
     * a return value of 0 means: one of the nodes is a leaf
     *
     * @param maxsearch         max search depth
     * @param skipNode          do not descent into that part of the tree
     */

    if ((node[0] && node[0]->is_leaf) || (node[1] && node[1]->is_leaf))
    {
        return 0;
    }

    for (int n=0; n<2 && maxsearch; n++)
    {
        if (node[n] && node[n]!=skipNode)
        {
            for (int e=0; e<3; e++)
            {
                AP_tree_edge *ed = node[n]->edge[e];

                if (ed && ed!=this)
                {
                    int dist = ed->distanceToBorder(maxsearch-1, node[n])+1;
                    if (dist<maxsearch) maxsearch = dist;
                }
            }
        }
    }

    return maxsearch;
}

void AP_tree_edge::countSpecies(int deep, const AP_tree_nlen *skip)
{
    if (!skip)
    {
        speciesInTree = 0;
        nodesInTree   = 0;
    }

    if (deep--)
    {
        for (int n=0; n<2; n++)
        {
            if (node[n]->is_leaf)
            {
                speciesInTree++;
                nodesInTree++;
            }
            else if (node[n]!=skip)
            {
                nodesInTree++;

                for (int e=0; e<3; e++)
                {
                    if (node[n]->edge[e]!=this)
                    {
                        node[n]->edge[e]->countSpecies(deep, node[n]);
                    }
                }
            }
        }
    }
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
    if (!rootNode())         return 0.0;
    if (rootNode()->is_leaf) return rootNode()->costs();

    AP_tree_edge *oldRootEdge = rootEdge();

    if (depth>=0) set_root();

    AP_FLOAT old_parsimony = rootNode()->costs();
    AP_FLOAT new_parsimony = old_parsimony;

    ap_assert(allBranchlengthsAreDefined(rootNode()));

    EdgeChain    chain(this, depth, whichEdges, skipNode);
    bool         recalc_lengths = mode & AP_BL_BL_ONLY;
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
        AP_tree_edge *edge = *chain; ++chain;
        AP_tree_nlen *son  = edge->sonNode();
        AP_tree_nlen *fath = son;

        if (edge->otherNode(fath)==fath->get_father()) fath = fath->get_father();
        if (fath->father) {
            if (fath->father->father) {
                fath->set_root();
                new_parsimony = rootNode()->costs();
            }
        }
        if (mode & AP_BL_BOOTSTRAP_LIMIT) {
            if (fath->father) {
                son->set_root();
                new_parsimony = rootNode()->costs();
            }

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

int AP_tree_edge::dumpNNI = 0;
int AP_tree_edge::distInsertBorder;
int AP_tree_edge::basesChanged;
int AP_tree_edge::speciesInTree;
int AP_tree_edge::nodesInTree;

AP_FLOAT AP_tree_edge::nni_mutPerSite(AP_FLOAT pars_one, AP_BL_MODE mode, MutationsPerSite *mps)
{
    AP_tree_nlen *root = rootNode();

    if (node[0]->is_leaf || node[1]->is_leaf) { // a son at root
#if 0
        // calculate branch lengths at root
        if (mode&AP_BL_BL_ONLY) {
            AP_tree_nlen *tip, *brother;

            if (node[0]->is_leaf) {
                tip = node[0]; brother = node[1];
            }
            else {
                tip = node[1]; brother = node[0];
            }

            AP_FLOAT    Blen = pars_one - brother->costs();
            AP_FLOAT    Seq_len = tip->sequence->real_len();

            node[0]->father->leftlen = node[0]->father->rightlen = Blen/Seq_len*.5;
        }
#endif
        return pars_one;
    }

    AP_tree_edge_data oldData = data;

    AP_FLOAT    parsbest = pars_one,
        pars_two,
        pars_three;
    AP_tree_nlen *son = sonNode();
    int     betterValueFound = 0;
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
    {               // ********* first nni
        ap_main->remember();
        son->swap_assymetric(AP_LEFT);
        pars_two = root->costs(mps ? mps->data(1) : NULL);

        if (pars_two <= parsbest) {
            ap_main->accept_if(mode & AP_BL_NNI_ONLY);

            parsbest         = pars_two;
            betterValueFound = (int)(pars_one-pars_two);
        }
        else {
            ap_main->revert();
        }
    }
    {               // ********** second nni
        ap_main->remember();
        son->swap_assymetric(AP_RIGHT);
        pars_three = root->costs(mps ? mps->data(2) : NULL);

        if (pars_three <= parsbest) {
            ap_main->accept_if(mode & AP_BL_NNI_ONLY);

            parsbest         = pars_three;
            betterValueFound = (int)(pars_one-pars_three);
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

    // zu Auswertungszwecken doch unsortiert uebernehmen:

    data.parsValue[0] = pars_one;
    data.parsValue[1] = pars_two;
    data.parsValue[2] = pars_three;


    if (dumpNNI) {
        if (dumpNNI==2) GBK_terminate("NNI called between optimize and statistics");

        AP_tree_nlen *node0 = this->node[0];
        AP_tree_nlen *node1 = this->node[1];
        if (node0->gr.leaf_sum > node1->gr.leaf_sum) { // node0 is final son
            node0 = node1;
        }

        static int num = 0;

        node0->use_as_remark(GBS_global_string_copy("%i %4.0f:%4.0f:%4.0f     %4.0f:%4.0f:%4.0f\n",
                                                    num++,
                                                    oldData.parsValue[0], oldData.parsValue[1], oldData.parsValue[2],
                                                    data.parsValue[0], data.parsValue[1], data.parsValue[2]));

        cout
            << setw(4) << distInsertBorder
            << setw(6) << basesChanged
            << setw(4) << distanceToBorder()
            << setw(4) << data.distance
            << setw(4) << betterValueFound
            << setw(8) << oldData.parsValue[0]
            << setw(8) << oldData.parsValue[1]
            << setw(8) << oldData.parsValue[2]
            << setw(8) << data.parsValue[0]
            << setw(8) << data.parsValue[1]
            << setw(8) << data.parsValue[2]
            << '\n';
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

void AP_tree_edge::mixTree() {
    EdgeChain chain(this, UNLIMITED, ANY_EDGE);

    long edges = chain.size();
    long leafs = edges_2_leafs(edges, UNROOTED);

    double balanced_depth = log10(leafs) / log10(2);
    int    count          = int(balanced_depth*2.0 + .5);
    if (count<1) count    = 1;

    arb_progress progress(count*edges);
    while (count-- && !progress.aborted()) {
        chain.restart();
        while (chain) {
            AP_tree_nlen *son = (*chain)->sonNode();
            if (!son->is_leaf) son->swap_assymetric(GB_random(2) ? AP_LEFT : AP_RIGHT);
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
