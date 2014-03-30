// =============================================================== //
//                                                                 //
//   File      : awtc_next_neighbours.hxx                          //
//   Purpose   : Search relatives via PT server                    //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AWTC_NEXT_NEIGHBOURS_HXX
#define AWTC_NEXT_NEIGHBOURS_HXX

#ifndef ARBDB_H
#include <arbdb.h>
#endif


class AWTC_FIND_FAMILY_MEMBER {
    // list is sorted either by 'matches' or 'rel_matches' (descending)
    // depending on 'rel_matches' paramater to findFamily() 
public:
    AWTC_FIND_FAMILY_MEMBER *next;

    char   *name;
    long    matches;
    double  rel_matches;

    AWTC_FIND_FAMILY_MEMBER();
    ~AWTC_FIND_FAMILY_MEMBER();
};

struct struct_aisc_com;

enum FF_complement {
    FF_FORWARD            = 1,
    FF_REVERSE            = 2,
    FF_REVERSE_COMPLEMENT = 4,
    FF_COMPLEMENT         = 8,
    
    // do NOT change the order here w/o fixing ../PROBE/PT_family.cxx@FF_complement_dep
};

class AWTC_FIND_FAMILY {
    struct_aisc_com *link;
    GBDATA          *gb_main;
    long             com;
    long             locs;
    
    void     delete_family_list();
    GB_ERROR init_communication(void);
    GB_ERROR open(char *servername);
    GB_ERROR retrieve_family(char *sequence, int oligo_len, int mismatches, bool fast_flag, bool rel_matches, FF_complement compl_mode, int max_results);
    void     close();

    // valid after calling retrieve_family():
    AWTC_FIND_FAMILY_MEMBER *family_list;

    bool hits_truncated;
    int  real_hits;

public:

    AWTC_FIND_FAMILY(GBDATA *gb_maini);
    ~AWTC_FIND_FAMILY();

    GB_ERROR findFamily(int server_id,char *sequence, int oligo_len, int mismatches, bool fast_flag, bool rel_matches, FF_complement compl_mode, int max_results);

    const AWTC_FIND_FAMILY_MEMBER *getFamilyList() const { return family_list; }
    bool hits_were_truncated() const { return hits_truncated; }
    int getRealHits() const { return real_hits; }

    void print();
};

// --------------------------------------------------------------------------------

#define AWAR_NN_BASE "next_neighbours/"

#define AWAR_NN_OLIGO_LEN   AWAR_NN_BASE "oligo_len"
#define AWAR_NN_MISMATCHES  AWAR_NN_BASE "mismatches"
#define AWAR_NN_FAST_MODE   AWAR_NN_BASE "fast_mode"
#define AWAR_NN_REL_MATCHES AWAR_NN_BASE "rel_matches"

class AW_root;
class AW_window;

void AWTC_create_common_next_neighbour_vars(AW_root *aw_root);
void AWTC_create_common_next_neighbour_fields(AW_window *aws);

#else
#error awtc_next_neighbours.hxx included twice
#endif // AWTC_NEXT_NEIGHBOURS_HXX