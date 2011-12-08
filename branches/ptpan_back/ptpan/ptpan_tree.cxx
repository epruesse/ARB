/*!
 * \file ptpan_tree.cxx
 *
 * \date 12.02.2011
 * \author Tilo Eissler
 * \author Chris Hodges
 * \author Jörg Böhnel
 */
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <sys/mman.h>

#include <stdexcept>
#include <algorithm>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered_set.hpp>

#include "ptpan_tree.h"
#include "ptpan_filter.h"
#include "dna5_alphabet_specifics.h"

/*!
 * \brief Private default constructor
 */
PtpanTree::PtpanTree() :
        PtpanBase(), m_index_name(), m_custom_info(), m_map_file_size(0), m_map_file_buffer(
                NULL), m_as(NULL), m_prune_length(0), m_entry_count(0), m_entry_map(), m_TotalSeqSize(
                0), m_TotalSeqCompressedSize(0), m_TotalRawSize(0), m_AllHashSum(
                0), m_entries(NULL), m_partitions(), m_contains_features(false), m_ref_entry(
                NULL), m_EntriesBinTree(NULL), m_EntriesMap(NULL) {
}

/*!
 * \brief Constructor taking index name
 *
 * \param index_name complete name and path of index (header)
 *
 * \excpetion std::invalid_argument Thrown if index name is empty
 */
PtpanTree::PtpanTree(const std::string& index_name) :
        PtpanBase(), m_index_name(index_name), m_custom_info(), m_map_file_size(
                0), m_map_file_buffer(NULL), m_as(NULL), m_prune_length(0), m_entry_count(
                0), m_entry_map(HASH_SIZE_SEED), m_TotalSeqSize(0), m_TotalSeqCompressedSize(
                0), m_TotalRawSize(0), m_AllHashSum(0), m_entries(NULL), m_partitions(), m_contains_features(
                false), m_ref_entry(NULL), m_EntriesBinTree(NULL), m_EntriesMap(
                NULL) {
    if (m_index_name.empty()) {
        throw std::invalid_argument(
                "PtpanTree(index_name) given index name is empty!");
    }
    m_entries = (struct List *) malloc(sizeof(List));
    NewList(m_entries);
    loadIndexHeader();
    loadPartitions();
}

/*!
 * \brief Private copy constructor
 */
PtpanTree::PtpanTree(const PtpanTree& /*pt*/) {
}

/*!
 * \brief Private assignment operator
 */
PtpanTree& PtpanTree::operator=(const PtpanTree& pt) {
    if (this != &pt) {
        // no need at the moment
    }
    return *this;
}

/*!
 * \brief Destructor
 */
PtpanTree::~PtpanTree() {
    try {
        if (m_as) {
            delete m_as;
        }
        if (m_entries) {
            // unmap compressed seq
            struct PTPanEntry *ps = (struct PTPanEntry *) m_entries->lh_Head;
            while (ps->pe_Node.ln_Succ) {
                ps->pe_SeqDataCompressed = NULL;
                ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
            }
            free_all_ptpan_entries(m_entries);
            free(m_entries);
        }
        if (m_map_file_buffer) {
            munmap(m_map_file_buffer, m_map_file_size);
        }
        std::vector<PTPanPartition *>::const_iterator pit;
        for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
            free_partition(*pit);
        }
        if (m_ref_entry) {
            free_ptpan_reference_entry(m_ref_entry);
        }
        if (m_EntriesBinTree) {
            FreeBinTree(m_EntriesBinTree);
        }
        if (m_EntriesMap) {
            free(m_EntriesMap);
        }
        m_entry_map.clear();
    } catch (...) {
        // destructor must not throw
    }
}

/*!
 * \brief Constructor
 */
PtpanTree::SearchQueryHandle::SearchQueryHandle(const SearchQuery& sq) :
        sqh_State(), sqh_PosWeight(NULL), sqh_PosWeightLength(0), sqh_MaxLength(), sqh_HitsHash(
                NULL), sqh_hits() {
    // calculate maximum size of string that we have to examine
    // for hits with insertions or query length > prune length!
    sqh_MaxLength = sq.calculate_max_length();
    build_pos_weight(sq);
    if (sq.sq_AllowInsert || sq.sq_AllowDelete) {
        sqh_HitsHash = new boost::unordered_map<ULONG, ULONG>(HASH_SIZE_SEED);
    }
}

/*!
 * \brief Destructor
 */
PtpanTree::SearchQueryHandle::~SearchQueryHandle() {
    try {
        if (sqh_PosWeight) {
            delete[] sqh_PosWeight;
        }
        if (sqh_HitsHash) {
            delete sqh_HitsHash;
        }
    } catch (...) {
        // destructor must not throw
    }
}

/*!
 * \brief Private constructor
 */
PtpanTree::SearchQueryHandle::SearchQueryHandle() {
}

/*!
 * \brief Copy constructor
 *
 * \warning does not copy already present hits!
 */
PtpanTree::SearchQueryHandle::SearchQueryHandle(const SearchQueryHandle& sqh) :
        sqh_State(), sqh_PosWeight(NULL), sqh_PosWeightLength(
                sqh.sqh_PosWeightLength), sqh_MaxLength(sqh.sqh_MaxLength), sqh_HitsHash(
                NULL), sqh_hits() {
    if (sqh.sqh_PosWeight) {
        sqh_PosWeight = new double[sqh_PosWeightLength + 1];
        //..
        for (ULONG i = 0; i < sqh_PosWeightLength + 1; i++) {
            sqh_PosWeight[i] = sqh.sqh_PosWeight[i];
        }
    }
    if (sqh.sqh_HitsHash) {
        sqh_HitsHash = new boost::unordered_map<ULONG, ULONG>(HASH_SIZE_SEED);
    }
}

/*!
 * \brief Private assignment operator
 *
 * \warning does not copy already present hits!
 */
PtpanTree::SearchQueryHandle& PtpanTree::SearchQueryHandle::operator=(
        const SearchQueryHandle& sqh) {
    if (this != &sqh) {
        // sqh_State = SearchQueryState();
        sqh_PosWeightLength = sqh.sqh_PosWeightLength;
        if (sqh.sqh_PosWeight) {
            sqh_PosWeight = new double[sqh_PosWeightLength + 1];
            for (ULONG i = 0; i < sqh_PosWeightLength + 1; i++) {
                sqh_PosWeight[i] = sqh.sqh_PosWeight[i];
            }
        } else {
            sqh_PosWeight = NULL;
        }
        sqh_MaxLength = sqh.sqh_MaxLength;
        if (sqh.sqh_HitsHash) {
            sqh_HitsHash = new boost::unordered_map<ULONG, ULONG>(
                    HASH_SIZE_SEED);
        } else {
            sqh_HitsHash = NULL;
        }
        // sqh_hits;
    }
    return *this;
}

/*!
 * \brief Method for sorting QueryHits by AbsPos
 */
void PtpanTree::SearchQueryHandle::sortByAbsPos() {
    sqh_hits.sort();
}

/*!
 * \brief Constructor
 */
PtpanTree::DesignQueryHandle::DesignQueryHandle(const DesignQuery& dq,
        const PtpanTree::PtpanEntryMap& map,
        struct PTPanEntry **entries_direct_map, bool contains_features) :
        dqh_MarkedEntryNumbers(), dqh_Hits(), dqh_handleFeatures(
                contains_features), dqh_ptpanFeatureMap(), dqh_TempHit(NULL), dqh_TreeNode(
                NULL) {
    std::set<std::string>::const_iterator it;
    std::vector<std::string> split_vec;
    for (it = dq.dq_DesginIds->begin(); it != dq.dq_DesginIds->end(); it++) {

        boost::split(split_vec, *it, boost::is_any_of("/"),
                boost::token_compress_on);

        PtpanTree::PtpanEntryMap::const_iterator pit = map.find(
                split_vec[0].data());
        if (pit != map.end()) {
            // for info: "it->second - 1 == pe->pe_Num", so we don't need anything more!
            ULONG id = pit->second - 1;
            dqh_MarkedEntryNumbers.insert(id);
            if (dqh_handleFeatures && split_vec.size() > 1) {
                PTPanEntry *entry = entries_direct_map[id];
                if (entry) {
                    if (entry->pe_FeatureContainer->hasFeature(
                            split_vec[1].data())) {
                        //#ifdef DEBUG
                        //                        printf("%s feature: %s\n", entry->pe_Name, split_vec[1].data());
                        //#endif
                        dqh_ptpanFeatureMap.insert(
                                std::make_pair(
                                        id,
                                        const_cast<PTPanFeature*>(entry->pe_FeatureContainer->getFeature(
                                                split_vec[1].data()))));
                    }
                }
            }
        }
    }
    // if there are no features marked, turn off
    if (dqh_ptpanFeatureMap.empty()) {
#ifdef DEBUG
        printf("Deactivate feature checking as no are marked\n");
#endif
        dqh_handleFeatures = false;
    }
#ifdef DEBUG
    printf("Marked entries: %ld\n", dqh_MarkedEntryNumbers.size());
    printf("Marked features: %ld\n", dqh_ptpanFeatureMap.size());
#endif
}

/*!
 * \brief Destructor
 */
PtpanTree::DesignQueryHandle::~DesignQueryHandle() {
    try {
        if (dqh_TempHit) {
            delete dqh_TempHit;
        }
    } catch (...) {
        // destructor must not throw
    }
}

/*!
 * \brief Private constructor
 */
PtpanTree::DesignQueryHandle::DesignQueryHandle() :
        dqh_MarkedEntryNumbers(), dqh_Hits(), dqh_handleFeatures(false), dqh_ptpanFeatureMap(), dqh_TempHit(
                NULL), dqh_TreeNode(NULL) {
}

/*!
 * \brief Copy constructor
 *
 * \warning does not copy already present hits!
 */
PtpanTree::DesignQueryHandle::DesignQueryHandle(const DesignQueryHandle& dqh) :
        dqh_MarkedEntryNumbers(dqh.dqh_MarkedEntryNumbers), dqh_Hits(
                dqh.dqh_Hits), dqh_handleFeatures(dqh.dqh_handleFeatures), dqh_ptpanFeatureMap(
                dqh.dqh_ptpanFeatureMap), dqh_TempHit(NULL), dqh_TreeNode(NULL) {
}

/*!
 * \brief Private assignment operator
 *
 * \warning does not copy already present hits!
 */
PtpanTree::DesignQueryHandle& PtpanTree::DesignQueryHandle::operator=(
        const DesignQueryHandle& dqh) {
    if (this != &dqh) {
        dqh_MarkedEntryNumbers = dqh.dqh_MarkedEntryNumbers;
        dqh_Hits = dqh.dqh_Hits;
        dqh_handleFeatures = dqh.dqh_handleFeatures;
        dqh_ptpanFeatureMap = dqh.dqh_ptpanFeatureMap;
    }
    return *this;
}

/*!
 * \brief Check if QueryHit hits any feature
 *
 * \param qh
 *
 * \return bool TODO FIXME switch to PTPanFeature*
 */
bool PtpanTree::DesignQueryHandle::hitsAnyFeature(ULLONG abspos,
        PTPanEntry* entry, const DesignQuery& dq) const {

    boost::unordered_multimap<ULONG, PTPanFeature*>::const_iterator start, end;
    boost::tuples::tie(start, end) = dqh_ptpanFeatureMap.equal_range(
            entry->pe_Num);
    ULONG pos = 0;
    while (start != end) {
        pos = abspos - entry->pe_AbsOffset;
        if (start->second->inSameRange(pos, dq.dq_ProbeLength)) {
            return true; // TODO FIXME return PTPanFeature* ??
        }
        ++start;
    }
    return false; // TODO FIXME return NULL ??
}

/*!
 * \brief Check if QueryHit hits any feature
 *
 * \param qh
 *
 * \return PTPanFeature*
 */
const PTPanFeature* PtpanTree::DesignQueryHandle::hitsAnyFeatureReturn(
        ULLONG abspos, PTPanEntry* entry, const DesignQuery& dq) const {

    boost::unordered_multimap<ULONG, PTPanFeature*>::const_iterator start, end;
    boost::tuples::tie(start, end) = dqh_ptpanFeatureMap.equal_range(
            entry->pe_Num);
    ULONG pos = 0;
    while (start != end) {
        pos = abspos - entry->pe_AbsOffset;
        if (start->second->inSameRange(pos, dq.dq_ProbeLength)) {
            return start->second;
        }
        ++start;
    }
    return NULL;
}

/*!
 * \brief Destructor
 */
PtpanTree::SimilaritySearchQueryHandle::~SimilaritySearchQueryHandle() {
    if (ssqh_filtered_seq) {
        free(ssqh_filtered_seq);
    }
    delete[] ssqh_hit_count;
    delete[] ssqh_rel_hit_count;
}

/*!
 * \brief Private constructor
 */
PtpanTree::SimilaritySearchQueryHandle::SimilaritySearchQueryHandle(
        ULONG entry_count) :
        ssqh_Hits(), ssqh_filtered_seq(NULL), ssqh_filtered_seq_len(0), ssqh_entry_count(
                entry_count), ssqh_hit_count(NULL), ssqh_rel_hit_count(NULL) {
    ssqh_hit_count = new boost::atomic_ulong[ssqh_entry_count];
    ssqh_rel_hit_count = new double[ssqh_entry_count];
    for (ULONG cnt = 0; cnt < ssqh_entry_count; cnt++) {
        ssqh_hit_count[cnt].store(0);
        ssqh_rel_hit_count[cnt] = 0.0;
    }
}

/*!
 * \brief Private constructor
 */
PtpanTree::SimilaritySearchQueryHandle::SimilaritySearchQueryHandle() :
        ssqh_Hits(), ssqh_filtered_seq(NULL), ssqh_filtered_seq_len(0), ssqh_entry_count(
                0), ssqh_hit_count(NULL), ssqh_rel_hit_count(NULL) {
}

/*!
 * \brief Copy constructor
 *
 * \warning does not copy already present hits!
 */
PtpanTree::SimilaritySearchQueryHandle::SimilaritySearchQueryHandle(
        const SimilaritySearchQueryHandle& /*ssqh*/) :
        ssqh_Hits(), ssqh_filtered_seq(NULL), ssqh_filtered_seq_len(0), ssqh_entry_count(
                0), ssqh_hit_count(NULL), ssqh_rel_hit_count(NULL) {
}

/*!
 * \brief Private assignment operator
 *
 * \warning does not copy already present hits!
 */
PtpanTree::SimilaritySearchQueryHandle& PtpanTree::SimilaritySearchQueryHandle::operator=(
        const SimilaritySearchQueryHandle& ssqh) {
    if (this != &ssqh) {
        // nothing at the moment
    }
    return *this;
}

/*!
 * \brief Return custom information
 *
 * \return const std::string
 */
const std::string PtpanTree::getCustomInformation() const {
    return m_custom_info;
}

/*!
 * \brief Return number of entries in index
 *
 * \return ULONG
 */
ULONG PtpanTree::countEntries() const {
    return m_entry_count;
}

/*!
 * \brief Returns true if index contains entry with given id
 *
 * \param id entry id as c-string
 *
 * \return bool
 */
bool PtpanTree::containsEntry(const char *id) const {
    return containsEntry(std::string(id));
}

/*!
 * \brief Returns true if index contains entry with given id
 *
 * \param id entry id as std-string
 *
 * \return bool
 */
bool PtpanTree::containsEntry(const std::string& id) const {

    if (m_contains_features) {
        // split id at '/' and try to find the parts!
        // TODO FIXME better take '#' as delimiter, split and take first and second value of vector!
        std::size_t pos = id.find('/');
        if (pos != std::string::npos) {
            std::string entry = id.substr(0, pos);
            std::string feature = id.substr(pos + 1);
            //#ifdef DEBUG
            //            printf("SPLITTE: '%s'--'%s'\n", entry.data(), feature.data());
            //#endif
            // NUMBER OF ENTRY: it->second - 1
            PtpanEntryMap::const_iterator it = m_entry_map.find(entry.data());
            if (it != m_entry_map.end()) {
                PTPanEntry* ptpan_entry = m_EntriesMap[(it->second) - 1];
                if (ptpan_entry) {
                    return ptpan_entry->pe_FeatureContainer->hasFeature(
                            feature.data());
                }
            }
        } else {
            // no feature(s), but maybe whole genomes!!
            return m_entry_map.find(id.data()) != m_entry_map.end();
        }
    } else {
        return m_entry_map.find(id.data()) != m_entry_map.end();
    }
    return false;
}

/*!
 * \brief Return true if a reference entry exists
 *
 * \return bool
 */
bool PtpanTree::containsReferenceEntry() const {
    if (m_ref_entry) {
        return true;
    }
    return false;
}

/*!
 * \brief Return PTPanReferenceEntry
 *
 * \warning Do not free! PtpanTree is still owner! Does not return a copy!
 *
 * May return NULL if not available!
 */
const PTPanReferenceEntry* PtpanTree::getReferenceEntry() const {
    if (m_ref_entry) {
        return m_ref_entry;
    }
    return NULL;
}

/*!
 * \brief Return flag if this index contains features or not
 *
 * E.g. a genome based index may contain gene information
 *
 * \return bool
 */
bool PtpanTree::containsFeatures() const {
    return m_contains_features;
}

/*!
 * \brief Return pointer to first PTPanEntry in list
 *
 * \warning Do not free! PtpanTree is still owner! Does not return a copy!
 *
 * \return struct PTPanEntry*
 */
const PTPanEntry* PtpanTree::getFirstEntry() const {
    if (m_entries) {
        return (struct PTPanEntry*) m_entries->lh_Head;
    }
    return NULL;
}

/*!
 * \brief Return PTPanEntry
 *
 * \warning Do not free! PtpanTree is still owner! Does not return a copy!
 *
 * May return NULL if not available!
 */
const PTPanEntry* PtpanTree::getEntry(ULONG id) const {
    if (m_EntriesMap) {
        return m_EntriesMap[id];
    }
    return NULL;
}

/*!
 * \brief Return PTPanEntry for given id
 *
 * \warning Do not free! PtpanTree is still owner! Does not return a copy!
 *
 * May return NULL if not available!
 */
const PTPanEntry* PtpanTree::getEntry(const std::string& id) const {

    if (m_contains_features) {
        // split id at '/' and try to find the parts!
        // TODO FIXME better take '#' as delimiter, split and take first and second value of vector!
        std::size_t pos = id.find('/');
        if (pos != std::string::npos) {
            std::string entry = id.substr(0, pos);
            // NUMBER OF ENTRY: it->second - 1
            PtpanEntryMap::const_iterator it = m_entry_map.find(entry.data());
            if (it != m_entry_map.end()) {
                PTPanEntry* ptpan_entry = m_EntriesMap[(it->second) - 1];
                if (ptpan_entry) {
                    return ptpan_entry;
                }
            }
        } else {
            // no feature(s), but maybe whole genomes!!
            PtpanEntryMap::const_iterator it = m_entry_map.find(id.data());
            if (it != m_entry_map.end()) {
                PTPanEntry* ptpan_entry = m_EntriesMap[(it->second) - 1];
                if (ptpan_entry) {
                    return ptpan_entry;
                }
            }
        }
    } else {
        PtpanEntryMap::const_iterator it = m_entry_map.find(id.data());
        if (it != m_entry_map.end()) {
            PTPanEntry* ptpan_entry = m_EntriesMap[(it->second) - 1];
            if (ptpan_entry) {
                return ptpan_entry;
            }
        }
    }
    return NULL;
}

/*!
 * \brief Return full name of entry
 *
 * \warning Do not free! PtpanTree is still owner! Does not return a copy!
 *
 * May return NULL if not available!
 */
CONST_STRPTR PtpanTree::getEntryFullName(ULONG id) const {
    if (m_EntriesMap) {
        return m_EntriesMap[id]->pe_FullName;
    }
    return NULL;
}

/*!
 * \brief Return (string) id of entry
 *
 * \warning Do not free! PtpanTree is still owner! Does not return a copy!
 *
 * May return NULL if not available!
 */
CONST_STRPTR PtpanTree::getEntryId(ULONG id) const {
    if (m_EntriesMap) {
        return (m_EntriesMap[id]->pe_Name);
    }
    return NULL;
}

/*!
 * \brief Returns all entry ids as strings
 *
 * \return const std::vector<std::string>
 */
const std::vector<std::string> PtpanTree::getAllEntryIds() const {
    std::vector<std::string> ret_values;
    struct PTPanEntry* ps = (struct PTPanEntry *) m_entries->lh_Head;
    while (ps->pe_Node.ln_Succ) {
        ret_values.push_back(ps->pe_Name);
        ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
    }
    return ret_values;
}

/*!
 * \brief Returns all entries
 *
 * \return const std::vector<PTPanEntry*>
 */
const std::vector<PTPanEntry*> PtpanTree::getAllEntries() const {
    std::vector<PTPanEntry*> ret_values;
    struct PTPanEntry* ps = (struct PTPanEntry *) m_entries->lh_Head;
    while (ps->pe_Node.ln_Succ) {
        ret_values.push_back(ps);
        ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
    }
    return ret_values;
}

/*!
 * \brief Returns the prune length of the ptpan index
 *
 * \return ULONG
 */
ULONG PtpanTree::getPruneLength() const {
    return m_prune_length;
}

/*!
 * \brief Return pointer to alphabet specifics of index
 *
 * \return AbstractAlphabetSpecifics*
 */
const AbstractAlphabetSpecifics* PtpanTree::getAlphabetSpecifics() const {
    return m_as;
}

/*!
 * \brief Search tree for hits matching given settings in SearchQuery
 *
 * \param sq SearchQuery
 * \param sort
 * \param build_diff Build differential alignment if set to true
 *
 * \return const boost::ptr_vector<QueryHit>
 */
const boost::ptr_vector<QueryHit> PtpanTree::searchTree(const SearchQuery& sq,
        bool sort, bool build_diff) const {
    boost::lock_guard<boost::mutex> lock(m_pool_mutex);

    boost::ptr_vector<QueryHit> ret_values;
    if (!sq.sq_Query.empty()) {
        SearchQuery internal_sq = sq;
        bool reversed_too = internal_sq.sq_Reversed;
        if (internal_sq.sq_MaxErrors == 0.0) {
            internal_sq.sq_AllowInsert = false;
            internal_sq.sq_AllowDelete = false;
            internal_sq.sq_AllowReplace = false;
        }
        internal_sq.sq_Reversed = false;
        SearchQueryHandle sqh = SearchQueryHandle(internal_sq);

        std::vector<PTPanPartition *>::const_iterator pit;
        for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
            search_partition(*pit, internal_sq, sqh);
        }
        post_filter_query_hits(sq, sqh);

#ifdef DEBUG
        printf("after all partitions count %lu\n", sqh.sqh_hits.size());
        printf("depth: \t%lu\n", m_prune_length);
        printf("query-length: \t%lu\n", internal_sq.sq_Query.size());
        printf("error count: %2.1f\n", internal_sq.sq_MaxErrors);
#endif
        if (m_prune_length < sqh.sqh_MaxLength) {
#ifdef DEBUG
            printf("may contain UNSAFE! %lu\n", sqh.sqh_MaxLength);
#endif
            validate_unsafe_hits(internal_sq, sqh);
        }
        if (build_diff) {
            if (!sqh.sqh_hits.empty()) {
#ifdef DEBUG
                printf(">> CreateDiffAlignment for %lu hits\n",
                        sqh.sqh_hits.size());
#endif
                if (sqh.sqh_hits.size() < m_num_threads || m_num_threads <= 1) {
                    create_diff_alignment(internal_sq, sqh);
                } else {
                    std::size_t i = 0;
                    ULONG participators = m_num_threads;
                    for (; i < participators - 1; i++) {
                        boost::threadpool::schedule(
                                *m_threadpool,
                                boost::bind(&PtpanTree::create_diff_alignment,
                                        this, boost::ref(internal_sq),
                                        boost::ref(sqh), i, participators));
                    }
                    create_diff_alignment(internal_sq, sqh, i, participators); // main thread can do some work as well!
                    m_threadpool->wait();
                }
            }
        }

        if (reversed_too) {
            STRPTR query_str = const_cast<char *>(internal_sq.sq_Query.c_str());
            m_as->complementSequence(query_str);
            m_as->reverseSequence(query_str);
            internal_sq.sq_Reversed = true;

            SearchQueryHandle sqh_rev = SearchQueryHandle(internal_sq);
#ifdef DEBUG
            printf("check reverse complement as well (%s)\n",
                    internal_sq.sq_Query.c_str());
#endif
            // TODO FIXME double code is bad, but there is no time left :-(
            std::vector<PTPanPartition *>::const_iterator pit;
            for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
                search_partition(*pit, internal_sq, sqh_rev);
            }
            post_filter_query_hits(sq, sqh_rev);

#ifdef DEBUG
            printf("after all partitions rev-compl count %lu\n",
                    sqh_rev.sqh_hits.size());
#endif
            if (m_prune_length < sqh_rev.sqh_MaxLength) {
#ifdef DEBUG
                printf("may contain UNSAFE! %lu\n", sqh_rev.sqh_MaxLength);
#endif
                validate_unsafe_hits(internal_sq, sqh_rev);
            }
            if (build_diff) {
                if (!sqh_rev.sqh_hits.empty()) {
#ifdef DEBUG
                    printf(">> CreateDiffAlignment for %lu hits\n",
                            sqh_rev.sqh_hits.size());
#endif
                    if (sqh_rev.sqh_hits.size() < m_num_threads
                            || m_num_threads <= 1) {
                        create_diff_alignment(internal_sq, sqh_rev);
                    } else {
                        std::size_t i = 0;
                        ULONG participators = m_num_threads;
                        for (; i < participators - 1; i++) {
                            boost::threadpool::schedule(
                                    *m_threadpool,
                                    boost::bind(
                                            &PtpanTree::create_diff_alignment,
                                            this, boost::ref(internal_sq),
                                            boost::ref(sqh_rev), i,
                                            participators));
                        }
                        create_diff_alignment(internal_sq, sqh_rev, i,
                                participators); // main thread can do some work as well!
                        m_threadpool->wait();
                    }
                }
            }
            // we want to transfer control over results to sqh
            // before we sort and return results
            sqh.sqh_hits.transfer(sqh.sqh_hits.end(), sqh_rev.sqh_hits);
        }

        if (sort) {
            sort_hits_list(internal_sq, sqh);
        }
        // now swap to get results into return
        ret_values.swap(sqh.sqh_hits);
    }
    return ret_values;
}

/*!
 * \brief Preferred method to get all probes of a certain length
 *
 * If given length is longer than prune length, Probes are returned for
 * length equal prune length!
 *
 * \param length Length of Probes to return
 *
 * \return std::vector<std::string>
 */
const std::vector<std::string> PtpanTree::getAllProbes(ULONG length) const {
    std::vector<std::string> ret_values;
    if (length <= m_prune_length) {
        struct TreeNode *root = NULL;
        std::vector<PTPanPartition *>::const_iterator pit;
        for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
            root = read_packed_node(*pit, 0); // get root node
            find_probes_rec(root, ret_values, 0, length);
            TreeNode::free_tree_node(root);
        }
    } else {
        printf(
                "ERROR: probe length is longer than prune length! (%lu > %lu)!\n",
                length, m_prune_length);
    }
    return ret_values;
}

/*!
 * \brief LEGACY method!! Generate probes of given length up to given number
 *
 * \deprecated avoid usage as this method may be removed in a future version
 *
 * \warning You may want to use getAllProbes(ULONG length) method instead!
 *
 * If seed probe is empty, probes are generated from scratch!
 *
 * \param seed_probe Seed probe as starting point
 * \param length Length of probe (if > prune length -> prune length is taken!)
 * \param num_probes Maximum number of probes to return
 *
 * \return char* Pointer to 0-terminated c-string (must be freed by caller!)
 */
STRPTR PtpanTree::getProbes(STRPTR seed_probe, ULONG length,
        ULONG num_probes) const {
    struct TreeNode *tn;
    struct TreeNode *parenttn;
    struct PTPanPartition *best_pp = NULL;
    UWORD partition_count = 0;

    if (seed_probe) {
        // find correct path or 'nearest' one!
        bool found_perfect = false;
        ULONG seed_length = strlen(seed_probe);
        if (m_partitions.size() > 1) {
            ULONG *compressed_seed = m_as->compressSequence(seed_probe);

            ULONG first_characters;
            if (seed_length >= m_as->max_code_fit_long) {
                first_characters = *compressed_seed
                        >> m_as->bits_shift_table[m_as->max_code_fit_long];
            } else {
                first_characters = *compressed_seed
                        >> m_as->bits_shift_table[seed_length];
            }
            free(compressed_seed);

            std::vector<PTPanPartition *>::const_iterator pit;
            for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
                best_pp = *pit;
                // shift compressed sequences to correct position (the longer one!)
                ULONG shifted_seed = first_characters;
                ULONG shifted_prefix = (*pit)->pp_Prefix;
                if ((*pit)->pp_PrefixLen > seed_length) {
                    shifted_prefix = shifted_prefix
                            / m_as->power_table[((*pit)->pp_PrefixLen
                                    - seed_length)];
                } else if ((*pit)->pp_PrefixLen < seed_length) {
                    shifted_seed = shifted_seed
                            / m_as->power_table[(seed_length
                                    - (*pit)->pp_PrefixLen)];
                }
                // ... and compare them
                if (shifted_seed == shifted_prefix) {
                    // fine, the seed matches the prefix of the partition!
                    if ((*pit)->pp_PrefixLen < seed_length) {
                        // we only need to examin further if seed-length is longer
                        // than the partition prefix
                        found_perfect = true;
                    }
                    break;
                } else if (shifted_seed > shifted_prefix) {
                    // we're not equal, so we take partition with lower prefix as
                    // best partition!
                    break;
                }
                // if we haven't found it, increase count!
                partition_count++;
            }
        } else {
            found_perfect = true;
        }
        // start at root node
        tn = read_packed_node(best_pp, 0);
        // there are more characters to compare left ...
        if (found_perfect) {
            // prefix matches, so skip some characters
            STRPTR seed_current = seed_probe + best_pp->pp_PrefixLen;
            ULONG seed_remain = seed_length - best_pp->pp_PrefixLen;
            // find correct partition and treenode (if existing), otherwise
            // nearest neighbor!
            bool exists = false;
            UBYTE next_seq_code;
            while (!exists) {
                next_seq_code = m_as->compress_table[(UBYTE) seed_current[0]];
                if (tn->tn_Children && tn->tn_Children[next_seq_code]) {
                    tn = go_down_node_child(tn, next_seq_code);
                    seed_current++; // first character matches!
                    // check edge
                    bool edge_matches = true;
                    UWORD edge_code;
                    for (LONG i = 1; i < tn->tn_EdgeLen && (seed_remain > 0);
                            i++) {
                        next_seq_code =
                                m_as->compress_table[(UBYTE) seed_current[0]];
                        edge_code =
                                m_as->compress_table[(UBYTE) tn->tn_Edge[i]];
                        seed_current++;
                        seed_remain--;
                        if (edge_code != next_seq_code) {
                            edge_matches = false;
                            break;
                        }
                    }
                    if (edge_matches) {
                        if (!seed_remain || tn->tn_TreeOffset >= length) {
                            exists = true;
                        }
                    } else {
                        // didn't match, so reset to last treenode matching
                        parenttn = tn->tn_Parent;
                        TreeNode::free_tree_node(tn);
                        tn = parenttn;
                    }
                }
            }
        }
    } else {
        best_pp = m_partitions.at(0);
        partition_count = 0;
        // no seed probe, so start with root of first partition!
        tn = read_packed_node(best_pp, 0);
    }
    // collect all the strings that are on our way
    bool done = false;
    UWORD seqcode = 0;
    ULONG length_corr =
            (length < best_pp->pp_TreePruneLength) ?
                    length : best_pp->pp_TreePruneLength;

    ULONG buffer_size;
    ULONG buffer_consumed = 0;
    if (num_probes > 0) {
        buffer_size = num_probes * (length_corr + 1) + 1;
    } else {
        buffer_size = 100 * (length_corr + 1) + 1;
    }
    // allocate output string (don't forget the ';'!)
    STRPTR outptr = (STRPTR) malloc(buffer_size);
    STRPTR collect_ptr = outptr;

    for (ULONG cnt = 0; cnt < num_probes || (num_probes == 0); cnt++) {
        //printf("Cnt: %ld\n", cnt);
        /* go down! */
        while (tn->tn_TreeOffset < length_corr) {
            while (seqcode < m_as->alphabet_size) {
                //printf("Seqcode %d %ld\n", seqcode, tn->tn_Children[seqcode]);
                if (tn->tn_Children[seqcode]) {
                    /* there is a child, go down */
                    tn = go_down_node_child(tn, seqcode);
                    //printf("Down %d %08lx\n", seqcode, tn);
                    seqcode = 0;
                    break;
                }
                seqcode++;
            }

            while (seqcode == m_as->alphabet_size) { // we didn't find any children
                /* go up again */
                //printf("Up\n");
                parenttn = tn->tn_Parent;
                seqcode = tn->tn_ParentSeq + 1;
                TreeNode::free_tree_node(tn);
                tn = parenttn;
                if (!tn) {
                    // we're done with this partition as we reached the root node!
                    if (partition_count + 1 < (UWORD) m_partitions.size()) {
                        partition_count++;
                        // next partition
                        best_pp = m_partitions.at(partition_count);
                        // get root node
                        tn = read_packed_node(best_pp, 0);
                    } else {
                        done = true;
                        break; /* we're done! */
                    }
                }
            }
            if (done) {
                break;
            }
        }
        if (done) {
            break;
        }

        // TODO FIXME use ostringstream instead!
        //        if (!first) {
        //            *collect_ptr++ = ';';
        //            buffer_consumed++;
        //        } else {
        //            first = false;
        //        }
        //        get_tree_path(tn, collect_ptr, length_corr);
        //        collect_ptr += length_corr;
        //        buffer_consumed += length_corr;
        //        // if necessary, enlarge buffer!
        //        if ((buffer_consumed + length_corr + 1) > buffer_size) {
        //            outptr = (STRPTR) realloc(outptr, 2 * buffer_consumed);
        //            collect_ptr = outptr + buffer_consumed;
        //        }

        parenttn = tn->tn_Parent;
        seqcode = tn->tn_ParentSeq + 1;
        TreeNode::free_tree_node(tn);
        tn = parenttn;
    }
    // clean up nodes!
    while (tn->tn_Parent) {
        parenttn = tn->tn_Parent;
        TreeNode::free_tree_node(tn);
        tn = parenttn;
    }
    TreeNode::free_tree_node(tn);
    // finally add terminating '0' to output
    *collect_ptr = 0;
    outptr = (STRPTR) realloc(outptr, buffer_consumed);
    return outptr;
}

/*!
 * \brief Design probes
 *
 * \param dq DesignQuery
 *
 * \return const boost::ptr_vector<DesignHit>
 *
 * \exception std::invalid_argument Thrown in case of a improper DesignQuery
 * \exception std::runtime_error Thrown in case of something unexpected
 */
const boost::ptr_vector<DesignHit> PtpanTree::designProbes(
        const DesignQuery& dq) const {
    boost::ptr_vector<DesignHit> ret_values;
    if (!dq.dq_DesginIds->empty()) {

        DesignQueryHandle dqh = DesignQueryHandle(dq, m_entry_map, m_EntriesMap,
                m_contains_features);

#ifdef DEBUG
        if (m_contains_features) {
            // TODO FIXME handle feature ids!!
            std::set<std::string>::const_iterator it;
            for (it = dq.dq_DesginIds->begin(); it != dq.dq_DesginIds->end();
                    it++) {
                printf("%s\n", it->data());
            }
        }
#endif

        // first find probe candidates!
        std::vector<PTPanPartition *>::const_iterator pit;
        for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
            find_probe_in_partition(*pit, dq, dqh);
        }

#ifdef DEBUG
        printf("%ld probes generated...\n", dqh.dqh_Hits.size());
        printf("Calc Probe Quality\n");
#endif
// TODO get parallel code running        if (dqh.dqh_Hits.size() < m_num_threads || m_num_threads <= 1) {
        design_probes(dq, dqh);
//        } else {
//            std::size_t i = 0;
//            ULONG participators = m_num_threads;
//            for (; i < participators - 1; i++) {
//                boost::threadpool::schedule(
//                        *m_threadpool,
//                        boost::bind(&PtpanTree::design_probes, this,
//                                boost::ref(dq), boost::ref(dqh), i,
//                                participators));
//            }
//            design_probes(dq, dqh, i, participators); // main thread can do some work as well!
//            m_threadpool->wait();
//        }
        // remove invalid design hits! due to parallelization, we cannot do this before!
        boost::ptr_vector<DesignHit>::iterator dh;
        for (dh = dqh.dqh_Hits.begin(); dh != dqh.dqh_Hits.end();) {
            if (dh->dh_valid) {
                dh++;
            } else {
                dh = dqh.dqh_Hits.erase(dh);
            }
        }
        // quality calculation ...
        if (dqh.dqh_Hits.size() < m_num_threads || m_num_threads <= 1) {
            calculate_probe_quality(dqh);
        } else {
            std::size_t i = 0;
            ULONG participators = m_num_threads;
            for (; i < participators - 1; i++) {
                boost::threadpool::schedule(
                        *m_threadpool,
                        boost::bind(&PtpanTree::calculate_probe_quality, this,
                                boost::ref(dqh), i, participators));
            }
            calculate_probe_quality(dqh, i, participators); // main thread can do some work as well!
            m_threadpool->wait();
        }
        // sort hits ...
        switch (dq.dq_SortMode) {
        case 0:
            // sort by quality value
            dqh.dqh_Hits.sort(DesignHit::qualityCompare);
            break;
        case 1: {
            // compare sequences
            for (dh = dqh.dqh_Hits.begin(); dh != dqh.dqh_Hits.end(); dh++) {
                // we assume values lower length 27 at the moment!
                ULONG *value = m_as->compressSequence(dh->dh_ProbeSeq.data());
                dh->dh_sortKey = *value;
                free(value);
            }
            dqh.dqh_Hits.sort(DesignHit::keyCompare);
            break;
        }
        default:
            break;
        }

        // clip results ...
        ULONG i = 0;
        for (dh = dqh.dqh_Hits.begin();
                dh != dqh.dqh_Hits.end() && i < dq.dq_MaxHits; dh++) {
            i++;
        }
        if (dh != dqh.dqh_Hits.end()) {
            dqh.dqh_Hits.erase(dh, dqh.dqh_Hits.end());
        }
        ret_values.swap(dqh.dqh_Hits);
    }
    return ret_values;
}

/*!
 * \brief Returns the step width for the non-group hits
 * for increasing distance
 *
 * \return double
 */
double PtpanTree::probeNongroupIncreaseWMis() const {
    return PROBE_MISM_DEC;
}

/*!
 * \brief Returns the array width of the non-group hits
 * array (i.e. number of entries)
 *
 * \return ULONG
 */
ULONG PtpanTree::probeNongroupIncreaseArrayWidth() const {
    return DECR_TEMP_CNT;
}

/*!
 * \brief SimilaritySearch for given SimilaritySearchQuery
 *
 * \param ssq SimilaritySearchQuery
 */
const boost::ptr_vector<SimilaritySearchHit> PtpanTree::similaritySearch(
        const SimilaritySearchQuery& ssq) const {
#ifdef DEBUG
    printf("EXTERN: SimilaritySearch()\n");
#endif
    SimilaritySearchQueryHandle ssqh = SimilaritySearchQueryHandle(
            m_entry_count);

    boost::ptr_vector<SimilaritySearchHit> ret_values;
    if (!ssq.ssq_SourceSeq.empty()) {
        try {
            ssqh.ssqh_filtered_seq = (STRPTR) calloc(
                    ssq.ssq_SourceSeq.size() + 1, 1);
            ssqh.ssqh_filtered_seq_len = m_as->filterSequenceTo(
                    ssq.ssq_SourceSeq.data(), ssqh.ssqh_filtered_seq);
            // TODO TEST
            // ULONG* compr_seq = compressSequence(ssq.ssq_SourceSeq.data());
            // DO NOT COMPRESS whole sequence, but only window-sized snippets
            // filter Ns?
            // regain rev, compl and rev-compl during decompression
            //            printf("Length of filtered sequence: %ld\n",
            //                    ssqh.ssqh_filtered_seq_len);
            //
            //            std::map<ULONG, ULONG> simsearch_set;
            //            ULONG pval = 0;
            //
            //            // init pval with (ssq.ssq_WindowSize - 1) values
            //            ULONG i;
            //            for (i = 0; i < ssq.ssq_WindowSize - 1; i++) {
            //                pval *= m_as->alphabet_size;
            //                pval += m_as->compress_table[ssqh.ssqh_filtered_seq[i]];
            //            }
            //            std::map<ULONG, ULONG>::iterator it;
            //            for (; i < ssqh.ssqh_filtered_seq_len - ssq.ssq_WindowSize; i++) {
            //
            //                // sliding window:
            //                pval %= m_as->power_table[ssq.ssq_WindowSize - 1];
            //                pval *= m_as->alphabet_size;
            //                pval += m_as->compress_table[ssqh.ssqh_filtered_seq[i]];
            //
            //                it = simsearch_set.find(pval);
            //                if (it != simsearch_set.end()) {
            //                    it->second++;
            //                } else {
            //                    simsearch_set.insert(std::make_pair(pval, 1));
            //                }
            //            }
            //            printf("Number of unique window probes: %ld\n",
            //                    simsearch_set.size());

            if (ssq.ssq_ComplementMode & (1 << 0)) { // pf_ComplementMode & 0x01 == 0x01 // forward
                ss_window_sequence(ssq, ssqh);
            }

            m_as->reverseSequence(ssqh.ssqh_filtered_seq); // Sequence is now reversed
            if (ssq.ssq_ComplementMode & (1 << 1)) { // 0x02 // reverse
                ss_window_sequence(ssq, ssqh);
            }

            m_as->complementSequence(ssqh.ssqh_filtered_seq); // Sequence is now reversed and complemented
            if (ssq.ssq_ComplementMode & (1 << 2)) { // 0x04 // reverse-complement
                ss_window_sequence(ssq, ssqh);
            }

            m_as->reverseSequence(ssqh.ssqh_filtered_seq); // Sequence is now complemented
            if (ssq.ssq_ComplementMode & (1 << 3)) { // 0x08 // complement
                ss_window_sequence(ssq, ssqh);
            }

            ss_create_hits(ssq, ssqh);
            if (ssq.ssq_SortType == 0) {
                // sort by absolute hits
                ssqh.ssqh_Hits.sort(SimilaritySearchHit::greaterThan);
            } else {
                // sort by relative hits
                ssqh.ssqh_Hits.sort(SimilaritySearchHit::greaterThanRel);
            }

        } catch (...) {
            throw std::runtime_error(
                    "PtpanTree::SimilaritySearch() unexpected error!");
        }
        ret_values.swap(ssqh.ssqh_Hits);
    }
    return ret_values;
}

/*!
 * \brief Load index header from given file ...
 */
void PtpanTree::loadIndexHeader() {
#ifdef DEBUG
    printf("Load index header ...\n");
#endif

    FILE *fh;
    if (!(fh = fopen(m_index_name.data(), "r"))) {
        std::invalid_argument(
                "PtpanTree::loadIndexHeader() Could not open index header file!");
    }
    fseek(fh, 0L, SEEK_END);
    m_map_file_size = ftell(fh);
    fseek(fh, 0L, SEEK_SET);
    m_map_file_buffer = (UBYTE *) mmap(0, m_map_file_size, PROT_READ,
            MAP_SHARED, fileno(fh), 0);
    if (!m_map_file_buffer) {
        printf("PtpanTree::loadIndexHeader() ERROR mapping index header.\n");
    }

    // read id string
    char idstr[16];
    std::size_t dummy = fread(idstr, 16, 1, fh);
    if (strncmp("TUM PeTerPAN IDX", idstr, 16)) {
        fclose(fh);
        std::invalid_argument(
                "PtpanTree::loadIndexHeader() This is no index file!");
    }
    // check endianness
    ULONG endian = 0;
    dummy = fread(&endian, sizeof(endian), 1, fh);
    if (endian != 0x01020304) {
        fclose(fh);
        std::invalid_argument(
                "PtpanTree::loadIndexHeader() Index was created on a different endian machine!");
    }
    // check file structure version
    UWORD version = 0;
    dummy = fread(&version, sizeof(version), 1, fh);
    if (version != FILESTRUCTVERSION) {
        printf(
                "ERROR: Index (V%d.%d) does not match current file structure version (V%d.%d)!\n",
                version >> 8, version & 0xff, FILESTRUCTVERSION >> 8,
                FILESTRUCTVERSION & 0xff);
        fclose(fh);
        std::invalid_argument(
                "PtpanTree::loadIndexHeader() Index does not match current file structure version!");
    }

    // read the rest of the important data
    UWORD alphabet_type;
    dummy = fread(&alphabet_type, sizeof(alphabet_type), 1, fh);
    // init correct alphabet specifics
    switch (AbstractAlphabetSpecifics::intAsType(alphabet_type)) {
    case AbstractAlphabetSpecifics::DNA5:
#ifdef DEBUG
        printf("DNA5 alphabet found\n");
#endif
        // init DNA5 alphabet
        m_as = (AbstractAlphabetSpecifics *) new Dna5AlphabetSpecifics();
        break;
    default:
        throw std::runtime_error(
                "PtpanTree::loadIndexHeader() unknown alphabet type!");
    }
    dummy = fread(&m_AllHashSum, sizeof(m_AllHashSum), 1, fh);
    dummy = fread(&m_entry_count, sizeof(m_entry_count), 1, fh);
    dummy = fread(&m_prune_length, sizeof(m_prune_length), 1, fh);

    if (m_verbose) {
        printf("This index contains %ld entries!\n", m_entry_count);
    }

    UBYTE feature = '1';
    dummy = fread(&feature, sizeof(feature), 1, fh);
    if (feature == '0') {
        m_contains_features = true;
    } else {
        m_contains_features = false;
    }

    // read reference sequence
    ULONG ref_size = 0;
    dummy = fread(&ref_size, sizeof(ref_size), 1, fh);
    if (ref_size > 0) {
        m_ref_entry = (struct PTPanReferenceEntry*) malloc(
                sizeof(struct PTPanReferenceEntry));
        m_ref_entry->pre_ReferenceSeqSize = ref_size;
        m_ref_entry->pre_ReferenceSeq = (char*) malloc(
                m_ref_entry->pre_ReferenceSeqSize + 1);
        if (!m_ref_entry->pre_ReferenceSeq) {
            throw std::runtime_error(
                    "PtpanTree::loadIndexHeader() Out of memory (m_ref_entry->pre_ReferenceSeq)");
        }
        dummy = fread(m_ref_entry->pre_ReferenceSeq, 1,
                m_ref_entry->pre_ReferenceSeqSize + 1, fh);
        m_ref_entry->pre_ReferenceBaseTable = (ULONG *) calloc(
                m_ref_entry->pre_ReferenceSeqSize + 1, sizeof(ULONG));
        if (!m_ref_entry->pre_ReferenceBaseTable) {
            throw std::runtime_error(
                    "PtpanTree::loadIndexHeader() Out of memory (m_ref_entry->pre_ReferenceBaseTable)");
        }
        dummy = fread(m_ref_entry->pre_ReferenceBaseTable, sizeof(ULONG),
                m_ref_entry->pre_ReferenceSeqSize + 1, fh);
    }
    // read custom information
    ULONG size = 0;
    dummy = fread(&size, sizeof(size), 1, fh);
    if (size > 0) {
        STRPTR custom = (STRPTR) calloc(size + 1, 1);
        dummy = fread(custom, 1, size, fh);
        m_custom_info = std::string(custom);
        free(custom);
#ifdef DEBUG
        printf("Read custom info: \n'%s'\n", m_custom_info.data());
#endif
    }
    // add the species to the list
    m_EntriesMap = (struct PTPanEntry **) calloc(sizeof(struct PTPanEntry *),
            m_entry_count);
    ULONG numentry = 0;
    struct PTPanEntry *pe;
    while (numentry < m_entry_count) {
        // get name of species on disk
        UWORD len;
        dummy = fread(&len, sizeof(len), 1, fh);
        STRPTR filespname = (STRPTR) calloc(len + 1, 1);
        dummy = fread(filespname, len, 1, fh);

        dummy = fread(&len, sizeof(len), 1, fh);
        STRPTR fullname = (STRPTR) calloc(len + 1, 1);
        dummy = fread(fullname, len, 1, fh);

        // okay, cannot fail now anymore, allocate a PTPanEntry structure
        pe = (struct PTPanEntry *) calloc(1, sizeof(struct PTPanEntry));
        m_EntriesMap[numentry] = pe;
        pe->pe_Num = numentry;

        /* write name and long name into the structure */
        pe->pe_Name = filespname;
        pe->pe_FullName = fullname;

        /* load in the alignment information */
        dummy = fread(&pe->pe_SeqDataSize, sizeof(pe->pe_SeqDataSize), 1, fh);
        dummy = fread(&pe->pe_RawDataSize, sizeof(pe->pe_RawDataSize), 1, fh);
        dummy = fread(&pe->pe_AbsOffset, sizeof(pe->pe_AbsOffset), 1, fh);
        // read compressedSequenceShortcuts
        dummy = fread(&pe->pe_CompressedShortcutsCount,
                sizeof(pe->pe_CompressedShortcutsCount), 1, fh);
        pe->pe_CompressedShortcuts = (struct PTPanComprSeqShortcuts *) calloc(
                pe->pe_CompressedShortcutsCount,
                sizeof(struct PTPanComprSeqShortcuts));
        struct PTPanComprSeqShortcuts *csc;
        for (ULONG cnt = 0; cnt < pe->pe_CompressedShortcutsCount; cnt++) {
            csc =
                    (struct PTPanComprSeqShortcuts *) &pe->pe_CompressedShortcuts[cnt];
            csc->pcs_AbsPosition = read_byte(fh);
            csc->pcs_BitPosition = read_byte(fh);
            csc->pcs_RelPosition = read_byte(fh);
        }

        ULONG count = 0;
        dummy = fread(&count, sizeof(count), 1, fh);
        if (count > 0) {
            pe->pe_FeatureContainer = new PTPanFeatureContainer();

            ULONG length = 0;
            UWORD pos_count = 0;
            PTPanFeature *feature;
            for (ULONG i = 0; i < count; i++) {
                feature = PTPanFeature::createPtpanFeature();
                dummy = fread(&length, sizeof(length), 1, fh);
                STRPTR name = (STRPTR) calloc(length + 1, 1);
                dummy = fread(name, length, 1, fh);
                feature->pf_name = name;
                // get position count:
                dummy = fread(&pos_count, sizeof(pos_count), 1, fh);
                feature->pf_num_pos = pos_count;
                feature->pf_pos = PTPanPositionPair::createPtpanPositionPair(
                        pos_count);
                for (UWORD j = 0; j < pos_count; j++) {
                    feature->pf_pos[j].pop_start = read_byte(fh);
                    feature->pf_pos[j].pop_end = read_byte(fh);
                    UBYTE complement_flag;
                    dummy = fread(&complement_flag, sizeof(UBYTE), 1, fh);
                    feature->pf_pos[j].complement =
                            complement_flag == 0 ? false : true;
                }
                pe->pe_FeatureContainer->add(feature);
            }
            assert(count == pe->pe_FeatureContainer->size());
        }

        dummy = fread(&pe->pe_SeqDataCompressedSize,
                sizeof(pe->pe_SeqDataCompressedSize), 1, fh);
        ULONG pos = ftell(fh);
        pe->pe_SeqDataCompressed = &m_map_file_buffer[pos];
        if (!pe->pe_SeqDataCompressed) {
            throw std::runtime_error(
                    "PtpanTree::loadIndexHeader() Error mapping compressed SeqData (with '.' and '-')!");
        }
        fseek(fh, (pe->pe_SeqDataCompressedSize >> 3) + 1, SEEK_CUR);
        pe->pe_Node.ln_Pri = pe->pe_AbsOffset;

        /* Init complete, now add it to the list */
        //printf("Added %s ('%s')...\n", pe->pe_Name, pe->pe_FullName);
        AddTail(m_entries, &pe->pe_Node);
        numentry++;

#ifdef DEBUG
        /* visual feedback */
        if ((numentry % 20) == 0) {
            if (numentry % 1000) {
                printf(".");
                fflush(stdout);
            } else {
                printf(".%6ld (%6lld KB)\n", numentry,
                        (pe->pe_AbsOffset >> 10));
            }
        }
#endif

    } // end while (numentry < m_entry_count)

    if (numentry != m_entry_count) {
        printf("ERROR: Number of entries has changed!\n");
        fclose(fh);
        throw std::runtime_error(
                "PtpanTree::loadIndexHeader() Number of entries has changed!");
    }

    /* build tree to find entries quicker by raw position */
    m_EntriesBinTree = BuildBinTree(m_entries);

    // build a entry name hash to mark entries in groups
    m_entry_map.rehash(m_entry_count);
    pe = (struct PTPanEntry *) m_entries->lh_Head;
    while (pe->pe_Node.ln_Succ) {
        m_entry_map[pe->pe_Name] = pe->pe_Num + 1;
        pe = (struct PTPanEntry *) pe->pe_Node.ln_Succ;
    }assert(m_entry_map.size() <= m_entry_count);

    dummy = fread(&m_TotalSeqSize, sizeof(m_TotalSeqSize), 1, fh);
    dummy = fread(&m_TotalSeqCompressedSize, sizeof(m_TotalSeqCompressedSize),
            1, fh);
    dummy = fread(&m_TotalRawSize, sizeof(m_TotalRawSize), 1, fh);

    UWORD num_partitions;
    dummy = fread(&num_partitions, sizeof(num_partitions), 1, fh);

#ifdef DEBUG
    printf("\n\nDatabase contains %ld valid entries.\n"
            "%lld bytes alignment data (%lld bases).\n", m_entry_count,
            m_TotalSeqSize, m_TotalRawSize);
    printf(
            "Compressed sequence data (with dots and hyphens): %llu byte (%llu kb, %llu mb)\n",
            m_TotalSeqCompressedSize, m_TotalSeqCompressedSize >> 10,
            m_TotalSeqCompressedSize >> 20);

    printf("Number of partitions: %d\n", num_partitions);
#endif

    struct PTPanPartition *pp;
    for (UWORD cnt = 0; cnt < num_partitions; cnt++) {
        pp = (struct PTPanPartition *) calloc(1, sizeof(struct PTPanPartition));
        if (!pp) {
            fclose(fh);
            throw std::runtime_error(
                    "PtpanTree::loadIndexHeader() Out of memory (PTPanPartition)");
        }
        dummy = fread(&pp->pp_ID, sizeof(pp->pp_ID), 1, fh);
        dummy = fread(&pp->pp_Prefix, sizeof(pp->pp_Prefix), 1, fh);
        dummy = fread(&pp->pp_PrefixLen, sizeof(pp->pp_PrefixLen), 1, fh);
        dummy = fread(&pp->pp_Size, sizeof(pp->pp_Size), 1, fh);

        pp->pp_PartitionName = (STRPTR) calloc(m_index_name.size() + 6, 1);
        strncpy(pp->pp_PartitionName, m_index_name.data(),
                m_index_name.size() - 2);
        sprintf(&pp->pp_PartitionName[m_index_name.size() - 2], "t%04ld",
                pp->pp_ID);
        // generate partition prefix string
        ULONG pcnt = pp->pp_PrefixLen;
        pp->pp_PrefixSeq = (STRPTR) malloc(pcnt + 1);
        while (pcnt) {
            pp->pp_PrefixSeq[pp->pp_PrefixLen - pcnt] =
                    m_as->decompress_table[(pp->pp_Prefix
                            / m_as->power_table[pcnt - 1]) % m_as->alphabet_size];
            pcnt--;
        }
        pp->pp_PrefixSeq[pp->pp_PrefixLen] = 0;
        //printf("Part %ld (%ld = '%s')\n", pp->pp_ID, pp->pp_Prefix, pp->pp_PrefixSeq);
        m_partitions.push_back(pp);
    }
    fclose(fh);
}

/*!
 * \brief Load partitions (better: headers) ...
 */
void PtpanTree::loadPartitions() {
#ifdef DEBUG
    printf("Load all partitions\n");
#endif

    std::vector<PTPanPartition *>::const_iterator pit;
    for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
#ifdef DEBUG
        printf("Loading partition file %s... ", (*pit)->pp_PartitionName);
#endif
        FILE *fh;
        if (!(fh = fopen((*pit)->pp_PartitionName, "r"))) {
            throw std::runtime_error(
                    "PtpanTree::loadPartitions() Couldn't open partition file!");
        }
        /* read id string */
        char idstr[16];
        std::size_t dummy = fread(idstr, 16, 1, fh);
        if (strncmp("TUM PeTerPAN P3I", idstr, 16)) {
            fclose(fh);
            throw std::runtime_error(
                    "PtpanTree::loadPartitions() This is no partition file!");
        }
        ULONG hashsum;
        dummy = fread(&hashsum, sizeof(m_AllHashSum), 1, fh);
        if (hashsum != m_AllHashSum) {
            fclose(fh);
            throw std::runtime_error(
                    "PtpanTree::loadPartitions() Partition file does not match index file!");
        }
        /* read partition data */
        dummy = fread(&(*pit)->pp_ID, sizeof((*pit)->pp_ID), 1, fh);
        dummy = fread(&(*pit)->pp_Prefix, sizeof((*pit)->pp_Prefix), 1, fh);
        dummy = fread(&(*pit)->pp_PrefixLen, sizeof((*pit)->pp_PrefixLen), 1,
                fh);
        dummy = fread(&(*pit)->pp_Size, sizeof((*pit)->pp_Size), 1, fh);
        dummy = fread(&(*pit)->pp_TreePruneDepth,
                sizeof((*pit)->pp_TreePruneDepth), 1, fh);
        dummy = fread(&(*pit)->pp_TreePruneLength,
                sizeof((*pit)->pp_TreePruneLength), 1, fh);
        dummy = fread(&(*pit)->pp_LongDictSize, sizeof((*pit)->pp_LongDictSize),
                1, fh);
        dummy = fread(&(*pit)->pp_LongRelPtrBits,
                sizeof((*pit)->pp_LongRelPtrBits), 1, fh);

        /* read huffman tables */
        (*pit)->pp_BranchTree = ReadHuffmanTree(fh);
        (*pit)->pp_ShortEdgeTree = ReadHuffmanTree(fh);
        (*pit)->pp_LongEdgeLenTree = ReadHuffmanTree(fh);

        /* read compressed dictionary */
        ULONG len;
        dummy = fread(&len, sizeof(len), 1, fh);
        (*pit)->pp_LongDictRaw = (ULONG *) malloc(len);
        if ((*pit)->pp_LongDictRaw) {
            dummy = fread((*pit)->pp_LongDictRaw, len, 1, fh);
            // read compressed tree
            dummy = fread(&(*pit)->pp_DiskTreeSize,
                    sizeof((*pit)->pp_DiskTreeSize), 1, fh);

            LONG pos = ftell(fh);
            (*pit)->pp_MapFileSize = pos + (*pit)->pp_DiskTreeSize;
            (*pit)->pp_MapFileBuffer = (UBYTE *) mmap(0, (*pit)->pp_MapFileSize,
                    PROT_READ, MAP_SHARED, fileno(fh), 0);
            if ((*pit)->pp_MapFileBuffer) {
                fclose(fh);
                /* calculate start of buffer inside file */
                (*pit)->pp_DiskTree = &(*pit)->pp_MapFileBuffer[pos];
#ifdef DEBUG
                printf("VMEM!\n");
#endif
            } else {
                fclose(fh);
                FreeHuffmanTree((*pit)->pp_BranchTree);
                FreeHuffmanTree((*pit)->pp_ShortEdgeTree);
                FreeHuffmanTree((*pit)->pp_LongEdgeLenTree);
                (*pit)->pp_BranchTree = NULL;
                (*pit)->pp_ShortEdgeTree = NULL;
                (*pit)->pp_LongEdgeLenTree = NULL;
                free((*pit)->pp_LongDictRaw);
                (*pit)->pp_LongDictRaw = NULL;
                throw std::runtime_error(
                        "PtpanTree::loadPartitions() Unable to map file to ram!");
            }
        } else {
            fclose(fh);
            FreeHuffmanTree((*pit)->pp_BranchTree);
            FreeHuffmanTree((*pit)->pp_ShortEdgeTree);
            FreeHuffmanTree((*pit)->pp_LongEdgeLenTree);
            (*pit)->pp_BranchTree = NULL;
            (*pit)->pp_ShortEdgeTree = NULL;
            (*pit)->pp_LongEdgeLenTree = NULL;
            throw std::runtime_error(
                    "PtpanTree::loadPartitions() Out of memory while loading long dictionary!");
        }
    }
}

/*!
 * \brief Private method to get tree path from given node to root up to given length
 *
 * \param tn TreeNode to start at
 * \param strptr Buffer to copy into
 * \param len Maximum length to retrieve (+1 ??)
 */
void PtpanTree::get_tree_path(struct TreeNode *tn, std::string& strptr,
        ULONG len) const {
    if (!len) {
        return;
    }
    /* if the edge is too short, fill with question marks */
    while (len > tn->tn_TreeOffset) {
        len--;
        strptr[len] = '?';
    }
    while (len) {
        while (len + tn->tn_EdgeLen > tn->tn_TreeOffset) {
            len--;
            strptr[len] = tn->tn_Edge[len + tn->tn_EdgeLen - tn->tn_TreeOffset];
        }
        tn = tn->tn_Parent;
    }
}

/*!
 * \brief Private method to get tree path from given node to root up to given length
 *
 * \param tn TreeNode to start at
 * \param len Maximum length to retrieve (+1 ??)
 *
 * \return
 */
std::string PtpanTree::get_tree_path(struct TreeNode *tn, ULONG len) const {
    std::string probe = std::string(len, '?');
    while (len) {
        while (len + tn->tn_EdgeLen > tn->tn_TreeOffset) {
            len--;
            probe[len] = tn->tn_Edge[len + tn->tn_EdgeLen - tn->tn_TreeOffset];
        }
        tn = tn->tn_Parent;
    }
    return probe;
}

/*!
 * \brief Go down node child
 *
 * \param oldtn
 * \param seqcode
 *
 * \return struct TreeNode*
 */
struct PtpanTree::TreeNode* PtpanTree::go_down_node_child(
        struct PtpanTree::TreeNode *oldtn, UWORD seqcode) const {
    // this method will be called only for inner nodes, so don't mind about
    // everything else!
    if (!oldtn->tn_Children[seqcode]) {
        return (NULL);
    }

    struct PTPanPartition *pp = oldtn->tn_PTPanPartition;
    struct TreeNode *tn = read_packed_node(pp, oldtn->tn_Children[seqcode]);
    /* fill in downlink information */
    tn->tn_TreeOffset = oldtn->tn_TreeOffset + tn->tn_EdgeLen;
    *tn->tn_Edge = m_as->decompress_table[seqcode];
    tn->tn_ParentSeq = seqcode;
    tn->tn_Parent = oldtn;
    return (tn);
}

/*!
 * \brief Go down node child no edge
 *
 * \param oldtn
 * \param seqcode
 *
 * \return struct TreeNode*
 */
struct PtpanTree::TreeNode* PtpanTree::go_down_node_child_no_edge(
        struct PtpanTree::TreeNode *oldtn, UWORD seqcode) const {
    if (!oldtn->tn_Children[seqcode]) {
        return (NULL);
    }

    struct PTPanPartition *pp = oldtn->tn_PTPanPartition;
    struct TreeNode *tn = read_packed_node_no_edge(pp,
            oldtn->tn_Children[seqcode]);
    /* fill in downlink information */
    tn->tn_TreeOffset = oldtn->tn_TreeOffset + tn->tn_EdgeLen;
    tn->tn_ParentSeq = seqcode;
    tn->tn_Parent = oldtn;
    return (tn);
}

/*!
 * \brief Read packed node (with edge)
 */
struct PtpanTree::TreeNode* PtpanTree::read_packed_node(
        struct PTPanPartition *pp, ULONG pos) const {
    UBYTE *treeptr = &pp->pp_DiskTree[pos];

#if 0 // debugging
    bitpos = 8;
    if(*treeptr != 0xf2)
    {
        printf("Pos %08lx: Magic %02x. We're in trouble.\n", pos, *treeptr);
        return(NULL);
    } else {
        //printf("Pos %08lx: GOOD\n", pos);
    }
#endif
    //printf("Pos %ld ", pos);
    /* read short edge tree */
    ULONG bitpos = 0;
    struct HuffTree *ht = FindHuffTreeID(pp->pp_ShortEdgeTree, treeptr, bitpos);
    bitpos += ht->ht_CodeLength;
    ULONG pval = ht->ht_ID;
    ULONG edgelen;
    ULONG edgepos = 0;
    bool shortedge;
    if (pval) /* short edge */
    {
        /* id refers to a short edge code */
        /* find out length of edge */
        //printf("SHORT EDGE ID %ld (bitlen %d)\n", pval, ht->ht_CodeLength);
        if (pval > 1) {
            edgelen = m_as->short_edge_max;
            while (!(pval & (1UL << m_as->bits_use_table[edgelen]))) {
                edgelen--;
            }
            pval -= 1UL << m_as->bits_use_table[edgelen];
        } else {
            edgelen = 1;
        }
        shortedge = true;
    } else {
        /* long edge */
        ht = FindHuffTreeID(pp->pp_LongEdgeLenTree, treeptr, bitpos);
        bitpos += ht->ht_CodeLength;
        edgelen = ht->ht_ID;
        //printf("LONG EDGE ID %ld (bitlen %d) ", edgelen, ht->ht_CodeLength);
        edgepos = AbstractAlphabetSpecifics::readBits(treeptr, bitpos,
                pp->pp_LongRelPtrBits);
        bitpos += pp->pp_LongRelPtrBits;
        //printf("RELPTR %ld (bitlen %d)\n", edgepos, pp->pp_LongRelPtrBits);
        shortedge = false;
    }

    // struct TreeNode *tn = (struct TreeNode *) calloc(sizeof(struct TreeNode)
    // + edgelen + 1, 1);
    struct TreeNode *tn = (struct TreeNode *) calloc(
            sizeof(struct TreeNode) + edgelen + 1, 1);
    tn->tn_Children = (ULONG *) calloc(m_as->alphabet_size * sizeof(ULONG), 1);

    /* fill in data */
    tn->tn_PTPanPartition = pp;
    tn->tn_Pos = pos;
    tn->tn_NumLeaves = 0;
    tn->tn_EdgeLen = edgelen;
    // NOTE: the memory for the edge is allocated together with the struct itself
    // -> no further allocation necessary
    tn->tn_Edge[0] = 'X';
    if (shortedge) /* decompress edge */
    {
        //printf("Short %ld ", pval);
        ULONG cnt = edgelen;
        while (--cnt) {
            tn->tn_Edge[cnt] =
                    m_as->decompress_table[pval % m_as->alphabet_size];
            pval /= m_as->alphabet_size;
        }
    } else {
        //printf("Long");
        m_as->decompressSequencePartTo(pp->pp_LongDictRaw, edgepos, edgelen - 1,
                &tn->tn_Edge[1]);
    }

    /* decode branch array */
    ht = FindHuffTreeID(pp->pp_BranchTree, treeptr, bitpos);
    bitpos += ht->ht_CodeLength;
    ULONG alphamask = ht->ht_ID;

    if (alphamask != 0) {
        ULONG mask;
        //printf("AlphaMask %lx (bitlen %d)\n", alphamask, ht->ht_CodeLength);
        for (UWORD cnt = 0; cnt < m_as->alphabet_size; cnt++) {
            if (alphamask & (1UL << cnt)) /* bit set? */
            {
                mask = AbstractAlphabetSpecifics::readBits(treeptr, bitpos, 4);
                // printf("Mask %lx\n", mask);
                if (mask >> 3) /* {1} */
                {
                    /* first bit is 1 (no "rel is next" pointer) */
                    if ((mask >> 2) == 0x2) /* {10} */
                    {
                        /* this is a 6 bit relative node pointer */
                        bitpos += 2;
                        tn->tn_Children[cnt] =
                                AbstractAlphabetSpecifics::readBits(treeptr,
                                        bitpos, 6);
                        bitpos += 6;
                    } else {
                        if ((mask >> 1) == 0x7) { /* {111} */
                            /* this is a 11 bit relative node pointer */
                            bitpos += 3;
                            tn->tn_Children[cnt] =
                                    AbstractAlphabetSpecifics::readBits(treeptr,
                                            bitpos, 11);
                            bitpos += 11;
                        } else if (mask == 0xc) { /* {1100} */
                            /* this is a 27 bit relative node pointer */
                            bitpos += 4;
                            tn->tn_Children[cnt] =
                                    AbstractAlphabetSpecifics::readBits(treeptr,
                                            bitpos, 27);
                            bitpos += 27;
                        } else if (mask == 0xd) { /* {1101} */
                            /* this is a 36 bit relative node pointer */
                            bitpos += 4;
                            tn->tn_Children[cnt] =
                                    AbstractAlphabetSpecifics::readBits(treeptr,
                                            bitpos, 36);
                            bitpos += 36;
                        }
                    }
                } else { /* {0} */
                    /* rel is next pointer */
                    bitpos++;
                    tn->tn_Children[cnt] = 0;
                }
                //printf("Child %08lx\n", children[cnt]);
            }
        }
        /* add size and pos to relative pointers */
        tn->tn_Size = (bitpos + 7) >> 3;
        ULONG newbase = pos;
        for (UWORD cnt = 0; cnt < m_as->alphabet_size; cnt++) {
            if (alphamask & (1UL << cnt)) /* bit set? */
            {
                //printf("N[%08lx] ", children[cnt]);
                newbase += tn->tn_Children[cnt];
                tn->tn_Children[cnt] = newbase;
                tn->tn_Children[cnt] += tn->tn_Size;
            }
        }
    } else {
        tn->tn_Size = (bitpos + 7) >> 3;
        // if border node, read leaves as well!
        read_packed_leaf_array(pp, pos + tn->tn_Size, tn);
    }

    return (tn);
}

/*!
 * \brief Read packed node (without edge)
 */
struct PtpanTree::TreeNode* PtpanTree::read_packed_node_no_edge(
        struct PTPanPartition *pp, ULONG pos) const {
    ULONG bitpos = 0;
    UBYTE *treeptr = &pp->pp_DiskTree[pos];
    ULONG edgelen;

    /* read short edge tree */
    struct HuffTree *ht = FindHuffTreeID(pp->pp_ShortEdgeTree, treeptr, bitpos);
    bitpos += ht->ht_CodeLength;
    ULONG pval = ht->ht_ID;
    /* this is the quick version -- no need to fully decode the edge */
    if (pval) /* short edge */
    {
        /* id refers to a short edge code */
        /* find out length of edge */
        if (pval > 1) {
            edgelen = m_as->short_edge_max;
            while (!(pval & (1UL << m_as->bits_use_table[edgelen]))) {
                edgelen--;
            }
        } else {
            edgelen = 1;
        }
    } else {
        /* long edge */
        ht = FindHuffTreeID(pp->pp_LongEdgeLenTree, treeptr, bitpos);
        bitpos += ht->ht_CodeLength;
        edgelen = ht->ht_ID;
        bitpos += pp->pp_LongRelPtrBits;
    }

    struct TreeNode *tn = (struct TreeNode *) calloc(sizeof(struct TreeNode),
            1);
    tn->tn_Children = (ULONG *) calloc(m_as->alphabet_size * sizeof(ULONG), 1);
    /* fill in data */
    tn->tn_PTPanPartition = pp;
    tn->tn_Pos = pos;
    tn->tn_NumLeaves = 0;
    tn->tn_EdgeLen = edgelen;

    /* decode branch array */
    ht = FindHuffTreeID(pp->pp_BranchTree, treeptr, bitpos);
    bitpos += ht->ht_CodeLength;
    ULONG alphamask = ht->ht_ID;

    if (alphamask != 0) {
        ULONG mask;
        //printf("AlphaMask %lx (bitlen %d)\n", alphamask, ht->ht_CodeLength);
        for (UWORD cnt = 0; cnt < m_as->alphabet_size; cnt++) {
            if (alphamask & (1UL << cnt)) /* bit set? */
            {
                mask = AbstractAlphabetSpecifics::readBits(treeptr, bitpos, 4);
                // printf("Mask %lx\n", mask);
                if (mask >> 3) /* {1} */
                {
                    /* first bit is 1 (no "rel is next" pointer) */
                    if ((mask >> 2) == 0x2) /* {10} */
                    {
                        /* this is a 6 bit relative node pointer */
                        bitpos += 2;
                        tn->tn_Children[cnt] =
                                AbstractAlphabetSpecifics::readBits(treeptr,
                                        bitpos, 6);
                        bitpos += 6;
                    } else {
                        if ((mask >> 1) == 0x7) { /* {111} */
                            /* this is a 11 bit relative node pointer */
                            bitpos += 3;
                            tn->tn_Children[cnt] =
                                    AbstractAlphabetSpecifics::readBits(treeptr,
                                            bitpos, 11);
                            bitpos += 11;
                        } else if (mask == 0xc) { /* {1100} */
                            /* this is a 27 bit relative node pointer */
                            bitpos += 4;
                            tn->tn_Children[cnt] =
                                    AbstractAlphabetSpecifics::readBits(treeptr,
                                            bitpos, 27);
                            bitpos += 27;
                        } else if (mask == 0xd) { /* {1101} */
                            /* this is a 36 bit relative node pointer */
                            bitpos += 4;
                            tn->tn_Children[cnt] =
                                    AbstractAlphabetSpecifics::readBits(treeptr,
                                            bitpos, 36);
                            bitpos += 36;
                        }
                    }
                } else { /* {0} */
                    /* rel is next pointer */
                    bitpos++;
                    tn->tn_Children[cnt] = 0;
                }
                //printf("Child %08lx\n", children[cnt]);
            }
        }
        /* add size and pos to relative pointers */
        tn->tn_Size = (bitpos + 7) >> 3;
        ULONG newbase = pos;
        for (UWORD cnt = 0; cnt < m_as->alphabet_size; cnt++) {
            if (alphamask & (1UL << cnt)) /* bit set? */
            {
                //printf("N[%08lx] ", children[cnt]);
                newbase += tn->tn_Children[cnt];
                tn->tn_Children[cnt] = newbase;
                tn->tn_Children[cnt] += tn->tn_Size;
            }
        }
    } else {
        tn->tn_Size = (bitpos + 7) >> 3;
        // if border node, read leaves as well!
        read_packed_leaf_array(pp, pos + tn->tn_Size, tn);
    }
    return (tn);
}

/*!
 * \brief Read packed leaf array
 */
void PtpanTree::read_packed_leaf_array(struct PTPanPartition *pp, ULONG pos,
        struct TreeNode *tn) const {
    /* compression:
     msb == {1}    -> 1 byte offset (-   63 -     63)
     msb == {01}   -> 2 byte offset (- 8192 -   8191)
     msb == {001}  -> 3 byte offset (- 2^20 - 2^20-1)
     msb == {0001} -> 4 byte offset (- 2^27 - 2^27-1)
     msb == {0000} -> 5 byte offset (- 2^35 - 2^35-1)
     */
    UBYTE *treeptr = &pp->pp_DiskTree[pos];
    ULONG leaves = 0;
    UBYTE code;
    /* get number of leaves first */
    while ((code = *treeptr) != 0xff) {
        leaves++;
        if (code >> 7) /* {1} */
        {
            /* 1 byte */
            treeptr++;
        } else if (code >> 6) /* {01} */
        {
            /* 2 bytes */
            treeptr += 2;
        } else if (code >> 5) /* {001} */
        {
            /* 3 bytes */
            treeptr += 3;
        } else if (code >> 4) /* {0001} */
        {
            /* 4 bytes */
            treeptr += 4;
        } else { /* {0000} */
            // TODO FIXME this may cause trouble
            /* 5 bytes */
            treeptr += 5;
        }
    }

    tn->tn_NumLeaves = leaves;
    tn->tn_Leaves = (LONG *) calloc(sizeof(LONG) * leaves, 1);
    treeptr = &pp->pp_DiskTree[pos];
    LONG *lptr = tn->tn_Leaves;
    LONG val;
    while ((code = *treeptr++) != 0xff) {
        if (code >> 7) /* {1} */
        {
            /* 1 byte */
            *lptr++ = (code & 0x7f) - 63;
        } else if (code >> 6) /* {01} */
        {
            /* 2 bytes */
            val = (code & 0x3f) << 8;
            val |= *treeptr++;
            *lptr++ = val - 8192;
        } else if (code >> 5) /* {001} */
        {
            /* 3 bytes */
            val = (code & 0x1f) << 8;
            val |= *treeptr++;
            val <<= 8;
            val |= *treeptr++;
            *lptr++ = val - (1L << 20);
        } else if (code >> 4) /* {0001} */
        {
            /* 4 bytes */
            val = (code & 0x0f) << 8;
            val |= *treeptr++;
            val <<= 8;
            val |= *treeptr++;
            val <<= 8;
            val |= *treeptr++;
            *lptr++ = val - (1L << 27);
        } else { /* {0000} */
            /* 5 bytes */
            val = (code & 0x0f) << 8;
            val |= *treeptr++;
            val <<= 8;
            val |= *treeptr++;
            val <<= 8;
            val |= *treeptr++;
            val <<= 8;
            val |= *treeptr++;
            *lptr++ = val - (1L << 35);
        }
    }
    /* double delta decode */
#ifdef DOUBLEDELTAOPTIONAL
    if(((LONG) *tn->tn_Leaves) < 0)
    {
        *tn->tn_Leaves = -(*tn->tn_Leaves);
        (*tn->tn_Leaves)++;
#else
    {
#endif

#ifdef DOUBLEDELTALEAVES
        for(cnt = 2; cnt < leaves; cnt++)
        {
            tn->tn_Leaves[cnt] += tn->tn_Leaves[cnt-1];
        }
#endif
#ifdef DOUBLEDELTAOPTIONAL
    }
#else
    }
#endif

    for (ULONG cnt = 1; cnt < leaves; cnt++) {
        tn->tn_Leaves[cnt] += tn->tn_Leaves[cnt - 1];
        assert((ULONG)tn->tn_Leaves[cnt] < m_TotalRawSize);
    }
}

/*!
 * \brief Read byte from file and return value
 *
 * \return ULONG
 */
ULONG PtpanTree::read_byte(FILE *fileheader) const {
    char prefix;
    std::size_t dummy = fread(&prefix, 1, 1, fileheader);
    ULONG value = 0;
    switch (prefix) {
    case '0':
        // 00 16bit
        dummy = fread(&value, 2, 1, fileheader);
        break;
    case '1':
        // 01 24bit
        dummy = fread(&value, 3, 1, fileheader);
        break;
    case '2':
        // 10 32bit
        dummy = fread(&value, 4, 1, fileheader);
        break;
    case '3':
        // 11 64bit
        dummy = fread(&value, sizeof(ULONG), 1, fileheader);
        break;
    default:
        break;
    }
    return value;
}

/*!
 * \brief copy a sequence
 *
 * Copy a sequence from the original sequence to sq->sq_SourceSeq based on the given query hit.
 * stores the number of nmismatches in nmismatches and returns the length
 * of the copy.
 */
ULONG PtpanTree::copy_sequence(const SearchQuery& sq, std::string& source_seq,
        const QueryHit& qh, ULONG &bitpos, ULONG *nmismatch,
        ULONG &count) const {
    struct PTPanEntry *pe = qh.qh_Entry;
    ULONG tarlen = sq.sq_Query.size() - qh.qh_DeleteCount + qh.qh_InsertCount;
    if (source_seq.size() != tarlen) {
        source_seq.resize(tarlen);
    }
    UBYTE code;
    *nmismatch = 0;

    for (unsigned int cnt = 0; cnt < tarlen;) {
        if (bitpos >= pe->pe_SeqDataCompressedSize) {
            assert(false);
            break;
        }
        code = m_as->getNextCharacter(pe->pe_SeqDataCompressed, bitpos, count);
        if (m_as->seq_code_valid_table[code]) {
            source_seq[cnt++] = code;
            if (code == 'N') {
                (*nmismatch)++;
            }
        }
    }
    return tarlen;
}

/*!
 * \brief match sequence ...
 */
bool PtpanTree::match_sequence(const SearchQuery& sq, SearchQueryHandle& sqh,
        const std::string& compare, const QueryHit& qh) const {
    sqh.sqh_State.reset();
    if (qh.qh_InsertCount > 0 || qh.qh_DeleteCount > 0
            || ((sq.sq_AllowInsert || sq.sq_AllowDelete)
                    && (qh.qh_Flags & QHF_UNSAFE))) {
        sqh.sqh_State.sqs_WMisCount = qh.qh_WMisCount;
        return levenshtein_match(sq, sqh, compare, NULL, false);
    }
    return hamming_match(sq, sqh, compare, NULL, false);
}

/*!
 * \brief This function computes the Levenshtein distance (edit distance with substitution, insertion
 * and deletion) and does the backtracking to return also the comparison string.
 */
bool PtpanTree::levenshtein_match(const SearchQuery& sq, SearchQueryHandle& sqh,
        const std::string& compare, STRPTR tarstr, bool create_string) const {
    LONG source_len = (LONG) compare.size();
    LONG query_len = (LONG) sq.sq_Query.size();
    UBYTE query[query_len + 1];
    UBYTE source[source_len + 1];

    for (LONG i = 0; i < source_len; i++) {
        source[i] = m_as->compress_table[(LONG) compare[i]];
    }
    for (ULONG i = 0; i < sq.sq_Query.size(); i++) {
        query[i] = m_as->compress_table[(LONG) sq.sq_Query[i]];
    }

    //Define the lattices where we will compute the Levenshtein distance and do the backtracking
    double distance_matrix[source_len + 1][query_len + 1];
    char direction_matrix[source_len + 1][query_len + 1];

    // Initialize the first row and first columns
    distance_matrix[0][0] = 0.0;
    direction_matrix[0][0] = LEV_START;

    // quick'n'dirty, but easy
    if (sq.sq_WeightedSearchMode) {

        for (LONG i = 1; i < query_len + 1; i++) {
            distance_matrix[0][i] = double(i)
                    * sq.sq_MismatchWeights->mw_Delete[query[i - 1]];
            direction_matrix[0][i] = LEV_DEL;
        }
        for (LONG j = 1; j < source_len + 1; j++) {
            distance_matrix[j][0] = double(j)
                    * sq.sq_MismatchWeights->mw_Insert[source[j - 1]];
            direction_matrix[j][0] = LEV_INS;
        }

        // Here we compute the individual fields in the lattice
        for (LONG j = 1; j < source_len + 1; j++) {
            UBYTE seqcode_source = source[j - 1];

            for (LONG i = 1; i < query_len + 1; i++) {
                // If the chars are equal, we pay nothing (notice that we must
                // subtract one from the indices, because our lattice is one number bigger in each
                // dimension)
                UBYTE seqcode_query = query[i - 1];
                if (seqcode_query == seqcode_source || !seqcode_source) {
                    distance_matrix[j][i] = distance_matrix[j - 1][i - 1];
                    direction_matrix[j][i] = LEV_EQ;
                } else // or else, we must check which would be cheapest: delete, insert or substitute.
                {
                    double deletion_cost = distance_matrix[j][i - 1]
                            + sq.sq_MismatchWeights->mw_Delete[seqcode_query];
                    double insertion_cost = distance_matrix[j - 1][i]
                            + sq.sq_MismatchWeights->mw_Insert[seqcode_source];
                    double substitution_cost = distance_matrix[j - 1][i - 1]
                            + sq.sq_MismatchWeights->mw_Replace[(seqcode_query
                                    * m_as->alphabet_size) + seqcode_source]
                                    * sqh.sqh_PosWeight[i - 1];
                    if ((substitution_cost <= deletion_cost)
                            && (substitution_cost <= insertion_cost)) {
                        if (sq.sq_AllowReplace) {
                            distance_matrix[j][i] = substitution_cost;
                        } else {
                            distance_matrix[j][i] = sq.sq_MaxErrors + 1.0f;
                        }
                        direction_matrix[j][i] = LEV_SUB;
                    } else if ((insertion_cost <= deletion_cost)
                            && (insertion_cost <= substitution_cost)) {
                        if (sq.sq_AllowInsert) {
                            distance_matrix[j][i] = insertion_cost;
                        } else {
                            distance_matrix[j][i] = sq.sq_MaxErrors + 1.0f;
                        }
                        direction_matrix[j][i] = LEV_INS;
                    } else {
                        if (sq.sq_AllowDelete) {
                            distance_matrix[j][i] = deletion_cost;
                        } else {
                            distance_matrix[j][i] = sq.sq_MaxErrors + 1.0f;
                        }
                        direction_matrix[j][i] = LEV_DEL;
                    }
                }
            }
        }
    } else {
        // basic mode
        for (LONG i = 1; i < query_len + 1; i++) {
            distance_matrix[0][i] = double(i);
            direction_matrix[0][i] = LEV_DEL;
        }
        for (LONG j = 1; j < source_len + 1; j++) {
            distance_matrix[j][0] = double(j);
            direction_matrix[j][0] = LEV_INS;
        }

        // Here we compute the individual fields in the lattice
        for (LONG j = 1; j < source_len + 1; j++) {
            UBYTE seqcode_source = source[j - 1];

            for (LONG i = 1; i < query_len + 1; i++) {
                // If the chars are equal, we pay nothing (notice that we must
                // subtract one from the indices, because our lattice is one number bigger in each
                // dimension)
                UBYTE seqcode_query = query[i - 1];

                if (seqcode_query == seqcode_source || !seqcode_source) {
                    distance_matrix[j][i] = distance_matrix[j - 1][i - 1];
                    direction_matrix[j][i] = LEV_EQ;
                } else { // or else, we must check which would be cheapest: delete, insert or substitute.
                    double deletion_cost = distance_matrix[j][i - 1] + 1.0f;
                    double insertion_cost = distance_matrix[j - 1][i] + 1.0f;
                    double substitution_cost = distance_matrix[j - 1][i - 1]
                            + 1.0f;

                    if ((substitution_cost <= deletion_cost)
                            && (substitution_cost <= insertion_cost)) {
                        if (sq.sq_AllowReplace) {
                            distance_matrix[j][i] = substitution_cost;
                        } else {
                            distance_matrix[j][i] = sq.sq_MaxErrors + 1.0f;
                        }
                        direction_matrix[j][i] = LEV_SUB;
                    } else if ((insertion_cost <= deletion_cost)
                            && (insertion_cost <= substitution_cost)) {
                        if (sq.sq_AllowInsert) {
                            distance_matrix[j][i] = insertion_cost;
                        } else {
                            distance_matrix[j][i] = sq.sq_MaxErrors + 1.0f;
                        }
                        direction_matrix[j][i] = LEV_INS;
                    } else {
                        if (sq.sq_AllowDelete) {
                            distance_matrix[j][i] = deletion_cost;
                        } else {
                            distance_matrix[j][i] = sq.sq_MaxErrors + 1.0f;
                        }
                        direction_matrix[j][i] = LEV_DEL;
                    }
                }
            }
        }
    }

    sqh.sqh_State.sqs_ErrorCount =
            (float) distance_matrix[source_len][query_len];
    if (sqh.sqh_State.sqs_ErrorCount > sq.sq_MaxErrors) {
        //        #ifdef DEBUG
        //                printf("TOO MANY ERRORS? %3.1f (%lu/%lu)\n",
        //                        sqh.sqh_State.sqs_ErrorCount, source_len, query_len);
        //                printf("%s\n", compare.data());
        //                for (LONG z = 0; z < source_len + 1; z++) {
        //                    for (LONG y = 0; y < query_len + 1; y++) {
        //                        printf("%3.1f ", distance_matrix[z][y]);
        //                    }
        //                    printf("\n");
        //                }
        //                printf("\n");
        //        #endif
        return false;
    }

    // Now we know the Levenshtein distance between the two strings.
    // It is distance_matrix[source_len][query_len].
    // We still need to do the backtracking to find out which operations
    // yield this distance. This is what we do next...
    if (create_string) {
        //Check how long the strings are and compute the length of the comparison string
        LONG compare_len = query_len + 1;
        char * compare = (char*) malloc(compare_len * sizeof(char));
        compare[compare_len - 1] = 0;

        LONG j = source_len;
        LONG i = query_len;
        LONG k = compare_len - 1;
        while (i >= 0 && j >= 0) {
            switch (direction_matrix[j][i]) {
            case LEV_INS:
                sqh.sqh_State.sqs_InsertCount++;
                compare[k] = '*';
                j--;
                break;
            case LEV_DEL:
                sqh.sqh_State.sqs_DeleteCount++;
                compare[--k] = '_';
                i--;
                break;
            case LEV_SUB: {
                sqh.sqh_State.sqs_ReplaceCount++;
                UBYTE seqcode_source = source[j - 1];
                if (seqcode_source) /* only for A, C, G, T codes, not for N */
                {
                    float misweight = distance_matrix[j][i]
                            - distance_matrix[j - 1][i - 1];
                    // is it a great mismatch?
                    if (misweight > sq.sq_MinorMisThres) {
                        /* output upper case letter */
                        compare[--k] = m_as->decompress_table[seqcode_source];
                    } else {
                        // output lower case letter
                        compare[--k] = m_as->decompress_table[seqcode_source]
                                | 0x20;
                    }
                } else {
                    compare[--k] = m_as->decompress_table[seqcode_source];
                }
                j--;
                i--;
                break;
            }
            case LEV_EQ: {
                UBYTE seqcode_source = source[j - 1];
                if (seqcode_source) {
                    compare[--k] = '=';
                } else {
                    compare[--k] = m_as->decompress_table[seqcode_source];
                }
                j--;
                i--;
                break;
            }
            default: // this will only happen if we get START
                i--;
                j--;
                break;
            }
        }
        while (j > 0) {
            sqh.sqh_State.sqs_InsertCount++;
            compare[k] = '*';
            if (k > 0) {
                k--;
            }
            j--;
        }
        while (i > 0) {
            sqh.sqh_State.sqs_DeleteCount++;
            compare[--k] = '_';
            i--;
        }

        for (LONG c = 0; c < query_len; c++) {
            *tarstr++ = compare[c];
        }
        free(compare);

    } else {
        // we don't need the overlay string, so just count ...
        LONG j = source_len;
        LONG i = query_len;
        while (i >= 0 && j >= 0) {
            switch (direction_matrix[j][i]) {
            case LEV_INS:
                sqh.sqh_State.sqs_InsertCount++;
                j--;
                break;
            case LEV_DEL:
                sqh.sqh_State.sqs_DeleteCount++;
                i--;
                break;
            case LEV_SUB:
                sqh.sqh_State.sqs_ReplaceCount++;
                j--;
                i--;
                break;
            default:
                i--;
                j--;
                break;
            }
        }
        while (j > 0) {
            sqh.sqh_State.sqs_InsertCount++;
            j--;
        }
        while (i > 0) {
            sqh.sqh_State.sqs_DeleteCount++;
            i--;
        }
    }
    return true;
}

/*!
 * \brief This function computes the Levenshtein distance (edit distance with substitution, insertion
 * and deletion) and does the backtracking to return also the comparison string.
 */
bool PtpanTree::hamming_match(const SearchQuery& sq, SearchQueryHandle& sqh,
        const std::string& compare, STRPTR tarstr, bool create_string) const {
    LONG query_len = (LONG) sq.sq_Query.size();
    if ((LONG) compare.size() != query_len) {
        // this is no hamming match, so return false!
        return false;
    }

    UBYTE seqcode_query;
    UBYTE seqcode_source;
    double wmis;

    for (LONG i = 0; i < query_len; i++) {

        seqcode_source = m_as->compress_table[(LONG) compare[i]];
        if (!seqcode_source) {
            // it is a wildcard (e.g. a 'N' for DNA/RNA)
            if (++sqh.sqh_State.sqs_NCount > sq.sq_KillNSeqsAt) {
                return false;
            }
            if (create_string) {
                tarstr[i] = compare[i];
            }
        } else {
            seqcode_query = m_as->compress_table[(LONG) sq.sq_Query[i]];

            if (seqcode_source != seqcode_query) {
                if (sq.sq_AllowReplace) {
                    // increase error level by replace operation
                    sqh.sqh_State.sqs_ReplaceCount++;
                    wmis = sq.sq_MismatchWeights->mw_Replace[(seqcode_query
                            * m_as->alphabet_size) + seqcode_source]
                            * sqh.sqh_PosWeight[i];
                    sqh.sqh_State.sqs_WMisCount += wmis;
                    if (sq.sq_WeightedSearchMode) {
                        sqh.sqh_State.sqs_ErrorCount += wmis;
                    } else {
                        sqh.sqh_State.sqs_ErrorCount += 1.0f;
                    }
                    if (create_string) {
                        if (wmis > sq.sq_MinorMisThres) {
                            // output upper case letter
                            tarstr[i] = m_as->decompress_table[seqcode_source];
                        } else {
                            // output lower case letter
                            tarstr[i] = m_as->decompress_table[seqcode_source]
                                    | 0x20;
                        }
                    }
                } else {
                    return false;
                }
            } else {
                if (create_string) {
                    tarstr[i] = '=';
                }
            }
        }
    } // end for-loop

    if (sqh.sqh_State.sqs_ErrorCount > sq.sq_MaxErrors) {
        return false;
    }
    return true;
}

/*!
 * \brief Search partition
 *
 * \param pp PTPanPartition
 * \param sq SearchQuery
 */
void PtpanTree::search_partition(struct PTPanPartition *pp,
        const SearchQuery& sq, SearchQueryHandle& sqh) const {
    //#ifdef DEBUG
    // printf("== SearchPartition: for %s %lu (%s %lu)\n", sq->sq_Query,
    //   sq->sq_QueryLen, pp->pp_PrefixSeq, pp->pp_PrefixLen);
    // printf("allowed errors %d\n", sq->sq_MaxErrors);
    //#endif

    bool check_partition = (pp->pp_PrefixLen == 0)
            || (sq.sq_AllowInsert || sq.sq_AllowDelete);

    // shortcut to prevent unnecessary partition fetching
    // at the moment, this check is limited to looking for replacements, no indels
    // FIXME maybe we want to change this!
    if (!check_partition) {
        // we just check for replacements of single characters, no indels
        float error_count = 0.0;
        for (ULONG i = 0; i < pp->pp_PrefixLen && i < sq.sq_Query.size(); i++) {
            if (pp->pp_PrefixSeq[i] != sq.sq_Query[i]) {
                if (!m_as->compress_table[(int) pp->pp_PrefixSeq[i]]) { // check if one is an N
                    continue;
                } else {
                    if (sq.sq_WeightedSearchMode) {
                        error_count +=
                                sq.sq_MismatchWeights->mw_Replace[(m_as->compress_table[(LONG) sq.sq_Query[i]]
                                        * m_as->alphabet_size)
                                        + m_as->compress_table[(LONG) pp->pp_PrefixSeq[i]]];
                    } else {
                        error_count += 1.0;
                    }
                    if (error_count > sq.sq_MaxErrors) {
                        break;
                    }
                }
            }
        }
        if (error_count <= sq.sq_MaxErrors) {
            check_partition = true;
        }
    }

    if (check_partition) {
        search_tree(pp, sq, sqh);

#ifdef DEBUG
        printf("after partition '%s' count = %lu\n", pp->pp_PrefixSeq,
                sqh.sqh_hits.size());
#endif
    }
}

/*!
 * \brief Search index tree of given partition
 */
void PtpanTree::search_tree(struct PTPanPartition *pp, const SearchQuery& sq,
        SearchQueryHandle& sqh) const {

    sqh.sqh_State.reset();
    sqh.sqh_State.sqs_TreeNode = read_packed_node(pp, 0); // TODO change to partition caching root!

    if (sq.sq_AllowInsert || sq.sq_AllowDelete) {
#ifdef DEBUG
        printf("Search by Levenshtein\n");
#endif
        search_tree_levenshtein(sq, sqh);
    } else {
#ifdef DEBUG
        printf("Search by Hamming\n");
#endif
        search_tree_hamming(sq, sqh);
    }
    TreeNode::free_tree_node(sqh.sqh_State.sqs_TreeNode); // TODO change to partition caching root!

#ifdef DEBUG
    printf("after search_tree count = %lu\n", sqh.sqh_hits.size());
#endif
}

/*!
 * \brief add QueryHit
 *
 * TODO FIXME this is just an itermediate representation, not the final one!
 */
void PtpanTree::add_query_hit(const SearchQuery& sq, SearchQueryHandle& sqh,
        ULONG hitpos) const {

    //#ifdef DEBUG
    //  struct TreeNode *tn;
    //  printf(">> AddQueryHit: ");
    //  tn = sq->sq_State.sqs_TreeNode;
    //  do {
    //   printf("%s", tn->tn_Edge);
    //   tn = tn->tn_Parent;
    //   if (tn)
    //    printf("-");
    //  } while (tn);
    //  printf(" (%f/%d/%d/%d) QryPos %ld/%ld\n", sq->sq_State.sqs_ErrorCount,
    //    sq->sq_State.sqs_ReplaceCount, sq->sq_State.sqs_InsertCount,
    //    sq->sq_State.sqs_DeleteCount, sq->sq_State.sqs_QueryPos,
    //    sq->sq_QueryLen);
    //#endif
    ULONG hitpos_corrected;
    struct QueryHit *qh;
    if (sq.sq_AllowDelete || sq.sq_AllowInsert) {
        hitpos_corrected = hitpos + sqh.sqh_State.sqs_InsertCount
                - sqh.sqh_State.sqs_DeleteCount;
        /* try eliminating duplicates even at this stage */
        boost::unordered_map<ULONG, ULONG>::const_iterator hash =
                sqh.sqh_HitsHash->find(hitpos_corrected);
        if (hash != sqh.sqh_HitsHash->end()) {
            qh = (QueryHit *) hash->second;
            if (qh) {
                if (qh->qh_CorrectedAbsPos == hitpos_corrected) {
                    /* check, if the new hit was better */
                    if (qh->qh_ErrorCount > sqh.sqh_State.sqs_ErrorCount) {
                        qh->qh_AbsPos = hitpos;
                        //obviously already correct qh->qh_CorrectedAbsPos = hitpos_corrected;
                        qh->qh_ErrorCount = sqh.sqh_State.sqs_ErrorCount;
                        qh->qh_WMisCount = sqh.sqh_State.sqs_WMisCount;
                        qh->qh_ReplaceCount = sqh.sqh_State.sqs_ReplaceCount;
                        qh->qh_InsertCount = sqh.sqh_State.sqs_InsertCount;
                        qh->qh_DeleteCount = sqh.sqh_State.sqs_DeleteCount;
                        qh->qh_nmismatch = sqh.sqh_State.sqs_NCount;
                        if (sq.sq_Reversed) { // copy reversed flag
                            qh->qh_Flags |= QHF_REVERSED;
                        } else {
                            qh->qh_Flags &= ~QHF_REVERSED;
                        }
                    } else if (qh->qh_ErrorCount
                            == sqh.sqh_State.sqs_ErrorCount) {
                        // check for better error ratio!
                        // we prefer replacements over insertion/deletions if
                        // overall error count is equal!
                        if ((qh->qh_InsertCount + qh->qh_DeleteCount)
                                > (sqh.sqh_State.sqs_InsertCount
                                        + sqh.sqh_State.sqs_DeleteCount)) {
                            qh->qh_AbsPos = hitpos;
                            //obviously already correct qh->qh_CorrectedAbsPos = hitpos_corrected;
                            qh->qh_ErrorCount = sqh.sqh_State.sqs_ErrorCount;
                            qh->qh_WMisCount = sqh.sqh_State.sqs_WMisCount;
                            qh->qh_ReplaceCount =
                                    sqh.sqh_State.sqs_ReplaceCount;
                            qh->qh_InsertCount = sqh.sqh_State.sqs_InsertCount;
                            qh->qh_DeleteCount = sqh.sqh_State.sqs_DeleteCount;
                            qh->qh_nmismatch = sqh.sqh_State.sqs_NCount;
                            if (sq.sq_Reversed) { // copy reversed flag
                                qh->qh_Flags |= QHF_REVERSED;
                            } else {
                                qh->qh_Flags &= ~QHF_REVERSED;
                            }
                        }
                    }
                    // if the hit was safe now, we can clear the unsafe bit
                    if ((sqh.sqh_State.sqs_QueryPos
                            >= (ULONG) sq.sq_Query.size())) {
                        qh->qh_Flags &= ~QHF_UNSAFE; // clear unsafe bit
                    }
                    return;
                }
            } else {
                sqh.sqh_HitsHash->erase(hash);
            }
        }
    } else {
        hitpos_corrected = hitpos;
    }
    qh = new QueryHit();
    qh->qh_AbsPos = hitpos;
    qh->qh_CorrectedAbsPos = hitpos_corrected;
    qh->qh_ErrorCount = sqh.sqh_State.sqs_ErrorCount;
    qh->qh_WMisCount = sqh.sqh_State.sqs_WMisCount;
    qh->qh_ReplaceCount = sqh.sqh_State.sqs_ReplaceCount;
    qh->qh_InsertCount = sqh.sqh_State.sqs_InsertCount;
    qh->qh_DeleteCount = sqh.sqh_State.sqs_DeleteCount;
    qh->qh_Flags = QHF_ISVALID;
    qh->qh_nmismatch = sqh.sqh_State.sqs_NCount;
    if (sq.sq_Reversed) {
        qh->qh_Flags |= QHF_REVERSED;
    }
    if (sqh.sqh_State.sqs_QueryPos < (ULONG) sq.sq_Query.size()) {
        qh->qh_Flags |= QHF_UNSAFE;
    }
    sqh.sqh_hits.push_back(qh);

    if (sq.sq_AllowDelete || sq.sq_AllowInsert) {
        (*sqh.sqh_HitsHash)[hitpos_corrected] = (ULONG) qh;
    }
    //#ifdef DEBUG
    //  struct TreeNode *tn;
    //  printf("<< AddQueryHit: %lld [%08llx] <>: ", qh->qh_AbsPos,
    //    qh->qh_AbsPos);
    //  tn = sq->sq_State.sqs_TreeNode;
    //  do {
    //   printf("%s", tn->tn_Edge);
    //   tn = tn->tn_Parent;
    //   if (tn)
    //    printf("-");
    //  } while (tn);
    //
    //  printf(" (%f/%d/%d/%d)\n", qh->qh_ErrorCount, qh->qh_ReplaceCount,
    //    qh->qh_InsertCount, qh->qh_DeleteCount);
    //#endif
}

/*!
 * \brief Search index tree recursive (hamming distance)
 */
void PtpanTree::search_tree_hamming(const SearchQuery& sq,
        SearchQueryHandle& sqh) const {
    struct TreeNode *oldtn = sqh.sqh_State.sqs_TreeNode;
    // if we have leaves, it's a border node!
    if (oldtn->tn_Leaves) {
        ULONG cnt = oldtn->tn_NumLeaves;
        LONG *leafptr = oldtn->tn_Leaves;
        do {
            assert((ULONG)*leafptr < m_TotalRawSize);
            // TODO FIXME add Hamming queryHit
            add_query_hit(sq, sqh, (ULONG) *leafptr++);
        } while (--cnt);
        return;
    }

    struct SearchQueryState oldstate = sqh.sqh_State;
    UWORD seqcodetree;
    UWORD seqcode;
    UWORD seqcode2;
    bool ignore;
    struct TreeNode *tn;

    for (seqcode = 0; seqcode < m_as->alphabet_size; seqcode++) {

        if (oldtn->tn_Children[seqcode]) {
            tn = NULL;
            ignore = false;

            if (!seqcode) {
                if (++sqh.sqh_State.sqs_NCount > sq.sq_KillNSeqsAt) {
                    ignore = true;
                }
            } else {
                // check, if first seqcode of edge matches (N (seqcode 0) matches always)
                seqcode2 =
                        m_as->compress_table[(int) sq.sq_Query[sqh.sqh_State.sqs_QueryPos]];
                if (seqcode2 != seqcode) {

                    if (sq.sq_AllowReplace) {
                        /* increase error level by replace operation */
                        sqh.sqh_State.sqs_ReplaceCount++;
                        sqh.sqh_State.sqs_WMisCount +=
                                sq.sq_MismatchWeights->mw_Replace[(seqcode2
                                        * m_as->alphabet_size) + seqcode]
                                        * sqh.sqh_PosWeight[sqh.sqh_State.sqs_QueryPos];
                        if (sq.sq_WeightedSearchMode) {
                            sqh.sqh_State.sqs_ErrorCount +=
                                    sq.sq_MismatchWeights->mw_Replace[(seqcode2
                                            * m_as->alphabet_size) + seqcode]
                                            * sqh.sqh_PosWeight[sqh.sqh_State.sqs_QueryPos];
                        } else {
                            sqh.sqh_State.sqs_ErrorCount += 1.0;
                        }
                    } else {
                        ignore = true;
                    }
                }
                /* check, if more errors are tolerable. */
                if (sqh.sqh_State.sqs_ErrorCount > sq.sq_MaxErrors) {
                    // too many errors, do not recurse
                    ignore = true;
                }
            }

            if (!ignore) {
                sqh.sqh_State.sqs_QueryPos++;
                tn = go_down_node_child(oldtn, seqcode);

                // do we reach the end of the query with the first seqcode in the edge?
                if (sqh.sqh_State.sqs_QueryPos < sq.sq_Query.size()) {
                    // we have a longer edge, so check its contents before we follow it to the end
                    if (tn->tn_EdgeLen > 1) {
                        STRPTR tarptr = &tn->tn_Edge[1];
                        UBYTE tcode;
                        while ((tcode = *tarptr++)) {
                            // check, if codes are the same (or N is found)
                            seqcodetree = m_as->compress_table[tcode];
                            seqcode2 =
                                    m_as->compress_table[(int) sq.sq_Query[sqh.sqh_State.sqs_QueryPos]];
                            if (seqcode2 != seqcodetree) {
                                if (!seqcodetree) {
                                    if (++sqh.sqh_State.sqs_NCount
                                            > sq.sq_KillNSeqsAt) {
                                        ignore = true;
                                        break;
                                    }
                                } else {
                                    if (sq.sq_AllowReplace) {
                                        sqh.sqh_State.sqs_ReplaceCount++;
                                        sqh.sqh_State.sqs_WMisCount +=
                                                sq.sq_MismatchWeights->mw_Replace[seqcode2
                                                        * m_as->alphabet_size
                                                        + seqcodetree]
                                                        * sqh.sqh_PosWeight[sqh.sqh_State.sqs_QueryPos];
                                        if (sq.sq_WeightedSearchMode) {
                                            sqh.sqh_State.sqs_ErrorCount +=
                                                    sq.sq_MismatchWeights->mw_Replace[seqcode2
                                                            * m_as->alphabet_size
                                                            + seqcodetree]
                                                            * sqh.sqh_PosWeight[sqh.sqh_State.sqs_QueryPos];
                                        } else {
                                            sqh.sqh_State.sqs_ErrorCount += 1.0;
                                        }
                                        /* check, if more errors are tolerable. */
                                        if (sqh.sqh_State.sqs_ErrorCount
                                                > sq.sq_MaxErrors) {
                                            /* too many errors, do not recurse */
                                            ignore = true;
                                            break;
                                        }
                                    } else {
                                        ignore = true;
                                    }
                                }
                            }
                            /* check if the end of the query string was reached */
                            if (++sqh.sqh_State.sqs_QueryPos
                                    >= sq.sq_Query.size()) {
                                break;
                            }
                        }
                    }
                }
                /* are we allowed to go any further down the tree? */
                if (!ignore) {
                    /* recurse */
                    sqh.sqh_State.sqs_TreeNode = tn;
                    if (sqh.sqh_State.sqs_QueryPos < sq.sq_Query.size()) {
                        search_tree_hamming(sq, sqh);
                    } else {
                        collect_tree_rec(sq, sqh);
                    }
                }
            }

            TreeNode::free_tree_node(tn);
        }
        /* restore possible altered data */
        sqh.sqh_State = oldstate;
    }
}

/*!
 * \brief Search index tree recursive
 */
void PtpanTree::search_tree_levenshtein(const SearchQuery& sq,
        SearchQueryHandle& sqh) const {
    LONG query_len =
            sq.sq_Query.size() > m_prune_length ?
                    m_prune_length : (LONG) sq.sq_Query.size();
    LONG source_len =
            sqh.sqh_MaxLength > m_prune_length ?
                    m_prune_length : (LONG) sqh.sqh_MaxLength;

    LONG max_check =
            sq.sq_Query.size() < m_prune_length ?
                    m_prune_length - sq.sq_Query.size() : 0;
    if (max_check > sq.sq_MaxErrors) {
        max_check = (LONG) sq.sq_MaxErrors;
    }

#ifdef DEBUG
    printf("length of source/query %lu/%lu (prune-length=%lu; max_check=%ld)\n",
            source_len, query_len, m_prune_length, max_check);
#endif

    UBYTE query[query_len + 1];
    // ..
    for (LONG i = 0; i < query_len; i++) {
        query[i] = m_as->compress_table[(LONG) sq.sq_Query[i]];
    }
    UBYTE source[source_len + 1];

    //Define the lattices where we will compute the Levenshtein distance and do the backtracking
    LevenshteinSupport levenshtein(source_len, query_len, max_check);

    // Initialize the first row and first columns
    levenshtein.m_distance_matrix[0][0] = 0.0;
    levenshtein.m_direction_matrix[0][0] = LEV_START;

    for (LONG i = 1; i < query_len + 1; i++) {
        // to avoid leading deletions, set to more than max error
        levenshtein.m_distance_matrix[0][i] = sq.sq_MaxErrors + 1.0f;
        levenshtein.m_direction_matrix[0][i] = LEV_DEL;
    }
    for (LONG j = 1; j < source_len + 1; j++) {
        // to avoid leading insertions, set to more than max error
        levenshtein.m_distance_matrix[j][0] = sq.sq_MaxErrors + 1.0f;
        levenshtein.m_direction_matrix[j][0] = LEV_INS;
    }

    search_tree_levenshtein_rec(sq, sqh, &source[0], &query[0], levenshtein,
            1L);
}

/*!
 * \brief Search index tree recursive
 */
void PtpanTree::search_tree_levenshtein_rec(const SearchQuery& sq,
        SearchQueryHandle& sqh, UBYTE *source, UBYTE *query,
        LevenshteinSupport& levenshtein, ULONG start_j) const {

    struct TreeNode *oldtn = sqh.sqh_State.sqs_TreeNode;
    struct SearchQueryState oldstate;
    struct TreeNode *tn = NULL;

    for (UWORD seqcode = 0; seqcode < m_as->alphabet_size; seqcode++) {
        if (oldtn->tn_Children[seqcode]) {
            bool ignore = false;
            bool direct_evaluate = false;
            oldstate = sqh.sqh_State;
            double deletion_cost;
            double insertion_cost;
            double substitution_cost;

            source[start_j - 1] = seqcode;
            // first test with help of branch mask to avoid going down!
            if (!seqcode) {
                // first check for wildcard, e.g. N
                if (++sqh.sqh_State.sqs_NCount > sq.sq_KillNSeqsAt) {
                    ignore = true;
                }
            }
            if (!ignore) {
                double min_column_error = sq.sq_MaxErrors + 1.0f;

                for (ULONG i = 1; i < levenshtein.m_query_len + 1; i++) {

                    UBYTE seqcode_query = query[i - 1];

                    if (seqcode_query == seqcode || !seqcode) {
                        levenshtein.m_distance_matrix[start_j][i] =
                                levenshtein.m_distance_matrix[start_j - 1][i - 1];
                        levenshtein.m_direction_matrix[start_j][i] = LEV_EQ;

                    } else { // or else, we must check which would be cheapest: delete, insert or substitute.
                        if (sq.sq_WeightedSearchMode) {
                            deletion_cost =
                                    levenshtein.m_distance_matrix[start_j][i - 1]
                                            + sq.sq_MismatchWeights->mw_Delete[seqcode_query];
                            insertion_cost =
                                    levenshtein.m_distance_matrix[start_j - 1][i]
                                            + sq.sq_MismatchWeights->mw_Insert[seqcode];
                            substitution_cost =
                                    levenshtein.m_distance_matrix[start_j - 1][i
                                            - 1]
                                            + sq.sq_MismatchWeights->mw_Replace[(seqcode_query
                                                    * m_as->alphabet_size)
                                                    + seqcode]
                                                    * sqh.sqh_PosWeight[i - 1];
                        } else {
                            deletion_cost =
                                    levenshtein.m_distance_matrix[start_j][i - 1]
                                            + 1.0f;
                            insertion_cost =
                                    levenshtein.m_distance_matrix[start_j - 1][i]
                                            + 1.0f;
                            substitution_cost =
                                    levenshtein.m_distance_matrix[start_j - 1][i
                                            - 1] + 1.0f;
                        }

                        if ((substitution_cost <= deletion_cost)
                                && (substitution_cost <= insertion_cost)) {
                            if (sq.sq_AllowReplace) {
                                levenshtein.m_distance_matrix[start_j][i] =
                                        substitution_cost;
                            } else {
                                levenshtein.m_distance_matrix[start_j][i] =
                                        sq.sq_MaxErrors + 1.0f;
                            }
                            levenshtein.m_direction_matrix[start_j][i] = LEV_SUB;

                        } else if ((insertion_cost <= deletion_cost)
                                && (insertion_cost <= substitution_cost)) {
                            if (sq.sq_AllowInsert) {
                                levenshtein.m_distance_matrix[start_j][i] =
                                        insertion_cost;
                            } else {
                                levenshtein.m_distance_matrix[start_j][i] =
                                        sq.sq_MaxErrors + 1.0f;
                            }
                            levenshtein.m_direction_matrix[start_j][i] = LEV_INS;

                        } else {
                            if (sq.sq_AllowDelete) {
                                levenshtein.m_distance_matrix[start_j][i] =
                                        deletion_cost;
                            } else {
                                levenshtein.m_distance_matrix[start_j][i] =
                                        sq.sq_MaxErrors + 1.0f;
                            }
                            levenshtein.m_direction_matrix[start_j][i] = LEV_DEL;
                        }
                    }
                    // check whether we have a lower error value after the taken action
                    if (levenshtein.m_distance_matrix[start_j][i]
                            < min_column_error) {
                        min_column_error =
                                levenshtein.m_distance_matrix[start_j][i];
                    }
                }

                // if all values in the column exceed the error threshold, we can cut off
                // and stop traversing down the suffix tree!
                if (min_column_error > sq.sq_MaxErrors) {
                    if (start_j < levenshtein.m_query_len - sq.sq_MaxErrors) {
                        ignore = true;
                    } else {
                        // in this case we need to check if the already evaluated columns deliver
                        // a valid hit, so we need to skip the recursion, but we must
                        // check for hits, so we cannot simply ignore this case!
                        ULONG j = start_j + 1;
                        for (; j < levenshtein.m_source_len + 1; j++) {
                            levenshtein.m_distance_matrix[j][levenshtein.m_query_len] =
                                    sq.sq_MaxErrors + 1.0f; // to avoid later misleading values
                            levenshtein.m_direction_matrix[j][levenshtein.m_query_len] =
                                    LEV_DEL;
                        }
                        direct_evaluate = true;
                    }
                }
            }

            if (!ignore) {
                if (!direct_evaluate) {
                    tn = go_down_node_child(oldtn, seqcode);
                    sqh.sqh_State.sqs_TreeNode = tn;
                    if (tn->tn_EdgeLen > 1) {
                        for (UWORD k = 1; k < tn->tn_EdgeLen; k++) {
                            source[start_j - 1 + k] =
                                    m_as->compress_table[(LONG) tn->tn_Edge[k]];
                            if (start_j + k > levenshtein.m_source_len + 1) {
                                break;
                            }
                        }

                        for (ULONG j = start_j + 1;
                                (j < start_j + tn->tn_EdgeLen)
                                        && (j < levenshtein.m_source_len + 1);
                                j++) {

                            UBYTE seqcode_source = source[j - 1];
                            if (!seqcode_source) {
                                // N-mismatch
                                if (++sqh.sqh_State.sqs_NCount
                                        > sq.sq_KillNSeqsAt) {
                                    ignore = true;
                                    break;
                                }
                            }
                            double min_column_error = sq.sq_MaxErrors + 1.0f;

                            for (ULONG i = 1; i < levenshtein.m_query_len + 1;
                                    i++) {

                                UBYTE seqcode_query = query[i - 1];

                                if (seqcode_query == seqcode_source
                                        || !seqcode_source) {
                                    levenshtein.m_distance_matrix[j][i] =
                                            levenshtein.m_distance_matrix[j - 1][i
                                                    - 1];
                                    levenshtein.m_direction_matrix[j][i] =
                                            LEV_EQ;

                                } else { // or else, we must check which would be cheapest: delete, insert or substitute.
                                    if (sq.sq_WeightedSearchMode) {
                                        deletion_cost =
                                                levenshtein.m_distance_matrix[j][i
                                                        - 1]
                                                        + sq.sq_MismatchWeights->mw_Delete[seqcode_query];
                                        insertion_cost =
                                                levenshtein.m_distance_matrix[j
                                                        - 1][i]
                                                        + sq.sq_MismatchWeights->mw_Insert[seqcode_source];
                                        substitution_cost =
                                                levenshtein.m_distance_matrix[j
                                                        - 1][i - 1]
                                                        + sq.sq_MismatchWeights->mw_Replace[(seqcode_query
                                                                * m_as->alphabet_size)
                                                                + seqcode_source]
                                                                * sqh.sqh_PosWeight[i
                                                                        - 1];
                                    } else {
                                        deletion_cost =
                                                levenshtein.m_distance_matrix[j][i
                                                        - 1] + 1.0f;

                                        insertion_cost =
                                                levenshtein.m_distance_matrix[j
                                                        - 1][i] + 1.0f;

                                        substitution_cost =
                                                levenshtein.m_distance_matrix[j
                                                        - 1][i - 1] + 1.0f;
                                    }

                                    if ((substitution_cost <= deletion_cost)
                                            && (substitution_cost
                                                    <= insertion_cost)) {
                                        if (sq.sq_AllowReplace) {
                                            levenshtein.m_distance_matrix[j][i] =
                                                    substitution_cost;
                                        } else {
                                            levenshtein.m_distance_matrix[j][i] =
                                                    sq.sq_MaxErrors + 1.0f;
                                        }
                                        levenshtein.m_direction_matrix[j][i] =
                                                LEV_SUB;

                                    } else if ((insertion_cost <= deletion_cost)
                                            && (insertion_cost
                                                    <= substitution_cost)) {
                                        if (sq.sq_AllowInsert) {
                                            levenshtein.m_distance_matrix[j][i] =
                                                    insertion_cost;
                                        } else {
                                            levenshtein.m_distance_matrix[j][i] =
                                                    sq.sq_MaxErrors + 1.0f;
                                        }
                                        levenshtein.m_direction_matrix[j][i] =
                                                LEV_INS;

                                    } else {
                                        if (sq.sq_AllowDelete) {
                                            levenshtein.m_distance_matrix[j][i] =
                                                    deletion_cost;
                                        } else {
                                            levenshtein.m_distance_matrix[j][i] =
                                                    sq.sq_MaxErrors + 1.0f;
                                        }
                                        levenshtein.m_direction_matrix[j][i] =
                                                LEV_DEL;
                                    }
                                }
                                // check whether we have a lower error value after the taken action
                                if (levenshtein.m_distance_matrix[j][i]
                                        < min_column_error) {
                                    min_column_error =
                                            levenshtein.m_distance_matrix[j][i];
                                }
                            }
                            // if all values in the column exceed the error threshold, we can cut off
                            // and stop traversing down the suffix tree!
                            if (min_column_error > sq.sq_MaxErrors) {
                                if (j
                                        < levenshtein.m_query_len
                                                - sq.sq_MaxErrors) {
                                    ignore = true;
                                    break;
                                } else {
                                    // in this case we need to check if the already evaluated columns deliver
                                    // a valid hit, so we need to skip the recursion, but we must
                                    // check for hits, so we cannot simply ignore this case!
                                    j++;
                                    for (; j < levenshtein.m_source_len + 1;
                                            j++) {
                                        levenshtein.m_distance_matrix[j][levenshtein.m_query_len] =
                                                sq.sq_MaxErrors + 1.0f; // to avoid later misleading values
                                        levenshtein.m_direction_matrix[j][levenshtein.m_query_len] =
                                                LEV_DEL;
                                    }
                                    direct_evaluate = true;
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    tn = NULL;
                }
                if (!ignore) {

                    if (!direct_evaluate
                            && (!tn->tn_Leaves
                                    && (start_j + tn->tn_EdgeLen
                                            < levenshtein.m_source_len + 1))) {
                        // it is an inner node, so go further down
                        search_tree_levenshtein_rec(sq, sqh, source, query,
                                levenshtein, start_j + tn->tn_EdgeLen);

                    } else {
                        ULONG length = levenshtein.m_query_len;
                        sqh.sqh_State.sqs_ErrorCount =
                                (float) levenshtein.m_distance_matrix[length][levenshtein.m_query_len];
                        // find the match with the lowest error count in range
                        // range = query length +- number of allowed indels
                        if (sq.sq_AllowInsert) {
                            for (ULONG l = 1; l <= levenshtein.m_max_check;
                                    l++) {
                                if (levenshtein.m_distance_matrix[levenshtein.m_query_len
                                        + l][levenshtein.m_query_len]
                                        < sqh.sqh_State.sqs_ErrorCount && levenshtein.m_direction_matrix[levenshtein.m_query_len
                                        + l][levenshtein.m_query_len]
                                        == LEV_EQ) { // TODO FIXME why LEV_EQ??
sqh                                .sqh_State.sqs_ErrorCount =
                                levenshtein.m_distance_matrix[levenshtein.m_query_len
                                + l][levenshtein.m_query_len];
                                length = levenshtein.m_query_len + l;
                            }
                        }
                    }

                        for (int l = 1; l <= (int) sq.sq_MaxErrors; l++) {
                            if (levenshtein.m_distance_matrix[levenshtein.m_query_len
                                    - l][levenshtein.m_query_len]
                                    < sqh.sqh_State.sqs_ErrorCount && levenshtein.m_direction_matrix[levenshtein.m_query_len
                                    - l][levenshtein.m_query_len]
                                    == LEV_EQ) { // TODO FIXME why LEV_EQ??
sqh                            .sqh_State.sqs_ErrorCount =
                            levenshtein.m_distance_matrix[levenshtein.m_query_len
                            - l][levenshtein.m_query_len];
                            length = levenshtein.m_query_len - l;
                        }
                    }
                        ULONG i_length = levenshtein.m_query_len;
                        if (levenshtein.m_source_len == m_prune_length
                                && length == m_prune_length
                                && sqh.sqh_State.sqs_ErrorCount
                                        > sq.sq_MaxErrors) {
                            for (int l = 1; l <= (int) sq.sq_MaxErrors; l++) {
                                if (levenshtein.m_distance_matrix[length][levenshtein.m_query_len
                                        - l] <= sq.sq_MaxErrors) {
                                    // && levenshtein.m_direction_matrix[length][levenshtein.m_query_len - l + 1] == LEV_DEL
                                    sqh.sqh_State.sqs_ErrorCount =
                                            levenshtein.m_distance_matrix[length][levenshtein.m_query_len
                                                    - l];
                                    i_length = levenshtein.m_query_len - l;
                                    break;
                                }
                            }
                        }

                        //     if (sq->sq_State.sqs_ErrorCount <= 3.0f) {
                        //      printf("errors: %3.2f (%lu)\n",
                        //        sq->sq_State.sqs_ErrorCount, length);
                        //      printf("%d\n", (int) (2 * sq->sq_MaxErrors));
                        //
                        //      for (LONG z = 0; z < levenshtein.m_source_len; z++) {
                        //       for (LONG y = 0; y < levenshtein.m_query_len; y++) {
                        //        printf("%3.1f ",
                        //          levenshtein.m_distance_matrix[z][y]);
                        //       }
                        //       printf("\n");
                        //      }
                        //      printf("\n");
                        //     }

                        //      if (sq->sq_State.sqs_ErrorCount <= sq.sq_MaxErrors
                        //        && levenshtein.m_direction_matrix[length][levenshtein.m_query_len]
                        //          != LEV_DEL) {
                        if (sqh.sqh_State.sqs_ErrorCount <= sq.sq_MaxErrors) {
                            //#ifdef DEBUG
                            //     printf(
                            //       "Got %3.2f errors\n",
                            //       levenshtein.m_distance_matrix[sq->sq_MaxLength][sq->sq_QueryLen]);
                            // printf("errors: %3.2f \n", sq->sq_State.sqs_ErrorCount);
                            //#endif
                            LONG j = length;
                            LONG i = i_length; //levenshtein.m_query_len;

                            if (levenshtein.m_direction_matrix[length][i_length]
                                    == LEV_DEL) {
                                // avoid trailing deletions
                                i--;
                            }

                            //       if (sq->sq_WeightedSearchMode) {
                            //        while (i >= 0 && j >= 0) {
                            //         switch (levenshtein.m_direction_matrix[j][i]) {
                            //         case LEV_INS:
                            //          sq->sq_State.sqs_InsertCount++;
                            //          j--;
                            //          break;
                            //         case LEV_DEL:
                            //          sq->sq_State.sqs_DeleteCount++;
                            //          i--;
                            //          break;
                            //         case LEV_SUB:
                            //          sq->sq_State.sqs_ReplaceCount++;
                            //          j--;
                            //          i--;
                            //          break;
                            //         default:
                            //          i--;
                            //          j--;
                            //          break;
                            //         }
                            //        }
                            //        sq->sq_State.sqs_WMisCount
                            //          = sq->sq_State.sqs_ErrorCount;
                            //       } else {
                            sqh.sqh_State.sqs_WMisCount = 0.0f;
                            while (i >= 0 && j >= 0) {
                                switch (levenshtein.m_direction_matrix[j][i]) {
                                case LEV_INS:
                                    sqh.sqh_State.sqs_WMisCount +=
                                            sq.sq_MismatchWeights->mw_Insert[source[j]];
                                    sqh.sqh_State.sqs_InsertCount++;
                                    j--;
                                    //printf("i");
                                    break;
                                case LEV_DEL:
                                    sqh.sqh_State.sqs_WMisCount +=
                                            sq.sq_MismatchWeights->mw_Delete[query[i]];
                                    sqh.sqh_State.sqs_DeleteCount++;
                                    i--;
                                    //printf("d");
                                    break;
                                case LEV_SUB:
                                    sqh.sqh_State.sqs_WMisCount +=
                                            sq.sq_MismatchWeights->mw_Replace[(query[i
                                                    - 1] * m_as->alphabet_size)
                                                    + source[j - 1]]
                                                    * sqh.sqh_PosWeight[i - 1];
                                    sqh.sqh_State.sqs_ReplaceCount++;
                                    //printf("r");
                                    j--;
                                    i--;
                                    break;
                                default:
                                    //printf("=");
                                    i--;
                                    j--;
                                    break;
                                }
                            }

                            if (sq.sq_WeightedSearchMode) {
                                sqh.sqh_State.sqs_ErrorCount =
                                        sqh.sqh_State.sqs_WMisCount;
                            } else {
                                sqh.sqh_State.sqs_ErrorCount =
                                        sqh.sqh_State.sqs_ReplaceCount
                                                + sqh.sqh_State.sqs_InsertCount
                                                + sqh.sqh_State.sqs_DeleteCount;
                            }
                            if (sqh.sqh_State.sqs_ErrorCount
                                    > sq.sq_MaxErrors) {
                                //printf(" break ");
                                //        printf("IGNORE %3.2f\n",
                                //          sq->sq_State.sqs_ErrorCount);
                                ignore = true;
                            }
                            //printf("\n");
                            // }

                            //       printf("%ld %ld %ld\n",
                            //         sq->sq_State.sqs_InsertCount,
                            //         sq->sq_State.sqs_DeleteCount,
                            //         sq->sq_State.sqs_ReplaceCount);

                            if (!ignore) {
                                if ((levenshtein.m_query_len
                                        + sqh.sqh_State.sqs_InsertCount
                                        - sqh.sqh_State.sqs_DeleteCount)
                                        > m_prune_length) {
                                    // printf("force verify\n");
                                    sqh.sqh_State.sqs_QueryPos =
                                            levenshtein.m_query_len
                                                    - sqh.sqh_State.sqs_InsertCount
                                                    + sqh.sqh_State.sqs_DeleteCount;
                                } else {
                                    sqh.sqh_State.sqs_QueryPos =
                                            levenshtein.m_query_len;
                                }
                                collect_tree_rec(sq, sqh);
                            }
                        }
                    }
                }
                TreeNode::free_tree_node(tn);
            }
            // restore possible altered data
            sqh.sqh_State = oldstate;
        }
    }
}

/*!
 * \brief Collect tree leaves recursive
 */
void PtpanTree::collect_tree_rec(const SearchQuery& sq,
        SearchQueryHandle& sqh) const {
    struct TreeNode *oldtn = sqh.sqh_State.sqs_TreeNode;
    ULONG cnt;

    //#ifdef DEBUG
    // const char* indent;
    // const char* spaces = "                                            ";
    // indent = spaces + strlen(spaces) - sq->sq_State.sqs_QueryPos;
    //#endif

    /* collect leaves on our way down */
    if ((cnt = oldtn->tn_NumLeaves)) {
        /* collect hits */
        LONG *leafptr = oldtn->tn_Leaves;
        do {
            assert((ULONG)*leafptr < m_TotalRawSize);
            // TODO FIXME ADD Levenshtein hit or hamming hit
            add_query_hit(sq, sqh, (ULONG) *leafptr++);
        } while (--cnt);
        /* if we got leaves, we're at the end of the tree */
        return;
    }

    //#ifdef DEBUG
    //  printf("%s=> CollectTreeRec: Qry %ld: ", indent,
    //    , sq->sq_State.sqs_QueryPos);
    //  tn = sq->sq_State.sqs_TreeNode;
    //  do {
    //   printf("%s", tn->tn_Edge);
    //   tn = tn->tn_Parent;
    //   if (tn)
    //    printf("-");
    //  } while (tn);
    //  printf(" (%f/%d/%d/%d)\n", sq->sq_State.sqs_ErrorCount,
    //    sq->sq_State.sqs_ReplaceCount, sq->sq_State.sqs_InsertCount,
    //    sq->sq_State.sqs_DeleteCount);
    //#endif

    // recurse on children
    struct TreeNode *tn;
    for (UWORD seqcode = 0; seqcode < m_as->alphabet_size; seqcode++) {
        if (oldtn->tn_Children[seqcode]) {
            tn = go_down_node_child_no_edge(oldtn, seqcode);
            sqh.sqh_State.sqs_TreeNode = tn;
            collect_tree_rec(sq, sqh);
            TreeNode::free_tree_node(tn);
        }
    }
    sqh.sqh_State.sqs_TreeNode = oldtn;
}

/*!
 * \brief Filter hits after matching, removing duplicate ones and
 *     hits crossing entry-border
 */
void PtpanTree::post_filter_query_hits(const SearchQuery& sq,
        SearchQueryHandle& sqh) const {
    if (sqh.sqh_hits.size() > 0) {

        sqh.sortByAbsPos();

        ULONG query_length = sq.sq_Query.size();
        struct PTPanEntry *ps = (struct PTPanEntry *) m_entries->lh_Head;
        boost::ptr_vector<QueryHit>::iterator it;

#ifdef DEBUG
        printf("before border crossing: hits %ld\n", sqh.sqh_hits.size());
#endif

        for (it = sqh.sqh_hits.begin(); it != sqh.sqh_hits.end();) {
            // check if current species is still valid
            if ((it->qh_AbsPos < ps->pe_AbsOffset)
                    || (it->qh_AbsPos
                            >= (ps->pe_AbsOffset + (ULLONG) ps->pe_RawDataSize))) {
                // go to next node by chance
                ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
                if (ps->pe_Node.ln_Succ) {
                    if ((it->qh_AbsPos < ps->pe_AbsOffset)
                            || (it->qh_AbsPos
                                    >= (ps->pe_AbsOffset
                                            + (ULLONG) ps->pe_RawDataSize))) {
                        /* still didn't match, so find it using the hard way */
                        ps = (struct PTPanEntry *) FindBinTreeLowerKey(
                                m_EntriesBinTree, it->qh_AbsPos);
                    }
                } else {
                    ps = (struct PTPanEntry *) FindBinTreeLowerKey(
                            m_EntriesBinTree, it->qh_AbsPos);
                }
            }
#ifdef DEBUG
            if ((it->qh_AbsPos < ps->pe_AbsOffset)
                    || (it->qh_AbsPos
                            >= (ps->pe_AbsOffset + (ULLONG) ps->pe_RawDataSize))) {
                printf(
                        "ERROR in PostFilterQueryHits (%s(%ld) Pos: %lld, Len %ld HitPos %lld)!\n",
                        ps->pe_Name, ps->pe_Num, ps->pe_AbsOffset,
                        ps->pe_RawDataSize, it->qh_AbsPos);
            }
#endif
            // filter alignment crossing hits
            if (it->qh_AbsPos - ps->pe_AbsOffset
                    > ps->pe_RawDataSize - query_length) {
                if (sqh.sqh_HitsHash) {
                    sqh.sqh_HitsHash->erase(it->qh_CorrectedAbsPos);
                }
                it = sqh.sqh_hits.erase(it);
            } else {
                it->qh_Entry = ps;
                it++;
            }
        }

#ifdef DEBUG
        printf("after border crossing: hits %ld\n", sqh.sqh_hits.size());
#endif

        if (sqh.sqh_HitsHash) {
            // in case of Levenshtein
            it = sqh.sqh_hits.begin();
            boost::ptr_vector<QueryHit>::iterator next_it;
            if (it != sqh.sqh_hits.end()) {
                next_it = it++;
            } else {
                next_it = it;
            }
#ifdef DEBUG
            ULONG count_equal = 0;
#endif
            for (; next_it != sqh.sqh_hits.end();) {
                if (next_it->qh_AbsPos == it->qh_AbsPos) {
#ifdef DEBUG
                    count_equal++;
#endif
                    if (next_it->qh_ErrorCount < it->qh_ErrorCount) {
                        sqh.sqh_HitsHash->erase(it->qh_CorrectedAbsPos);
                        it = sqh.sqh_hits.erase(it);
                        next_it = it++;
                    } else if (next_it->qh_ErrorCount == it->qh_ErrorCount) {
                        if ((next_it->qh_InsertCount + next_it->qh_DeleteCount)
                                < (it->qh_InsertCount + it->qh_DeleteCount)) {
                            sqh.sqh_HitsHash->erase(it->qh_CorrectedAbsPos);
                            it = sqh.sqh_hits.erase(it);
                            next_it = it++;
                        } else {
                            sqh.sqh_HitsHash->erase(
                                    next_it->qh_CorrectedAbsPos);
                            next_it = sqh.sqh_hits.erase(next_it);
                        }
                    } else {
                        sqh.sqh_HitsHash->erase(next_it->qh_CorrectedAbsPos);
                        next_it = sqh.sqh_hits.erase(next_it);
                    }
                } else {
                    // nothing to delete, so increase both iterators
                    it = next_it;
                    next_it++;
                }
            }
#ifdef DEBUG
            printf("equal = %lu (count after: %lu)\n", count_equal,
                    sqh.sqh_hits.size());
#endif
        }
    }
}

/*!
 * \brief Calculate error count
 *
 * \param sq SearchQuery with hits to calculate error count for
 * \param maxlen Maximum length we need to examine
 */
void PtpanTree::validate_unsafe_hits(const SearchQuery& sq,
        SearchQueryHandle& sqh) const {

    std::string source_seq = std::string(sqh.sqh_MaxLength + 1, 0);
    ULONG count;
    UBYTE code;
    ULONG tmp;
    struct PTPanEntry *pe = NULL;
    ULONG bitpos;
    ULONG abspos;

    boost::ptr_vector<QueryHit>::iterator it;
    for (it = sqh.sqh_hits.begin(); it != sqh.sqh_hits.end();) {
        bool erase_it = false;
        //#ifdef DEBUG
        //        if ((it->qh_InsertCount - it->qh_DeleteCount) > 2) {
        //            printf("Found not marked unsafe one %ld %ld %ld\n",
        //                    it->qh_ReplaceCount, it->qh_InsertCount,
        //                    it->qh_DeleteCount);
        //        }
        //#endif
        if (it->qh_Flags & QHF_UNSAFE) {
            //#ifdef DEBUG
            //            printf("Found unsafe one %ld %ld %ld\n", it->qh_ReplaceCount,
            //                    it->qh_InsertCount, it->qh_DeleteCount);
            //#endif
            bitpos = 0;
            pe = it->qh_Entry;
            abspos = it->qh_AbsPos - pe->pe_AbsOffset;

            if (pe->pe_CompressedShortcuts) {
                long cnt = -1;
                while ((ULONG) (cnt + 1) < pe->pe_CompressedShortcutsCount
                        && pe->pe_CompressedShortcuts[cnt + 1].pcs_AbsPosition
                                < abspos) {
                    cnt++;
                }
                // shortcuts to leap over some unnecessary compressed bits
                if (cnt >= 0) {
                    bitpos = pe->pe_CompressedShortcuts[cnt].pcs_BitPosition;
                    abspos -= pe->pe_CompressedShortcuts[cnt].pcs_AbsPosition;
                }
            }

            while (bitpos < pe->pe_SeqDataCompressedSize) { // get relpos
                code = m_as->getNextCharacter(pe->pe_SeqDataCompressed, bitpos,
                        count);
                if (m_as->seq_code_valid_table[code]) { // it's a valid char
                    if (!(abspos--)) {
                        break; // position found
                    }
                } else {
                    // it's not a valid char
                    assert((code == '.') || (code == '-'));
                }
            }

            assert(bitpos < pe->pe_SeqDataCompressedSize);
            bitpos -= 3; // bitpos now points to the first character of found seq
            // we need to verify the hit?
            while (it->qh_Flags & QHF_UNSAFE) {
                ULONG tarlen = copy_sequence(sq, source_seq, *it, bitpos, &tmp,
                        count);
                if (!match_sequence(sq, sqh, source_seq, *it)) {
                    erase_it = true;
                    break;
                }
                if (!(it->qh_ErrorCount == sqh.sqh_State.sqs_ErrorCount
                        && it->qh_ReplaceCount == sqh.sqh_State.sqs_ReplaceCount
                        && it->qh_InsertCount == sqh.sqh_State.sqs_InsertCount
                        && it->qh_DeleteCount == sqh.sqh_State.sqs_DeleteCount
                        && it->qh_nmismatch == sqh.sqh_State.sqs_NCount)) {
                    it->qh_ErrorCount = sqh.sqh_State.sqs_ErrorCount;
                    it->qh_ReplaceCount = sqh.sqh_State.sqs_ReplaceCount;
                    it->qh_InsertCount = sqh.sqh_State.sqs_InsertCount;
                    it->qh_DeleteCount = sqh.sqh_State.sqs_DeleteCount;
                    it->qh_nmismatch = sqh.sqh_State.sqs_NCount;
                    it->qh_WMisCount = sqh.sqh_State.sqs_WMisCount;
                }
                if (tarlen
                        == (ULONG) (sq.sq_Query.size() - it->qh_DeleteCount
                                + it->qh_InsertCount)) {
                    it->qh_Flags &= ~QHF_UNSAFE;
                }
            }
        }
        if (erase_it) {
            if (sqh.sqh_HitsHash) {
                sqh.sqh_HitsHash->erase(it->qh_CorrectedAbsPos);
            }
            it = sqh.sqh_hits.erase(it);
        } else {
            it++;
        }
    }
}

/*!
 * \brief Sort hits list
 *
 * \param sq SearchQuery with hits to sort
 */
void PtpanTree::sort_hits_list(const SearchQuery& sq,
        SearchQueryHandle& sqh) const {
    if (sqh.sqh_hits.size() > 1) {
        // enter priority and sort
        boost::ptr_vector<QueryHit>::iterator qh;
        if (!sq.sq_WeightedSearchMode) {
            /* sorting criteria:
             - normal/composite (1 bit)
             - replace only or insert/delete (1 bit)
             - mismatch count (5 bits)
             - error count (8 bits)
             - species (20 bits)
             - absolute position (28 bits)
             */
            //printf("Sort no weight...\n");
            for (qh = sqh.sqh_hits.begin(); qh != sqh.sqh_hits.end(); qh++) {
                assert(
                        ((LLONG) (qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount)) <= 0x1f);
                // 5 bit
                assert(((LLONG) round(qh->qh_ErrorCount * 10.0)) <= 0xff);
                //  8 bit
                assert(((LLONG) qh->qh_Entry->pe_Num) <= 0xfffff);
                // 20 bit
                assert(
                        ((LLONG) (qh->qh_AbsPos - qh->qh_Entry->pe_AbsOffset)) <= 0xfffffff);
                // 28 bit
                qh->qh_sortKey =
                        (LLONG) (
                                (qh->qh_Flags & QHF_REVERSED) ?
                                        (1LL << 62) : 0LL)
                                + ((qh->qh_InsertCount | qh->qh_DeleteCount) ?
                                        (1LL << 61) : 0LL)
                                + (((LLONG) (qh->qh_ReplaceCount
                                        + qh->qh_InsertCount
                                        + qh->qh_DeleteCount)) << 56)
                                + (((LLONG) qh->qh_nmismatch) << 53)
                                + (((LLONG) round(qh->qh_ErrorCount * 10.0))
                                        << 48)
                                + (((LLONG) round(qh->qh_WMisCount * 10.0))
                                        << 45)
                                + (((LLONG) qh->qh_Entry->pe_Num) << 25)
                                + ((LLONG) (qh->qh_AbsPos
                                        - qh->qh_Entry->pe_AbsOffset));
            }
        } else {
            //printf("Sort with weight...\n");
            for (qh = sqh.sqh_hits.begin(); qh != sqh.sqh_hits.end(); qh++) {
                assert(
                        ((LLONG) (qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount)) <= 0x1f);
                // 5 bit
                assert(((LLONG) round(qh->qh_ErrorCount * 10.0)) <= 0xff);
                //  8 bit
                assert(((LLONG) qh->qh_Entry->pe_Num) <= 0xfffff);
                // 20 bit
                assert(
                        ((LLONG) (qh->qh_AbsPos - qh->qh_Entry->pe_AbsOffset)) <= 0xfffffff);
                // 28 bit
                qh->qh_sortKey =
                        (LLONG) (
                                (LLONG) (qh->qh_Flags & QHF_REVERSED) ?
                                        (1LL << 62) : 0LL)
                                + ((LLONG) (qh->qh_InsertCount
                                        | qh->qh_DeleteCount) ?
                                        (1LL << 61) : 0LL)
                                + (((LLONG) round(qh->qh_ErrorCount * 10.0))
                                        << 56)
                                + (((LLONG) qh->qh_nmismatch) << 53)
                                + (((LLONG) (qh->qh_ReplaceCount
                                        + qh->qh_InsertCount
                                        + qh->qh_DeleteCount)) << 48)
                                + ((LLONG) qh->qh_Entry->pe_Num << 28)
                                + ((LLONG) (qh->qh_AbsPos
                                        - qh->qh_Entry->pe_AbsOffset));
            }
        }
        // finally sort by key
        sqh.sqh_hits.sort(QueryHit::keyCompare);
    }
}

/*!
 * \brief Build differential alignment for hits in SearchQuery
 *
 * \param sq pointer to SearchQuery
 * \param maxlen Maximum length we need to examine
 * \param offset offset of the thread (default = 0)
 * \param participators Number of concurrent threads working (default = 1)
 */
void PtpanTree::create_diff_alignment(const SearchQuery& sq,
        SearchQueryHandle& sqh, int offset, int participators) const {

    std::string source_seq = std::string(sqh.sqh_MaxLength + 1, 0);
    // copy SearchQueryHandle because of the state machine inside
    // a shallow copy is sufficient!
    SearchQueryHandle local_handle = SearchQueryHandle(sqh);

    ULONG query_length = sq.sq_Query.size();
    struct PTPanEntry *pe = NULL;
    char prefix[10], postfix[10]; // we use this as it is good for the cache!
    ULONG count = 0;
    UBYTE code = 0;
    LONG relpos = 0;
    ULONG nmismatch = 0;
    ULONG abspos = 0;
    ULONG bitpos = 0;
    QueryHit *it = NULL;
    STRPTR seqout = NULL;

    // TODO FIXME idea: one fixed size buffer (cache) and always copy to seqout!

    prefix[9] = '-'; // 1st delimiter (never changes!)
    postfix[0] = '-'; // 2nd delimiter (never changes!)

    for (ULONG i = offset; i < sqh.sqh_hits.size(); i += participators) {
        it = &(sqh.sqh_hits.at(i));

        pe = it->qh_Entry;
        relpos = 0;
        abspos = it->qh_AbsPos - pe->pe_AbsOffset;
        bitpos = 0;

        if (abspos >= 9) {
            if (pe->pe_CompressedShortcuts) {
                long cnt = -1;
                // correct search by 9 characters -> prefix of differential alignment!
                while ((ULONG) (cnt + 1) < pe->pe_CompressedShortcutsCount
                        && pe->pe_CompressedShortcuts[cnt + 1].pcs_AbsPosition
                                < (abspos - 9)) {
                    cnt++;
                }
                // shortcuts to leap over some unnecessary compressed bits
                if (cnt >= 0) {
                    bitpos = pe->pe_CompressedShortcuts[cnt].pcs_BitPosition;
                    abspos -= pe->pe_CompressedShortcuts[cnt].pcs_AbsPosition;
                    relpos += pe->pe_CompressedShortcuts[cnt].pcs_RelPosition;
                }
            }
        } else {
            // fill prefix with dots if necessary
            for (int i = 0; i < 9; i++) {
                prefix[i] = '.';
            }
        }

        // given an absolute sequence position, search for the relative one,
        // e.g. abspos 2 on "-----UU-C-C" will yield 8
        // abspos:           01 2 3
        // relpos:      0123456789a
        while (true) { // we check this anyways after the loop
            // it's RISKY, but FASTER! and if this fails, the data is corrupt anyways
            // while (bitpos < pe->pe_SeqDataCompressedSize) { // old while statement
            // get relpos and store prefix
            code = m_as->getNextCharacter(pe->pe_SeqDataCompressed, bitpos,
                    count);
            if (m_as->seq_code_valid_table[code]) { // it's a valid char
                if (!(abspos--)) {
                    break; // position found
                }
                if (abspos <= 8) {
                    prefix[8 - abspos] = code; // store prefix
                }
                ++relpos;
            } else { // it's not a valid char
                assert((code == '.') || (code == '-'));
                relpos += count;
                if ((code == '.') && (abspos <= 9)) { // fill prefix with '.'
                    for (ULONG i = 0; i < (9 - abspos); ++i) {
                        prefix[i] = '.';
                    }
                }
            }
        }
        //..
        assert(bitpos < pe->pe_SeqDataCompressedSize);
        bitpos -= 3; // bitpos now points to the first character of found seq

        ULONG tarlen = copy_sequence(sq, source_seq, *it, bitpos, &nmismatch,
                count);

        if (nmismatch > sq.sq_KillNSeqsAt) {
            // TODO FIXME throw error!
        } else {
            it->qh_seqout.resize(9 + 1 + query_length + 1 + 9);
            seqout = (STRPTR) it->qh_seqout.data();
            strncpy(seqout, prefix, 10); // copy prefix
            // create appropriate output
            if (it->qh_ErrorCount == 0) {
                if (it->qh_nmismatch == 0) {
                    STRPTR out = &seqout[10];
                    for (ULONG i = 0; i < query_length; i++) {
                        *out++ = '=';
                    }
                } else {
                    // N-mismatch only, no substitutions
                    STRPTR out = &seqout[10];
                    for (ULONG i = 0; i < query_length; i++) {
                        if (!m_as->compress_table[(int) source_seq[i]]) {
                            *out++ = source_seq[i];
                        } else {
                            *out++ = '=';
                        }
                    }
                }
            } else {
                local_handle.sqh_State.reset();
                if (it->qh_InsertCount > 0 || it->qh_DeleteCount > 0) {
                    // we have indels, so it's a Levenshtein
                    if (!levenshtein_match(sq, local_handle, source_seq,
                            &seqout[10], true)) {
                        // TODO FIXME throw error!
                    }
                } else {
                    // no indels, so it's a Hamming
                    if (!hamming_match(sq, local_handle, source_seq,
                            &seqout[10], true)) {
                        // TODO FIXME throw error!
                    }
                }
            }
            ULONG cnt = 1;
            for (; cnt < 10;) {
                // generate postfix
                code = m_as->getNextCharacter(pe->pe_SeqDataCompressed, bitpos,
                        count);
                if (code == 0xff) {
                    // end of sequence reached!
                    break;
                }
                if (m_as->seq_code_valid_table[code]) {
                    // valid character
                    postfix[cnt++] = code;
                } else if (code == '.') {
                    // '.' found
                    for (; cnt < 10; ++cnt) {
                        // fill postfix with '.'
                        postfix[cnt] = '.';
                    }
                }
            }
            // fill end with dots if necessary
            for (; cnt < 10; cnt++) {
                postfix[cnt] = '.';
            }
            strncpy(&seqout[10 + query_length], postfix, 10); // copy postfix
            // set relpos and nmismatch
            it->qh_relpos = relpos;
            it->qh_nmismatch = nmismatch;
        }
    }
}

/*!
 * \brief Find probe candidates in given partition for given length
 *
 * \param length  ULONG length of proves; must not exceed prune length!
 * \param results results are put directly into vector
 */
void PtpanTree::find_probes_rec(struct TreeNode *parent_tn,
        std::vector<std::string>& results, ULONG depth, ULONG length) const {
    struct TreeNode *tn = NULL;

    for (UWORD seqcode = m_as->wildcard_code + 1; seqcode < m_as->alphabet_size;
            seqcode++) {
        if (parent_tn->tn_Children[seqcode]) {
            tn = go_down_node_child(parent_tn, seqcode);

            // check if length exceeded or if N inside edge
            if (!m_as->checkUncompressedForWildcard(tn->tn_Edge)) {
                // no wildcard in edge ...
                if (depth + tn->tn_EdgeLen < length) {
                    // recurse as we did not reach appropriate depth!
                    find_probes_rec(tn, results, depth + tn->tn_EdgeLen,
                            length);
                    // NOTE: as we filter longer requests before, we do not need
                    // to check if this is a border node as in this
                    // case, the length will be already reached!
                } else {
                    // collect edge!
                    results.push_back(get_tree_path(tn, length));
                }
            }
            TreeNode::free_tree_node(tn);
        } // end branch mask check
    } // end for loop
}

/*!
 * \brief Find probe candidates in given partition
 *
 * \param pp PTPanPartition
 * \param dq DesignQuery
 * \param dqh DesignQueryHandle
 *
 * TODO FIXME: gc content evaluation is still hardcoded :-(
 */
void PtpanTree::find_probe_in_partition(struct PTPanPartition *pp,
        const DesignQuery& dq, DesignQueryHandle& dqh) const {

#ifdef DEBUG
    bool nohits = true;
#endif
    struct TreeNode *parenttn;
    ULONG cnt;
    STRPTR edgeptr;
    bool abortpath;
    ULONG pathleft;
    struct TreeNode *tn = read_packed_node(pp, 0); // get root node

    /* collect all the strings that are on our way */
    bool done = false;
    UWORD seqcode = m_as->wildcard_code + 1;
    ULONG length =
            (dq.dq_ProbeLength < pp->pp_TreePruneLength) ?
                    dq.dq_ProbeLength : pp->pp_TreePruneLength;
    double currtemp = 0.0;
    UWORD currgc = 0;
    do {
        //printf("Cnt: %ld\n", cnt);
        /* go down! */
        while (tn->tn_TreeOffset < length) {
            while (seqcode < m_as->alphabet_size) {
                //printf("Seqcode %d %ld\n", seqcode, tn->tn_Children[seqcode]);
                if (tn->tn_Children[seqcode]) {
                    /* there is a child, go down */
                    tn = go_down_node_child(tn, seqcode);

                    abortpath = false;
                    /* do some early checks */
                    edgeptr = tn->tn_Edge;
                    if (tn->tn_TreeOffset < length) {
                        pathleft = length - tn->tn_TreeOffset;
                        cnt = 0;
                    } else {
                        cnt = tn->tn_TreeOffset - tn->tn_EdgeLen;
                        pathleft = 0;
                    }
                    while (*edgeptr && (cnt++ < length)) {
                        // find a better way for the switch case as this
                        // is alphabet specific stuff !
                        switch (*edgeptr++) {
                        case 'C':
                        case 'c':
                        case 'G':
                        case 'g':
                            currtemp += 4.0;
                            currgc++;
                            break;
                        case 'A':
                        case 'a':
                        case 'T':
                        case 't':
                        case 'U':
                        case 'u':
                            currtemp += 2.0;
                            break;
                        default:
                            //printf("N seq\n");
                            abortpath = true;
                            break;
                        }
                    }

                    if ((currtemp > dq.dq_MaxTemp + EPSILON) || // check temperature out of range
                            (currtemp + (4.0 * pathleft)
                                    < dq.dq_MinTemp - EPSILON)) {
                        /*printf("temp %f <= [%f] <= %f out of range!\n",
                         dq->dq_MinTemp, currtemp, dq->dq_MaxTemp);*/
                        abortpath = true;
                    }
                    if ((currgc > dq.dq_MaxGC) || // check gc content
                            (currgc + pathleft < dq.dq_MinGC)) {
                        /*printf("gc content %ld <= [%ld] <= %ld out of range!\n",
                         dq->dq_MinGC, currgc, dq->dq_MaxGC);*/
                        abortpath = true;
                    }
                    if (abortpath) // abort path processing here
                    {
                        //GetTreePath(tn, buf, 24);
                        //printf("Path aborted: %s\n", buf);
                        seqcode = m_as->alphabet_size;
                        break;
                    }
                    //printf("Down %d %08lx\n", seqcode, tn);
                    seqcode = m_as->wildcard_code + 1; // equals A in DNA5!
                    break;
                }
                seqcode++;
            }

            while (seqcode == m_as->alphabet_size) { // we didn't find any children
                /* go up again */
                //printf("Up\n");
                /* undo temperature and GC */
                edgeptr = tn->tn_Edge;
                cnt = tn->tn_TreeOffset - tn->tn_EdgeLen;
                while (*edgeptr && (cnt++ < length)) {
                    // find a better way for the switch case as this
                    // is alphabet specific stuff !
                    switch (*edgeptr++) {
                    case 'C':
                    case 'c':
                    case 'G':
                    case 'g':
                        currtemp -= 4.0;
                        currgc--;
                        break;
                    case 'A':
                    case 'a':
                    case 'T':
                    case 't':
                    case 'U':
                    case 'u':
                        currtemp -= 2.0;
                        break;
                    }
                }
                parenttn = tn->tn_Parent;
                seqcode = tn->tn_ParentSeq + 1;
                TreeNode::free_tree_node(tn);
                tn = parenttn;
                if (!tn) {
                    /* we're done with this partition */
                    done = true;
                    break; /* we're done! */
                }
            }
            if (done) {
                break;
            }
        }

        if (done) {
            break;
        }
#ifdef DEBUG
        nohits = false;
#endif
        // now examine subtree for valid hits
        dqh.dqh_TreeNode = tn;
        add_design_hit(dq, dqh);

        // undo temperature and GC
        edgeptr = tn->tn_Edge;
        cnt = tn->tn_TreeOffset - tn->tn_EdgeLen;
        while (*edgeptr && (cnt++ < length)) {
            // find a better way for the switch case as this
            // is alphabet specific stuff !
            switch (*edgeptr++) {
            case 'C':
            case 'c':
            case 'G':
            case 'g':
                currtemp -= 4.0;
                currgc--;
                break;
            case 'A':
            case 'a':
            case 'T':
            case 't':
            case 'U':
            case 'u':
                currtemp -= 2.0;
                break;
            }
        }
        parenttn = tn->tn_Parent;
        seqcode = tn->tn_ParentSeq + 1;
        TreeNode::free_tree_node(tn);
        tn = parenttn;
    } while (true);

#ifdef DEBUG
    if (nohits) {
        printf(
                "Warning: Temperature and GC content limitations already outrule any potential hits!\n");
    }
#endif
}

/*!
 * \brief Method to check probe candidate stats with help of the index
 *
 * \param dq
 * \param dqh
 * \param offset
 * \param participators
 */
void PtpanTree::design_probes(const DesignQuery& dq, DesignQueryHandle& dqh,
        int offset, int participators) const {
    struct MismatchWeights* mws = MismatchWeights::cloneMismatchWeightMatrix(
            m_as->mismatch_weights);
    SearchQuery sq = SearchQuery(mws); // note: SearchQuery take ownership of MismatchWeights!
    sq.sq_Query = std::string(dq.dq_ProbeLength, 'N'); // we need a dummy sequence!
    sq.sq_MaxErrors = (float) DECR_TEMP_CNT * PROBE_MISM_DEC;
    sq.sq_WeightedSearchMode = true;
    sq.sq_AllowReplace = true;
    sq.sq_AllowInsert = true;
    sq.sq_AllowDelete = true;
    sq.sq_KillNSeqsAt = dq.dq_ProbeLength / 3;

    SearchQueryHandle sqh = SearchQueryHandle(sq);

#ifdef DEBUG
    ULONG number = 1;
#endif

    DesignHit* dh;
    ULONG hitpos;
    std::vector<PTPanPartition *>::const_iterator pit;
    for (ULONG i = offset; i < dqh.dqh_Hits.size(); i += participators) {
        dh = &(dqh.dqh_Hits.at(i));

        // reset values to new DesignHit
        sq.sq_Query = dh->dh_ProbeSeq;
        sqh.reset();
        // search over partitions
        for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
            // search normal
            search_partition(*pit, sq, sqh); // TODO FIXME do this num_threads times in parallel
            // TODO FIXME it is no good to create a SearchQuery for each candidate
            // as this may result in way to large memory consumption, so we go over
            // all partitions parallel and then all threads increment, and then again
            // every thread goes over all partitions again
            // -> we need a barrier! boost::barrier(num-participants)
        }
        post_filter_query_hits(sq, sqh);
        // check all hits
        boost::ptr_vector<QueryHit>::iterator qh;
        for (qh = sqh.sqh_hits.begin(); qh != sqh.sqh_hits.end(); qh++) {
            if ((dqh.dqh_MarkedEntryNumbers.count(qh->qh_Entry->pe_Num) == 0)
                    || (dqh.dqh_handleFeatures
                            && !dqh.hitsAnyFeature(qh->qh_AbsPos, qh->qh_Entry,
                                    dq))) {
                hitpos = (ULONG) (qh->qh_ErrorCount * (1.0 / PROBE_MISM_DEC));
                if (((ULONG) floor(qh->qh_ErrorCount * 10)) % 2 == 0) {
                    // this is done to achieve <= instead of < without it!
                    hitpos--;
                }
                if (hitpos < DECR_TEMP_CNT) {
                    dh->dh_NonGroupHitsPerc[hitpos]++;
                }
            }
        }
#ifdef DEBUG
        printf("Probe (%ld) %s \n", number++, dh->dh_ProbeSeq.data());
        //            ULONG sum = 0;
        //            for (hitpos = 0; hitpos < DECR_TEMP_CNT; hitpos++) {
        //                sum += dh->dh_NonGroupHitsPerc[hitpos];
        //                printf("%ld ", sum);
        //            }
        //            printf("\n");
#endif
        {
            // calculate relative position position if reference entry
            // is available (and thus alignment data)
            boost::ptr_vector<HitTuple>::const_iterator tuple =
                    dh->dh_Matches.begin();
            if (tuple != dh->dh_Matches.end()) {
                struct PTPanEntry *ps = tuple->ht_Entry;
                LONG relpos = 0;
                ULONG abspos = tuple->ht_AbsPos - ps->pe_AbsOffset;
                UBYTE code;
                ULONG bitpos = 0;
                ULONG count;
                while (bitpos < ps->pe_SeqDataCompressedSize) { // get relpos
                    code = m_as->getNextCharacter(ps->pe_SeqDataCompressed,
                            bitpos, count);
                    if (m_as->seq_code_valid_table[code]) { // it's a valid char
                        if (!(abspos--)) {
                            break; // position found
                        }
                        ++relpos;
                    } else { // it's not a valid char
                        relpos += count;
                    }
                }
                dh->dh_relpos = relpos;
            } else {
                dh->dh_relpos = 0;
            }
            ULONG check_pos = dh->dh_relpos;
            if (m_ref_entry) {
                if (check_pos < m_ref_entry->pre_ReferenceSeqSize) {
                    check_pos = m_ref_entry->pre_ReferenceBaseTable[check_pos];
                } else {
                    check_pos =
                            m_ref_entry->pre_ReferenceBaseTable[m_ref_entry->pre_ReferenceSeqSize];
                }
            }
            if ((LONG) check_pos < dq.dq_MinPos
                    || ((LONG) check_pos > dq.dq_MaxPos && dq.dq_MaxPos != -1)) {
                // we must delay deletion as we may have more then one threads!
                dh->dh_valid = false;
            } else {
                dh->dh_valid = true;
                dh->dh_ref_pos = check_pos;
            }
        }
    }
}

/*!
 * \brief Calculate Probe candidate quality
 */
void PtpanTree::calculate_probe_quality(DesignQueryHandle& dqh, int offset,
        int participators) const {
    DesignHit* dh;
    for (ULONG i = offset; i < dqh.dqh_Hits.size(); i += participators) {
        dh = &(dqh.dqh_Hits.at(i));
        int i;
        for (i = 0; i < DECR_TEMP_CNT - 1; i++) {
            if (dh->dh_NonGroupHitsPerc[i] > dh->dh_NonGroupHits) {
                break;
            }
        }
        dh->dh_Quality = ((double) dh->dh_GroupHits * i)
                + 1000.0 / (1000.0 + dh->dh_NonGroupHitsPerc[i]);
    }
}

/*!
 * \brief add probe design hit to DesignQuery
 *
 * \param dq DesignQuery
 * \param dqh DesignQueryHandle
 *
 * TODO FIXME: gc content evaluation is still hardcoded :-(
 */
void PtpanTree::add_design_hit(const DesignQuery& dq,
        DesignQueryHandle& dqh) const {

    if (!dqh.dqh_TempHit) {
        dqh.dqh_TempHit = new DesignHit(dq.dq_ProbeLength);
    } else {
        dqh.dqh_TempHit->reset();
    }
    struct DesignHit *dh = dqh.dqh_TempHit;

    struct PTPanEntry *ps = NULL;
    ULONG cnt;
    bool take = true;
    struct TreeNode *tn = dqh.dqh_TreeNode;
    struct TreeNode *parenttn = NULL;

    // now iterate through the lower parts of the tree, collecting all leaf nodes
    UBYTE seqcode = m_as->wildcard_code;
    bool done = false;
    do {
        while (seqcode < m_as->alphabet_size) {
            //printf("Seqcode %d %ld\n", seqcode, tn->tn_Children[seqcode]);
            if (tn->tn_Children[seqcode]) {
                /* there is a child, go down */
                tn = go_down_node_child_no_edge(tn, seqcode);
                seqcode = m_as->wildcard_code;
                //printf("Down %d %08lx\n", seqcode, tn);
            } else {
                seqcode++;
            }
        }

        while (seqcode == m_as->alphabet_size) { // we didn't find any children
            // when going up, collect any leafs
            if (tn->tn_NumLeaves) {
                cnt = tn->tn_NumLeaves;
                // enter leaves
                LONG *leafptr = tn->tn_Leaves;
                do {
                    dh->dh_Matches.push_back(
                            new HitTuple((ULLONG) *leafptr++, NULL));
                } while (--cnt);
                /* if we got more matches than the sum of marked and nongroup hits,
                 it's very probable that we have exceeded non group hits, so check */
                if (dh->dh_Matches.size()
                        > dqh.dqh_MarkedEntryNumbers.size()
                                + dq.dq_MaxNonGroupHits) {
                    dh->dh_NonGroupHits = 0;

                    boost::ptr_vector<HitTuple>::iterator ht;
                    for (ht = dh->dh_Matches.begin();
                            ht != dh->dh_Matches.end(); ht++) {
                        if (!(ps = ht->ht_Entry)) {
                            ps = ht->ht_Entry =
                                    (struct PTPanEntry *) FindBinTreeLowerKey(
                                            m_EntriesBinTree, ht->ht_AbsPos);
                        }
                        /* check, if hit is really within sequence */
                        if (ht->ht_AbsPos + dq.dq_ProbeLength
                                < ps->pe_AbsOffset + ps->pe_RawDataSize) {

                            if ((dqh.dqh_MarkedEntryNumbers.count(
                                    ht->ht_Entry->pe_Num) == 0)
                                    || (dqh.dqh_handleFeatures
                                            && !dqh.hitsAnyFeature(
                                                    ht->ht_AbsPos, ht->ht_Entry,
                                                    dq))) {
                                // check if we are over the limit
                                if (++dh->dh_NonGroupHits
                                        > dq.dq_MaxNonGroupHits) {
                                    /* printf("NonGroupHits exceeded! Species: %s abspos: %lu, relpos: %lu\n",
                                     ps->pe_Name, ht->ht_AbsPos,
                                     ht->ht_AbsPos - ps->pe_AbsOffset);*/
                                    take = false;
                                    done = true;
                                    break;
                                }
                            }

                        }
                    }

                }
            }
            if (tn == dqh.dqh_TreeNode) {
                /* we're back at the top level where we started -- stop collecting */
                done = true;
                break;
            }
            /* go up again */
            //printf("Up\n");
            parenttn = tn->tn_Parent;
            seqcode = tn->tn_ParentSeq + 1;
            TreeNode::free_tree_node(tn);
            tn = parenttn;
        }
    } while (!done);

    while (tn != dqh.dqh_TreeNode) {
        parenttn = tn->tn_Parent;
        TreeNode::free_tree_node(tn);
        tn = parenttn;
    }

#ifdef DEBUG
    if (tn != dqh.dqh_TreeNode) {
        printf("BREAKBREAKBREAK\n");
    }
#endif

    // if the number of hits is smaller than the minimum number
    // of group hits, we don't need to verify, as the equation won't hold.
    if (dh->dh_Matches.size() < dq.dq_MinGroupHits) {
        //        printf("NumMatches %ld < MinGroupHits %ld\n",
        //            dh->dh_NumMatches, dq->dq_MinGroupHits);
        take = false;
    }

    /* verify non group hits and group hits limitations */
    if (take) {
        dh->dh_NonGroupHits = 0;
        dh->dh_GroupHits = 0;

        boost::unordered_set<ULONG> unique_hits;

        boost::ptr_vector<HitTuple>::iterator ht;
        for (ht = dh->dh_Matches.begin(); ht != dh->dh_Matches.end(); ht++) {
            if (!(ps = ht->ht_Entry)) {
                ps = ht->ht_Entry = (struct PTPanEntry *) FindBinTreeLowerKey(
                        m_EntriesBinTree, ht->ht_AbsPos);
            }
            //printf("%ld: %s %ld\n", cnt, ht->ht_Entry->pe_Name, ht->ht_AbsPos);
            // check, if hit is really within sequence or crosses border
            if (ht->ht_AbsPos + dq.dq_ProbeLength
                    < ps->pe_AbsOffset + ps->pe_RawDataSize) {

                if (dqh.dqh_MarkedEntryNumbers.count(ps->pe_Num) == 1) {
                    if (dqh.dqh_handleFeatures) {

                        // TODO FIXME if "has-marked-features"
                        // This would be required by a "mixed-mode" to design
                        // probes for whole genomes combined with some genes on others!

                        // check if it hits marked feature!!
                        const PTPanFeature* feature_ptr =
                                dqh.hitsAnyFeatureReturn(ht->ht_AbsPos,
                                        ht->ht_Entry, dq);
                        if (feature_ptr) {
                            std::size_t key = 0;
                            boost::hash_combine(key, ps->pe_Num);
                            boost::hash_combine(key, (ULONG) feature_ptr);
                            if (unique_hits.find(key) == unique_hits.end()) {
                                unique_hits.insert(key);
                            }

                        } else {
                            // TODO FIXME if only genomes with features marked are allowed this is ok as it is
                            // otherwise we want to add by calling "unique_hits.insert(ps->pe_Num);"!!
                            // or something like that after checking if no marked features exist!!
                            // or maybe we want to check "has-marked-features" above!

                            // It's a non-group hit as it misses the marked features
                            if (++dh->dh_NonGroupHits > dq.dq_MaxNonGroupHits) {
                                take = false;
                                break;
                            }
                        }
                    } else {
                        if (unique_hits.find(ps->pe_Num) == unique_hits.end()) {
                            unique_hits.insert(ps->pe_Num);
                        }
                    }
                } else {
                    // check if we are over the limit
                    if (++dh->dh_NonGroupHits > dq.dq_MaxNonGroupHits) {
                        // printf("NonGroupHits exceeded (2)! Entry: %s abspos: %lu, relpos: %lu\n",
                        // ps->pe_Name, ht->ht_AbsPos, ht->ht_AbsPos - ps->pe_AbsOffset);
                        take = false;
                        break;
                    }
                }
            }
        }
        dh->dh_GroupHits = unique_hits.size();

        if (dh->dh_GroupHits < dq.dq_MinGroupHits) {
            // printf("grouphits %ld < MinGroupHits %ld\n", dh->dh_GroupHits, dq->dq_MinGroupHits);
            take = false;
        }
    }
    if (take) {
        get_tree_path(tn, dh->dh_ProbeSeq, dq.dq_ProbeLength);

        dh->dh_Temp = 0.0; // calculate temperature
        dh->dh_GCContent = 0; // and GC content
        for (cnt = 0; cnt < dq.dq_ProbeLength; cnt++) {
            // find a better way for the switch case as this
            // is alphabet specific stuff !
            switch (dh->dh_ProbeSeq[cnt]) {
            case 'C':
            case 'c':
            case 'G':
            case 'g':
                dh->dh_Temp += 4.0;
                dh->dh_GCContent++;
                break;
            case 'A':
            case 'a':
            case 'T':
            case 't':
            case 'U':
            case 'u':
                dh->dh_Temp += 2.0;
                break;

            default:
                take = false; // ignore this probe -- it contains N sequences!
                cnt = dq.dq_ProbeLength; // abort
                break;
            }
        }
        if ((dh->dh_Temp < dq.dq_MinTemp - EPSILON)
                || (dh->dh_Temp > dq.dq_MaxTemp + EPSILON)) {
            take = false; // temperature was out of given range
        }
        if ((dh->dh_GCContent < dq.dq_MinGC)
                || (dh->dh_GCContent > dq.dq_MaxGC)) {
            take = false; // gc content was out of given range
        }
        if (!take) {
            throw std::runtime_error(
                    "PtpanTree::add_design_hit() Probe candidate not conform to settings!");
        }
#ifdef DEBUG
        printf("GOOD PROBE %s! (GroupHits=%ld, NonGroupHits=%ld)\n",
                dh->dh_ProbeSeq.data(), dh->dh_GroupHits, dh->dh_NonGroupHits);
#endif
        dqh.dqh_Hits.push_back(dh);
        dqh.dqh_TempHit = NULL; // set to NULL, we used it!
    }
}

/*!
 * \brief Private method for find family
 *
 * Creates probes of size window and matches them against ptpan tree(s)
 */
void PtpanTree::ss_window_sequence(const SimilaritySearchQuery& ssq,
        SimilaritySearchQueryHandle& ssqh) const {

    // TODO get parallel code running    if (ssqh.ssqh_filtered_seq_len < ssq.ssq_WindowSize || m_num_threads <= 1) {
    ss_window_search(ssq, ssqh);
//    } else {
//        std::size_t i = 0;
//        ULONG participators = m_num_threads;
//        for (; i < participators - 1; i++) {
//            boost::threadpool::schedule(
//                    *m_threadpool,
//                    boost::bind(&PtpanTree::ss_window_search, this,
//                            boost::ref(ssq), boost::ref(ssqh), i,
//                            participators));
//        }
//        ss_window_search(ssq, ssqh, i, participators); // main thread can do some work as well!
//        m_threadpool->wait();
//    }
}

/*!
 * \brief Conduct the search itself (once for every thread participating)
 *
 * \param ssq
 * \param ssqh
 * \param offset
 * \param participators
 */
void PtpanTree::ss_window_search(const SimilaritySearchQuery& ssq,
        SimilaritySearchQueryHandle& ssqh, int offset,
        int participators) const {
    MismatchWeights *mw = MismatchWeights::cloneMismatchWeightMatrix(
            m_as->mismatch_weights);
    SearchQuery sq = SearchQuery(mw);
    sq.sq_Query = std::string(ssq.ssq_WindowSize, 'N'); // set dummy query string
    sq.sq_MaxErrors = ssq.ssq_NumMaxMismatches;
    sq.sq_Reversed = false;
    if (sq.sq_MaxErrors == 0.0) {
        sq.sq_AllowReplace = false;
        sq.sq_AllowInsert = false;
        sq.sq_AllowDelete = false;
    } else {
        sq.sq_AllowReplace = true;
        sq.sq_AllowInsert = true; // TODO: really allow Insert
        sq.sq_AllowDelete = true; //       and Delete?
    }
    sq.sq_KillNSeqsAt = ssq.ssq_WindowSize / 3; // TODO: limit N in hits?
    sq.sq_WeightedSearchMode = false; // TODO: use weighted mismatch?

    SearchQueryHandle sqh = SearchQueryHandle(sq);

    AbstractQueryFilter *filter = NULL;
    if (ssq.ssq_RangeEnd != -1 || ssq.ssq_RangeStart != -1) {
        // TODO FIXME filter = (correct-cast-to AbstractQueryFilter *) new RichBaseRangeQueryFilter(...);
    }

    for (ULONG i = offset; i < ssqh.ssqh_filtered_seq_len - ssq.ssq_WindowSize;
            i += participators) {
        if ((ssq.ssq_SearchMode == 0) || (ssqh.ssqh_filtered_seq[i] == 'A')
                || (ssqh.ssqh_filtered_seq[i] == 'a')) {
            // direct copy to search sequence (which is the same size every time!)
            memcpy(const_cast<STRPTR>(sq.sq_Query.data()),
                    &ssqh.ssqh_filtered_seq[i], ssq.ssq_WindowSize);
            sqh.reset();
            { // search all occurrences and increase hitcounts!
                std::vector<PTPanPartition *>::const_iterator pit;
                for (pit = m_partitions.begin(); pit != m_partitions.end();
                        pit++) {
                    search_partition(*pit, sq, sqh);
                    // TODO FIXME in parallel mode, we may want a barrier here!?
                }
                post_filter_query_hits(sq, sqh);

                boost::ptr_vector<QueryHit>::iterator qh;
                for (qh = sqh.sqh_hits.begin(); qh != sqh.sqh_hits.end();
                        qh++) {
                    if (!filter || filter->matchesFilter(*qh)) {
                        ssqh.ssqh_hit_count[qh->qh_Entry->pe_Num].fetch_add(1,
                                boost::memory_order_relaxed);
                    }
                }
            }
        }
    }
}

/*!
 * \brief Calculate relative error count
 *
 * \param ssq
 * \param ssqh
 */
void PtpanTree::ss_create_hits(const SimilaritySearchQuery& ssq,
        SimilaritySearchQueryHandle& ssqh) const {
    for (ULONG i = 0; i < m_entry_count; ++i) {
        ULONG all_len = std::min(m_EntriesMap[i]->pe_RawDataSize,
                ssqh.ssqh_filtered_seq_len) - ssq.ssq_WindowSize + 1;
        if (all_len <= 0) {
            ssqh.ssqh_rel_hit_count[i] = 0.0;
        } else {
            ssqh.ssqh_rel_hit_count[i] = ssqh.ssqh_hit_count[i]
                    / (double) all_len;
        }
        ssqh.ssqh_Hits.push_back(
                new SimilaritySearchHit(ssqh.ssqh_hit_count[i],
                        ssqh.ssqh_rel_hit_count[i], m_EntriesMap[i]));
    }
}
