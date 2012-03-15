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

class PartRegistry : virtual Noncopyable {
    int     hash_size;
    int     max_part_percent;
    HNODE **Hashlist;
    HNODE  *Sortedlist;

    inline void track_max_part_percent(int pc) { if (pc>max_part_percent) max_part_percent = pc; }

public:
    PartRegistry();
    ~PartRegistry();

    PART *get_part(int source_trees);
    void  insert(PART*& part, int weight);

    void  build_sorted_list();
};

#else
#error CT_hash.hxx included twice
#endif // CT_HASH_HXX
