// =============================================================== //
//                                                                 //
//   File      : NJ.cxx                                            //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NJ.hxx"
#include <neighbourjoin.hxx>
#include <TreeNode.h>
#include <arb_diff.h>

PH_NEIGHBOUR_DIST::PH_NEIGHBOUR_DIST()
{
    memset((char *)this, 0, sizeof(PH_NEIGHBOUR_DIST));
}


void PH_NEIGHBOURJOINING::remove_taxa_from_dist_list(long i) { // O(n/2)
    long a, j;
    PH_NEIGHBOUR_DIST *nd;
    for (a=0; a<swap_size; a++) {
        j = swap_tab[a];
        if (i==j) continue;
        if (j<i) {
            nd = &(dist_matrix[i][j]);
        }
        else {
            nd = &(dist_matrix[j][i]);
        }
        nd->remove();
        net_divergence[j] -= nd->val;   // corr net divergence
    }
}
void PH_NEIGHBOURJOINING::add_taxa_to_dist_list(long i) // O(n/2)
{
    long a;
    long pos, j;
    PH_NEIGHBOUR_DIST *nd;
    AP_FLOAT my_nd = 0.0;
    for (a=0; a<swap_size; a++) {
        j = swap_tab[a];
        if (i==j) continue;
        if (j<i) {
            nd = &(dist_matrix[i][j]);
        }
        else {
            nd = &(dist_matrix[j][i]);
        }
        ph_assert(!nd->previous);
        pos = (int)(nd->val*dist_list_corr);
        if (pos >= dist_list_size) {
            pos = dist_list_size-1;
        } else if (pos<0)
            pos = 0;
        nd->add(&(dist_list[pos]));

        net_divergence[j] += nd->val;   // corr net divergence
        my_nd += nd->val;
    }
    net_divergence[i] = my_nd;
}

AP_FLOAT PH_NEIGHBOURJOINING::get_max_net_divergence()      // O(n/2)
{
    long a, i;
    AP_FLOAT max = 0.0;
    for (a=0; a<swap_size; a++) {
        i = swap_tab[a];
        if (net_divergence[i] > max) max = net_divergence[i];
    }
    return max;
}

void PH_NEIGHBOURJOINING::remove_taxa_from_swap_tab(long i) // O(n/2)
{
    long a;
    long *source, *dest;
    source = dest = swap_tab;
    for (a=0; a<swap_size; a++) {
        if (swap_tab[a] == i) {
            source++;
        }
        else {
            *(dest++) = *(source++);
        }
    }
    swap_size --;
}

PH_NEIGHBOURJOINING::PH_NEIGHBOURJOINING(const AP_smatrix& smatrix) {
    size = smatrix.size();

    // init swap tab
    swap_size = size;
    swap_tab  = new long[size];
    for (long i=0; i<swap_size; i++) swap_tab[i] = i;

    ARB_calloc(net_divergence, size);

    dist_list_size = size;                                  // hope to be the best
    dist_list      = new PH_NEIGHBOUR_DIST[dist_list_size]; // the roots, no elems
    dist_list_corr = (dist_list_size-2.0)/smatrix.get_max_value();

    dist_matrix = new PH_NEIGHBOUR_DIST*[size];
    for (long i=0; i<size; i++) {
        dist_matrix[i] = new PH_NEIGHBOUR_DIST[i];
        for (long j=0; j<i; j++) {
            dist_matrix[i][j].val = smatrix.fast_get(i, j);
            dist_matrix[i][j].i = i;
            dist_matrix[i][j].j = j;
        }
    }
    for (long i=0; i<size; i++) {
        swap_size = i;              // to calculate the correct net divergence..
        add_taxa_to_dist_list(i);   // ..add to dist list and add n.d.
    }
    swap_size = size;
}

PH_NEIGHBOURJOINING::~PH_NEIGHBOURJOINING() {
    for (long i=0; i<size; i++) delete [] dist_matrix[i];
    delete [] dist_matrix;
    delete [] dist_list;
    free(net_divergence);
    delete [] swap_tab;
}

AP_FLOAT PH_NEIGHBOURJOINING::get_min_ij(long& mini, long& minj) { // O(n*n/speedup)
    // returns minval (only used by test inspection)

    AP_FLOAT maxri = get_max_net_divergence(); // O(n/2)
    PH_NEIGHBOUR_DIST *dl;
    long stat  = 0;
    AP_FLOAT x;
    AP_FLOAT minval;
    minval = 100000.0;
    AP_FLOAT N_1   = 1.0/(swap_size-2.0);
    maxri = maxri*2.0*N_1;
    long pos;

    get_last_ij(mini, minj);

    for (pos=0; pos<dist_list_size; pos++) {
        if (minval < pos/dist_list_corr - maxri) break;
        // no way to get a better minimum
        dl = dist_list[pos].next;   // first entry does not contain information
        for (; dl; dl=dl->next) {
            x = (net_divergence[dl->i] + net_divergence[dl->j])*N_1;
            if (dl->val-x<minval) {
                minval = dl->val -x;
                minj = dl->i;
                mini = dl->j;
            }
            stat++;
        }
    }
    return minval;
}

void PH_NEIGHBOURJOINING::join_nodes(long i, long j, AP_FLOAT &leftl, AP_FLOAT& rightl)
{
    PH_NEIGHBOUR_DIST **d = dist_matrix;
    AP_FLOAT dji;

    AP_FLOAT dist = get_dist(i, j);

    leftl = dist*.5 + (net_divergence[i] - net_divergence[j])*.5/
        (swap_size - 2.0);
    rightl = dist - leftl;

    remove_taxa_from_dist_list(j);
    remove_taxa_from_swap_tab(j);
    remove_taxa_from_dist_list(i);

    long a, k;
    dji = d[j][i].val;
    for (a=0; a<swap_size; a++) {
        k = swap_tab[a];
        if (k==i) continue; // k == j not possible
        if (k>i) {
            if (k>j) {
                d[k][i].val = .5*(d[k][i].val + d[k][j].val - dji);
            }
            else {
                d[k][i].val = .5*(d[k][i].val + d[j][k].val - dji);
            }
        }
        else {
            d[i][k].val = 0.5 * (d[i][k].val + d[j][k].val - dji);
        }
    }
    add_taxa_to_dist_list(i);
}

void PH_NEIGHBOURJOINING::get_last_ij(long& i, long& j)
{
    i = swap_tab[0];
    j = swap_tab[1];
}

AP_FLOAT PH_NEIGHBOURJOINING::get_dist(long i, long j)
{
    return dist_matrix[j][i].val;
}

TreeNode *neighbourjoining(const char *const *names, const AP_smatrix& smatrix, TreeRoot *troot) { // @@@ pass ConstStrArray
    // structure_size >= sizeof(TreeNode);
    // lower triangular matrix
    // size: size of matrix

    PH_NEIGHBOURJOINING   nj(smatrix);
    TreeNode            **nodes = ARB_calloc<TreeNode*>(smatrix.size());

    for (size_t i=0; i<smatrix.size(); i++) {
        nodes[i] = troot->makeNode();
        nodes[i]->name = strdup(names[i]);
        nodes[i]->is_leaf = true;
    }

    for (size_t i=0; i<smatrix.size()-2; i++) {
        long a, b;
        nj.get_min_ij(a, b);

        AP_FLOAT ll, rl;
        nj.join_nodes(a, b, ll, rl);

        TreeNode *father = troot->makeNode();
        father->leftson  = nodes[a];
        father->rightson = nodes[b];
        father->leftlen  = ll;
        father->rightlen = rl;
        nodes[a]->father = father;
        nodes[b]->father = father;
        nodes[a]         = father;
    }

    {
        long a, b;
        nj.get_last_ij(a, b);

        AP_FLOAT dist = nj.get_dist(a, b);

        AP_FLOAT ll = dist*0.5;
        AP_FLOAT rl = dist*0.5;

        TreeNode *father    = troot->makeNode();

        father->leftson  = nodes[a];
        father->rightson = nodes[b];

        father->leftlen  = ll;
        father->rightlen = rl;

        nodes[a]->father = father;
        nodes[b]->father = father;

        free(nodes);

        father->get_tree_root()->change_root(NULL, father);
        return father;
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static const AP_FLOAT EPSILON = 0.0001;

static arb_test::match_expectation min_ij_equals(PH_NEIGHBOURJOINING& nj, long expected_i, long expected_j, AP_FLOAT expected_minval) {
    using namespace   arb_test;
    expectation_group expected;

    long     i, j;
    AP_FLOAT minval = nj.get_min_ij(i, j);

    expected.add(that(i).is_equal_to(expected_i));
    expected.add(that(j).is_equal_to(expected_j));
    expected.add(that(minval).fulfills(epsilon_similar(EPSILON), expected_minval));

    return all().ofgroup(expected);
}

#define TEST_EXPECT_MIN_IJ(nj,i,j,minval) TEST_EXPECTATION(min_ij_equals(nj,i,j,minval))

void TEST_neighbourjoining() {
    const size_t SIZE = 4;

#define TEST_FORWARD_ORDER // @@@ changing the order of nodes here changes the resulting trees
                           // i do not understand, if that means there is sth wrong or not..

#if defined(TEST_FORWARD_ORDER)
    const char *names[SIZE] = { "A", "B", "C", "D"};
    enum { A, B, C, D };
#else // !defined(TEST_FORWARD_ORDER)
    const char *names[SIZE] = { "D", "C", "B", "A"};
    enum { D, C, B, A };
#endif

    for (int test = 1; test <= 2; ++test) {
        AP_smatrix sym_matrix(SIZE);

        // Note: values used here are distances
        for (size_t i = 0; i < SIZE; ++i) sym_matrix.set(i, i, 0.0);

        sym_matrix.set(A, B, 0.0765); sym_matrix.set(A, C, 0.1619); sym_matrix.set(A, D, 0.2266);
        sym_matrix.set(B, C, 0.1278); sym_matrix.set(B, D, 0.2061);

        switch (test) {
            case 1: sym_matrix.set(C, D, 0.1646); break;
            case 2: sym_matrix.set(C, D, 0.30); break;
            default: arb_assert(0); break;
        }

        // check net_divergence values:
        {
            PH_NEIGHBOURJOINING nj(sym_matrix);

            TEST_EXPECT_SIMILAR(nj.get_net_divergence(A), 0.4650, EPSILON);
            TEST_EXPECT_SIMILAR(nj.get_net_divergence(B), 0.4104, EPSILON);
            switch (test) {
                case 1:
                    TEST_EXPECT_SIMILAR(nj.get_net_divergence(C), 0.4543, EPSILON);
                    TEST_EXPECT_SIMILAR(nj.get_net_divergence(D), 0.5973, EPSILON);

#define EXPECTED_MIN_IJ -0.361200
#if defined(TEST_FORWARD_ORDER)
                    TEST_EXPECT_MIN_IJ(nj, A, B, EXPECTED_MIN_IJ);
#else // !defined(TEST_FORWARD_ORDER)
                    TEST_EXPECT_MIN_IJ(nj, D, C, EXPECTED_MIN_IJ);
#endif
#undef EXPECTED_MIN_IJ
                    break;
                case 2:
                    TEST_EXPECT_SIMILAR(nj.get_net_divergence(C), 0.5897, EPSILON);
                    TEST_EXPECT_SIMILAR(nj.get_net_divergence(D), 0.7327, EPSILON);

#define EXPECTED_MIN_IJ -0.372250
#if defined(TEST_FORWARD_ORDER)
#if defined(ARB_64)
                    TEST_EXPECT_MIN_IJ(nj, B, C, EXPECTED_MIN_IJ);
#else // !defined(ARB_64)
                    TEST_EXPECT_MIN_IJ(nj, A, D, EXPECTED_MIN_IJ); // @@@ similar to 64-bit w/o TEST_FORWARD_ORDER
#endif
#else // !defined(TEST_FORWARD_ORDER)
                    TEST_EXPECT_MIN_IJ(nj, D, A, EXPECTED_MIN_IJ); // @@@ no differences between 32-/64-bit version w/o TEST_FORWARD_ORDER
#endif
#undef EXPECTED_MIN_IJ
                    break;
                default: arb_assert(0); break;
            }

        }

        TreeNode *tree = neighbourjoining(names, sym_matrix, new SimpleRoot);

        switch (test) {
#if defined(TEST_FORWARD_ORDER)
#if defined(ARB_64)
            case 1: TEST_EXPECT_NEWICK(nSIMPLE, tree, "(((A,B),C),D);"); break;
            case 2: TEST_EXPECT_NEWICK(nSIMPLE, tree, "((A,(B,C)),D);"); break;
#else // !defined(ARB_64)
                // @@@ 32bit version behaves different
            case 1: TEST_EXPECT_NEWICK(nSIMPLE, tree, "(((A,B),D),C);"); break;
            case 2: TEST_EXPECT_NEWICK(nSIMPLE, tree, "(((A,D),B),C);"); break; // similar to 64-bit w/o TEST_FORWARD_ORDER
#endif
#else // !defined(TEST_FORWARD_ORDER)
                // @@@ no differences between 32-/64-bit version w/o TEST_FORWARD_ORDER
            case 1: TEST_EXPECT_NEWICK(nSIMPLE, tree, "(((D,C),A),B);"); break;
            case 2: TEST_EXPECT_NEWICK(nSIMPLE, tree, "(((D,A),B),C);"); break;
#endif
            default: arb_assert(0); break;
        }

        destroy(tree);
    }
}
TEST_PUBLISH(TEST_neighbourjoining);

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
