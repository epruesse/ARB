/*!
 * \file ptpan_tree.h
 *
 * \date 12.02.2011
 * \author Tilo Eissler
 * \author Chris Hodges
 * \author Jörg Böhnel
 */

#ifndef PTPAN_TREE_H_
#define PTPAN_TREE_H_

#include <stdlib.h>

#include <string>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/atomic.hpp>

#include "ptpan_base.h"
#include "abstract_alphabet_specifics.h"
#include "hash_key.h"

// Levenshtein distance: Some definitions for the direction matrix (for backtracking)
#define LEV_SUB   0
#define LEV_INS   1
#define LEV_DEL   2
#define LEV_EQ    3
#define LEV_START 4

/*!
 * \brief PTPanTree class is the main class for querying a PTPan index
 *
 * Yes, some methods reveal inner structures, so be careful with free'ing memory.
 */
class PtpanTree: public PtpanBase {
private:
    typedef boost::unordered_map<const char *, long, hash_key<const char *>,
            equal_str> PtpanEntryMap;

    // decrunched tree node data structure
    struct TreeNode {
        struct PTPanPartition *tn_PTPanPartition; /* uplinking */
        ULONG tn_Pos; /* absolute tree position */
        ULONG tn_Size; /* size of the uncompressed node in bytes */

        UWORD tn_ParentSeq; /* branch that lead there */
        struct TreeNode *tn_Parent; /* parent node */
        UWORD tn_TreeOffset; /* sum of the parent edges (i.e. relative string position) */

        ULONG *tn_Children; /* absolute position of the children [ALPHASIZE] */
        ULONG tn_NumLeaves; /* number of leaves in this node; inner nodes have always 0 leaves,
         border nodes have at least 1 leaf! */
        LONG *tn_Leaves; /* pointer to leaf array */

        UWORD tn_EdgeLen; /* length of edge */
        char tn_Edge[1]; /* decompressed edge; NOTE: at the moment, this must be at the end
         of the struct as the necessary memory is allocated together with the
         rest of the struct !! */

        /*!
         * \brief Free given tree node
         */
        static void free_tree_node(struct TreeNode *tn) {
            if (tn) {
                if (tn->tn_NumLeaves > 0) {
                    free(tn->tn_Leaves);
                }
                if (tn->tn_Children) {
                    free(tn->tn_Children);
                }
                free(tn);
                tn = NULL;
            }
        }
    };

    /*!
     * \brief automata state for search algorithm
     */
    struct SearchQueryState {
        struct TreeNode *sqs_TreeNode; /* tree node */
        ULONG sqs_QueryPos; /* current position within query */
        UWORD sqs_ReplaceCount; /* number of replace operations so far */
        UWORD sqs_InsertCount; /* number of insert operations so far */
        UWORD sqs_DeleteCount; /* number of delete operations so far */
        UWORD sqs_NCount; /* number of N-sequence codes encountered */
        float sqs_WMisCount; /* weighted mismatch count */
        float sqs_ErrorCount; /* errors currently found */

        void reset() {
            sqs_TreeNode = NULL;
            sqs_QueryPos = 0;
            sqs_ReplaceCount = 0;
            sqs_InsertCount = 0;
            sqs_DeleteCount = 0;
            sqs_NCount = 0;
            sqs_WMisCount = 0.0;
            sqs_ErrorCount = 0.0;
        }
    };

    /*!
     * \brief This class is responsible for a SearchQuery and takes
     *        care of running variables and other altering stuff.
     */
    class SearchQueryHandle {
    public:
        SearchQueryHandle(const SearchQuery& sq);
        virtual ~SearchQueryHandle();
        SearchQueryHandle(const SearchQueryHandle& sqh);
        SearchQueryHandle& operator=(const SearchQueryHandle& sqh);

        struct SearchQueryState sqh_State; // running error variables

        double* sqh_PosWeight; // mismatch multiplier (Gaussian like distribution over search string)
        ULONG sqh_PosWeightLength;
        ULONG sqh_MaxLength; // Maximum length of source string (> query-length if indels allowed)

        boost::unordered_map<ULONG, ULONG> *sqh_HitsHash; // hash array to avoid double entries, only for Levenshtein
        boost::ptr_vector<QueryHit> sqh_hits;

        void sortByAbsPos();

        void reset() {
            sqh_State.reset();
            if (sqh_HitsHash) {
                sqh_HitsHash->clear();
            }
            sqh_hits.clear();
        }

    private:
        SearchQueryHandle();

        /*!
         * \brief Calculate weighted mismatch value for position
         */
        double calc_position_wmis(int pos, int seq_len, double y1,
                double y2) const {
            return (double) (((double) (pos * (seq_len - 1 - pos))
                    / (double) ((seq_len - 1) * (seq_len - 1)))
                    * (double) (y2 * 4.0) + y1);
        }

        /*!
         * \brief Build positional weight matrix for either weighted or non-weighted mismatches
         */
        void build_pos_weight(const SearchQuery& sq) {
            sqh_PosWeightLength = sq.sq_Query.size();
            sqh_PosWeight = new double[sqh_PosWeightLength + 1];
// ...
for(            ULONG pos = 0; pos < sqh_PosWeightLength; ++pos) {
                if (sq.sq_WeightedSearchMode) {
                    sqh_PosWeight[pos] = calc_position_wmis(pos,
                    sqh_PosWeightLength, 0.3, 1.0);
                } else {
                    sqh_PosWeight[pos] = 1.0;
                }
            }
            sqh_PosWeight[sqh_PosWeightLength] = 0.0;
        }

    };

    /*!
     * \brief This class is responsible for a DesignQuery and takes
     *        care of running variables and other altering stuff.
     */
    class DesignQueryHandle {
    public:
        DesignQueryHandle(const DesignQuery& dq,
                const PtpanTree::PtpanEntryMap& map,
                struct PTPanEntry **entries_direct_map, bool contains_features);
        virtual ~DesignQueryHandle();
        DesignQueryHandle(const DesignQueryHandle& dqh);
        DesignQueryHandle& operator=(const DesignQueryHandle& dqh);

        std::set<ULONG> dqh_MarkedEntryNumbers;
        boost::ptr_vector<DesignHit> dqh_Hits;

        // evtl. mit unordered_multimap !?
        bool dqh_handleFeatures;
        // store marked features under id of corresponding PTPanEntry
        boost::unordered_multimap<ULONG, PTPanFeature*> dqh_ptpanFeatureMap;

        // here are some "running variables"
        DesignHit *dqh_TempHit; // so far unused DesignHit
        struct TreeNode *dqh_TreeNode; // current tree node traversed

        void reset() {
            dqh_Hits.clear();
            if (dqh_TempHit) {
                delete dqh_TempHit;
            }
            dqh_TreeNode = NULL;
        }

        // check if any feature is hit by QueryHit
        bool hitsAnyFeature(ULLONG abspos, PTPanEntry* entry,
                const DesignQuery& dq) const;

    private:
        DesignQueryHandle();

    };

    friend class DesignQueryHandle;

    /*!
     * \brief This class is responsible for a SimilaritySearchQuery and takes
     *        care of running variables and other altering stuff.
     */
    class SimilaritySearchQueryHandle {
    public:
        SimilaritySearchQueryHandle(ULONG entry_count);
        virtual ~SimilaritySearchQueryHandle();
        SimilaritySearchQueryHandle(const SimilaritySearchQueryHandle& ssqh);
        SimilaritySearchQueryHandle& operator=(
                const SimilaritySearchQueryHandle& ssqh);

        boost::ptr_vector<SimilaritySearchHit> ssqh_Hits;
        STRPTR ssqh_filtered_seq; // maybe we get rid of this someday!
        ULONG ssqh_filtered_seq_len; // maybe we get rid of this someday!

        ULONG ssqh_entry_count;
        boost::atomic_ulong* ssqh_hit_count;
        double* ssqh_rel_hit_count;

        void reset() {
            ssqh_Hits.clear();
            if (ssqh_filtered_seq) {
                free(ssqh_filtered_seq);
            }
            ssqh_filtered_seq_len = 0;
            for (ULONG cnt = 0; cnt < ssqh_entry_count; cnt++) {
                ssqh_hit_count[cnt].store(0);
                ssqh_rel_hit_count[cnt] = 0.0;
            }
        }

    private:
        SimilaritySearchQueryHandle();
    };

    /*!
     * \brief Private subclass to handle housekeeping of Levenshtein matching
     *     over PTPan
     */
    class LevenshteinSupport {
        LevenshteinSupport(const LevenshteinSupport&);
        LevenshteinSupport& operator=(const LevenshteinSupport&);
    public:
        LevenshteinSupport(ULONG source_len, ULONG query_len, ULONG max_check) :
                m_distance_matrix(NULL), m_direction_matrix(NULL), m_source_len(
                        source_len), m_query_len(query_len), m_max_check(
                        max_check) {

            m_distance_matrix = (double **) calloc(
                    (m_source_len + 1) * sizeof(double *), 1);
            for (ULONG i = 0; i < m_source_len + 1; i++) {
                m_distance_matrix[i] = (double *) calloc(
                        (m_query_len + 1) * sizeof(double), 1);
            }

            m_direction_matrix = (char **) calloc(
                    (m_source_len + 1) * sizeof(char *), 1);
            for (ULONG i = 0; i < m_source_len + 1; i++) {
                m_direction_matrix[i] = (char *) calloc(
                        (m_query_len + 1) * sizeof(char), 1);
            }

        }

        virtual ~LevenshteinSupport() {
            try {
                if (m_direction_matrix) {
                    for (ULONG i = 0; i < m_source_len + 1; i++) {
                        free(m_direction_matrix[i]);
                    }
                    free(m_direction_matrix);
                }
                if (m_distance_matrix) {
                    for (ULONG i = 0; i < m_source_len + 1; i++) {
                        free(m_distance_matrix[i]);
                    }
                    free(m_distance_matrix);
                }
            } catch (...) {
                // destructor must not throw ...
            }
        }

        double **m_distance_matrix;
        char **m_direction_matrix;
        ULONG m_source_len;
        ULONG m_query_len;
        ULONG m_max_check;
    };

public:
    PtpanTree(const std::string& index_name);
    virtual ~PtpanTree();

    /*!
     * \brief PTPan version number as string
     *
     * scheme: MAJOR.MINOR.BUGFIX
     */
    static const std::string version() {
        return "0.6.5_beta";
    }

    const std::string getCustomInformation() const;

    ULONG countEntries() const;
    bool containsEntry(const char *id) const;
    bool containsEntry(const std::string& id) const;

    bool containsReferenceEntry() const;
    const PTPanReferenceEntry* getReferenceEntry() const;

    bool containsFeatures() const;
    const PTPanEntry* getFirstEntry() const;
    const PTPanEntry* getEntry(ULONG id) const;
    CONST_STRPTR getEntryFullName(ULONG id) const;
    CONST_STRPTR getEntryId(ULONG id) const;

    const std::vector<std::string> getAllEntryIds() const;

    ULONG getPruneLength() const;
    const AbstractAlphabetSpecifics* getAlphabetSpecifics() const;

    // search and information
    const boost::ptr_vector<QueryHit> searchTree(const SearchQuery& sq,
            bool sort, bool build_diff) const;

    const std::vector<std::string> getAllProbes(ULONG length) const;
    STRPTR getProbes(STRPTR seed_probe, ULONG length, ULONG num_probes) const;
    const boost::ptr_vector<DesignHit> designProbes(
            const DesignQuery& dq) const; // TODO filter!?

    double probeNongroupIncreaseWMis() const;
    ULONG probeNongroupIncreaseArrayWidth() const;

    const boost::ptr_vector<SimilaritySearchHit> similaritySearch(
            const SimilaritySearchQuery& ssq) const;

private:
    PtpanTree();
    PtpanTree(const PtpanTree& pt);
    PtpanTree& operator=(const PtpanTree& pt);

    void loadIndexHeader();
    void loadPartitions();

    std::string m_index_name;
    std::string m_custom_info;
    ULONG m_map_file_size;
    UBYTE *m_map_file_buffer;
    AbstractAlphabetSpecifics *m_as;
    ULONG m_prune_length;
    ULONG m_entry_count;
    PtpanEntryMap m_entry_map;

    ULLONG m_TotalSeqSize; // total size of sequences
    ULLONG m_TotalSeqCompressedSize; // total size of compressed Seq. Data (with '.' and '-') in byte
    ULLONG m_TotalRawSize; // total size of data tuples in entries
    ULONG m_AllHashSum; // hash sum over everything to checksum integrity

    struct List *m_entries; // list of entry data
    std::vector<PTPanPartition *> m_partitions; // prefix partitions

    bool m_contains_features;

    struct PTPanReferenceEntry *m_ref_entry;
    // entries name/id hashing
    struct BinTree *m_EntriesBinTree; // tree to find an entry by its absolute offset
    struct PTPanEntry **m_EntriesMap; // direct mapping

    void get_tree_path(struct TreeNode *tn, std::string& strptr,
            ULONG len) const;
    std::string get_tree_path(struct TreeNode *tn, ULONG len) const;
    struct TreeNode*
    go_down_node_child(struct TreeNode *oldtn, UWORD seqcode) const;
    struct TreeNode* go_down_node_child_no_edge(struct TreeNode *oldtn,
            UWORD seqcode) const;

    struct TreeNode*
    read_packed_node(struct PTPanPartition *pp, ULONG pos) const;
    struct TreeNode* read_packed_node_no_edge(struct PTPanPartition *pp,
            ULONG pos) const;
    void read_packed_leaf_array(struct PTPanPartition *pp, ULONG pos,
            struct TreeNode *tn) const;

    ULONG read_byte(FILE *fileheader) const;

    ULONG calculate_max_length(const SearchQuery& sq) const;
    double calc_position_wmis(int pos, int seq_len, double y1, double y2) const;
    void build_pos_weight(SearchQueryHandle& sqh, const SearchQuery& sq) const;
    ULONG copy_sequence(const SearchQuery& sq, std::string& source_seq,
            const QueryHit& qh, ULONG &bitpos, ULONG *nmismatch,
            ULONG &count) const;
    bool match_sequence(const SearchQuery& sq, SearchQueryHandle& sqh,
            const std::string& compare, const QueryHit& qh) const;
    bool levenshtein_match(const SearchQuery& sq, SearchQueryHandle& sqh,
            const std::string& compare, STRPTR tarstr,
            bool create_string) const;
    bool hamming_match(const SearchQuery& sq, SearchQueryHandle& sqh,
            const std::string& compare, STRPTR tarstr,
            bool create_string) const;
    void
    search_partition(struct PTPanPartition *pp, const SearchQuery& sq,
            SearchQueryHandle& sqh) const;
    void search_tree(struct PTPanPartition *pp, const SearchQuery& sq,
            SearchQueryHandle& sqh) const;
    void add_query_hit(const SearchQuery& sq, SearchQueryHandle& sqh,
            ULONG hitpos) const;
    void search_tree_hamming(const SearchQuery& sq,
            SearchQueryHandle& sqh) const;
    void search_tree_levenshtein(const SearchQuery& sq,
            SearchQueryHandle& sqh) const;
    void search_tree_levenshtein_rec(const SearchQuery& sq,
            SearchQueryHandle& sqh, UBYTE *source, UBYTE *query,
            LevenshteinSupport& levenshtein, ULONG start_j) const;
    void collect_tree_rec(const SearchQuery& sq, SearchQueryHandle& sqh) const;
    void post_filter_query_hits(const SearchQuery& sq,
            SearchQueryHandle& sqh) const;
    void validate_unsafe_hits(const SearchQuery& sq,
            SearchQueryHandle& sqh) const;
    void sort_hits_list(const SearchQuery& sq, SearchQueryHandle& sqh) const;
    void create_diff_alignment(const SearchQuery& sq, SearchQueryHandle& sqh,
            int offset = 0, int participators = 1) const;

    // some methods used by probe design
    void find_probes_rec(struct TreeNode *parent_tn,
            std::vector<std::string>& results, ULONG depth, ULONG length) const;
    void find_probe_in_partition(struct PTPanPartition *pp,
            const DesignQuery& dq, DesignQueryHandle& dqh) const;
    void design_probes(const DesignQuery& dq, DesignQueryHandle& dqh,
            int offset = 0, int participators = 1) const;
    void calculate_probe_quality(DesignQueryHandle& dqh, int offset = 0,
            int participators = 1) const;
    void add_design_hit(const DesignQuery& dq, DesignQueryHandle& dqh) const;

    void
    ss_window_sequence(const SimilaritySearchQuery& ssq,
            SimilaritySearchQueryHandle& ssqh) const;
    void
    ss_window_search(const SimilaritySearchQuery& ssq,
            SimilaritySearchQueryHandle& ssqh, int offset = 0,
            int participators = 1) const;
    void ss_create_hits(const SimilaritySearchQuery& ssq,
            SimilaritySearchQueryHandle& ssqh) const;

};

#endif /* PTPAN_TREE_H_ */
