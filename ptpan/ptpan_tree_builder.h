/*!
 * \file ptpan_tree_builder.h
 *
 * \date 10.02.2011
 * \author Tilo Eissler
 * \author Chris Hodges
 * \author Jörg Böhnel
 */

#ifndef PTPAN_TREE_BUILDER_H_
#define PTPAN_TREE_BUILDER_H_

#include <boost/thread/mutex.hpp>

#include "ptpan_tree.h"
#include "ptpan_base.h"
#include "ptpan_build_settings.h"
#include "abstract_alphabet_specifics.h"
#include "abstract_data_retriever.h"

// some required defines
#define RELOFFSETBITS 27
#define RELOFFSETMASK ((1UL << RELOFFSETBITS) - 1)
#define INNERNODEBIT (1U << (RELOFFSETBITS + 2))
#define SHORTEDGEBIT (1U << (RELOFFSETBITS + 1))
#define NONUNIQUELONGEDGEBIT (1U << (RELOFFSETBITS + 3))

#define SIGNIFICANCE_THRESHOLD 0.25

/*!
 * \brief PTPan builder class
 */
class PtpanTreeBuilder: public PtpanBase {
private:
    //
    // first some private structs:
    //
    // another help structure for concatenated long edges
    struct ConcatenatedLongEdge {
        ULONG start_pos; /* The starting pos in the tmp-dictionary and afterwards the final dictionary */
        ULONG length; /* length of the concatenated long edge */
    };

    /* supporting structure for building long edge dictionary */
    struct BuildConcLongEdge {
        UBYTE *bcll_ConcatenatedLongEdges;
        ULONG bcll_ConcatenatedLongEdgeCount;
        ULONG bcll_ConcatenatedLongEdgeOffset;
        ULONG bcll_TouchedCount;
    };

    struct Leaf {
        ULONG sn_position;
        Leaf *next;
    };

    /* suffix tree node template */
    /*
     * AlphaMask upper 4 bits:
     * bit 32: short (1) or long (0) (edge-)label
     * bit 31: inner (1) or border (0) node
     * bit 30: short edge (1) or long edge (0) (used during calculate stats!)
     * bit 29: used during LongDict generation for extended edges!
     *
     * 00001 -> SEQCODE_N
     * 00010 -> SEQCODE_A
     * 00100 -> SEQCODE_C
     * 01000 -> SEQCODE_G
     * 10000 -> SEQCODE_T
     */
    struct SfxNode {
        ULONG sn_StartPos; /* start position; we need this for the long edge dictionary building */
        ULONG sn_Edge; /* edge label or pointer to label buffer (or prefix or dictionary position) */
        UINT sn_EdgeLen; /* length of edge */
        UINT sn_AlphaMask; /* alpha mask (upper 4 bits are used as flags)*/
    };

    struct SfxInnerNode {
        ULONG sn_StartPos; /* start position; we need this for the long edge dictionary building */
        ULONG sn_Edge; /* edge label or pointer to label buffer (or prefix or dictionary position) */
        UINT sn_EdgeLen; /* length of edge */
        UINT sn_AlphaMask; /* alpha mask (upper 4 bits are used as flags) used for quick lookup of first outgoing edge character */
        /* use end of struct-array trick to reduce number of allocs */
        ULONG sn_Children[1]; /* array of children */
    };

    struct SfxBorderNode {
        ULONG sn_StartPos; /* start position; we need this for the long edge dictionary building */
        ULONG sn_Edge; /* edge label or pointer to label buffer (or prefix or dictionary position) */
        UINT sn_EdgeLen; /* length of edge */
        UINT sn_AlphaMask; /* used for counting number of leafs in border nodes */
        ULONG sn_NumLeafs; /* number of leafs */
        ULONG sn_FirstLeaf; /* Pointer to first leaf */
        ULONG sn_LastLeaf; /* Pointer to last leaf (as shortcut) */
    };

    /*!
     * \brief Private subclass utilized during partition determination
     */
    class PartitionDetermination {
    public:
        PartitionDetermination(ULONG prefix, UWORD length, ULONG range_start,
                ULONG range_end) :
                m_prefix(prefix), m_prefix_length(length), m_base_count(0), m_prefix_range_start(
                        range_start), m_prefix_range_end(range_end), m_blanks(0) {
        }

        ~PartitionDetermination() {
        }

        PartitionDetermination(const PartitionDetermination& pd) :
                m_prefix(pd.m_prefix), m_prefix_length(pd.m_prefix_length), m_base_count(
                        pd.m_base_count), m_prefix_range_start(
                        pd.m_prefix_range_start), m_prefix_range_end(
                        pd.m_prefix_range_end), m_blanks(pd.m_blanks) {

        }

        PartitionDetermination& operator=(const PartitionDetermination& pd) {
            if (this != &pd) {
                m_prefix = pd.m_prefix;
                m_prefix_length = pd.m_prefix_length;
                m_base_count = pd.m_base_count;
                m_prefix_range_start = pd.m_prefix_range_start;
                m_prefix_range_end = pd.m_prefix_range_end;
                m_blanks = pd.m_blanks;
            }
            return *this;
        }

        /*!
         * \brief The '<' operator is needed to support the usage of std::set
         *
         * WARNING: it return '>' not '<' as it helps us more!
         *
         * \return bool
         */
        bool operator<(const PartitionDetermination& other) const {
            return m_base_count > other.m_base_count;
        }

        ULONG m_prefix;
        UWORD m_prefix_length;
        ULONG m_base_count;
        ULONG m_prefix_range_start;
        ULONG m_prefix_range_end;

        ULONG m_blanks;
    private:
        PartitionDetermination() {
        }

    };

    class PartitionStatistics {
    public:
        PartitionStatistics(ULONG id, UWORD alphabet_size,
                ULONG max_partition_count);
        ~PartitionStatistics();
        PartitionStatistics(const PartitionStatistics& ps);
        PartitionStatistics& operator=(const PartitionStatistics& ps);

        double load() const;
        bool significant() const;

        ULONG getId() const;
        ULONG getMaxPartitionCount() const;
        ULONG getOverallCount() const;
        ULONG getInnerNodes() const;
        ULONG getBorderNodes() const;
        UWORD getAlphabetSize() const;

        void setOverallCount(ULONG ocount);
        void setInnerNodes(ULONG inner);
        void setBorderNodes(ULONG border);

    private:
        PartitionStatistics();

        ULONG m_id;
        ULONG m_max_part_count;
        ULONG m_overall_count;
        ULONG m_inner_nodes;
        ULONG m_border_nodes;
        UWORD m_alphabet_size;

    };
    friend class PartitionStatistics;

    /*!
     * \brief Private struct containing some variables used only for building
     *     index partition
     */
    struct PTPanBuildPartition {
        struct PTPanPartition *pbp_PTPanPartition; /* up link */

        /* intermediate tree in memory */
        struct SfxInnerNode *pbp_SfxRoot; /* root of all evil */
        UBYTE *pbp_SfxNodes; /* suffix node array */
        ULONG pbp_SfxEdgeOffset; /* offset for inner and border nodes */
        UBYTE *pbp_Leafs; /* leaf buffer */
        ULONG pbp_LeafOffset; /* offset for leafs in buffer */
        /* extended long edge buffer stuff */
        ULONG pbp_ExtLongEdgeMemorySize; /* size of the extended long edge buffer */
        UBYTE *pbp_ExtLongEdgeBuffer; /* extended long edge buffer */
        ULONG pbp_ExtLongEdgeOffset; /* offset for extended long edges in buffer */

        ULONG pbp_NumBorderNodes; /* number of border nodes */
        ULONG pbp_NumInnerNodes; /* number of inner nodes */
        ULONG pbp_NumLeafs; /* number of leafs */

        // Prefix HashTable for build process speed-up
        ULONG *pbp_QuickPrefixLookup; /* hash table lookup */
        ULONG pbp_QuickPrefixCount; /* number of time the qpl was taken */

        struct HuffCode *pbp_BranchCode; /* branch combination histogram */

        /* edges huffman code stuff */
        ULONG pbp_EdgeCount; /* number of edges in the final tree */
        ULONG pbp_LongEdgeCount; /* number of long edges */

        struct HuffCode *pbp_ShortEdgeCode; /* short edge combination histogram */
        struct SfxNode **pbp_LongEdges; /* sorted array of tree pointers to long edges */
        ULONG pbp_unique_long_edges;

        ULONG pbp_LongEdgeLenSize; /* size of the huffman array (longest length + 1) */
        struct HuffCode *pbp_LongEdgeLenCode; /* long edges length histogram */

        ULONG pbp_LongDictRawSize; /* compressed long dictionary size on disk */

        /* leaf collection */
        LONG *pbp_LeafBuffer; /* temporary leaf buffer */
        ULONG pbp_LeafBufferSize; /* size of leaf buffer */

        /* file saving */
        ULONG *pbp_WriteNodesBufferPtr; /* current node pointer buffer pointer position */
        ULONG pbp_DiskBufferSize; /* Size of disk buffer */

        ULONG max_prefix_lookup_size; /* maximum prefix lookup length (default is 7) */
    };

    class LongEdgeComparator {
    private:
        LongEdgeComparator();
    public:
        LongEdgeComparator(AbstractAlphabetSpecifics *as,
                bool extended_long_edges);
        LongEdgeComparator(const LongEdgeComparator& comp);
        LongEdgeComparator& operator=(const LongEdgeComparator& comp);
        virtual ~LongEdgeComparator();

        /*!
         * \brief Function operator
         *
         * Ugly, but necessary
         */
        bool operator()(struct SfxNode* node_one, struct SfxNode* node_two) {
            if (m_extended_long_edges) {
                // TODO FIXME
            } else {
                if (node_one->sn_StartPos == node_two->sn_StartPos) {
                    // no need to compare as they start at the same position
                    if (node_one->sn_EdgeLen < node_two->sn_EdgeLen) {
                        return true;
                    }
                    return false; // string exactly equal
                }

                if (node_one->sn_EdgeLen == node_two->sn_EdgeLen) {
                    if (node_one->sn_Edge == node_two->sn_Edge) {
                        if (node_one->sn_StartPos < node_two->sn_StartPos) {
                            node_two->sn_StartPos = node_one->sn_StartPos;
                        }
                        return false;
                    } else {
                        if (node_one->sn_Edge < node_two->sn_Edge) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                }

                if (node_one->sn_EdgeLen < node_two->sn_EdgeLen) {
                    // take edge of node 1 and compare it to the first
                    // part of node 2 (with length of node 1)
                    if (node_one->sn_Edge
                            > (node_two->sn_Edge
                                    / m_as->power_table[(node_two->sn_EdgeLen
                                            - node_one->sn_EdgeLen)])) {
                        return false;
                    }
                    // shorter sequence is "smaller"
                    return true;
                } else {
                    ULONG code_one = (node_one->sn_Edge
                            / m_as->power_table[(node_one->sn_EdgeLen
                                    - node_two->sn_EdgeLen)]);
                    if (code_one > node_two->sn_Edge) {
                        return false;
                    } else if (code_one < node_two->sn_Edge) {
                        return true;
                    }
                    // sequence prefixes were the same!
                    // move starting pos "down"
                    if (node_one->sn_StartPos < node_two->sn_StartPos) {
                        node_two->sn_StartPos = node_one->sn_StartPos;
                    }
                    return false;
                }
            }
            return false;
        }

    private:
        AbstractAlphabetSpecifics *m_as;
        bool m_extended_long_edges;
    };

public:
    virtual ~PtpanTreeBuilder();

    static void buildPtpanTree(PtpanBuildSettings *settings,
            AbstractAlphabetSpecifics *as, AbstractDataRetriever *dr,
            const std::string& custom_info);

    static const std::string version() {
        return PtpanTree::version();
    }

private:
    PtpanTreeBuilder();
    PtpanTreeBuilder(const PtpanTreeBuilder& ptb);
    PtpanTreeBuilder& operator=(const PtpanTreeBuilder& ptb);

    void checkSettings();
    void loadReferenceEntry(); // load reference entry (if available)
    void loadEntry();
    void loadEntries(); // load data, build index header partly, split points
    void freeStuff();
    void reInitStuff();
    void mergeEntry();
    void buildMergedRawData();
    void partitionDetermination(); // including update of index header, store alphabetSpecifics as well!!
    void refinePartition(const PartitionDetermination& pd,
            std::deque<PartitionDetermination>& queue, ULONG max_prefix_length);
    void initIndexHeader(const std::string& custom_info); // init the index header and write some data (still some missing!)
    void storeIndexHeader(); // store index header after all information is available
    void buildTree(); // build Tree for all Partitions
    void buildTreeThread();
    void buildPartitionTree(struct PTPanPartition *pp);
    // some methods called for each partition:
    struct PTPanBuildPartition* pbt_initPtpanBuildPartition(
            struct PTPanPartition *pp);
    void pbt_freePtpanBuildPartition(struct PTPanBuildPartition* pbp);
    void pbt_buildMemoryTree(struct PTPanBuildPartition* pbp);
    void pbt_insertTreePos(struct PTPanBuildPartition* pbp, ULONG pos,
            ULONG* window);
    void pbt_calculateTreeStats(struct PTPanBuildPartition* pbp);
    void pbt_getTreeStatsBranchHistoRec(struct PTPanBuildPartition *pbp,
            struct SfxInnerNode *sfxnode);
    void pbt_getTreeStatsShortEdgesRec(struct PTPanBuildPartition *pbp,
            struct SfxNode *sfxnode);
    void pbt_buildLongEdgeDictionary(struct PTPanBuildPartition *pbp);
    void pbt_getTreeStatsLongEdgesRec(struct PTPanBuildPartition *pbp,
            struct SfxNode *sfxnode,
            boost::unordered_map<ULONG, SfxNode*>& unique_long_edges);
    void pbt_correctLongEdgeSorting(struct SfxNode ** edges, ULONG edge_count);
    struct BuildConcLongEdge* pbt_initBuildConcLongEdge(ULONG edge_count);
    void pbt_freeBuildConcLongEdge(struct BuildConcLongEdge *bcll);
    STRPTR pbt_buildConcatenatedLongEdges(struct SfxNode ** edges,
            ULONG edge_count, struct BuildConcLongEdge *longEdgeConc,
            STRPTR tmp_buffer, ULONG *tmp_buffer_size, ULONG *tmp_buffer_index);
    void pbt_writeTreeToDisk(struct PTPanBuildPartition *pbp);
    void pbt_writeTreeHeader(struct PTPanBuildPartition *pbp, FILE *fh);
    ULONG pbt_fixRelativePointersRec(struct PTPanBuildPartition *pbp,
            struct SfxNode *sfxnode);
    ULONG pbt_calcPackedNodeSize(struct PTPanBuildPartition *pbp,
            struct SfxNode *sfxnode);
    ULONG pbt_calcPackedLeafSize(struct PTPanBuildPartition *pbp,
            struct SfxBorderNode *sfxnode);
    ULONG pbt_writePackedNode(struct PTPanBuildPartition *pbp,
            struct SfxNode *sfxnode, UBYTE *buf);
    ULONG pbt_writePackedLeaf(struct PTPanBuildPartition *pbp,
            struct SfxBorderNode *sfxnode, UBYTE *buf);

    /*!
     * \brief Private subclass handling thread-safe retrieval of
     * next entry from list
     */
    class EntryDispatcher {
        EntryDispatcher(const EntryDispatcher&);
        EntryDispatcher& operator=(const EntryDispatcher&);
        
    public:
        EntryDispatcher() :
            m_ps(NULL), m_mutex(), m_first(true) {
        }

        ~EntryDispatcher() {
        }

        void setEntryHead(struct PTPanEntry* ps) {
            m_ps = ps;
            m_first = true;
        }

        struct PTPanEntry* getNext() const {
            boost::unique_lock<boost::mutex> lock(m_mutex);
            if (m_first) {
                m_first = false;
                return m_ps;
            }
            // maybe this can be done easier, but I cannot think clear now
            // and it works, so I leave it for the moment ...
            if (m_ps) {
                if (m_ps->pe_Node.ln_Succ) {
                    m_ps = (struct PTPanEntry *) m_ps->pe_Node.ln_Succ;
                }
            }
            if (m_ps->pe_Node.ln_Succ) {
                return m_ps;
            }
            return NULL;
        }

    private:
        mutable struct PTPanEntry* m_ps;
        mutable boost::mutex m_mutex;
        mutable bool m_first;
    };

    typedef std::multimap<ULONG, struct PTPanPartition *> pmap;

    /*!
     * \brief Private subclass handling thread-safe retrieval of
     * next partition
     */
    class PartitionDispatcher {
    public:
        PartitionDispatcher() :
                m_mutex() {
        }

        ~PartitionDispatcher() {
        }

        void setPartitionList(const pmap& partitions) {
            m_sorted_partitions = partitions;
            m_current = m_sorted_partitions.rbegin();
        }

        struct PTPanPartition * getNext() const {
            boost::unique_lock<boost::mutex> lock(m_mutex);
            if (m_current != m_sorted_partitions.rend()) {
                // less simple solution: pmap::const_reverse_iterator tmp = m_current;
                return (m_current++)->second;
            }
            return NULL;
        }

    private:
        mutable boost::mutex m_mutex;
        // partitions may have the same size! so we use multimap
        pmap m_sorted_partitions;
        mutable pmap::const_reverse_iterator m_current;
    };

    mutable boost::mutex m_build_mutex;

    PtpanBuildSettings *m_settings;
    AbstractAlphabetSpecifics *m_as;
    AbstractDataRetriever *m_dr;

    struct PTPanReferenceEntry *m_ref_entry;
    ULONG m_num_entries; /* number of entries accounted for */
    struct List *m_entries; // list of entry data

    std::string m_index_name;
    FILE *m_index_fh; // index header file
    ULONG m_map_file_size;
    UBYTE *m_map_file_buffer;
    std::string m_tmp_raw_file_name;
    ULONG m_merged_raw_data_size;
    ULONG *m_merged_raw_data; // all compressed and merged sequence data

    EntryDispatcher m_dispatcher;
    std::map<ULONG, ULONG> m_merge_border_map;
    PartitionDispatcher m_partition_dispatcher;

    ULLONG m_total_seq_size;
    ULLONG m_total_raw_size;
    ULLONG m_total_seq_compressed_size; // in byte!

    bool m_extended_long_edges;
    UWORD m_long_edge_max_size_factor;

    ULONG m_all_hash_sum;

    pmap m_partitions; // prefix partitions
    std::vector<PartitionStatistics> m_partitions_statistics;
    ULONG m_max_partition_size;

    ULONG write_byte(ULONG value, UBYTE *buffer);
    void write_byte(ULONG value, ULONG count, UBYTE *buffer);
    void seek_byte(FILE *fileheader);

    /*!
     * \brief ConcatenatedLongEdgeLengthCompare()
     */
    static bool ConcatenatedLongEdgeLengthCompare(
            struct ConcatenatedLongEdge *edge1,
            struct ConcatenatedLongEdge *edge2) {
        return edge2->length < edge1->length;
    }

    /*!
     * \brief LongEdgeLengthCompare()
     */
    static bool LongEdgeLengthCompare(struct SfxNode *node1,
            struct SfxNode *node2) {
        return node2->sn_EdgeLen < node1->sn_EdgeLen;
    }

    /*!
     * \brief LongEdgePosCompare()
     */
    static bool LongEdgePosCompare(struct SfxNode *node1,
            struct SfxNode *node2) {
        return node1->sn_StartPos < node2->sn_StartPos;
    }

};

#endif /* PTPAN_TREE_BUILDER_H_ */
