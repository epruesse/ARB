/*!
 * \file ptpan.cxx
 *
 * \date ??.??.??
 * \author Tilo Eissler
 */

#include "ptpan.h"

/*!
 * Default constructor.
 */
QueryHit::QueryHit() :
        qh_AbsPos(0), qh_CorrectedAbsPos(0), qh_Flags(0), qh_ReplaceCount(0), qh_InsertCount(
                0), qh_DeleteCount(0), qh_nmismatch(0), qh_WMisCount(0.0), qh_ErrorCount(
                0.0), qh_Entry(NULL), qh_seqout(), qh_relpos(0), qh_sortKey(0) {
}

/*!
 * \brief Copy constructor.
 */
QueryHit::QueryHit(const QueryHit& hit) :
        qh_AbsPos(hit.qh_AbsPos), qh_CorrectedAbsPos(hit.qh_CorrectedAbsPos), qh_Flags(
                hit.qh_Flags), qh_ReplaceCount(hit.qh_ReplaceCount), qh_InsertCount(
                hit.qh_InsertCount), qh_DeleteCount(hit.qh_DeleteCount), qh_nmismatch(
                hit.qh_nmismatch), qh_WMisCount(hit.qh_WMisCount), qh_ErrorCount(
                hit.qh_ErrorCount), qh_Entry(hit.qh_Entry), qh_seqout(
                hit.qh_seqout), qh_relpos(hit.qh_relpos), qh_sortKey(
                hit.qh_sortKey) {
}

/*!
 * \brief Destructor.
 */
QueryHit::~QueryHit() {
}

/*!
 * \brief The assignment operator.
 *
 * \param hit The QueryHit to copy
 */
QueryHit& QueryHit::operator =(const QueryHit & hit) {
    if (this != &hit) {
        qh_AbsPos = hit.qh_AbsPos;
        qh_CorrectedAbsPos = hit.qh_CorrectedAbsPos;
        qh_Flags = hit.qh_Flags;
        qh_ReplaceCount = hit.qh_ReplaceCount;
        qh_InsertCount = hit.qh_InsertCount;
        qh_DeleteCount = hit.qh_DeleteCount;
        qh_nmismatch = hit.qh_nmismatch;
        qh_WMisCount = hit.qh_WMisCount;
        qh_ErrorCount = hit.qh_ErrorCount;
        qh_Entry = hit.qh_Entry;
        qh_seqout = hit.qh_seqout;
        qh_relpos = hit.qh_relpos;
        qh_sortKey = hit.qh_sortKey;
    }
    return *this;
}

/*!
 * Default constructor.
 */
SearchQuery::SearchQuery() :
        sq_Query(), sq_Reversed(false), sq_AllowReplace(false), sq_AllowInsert(
                false), sq_AllowDelete(false), sq_KillNSeqsAt(0x80000000), sq_MaxErrors(
                0.0), sq_WeightedSearchMode(false), sq_MinorMisThres(0.5), sq_MismatchWeights(
                NULL) {
}

/*!
 * Constructor.
 *
 * \param struct MismatchWeights *wm SearchQuery takes ownership of this
 */
SearchQuery::SearchQuery(struct MismatchWeights *wm) :
        sq_Query(), sq_Reversed(false), sq_AllowReplace(false), sq_AllowInsert(
                false), sq_AllowDelete(false), sq_KillNSeqsAt(0x80000000), sq_MaxErrors(
                0.0), sq_WeightedSearchMode(false), sq_MinorMisThres(0.5), sq_MismatchWeights(
                wm) {
}

/*!
 * \brief Copy constructor.
 */
SearchQuery::SearchQuery(const SearchQuery& query) :
        sq_Query(query.sq_Query), sq_Reversed(query.sq_Reversed), sq_AllowReplace(
                query.sq_AllowReplace), sq_AllowInsert(query.sq_AllowInsert), sq_AllowDelete(
                query.sq_AllowDelete), sq_KillNSeqsAt(query.sq_KillNSeqsAt), sq_MaxErrors(
                query.sq_MaxErrors), sq_WeightedSearchMode(
                query.sq_WeightedSearchMode), sq_MinorMisThres(
                query.sq_MinorMisThres), sq_MismatchWeights(
                MismatchWeights::cloneMismatchWeightMatrix(
                        query.sq_MismatchWeights)) {
}

/*!
 * \brief Destructor.
 */
SearchQuery::~SearchQuery() {
    if (sq_MismatchWeights) {
        MismatchWeights::freeMismatchWeightMatrix(sq_MismatchWeights);
    }
}

/*!
 * \brief The assignment operator.
 *
 * \param query The SearchQuery to copy
 */
SearchQuery& SearchQuery::operator =(const SearchQuery & query) {
    if (this != &query) {
        sq_Query = query.sq_Query;
        sq_Reversed = query.sq_Reversed;
        sq_AllowReplace = query.sq_AllowReplace;
        sq_AllowInsert = query.sq_AllowInsert;
        sq_AllowDelete = query.sq_AllowDelete;
        sq_KillNSeqsAt = query.sq_KillNSeqsAt;
        sq_MaxErrors = query.sq_MaxErrors;
        sq_WeightedSearchMode = query.sq_WeightedSearchMode;
        sq_MinorMisThres = query.sq_MinorMisThres;
        sq_MismatchWeights = MismatchWeights::cloneMismatchWeightMatrix(
                query.sq_MismatchWeights);
    }
    return *this;
}

/*!
 * \brief Constructor
 */
HitTuple::HitTuple() {
}

/*!
 * \brief Constructor
 */
HitTuple::HitTuple(ULLONG abs_pos, struct PTPanEntry *entry) :
        ht_AbsPos(abs_pos), ht_Entry(entry) {
}

/*!
 * \brief Destructor
 */
HitTuple::~HitTuple() {
}

/*!
 * Default constructor.
 */
DesignHit::DesignHit() :
        dh_ProbeSeq(), dh_GroupHits(0), dh_NonGroupHits(0), dh_Temp(0.0), dh_GCContent(
                0), dh_relpos(0), dh_ref_pos(0), dh_Quality(0.0), dh_Matches(), dh_NonGroupHitsPerc(), dh_sortKey(
                0) {
}

/*!
 * \brief Copy constructor.
 */
DesignHit::DesignHit(const DesignHit& hit) :
        dh_ProbeSeq(hit.dh_ProbeSeq), dh_GroupHits(hit.dh_GroupHits), dh_NonGroupHits(
                hit.dh_NonGroupHits), dh_Temp(hit.dh_Temp), dh_GCContent(
                hit.dh_GCContent), dh_relpos(hit.dh_relpos), dh_ref_pos(
                hit.dh_ref_pos), dh_Quality(hit.dh_Quality), dh_Matches(
                hit.dh_Matches), dh_NonGroupHitsPerc(), dh_sortKey(
                hit.dh_sortKey) {
    for (int i = 0; i < DECR_TEMP_CNT; i++) {
        dh_NonGroupHitsPerc[i] = hit.dh_NonGroupHitsPerc[i];
    }
}

/*!
 * \brief Constructor.
 */
DesignHit::DesignHit(ULONG probe_length) :
        dh_ProbeSeq(std::string(probe_length, '0')), dh_GroupHits(0), dh_NonGroupHits(
                0), dh_Temp(0.0), dh_GCContent(0), dh_relpos(0), dh_ref_pos(0), dh_Quality(
                0.0), dh_Matches(), dh_NonGroupHitsPerc(), dh_sortKey(0) {
}

/*!
 * \brief Destructor.
 */
DesignHit::~DesignHit() {
}

/*!
 * \brief The assignment operator.
 *
 * \param hit The DesignHit to copy
 */
DesignHit& DesignHit::operator =(const DesignHit & hit) {
    if (this != &hit) {
        dh_ProbeSeq = hit.dh_ProbeSeq;
        dh_GroupHits = hit.dh_GroupHits;
        dh_NonGroupHits = hit.dh_NonGroupHits;
        dh_Temp = hit.dh_Temp;
        dh_GCContent = hit.dh_GCContent;
        dh_relpos = hit.dh_relpos;
        dh_ref_pos = hit.dh_ref_pos;
        dh_Quality = hit.dh_Quality;
        dh_Matches = hit.dh_Matches;
        for (int i = 0; i < DECR_TEMP_CNT; i++) {
            dh_NonGroupHitsPerc[i] = hit.dh_NonGroupHitsPerc[i];
        }
        dh_sortKey = hit.dh_sortKey;
    }
    return *this;
}

/*!
 * \brief Private default constructor.
 */
DesignQuery::DesignQuery() :
        dq_ProbeLength(0), dq_SortMode(0), dq_MinGroupHits(0), dq_MaxNonGroupHits(
                0), dq_MinTemp(0.0), dq_MaxTemp(0.0), dq_MinGC(0), dq_MaxGC(0), dq_MinPos(
                0), dq_MaxPos(0), dq_MaxHits(0), dq_MismatchWeights(NULL), dq_DesginIds(
                NULL) {
}

/*!
 * \brief Constructor.
 *
 * \param struct MismatchWeights *wm DesignQuery takes ownership of this
 * \param ids DesignQuery takes ownership of this
 */
DesignQuery::DesignQuery(struct MismatchWeights *wm,
        std::set<std::string> * ids) :
        dq_ProbeLength(0), dq_SortMode(0), dq_MinGroupHits(0), dq_MaxNonGroupHits(
                0), dq_MinTemp(0.0), dq_MaxTemp(0.0), dq_MinGC(0), dq_MaxGC(0), dq_MinPos(
                0), dq_MaxPos(0), dq_MaxHits(0), dq_MismatchWeights(wm), dq_DesginIds(
                ids) {
}

/*!
 * \brief Copy constructor.
 */
DesignQuery::DesignQuery(const DesignQuery& query) :
        dq_ProbeLength(query.dq_ProbeLength), dq_SortMode(query.dq_SortMode), dq_MinGroupHits(
                query.dq_MinGroupHits), dq_MaxNonGroupHits(
                query.dq_MaxNonGroupHits), dq_MinTemp(query.dq_MinTemp), dq_MaxTemp(
                query.dq_MaxTemp), dq_MinGC(query.dq_MinGC), dq_MaxGC(
                query.dq_MaxGC), dq_MinPos(query.dq_MinPos), dq_MaxPos(
                query.dq_MaxPos), dq_MaxHits(query.dq_MaxHits), dq_MismatchWeights(
                MismatchWeights::cloneMismatchWeightMatrix(
                        query.dq_MismatchWeights)), dq_DesginIds(
                new std::set<std::string>(*query.dq_DesginIds)) {
}

/*!
 * \brief Destructor.
 */
DesignQuery::~DesignQuery() {
    try {
        if (dq_MismatchWeights) {
            MismatchWeights::freeMismatchWeightMatrix(dq_MismatchWeights);
        }
        if (dq_DesginIds) {
            delete dq_DesginIds;
        }
    } catch (...) {
        // nothing
    }
}

/*!
 * \brief The assignment operator.
 *
 * \param query The DesignQuery to copy
 */
DesignQuery& DesignQuery::operator =(const DesignQuery & query) {
    if (this != &query) {
        dq_ProbeLength = query.dq_ProbeLength;
        dq_SortMode = query.dq_SortMode;
        dq_MinGroupHits = query.dq_MinGroupHits;
        dq_MaxNonGroupHits = query.dq_MaxNonGroupHits;
        dq_MinTemp = query.dq_MinTemp;
        dq_MaxTemp = query.dq_MaxTemp;
        dq_MinGC = query.dq_MinGC;
        dq_MaxGC = query.dq_MaxGC;
        dq_MinPos = query.dq_MinPos;
        dq_MaxPos = query.dq_MaxPos;
        dq_MaxHits = query.dq_MaxHits;
        dq_MismatchWeights = MismatchWeights::cloneMismatchWeightMatrix(
                query.dq_MismatchWeights);
        dq_DesginIds = new std::set<std::string>(*query.dq_DesginIds);
    }
    return *this;
}

/*!
 * Default constructor.
 */
SimilaritySearchHit::SimilaritySearchHit() :
        ssh_Matches(0), ssh_RelMatches(0.0), ssh_Entry(NULL) {
}

/*!
 * \brief Copy constructor.
 */
SimilaritySearchHit::SimilaritySearchHit(const SimilaritySearchHit& hit) :
        ssh_Matches(hit.ssh_Matches), ssh_RelMatches(hit.ssh_RelMatches), ssh_Entry(
                hit.ssh_Entry) {
}

/*!
 * \brief Constructor.
 */
SimilaritySearchHit::SimilaritySearchHit(ULLONG matches, double relMatches,
        struct PTPanEntry * entry) :
        ssh_Matches(matches), ssh_RelMatches(relMatches), ssh_Entry(entry) {
}

/*!
 * \brief Destructor.
 */
SimilaritySearchHit::~SimilaritySearchHit() {
}

/*!
 * \brief The assignment operator.
 *
 * \param hit The SimilaritySearchHit to copy
 */
SimilaritySearchHit& SimilaritySearchHit::operator =(
        const SimilaritySearchHit & hit) {
    if (this != &hit) {
        ssh_Matches = hit.ssh_Matches;
        ssh_RelMatches = hit.ssh_RelMatches;
        ssh_Entry = hit.ssh_Entry;
    }
    return *this;
}

/*!
 * \brief Private default constructor.
 */
SimilaritySearchQuery::SimilaritySearchQuery() :
        ssq_SourceSeq(), ssq_WindowSize(0), ssq_SearchMode(0), ssq_ComplementMode(
                0), ssq_SortType(0), ssq_NumMaxMismatches(0.0), ssq_RangeStart(
                -1), ssq_RangeEnd(-1) {
}

/*!
 * \brief Constructor.
 *
 * \param source_seq Must not contain alignment information, just the plain sequence
 *                   If necessary, filter before!
 */
SimilaritySearchQuery::SimilaritySearchQuery(const std::string& source_seq) :
        ssq_SourceSeq(source_seq), ssq_WindowSize(0), ssq_SearchMode(0), ssq_ComplementMode(
                0), ssq_SortType(0), ssq_NumMaxMismatches(0.0), ssq_RangeStart(
                -1), ssq_RangeEnd(-1) {
}

/*!
 * \brief Copy constructor.
 */
SimilaritySearchQuery::SimilaritySearchQuery(const SimilaritySearchQuery& query) :
        ssq_SourceSeq(query.ssq_SourceSeq), ssq_WindowSize(
                query.ssq_WindowSize), ssq_SearchMode(query.ssq_SearchMode), ssq_ComplementMode(
                query.ssq_ComplementMode), ssq_SortType(query.ssq_SortType), ssq_NumMaxMismatches(
                query.ssq_NumMaxMismatches), ssq_RangeStart(
                query.ssq_RangeStart), ssq_RangeEnd(query.ssq_RangeEnd) {
}

/*!
 * \brief Destructor.
 */
SimilaritySearchQuery::~SimilaritySearchQuery() {
}

/*!
 * \brief The assignment operator.
 *
 * \param query The SimilaritySearchQuery to copy
 */
SimilaritySearchQuery& SimilaritySearchQuery::operator =(
        const SimilaritySearchQuery & query) {
    if (this != &query) {
        ssq_SourceSeq = query.ssq_SourceSeq;
        ssq_WindowSize = query.ssq_WindowSize;
        ssq_SearchMode = query.ssq_SearchMode;
        ssq_ComplementMode = query.ssq_ComplementMode;
        ssq_SortType = query.ssq_SortType;
        ssq_NumMaxMismatches = query.ssq_NumMaxMismatches;
        ssq_RangeStart = query.ssq_RangeStart;
        ssq_RangeEnd = query.ssq_RangeEnd;
    }
    return *this;
}

/*!
 * \brief Feature Container constructor
 */
PTPanFeatureContainer::PTPanFeatureContainer() :
        m_features(NULL), m_feat_array(NULL), m_feat_count(0), m_max_feat_count(
                0), m_feature_map(NULL), m_feature_pos_map(NULL), m_normal_mode(
                true), m_simple_mode(false) {
    m_features = new std::vector<PTPanFeature*>();
}

/*!
 * \brief Feature Container constructor
 */
PTPanFeatureContainer::PTPanFeatureContainer(bool simple_mode) :
        m_features(NULL), m_feat_array(NULL), m_feat_count(0), m_max_feat_count(
                0), m_feature_map(NULL), m_feature_pos_map(NULL), m_normal_mode(
                true), m_simple_mode(simple_mode) {
    m_features = new std::vector<PTPanFeature*>();
    if (!m_simple_mode) {
        m_feature_map = new PtpanFeatureMap();
        m_feature_pos_map = new FeaturePositionT();
    }
}

/*!
 * \descrutcor
 *
 * Free's the PTPanFeature contained!
 */
PTPanFeatureContainer::~PTPanFeatureContainer() {
    try {
        if (m_normal_mode) {
            if (!m_features->empty()) {
                PTPanFeatureIterator it;
                for (it = m_features->begin(); it != m_features->end(); it++) {
                    PTPanFeature::freePtpanFeature(*it);
                }
            }
            if (!m_simple_mode) {
                delete m_feature_map;
                delete m_feature_pos_map;
            }
        }
    } catch (...) {
        // do nothing, only because a destructor must not throw!
    }
}

/*!
 * \brief Returns simple mode flag value
 *
 * \return bool
 */
bool PTPanFeatureContainer::simpleMode() const {
    return m_simple_mode;
}

/*!
 * \brief Returns normal mode flag value
 *
 * \return bool
 */
bool PTPanFeatureContainer::normalMode() const {
    return m_normal_mode;
}

/*!
 * \brief Clear container
 */
void PTPanFeatureContainer::clear() {
    if (m_normal_mode) {
        PTPanFeatureIterator it;
        for (it = m_features->begin(); it != m_features->end(); it++) {
            PTPanFeature::freePtpanFeature(*it);
        }
        m_features->clear();
        if (!m_simple_mode) {
            m_feature_map->clear();
            m_feature_pos_map->clear();
        }
    }
    // TODO FIXME else for special mode
}

/*!
 * \brief Return empty
 *
 * \return bool
 */
bool PTPanFeatureContainer::empty() const {
    if (m_normal_mode) {
        return m_features->empty();
    }
    return m_feat_count == 0;
}

/*!
 * \brief Return size (=number of Features)
 *
 * \return ULONG
 */
ULONG PTPanFeatureContainer::size() const {
    if (m_normal_mode) {
        return m_features->size();
    }
    return m_feat_count;
}

/*!
 * \brief Return pointer to Feature if number is valid, else NULL
 *
 * \param number
 *
 * \return const PTPanFeature *
 */
const PTPanFeature * PTPanFeatureContainer::getFeatureByNumber(
        ULONG number) const {
    if (m_normal_mode) {
        if (number < m_features->size()) {
            return m_features->at(number);
        }
    } else {
        if (number < m_feat_count) {
            return m_feat_array[number];
        }
    }
    return NULL;
}

/*!
 * \brief Returns the maximum byte size of this container for storing
 *
 * \return ULONG
 */
ULONG PTPanFeatureContainer::byteSize() const {
    ULONG size = 0;
    if (m_normal_mode) {
        PTPanFeatureIterator it;
        for (it = m_features->begin(); it != m_features->end(); it++) {
            size += sizeof(ULONG);
            size += strlen((*it)->pf_name);
            size += sizeof((*it)->pf_num_pos);
            size += (*it)->pf_num_pos * (2 * sizeof(ULONG) + sizeof(UBYTE));
        }
    } else {
        for (ULONG i = 0; i < m_feat_count; i++) {
            size += sizeof(ULONG);
            size += strlen(m_feat_array[i]->pf_name);
            size += sizeof(m_feat_array[i]->pf_num_pos);
            size += m_feat_array[i]->pf_num_pos
                    * (2 * sizeof(ULONG) + sizeof(UBYTE));
        }
    }
    return size;
}

/*!
 * \brief Add PTPanFeature to container
 *
 * The container takes control over the feature, thus it is free'd when
 * the container is deconstructed!
 */
void PTPanFeatureContainer::add(PTPanFeature *feature) {
    if (m_normal_mode) {
        m_features->push_back(feature);
        if (m_simple_mode) {
            (*m_feature_map)[feature->pf_name] = feature;
            MemberSetT feature_set;
            feature_set.insert((ULONG) feature);
            for (int i = 0; i < feature->pf_num_pos; i++) {
                m_feature_pos_map->add(
                        std::make_pair(
                                boost::icl::discrete_interval<ULONG>::closed(
                                        feature->pf_pos[i].pop_start,
                                        feature->pf_pos[i].pop_end),
                                feature_set));
            }
        }
    } else {
        if (m_feat_count + 1 > m_max_feat_count) {
            // double size of m_max_feat_count
            m_max_feat_count = m_max_feat_count << 1;
            m_feat_array = (struct PTPanFeature**) realloc(m_feat_array,
                    m_max_feat_count * sizeof(PTPanFeature*));
        }
        m_feat_array[m_feat_count] = feature;
        m_feat_count++;
    }
}

/*!
 * \brief Returns feature list at position
 *
 * If there is no feature at given position or postition + length
 * is out of range, an empty list is returned.
 *
 * \param position
 * \param length
 *
 * \return std::vector<PTPanFeature*>
 */
const std::vector<PTPanFeature*> PTPanFeatureContainer::getFeature(
        ULONG position) const {
    std::vector<PTPanFeature*> ret_values;
    if (m_normal_mode && !m_simple_mode) {
        if (!m_feature_pos_map->empty()) {
            FeaturePositionT::const_iterator it = m_feature_pos_map->find(
                    position);
            if (it != m_feature_pos_map->end()) {
                MemberSetT::const_iterator mit;
                for (mit = it->second.begin(); mit != it->second.end(); mit++) {
                    ret_values.push_back((PTPanFeature *) (*mit));
                }
            }
        }
    }
    return ret_values;
}

/*!
 * \brief Check if a feature exists for given name
 *
 * \param name
 *
 * \return bool
 */
bool PTPanFeatureContainer::hasFeature(CONST_STRPTR name) const {
    if (name) {
        if (m_normal_mode && !m_simple_mode) {
            return m_feature_map->find(name) != m_feature_map->end();
        }
    }
    return false;
}

/*!
 * \brief Get feature by its name
 *
 * If there is no feature for given name, NULL is returned
 *
 * \return PTPanFeature* or NULL
 */
const PTPanFeature* PTPanFeatureContainer::getFeature(CONST_STRPTR name) const {
    if (name) {
        if (m_normal_mode && !m_simple_mode) {
            PtpanFeatureMap::const_iterator it = m_feature_map->find(name);
            if (it != m_feature_map->end()) {
                return it->second;
            }
        }
    }
    return NULL;
}
