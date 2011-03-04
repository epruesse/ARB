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
/* the partitions are implemented as an array of longs */
/* Each leaf in a GBT-Tree is represented as one Bit in the Partition */

#include "CT_part.hxx"
#include "CT_mem.hxx"

#include <arbdbt.h>

PELEM cutmask;                                      // this mask is necessary to cut the not needed bits from the last long
int   longs = 0;                                    // number of longs per part
int   plen  = 0;                                    // number of bits per part


void part_init(int len) {
    /*! Function to initialize the variables above
     * @param len number of bits the part should content
     *
     * result: calculate cutmask, longs, plen
     */
    int i, j;

    /* cutmask is necessary to cut unused bits */
    j = 8 * sizeof(PELEM);
    j = len % j;
    if (!j) j += 8 * sizeof(PELEM);
    cutmask = 0;
    for (i=0; i<j; i++) {
        cutmask <<= 1;
        cutmask |= 1;
    }
    longs = (((len + 7) / 8)+sizeof(PELEM)-1) / sizeof(PELEM);
    plen = len;
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
    /*! build a partion that totally consists of 111111...1111 that is needed to
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


int part_cmp(PART *p1, PART *p2) {
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
    /*! set the len of this edge (this part) */
    p->len = len;
}


void part_setperc(PART *p, int perc) {
    /*! set the percentaged appearance of this part in "entrytrees" */
    p->percent = perc;
}


void part_addperc(PART *p, int perc) {
    /*! add 'perc' on percent of p */
    p->percent += perc;
}

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

