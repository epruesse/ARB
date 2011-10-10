/*!
 * \file ptpan.h
 *
 * \date ??.??.??
 * \author Chris Hodges
 * \author Jörg Böhnel
 * \author Tilo Eissler
 */

#ifndef ARB_64
    // TODO currently the 32bit-mode is not tested!
    #error 32-bit mode currently untested
#endif

#ifndef PTPAN_H_
#define PTPAN_H_

#include <stdlib.h>

#include <boost/unordered_map.hpp>
#include <vector>
#include <map>
#include <set>

#include <boost/icl/interval_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "hash_key.h"

#include "dlist.h"
#include "bin_tree.h"
#include "huffman.h"

//
// Now first some global defines
//

/* double delta leaf compression */
// TODO #define DOUBLEDELTALEAVES
//#define DOUBLEDELTAOPTIONAL
/* size of hash table for duplicates detection during query */
#define HASH_SIZE_SEED 3571 // OLD:12800009
/* error decrease for mismatches in probe design */
#define DECR_TEMP_CNT 20 // max number of nongroup hits with decreasing temperature
#define PROBE_MISM_DEC 0.2

#define FILESTRUCTVERSION 0x0109 /* version<<8 and revision */

#define COMPRESSED_SEQUENCE_SHORTCUT_BASE 65536

#define QHF_UNSAFE   0x0001   /* this hit must be verified */
#define QHF_REVERSED 0x0002   /* this hit was found in the reversed sequence */
#define QHF_ISVALID  0x8000   /* it's a valid hit */

// Allowed error when comparing floats
#define EPSILON 0.000001

//
// Some global structures ...
//

/* entries shortcut compressed sequence offsets */
struct PTPanComprSeqShortcuts {
    ULONG pcs_AbsPosition; /* Absolute Position, including . and - */
    ULONG pcs_BitPosition; /* Corresponding bit position in compressed sequence */
    ULONG pcs_RelPosition; /* Relative Position, without . and - */
};

/*!
 * \brief Position pair
 */
struct PTPanPositionPair {
    ULONG pop_start; /* start position inside sequence */
    ULONG pop_end; /* end position inside sequence */
    bool complement; /* if = false, it's on the positive strand, if true it's on the reverse one */

    /*!
     * \brief Create an array of PTPanPositionPair
     *
     * \warning uses calloc, so user must free it!
     */
    static struct PTPanPositionPair* createPtpanPositionPair(ULONG count) {
        return (struct PTPanPositionPair*) calloc(count,
                sizeof(struct PTPanPositionPair));
    }

    bool inRange(ULONG position) {
        return position >= pop_start && position <= pop_end;
    }

    bool inSameRange(ULONG position, ULONG offset) const {
        return (pop_start <= position) && (position + offset <= pop_end);
    }
};

/*!
 * \brief Feature (e.g. gene ...)
 */
struct PTPanFeature {
    STRPTR pf_name; /* name of the feature */
    UWORD pf_num_pos; /* number of positions (< 1 means splitted feature) */
    PTPanPositionPair *pf_pos; /* array of pointers with positions */

    /*!
     * \brief Create a PTPanFeature
     *
     * \warning uses calloc, so user must free it!
     */
    static struct PTPanFeature* createPtpanFeature() {
        return (struct PTPanFeature*) calloc(1, sizeof(struct PTPanFeature));
    }

    /*!
     * \brief Free PTPanFeature memory
     *
     * \param feature
     */
    static void freePtpanFeature(PTPanFeature* feature) {
        if (feature) {
            if (feature->pf_pos) {
                free(feature->pf_pos);
                feature->pf_pos = NULL;
            }
            if (feature->pf_name) {
                free(feature->pf_name);
                feature->pf_name = NULL;
            }
            free(feature);
            feature = NULL;
        }
    }

    /*!
     * \brief Check whether given position is in range of this Feature
     *
     * \param position
     *
     * \return bool
     */
    bool inRange(ULONG position) const {
        for (UWORD i = 0; i < pf_num_pos; i++) {
            if (pf_pos[i].inRange(position)) {
                return true;
            }
        }
        return false;
    }

    /*!
     * \brief Check whether given position and position+offset are in the same range within this Feature
     *
     * \param position
     * \param offset
     *
     * \return bool
     */
    bool inSameRange(ULONG position, ULONG offset) const {
        for (UWORD i = 0; i < pf_num_pos; i++) {
            if (pf_pos[i].inSameRange(position, offset)) {
                return true;
            }
        }
        return false;
    }

    ULONG getPositionInFeature(ULONG position) const {
        for (UWORD i = 0; i < pf_num_pos; i++) {
            if (pf_pos[i].inRange(position)) {
                return position - pf_pos[i].pop_start;
            }
        }
        return 0;
    }

    std::pair<ULONG, LONG> getBorderDistance(ULONG position,
            ULONG offset) const {
        for (UWORD i = 0; i < pf_num_pos; i++) {
            //if (pf_pos[i].pop_start <= position) {
            if (pf_pos[i].inSameRange(position, offset)) {
                return std::make_pair(position - pf_pos[i].pop_start,
                        pf_pos[i].pop_end - (position + offset));
            }
        }
        return std::make_pair(0, 0);
    }
};

/*!
 * \brief Feature Container
 */
class PTPanFeatureContainer {
public:
    PTPanFeatureContainer();
    PTPanFeatureContainer(bool simple_mode);
    virtual ~PTPanFeatureContainer();

    /*!
     * \brief This is special stuff necessary to subdue the
     *        ARBDB memory beast ;-)
     *        Normally you want to rely on the constructors!
     */
    static PTPanFeatureContainer* createSpecialMode(ULONG init_count = 500) {
        PTPanFeatureContainer* container =
                (struct PTPanFeatureContainer*) calloc(1,
                        sizeof(PTPanFeatureContainer));
        container->m_feat_count = 0;
        container->m_max_feat_count = init_count;
        container->m_feat_array = (struct PTPanFeature**) malloc(
                init_count * sizeof(PTPanFeature*));
        container->m_normal_mode = false;
        container->m_simple_mode = false;
        return container;
    }

    /*!
     * \brief This is special stuff necessary to subdue the
     *        ARBDB memory beast ;-)
     *        Normally you want to rely on the destructor!
     */
    static void freeSpecialMode(PTPanFeatureContainer* container) {
        if (container) {
            if (!container->m_normal_mode) {
                for (ULONG i = 0; i < container->m_feat_count; i++) {
                    if (container->m_feat_array[i]) {
                        PTPanFeature::freePtpanFeature(
                                container->m_feat_array[i]);
                        container->m_feat_array[i] = NULL;
                    }
                }
                if (container->m_feat_array) {
                    free(container->m_feat_array);
                    container->m_feat_array = NULL;
                }
                free(container);
                container = NULL;
            }
        }
    }

    bool simpleMode() const;
    bool normalMode() const;

    void clear();
    bool empty() const;
    ULONG size() const;

    const PTPanFeature * getFeatureByNumber(ULONG number) const;

    ULONG byteSize() const;

    void add(PTPanFeature *feature);

    const std::vector<PTPanFeature*>
    getFeature(ULONG position) const;
    bool hasFeature(CONST_STRPTR name) const;
    const PTPanFeature* getFeature(CONST_STRPTR name) const;

private:
    PTPanFeatureContainer(const PTPanFeatureContainer& /*pfc*/) {
        // do nothing, only avoid public copy constructor
    }
    PTPanFeatureContainer& operator=(const PTPanFeatureContainer& /*fc*/) {
        // do nothing, only avoid public assignment operator
        return *this;
    }

    typedef std::vector<PTPanFeature*>::const_iterator PTPanFeatureIterator;

    std::vector<PTPanFeature*>* m_features;
    // for special mode only:
    PTPanFeature** m_feat_array;
    ULONG m_feat_count;
    ULONG m_max_feat_count;

    typedef boost::unordered_map<const char *, PTPanFeature*,
            hash_key<const char *>, equal_str> PtpanFeatureMap;
    PtpanFeatureMap* m_feature_map;

    typedef std::set<ULONG> MemberSetT;
    typedef boost::icl::interval_map<ULONG, MemberSetT> FeaturePositionT;
    FeaturePositionT* m_feature_pos_map;

    bool m_normal_mode;
    bool m_simple_mode;

};

/*!
 * \brief information about an entry
 * */
struct PTPanEntry {
    struct Node pe_Node; /* linkage */
    ULONG pe_Num; /* entry number for map */

    STRPTR pe_Name; /* short name of entry */
    STRPTR pe_FullName; /* long name of entry */

    ULONG pe_SeqDataSize; /* length of original sequence data */
    ULONG pe_RawDataSize; /* length of alignment data (filtered) */
    ULLONG pe_AbsOffset; /* absolute offset in the resulting raw image */

    UBYTE *pe_SeqDataCompressed; // compressed Seq. Data (with '.' and '-')
    ULONG pe_SeqDataCompressedSize; // size in bits (includes end flag 111)

    ULONG pe_CompressedShortcutsCount;
    struct PTPanComprSeqShortcuts *pe_CompressedShortcuts;

    PTPanFeatureContainer *pe_FeatureContainer;
};

/* Reference Sequence Entry, e.g. Ecoli for 16s RNA*/
struct PTPanReferenceEntry {
    STRPTR pre_ReferenceSeq; /* e.g. Ecoli sequence data */
    ULONG pre_ReferenceSeqSize; /* length of reference sequence */
    ULONG *pre_ReferenceBaseTable; /* quick table lookup for reference base position */
};

/* mismatch matrix and edit distance weights */
struct MismatchWeights {
    float *mw_Replace; /* costs for replacing one char by the other [ALPHASIZE * ALPHASIZE] */
    float *mw_Insert; /* cost for inserting one char [ALPHASIZE] */
    float *mw_Delete; /* cost for deleting one char [ALPHASIZE] */
    ULONG mw_alphabet_size;

    /*!
     * \brief init MismatchWeights (no values, all 0)
     *
     * \warning user must free it (as it is calloc'd)
     */
    static struct MismatchWeights * createMismatchWeightMatrix(
            ULONG alphabet_size) {
        struct MismatchWeights* new_mw = (struct MismatchWeights *) calloc(
                sizeof(MismatchWeights), 1);
        new_mw->mw_Replace = (float*) calloc(
                alphabet_size * alphabet_size * sizeof(float), 1);
        new_mw->mw_Insert = (float*) calloc(alphabet_size * sizeof(float), 1);
        new_mw->mw_Delete = (float*) calloc(alphabet_size * sizeof(float), 1);
        new_mw->mw_alphabet_size = alphabet_size;
        return new_mw;
    }

    /*!
     * \brief Free MismatchWeights
     */
    static void freeMismatchWeightMatrix(struct MismatchWeights *mw) {
        if (mw) {
            free(mw->mw_Replace);
            free(mw->mw_Insert);
            free(mw->mw_Delete);
            free(mw);
        }
    }

    /*!
     * \brief Clone MismatchWeights
     */
    static struct MismatchWeights* cloneMismatchWeightMatrix(
            struct MismatchWeights *mw) {
        if (mw) {
            struct MismatchWeights *new_mw = createMismatchWeightMatrix(
                    mw->mw_alphabet_size);
            ULONG cnt;
            for (cnt = 0;
                    cnt < new_mw->mw_alphabet_size * new_mw->mw_alphabet_size;
                    cnt++) {
                new_mw->mw_Replace[cnt] = mw->mw_Replace[cnt];
            }
            for (cnt = 0; cnt < new_mw->mw_alphabet_size; cnt++) {
                new_mw->mw_Insert[cnt] = mw->mw_Insert[cnt];
            }
            for (cnt = 0; cnt < new_mw->mw_alphabet_size; cnt++) {
                new_mw->mw_Delete[cnt] = mw->mw_Delete[cnt];
            }
            return new_mw;
        }
        return NULL;
    }
};

/* index partition structure (memory) */
struct PTPanPartition {
    ULONG pp_ID; /* partition id, e.g. used for filename */
    STRPTR pp_PartitionName; /* file name of partition */

    ULONG pp_Prefix; /* compressed prefix code */
    ULONG pp_PrefixLen; /* length of prefix code */
    STRPTR pp_PrefixSeq; /* prefix sequence */
    ULONG pp_Size; /* number of nodes for this prefix */

    ULONG pp_TreePruneDepth; /* at which depth should the tree be pruned? */
    ULONG pp_TreePruneLength; /* length at which tree should be pruned? */

    ULONG pp_LongDictSize; /* allocation size */
    UWORD pp_LongRelPtrBits; /* bits to use to reference a long edge offset */
    ULONG *pp_LongDictRaw; /* compressed long dictionary */

    ULONG pp_DiskTreeSize; /* backward position on disk (growing size) */
    UBYTE *pp_MapFileBuffer; /* buffer for mapped file */
    ULONG pp_MapFileSize; /* size of virtual memory */
    UBYTE *pp_DiskTree; /* start of compressed tree */

    /* node decrunching */
    struct HuffTree *pp_BranchTree; /* branch combination tree */
    struct HuffTree *pp_ShortEdgeTree; /* short edge combination tree */
    struct HuffTree *pp_LongEdgeLenTree; /* long edges length tree */

};

/*!
 * \brief
 */
class QueryHit {
public:
    QueryHit();
    virtual ~QueryHit();
    QueryHit(const QueryHit& hit);
    QueryHit& operator=(const QueryHit& hit);

    /*!
     * \brief The '<' operator is needed to support the usage of std::set
     *
     * \return bool
     */
    bool operator<(const QueryHit& other) const {
        return qh_AbsPos < other.qh_AbsPos;
    }

    /*!
     * \brief E.g. for sort by key compare
     */
    static bool keyCompare(const QueryHit& hit1, const QueryHit& hit2) {
        return hit1.qh_sortKey < hit2.qh_sortKey;
    }

    ULLONG qh_AbsPos; /* absolute position where hit was found in merged raw data */
    ULLONG qh_CorrectedAbsPos; /* absolute position corrected by insertion/deletion count where hit was found */
    UWORD qh_Flags; /* is this still valid */
    UWORD qh_ReplaceCount; /* Amount of replace operations required */
    UWORD qh_InsertCount; /* Amount of insert operations required */
    UWORD qh_DeleteCount; /* Amount of delete operations required */
    UWORD qh_nmismatch; /* nmismatch count */
    float qh_WMisCount; /* weighted mismatch error count */
    float qh_ErrorCount; /* errors that were needed to reach that hit */
    struct PTPanEntry *qh_Entry; /* pointer to entry this hit is in */
    // some stuff for output:
    std::string qh_seqout; /* used for differential alignment string */
    ULLONG qh_relpos; /* Relative position (if in alignment) (normally NOT pre-calculated!) */
    LLONG qh_sortKey;
};

/*!
 * \brief Class for string matching query settings
 */
class SearchQuery {
public:
    SearchQuery(struct MismatchWeights *wm);
    virtual ~SearchQuery();
    SearchQuery(const SearchQuery& query);
    SearchQuery& operator=(const SearchQuery& query);

    std::string sq_Query; /* query string */

    bool sq_Reversed; /* query is reversed */
    bool sq_AllowReplace; /* allow replace operations */
    bool sq_AllowInsert; /* allow inserting operations */
    bool sq_AllowDelete; /* allow delete operations */
    ULONG sq_KillNSeqsAt; /* if string has too many Ns, kill it */

    float sq_MaxErrors; /* maximum allowed errors */
    bool sq_WeightedSearchMode; /* Search mode; false = non-weighted, true = weighted */

    float sq_MinorMisThres; /* threshold for small letter mismatch output */
    struct MismatchWeights *sq_MismatchWeights; /* pointer to matrix for (weighted) mismatches */

    /*!
     * \brief Calculate SearchQuery max length value
     *
     * \return max length
     */
    ULONG calculate_max_length() const {
        // maxlength must not exceed prune length!
        ULONG maxlen = sq_Query.size();
        if (sq_MismatchWeights) {
            if (sq_AllowInsert) {
                if (sq_WeightedSearchMode) {
                    float minweight = sq_MismatchWeights->mw_Insert[0];
                    for (ULONG cnt = 1;
                            cnt < sq_MismatchWeights->mw_alphabet_size; cnt++) {
                        if (sq_MismatchWeights->mw_Insert[cnt] < minweight) {
                            minweight = sq_MismatchWeights->mw_Insert[cnt];
                        }
                    }
                    maxlen += (ULONG) ((sq_MaxErrors + minweight) / minweight);
                } else {
                    // all errors have weight = 1.0, so max errors is the maximum
                    // number of insertions possible!
                    maxlen += (ULONG) sq_MaxErrors;
                }
            }
        }
        return maxlen;
    }

private:
    SearchQuery();
};

/* data tuple for collected hits */
class HitTuple {
    HitTuple& operator = (const HitTuple& other);
public:
    HitTuple(ULLONG abs_pos, struct PTPanEntry *entry);
    HitTuple(const HitTuple& other);
    virtual ~HitTuple();

    ULLONG ht_AbsPos; /* absolute position of hit */
    struct PTPanEntry *ht_Entry; /* link to entry */

private:
    HitTuple();
};

/* probe candidate */
class DesignHit {
public:
    DesignHit(ULONG probe_length);
    virtual ~DesignHit();
    DesignHit(const DesignHit& hit);
    DesignHit& operator=(const DesignHit& hit);

    /*!
     * \brief The '<' operator is needed to support the usage of e.g. std::set
     *
     * Compares quality values
     *
     * \return bool
     */
    bool operator<(const DesignHit& other) const {
        return dh_Quality < other.dh_Quality;
    }

    /*!
     * \brief E.g. for sort
     */
    static bool qualityCompare(const DesignHit& hit1, const DesignHit& hit2) {
        return hit1.dh_Quality > hit2.dh_Quality;
    }

    /*!
     * \brief E.g. for sort
     */
    static bool keyCompare(const DesignHit& hit1, const DesignHit& hit2) {
        return hit1.dh_sortKey < hit2.dh_sortKey;
    }

    /*!
     * \brief Reset values
     */
    void reset() {
        dh_GroupHits = 0;
        dh_NonGroupHits = 0;
        dh_Temp = 0.0;
        dh_GCContent = 0;
        dh_relpos = 0;
        dh_ref_pos = 0;
        dh_Quality = 0.0;
        dh_Matches.clear();
        for (int i = 0; i < DECR_TEMP_CNT; i++) {
            dh_NonGroupHitsPerc[i] = 0;
        }
        dh_sortKey = 0;
    }

    std::string dh_ProbeSeq; /* sequence of the probe */
    ULONG dh_GroupHits; /* number of species covered by the hit */
    ULONG dh_NonGroupHits; /* number of non group hits inside this hit */
    double dh_Temp; /* temperature */
    UWORD dh_GCContent; /* number of G and C nucleotides in the hit */
    ULONG dh_relpos; /* relative position in aligned sequence */
    ULONG dh_ref_pos; /* position in reference entry (0 if no reference entry available!) */
    double dh_Quality; /* Quality of the design hit */
    boost::ptr_vector<HitTuple> dh_Matches; /* Absolute pos of all hits */
    ULONG dh_NonGroupHitsPerc[DECR_TEMP_CNT]; /* number of nongroup hits with decreasing temperature */
    ULONG dh_sortKey;

private:
    DesignHit();
};

/* data structure for probe design */
class DesignQuery {
public:
    DesignQuery(struct MismatchWeights *wm, std::set<std::string> * ids);
    virtual ~DesignQuery();
    DesignQuery(const DesignQuery& query);
    DesignQuery& operator=(const DesignQuery& query);

    UWORD dq_ProbeLength; /* length of the probe */
    UWORD dq_SortMode; /*  mode 0 quality, mode 1 sequence */
    ULONG dq_MinGroupHits; /* minimum species covered by the hit */
    ULONG dq_MaxNonGroupHits; /* number of non group hits allowed */
    double dq_MinTemp; /* minimum temperature */
    double dq_MaxTemp; /* maximum temperature */
    UWORD dq_MinGC; /* minimum number of G and C nucleotides in the hit */
    UWORD dq_MaxGC; /* maximum number of G and C nucleotides in the hit */
    LONG dq_MinPos; /* minimum position in entry sequence */
    LONG dq_MaxPos; /* maximum position in entry sequence, -1 means full length */
    ULONG dq_MaxHits; /* maximum number of matches */
    struct MismatchWeights *dq_MismatchWeights; /* pointer to matrix for mismatches */
    std::set<std::string> *dq_DesginIds; // The ids the probes are designed for

private:
    DesignQuery();

};

/*!
 * \brief ...
 */
class SimilaritySearchHit {
public:
    SimilaritySearchHit(ULLONG matches, double relMatches,
            struct PTPanEntry * entry);
    virtual ~SimilaritySearchHit();
    SimilaritySearchHit(const SimilaritySearchHit& hit);
    SimilaritySearchHit& operator=(const SimilaritySearchHit& hit);

    ULLONG ssh_Matches; /* Number of matches */
    double ssh_RelMatches; /* relative number of matches */
    struct PTPanEntry *ssh_Entry; /* pointer to entry this hit is in */

    /*!
     * \brief The '<' operator is needed to support the usage of e.g. std::set
     *
     * Compares quality values
     *
     * \return bool
     */
    bool operator<(const SimilaritySearchHit& other) const {
        return ssh_Matches < other.ssh_Matches;
    }

    /*!
     * \brief Returns 'hit1.ssh_Matches > hit2.ssh_Matches'
     *
     * \return bool
     */
    static bool greaterThan(const SimilaritySearchHit& hit1,
            const SimilaritySearchHit& hit2) {
        return hit1.ssh_Matches > hit2.ssh_Matches;
    }

    /*!
     * \brief Returns 'hit1.ssh_RelMatches > hit2.ssh_RelMatches'
     *
     * \return bool
     */
    static bool greaterThanRel(const SimilaritySearchHit& hit1,
            const SimilaritySearchHit& hit2) {
        return hit1.ssh_RelMatches > hit2.ssh_RelMatches;
    }

private:
    SimilaritySearchHit();
};

/*!
 * \brief
 */
class SimilaritySearchQuery {
public:
    SimilaritySearchQuery(const std::string& source_seq);
    virtual ~SimilaritySearchQuery();
    SimilaritySearchQuery(const SimilaritySearchQuery& query);
    SimilaritySearchQuery& operator=(const SimilaritySearchQuery& query);

    std::string ssq_SourceSeq; /* sequence to find family for */

    UWORD ssq_WindowSize; /* length of the probes */
    UWORD ssq_SearchMode; /* mode: 0 = search all [= normal mode], 1 = a* [= fast mode!] */
    UWORD ssq_ComplementMode; /*  Search for complement? (combination of: fwd=1, rev=2, rev.compl=4, compl=8) */
    UWORD ssq_SortType; /*  Type of sorting 0 = MATCHES, 1 = REL_MATCHES */
    double ssq_NumMaxMismatches; /* Number of maximum mismatches */
    LONG ssq_RangeStart; /* Start of range (-1 means ignore) */
    LONG ssq_RangeEnd; /* End of range (-1 means ignore) */

private:
    SimilaritySearchQuery();

};

#endif /* PTPAN_H_ */
