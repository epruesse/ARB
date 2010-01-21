/* This module is desined to organize the data structure partitions
   partitions represent the edges of a tree */
/* the partitions are implemented as an array of longs */
/* Each leaf in a GBT-Tree is represented as one Bit in the Partition */

#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "CT_mem.hxx"
#include "CT_part.hxx"

PELEM   cutmask;        /* this mask is necessary to cut the not
                           needed bits from the last long       */
int     longs = 0,      /* number of longs per part             */
    plen = 0;       /* number of bits per part              */


/** Function to initialize the variables above
    @PARAMETER  len     number of bits the part should content
    result:         calculate cutmask, longs, plen */
void part_init(int len)
{
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


/** Testfunction to print a part
    @PARAMETER  p   pointer to the part */
void part_print(PART *p)
{
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


/** construct new part */
PART *part_new(void)
{
    PART *p;

    p = (PART *) getmem(sizeof(PART));
    p->p = (PELEM *) getmem(longs * sizeof(PELEM));

    return p;
}


/** destruct part
    @PARAMETER  p   partition */
void part_free(PART *p)
{
    free(p->p);
    free(p);
}


/** build a partion that totally consists of 111111...1111 that is needed to
    build the root of a specific ntree */
PART *part_root(void)
{
    int i;
    PART *p;
    p = part_new();
    for (i=0; i<longs; i++) {
        p->p[i] = ~p->p[i];
    }
    p->p[longs-1] &= cutmask;
    return p;
}


/** set the bit of the part p at the position pos
    @PARAMETER  p   partion
    pos position */
void part_setbit(PART *p, int pos)
{
    p->p[(pos / sizeof(PELEM) / 8)] |= (1 << (pos % (sizeof(PELEM)*8)));
}



/* test if the part son is possibly a son of the part father, a father defined
   in this context as a part covers every bit of his son. needed in CT_ntree
   @PARAMETER  son part
   father  part */
int son(PART *son, PART *father)
{
    int i;

    for (i=0; i<longs; i++) {
        if ((son->p[i] & father->p[i]) != son->p[i]) return 0;
    }
    return 1;
}


/** test if two parts are brothers, brothers mean that every bit in p1 is
    different from p2 and vice versa. needed in CT_ntree
    @PARAMETER  p1  part
    p2  part */
int brothers(PART *p1, PART *p2)
{
    int i;

    for (i=0; i<longs; i++) {
        if (p1->p[i] & p2->p[i]) return 0;
    }
    return 1;
}


/** invert a part
    @PARAMETER  p   part */
void part_invert(PART *p)
{
    int i;

    for (i=0; i<longs; i++)
        p->p[i] = ~p->p[i];
    p->p[longs-1] &= cutmask;
}


/** d  = s or d
    @PARMETER   s   source
    d   destination */
void part_or(PART *s, PART *d)
{
    int i;

    for (i=0; i<longs; i++) {
        d->p[i] |= s->p[i];
    }
}


/** compare two parts p1 and p2
    result:     1, if p1 equal p2
    0, else */
int part_cmp(PART *p1, PART *p2)
{
    int i;

    for (i=0; i<longs; i++) {
        if (p1->p[i] != p2->p[i]) return 0;
    }

    return 1;
}


/** calculate a hashkey from p, needed in hash */
int part_key(PART *p)
{
    int i;
    PELEM ph=0;

    for (i=0; i<longs; i++) {
        ph ^= p->p[i];
    }
    i = (int) ph;
    if (i<0) i *= -1;

    return i;
}


/** set the len of this edge (this part) */
void part_setlen(PART *p, GBT_LEN len)
{
    p->len = len;
}


/** set the percentaged appearance of this part in "entrytrees" */
void part_setperc(PART *p, int perc)
{
    p->percent = perc;
}


/** add perc on percent from p */
void part_addperc(PART *p, int perc)
{
    p->percent += perc;
}


/** copy part s in part d
    @PARAMETER  s   source
    d   destination */
void part_copy(PART *s, PART *d)
{
    int i;

    for (i=0; i<longs; i++)
        d->p[i] = s->p[i];
    d->len = s->len;
    d->percent = s->percent;
}


/** standardize the partitions, two parts are equal if one is just the inverted
    version of the other so the standard is defined that the version is the
    representant, whose first bit is equal 1 */
void part_standard(PART *p)
{
    int i;

    if (p->p[0] & 1) return;

    for (i=0; i<longs; i++)
        p->p[i] = ~ p->p[i];
    p->p[longs-1] &= cutmask;
}


/** calculate the first bit set in p, this is only useful if only one bit is set,
    this is used toidentify leafs in a ntree
    attention:  p must be != NULL !!! */
int calc_index(PART *p)
{
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






















