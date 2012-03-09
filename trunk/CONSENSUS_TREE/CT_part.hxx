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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef CT_DEF_HXX
#include "CT_def.hxx"
#endif

typedef unsigned int PELEM;

struct PART {
    PELEM   *p;
    GBT_LEN  len;               // Length between two knots
    int      percent;           // Count how often this partition appears
    char    *source;            // From which tree comes the partition
};

void part_init(int len);
void part_cleanup();

PART *part_new();
PART *part_root();
void  part_setbit(PART *p, int pos);

bool is_son_of(const PART *son, const PART *father); // @@@ bool result
bool are_brothers(const PART *p1, const PART *p2);
bool parts_equal(const PART *p1, const PART *p2); // @@@ 

void part_invert(PART *p);
void part_or(const PART *s, PART *d);
void part_copy(const PART *s, PART *d);
void part_standard(PART *p);
int  calc_index(const PART *p);
void part_free(PART *p);
int  part_key(const PART *p);
void part_setlen(PART *p, GBT_LEN len);

#if defined(NTREE_DEBUG_FUNCTIONS)
void  part_print(const PART *p);
#endif
#if defined(DUMP_DROPS)
int part_size(const PART *p);
#endif

#else
#error CT_part.hxx included twice
#endif // CT_PART_HXX
