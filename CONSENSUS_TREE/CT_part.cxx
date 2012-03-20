// ============================================================= //
//                                                               //
//   File      : CT_part.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

/* This module is designed to organize the data structure partitions
   partitions represent the edges of a tree */
// the partitions are implemented as an array of longs
// Each leaf in a GBT-Tree is represented as one Bit in the Partition

#include "CT_part.hxx"
#include <arbdbt.h>

#if defined(DEBUG)
#endif

#define BITS_PER_PELEM (sizeof(PELEM)*8)

#if defined(DUMP_PART_INIT) || defined(UNIT_TESTS)
static const char *readable_cutmask(PELEM mask) {
    static char readable[BITS_PER_PELEM+1];
    memset(readable, '0', BITS_PER_PELEM);
    readable[BITS_PER_PELEM]        = 0;

    for (int b = BITS_PER_PELEM-1; b >= 0; --b) {
        if (mask&1) readable[b] = '1';
        mask = mask>>1;
    }
    return readable;
}
#endif

PartitionSize::PartitionSize(const int len)
    : cutmask(0),
      longs((((len + 7) / 8)+sizeof(PELEM)-1) / sizeof(PELEM)),
      bits(len),
      id(0)
{
    /*! Function to initialize the global variables above
     * @param len number of bits the part should content
     *
     * result: calculate cutmask, longs, plen
     */

    int j      = len % BITS_PER_PELEM;
    if (!j) j += BITS_PER_PELEM;

    for (int i=0; i<j; i++) {
        cutmask <<= 1;
        cutmask |= 1;
    }

#if defined(DEBUG)
    size_t possible = longs*BITS_PER_PELEM;
    arb_assert((possible-bits)<BITS_PER_PELEM); // longs is too big (wasted space)

#if defined(DUMP_PART_INIT)
    printf("leafs=%i\n", len);
    printf("cutmask='%s'\n", readable_cutmask(cutmask));
    printf("longs=%i (can hold %zu bits)\n", longs, possible);
    printf("bits=%i\n", bits);
#endif
#endif
}

#if defined(NTREE_DEBUG_FUNCTIONS)
void PART::print() const {
    // ! Testfunction to print a part
    int i, j, k=0;
    PELEM l;

    int longs = get_longs();
    int plen = info->get_bits();
    for (i=0; i<longs; i++) {
        l = 1;
        for (j=0; k<plen && size_t(j)<sizeof(PELEM)*8; j++, k++) {
            if (p[i] & l)
                printf("1");
            else
                printf("0");
            l <<= 1;
        }
    }

    printf(":%.2f,%d%%:  dist2center=%i\n", len, percent, distance_to_tree_center());
}
#endif

PART *PartitionSize::create_root() const {
    /*! build a partition that totally consists of 111111...1111 that is needed to
     * build the root of a specific ntree
     */

    PART *p = new PART(this, 0);
    p->invert();
    arb_assert(p->is_valid());
    return p;
}


bool PART::is_son_of(const PART *father) const {
    /*! test if the part 'son' is possibly a son of the part 'father'.
     *
     * A father defined in this context as a part covers every bit of his son. needed in CT_ntree
     */

    arb_assert(is_valid());
    arb_assert(father->is_valid());
    
    bool is_equal = true;
    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        PELEM s = p[i];
        PELEM f = father->p[i];

        if ((s&f) != s) return false; // father has not all son bits set
        if (s != f) is_equal = false;
    }

    arb_assert(!is_equal); // if is_equal, father and son are identical (which is wrong);
                           // e.g. happens when PartRegistry stores multiple instances of the same part
    return true;
}


bool PART::overlaps_with(const PART *other) const {
    /*! test if two parts overlap (i.e. share common bits)
     */

    arb_assert(is_valid());
    arb_assert(other->is_valid());

    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        if (p[i] & other->p[i]) return true;
    }
    return false;
}

void PART::invert() {
    //! invert a part
    //
    // Each edge in a tree connects two subtrees.
    // These subtrees are represented by inverse partitions

    arb_assert(is_valid());
    
    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        p[i] = ~p[i];
    }
    p[longs-1] &= get_cutmask();

    members = get_maxsize()-members;

    arb_assert(is_valid());
}

void PART::add_from(const PART *source) {
    //! destination = source or destination
    arb_assert(source->is_valid());
    arb_assert(is_valid());

    bool distinct = disjunct_from(source);

    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        p[i] |= source->p[i];
    }

    if (distinct) {
        members += source->members;
    }
    else {
        members = count_members();
    }

    arb_assert(is_valid());
}


bool PART::equals(const PART *other) const {
    /*! return true if p1 and p2 are equal
     */
    arb_assert(is_valid());
    arb_assert(other->is_valid());

    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        if (p[i] != other->p[i]) return false;
    }
    return true;
}


unsigned PART::key() const {
    //! calculate a hashkey from part
    arb_assert(is_valid());

    PELEM ph = 0;
    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        ph ^= p[i];
    }
 
    return ph;
}

int PART::count_members() const { 
    //! count the number of leafs in partition
    int leafs = 0;
    int longs = get_longs();
    for (int i = 0; i<longs; ++i) {
        PELEM e = p[i];

        if (i == (longs-1)) e = e&get_cutmask();

        for (size_t b = 0; b<(sizeof(e)*8); ++b) {
            if (e&1) leafs++;
            e = e>>1;
        }
    }
    return leafs;
}

bool PART::is_standardized() const { // @@@ inline
    return p[0] & 1;
}

void PART::standardize() { // @@@ inline or elim
    /*! standardize the partition
     *
     * two parts are equal if one is just the inverted version of the other.
     * so the standard is defined that the version is the representant, whose first bit is equal 1
     */
    arb_assert(is_valid());
    if (!is_standardized()) invert();
    arb_assert(is_valid());
}

int PART::index() const {
    /*! calculate the first bit set in p,
     *
     * this is only useful if only one bit is set,
     * this is used to identify leafs in a ntree
     *
     * ATTENTION: p must be != NULL
     */
    arb_assert(is_valid());
    arb_assert(is_leaf_edge());

    int pos   = 0;
    int longs = get_longs();
    for (int i=0; i<longs; i++) {
        PELEM p_temp = p[i];
        pos = i * sizeof(PELEM) * 8;
        if (p_temp) {
            for (; p_temp; p_temp >>= 1, pos++) {
                ;
            }
            break;
        }
    }
    return pos-1;
}

int PART::insertionOrder_cmp(const PART *other) const {
    // defines order in which edges will be inserted into the consensus tree

    if (this == other) return 0;

    int cmp = is_leaf_edge() - other->is_leaf_edge();

    if (!cmp) {
        cmp = other->percent - percent;

        if (!cmp) {
            int centerdist1 = distance_to_tree_center();
            int centerdist2 = other->distance_to_tree_center();

            cmp = centerdist1-centerdist2; // insert central edges first

            if (!cmp) {
                double fcmp = get_len() - other->get_len(); // insert bigger len first
                cmp = fcmp<0 ? -1 : (fcmp>0 ? 1 : 0);

                if (!cmp) {
                    cmp = id - other->id; // strict by definition
                    arb_assert(cmp);
                }
            }
        }
    }

    return cmp;
}
inline int PELEM_cmp(const PELEM& p1, const PELEM& p2) {
    return p1<p2 ? -1 : (p1>p2 ? 1 : 0);
}

int PART::topological_cmp(const PART *other) const {
    // define a strict order on topologies defined by edges
    
    if (this == other) return 0;

    arb_assert(is_standardized());
    arb_assert(other->is_standardized());

    int cmp = members - other->members;
    if (!cmp) {
        int longs = get_longs();
        for (int i = 0; !cmp && i<longs; ++i) {
            cmp = PELEM_cmp(p[i], other->p[i]);
        }
    }

    arb_assert(contradicted(cmp, equals(other)));

    return cmp;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_PartRegistry() {
    {
        PartitionSize reg(0);
        TEST_ASSERT_EQUAL(reg.get_bits(), 0);
        TEST_ASSERT_EQUAL(reg.get_longs(), 0);
        // cutmask doesnt matter
    }

    {
        PartitionSize reg(1);
        TEST_ASSERT_EQUAL(reg.get_bits(), 1);
        TEST_ASSERT_EQUAL(reg.get_longs(), 1);
        TEST_ASSERT_EQUAL(readable_cutmask(reg.get_cutmask()), "00000000000000000000000000000001");
    }

    {
        PartitionSize reg(31);
        TEST_ASSERT_EQUAL(reg.get_bits(), 31);
        TEST_ASSERT_EQUAL(reg.get_longs(), 1);
        TEST_ASSERT_EQUAL(readable_cutmask(reg.get_cutmask()), "01111111111111111111111111111111");
    }

    {
        PartitionSize reg(32);
        TEST_ASSERT_EQUAL(reg.get_bits(), 32);
        TEST_ASSERT_EQUAL(reg.get_longs(), 1);
        TEST_ASSERT_EQUAL(readable_cutmask(reg.get_cutmask()), "11111111111111111111111111111111");
    }

    {
        PartitionSize reg(33);
        TEST_ASSERT_EQUAL(reg.get_bits(), 33);
        TEST_ASSERT_EQUAL(reg.get_longs(), 2);
        TEST_ASSERT_EQUAL(readable_cutmask(reg.get_cutmask()), "00000000000000000000000000000001");
    }

    {
        PartitionSize reg(95);
        TEST_ASSERT_EQUAL(reg.get_bits(), 95);
        TEST_ASSERT_EQUAL(reg.get_longs(), 3);
        TEST_ASSERT_EQUAL(readable_cutmask(reg.get_cutmask()), "01111111111111111111111111111111");
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


