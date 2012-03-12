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

struct HNODE {
    PART  *part;
    HNODE *next;
};

#define HASH_MAX 123 // @@@ increase when tree generation is stable

void hash_init();
void hash_cleanup();

void  hash_settreecount(int tree_count);
PART *hash_getpart();
void  hash_insert(PART*& part, int weight);
void  build_sorted_list();

#else
#error CT_hash.hxx included twice
#endif // CT_HASH_HXX
