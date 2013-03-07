// =============================================================== //
//                                                                 //
//   File      : arb_sort.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "arb_sort.h"
#include <cstring>


struct comparator {
    gb_compare_function  compare;
    void                *client_data;
};

static comparator Compare; // current compare function + client data

static int qsort_compare(const void *v1, const void *v2) {
    return Compare.compare(*(void**)v1, *(void**)v2, Compare.client_data);
}

void GB_sort(void **array, size_t first, size_t behind_last, gb_compare_function compare, void *client_data) {
    /* sort 'array' of pointers from 'first' to last element
     * (specified by following element 'behind_last')
     * 'compare' is a compare function, with a strcmp-like result value
     */

    Compare.compare     = compare;
    Compare.client_data = client_data;

    qsort(array, behind_last-first, sizeof(*array), qsort_compare);
}

// --------------------------
//      some comparators

int GB_string_comparator(const void *v0, const void *v1, void */*unused*/) {
    return strcmp((const char *)v0, (const char *)v1);
}

