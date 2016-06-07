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
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef PT_GLOBAL_DEFS_H
#include <PT_global_defs.h>
#endif
#ifndef POS_RANGE_H
#include <pos_range.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif

#define ff_assert(bed) arb_assert(bed)

class FamilyList : virtual Noncopyable {
    // list is sorted either by 'matches' or 'rel_matches' (descending)
    // depending on 'rel_matches' parameter to PT_FamilyFinder::searchFamily()
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

class FamilyFinder : virtual Noncopyable {
    bool                 rel_matches;
    RelativeScoreScaling scaling;

protected:
    FamilyList *family_list;

    bool hits_truncated;

#if defined(WARN_TODO)
#warning change real_hits back to int when aisc_get() has been made failsafe
#endif 
    long real_hits;

    PosRange range;

public:
    FamilyFinder(bool rel_matches_, RelativeScoreScaling scaling_);
    virtual ~FamilyFinder();

    void restrict_2_region(const PosRange& range_) {
        // Restrict oligo search to 'range_'
        // Only oligos which are completely inside that region are used for calculating relationship.
        // Has to be called before calling searchFamily.
        range = range_;
    }

    void unrestrict() { range = PosRange(-1, -1); }
    const PosRange& get_TargetRange() const { return range; }

    virtual GB_ERROR searchFamily(const char *sequence, FF_complement compl_mode, int max_results, double min_score) = 0;

    const FamilyList *getFamilyList() const { return family_list; }
    void delete_family_list();

    bool hits_were_truncated() const { return hits_truncated; }
    bool uses_rel_matches() const { return rel_matches; }
    RelativeScoreScaling get_scaling() const { return scaling; }
    int getRealHits() const { return real_hits; }
};

class PT_FamilyFinder : public FamilyFinder { // derived from a Noncopyable
    GBDATA *gb_main;
    int     server_id;
    int     oligo_len;
    int     mismatches;
    bool    fast_flag;

    struct PT_FF_comImpl *ci;

    GB_ERROR init_communication();
    GB_ERROR open(const char *servername);
    GB_ERROR retrieve_family(const char *sequence, FF_complement compl_mode, int max_results, double min_score) __ATTR__USERESULT;
    void     close();

public:

    PT_FamilyFinder(GBDATA *gb_main_, int server_id_, int oligo_len_, int mismatches_, bool fast_flag_, bool rel_matches_, RelativeScoreScaling scaling_);
    ~PT_FamilyFinder() OVERRIDE;

    GB_ERROR searchFamily(const char *sequence, FF_complement compl_mode, int max_results, double min_score) OVERRIDE __ATTR__USERESULT;

    const char *results2string();
};

// --------------------------------------------------------------------------------

#define AWAR_NN_BASE "next_neighbours/"

#define AWAR_NN_OLIGO_LEN   AWAR_NN_BASE "oligo_len"
#define AWAR_NN_MISMATCHES  AWAR_NN_BASE "mismatches"
#define AWAR_NN_FAST_MODE   AWAR_NN_BASE "fast_mode"
#define AWAR_NN_REL_MATCHES AWAR_NN_BASE "rel_matches"
#define AWAR_NN_REL_SCALING AWAR_NN_BASE "scaling"

class AW_root;
class AW_window;

void AWTC_create_common_next_neighbour_vars(AW_root *aw_root, const RootCallback& awar_changed_cb);
void AWTC_create_common_next_neighbour_fields(AW_window *aws, int scaler_length);

#else
#error awtc_next_neighbours.hxx included twice
#endif // AWTC_NEXT_NEIGHBOURS_HXX
