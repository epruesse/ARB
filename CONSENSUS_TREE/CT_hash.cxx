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
#include <cmath>

#include <arb_sort.h>

PartRegistry::~PartRegistry() {
    for (PartSet::iterator p = parts.begin(); p != parts.end(); ++p) delete *p;
    for (PartSet::iterator p = artificial_parts.begin(); p != artificial_parts.end(); ++p) delete *p;
    for (PartVector::iterator p = sorted.begin(); p != sorted.end(); ++p) delete *p;
}

PART *PartRegistry::get_part() {
    /*! each call to get_part() returns the next part from the sorted PART-list.
     * build_sorted_list() has to be called once before.
     */

    arb_assert(retrieval_phase()); // call build_sorted_list() before retrieving

    PART *p = NULL;
    if (retrieved<sorted.size()) {
        std::swap(p, sorted[retrieved++]);
    }
    return p;
}

void PartRegistry::put_part_from_complete_tree(PART*& part) {
    //! insert part in PartRegistry (destroys/reassigns part)

    arb_assert(registration_phase());
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

void PartRegistry::put_artificial_part(PART*& part) {
    arb_assert(registration_phase());
    part->standardize();

    PartSet::iterator found = artificial_parts.find(part);
    if (found != artificial_parts.end()) {
        (*found)->addWeightAndLength(part);
        delete part;
    }
    else {
        artificial_parts.insert(part);
    }
    part = NULL;
}

void PartRegistry::put_part_from_partial_tree(PART*& part, const PART *partialTree) {
    //! insert part from partialTree into PartRegistry (destroys/reassigns part)
    //
    // Example:
    //
    // 'part':                     A---B        (A=part, B=invs)
    // set of missing species:     C
    //
    //                                   A
    //                                  /
    // assumed tree:               C---+
    //                                  \         .
    //                                   B
    //
    // inserted partitions: A---CB
    //                      AC---B
    //
    // Both partitions are inserted here. Since they mutually exclude each other,
    // only one of them can and will be inserted into the result tree.
    //
    // The algorithm assumes that all species missing in the partialTree,
    // reside on the bigger side of each branch of the partialTree.
    //
    // Therefore the partition representing that state is added with normal weight.
    // The opposite ("missing on smaller side of branch") is added with reduced
    // probability.
    //
    // Note: The PART 'C---AB' might exists at EACH branch in the deconstructed tree
    // and is inserted once globally (in deconstruct_partial_rootnode).

    arb_assert(registration_phase());

    PART *invs = part->clone();
    invs->invertInSuperset(partialTree);

    int diff = part->get_members() - invs->get_members();
    if (diff != 0) {
        if (diff<0) std::swap(part, invs);
        // now part contains bigger partition

        // Note: adding 'part' means "add unknown species to smaller side of the partition"
        int edges_in_part = leafs_2_edges(part->get_members()+1, ROOTED);
        int edges_in_invs = leafs_2_edges(invs->get_members()+1, ROOTED);

        double part_prob = double(edges_in_invs)/edges_in_part; // probability for one unkown species to reside inside smaller set
        arb_assert(part_prob>0.0 && part_prob<1.0);

        int    unknownCount     = partialTree->get_nonmembers();
        double all_in_part_prob = pow(part_prob, unknownCount); // probability ALL unkown species reside inside smaller set

        part->set_faked_weight(all_in_part_prob); // make "unknown in smaller" unlikely
                                                  // -> reduces chance to be picked and bootstrap IF picked
    }
    // else, if both partitions have equal size -> simply add both

    put_part_from_complete_tree(part); // inserts 'A---CB' (i.e. missing species added to invs, which is smaller)
    put_part_from_complete_tree(invs); // inserts 'B---CA' (i.e. missing species added to part)
}

inline bool insertionOrder_less(const PART *p1, const PART *p2) {
    return p1->insertionOrder_cmp(p2)<0;
}

void PartRegistry::build_sorted_list(double overall_weight) {
    //! sort the parts into insertion order.

    arb_assert(!parts.empty()); // nothing has been added
    arb_assert(registration_phase());

    merge_artificial_parts();

    copy(parts.begin(), parts.end(), std::back_insert_iterator<PartVector>(sorted));
    sort(sorted.begin(), sorted.end(), insertionOrder_less);

    parts.clear();

    for (PartVector::iterator pi = sorted.begin(); pi != sorted.end(); ++pi) {
        (*pi)->takeMean(overall_weight);
    }

    arb_assert(retrieval_phase());
}

void PartRegistry::merge_artificial_parts() {
    /*! 'artificial_parts' contains those partitions that did not occur in
     * one PARTIAL input tree.
     *
     * Not-yet-registered artificial_parts will be registered here.
     * Their weight is faked to zero (aka "never occurs").
     *
     * Already registered artificial_parts are only skipped -> their bootstrap
     * will be set correctly (just like when the branch as such does not exist
     * in a non-partial tree).
     */

    for (PartSet::iterator arti = artificial_parts.begin(); arti != artificial_parts.end(); ++arti) {
        if (parts.find(*arti) == parts.end()) {
            // partition was not added by any known edge -> add plain artificial edge
            (*arti)->set_faked_weight(0.0); // never occurs (implicates length=1.0; see get_len())
            parts.insert(*arti);
        }
        else {
            delete *arti;
        }
    }
    artificial_parts.clear();
}

