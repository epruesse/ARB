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

#define ph_assert(cond) arb_assert(cond)

typedef double AP_FLOAT;

struct PH_NEIGHBOUR_DIST {
    PH_NEIGHBOUR_DIST();
    long    i, j;
    AP_FLOAT val;
    PH_NEIGHBOUR_DIST *next, *previous;
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

    AP_FLOAT get_max_di(AP_FLOAT **m);
    void     remove_taxa_from_dist_list(long i);
    void     add_taxa_to_dist_list(long j);
    AP_FLOAT get_max_net_divergence();
    void     remove_taxa_from_swap_tab(long i);

public:

    PH_NEIGHBOURJOINING(AP_FLOAT **m, long size);
    ~PH_NEIGHBOURJOINING();

    void     join_nodes(long i, long j, AP_FLOAT &leftl, AP_FLOAT& rightlen);
    void     get_min_ij(long& i, long& j);
    void     get_last_ij(long& i, long& j);
    AP_FLOAT get_dist(long i, long j);
};

#else
#error NJ.hxx included twice
#endif // NJ_HXX
