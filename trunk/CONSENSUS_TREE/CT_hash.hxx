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

typedef struct hnode {
    PART         *part;
    struct hnode *next;
} HNODE;

extern int      Tree_count;
extern GB_HASH *Name_hash;

#define HASH_MAX 123

void  hash_print();
void  hash_init();
void  hash_settreecount(int tree_count);
void  hash_free();
PART *hash_getpart();
void  hash_insert(PART *part, int weight);
void  build_sorted_list();

#else
#error CT_hash.hxx included twice
#endif // CT_HASH_HXX
