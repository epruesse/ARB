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
#include "CT_mem.hxx"

#include <arbdbt.h>

#if defined(DEBUG)
// #define DUMP_PART_INIT
#endif


static PELEM cutmask = 0; // this mask is used to zero unused bits from the last long
static int   longs   = 0; // number of longs per part
static int   plen    = 0; // number of bits per part

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

void part_init(const int len) {
    /*! Function to initialize the global variables above
     * @param len number of bits the part should content
     *
     * result: calculate cutmask, longs, plen
     */

    arb_assert(!cutmask && !longs && !plen); // forgot to call part_cleanup?
    
    int j      = len % BITS_PER_PELEM;
    if (!j) j += BITS_PER_PELEM;

    cutmask = 0;
    for (int i=0; i<j; i++) {
        cutmask <<= 1;
        cutmask |= 1;
    }
    longs = (((len + 7) / 8)+sizeof(PELEM)-1) / sizeof(PELEM);
    plen  = len;

#if defined(DEBUG)
    size_t possible = longs*BITS_PER_PELEM;
    arb_assert((possible-plen)<BITS_PER_PELEM); // longs is too big (wasted space)

#if defined(DUMP_PART_INIT)
    printf("leafs=%i\n", len);
    printf("cutmask='%s'\n", readable_cutmask(cutmask));
    printf("longs=%i (can hold %zu bits)\n", longs, possible);
    printf("plen=%i\n", plen);
#endif
#endif
}

void part_cleanup() {
    cutmask = 0;
    longs   = plen = 0;
}

void part_print(PART *p) {
    // ! Testfunction to print a part
    int i, j, k=0;
    PELEM l;

    for (i=0; i<longs; i++) {
        l = 1;
        for (j=0; k<plen && size_t(j)<sizeof(PELEM)*8; j++, k++) {
            if (p->p[i] & l)
                printf("1");
            else
                printf("0");
            l <<= 1;
        }
    }

    printf(":%.2f,%d%%: ", p->len, p->percent);
}

PART *part_new() {
    //! construct new part
    PART *p;

    p = (PART *) getmem(sizeof(PART));
    p->p = (PELEM *) getmem(longs * sizeof(PELEM));

    return p;
}

void part_free(PART *p) {
    //! destroy part
    free(p->p);
    free(p);
}


PART *part_root() {
    /*! build a partition that totally consists of 111111...1111 that is needed to
     * build the root of a specific ntree
     */
    int i;
    PART *p;
    p = part_new();
    for (i=0; i<longs; i++) {
        p->p[i] = ~p->p[i];
    }
    p->p[longs-1] &= cutmask;
    return p;
}


void part_setbit(PART *p, int pos) {
    /*! set the bit of the part 'p' at the position 'pos'
     */
    p->p[(pos / sizeof(PELEM) / 8)] |= (1 << (pos % (sizeof(PELEM)*8)));
}



int son(PART *son, PART *father) {
    /*! test if the part 'son' is possibly a son of the part 'father'.
     *
     * A father defined in this context as a part covers every bit of his son. needed in CT_ntree
     */
    int i;

    for (i=0; i<longs; i++) {
        if ((son->p[i] & father->p[i]) != son->p[i]) return 0;
    }
    return 1;
}


int brothers(PART *p1, PART *p2) {
    /*! test if two parts are brothers.
     *
     * "brothers" means that every bit in p1 is different from p2 and vice versa.
     * needed in CT_ntree
     */
    int i;

    for (i=0; i<longs; i++) {
        if (p1->p[i] & p2->p[i]) return 0;
    }
    return 1;
}


void part_invert(PART *p) {
    //! invert a part
    int i;

    for (i=0; i<longs; i++)
        p->p[i] = ~p->p[i];
    p->p[longs-1] &= cutmask;
}


void part_or(PART *source, PART *destination) {
    //! destination = source or destination
    for (int i=0; i<longs; i++) {
        destination->p[i] |= source->p[i];
    }
}


int parts_equal(PART *p1, PART *p2) {
    /*! compare two parts
     * @return 1 if equal, 0 otherwise
     */
    int i;

    for (i=0; i<longs; i++) {
        if (p1->p[i] != p2->p[i]) return 0;
    }

    return 1;
}


int part_key(PART *p) {
     //! calculate a hashkey from part 'p'
    int i;
    PELEM ph=0;
 
    for (i=0; i<longs; i++) {
         ph ^= p->p[i];
     }
    i = (int) ph;
    if (i<0) i *= -1;
 
    return i;
}

void part_setlen(PART *p, GBT_LEN len) {
    //! set the len of this edge (this part)
    p->len = len;
}

#if defined(DUMP_DROPS)
int part_size(PART *p) {
    //! count the number of leafs in partition
    int leafs = 0;
    for (int i = 0; i<longs; ++i) {
        PELEM e                    = p->p[i];
        // arb_assert((e&cutmask) == e); // @@@ fails sometimes - why?
        e                          = e&cutmask;
        for (size_t b = 0; b<(sizeof(e)*8); ++b) {
            if (e&1) leafs++;
            e = e>>1;
        }
    }
    return leafs;
}
#endif

void part_copy(PART *source, PART *destination) {
    //! copy source into destination
    for (int i=0; i<longs; i++) destination->p[i] = source->p[i];

    destination->len     = source->len;
    destination->percent = source->percent;
}


void part_standard(PART *p) {
    /*! standardize the partitions
     *
     * two parts are equal if one is just the inverted version of the other.
     * so the standard is defined that the version is the representant, whose first bit is equal 1
     */
    int i;

    if (p->p[0] & 1) return;

    for (i=0; i<longs; i++)
        p->p[i] = ~ p->p[i];
    p->p[longs-1] &= cutmask;
}


int calc_index(PART *p) {
    /*! calculate the first bit set in p,
     *
     * this is only useful if only one bit is set,
     * this is used toidentify leafs in a ntree
     *
     * ATTENTION: p must be != NULL
     */
    int i, pos=0;
    PELEM p_temp;

    for (i=0; i<longs; i++) {
        p_temp = p->p[i];
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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_part_init() {
    part_init(0);
    TEST_ASSERT_EQUAL(plen, 0);
    TEST_ASSERT_EQUAL(longs, 0);
    // cutmask doesnt matter
    part_cleanup();

    part_init(1);
    TEST_ASSERT_EQUAL(plen, 1);
    TEST_ASSERT_EQUAL(longs, 1);
    TEST_ASSERT_EQUAL(readable_cutmask(cutmask), "00000000000000000000000000000001");
    part_cleanup();

    part_init(31);
    TEST_ASSERT_EQUAL(plen, 31);
    TEST_ASSERT_EQUAL(longs, 1);
    TEST_ASSERT_EQUAL(readable_cutmask(cutmask), "01111111111111111111111111111111");
    part_cleanup();

    part_init(32);
    TEST_ASSERT_EQUAL(plen, 32);
    TEST_ASSERT_EQUAL(longs, 1);
    TEST_ASSERT_EQUAL(readable_cutmask(cutmask), "11111111111111111111111111111111");
    part_cleanup();

    part_init(33);
    TEST_ASSERT_EQUAL(plen, 33);
    TEST_ASSERT_EQUAL(longs, 2);
    TEST_ASSERT_EQUAL(readable_cutmask(cutmask), "00000000000000000000000000000001");
    part_cleanup();

    part_init(95);
    TEST_ASSERT_EQUAL(plen, 95);
    TEST_ASSERT_EQUAL(longs, 3);
    TEST_ASSERT_EQUAL(readable_cutmask(cutmask), "01111111111111111111111111111111");
    part_cleanup();
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
