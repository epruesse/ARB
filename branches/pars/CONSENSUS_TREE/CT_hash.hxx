// =============================================================== //
//                                                                 //
//   File      : CT_hash.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef CT_HASH_HXX
#define CT_HASH_HXX

#ifndef CT_PART_HXX
#include "CT_part.hxx"
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

typedef PART *PARTptr;
typedef std::set<PARTptr, bool (*)(const PART *, const PART *)> PartSet;
typedef std::vector<PARTptr> PartVector;

inline bool topological_less(const PART *p1, const PART *p2) { return p1->topological_cmp(p2)<0; }

class PartRegistry : virtual Noncopyable {
    PartSet    parts;
    PartSet    artificial_parts; // these occurred in no tree
    PartVector sorted;
    size_t     retrieved;

    bool registration_phase() const { return sorted.empty(); }
    bool retrieval_phase() const { return !registration_phase(); }

    void merge_artificial_parts();

public:
    PartRegistry()
        : parts(topological_less),
          artificial_parts(topological_less),
          retrieved(0)
    {
        arb_assert(registration_phase());
    }
    ~PartRegistry();

    void put_part_from_complete_tree(PART*& part);
    void put_part_from_partial_tree(PART*& part, const PART *partialTree);
    void put_artificial_part(PART*& part);

    void  build_sorted_list(double overall_weight);
    PART *get_part();

    size_t size() const {
        if (registration_phase()) return parts.size()+artificial_parts.size();
        return sorted.size();
    }
};

#else
#error CT_hash.hxx included twice
#endif // CT_HASH_HXX
