// ============================================================= //
//                                                               //
//   File      : CT_part.hxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_PART_HXX
#define CT_PART_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef CT_DEF_HXX
#include "CT_def.hxx"
#endif
#ifndef CT_MEM_HXX
#include "CT_mem.hxx"
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif


typedef unsigned int PELEM;
class PART;

class PartitionSize {
    PELEM cutmask;  // this mask is used to zero unused bits from the last long (in PART::p)
    int   longs;    // number of longs per part
    int   bits;     // number of bits per part ( = overall number of species)
    
    mutable size_t id;      // unique id (depending on part creation order, which depends on the added trees)

public:
    PartitionSize(int len);

    int get_longs() const { return longs; }
    int get_bits() const { return bits; }
    size_t get_unique_id() const { id++; return id; }
    PELEM get_cutmask() const { return cutmask; }

    PART *create_root() const;
    PELEM *alloc_mem() const { return (PELEM*)getmem(get_longs()*sizeof(PELEM)); }
};

class PART {
    const class PartitionSize *info;

    PELEM   *p;
    GBT_LEN  len;               // length between two nodes (weighted by weight)
    double   weight;            // sum of weights
    size_t   id;

    int members;

    PART(const PART& other)
        : info(other.info),
          p(info->alloc_mem()),
          len(other.len),
          weight(other.weight),
          id(info->get_unique_id()), 
          members(other.members)
    {
        int longs = get_longs();
        for (int i = 0; i<longs; ++i) {
            p[i] = other.p[i];
        }
        arb_assert(is_valid());
    }
    DECLARE_ASSIGNMENT_OPERATOR(PART);

    int count_members() const;

    void set_weight(double pc) {
        double lenScale  = pc/weight;
        weight           = pc;
        len             *= lenScale;
    }

public:

    PART(const PartitionSize* size_info, double weight_)
        : info(size_info),
          p(info->alloc_mem()),
          len(0),
          weight(weight_),
          id(info->get_unique_id()),
          members(0)
    {
        arb_assert(is_valid());
    }
    ~PART() {
        free(p);
    }

    PART *clone() const { return new PART(*this); }

    int get_longs() const { return info->get_longs(); }
    int get_maxsize() const { return info->get_bits(); }
    PELEM get_cutmask() const { return info->get_cutmask(); }

    bool is_valid() const {
        PELEM last = p[get_longs()-1];

        bool only_valid_bits    = (last&get_cutmask()) == last;
        bool valid_member_count = members >= 0 && members <= get_maxsize();
        bool valid_weight       = weight>0.0;

        return only_valid_bits && valid_member_count && valid_weight;
    }

    void setbit(int pos) {
        /*! set the bit of the part 'p' at the position 'pos'
         */
        arb_assert(is_valid());

        int   idx = pos / sizeof(PELEM) / 8;
        PELEM bit = 1 << (pos % (sizeof(PELEM)*8));

        if (!(p[idx]&bit)) {
            p[idx] |= bit;
            members++;
        }

        arb_assert(is_valid());
    }
    void set_len(GBT_LEN length) {
        arb_assert(is_valid());
        len = length*weight;
    }
    GBT_LEN get_len() const {
        return len/weight;
    }

    double get_weight() const { return weight; }

    void add(const PART* other) {
        weight += other->weight;
        len    += other->len;
    }

    void takeMean(double overall_weight) {
        set_weight(get_weight()/overall_weight);
        arb_assert(is_valid());
        arb_assert(weight <= 1.0);
    }

    void invert();
    bool is_standardized() const;
    void standardize();

    void add_from(const PART *source);

    bool is_son_of(const PART *father) const;

    bool overlaps_with(const PART *other) const;
    bool disjunct_from(const PART *other) const { return !overlaps_with(other); }

    bool equals(const PART *other) const;
    bool differs(const PART *other) const { return !equals(other); }

    int index() const;
    unsigned key() const;

    int get_members() const { return members; }

    bool is_leaf_edge() const { int size = members; return size == 1 || size == (get_maxsize()-1); }
    int distance_to_tree_center() const { return abs(get_maxsize()/2 - members); }

    int insertionOrder_cmp(const PART *other) const;
    int topological_cmp(const PART *other) const;
    
#if defined(NTREE_DEBUG_FUNCTIONS)
    void print() const;
#endif
};

#else
#error CT_part.hxx included twice
#endif // CT_PART_HXX
