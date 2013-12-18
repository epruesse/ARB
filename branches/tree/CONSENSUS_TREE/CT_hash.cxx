// ============================================================= //
//                                                               //
//   File      : CT_hash.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_hash.hxx"
#include "CT_ntree.hxx"
#include "CT_ctree.hxx"

#include <arb_sort.h>

PART *PartRegistry::get_part() {
    /*! each call to get_part() returns the next part from the sorted PART-list.
     * build_sorted_list() has to be called once before.
     */

    arb_assert(!sorted.empty()); // call build_sorted_list before
    
    PART *p = NULL;
    if (retrieved<sorted.size()) {
        p = sorted[retrieved++];
    }
    return p;
}

void PartRegistry::put_part(PART*& part) {
    //! insert part in PartRegistry (destroys/reassigns part)

    arb_assert(sorted.empty()); // too late, already build_sorted_list
    part->standardize();

    PartSet::iterator found = parts.find(part);

    if (found != parts.end()) {
        (*found)->addWeightAndLength(part);
        delete part;
    }
    else {
        parts.insert(part);
    }
    part = NULL;
}

inline bool insertionOrder_less(const PART *p1, const PART *p2) {
    return p1->insertionOrder_cmp(p2)<0;
}

void PartRegistry::build_sorted_list(double overall_weight) {
    //! sort the parts into insertion order.

    arb_assert(sorted.empty());
    arb_assert(!parts.empty());

    copy(parts.begin(), parts.end(), std::back_insert_iterator<PartVector>(sorted));
    sort(sorted.begin(), sorted.end(), insertionOrder_less);

    parts.clear();

    for (size_t i = 0; i<sorted.size(); ++i) { // @@@ use iterator here
        PART *pi = sorted[i];
        pi->takeMean(overall_weight);
    }

    arb_assert(!sorted.empty());
}

double PartRegistry::find_max_weight() const {
    arb_assert(sorted.empty());
    arb_assert(!parts.empty());

    double maxWeight = 0.0;

    PartSet::const_iterator end = parts.end();
    for (PartSet::const_iterator p = parts.begin(); p != end; ++p) {
        maxWeight = std::max(maxWeight, (*p)->get_weight());
    }

    arb_assert(maxWeight>0.0);
    return maxWeight;
}

