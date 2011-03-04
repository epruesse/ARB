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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

class FamilyList {
    // list is sorted either by 'matches' or 'rel_matches' (descending)
    // depending on 'rel_matches' paramater to PT_FamilyFinder::searchFamily()
public:
    FamilyList *next;

    char   *name;
    long    matches;
    double  rel_matches;

    FamilyList *insertSortedBy_matches(FamilyList *other);
    FamilyList *insertSortedBy_rel_matches(FamilyList *other);

    FamilyList();
    ~FamilyList();
};

struct aisc_com;

enum FF_complement {
    FF_FORWARD            = 1,
    FF_REVERSE            = 2,
    FF_REVERSE_COMPLEMENT = 4,
    FF_COMPLEMENT         = 8,

    // do NOT change the order here w/o fixing ../PROBE/PT_family.cxx@FF_complement_dep
};

class FamilyFinder {
    bool rel_matches;
    
protected:
    FamilyList *family_list;

    bool hits_truncated;
#if defined(DEVEL_RALF)
#warning change real_hits back to int when aisc_get() has been made failsafe
#endif // DEVEL_RALF
    long real_hits;

public:
    FamilyFinder(bool rel_matches_);
    virtual ~FamilyFinder();

    virtual GB_ERROR searchFamily(const char *sequence, FF_complement compl_mode, int max_results) = 0;

    const FamilyList *getFamilyList() const { return family_list; }
    void delete_family_list();
    
    bool hits_were_truncated() const { return hits_truncated; }
    bool uses_rel_matches() const { return rel_matches; }
    int getRealHits() const { return real_hits; }
};

class PT_FamilyFinder : public FamilyFinder {
    GBDATA *gb_main;
    int     server_id;
    int     oligo_len;
    int     mismatches;
    bool    fast_flag;

    aisc_com *link;
    long      com;
    long      locs;

    GB_ERROR init_communication();
    GB_ERROR open(const char *servername);
    GB_ERROR retrieve_family(const char *sequence, FF_complement compl_mode, int max_results) __ATTR__USERESULT;
    void     close();

public:

    PT_FamilyFinder(GBDATA *gb_main_, int server_id_, int oligo_len_, int mismatches_, bool fast_flag_, bool rel_matches_);
    ~PT_FamilyFinder();

    GB_ERROR searchFamily(const char *sequence, FF_complement compl_mode, int max_results) __ATTR__USERESULT;

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
