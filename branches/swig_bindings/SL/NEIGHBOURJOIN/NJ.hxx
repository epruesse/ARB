// =============================================================== //
//                                                                 //
//   File      : NJ.hxx                                            //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NJ_HXX
#define NJ_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef AP_MATRIX_HXX
#include <AP_matrix.hxx>
#endif

#define ph_assert(cond) arb_assert(cond)

typedef double AP_FLOAT;

struct PH_NEIGHBOUR_DIST {
    long     i, j;
    AP_FLOAT val;

    PH_NEIGHBOUR_DIST *next, *previous;

    PH_NEIGHBOUR_DIST();

    void remove() {
        ph_assert(previous); // already removed
        if (next) { next->previous = previous; }
        previous->next = next; previous = 0;
    };

    void add(PH_NEIGHBOUR_DIST *root) {
        next = root->next;
        root->next = this;
        previous = root;
        if (next) next->previous = this;
    };
};

class PH_NEIGHBOURJOINING : virtual Noncopyable {
    PH_NEIGHBOUR_DIST **dist_matrix;
    PH_NEIGHBOUR_DIST  *dist_list;   // array of dist_list lists
    long                dist_list_size;
    AP_FLOAT            dist_list_corr;
    AP_FLOAT           *net_divergence;

    long  size;
    long *swap_tab;
    long  swap_size;

    void     remove_taxa_from_dist_list(long i);
    void     add_taxa_to_dist_list(long j);
    AP_FLOAT get_max_net_divergence();
    void     remove_taxa_from_swap_tab(long i);

public:

    PH_NEIGHBOURJOINING(const AP_smatrix& smatrix);
    ~PH_NEIGHBOURJOINING();

    void     join_nodes(long i, long j, AP_FLOAT &leftl, AP_FLOAT& rightlen);
    AP_FLOAT get_min_ij(long& i, long& j);
    void     get_last_ij(long& i, long& j);
    AP_FLOAT get_dist(long i, long j);

#if defined(UNIT_TESTS) // UT_DIFF
    // test inspection
    AP_FLOAT get_net_divergence(long i) { return net_divergence[i]; }
#endif
};

#else
#error NJ.hxx included twice
#endif // NJ_HXX
