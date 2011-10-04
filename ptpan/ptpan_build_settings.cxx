/*!
 * \file ptpan_build_settings.cxx
 *
 * \date 12.02.2011
 * \author Tilo Eissler
 */

#include "ptpan_build_settings.h"
#include <boost/thread.hpp>

/*!
 * \brief Private default constructor
 */
PtpanBuildSettings::PtpanBuildSettings() :
        m_as(NULL), m_prune_length(0), m_memory_size(0), m_max_prefix_len(0), m_path(), m_index_name(), m_num_threads(
                boost::thread::hardware_concurrency()), m_include_features(
                false), m_verbose(false), m_force(false), m_ratio(1.0) {
}

/*!
 * \brief Default constructor
 *
 * All integer values are initialized with default settings:
 * prune length = 20
 * memory size = 1 Gb (but stored in bytes!)
 * max_prefix_length = 5
 * max_prefix_lookup_size = 7
 * include features = false
 * verbose = false
 * force = false
 *
 * \param name Name of the index
 * \param path Path of the index
 */
PtpanBuildSettings::PtpanBuildSettings(AbstractAlphabetSpecifics *as,
        const std::string& name, const std::string& path) :
        m_as(as), m_prune_length(20), m_memory_size(1024 * 1024 * 1024), m_max_prefix_len(
                5), m_path(path), m_index_name(name), m_max_prefix_lookup_size(
                7), m_num_threads(boost::thread::hardware_concurrency()), m_include_features(
                false), m_verbose(false), m_force(false), m_ratio(1.0) {
}

/*!
 * \brief Destructor
 */
PtpanBuildSettings::~PtpanBuildSettings() {
}

/*!
 * \brief Set prune (path) length
 *
 * The new length is checked to be in the allowed boundaries,
 * i.e. not shorter than max_prefix_lookup_size + 1 and
 * not longer than max_code_fit_loong
 *
 * \param length New prune length
 *
 * \return The new prune length
 */
ULONG PtpanBuildSettings::setPruneLength(ULONG length) {
    m_prune_length = length;
    if (m_prune_length < (m_max_prefix_lookup_size + 1)) {
        m_prune_length = m_max_prefix_lookup_size + 1;
    }
    // TODO enlarge to (4*max_code_fit_long -1)
    if (m_prune_length > m_as->max_code_fit_long) {
        m_prune_length = m_as->max_code_fit_long;
    }
    return m_prune_length;
}

/*!
 * \brief Set the memory size in bytes
 *
 * \param memory_size
 */
void PtpanBuildSettings::setMemorySize(ULONG memory_size) {
    m_memory_size = memory_size;
}

/*!
 * \brief Set maximum prefix length
 *
 * \param max_prefix_length
 */
void PtpanBuildSettings::setMaxPrefixLength(ULONG max_prefix_length) {
    m_max_prefix_len = max_prefix_length;
}

/*!
 * \brief Set index path
 *
 * \param index_path
 */
void PtpanBuildSettings::setIndexPath(const std::string& index_path) {
    m_path = index_path;
}

/*!
 * \brief Set index name
 *
 * \param index_name
 */
void PtpanBuildSettings::setIndexName(const std::string& index_name) {
    m_index_name = index_name;
}

/*!
 * \brief Set number of threads to use
 *
 * \warning If param is set to 0, the number of threads is set to 1
 *
 * \param num_threads
 */
void PtpanBuildSettings::setThreadNumber(ULONG num_threads) {
    if (num_threads == 0) {
        // error!?
        m_num_threads = 1;
    } else {
        m_num_threads = num_threads;
    }
}
/*!
 * \brief Set index flag to include features
 *
 * \param inlcude
 */
void PtpanBuildSettings::setIncludeFeatures(bool include) {
    m_include_features = include;
}

/*!
 * \brief Set verbose flag
 *
 * \param verbose
 */
void PtpanBuildSettings::setVerbose(bool verbose) {
    m_verbose = verbose;
}

/*!
 * \brief Set force flag
 *
 * If true, settings (e.g. number of threads) will be forced and automatic
 * optimizations are disabled.
 *
 * \param verbose
 */
void PtpanBuildSettings::setForce(bool force) {
    m_force = force;
}

/*!
 * \brief Set Partition ratio
 *
 * \warning If the value does not match your data, the build
 * algorithm performance may decrease caused by swapping!
 * Use carefully and take a look at your empirical data!
 *
 * (Build in verbose mode and look at the statistics!)
 *
 * \param ratio
 */
void PtpanBuildSettings::setPartitionRatio(double ratio) {
    if (ratio > 0.0 && ratio <= 1.0) {
        m_ratio = ratio;
    } else {
        throw std::invalid_argument(
                "PtpanBuildSettings::setPartitionRatio() given ratio is out of range!");
    }
}

/*!
 * \brief Return prune length (= maximum length of a path in the tree)
 *
 * Default value is currently 20.
 *
 * \return ULONG
 */
ULONG PtpanBuildSettings::getPruneLength() const {
    return m_prune_length;
}

/*!
 * \brief Return memory size in bytes
 *
 * Default is 1 Gb = 1024*1024*1024 bytes
 *
 * \return ULONG
 */
ULONG PtpanBuildSettings::getMemorySize() const {
    return m_memory_size;
}

/*!
 * \brief return the maximum prefix length
 *
 * Default is currently 5. You may want to alter it depending on
 * the amount of data to process!
 *
 * \return ULONG
 */
ULONG PtpanBuildSettings::getMaxPrefixLength() const {
    return m_max_prefix_len;
}

/*!
 * \brief Return the index path
 *
 * \return const std::string
 */
const std::string PtpanBuildSettings::getIndexPath() const {
    return m_path;
}

/*!
 * \brief Return the index name
 *
 * \return const std::string
 */
const std::string PtpanBuildSettings::getIndexName() const {
    return m_index_name;
}

/*!
 * \brief Return index name length
 *
 * \return ULONG
 */
ULONG PtpanBuildSettings::getIndexNameLength() const {
    return m_index_name.size();
}

/*!
 * \brief Return maximum prefix lookup size
 *
 * \return ULONG
 */
ULONG PtpanBuildSettings::getMaxPrefixLookupSize() const {
    return m_max_prefix_lookup_size;
}

/*!
 * \brief Return number of threads
 *
 * \return ULONG
 */
ULONG PtpanBuildSettings::getThreadNumber() const {
    return m_num_threads;
}

/*!
 * \brief Return include feature flag
 *
 * \return bool
 */
bool PtpanBuildSettings::getIncludeFeatures() const {
    return m_include_features;
}

/*!
 * \brief Return verbose flag
 *
 * \return bool
 */
bool PtpanBuildSettings::isVerbose() const {
    return m_verbose;
}

/*!
 * \brief Return force flag
 *
 * \return bool
 */
bool PtpanBuildSettings::isForce() const {
    return m_force;
}

/*!
 * \brief Return ratio value
 *
 * Default is 1.0
 *
 * \return double
 */
double PtpanBuildSettings::getPartitionRatio() const {
    return m_ratio;
}
