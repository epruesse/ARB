/*!
 * \file ptpan_tree_builder.cxx
 *
 * \date 10.02.2011
 * \author Tilo Eissler
 * \author Chris Hodges
 * \author Jörg Böhnel
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#include <boost/thread.hpp>

#include <algorithm>
#include <stdexcept>
#include <boost/unordered_map.hpp>
#include <deque>
#include <string>

#include "ptpan_tree_builder.h"

/*!
 * \brief Build Ptpan index for the given settings
 *
 * \param settings Pointer to PtpanBuildSettings
 * \param as Pointer to AbstractAlphabetSpecifics
 * \param dr Pointer to AbstractDataRetriever
 *
 * The PtpanTree is stored using path and name given in PtpanBuildSettings.
 *
 * \exception std::invalid_argument
 * \exception std::runtime_error
 */
void PtpanTreeBuilder::buildPtpanTree(PtpanBuildSettings *settings,
        AbstractAlphabetSpecifics *as, AbstractDataRetriever *dr,
        const std::string& custom_info) {
    if (!settings || !as || !dr) {
        throw std::invalid_argument(
                "PtpanTreeBuilder::buildPtpanTree() at least one of the given pointer is empty!");
    }
    PtpanTreeBuilder builder;
    dr->setIncludeFeatures(settings->getIncludeFeatures());

    builder.m_settings = settings;
    builder.m_as = as;
    builder.m_dr = dr;

    builder.setNumberOfThreads(settings->getThreadNumber());
    builder.checkSettings();
    builder.setVerbose(settings->isVerbose());

#ifdef DEBUG
    printf("********************************\n"
            "* Building new PT Pan Index... *\n"
            "********************************\n");
    printf("PTPan version v%s ('%s')\n", version().data(),
            builder.m_settings->getIndexName().data());
#endif

    if (!builder.m_dr->connected()) {
        builder.m_dr->openConnection();
    }

    if (builder.m_dr->containsReferenceEntry()) {
        builder.loadReferenceEntry();
    } else {
#ifdef DEBUG
        printf("No reference entry available!\n");
#endif
    }

#ifdef DEBUG
    if (builder.m_dr->containsEntries()) {
        printf("%lu entries found\n", builder.m_dr->countEntries());
    } else {
        printf("no entries found!?\n");
    }
#endif

    try {
        builder.initIndexHeader(custom_info);
        builder.loadEntries();
        builder.freeStuff();
        builder.m_dr->closeConnection();
        builder.reInitStuff();
        builder.buildMergedRawData();

        builder.partitionDetermination();

        builder.storeIndexHeader();
        builder.buildTree();
    } catch (std::exception& e) {
        printf("Build failed! (%s)\n", e.what());
        builder.cleanFail();
    }

#ifdef DEBUG
    printf("PTPan index build done.\n");
#endif
}

/*!
 * \brief Default constructor
 */
PtpanTreeBuilder::PtpanTreeBuilder() :
        PtpanBase(), m_build_mutex(), m_settings(NULL), m_as(NULL), m_dr(NULL), m_ref_entry(
                NULL), m_num_entries(0), m_entries(NULL), m_index_name(), m_index_fh(
                NULL), m_map_file_size(0), m_map_file_buffer(NULL), m_tmp_raw_file_name(
                ".raw_data.tmp"), m_merged_raw_data_size(0), m_merged_raw_data(
                NULL), m_dispatcher(), m_merge_border_map(), m_partition_dispatcher(), m_total_seq_size(
                0), m_total_raw_size(0), m_total_seq_compressed_size(0), m_extended_long_edges(
                false), m_long_edge_max_size_factor(1), m_all_hash_sum(rand()), m_partitions(), m_partitions_statistics(), m_max_partition_size(
                0) {
    m_entries = (struct List *) malloc(sizeof(List));
    NewList(m_entries);
}

/*!
 * \brief Destructor
 */
PtpanTreeBuilder::~PtpanTreeBuilder() {
    try {
        if (m_ref_entry) {
            free_ptpan_reference_entry(m_ref_entry);
            m_ref_entry = NULL;
        }
        if (m_entries) {
            free_all_ptpan_entries(m_entries);
            free(m_entries);
            m_entries = NULL;
        }
        pmap::const_iterator pit;
        for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
            free_partition(pit->second);
        }
    } catch (...) {
        // destructor must not throw
    }
}

/*!
 * \brief Private copy constructor
 *
 * This constructor slices! But it doesn't matter
 */
PtpanTreeBuilder::PtpanTreeBuilder(const PtpanTreeBuilder& ptb) :
        PtpanBase(), m_build_mutex(), m_settings(NULL), m_as(ptb.m_as), m_dr(
                NULL), m_ref_entry(NULL), m_num_entries(0), m_entries(NULL), m_index_name(), m_index_fh(
                NULL), m_map_file_size(0), m_map_file_buffer(NULL), m_tmp_raw_file_name(
                ".raw_data.tmp"), m_merged_raw_data_size(0), m_merged_raw_data(
                NULL), m_dispatcher(), m_merge_border_map(), m_partition_dispatcher(), m_total_seq_size(
                0), m_total_raw_size(0), m_total_seq_compressed_size(0), m_extended_long_edges(
                false), m_long_edge_max_size_factor(1), m_all_hash_sum(rand()), m_partitions(), m_partitions_statistics(), m_max_partition_size(
                0) {
}

/*!
 * \brief Private assignment operator
 */
PtpanTreeBuilder& PtpanTreeBuilder::operator=(const PtpanTreeBuilder& ptb) {
    if (this != &ptb) {
        // we don't use it, so we don't implement it!
        // partly implement for operator() + std::sort
        m_as = ptb.m_as;
    }
    return *this;
}

/*!
 * \brief Constructor (private)
 */
PtpanTreeBuilder::PartitionStatistics::PartitionStatistics() {
}

/*!
 * \brief Constructor
 *
 * \param id corresponding partition id
 * \param alphabet_size
 * \param max_partition_count
 */
PtpanTreeBuilder::PartitionStatistics::PartitionStatistics(ULONG id,
        UWORD alphabet_size, ULONG max_partition_count) :
        m_id(id), m_max_part_count(max_partition_count), m_overall_count(1), m_inner_nodes(
                1), m_border_nodes(1), m_alphabet_size(alphabet_size) {
}

/*!
 * \brief Destructor
 */
PtpanTreeBuilder::PartitionStatistics::~PartitionStatistics() {
}

/*!
 * \brief Copy constructor
 */
PtpanTreeBuilder::PartitionStatistics::PartitionStatistics(
        const PartitionStatistics& ps) :
        m_id(ps.m_id), m_max_part_count(ps.m_max_part_count), m_overall_count(
                ps.m_overall_count), m_inner_nodes(ps.m_inner_nodes), m_border_nodes(
                ps.m_border_nodes), m_alphabet_size(ps.m_alphabet_size) {
}

/*!
 * \brief Assignment operator
 */
PtpanTreeBuilder::PartitionStatistics& PtpanTreeBuilder::PartitionStatistics::operator=(
        const PartitionStatistics& ps) {
    if (this != &ps) {
        m_id = ps.m_id;
        m_max_part_count = ps.m_max_part_count;
        m_overall_count = ps.m_overall_count;
        m_inner_nodes = ps.m_inner_nodes;
        m_border_nodes = ps.m_border_nodes;
        m_alphabet_size = ps.m_alphabet_size;
    }
    return *this;
}

/*!
 * \brief Get load value
 */
double PtpanTreeBuilder::PartitionStatistics::load() const {
    // add up utilized memory for all kinds of structs
    ULONG memory = (m_inner_nodes
            * (sizeof(SfxInnerNode) + (m_alphabet_size - 1)))
            + (m_border_nodes * sizeof(SfxBorderNode))
            + m_overall_count * sizeof(ULONG);
    ULONG max_memory = m_overall_count
            * ((sizeof(SfxInnerNode) + (m_alphabet_size - 1))
                    + sizeof(SfxBorderNode) + sizeof(ULONG));

    // printf("%ld %ld\n", memory, max_memory);

    return ((double) (memory * 120 / max_memory)) / 100;
}

/*!
 * \brief Returns true if this partition is significant (i.e. the
 *          number of maximum tuples exceeds a certain threshold percentage)
 */
bool PtpanTreeBuilder::PartitionStatistics::significant() const {
    double ratio = (double) (m_overall_count * 100 / m_max_part_count);
    if (ratio > SIGNIFICANCE_THRESHOLD) {
        return true;
    }
    return false;
}

/*!
 * \brief Return corresponding partition id
 */
ULONG PtpanTreeBuilder::PartitionStatistics::getId() const {
    return m_id;
}

/*!
 * \brief Return max partition tuple count
 *
 * \return ULONG
 */
ULONG PtpanTreeBuilder::PartitionStatistics::getMaxPartitionCount() const {
    return m_max_part_count;
}

/*!
 * \brief Return overall count
 *
 * \return ULONG
 */
ULONG PtpanTreeBuilder::PartitionStatistics::getOverallCount() const {
    return m_overall_count;
}

/*!
 * \brief Return inner nodes count
 *
 * \return ULONG
 */
ULONG PtpanTreeBuilder::PartitionStatistics::getInnerNodes() const {
    return m_inner_nodes;
}

/*!
 * \brief Return border nodes count
 *
 * \return ULONG
 */
ULONG PtpanTreeBuilder::PartitionStatistics::getBorderNodes() const {
    return m_border_nodes;
}

/*!
 * \brief Return alphabet size
 *
 * \return UWORD
 */
UWORD PtpanTreeBuilder::PartitionStatistics::getAlphabetSize() const {
    return m_alphabet_size;
}

/*!
 * \brief Set overall count
 *
 * \param ocount
 */
void PtpanTreeBuilder::PartitionStatistics::setOverallCount(ULONG ocount) {
    m_overall_count = ocount;
}

/*!
 * \brief Set inner node count
 *
 * \param inner
 */
void PtpanTreeBuilder::PartitionStatistics::setInnerNodes(ULONG inner) {
    m_inner_nodes = inner;
}

/*!
 * \brief Set border node count
 *
 * \param border
 */
void PtpanTreeBuilder::PartitionStatistics::setBorderNodes(ULONG border) {
    m_border_nodes = border;
}

/*!
 * \brief Default constructor (private)
 */
PtpanTreeBuilder::LongEdgeComparator::LongEdgeComparator() :
        m_as(NULL), m_extended_long_edges(false) {
}

/*!
 * \brief Constructor
 */
PtpanTreeBuilder::LongEdgeComparator::LongEdgeComparator(
        AbstractAlphabetSpecifics *as, bool extended_long_edges) :
        m_as(as), m_extended_long_edges(extended_long_edges) {
}

/*!
 * \brief Copy constructor
 */
PtpanTreeBuilder::LongEdgeComparator::LongEdgeComparator(
        const LongEdgeComparator& comp) :
        m_as(comp.m_as), m_extended_long_edges(comp.m_extended_long_edges) {
}

/*!
 * \brief Assignment operator
 */
PtpanTreeBuilder::LongEdgeComparator& PtpanTreeBuilder::LongEdgeComparator::operator=(
        const LongEdgeComparator& comp) {
    if (this != &comp) {
        m_as = comp.m_as;
        m_extended_long_edges = comp.m_extended_long_edges;
    }
    return *this;
}

/*!
 * \brief Destructor
 */
PtpanTreeBuilder::LongEdgeComparator::~LongEdgeComparator() {
}

/*!
 * \brief Check settings
 */
void PtpanTreeBuilder::checkSettings() {
    // check if we use extended long edges (future work!)
    if (m_settings->getPruneLength() > m_as->max_code_fit_long) {
        m_extended_long_edges = true;
        m_long_edge_max_size_factor = (UWORD) (floor(
                m_settings->getPruneLength() / m_as->max_code_fit_long) + 1);
#ifdef DEBUG
        printf("Max edge length > max_code_fit_long\n");
#endif
    } else {
        m_extended_long_edges = false;
        m_long_edge_max_size_factor = 1;
#ifdef DEBUG
        printf("Max edge length < max_code_fit_long\n");
#endif
    }
    m_num_threads = m_settings->getThreadNumber();
}

/*!
 * \brief Load reference entry from database using AbstractDataRetriever
 */
void PtpanTreeBuilder::loadReferenceEntry() {
    if (m_dr->containsReferenceEntry()) {
        m_ref_entry = m_dr->getReferenceEntry();
    }
}

/*!
 * \brief Method for loading entries
 *
 * \exception std::invalid_argument Thrown if a sequence is to short
 */
void PtpanTreeBuilder::loadEntry() {
#ifdef DEBUG
    ULONG longestali = 0;
    ULONG maxBaseLength = 0;
#endif
    struct PTPanEntry *entry;
    struct PTPanComprSeqShortcuts *csc;

    ULONG buf_size = 0;
    UBYTE *buffer = NULL;

    while (m_dr->hasNextEntry()) {
        entry = m_dr->nextEntry();
        if (entry) {
            if (entry->pe_RawDataSize < 2 * m_as->max_code_fit_long) {
                std::ostringstream os;
                os
                        << "PtpanTreeBuilder::loadEntry() sorry, sequence is to short for PTPan! (";
                os << entry->pe_Name << ")\n";
                os << "Must currently be at least "
                        << (2 * m_as->max_code_fit_long) << " bases long!";
                throw std::invalid_argument(os.str());
            }

            UWORD len_id = strlen(entry->pe_Name);
            UWORD len_name = strlen(entry->pe_FullName);
            ULONG compr_size = (entry->pe_SeqDataCompressedSize >> 3) + 1;

            ULONG min_size = len_id + len_name
                    + (3 * entry->pe_CompressedShortcutsCount + 7 + 1)
                            * sizeof(ULONG) + 1;
            if (min_size > buf_size) {
                buf_size = min_size;
                buffer = (UBYTE *) realloc(buffer, buf_size);
                //#ifdef DEBUG
                // printf("resize buffer for entry information!\n");
                //#endif
            }

            UBYTE *buf_ptr = &buffer[0];
            ULONG buffer_count = sizeof(len_id);
            write_byte(len_id, sizeof(len_id), buf_ptr);
            buf_ptr = &buffer[buffer_count];
            memcpy(buf_ptr, entry->pe_Name, len_id);
            buffer_count += len_id;
            buf_ptr = &buffer[buffer_count];

            buffer_count += sizeof(len_name);
            write_byte(len_name, sizeof(len_name), buf_ptr);
            buf_ptr = &buffer[buffer_count];
            memcpy(buf_ptr, entry->pe_FullName, len_name);
            buffer_count += len_name;
            buf_ptr = &buffer[buffer_count];

            buffer_count += sizeof(entry->pe_SeqDataSize);
            write_byte(entry->pe_SeqDataSize, sizeof(entry->pe_SeqDataSize),
                    buf_ptr);
            buf_ptr = &buffer[buffer_count];

            buffer_count += sizeof(entry->pe_RawDataSize);
            write_byte(entry->pe_RawDataSize, sizeof(entry->pe_RawDataSize),
                    buf_ptr);
            buf_ptr = &buffer[buffer_count];

            ULONG buf_abs_pos = buffer_count;
            buffer_count += sizeof(entry->pe_AbsOffset);
            // DELAY this as we currently don't know it
            // write_byte(entry->pe_AbsOffset, sizeof(entry->pe_AbsOffset),
            // buf_ptr);
            buf_ptr = &buffer[buffer_count];

            buffer_count += sizeof(entry->pe_CompressedShortcutsCount);
            write_byte(entry->pe_CompressedShortcutsCount,
                    sizeof(entry->pe_CompressedShortcutsCount), buf_ptr);
            buf_ptr = &buffer[buffer_count];

            for (ULONG cnt = 0; cnt < entry->pe_CompressedShortcutsCount;
                    cnt++) {
                csc =
                        (struct PTPanComprSeqShortcuts *) &entry->pe_CompressedShortcuts[cnt];
                buffer_count += write_byte(csc->pcs_AbsPosition, buf_ptr);
                buf_ptr = &buffer[buffer_count];
                buffer_count += write_byte(csc->pcs_BitPosition, buf_ptr);
                buf_ptr = &buffer[buffer_count];
                buffer_count += write_byte(csc->pcs_RelPosition, buf_ptr);
                buf_ptr = &buffer[buffer_count];
            }

            ULONG count = 0;
            if (entry->pe_FeatureContainer) {
                count = entry->pe_FeatureContainer->size();
            }
            buffer_count += sizeof(count);
            write_byte(count, sizeof(count), buf_ptr);
            buf_ptr = &buffer[buffer_count];

            if (entry->pe_FeatureContainer) {
                if (!entry->pe_FeatureContainer->empty()) {
                    min_size += entry->pe_FeatureContainer->byteSize();
                    if (min_size > buf_size) {
                        buf_size = min_size;
                        buffer = (UBYTE *) realloc(buffer, buf_size);
                        //#ifdef DEBUG
                        // printf("resize buffer for entry information!\n");
                        //#endif
                        buf_ptr = &buffer[buffer_count];
                    }

                    ULONG length = 0;
                    const PTPanFeature* feat = NULL;
                    for (ULONG j = 0; j < entry->pe_FeatureContainer->size();
                            j++) {
                        feat = entry->pe_FeatureContainer->getFeatureByNumber(
                                j);
                        length = strlen(feat->pf_name);
                        buffer_count += sizeof(length);
                        write_byte(length, sizeof(length), buf_ptr);
                        buf_ptr = &buffer[buffer_count];

                        memcpy(buf_ptr, feat->pf_name, length);
                        buffer_count += length;
                        buf_ptr = &buffer[buffer_count];

                        buffer_count += sizeof(feat->pf_num_pos);
                        write_byte(feat->pf_num_pos, sizeof(feat->pf_num_pos),
                                buf_ptr);
                        buf_ptr = &buffer[buffer_count];

                        for (UWORD i = 0; i < feat->pf_num_pos; i++) {
                            buffer_count += write_byte(
                                    feat->pf_pos[i].pop_start, buf_ptr);
                            buf_ptr = &buffer[buffer_count];
                            buffer_count += write_byte(feat->pf_pos[i].pop_end,
                                    buf_ptr);
                            buf_ptr = &buffer[buffer_count];
                            // write complement flag as well
                            buffer_count += sizeof(UBYTE);
                            if (feat->pf_pos[i].complement) {
                                write_byte('1', sizeof(UBYTE), buf_ptr);
                            } else {
                                write_byte('0', sizeof(UBYTE), buf_ptr);
                            }
                            buf_ptr = &buffer[buffer_count];
                        }
                    }
                }
            }

            buffer_count += sizeof(entry->pe_SeqDataCompressedSize);
            write_byte(entry->pe_SeqDataCompressedSize,
                    sizeof(entry->pe_SeqDataCompressedSize), buf_ptr);
            buf_ptr = &buffer[buffer_count];

            {
                boost::lock_guard<boost::mutex> lock(m_build_mutex);

                AddTail(m_entries, &entry->pe_Node);
                m_num_entries++;
                if (entry->pe_RawDataSize == 0) {
                    printf("%s is corrupt, aborting!\n", entry->pe_Name);
                    throw std::runtime_error(
                            "PtpanTreeBuilder::loadEntry() entry data corrupt!");
                }

                entry->pe_AbsOffset = m_total_raw_size;
                // write pe_AbsOffset to correct position in buffer!
                buf_ptr = &buffer[buf_abs_pos];
                write_byte(entry->pe_AbsOffset, sizeof(entry->pe_AbsOffset),
                        buf_ptr);

                m_total_seq_size += entry->pe_SeqDataSize;
                m_total_raw_size += entry->pe_RawDataSize;
                m_total_seq_compressed_size += compr_size;
#ifdef DEBUG
                if (entry->pe_RawDataSize > maxBaseLength) {
                    maxBaseLength = entry->pe_RawDataSize;
                }
                if (entry->pe_SeqDataSize > longestali) {
                    longestali = entry->pe_SeqDataSize;
                }
#endif
                // write buffer and ...
                fwrite(&buffer[0], 1, buffer_count, m_index_fh);
                // ... compressed sequence data to file
                fwrite(entry->pe_SeqDataCompressed, 1, compr_size, m_index_fh);

            }
            free(entry->pe_SeqDataCompressed);
            entry->pe_SeqDataCompressed = NULL;
        }
    }
    if (buffer) {
        free(buffer);
    }

#ifdef DEBUG
    printf("Longest sequence was %ld bases (alignment size %ld).\n",
            maxBaseLength, longestali);
    printf(
            "Database contains %ld valid entries. (only last message is correct one!)\n"
                    "%llu bytes (alignment) sequence data (%llu bases).\n",
            m_num_entries, m_total_seq_size, m_total_raw_size);
    printf(
            "Compressed sequence data (with dots and hyphens): %llu byte (%llu kb, %llu mb)\n",
            m_total_seq_compressed_size, m_total_seq_compressed_size >> 10,
            m_total_seq_compressed_size >> 20);
#endif
}

/*!
 * \brief Load entries from database using AbstractDataRetriever
 */
void PtpanTreeBuilder::loadEntries() {
    if (m_dr->containsEntries()) {
        m_dr->resetIterator();

#ifdef DEBUG
        printf("Number of threads to use %lu\n", m_settings->getThreadNumber());
#endif

        if (m_settings->getThreadNumber() > 1) {
            for (std::size_t i = 0; i < m_settings->getThreadNumber() - 1;
                    i++) {
                boost::threadpool::schedule(*m_threadpool,
                        boost::bind(&PtpanTreeBuilder::loadEntry, this));
            }
            loadEntry(); // main thread can do some work as well!
            m_threadpool->wait();
            m_threadpool->clear(); // remove pending tasks (are there any?)
            // this was necessary to allow the clearing of the ARB DB!
            // TODO in case of an exception anywhere: threads.interrupt_all(); ??
        } else {
            // single threaded:
            loadEntry();
        }

        if (m_num_entries != m_dr->returnedEntryCount()) {
            printf("Loaded %lu out of %lu entries\n", m_num_entries,
                    m_dr->returnedEntryCount());
            throw std::runtime_error(
                    "PtpanTreeBuilder::loadEntries() failed to get some entries!");
        }
        // TODO FIXME update entry count in index header file!!
        ULONG index = 16 + sizeof(ULONG) + sizeof(UWORD) + sizeof(UWORD) + sizeof(ULONG);
        fseek(m_index_fh, index, SEEK_SET);
        fwrite(&m_num_entries, sizeof(m_num_entries), 1, m_index_fh);
        fseek(m_index_fh, 0, SEEK_END);

        fwrite(&m_total_seq_size, sizeof(m_total_seq_size), 1, m_index_fh);
        fwrite(&m_total_seq_compressed_size,
                sizeof(m_total_seq_compressed_size), 1, m_index_fh);
        fwrite(&m_total_raw_size, sizeof(m_total_raw_size), 1, m_index_fh);

        // map compressed sequences back!
        m_map_file_size = ftell(m_index_fh);
#ifdef DEBUG
        printf("Current file size %ld\n", m_map_file_size);
#endif
    }
}

/*!
 * \brief Free stuff to regain memory
 */
void PtpanTreeBuilder::freeStuff() {
    if (m_ref_entry) {
        free_ptpan_reference_entry(m_ref_entry);
        m_ref_entry = NULL;
    }
    if (m_entries) {
        free_all_ptpan_entries(m_entries);
        free(m_entries);
        m_entries = NULL;
    }
    if (m_index_fh) {
        fflush(m_index_fh);
        fclose(m_index_fh);
        m_index_fh = NULL;
    }
    pmap::const_iterator pit;
    for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
        free_partition(pit->second);
    }
    m_partitions.clear();
}

/*!
 * \brief re-init required stuff
 */
void PtpanTreeBuilder::reInitStuff() {

    m_entries = (struct List *) malloc(sizeof(List));
    NewList(m_entries);

#ifdef DEBUG
    printf("reopen file: '%s'\n", m_index_name.data());
#endif
    m_index_fh = fopen(m_index_name.data(), "a+"); /* open file for output */
    if (!m_index_fh) {
        throw std::runtime_error(
                "PtpanTreeBuilder::initIndexHeader() could not create index header file!");
    }

    m_map_file_buffer = (UBYTE *) mmap(0, m_map_file_size, PROT_READ,
            MAP_SHARED, fileno(m_index_fh), 0);
    if (!m_map_file_buffer) {
        printf(
                "PtpanTreeBuilder::reInitStuff() ERROR mapping index header back\n");
    }

    // start reading at prefix + endian + version + alphabet
    ULONG index = 16 + sizeof(ULONG) + sizeof(UWORD) + sizeof(UWORD);
    // add hash_sum, entry_count and prune_length variable size + feature flag
    index += sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(UBYTE);
    fseek(m_index_fh, index, SEEK_SET);

    // seek over reference sequence
    ULONG ref_size = 0;
    ULONG dummy = fread(&ref_size, sizeof(ref_size), 1, m_index_fh);
    index += sizeof(ULONG);

    if (ref_size > 0) {
        // seek over reference base table and reference sequence
        index += (ref_size + 1) * sizeof(ULONG);
        fseek(m_index_fh, (ref_size + 1) * sizeof(ULONG), SEEK_CUR);
        index += (ref_size + 1);
        fseek(m_index_fh, (ref_size + 1), SEEK_CUR);
    }
    // read custom information
    ULONG size = 0;
    dummy = fread(&size, sizeof(size), 1, m_index_fh);
    if (size > 0) {
        fseek(m_index_fh, size, SEEK_CUR);
    }
    // reload entries partially
    ULONG numentry = 0;
    struct PTPanEntry *pe;
    while (numentry < m_num_entries) {
        UWORD len;
        dummy = fread(&len, sizeof(len), 1, m_index_fh);
        fseek(m_index_fh, len, SEEK_CUR);
        dummy = fread(&len, sizeof(len), 1, m_index_fh);
        fseek(m_index_fh, len, SEEK_CUR);

        pe = (struct PTPanEntry *) calloc(1, sizeof(struct PTPanEntry));
        pe->pe_Num = numentry;

        dummy = fread(&pe->pe_SeqDataSize, sizeof(pe->pe_SeqDataSize), 1,
                m_index_fh);
        dummy = fread(&pe->pe_RawDataSize, sizeof(pe->pe_RawDataSize), 1,
                m_index_fh);
        dummy = fread(&pe->pe_AbsOffset, sizeof(pe->pe_AbsOffset), 1,
                m_index_fh);

        dummy = fread(&pe->pe_CompressedShortcutsCount,
                sizeof(pe->pe_CompressedShortcutsCount), 1, m_index_fh);

        for (ULONG cnt = 0; cnt < pe->pe_CompressedShortcutsCount; cnt++) {
            seek_byte(m_index_fh);
            seek_byte(m_index_fh);
            seek_byte(m_index_fh);
        }

        ULONG count = 0;
        dummy = fread(&count, sizeof(count), 1, m_index_fh);
        if (count > 0) {
            ULONG length = 0;
            UWORD pos_count = 0;
            for (ULONG i = 0; i < count; i++) {
                dummy = fread(&length, sizeof(length), 1, m_index_fh);
                fseek(m_index_fh, length, SEEK_CUR);
                dummy = fread(&pos_count, sizeof(pos_count), 1, m_index_fh);
                for (UWORD j = 0; j < pos_count; j++) {
                    seek_byte(m_index_fh);
                    seek_byte(m_index_fh);
                    fseek(m_index_fh, sizeof(UBYTE), SEEK_CUR); // complement flag
                }
            }
        }

        dummy = fread(&pe->pe_SeqDataCompressedSize,
                sizeof(pe->pe_SeqDataCompressedSize), 1, m_index_fh);

        index = ftell(m_index_fh);
        pe->pe_SeqDataCompressed = &m_map_file_buffer[index];
        fseek(m_index_fh, ((pe->pe_SeqDataCompressedSize >> 3) + 1), SEEK_CUR);
        index += ((pe->pe_SeqDataCompressedSize >> 3) + 1);

        AddTail(m_entries, &pe->pe_Node);
        numentry++;
    } // end while (numentry < m_entry_count)

    if (numentry != m_num_entries) {
        printf("ERROR: Number of entries has changed!\n");
        fclose(m_index_fh);
        throw std::runtime_error(
                "PtpanTree::reInitStuff() Number of entries has changed!");
    }
    fseek(m_index_fh, 0, SEEK_END);
}

/*!
 * \brief Method for merging entry sequence data into raw sequence data
 */
void PtpanTreeBuilder::mergeEntry() {
    struct PTPanEntry *ps = m_dispatcher.getNext();
    ULONG count = 0;

    while (ps) {

        ULONG pval = 0;
        UBYTE code = 0;
        ULONG bitpos = 0;
        ULONG *seqptr = &m_merged_raw_data[0]
                + (ULONG) (floor(ps->pe_AbsOffset / 27));

        ULONG offset = ps->pe_AbsOffset % 27;
        ULONG cnt = 27 - offset;
        ULONG exact_cnt = ps->pe_RawDataSize;
        if (offset == 0) {
            //#ifdef DEBUG
            // printf("exact start (offset %ld)[%ld]!\n", offset, ps->pe_Num);
            //#endif
            cnt = 0; // reset count to get correct trailing size!
        } else {
            //#ifdef DEBUG
            // printf("start offset %ld [%ld]!\n", offset, ps->pe_Num);
            //#endif
            exact_cnt -= cnt;
            for (ULONG i = 0; i < cnt;) { // only increment if valid character is found
                code = m_as->getNextCharacter(ps->pe_SeqDataCompressed, bitpos,
                        count);
                if (m_as->seq_code_valid_table[code]) {
                    pval *= m_as->alphabet_size;
                    pval += m_as->compress_table[code];
                    i++;
                }
            }
            {
                boost::lock_guard<boost::mutex> lock(m_build_mutex);
                std::map<ULONG, ULONG>::iterator it = m_merge_border_map.find(
                        ps->pe_Num - 1);
                // pe_Num 0 starts always at an exact offset so the subtract is no problem
                if (it != m_merge_border_map.end()) {
                    pval += it->second;
                    m_merge_border_map.erase(it);
                    // write out compressed longword (with eof bit)
                    *seqptr++ = (pval
                            << m_as->bits_shift_table[m_as->max_code_fit_long])
                            | m_as->bits_mask_table[m_as->max_code_fit_long];
                } else {
                    m_merge_border_map.insert(
                            std::make_pair(ps->pe_Num - 1, pval));
                    seqptr++; // we don't write but we must increase pointer anyways!
                }
            }
        }
        // check end count ...
        ULONG trailing = (ps->pe_RawDataSize - cnt) % 27;
        exact_cnt -= trailing;
        pval = 0;
        cnt = 0;

        for (ULONG i = 0; i < exact_cnt;) { // only increment if valid character is found
            // get all data and write it shifted and cleaned from . and -
            code = m_as->getNextCharacter(ps->pe_SeqDataCompressed, bitpos,
                    count);
            if (m_as->seq_code_valid_table[code]) {
                // add sequence code
                pval *= m_as->alphabet_size;
                pval += m_as->compress_table[code];
                // check, if storage capacity was reached?
                if (++cnt == m_as->max_code_fit_long) {
                    // write out compressed longword (with eof bit)
                    *seqptr++ = (pval << m_as->bits_shift_table[cnt])
                            | m_as->bits_mask_table[cnt];
                    cnt = 0;
                    pval = 0;
                }
                i++;
            }
        }
        //#ifdef DEBUG
        // printf("trailing %ld [%ld]!\n", trailing, ps->pe_Num);
        //#endif
        if (trailing != 0) {
            // write trailing data or put it into map!
            ULONG i = 0;
            for (; i < trailing;) {
                code = m_as->getNextCharacter(ps->pe_SeqDataCompressed, bitpos,
                        count);
                if (m_as->seq_code_valid_table[code]) {
                    pval *= m_as->alphabet_size;
                    pval += m_as->compress_table[code];
                    i++;
                }
            }
            // we need to correct the position of this part to ease addition
            // of next part
            pval = pval * m_as->power_table[m_as->max_code_fit_long - i];
            {
                boost::lock_guard<boost::mutex> lock(m_build_mutex);
                std::map<ULONG, ULONG>::iterator it = m_merge_border_map.find(
                        ps->pe_Num);
                // pe_Num 0 starts always at an exact offset so the subtract is no problem
                if (it != m_merge_border_map.end()) {
                    pval += it->second;
                    m_merge_border_map.erase(it);
                    // write out compressed longword (with eof bit)
                    *seqptr++ = (pval
                            << m_as->bits_shift_table[m_as->max_code_fit_long])
                            | m_as->bits_mask_table[m_as->max_code_fit_long];
                } else {
                    m_merge_border_map.insert(std::make_pair(ps->pe_Num, pval));
                }
            }
            // for correctness check last read after all is retrieved:
// if ((code = m_as->getNextCharacter(ps->pe_SeqDataCompressed, bitpos,
// count)) != 0xff) {
//#ifdef DEBUG
// printf("ERROR: not at end of sequence!\n");
//#endif
//// TODO this is an error!! it should be 0xff
//// throw something
//}
        }
        ps = m_dispatcher.getNext();
    }
}

/*!
 * \brief Build merged raw data taking entries loaded before
 */
void PtpanTreeBuilder::buildMergedRawData() {
    if (m_entries) {
        // allocate memory for compressed data
        // about the +3: 1 for rounding, 1 for filling up tail with neutral element
        // and 1 for terminal
        m_tmp_raw_file_name = m_settings->getIndexName() + m_tmp_raw_file_name;
#ifdef DEBUG
        printf("temporary file buffer name %s\n", m_tmp_raw_file_name.data());
#endif
        // we use mmap() here! We know the size already!
        // so create a temporary file which can be deleted afterwards (file buffer!)
        ULONG tmp_total = ((m_total_raw_size / m_as->max_code_fit_long) + 3);
        m_merged_raw_data_size = tmp_total * sizeof(ULONG);
#ifdef DEBUG
        printf("compressed merged raw data size %ld\n", m_merged_raw_data_size);
#endif

        FILE *fh_raw;
        if (!(fh_raw = fopen(m_tmp_raw_file_name.data(), "w+"))) {
            throw std::runtime_error(
                    "PtpanTree::buildMergedRawData() Couldn't open temporary file buffer for raw data!");
        }
        fseek(fh_raw, m_merged_raw_data_size - 1, SEEK_SET);
        UBYTE tmp = 0;
        fwrite(&tmp, sizeof(tmp), 1, fh_raw);
        fflush(fh_raw);
        m_merged_raw_data = (ULONG *) mmap(0, m_merged_raw_data_size,
                PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fh_raw), 0);
        fclose(fh_raw);
        if (!m_merged_raw_data) {
            printf(
                    "Sorry, couldn't map %llu KB of memory for the compressed DB!\n",
                    (((m_total_raw_size / m_as->max_code_fit_long) + 3)
                            * sizeof(ULONG)) >> 10);
            throw std::runtime_error(
                    "PtpanTreeBuilder::buildMergedRawData() out of memory");
        }

        if (m_settings->getThreadNumber() > 1) {

            // TODO TEST: REMOVE AFTERWARDS
            //struct PTPanEntry *tps = (struct PTPanEntry *) m_entries->lh_Head;
            //while (tps->pe_Node.ln_Succ) {
            //printf("%ld (%ld)\n", tps->pe_Num, (ULONG)tps);
            //tps = (struct PTPanEntry *) tps->pe_Node.ln_Succ;
            //printf("(%ld)\n", (ULONG)tps);
            //}

            {
                m_dispatcher.setEntryHead(
                        (struct PTPanEntry *) m_entries->lh_Head);
                for (std::size_t i = 0; i < m_settings->getThreadNumber() - 1;
                        i++) {
                    boost::threadpool::schedule(*m_threadpool,
                            boost::bind(&PtpanTreeBuilder::mergeEntry, this));
                }
                mergeEntry();
                m_threadpool->wait();
                m_threadpool->clear();
            }

            ULONG *seqptr = &m_merged_raw_data[tmp_total - 3];
            if (!m_merge_border_map.empty()) {
                assert(m_merge_border_map.size() == 1);
                ULONG pval = m_merge_border_map.begin()->second;
                // write pending bits (with eof bit)
                // padding with neutral seqcode is the only sensible thing
                // to keep code size down and reduce cases
                *seqptr++ = (pval
                        << m_as->bits_shift_table[m_as->max_code_fit_long])
                        | m_as->bits_mask_table[m_as->max_code_fit_long];
            } else {
                // in this case the last entry is an exact one
                // and already written
                seqptr++;
            }
            // add a final padding longword with SEQCODE_N
            *seqptr++ = m_as->bits_mask_table[m_as->max_code_fit_long];
            // and a terminating bit
            *seqptr = m_as->bits_mask_table[0];

        } else {
            // we only take one thread ...
            struct PTPanEntry *ps = (struct PTPanEntry *) m_entries->lh_Head;
            ULONG cnt = 0;
            ULONG pval = 0;
            ULONG abspos = ps->pe_AbsOffset;
            ULONG *seqptr = &m_merged_raw_data[0];
            UBYTE code = 0;
            ULONG count = 0;

            while (ps->pe_Node.ln_Succ) {

                //ULONG offset = ps->pe_AbsOffset % 27;
                //if (offset == 0) {
                //printf("exact start (offset %ld)[%ld]!\n", offset,
                //ps->pe_Num);
                //} else {
                //printf("start offset %ld/%ld remaining %ld [%ld]!\n",
                //offset, cnt, 27 - cnt, ps->pe_Num);
                //}
                //printf(
                //"AbsPos %ld (entry absoff: %llu) at %ld (length %ld) [%ld]\n",
                //abspos, ps->pe_AbsOffset, cnt, ps->pe_RawDataSize,
                //ps->pe_Num);
                //printf(
                //"pointer: %ld (=? %ld)\n",
                //(ULONG) seqptr,
                //(ULONG) (&m_merged_raw_data[0]
                //+ (ULONG) (floor(abspos / 27))));
                //offset = (ps->pe_RawDataSize - (27 - offset)) % 27;
                //if (offset == 0) {
                //printf("exact end (offset %ld)[%ld]!\n", offset,
                //ps->pe_Num);
                //} else {
                //printf("end offset %ld [%ld]!\n", offset, ps->pe_Num);
                //}

                /* compress sequence */
                if (abspos != ps->pe_AbsOffset) {
                    /* species seems to be corrupt! */
                    printf("AbsPos %ld != %llu mismatch at %ld\n", abspos,
                            ps->pe_AbsOffset, ps->pe_Num);
                    throw std::runtime_error(
                            "PtpanTreeBuilder::buildMergedRawData() entry data corrupt!");
                }

                ULONG verlen = 0;
                ULONG bitpos = 0;
                while ((code = m_as->getNextCharacter(ps->pe_SeqDataCompressed,
                        bitpos, count)) != 0xff) {
                    if (m_as->seq_code_valid_table[code]) {
                        /* add sequence code */
                        if (verlen++ < ps->pe_RawDataSize) {
                            abspos++;
                            pval *= m_as->alphabet_size;
                            pval += m_as->compress_table[code];
                            /* check, if storage capacity was reached? */
                            if (++cnt == m_as->max_code_fit_long) {
                                /* write out compressed longword (with eof bit) */
                                *seqptr++ =
                                        (pval << m_as->bits_shift_table[cnt])
                                                | m_as->bits_mask_table[cnt];
                                cnt = 0;
                                pval = 0;
                            }
                        }
                    }
                }
                if (verlen != ps->pe_RawDataSize) {
                    printf("Len %ld != %ld mismatch with %s\n", verlen,
                            ps->pe_RawDataSize, ps->pe_Name);
                    printf(
                            "Please check if this alignment is somehow corrupt!\n");
                }
                ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
            }
            //TODO REMOVE MEfor (int i = 0; i < 5; i++) {
            //if (&m_merged_raw_data[(m_merged_raw_data_size - i)]
            //== seqptr) {
            //printf("yes it is %d\n", i);
            //} else {
            //printf("NO senior %ld != %ld\n",
            //(ULONG)&m_merged_raw_data[(m_merged_raw_data_size - i)],
            //(ULONG)seqptr);
            //}
            //}
            //printf("%ld\n", (ULONG) seqptr - (ULONG) &m_merged_raw_data[0]);
            //ULONG rest = m_total_raw_size % m_as->max_code_fit_long;
            //printf("%ld %ld\n", rest, rest + 1);
            //printf(
            //"%ld\n",
            //(ULONG) seqptr
            //- (ULONG) &m_merged_raw_data[tmp_total-3]);

            // write pending bits (with eof bit)
            // padding with neutral seqcode is the only sensible thing
            // to keep code size down and reduce cases
            *seqptr++ = ((pval
                    * m_as->power_table[m_as->max_code_fit_long - cnt])
                    << m_as->bits_shift_table[m_as->max_code_fit_long])
                    | m_as->bits_mask_table[m_as->max_code_fit_long];
            // add a final padding longword with SEQCODE_N
            *seqptr++ = m_as->bits_mask_table[m_as->max_code_fit_long];
            // and a terminating bit
            *seqptr = m_as->bits_mask_table[0];
        } // end single threaded variant

#ifdef DEBUG
        printf("Done building merged raw data!\n");
#endif

    }
}

/*!
 * \brief Scan merged raw data for partition prefixes
 */
void PtpanTreeBuilder::partitionDetermination() {
    if (!m_merged_raw_data) {
        throw std::runtime_error(
                "PtpanTreeBuilder::prefixScan() merged raw data missing.");
    }

    ULONG partSizeFactor = (ULONG) (1.2
            * (sizeof(Leaf) + sizeof(SfxInnerNode)
                    + (m_as->alphabet_size - 1) * sizeof(ULONG)
                    + sizeof(SfxBorderNode)));

    m_max_partition_size = m_settings->getMemorySize() / partSizeFactor;
#ifdef DEBUG
    printf("factor: %lu\n", partSizeFactor);
#endif

#ifndef DEBUG
    if (m_verbose) {
#endif
    printf("Total number of bases: %lld\n", m_total_raw_size);
    printf("Maximum number of tuples: %lu\n", m_max_partition_size);
#ifndef DEBUG
}
#endif
    // calculate if we may need longer prefixes than default!
    ULONG factor = 1;
    // TODO FIXME respect num_threads! threshold = m_max_partition_size*num_threads !?!?
    while ((m_total_raw_size / pow(3, factor)) > m_max_partition_size) {
        factor++;
    }
#ifdef DEBUG
    printf("max prefix factor: %lu\n", factor + 1);
#endif

    ULONG max_prefix_length = m_settings->getMaxPrefixLength();
    if (factor > max_prefix_length) {
        max_prefix_length = factor + 1;
    }
    if (max_prefix_length > m_as->max_code_fit_long) {
        max_prefix_length = m_as->max_code_fit_long;
    }

#ifdef DEBUG
    printf("maximum allowed prefix length %lu\n", max_prefix_length);
    printf("Partition calculation...\n");
#endif
    if (m_total_raw_size < m_max_partition_size) {
        // everything fits into one partition
        struct PTPanPartition *pp = (struct PTPanPartition *) calloc(1,
                sizeof(struct PTPanPartition));
        if (!pp) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::partitionDetermination() out of memory (single partition)");
        }
        pp->pp_ID = 0;
        pp->pp_Prefix = 0;
        pp->pp_PrefixLen = 0;
        pp->pp_Size = m_total_raw_size;

        pp->pp_TreePruneDepth = m_settings->getPruneLength() + 1;
        pp->pp_TreePruneLength = m_settings->getPruneLength();

        pp->pp_PartitionName = (STRPTR) calloc(
                m_settings->getIndexNameLength() + 6, 1);
        if (!pp->pp_PartitionName) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::partitionDetermination() out of memory");
        }
        strncpy(pp->pp_PartitionName, m_settings->getIndexName().data(),
                m_settings->getIndexNameLength() - 2);
        strcat(pp->pp_PartitionName, "t0000");

        m_partitions.insert(std::make_pair(0, pp));

#ifndef DEBUG
        if (m_verbose) {
#endif
        m_partitions_statistics.push_back(
                PartitionStatistics(pp->pp_ID, m_as->alphabet_size,
                        m_max_partition_size));
#ifndef DEBUG
    }
#endif

    } else {

#ifdef DEBUG
        printf("Scanning through compact data...\n");
#endif
        // make histogram
        ULONG *seqptr = m_merged_raw_data;
        ULONG prefix = 0;
        ULONG cnt = 0;
        ULONG pval = 0;

        ULONG* histogram_table = (ULONG*) calloc(
                m_as->power_table[max_prefix_length], sizeof(ULONG));

        for (ULONG seqpos = 0;
                seqpos < m_total_raw_size + max_prefix_length - 1; seqpos++) {
            // get sequence code from packed data
            if (!cnt) {
                pval = *seqptr++;
                cnt = m_as->getCompressedLongSize(pval);
                pval >>= m_as->bits_shift_table[cnt];
            }

            // generate new prefix code
            prefix %= m_as->power_table[max_prefix_length - 1];
            prefix *= m_as->alphabet_size;
            prefix += (pval / m_as->power_table[--cnt]) % m_as->alphabet_size;

            // increase histogram value
            if (seqpos >= max_prefix_length - 1) {
                histogram_table[prefix]++;
            }
        }

#ifdef DEBUG
        //        ULONG dbg_blanks = 0;
        //        ULONG dbg_sum = 0;
        //        for (ULONG dbg_i = 0; dbg_i < m_as->power_table[max_prefix_length];
        //                dbg_i++) {
        //            if (histogram_table[dbg_i] == 0) {
        //                dbg_blanks++;
        //            } else {
        //                dbg_sum += histogram_table[dbg_i];
        //            }
        //        }
        //        printf(
        //                "dbg_blanks = '%ld' (%ld) [average non-empty: %ld]\n",
        //                dbg_blanks,
        //                m_as->power_table[max_prefix_length],
        //                (dbg_sum
        //                        / (m_as->power_table[max_prefix_length] - dbg_blanks)));

        printf("Determine partitions ...\n");
#endif
        std::deque<PartitionDetermination> refine_queue;
        // initial prefixes of length one!
        UWORD prefix_length = 1;
        ULONG offset = m_as->power_table[max_prefix_length - prefix_length];
        for (UWORD i = 0; i < m_as->alphabet_size; i++) {
            refine_queue.push_back(
                    PartitionDetermination(i, prefix_length, (i * offset),
                    (((i + 1) * offset) - 1)));
                }

        std::set<PartitionDetermination> valid_partitions;
        struct PTPanPartition *pp = NULL;

        // the following optimization is only used if
        // force flag is false!
        bool use_optimization = !m_settings->isForce()
                || (m_settings->getThreadNumber() == 1);
        std::set<PartitionDetermination> best_partition_distribution;
        ULONG best_thread_count = 1;
        ULONG last_thread_count = 1;
        double ratio = m_settings->getPartitionRatio();
        ULONG threshold = (ULONG) (SIGNIFICANCE_THRESHOLD
                * m_max_partition_size);
        bool recheck = false;

#ifndef DEBUG
        if (m_verbose) {
#endif
        printf("memory ratio = '%4.2f' = %ld bases (threshold = %ld)\n", ratio,
                (ULONG) (ratio * m_max_partition_size), threshold);
#ifndef DEBUG
    }
#endif
        bool too_big = true;
        while (too_big) {

            while (!refine_queue.empty()) {
                PartitionDetermination determine = refine_queue.front();
                refine_queue.pop_front();

                cnt = 0;
                for (ULONG j = determine.m_prefix_range_start;
                        j <= determine.m_prefix_range_end; j++) {
                    cnt += histogram_table[j];
                    if (histogram_table[j] == 0) {
                        determine.m_blanks++;
                    }
                }
                if ((ULONG) (cnt * ratio) <= m_max_partition_size) {
                    if (cnt != 0) {
                        determine.m_base_count = cnt;
                        valid_partitions.insert(determine);
                    }
                    // else this partition is empty and we don't need it!
                } else {
                    refinePartition(determine, refine_queue, max_prefix_length);
                }
            }
            if (m_settings->getThreadNumber() > 1) {

                if (use_optimization) {
                    if (best_thread_count == 1
                            && best_partition_distribution.empty()) {
                        // initial case
                        best_partition_distribution = valid_partitions;
                    } else {
                        if (!recheck) {
                            // check partition-to-threads ratio
                            if ((best_partition_distribution.size()
                                    / best_thread_count)
                                    > (valid_partitions.size()
                                            / last_thread_count)) {
                                best_partition_distribution = valid_partitions;
                                best_thread_count = last_thread_count;
                            }
                        }
                    }
                }

                // check if the largest partitions fit into memory parallel
                ULONG size_sum = 0;
                ULONG count;
                if (!use_optimization
                        || (last_thread_count == m_settings->getThreadNumber())) {
                    count = m_settings->getThreadNumber();
                } else {
                    if (recheck) {
                        count = last_thread_count;
                    } else {
                        count = ++last_thread_count;
                    }
                }
                ULONG decr_count = count;
                std::set<PartitionDetermination>::iterator it;
                for (it = valid_partitions.begin();
                        it != valid_partitions.end(); it++) {

                    if (it->m_base_count > threshold) {
                        size_sum += (ULONG) (it->m_base_count * ratio);
                    } else {
                        size_sum += it->m_base_count;
                    }

                    if (--decr_count == 0) {
                        break;
                    }
                }

                if (size_sum > m_max_partition_size) {
                    // remove largest and put it refined into queue!
                    std::set<PartitionDetermination>::iterator it =
                            valid_partitions.begin();
                    if (it != valid_partitions.end()) {
                        refinePartition(*it, refine_queue, max_prefix_length);
                        valid_partitions.erase(it);
                        recheck = true;
                    } else {
                        // TODO ERROR should never happen!
                    }
                } else {
                    recheck = false;
                    // we need to go deeper if best_thread_count < m_settings->getThreadNumber()
                    if (count == m_settings->getThreadNumber()) {
                        // otherwise it seems to fit, so abort!
                        too_big = false;
                        if (use_optimization) {
                            // check partition-to-threads ratio
                            if ((best_partition_distribution.size()
                                    / best_thread_count)
                                    > (valid_partitions.size()
                                            / last_thread_count)) {
                                best_partition_distribution = valid_partitions;
                                best_thread_count = last_thread_count;
                            }
                        }
                    }
                }

            } else {
                too_big = false;
            }
        } // end while(too_big)

#ifdef DEBUG
        printf("Create partition names etc. ...\n");
#endif

        if (use_optimization && !best_partition_distribution.empty()) {
            valid_partitions.swap(best_partition_distribution);
            m_num_threads = best_thread_count;
#ifdef DEBUG
            printf("Best thread count = %ld\n", best_thread_count);
#endif
        } else {
            m_num_threads = m_settings->getThreadNumber();
        }
        cnt = 0;
        std::set<PartitionDetermination>::const_iterator it;
        for (it = valid_partitions.begin(); it != valid_partitions.end();
                it++) {
            pp = (struct PTPanPartition *) calloc(1,
                    sizeof(struct PTPanPartition));
            if (!pp) {
                throw std::runtime_error(
                        "PtpanTreeBuilder::partitionDetermination() out of memory");
            }
            pp->pp_ID = cnt++;
            pp->pp_Prefix = it->m_prefix;
            pp->pp_PrefixLen = it->m_prefix_length;
            pp->pp_Size = it->m_base_count;

            pp->pp_TreePruneDepth = m_settings->getPruneLength() + 1;
            pp->pp_TreePruneLength = m_settings->getPruneLength();

            pp->pp_PartitionName = (STRPTR) calloc(
                    m_settings->getIndexNameLength() + 6, 1);
            if (!pp->pp_PartitionName) {
                throw std::runtime_error(
                        "PtpanTreeBuilder::partitionDetermination() out of memory");
            }
            strncpy(pp->pp_PartitionName, m_settings->getIndexName().data(),
                    m_settings->getIndexNameLength() - 2);
            sprintf(&pp->pp_PartitionName[m_settings->getIndexNameLength() - 2],
                    "t%04ld", pp->pp_ID);

            m_partitions.insert(std::make_pair(pp->pp_Size, pp));

#ifndef DEBUG
            if (m_verbose) {
#endif
            m_partitions_statistics.push_back(
                    PartitionStatistics(pp->pp_ID, m_as->alphabet_size,
                            m_max_partition_size));
            {
                char tempBuffer[128];
                STRPTR tarseq = tempBuffer;
                ULONG ccnt = pp->pp_PrefixLen;
                do {
                    *tarseq++ = m_as->decompress_table[(pp->pp_Prefix
                            / m_as->power_table[--ccnt]) % m_as->alphabet_size];
                } while (ccnt);
                *tarseq = 0;
                printf(
                        "(%ld) Partition %ld - %ld, size %ld, prefix = %ld, prefixlen = %ld %s\n",
                        pp->pp_ID, it->m_prefix_range_start,
                        it->m_prefix_range_end, it->m_base_count, pp->pp_Prefix,
                        pp->pp_PrefixLen, tempBuffer);
            }
#ifndef DEBUG
        }
#endif
        }

        if (histogram_table) {
            free(histogram_table);
        }
    } // end else if all fit into one partition
#ifndef DEBUG
    if (m_verbose) {
#endif
    printf("Using %ld partitions and %ld thread(s).\n", m_partitions.size(),
            m_num_threads);
#ifndef DEBUG
}
#endif
}

/*!
 * \brief Private method to refine a partition
 */
void PtpanTreeBuilder::refinePartition(const PartitionDetermination& pd,
        std::deque<PartitionDetermination>& queue, ULONG max_prefix_length) {
    // refine and add new PartitionDetermination objects
    ULONG chunk_size = m_as->power_table[max_prefix_length
            - (pd.m_prefix_length + 1)];
    for (UWORD i = 0; i < m_as->alphabet_size; i++) {
        PartitionDetermination refine_part = pd;

        refine_part.m_prefix = (refine_part.m_prefix * m_as->alphabet_size) + i;
        refine_part.m_prefix_length++;
        if (refine_part.m_prefix_length > max_prefix_length) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::partitionDetermination() Partition overflow! "
                            "Increase maximum prefix length!");
        }
        refine_part.m_blanks = 0;
        // the order is important:
        refine_part.m_prefix_range_end = (refine_part.m_prefix_range_start
                + (((i + 1) * chunk_size) - 1));
        refine_part.m_prefix_range_start = (refine_part.m_prefix_range_start
                + (i * chunk_size));
        queue.push_back(refine_part);
    }
}

/*!
 * \brief Init index header
 */
void PtpanTreeBuilder::initIndexHeader(const std::string& custom_info) {
#ifdef DEBUG
    printf("Init index header ...\n");
#endif
    // we need the number of entries to write it to the index header first!
    m_num_entries = m_dr->countEntries();

    // Delete old tree first (why, can't we just build a new one and
    // then rename it? Needs some extra disk space then though)
    FILE *test;
    if ((test = fopen(m_settings->getIndexName().data(), "r")) == NULL) {
#ifdef DEBUG
        printf("file non-existent!\n");
#endif
    } else {
        fclose(test);
        if (remove(m_settings->getIndexName().data()) != 0) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::initIndexHeader() could not remove old index");
        }
    }

    m_index_name = m_settings->getIndexName();
    m_index_fh = fopen(m_index_name.data(), "w+"); /* open file for output */
    if (!m_index_fh) {
        throw std::runtime_error(
                "PtpanTreeBuilder::initIndexHeader() could not create index header file!");
    }
    chmod(m_index_name.data(), (int) 0666); // TODO make this portable! (e.g. with boost::filesystem?)

    ULONG endian = 0x01020304;
    UWORD version = FILESTRUCTVERSION;

    /* write 16 bytes of ID */
    fputs("TUM PeTerPAN IDX", m_index_fh);
    /* write a specific code word to allow detection of correct endianess */
    fwrite(&endian, sizeof(endian), 1, m_index_fh);
    /* write file version */
    fwrite(&version, sizeof(version), 1, m_index_fh);

    /* write some global data */
    UWORD type = AbstractAlphabetSpecifics::typeAsInt(m_as->type());
    fwrite(&type, sizeof(type), 1, m_index_fh);
    fwrite(&m_all_hash_sum, sizeof(m_all_hash_sum), 1, m_index_fh);
    fwrite(&m_num_entries, sizeof(m_num_entries), 1, m_index_fh);
    ULONG prune_length = m_settings->getPruneLength();
    fwrite(&prune_length, sizeof(prune_length), 1, m_index_fh);

    // write flag indicating features available or not
    UBYTE feature;
    if (m_settings->getIncludeFeatures()) {
        feature = '0'; // true
    } else {
        feature = '1'; // false
    }
    fwrite(&feature, sizeof(feature), 1, m_index_fh);

    if (m_ref_entry) {
        // write reference entry only if available
        fwrite(&(m_ref_entry->pre_ReferenceSeqSize),
                sizeof(m_ref_entry->pre_ReferenceSeqSize), 1, m_index_fh);

        if (m_ref_entry->pre_ReferenceSeqSize > 0) {
            fwrite(m_ref_entry->pre_ReferenceSeq, 1,
                    m_ref_entry->pre_ReferenceSeqSize + 1, m_index_fh);
            fwrite(m_ref_entry->pre_ReferenceBaseTable, sizeof(ULONG),
                    m_ref_entry->pre_ReferenceSeqSize + 1, m_index_fh);
        }
    } else {
        // the size of this value relies on sizeof(m_ref_entry->pre_ReferenceSeqSize)!!
        ULONG value = 0;
        fwrite(&value, sizeof(value), 1, m_index_fh);
    }

    // write custom information
    ULONG size = custom_info.size();
    fwrite(&size, sizeof(size), 1, m_index_fh);
    if (!custom_info.empty()) {
        fwrite(custom_info.data(), 1, size, m_index_fh);
#ifdef DEBUG
        printf("write custom info: \n'%s'\n", custom_info.data());
#endif
    }

#ifdef DEBUG
    printf("Number of entries seeked: %ld\n", m_num_entries);
#endif
    // we need to reset the number of entries
    m_num_entries = 0;
}

/*!
 * \brief Store index header
 */
void PtpanTreeBuilder::storeIndexHeader() {
#ifdef DEBUG
    printf("store rest of index header ...\n");
#endif
    UWORD num_partitions = m_partitions.size();
    fwrite(&num_partitions, sizeof(num_partitions), 1, m_index_fh);

    // write partition info
    pmap::const_iterator pit;
    for (pit = m_partitions.begin(); pit != m_partitions.end(); pit++) {
        fwrite(&pit->second->pp_ID, sizeof(pit->second->pp_ID), 1, m_index_fh);
        fwrite(&pit->second->pp_Prefix, sizeof(pit->second->pp_Prefix), 1,
                m_index_fh);
        fwrite(&pit->second->pp_PrefixLen, sizeof(pit->second->pp_PrefixLen), 1,
                m_index_fh);
        fwrite(&pit->second->pp_Size, sizeof(pit->second->pp_Size), 1,
                m_index_fh);
    }

    struct PTPanEntry *ps = (struct PTPanEntry *) m_entries->lh_Head;
    while (ps->pe_Node.ln_Succ) {
        ps->pe_SeqDataCompressed = NULL;
        ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
    }
    // unmap index header
    if (m_map_file_buffer) {
        munmap(m_map_file_buffer, m_map_file_size);
    }
    fclose(m_index_fh);
}

/*!
 * \brief Build index tree for each partition
 */
void PtpanTreeBuilder::buildTree() {
#ifdef DEBUG
    printf("Build index tree for each partition ...\n");
#endif
    // build tree for each partition
    if (m_partitions.size() > 1 && m_num_threads > 1) {
        // build partitions parallel ...
        m_partition_dispatcher.setPartitionList(m_partitions);
        for (std::size_t i = 0; i < m_num_threads - 1; i++) {
            boost::threadpool::schedule(*m_threadpool,
                    boost::bind(&PtpanTreeBuilder::buildTreeThread, this));
        }
        buildTreeThread();
        m_threadpool->wait();

    } else {
        pmap::const_reverse_iterator pit;
        for (pit = m_partitions.rbegin(); pit != m_partitions.rend(); pit++) {
            buildPartitionTree(pit->second);
        }
    }
    // unmap merged raw data prior to deleting file
    if (m_merged_raw_data) {
        munmap(m_merged_raw_data, m_merged_raw_data_size);
        m_merged_raw_data = NULL;
    }
    // remove temporary file if existing
    if (remove(m_tmp_raw_file_name.data()) != 0) {
#ifdef DEBUG
        printf("Warning: could not remove temporary file (%s)\n",
                m_tmp_raw_file_name.data());
#endif
    }

#ifndef DEBUG
    if (m_verbose) {
#endif
    printf("Partition ration from settings: '%4.2f'\n",
            m_settings->getPartitionRatio());
    printf("(ID) load = 'percent' (overall/inner/border)\n");
    std::vector<PartitionStatistics>::const_iterator it;
    ULONG count = 0;
    double highest_significant = 0.0;
    double average_ratio = 0.0;

    for (it = m_partitions_statistics.begin();
            it != m_partitions_statistics.end(); it++) {
        std::string significance_str;
        double current_load = it->load();
        if (it->significant()) {
            significance_str = "*";
            average_ratio += current_load;
            count++;
            if (highest_significant < current_load) {
                highest_significant = current_load;
            }
        }
        printf("(%ld) load = '%4.2f' (%ld/%ld/%ld) %s\n", it->getId(),
                current_load, it->getOverallCount(), it->getInnerNodes(),
                it->getBorderNodes(), significance_str.data());
    }
    if (m_partitions_statistics.size() > 1) {
        printf("%ld *significant values: average='%4.2f' (highest='%4.2f')\n",
                count, (average_ratio / count), highest_significant);
        printf("%ld non-significant values\n",
                m_partitions_statistics.size() - count);
    }
#ifndef DEBUG
}
#endif

}

/*!
 * \brief Build index tree for each partition
 */
void PtpanTreeBuilder::buildTreeThread() {
    struct PTPanPartition *pp = m_partition_dispatcher.getNext();
    while (pp) {
        buildPartitionTree(pp);
        pp = m_partition_dispatcher.getNext();
    }
}

/*!
 * \brief Build index tree for partition
 */
void PtpanTreeBuilder::buildPartitionTree(struct PTPanPartition *pp) {

#ifdef DEBUG
    printf("*** Partition %ld/%ld: %ld nodes (%llu%%) ***\n", pp->pp_ID + 1,
            m_partitions.size(), pp->pp_Size,
            pp->pp_Size / (m_total_raw_size / 100));
#endif

    struct PTPanBuildPartition *pbp = pbt_initPtpanBuildPartition(pp);
    pbt_buildMemoryTree(pbp);
    pbt_calculateTreeStats(pbp);
    pbt_writeTreeToDisk(pbp);

#ifndef DEBUG
    if (m_verbose) {
#endif
    m_partitions_statistics.at(pp->pp_ID).setBorderNodes(
            pbp->pbp_NumBorderNodes);
    m_partitions_statistics.at(pp->pp_ID).setInnerNodes(pbp->pbp_NumInnerNodes);
    m_partitions_statistics.at(pp->pp_ID).setOverallCount(pp->pp_Size);
    // TODO gather statistical data! ? edge count data??
#ifndef DEBUG
}
#endif

    pbt_freePtpanBuildPartition(pbp);

#ifdef DEBUG
    printf("*** Partition %ld/%ld done ***\n", pp->pp_ID + 1,
            m_partitions.size());
#endif
}

/*!
 * \brief Init PTPanBuildPartition for given partition
 *
 * This includes initialization of some buffers
 *
 * \brief pp Pointer to associated partition
 */
struct PtpanTreeBuilder::PTPanBuildPartition* PtpanTreeBuilder::pbt_initPtpanBuildPartition(
        struct PTPanPartition *pp) {
    struct PTPanBuildPartition* pbp = (struct PTPanBuildPartition*) calloc(
            sizeof(struct PTPanBuildPartition), 1);

    // setup node buffer
    // we need enough space to support insertion of maximum 1 triple per
    // node (Leaf + SfxInnerNode + SfxBorderNode)
    // and some space for the special case of inner nodes with leafs
    ULONG factor = (sizeof(SfxInnerNode)
            + (m_as->alphabet_size - 1) * sizeof(ULONG)
            + sizeof(SfxBorderNode));
    ULONG threshold = (ULONG) (SIGNIFICANCE_THRESHOLD * m_max_partition_size);
    if (pp->pp_Size > threshold) {
        factor = (ULONG) (factor * m_settings->getPartitionRatio());
    }
    ULONG sfxMemorySize = (ULONG) (pp->pp_Size * factor) & ~3;
    ULONG leafMemorySize = pp->pp_Size * sizeof(Leaf) & ~3;

    if (m_extended_long_edges) {
        pbp->pbp_ExtLongEdgeMemorySize = (pp->pp_Size * sizeof(ULONG)
                * m_long_edge_max_size_factor) & ~3;

        pbp->pbp_ExtLongEdgeBuffer = (UBYTE *) malloc(
                pbp->pbp_ExtLongEdgeMemorySize);
        if (!pbp->pbp_ExtLongEdgeBuffer) {
            free(pbp);
            throw std::runtime_error(
                    "PtpanTreeBuilder::pbt_initPtpanBuildPartition() out of memory (pbp_ExtLongEdgeBuffer)");
        }
        pbp->pbp_ExtLongEdgeOffset = 0;
    }

    pbp->pbp_SfxNodes = (UBYTE *) malloc(sfxMemorySize);
    if (!pbp->pbp_SfxNodes) {
        if (pbp->pbp_ExtLongEdgeBuffer) {
            free(pbp->pbp_ExtLongEdgeBuffer);
        }
        free(pbp);
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_initPtpanBuildPartition() out of memory (pbp_SfxNodes)");
    }
    pbp->pbp_Leafs = (UBYTE *) malloc(leafMemorySize);
    if (!pbp->pbp_Leafs) {
        if (pbp->pbp_ExtLongEdgeBuffer) {
            free(pbp->pbp_ExtLongEdgeBuffer);
        }
        free(pbp->pbp_SfxNodes);
        free(pbp);
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_initPtpanBuildPartition() out of memory (pp_SfxNodes)");
    }

    pbp->pbp_SfxEdgeOffset = 0;
    pbp->pbp_LeafOffset = 0;
    pbp->pbp_LeafBufferSize = 0;
    pbp->pbp_LeafBuffer = NULL;

    pbp->pbp_PTPanPartition = pp;

    /* fill in root node */
    pbp->pbp_SfxRoot =
            (struct SfxInnerNode *) &pbp->pbp_SfxNodes[pbp->pbp_SfxEdgeOffset];
    // init special bits (root is an inner node)
    pbp->pbp_SfxRoot->sn_AlphaMask = INNERNODEBIT;
    pbp->pbp_SfxRoot->sn_Edge = 0;
    pbp->pbp_SfxRoot->sn_EdgeLen = 0;
    pbp->pbp_SfxRoot->sn_StartPos = 0; // we know this is not correct, but it doesn't matter for the root!
    memset(pbp->pbp_SfxRoot->sn_Children, 0,
            m_as->alphabet_size * sizeof(ULONG));
    pbp->pbp_SfxEdgeOffset += sizeof(struct SfxInnerNode)
            + (m_as->alphabet_size - 1) * sizeof(ULONG);
    pbp->pbp_NumInnerNodes = 1;
    pbp->pbp_NumBorderNodes = 0;
    pbp->pbp_NumLeafs = 0;

    pbp->max_prefix_lookup_size = m_settings->getMaxPrefixLookupSize();

#ifdef DEBUG
    printf("Allocated %ld KB for buffers (suffix nodes and leafs).\n",
            (sfxMemorySize + leafMemorySize) >> 10);
#endif

    return pbp;
}

/*!
 * \brief Free PTPanBuildPartition
 *
 * \param pbp pointer to PTPanBuildPartition to free
 */
void PtpanTreeBuilder::pbt_freePtpanBuildPartition(
        struct PTPanBuildPartition* pbp) {
#ifdef DEBUG
    printf(">>> Phase 4: Freeing memory and cleaning it up... <<<\n");
#endif
    /* return some memory not used anymore */
    free(pbp->pbp_SfxNodes);
    pbp->pbp_SfxNodes = NULL;
    if (pbp->pbp_BranchCode) {
        free(pbp->pbp_BranchCode);
        pbp->pbp_BranchCode = NULL;
    }
    if (pbp->pbp_ShortEdgeCode) {
        free(pbp->pbp_ShortEdgeCode);
        pbp->pbp_ShortEdgeCode = NULL;
    }
    if (pbp->pbp_LongEdgeLenCode) {
        free(pbp->pbp_LongEdgeLenCode);
        pbp->pbp_LongEdgeLenCode = NULL;
    }
    if (pbp->pbp_Leafs) {
        free(pbp->pbp_Leafs);
        pbp->pbp_Leafs = NULL;
    }
    if (m_extended_long_edges) {
        free(pbp->pbp_ExtLongEdgeBuffer);
        pbp->pbp_ExtLongEdgeBuffer = NULL;
    }
    if (pbp->pbp_LeafBuffer) {
        free(pbp->pbp_LeafBuffer);
        pbp->pbp_LeafBuffer = NULL;
    }
    free(pbp);
    pbp = NULL;
}

/*!
 * \brief Build index tree for partition in memory
 */
void PtpanTreeBuilder::pbt_buildMemoryTree(struct PTPanBuildPartition* pbp) {
#ifdef DEBUG
    printf(">>> Phase 1: Building up suffix tree in memory... <<<\n");
#endif
    /* main loop to build up the tree */
    /* NOTE: as a special precaution, all longwords have max_code_fit_long code length */

    /* allocate quick lookup table. This is used to speed up traversal of the
     max_prefix_lookup_size lowest levels */
    pbp->pbp_QuickPrefixLookup = (ULONG *) calloc(
            m_as->power_table[pbp->max_prefix_lookup_size], sizeof(ULONG));
    if (!pbp->pbp_QuickPrefixLookup) {
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_buildMemoryTree() out of memory (pbp_QuickPrefixLookup)");
    }

    ULONG len = m_total_raw_size;
    ULONG *seqptr = m_merged_raw_data;

    std::deque<ULONG> raw_slider;
    /* get first x longwords */
    for (int i = 0; i < m_long_edge_max_size_factor + 1; i++) {
        raw_slider.push_back(
                *seqptr++ >> m_as->bits_shift_table[m_as->max_code_fit_long]);
    }

    ULONG cnt = m_as->max_code_fit_long;
    ULONG pos = 0;
    ULONG nodecnt = 0;
#ifdef DEBUG
    pbp->pbp_QuickPrefixCount = 0;
#endif
    len -= m_as->max_code_fit_long * m_long_edge_max_size_factor;
    BOOL takepos;
    ULONG *window = (ULONG *) malloc(
            sizeof(ULONG) * m_long_edge_max_size_factor);

    ULONG prefix_length = pbp->pbp_PTPanPartition->pp_PrefixLen;
    ULONG max_minus_prefix_length = m_as->max_code_fit_long - prefix_length;
    ULONG prefix = pbp->pbp_PTPanPartition->pp_Prefix;

    do {
        *window = ((raw_slider[0] % m_as->power_table[cnt])
                * m_as->power_table[m_as->max_code_fit_long - cnt])
                + (raw_slider[1] / m_as->power_table[cnt]);
        // extending is done after prefix checking!!

        if (prefix_length) {
            /* check, if the prefix matches */
            takepos = (window[0] / m_as->power_table[max_minus_prefix_length]
                    == prefix);
        } else {
            takepos = TRUE; /* it's all one big partition */
        }

        if (takepos) { /* only add this position, if it matches the prefix */

            // if extended long edges -> extend window by missing values
            if (m_extended_long_edges) {
                for (int i = 1; i < m_long_edge_max_size_factor; i++) {
                    window[i] = ((raw_slider[i] % m_as->power_table[cnt])
                            * m_as->power_table[m_as->max_code_fit_long - cnt])
                            + (raw_slider[i + 1] / m_as->power_table[cnt]);
                }
            }

            pbt_insertTreePos(pbp, pos, window);

#ifdef DEBUG
            if ((++nodecnt & 0x3fff) == 0) {
                if ((nodecnt >> 14) % 50) {
                    printf(".");
                    fflush(stdout);
                } else {
                    printf(". %2llu%%\n", pos / (m_total_raw_size / 100));
                }
            }
#else
            ++nodecnt;
#endif
        }

        /* go to next base position */
        cnt--;
        if (len) {
            if (!cnt) {
                /* get next position */
                raw_slider.pop_front();
                raw_slider.push_back(
                        *seqptr++
                                >> m_as->bits_shift_table[m_as->max_code_fit_long]);
                cnt = m_as->max_code_fit_long;
            }
            len--;
        } else {
            if (!cnt) {
                raw_slider.pop_front();
                raw_slider.push_back(0);
                cnt = m_as->max_code_fit_long;
            }
        }
    } while (++pos < m_total_raw_size);

    free(window);

    /* free some memory not required anymore */
    free(pbp->pbp_QuickPrefixLookup);
    pbp->pbp_QuickPrefixLookup = NULL;

#ifdef DEBUG
    printf(
            "Quick Prefix Lookup speedup: %ld%% (%ld)\n",
            (pbp->pbp_QuickPrefixCount * 100)
                    / pbp->pbp_PTPanPartition->pp_Size,
            pbp->pbp_QuickPrefixCount);
#endif

    if (pbp->pbp_PTPanPartition->pp_Size != nodecnt) {
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_buildMemoryTree() "
                        "Predicted partition size didn't match actual generated nodes!");
    }

#ifdef DEBUG
    printf("Nodes         : %6ld\n", nodecnt);
    printf("InnerNodes    : %6ld (%ld%%)\n", pbp->pbp_NumInnerNodes,
            (pbp->pbp_NumInnerNodes * 100) / nodecnt);
    printf("BorderNodes   : %6ld (%ld%%)\n", pbp->pbp_NumBorderNodes,
            (pbp->pbp_NumBorderNodes * 100) / nodecnt);
    printf("Leafs         : %6ld\n", pbp->pbp_NumLeafs);
#endif
}

/*!
 * \brief Insert position in sequence data into memory tree for partition
 */
inline void PtpanTreeBuilder::pbt_insertTreePos(struct PTPanBuildPartition* pbp,
        ULONG pos, ULONG* window) {
    struct SfxNode *sfxnode = (struct SfxNode *) pbp->pbp_SfxRoot;

    // NOTE: we don't introduce the special case for the last parts
    // of the source string as it would lead to unnecessary special cases
    // query hit post filtering will take care of false hits!! Introduced by
    // this strategy! (the raw sequence is padded with SEQCODE_N at the end!)
    ULONG len = pbp->pbp_PTPanPartition->pp_TreePruneLength;

    UWORD window_offset = 0;
    ULONG treepos = 0;
    ULONG treepos_rel = m_as->max_code_fit_long - 1;
    ULONG prefix = 0;
    if (len > pbp->max_prefix_lookup_size) {
        prefix = window[window_offset]
                / m_as->power_table[m_as->max_code_fit_long
                        - pbp->max_prefix_lookup_size];
        ULONG childptr = pbp->pbp_QuickPrefixLookup[prefix];
        /* see if we can use a quick lookup to skip the root levels of the tree */
        if (childptr
                && (pos + pbp->max_prefix_lookup_size < m_total_raw_size)) {
#ifdef DEBUG
            pbp->pbp_QuickPrefixCount++;
#endif
            sfxnode = (struct SfxNode *) childptr; // real pointer, not an offset!
            treepos = pbp->max_prefix_lookup_size;
            treepos_rel = m_as->max_code_fit_long - 1
                    - pbp->max_prefix_lookup_size;
            len -= pbp->max_prefix_lookup_size;
        }
    }

    UBYTE seqcode;
    struct SfxNode *prevnode = (struct SfxNode *) pbp->pbp_SfxRoot; // the previous node

    if (m_extended_long_edges) {
        // ULONG limit = max_code_fit_long_DECREMENTED;
        // TODO FIXME

    } else {
        // all edges fit into max_code_fit_long
        while (len) {
            /* get first sequence code */
            seqcode = (window[0] / m_as->power_table[treepos_rel])
                    % m_as->alphabet_size;
            /* check, if there's already a child starting with seqcode */
            // 00001 -> SEQCODE_N
            // 00010 -> SEQCODE_A
            // 00100 -> SEQCODE_C
            // 01000 -> SEQCODE_G
            // 10000 -> SEQCODE_T
            // mask out highest 3 bit as they are used for other purposes
            if (!((sfxnode->sn_AlphaMask & RELOFFSETMASK) & (1U << seqcode))) {
                // we have no child starting with 'seqcode', so we can add it as a border node
                // with 1 Leaf
                // len contains number of remaining characters for edge
                // allocate a new border node
                struct SfxBorderNode *bordernode =
                        (struct SfxBorderNode *) &pbp->pbp_SfxNodes[pbp->pbp_SfxEdgeOffset];
                pbp->pbp_SfxEdgeOffset += sizeof(struct SfxBorderNode);
                pbp->pbp_NumBorderNodes++;
                bordernode->sn_AlphaMask = 0;
                bordernode->sn_StartPos = pos + treepos;

                bordernode->sn_EdgeLen = (UWORD) len;
                bordernode->sn_Edge = seqcode;
                UINT tmp_pos = treepos_rel - 1;
                while (--len) {
                    bordernode->sn_Edge *= m_as->alphabet_size;
                    bordernode->sn_Edge += (window[0]
                            / m_as->power_table[tmp_pos]) % m_as->alphabet_size;
                    tmp_pos--;
                }

                // add Leaf to this new border node!
                struct Leaf *leaf =
                        (struct Leaf *) &pbp->pbp_Leafs[pbp->pbp_LeafOffset];
                pbp->pbp_LeafOffset += sizeof(struct Leaf);
                pbp->pbp_NumLeafs++;
                leaf->sn_position = pos;

                bordernode->sn_FirstLeaf = (ULONG) leaf;
                bordernode->sn_LastLeaf = (ULONG) leaf;
                bordernode->sn_NumLeafs = 1;
                // correct child entry of previous node
                ((SfxInnerNode *) sfxnode)->sn_Children[seqcode] =
                        (ULONG) bordernode;
                sfxnode->sn_AlphaMask |= (1U << seqcode);

                return;
            }

            // we've got a child, so we advance to it
            prevnode = sfxnode;
            sfxnode =
                    (struct SfxNode *) ((SfxInnerNode *) prevnode)->sn_Children[seqcode];
            // now the suffix node is another inner node or an border node
            // in both cases we must check how many characters are matching
            // check the edge against the characters in the window
            ULONG edge_length = sfxnode->sn_EdgeLen;
            ULONG matchcount = 1; // we already know that the first character matches

            while (--edge_length) {
                if (((window[0] / m_as->power_table[treepos_rel - matchcount])
                        % m_as->alphabet_size)
                        == ((sfxnode->sn_Edge
                                / m_as->power_table[sfxnode->sn_EdgeLen - 1
                                        - matchcount]) % m_as->alphabet_size)) {
                    matchcount++;
                } else {
                    break;
                }
            }
            // check if we need to split or just advance one node further
            if (matchcount < sfxnode->sn_EdgeLen) {
                // split edge as it is only matched partly
                // insert new inner node with edgelen = matchsize and corresponding edge
                // correct sfxnode (border or inner!) as well as previous node and then go on
                // allocate a new inner node
                struct SfxInnerNode *innernode =
                        (struct SfxInnerNode *) &pbp->pbp_SfxNodes[pbp->pbp_SfxEdgeOffset];
                pbp->pbp_SfxEdgeOffset += sizeof(struct SfxInnerNode)
                        + m_as->alphabet_size * sizeof(ULONG);
                pbp->pbp_NumInnerNodes++;
                innernode->sn_AlphaMask = INNERNODEBIT; // it's an inner node!
                innernode->sn_StartPos = sfxnode->sn_StartPos;
                innernode->sn_EdgeLen = (UWORD) matchcount;
                innernode->sn_Edge = seqcode;
                UINT tmp_pos = 1;
                ULONG tmp_length = matchcount;
                while (--tmp_length) {
                    innernode->sn_Edge *= m_as->alphabet_size;
                    innernode->sn_Edge += (sfxnode->sn_Edge
                            / m_as->power_table[sfxnode->sn_EdgeLen - 1
                                    - tmp_pos]) % m_as->alphabet_size;
                    tmp_pos++;
                }
                // correct alpha mask
                UBYTE next_seqcode =
                        (sfxnode->sn_Edge
                                / m_as->power_table[sfxnode->sn_EdgeLen - 1
                                        - tmp_pos++]) % m_as->alphabet_size;
                innernode->sn_AlphaMask |= (1U << next_seqcode);
                // child of new inner node is current sfxnode
                innernode->sn_Children[next_seqcode] = (ULONG) sfxnode;
                // correct sfxnode edge as well
                ULONG tmp_edge = next_seqcode;
                tmp_length = sfxnode->sn_EdgeLen - matchcount;
                while (--tmp_length) {
                    tmp_edge *= m_as->alphabet_size;
                    tmp_edge += (sfxnode->sn_Edge
                            / m_as->power_table[sfxnode->sn_EdgeLen - 1
                                    - tmp_pos]) % m_as->alphabet_size;
                    tmp_pos++;
                }
                sfxnode->sn_EdgeLen -= matchcount;
                sfxnode->sn_Edge = tmp_edge;
                sfxnode->sn_StartPos += matchcount;

                // correct child entry of previous node; seqcode in alphamask does not change
                // because we already had a child!
                ((SfxInnerNode *) prevnode)->sn_Children[seqcode] =
                        (ULONG) innernode;
                // now our node to start from is the new inner node
                sfxnode = (struct SfxNode *) innernode;

                if (treepos + matchcount == pbp->max_prefix_lookup_size) {
                    if (prefix) {
                        // shortcut to new inner node
                        pbp->pbp_QuickPrefixLookup[prefix] = (ULONG) sfxnode;
                    }
                }
            }
            len -= matchcount;
            // now first check if we reached the end
            if (!len) {
                // we've reached the end, so check if we've got an inner node or a border node
                // if sfxnode is a border node, we're done and we just need to add a leaf
                struct Leaf *leaf =
                        (struct Leaf *) &pbp->pbp_Leafs[pbp->pbp_LeafOffset];
                pbp->pbp_LeafOffset += sizeof(struct Leaf);
                pbp->pbp_NumLeafs++;
                leaf->sn_position = pos;

                if (!(sfxnode->sn_AlphaMask & INNERNODEBIT)) {
                    SfxBorderNode *bordernode = (SfxBorderNode *) sfxnode;
                    ((Leaf *) bordernode->sn_LastLeaf)->next = leaf;
                    bordernode->sn_LastLeaf = (ULONG) leaf;
                    bordernode->sn_NumLeafs++;
                    return;
                } else {
                    // if it is an inner node, the algorithm failed!
                    throw std::runtime_error(
                            "Tried to add leaf to an inner node! This should never happen!");
                }
            }
            // if not the end, correct values and go on
            treepos += matchcount;
            treepos_rel -= matchcount;
        } // end while(len)
    }
}

/*!
 * \brief calculate Tree statistics and build some structures like
 * long edge dictionary
 */
void PtpanTreeBuilder::pbt_calculateTreeStats(struct PTPanBuildPartition* pbp) {
#ifdef DEBUG
    printf(">>> Phase 2: Calculating tree statistical data... <<<\n");
#endif
// now we need some statistical data. this includes:
// a) the appearance of all branch combinations (branch combo mask)
// out of a) we will generate a HUFFMAN CODE for the branch ptr mask
// b) the occurrences of the short edges codes and the number of long edges
// out of b) we will generate a HUFFMAN CODE for the edge codes
// moreover, it replaces the short edges by the indexes to the huffman code (sn_StartPos
// is recycled, gets bit 30 set).
// c) generating a HUFFMAN CODE fo the long edge lengths
// d) generating a lookup DICTIONARY for the long edge codes
// d) will allow storing shorter label pointers and only a fraction of memory for lookup
// long edges will be replaced by the position in the dictionary (sn_StartPos is recycled,
// gets bit 31 set).

// allocate branch histogram
    pbp->pbp_BranchCode = (struct HuffCode *) calloc(1UL << m_as->alphabet_size,
            sizeof(struct HuffCode));
    if (!pbp->pbp_BranchCode) {
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_calculateTreeStats() out of memory (pbp_BranchCode)");
    }
    pbt_getTreeStatsBranchHistoRec(pbp, pbp->pbp_SfxRoot);

    // generate further statistical data: edge lengths and combinations
    // allocate short edge code histogram (for edges between of 2-7 base pairs)
    ULONG bitsused = m_as->bits_use_table[m_as->short_edge_max] + 1;
    pbp->pbp_ShortEdgeCode = (struct HuffCode *) calloc((1UL << bitsused),
    sizeof(struct HuffCode));if
(    !pbp->pbp_ShortEdgeCode) {
        throw std::runtime_error(
        "PtpanTreeBuilder::pbt_calculateTreeStats() out of memory (pbp_ShortEdgeCode)");
    }
    // get short edge stats
    pbp->pbp_EdgeCount = 0;
    pbp->pbp_ShortEdgeCode[1].hc_Weight++;

    if (m_extended_long_edges) {
        // TODO FIXME
        // GetTreeStatsShortEdgesRecExt(pp, (struct SfxNode *) pp->pp_SfxRoot);
    } else {
        // root is the only node without ingoing edge,
        // so we do this here to avoid special case in method!
        struct SfxNode * sfxnode = (struct SfxNode *) pbp->pbp_SfxRoot;
        UWORD seqcode = 0;
        do {
            if (sfxnode->sn_AlphaMask & (1UL << seqcode)) {
                // there is a child for seqcode, so go down
                pbt_getTreeStatsShortEdgesRec(
                        pbp,
                        (struct SfxNode *) ((struct SfxInnerNode *) sfxnode)->sn_Children[seqcode]);
            }
        } while (++seqcode < m_as->alphabet_size);
    }

    // define threshold for small edge optimization
    ULONG threshold = pbp->pbp_EdgeCount / 10000;
    // calculate the number of long edges required
    ULONG edgetotal = pbp->pbp_EdgeCount;
    ULONG cnt;
    for (cnt = 1; cnt < (1UL << bitsused); cnt++) {
        if (pbp->pbp_ShortEdgeCode[cnt].hc_Weight > threshold) {
            // code will remain in the table
            edgetotal -= pbp->pbp_ShortEdgeCode[cnt].hc_Weight;
        }
    }
    // code 0 will be used for long edges
    if (edgetotal == 0) {
        pbp->pbp_ShortEdgeCode[0].hc_Weight = 0;
    } else {
        pbp->pbp_ShortEdgeCode[0].hc_Weight = edgetotal + 1; // +1 for the root as dummy count!
    }
#ifdef DEBUG
    printf("Considering %ld (%ld+%ld) edges for the final tree.\n",
            pbp->pbp_EdgeCount, pbp->pbp_EdgeCount - edgetotal, edgetotal);
    printf("Generating huffman code for short edges\n");
#endif
// generate huffman code for short edge, but only for codes that provide at least
// 1/10000th of the edges (other stuff goes into the long edges)
    BuildHuffmanCode(pbp->pbp_ShortEdgeCode, (1UL << bitsused), threshold);

// now generate dictionary for the long edges. This is done by generating an array of
// all long edges and then sorting it according to the length. Then, the dictionary
// string is built up. It replaces the starting pos with the pos in the dictionary
// and sets bit 31 to indicate this.
    pbt_buildLongEdgeDictionary(pbp);
// generate huffman code for edge bit mask, saves 1-2 bits per node
#ifdef DEBUG
    printf("Generating huffman code for branch mask\n");
#endif
    BuildHuffmanCode(pbp->pbp_BranchCode, (1UL << m_as->alphabet_size), 0);
}

/*!
 * \brief get branch histogram (= alphamask histogram)
 */
void PtpanTreeBuilder::pbt_getTreeStatsBranchHistoRec(
        struct PTPanBuildPartition *pbp, struct SfxInnerNode *sfxnode) {
    // it is an inner node!
    ULONG alphamask = sfxnode->sn_AlphaMask & RELOFFSETMASK;
    /* update branch histogramm */
    pbp->pbp_BranchCode[alphamask].hc_Weight++;
    // now traverse children
    UWORD seqcode = 0;
    struct SfxNode *nextnode;
    do {
        if (alphamask & (1UL << seqcode)) {
            // there is a child for seqcode, so go down
            // if it is an inner node, else count border node
            nextnode = (struct SfxNode *) sfxnode->sn_Children[seqcode];
            if (nextnode->sn_AlphaMask & INNERNODEBIT) {
                pbt_getTreeStatsBranchHistoRec(pbp,
                        (struct SfxInnerNode *) nextnode);
            } else {
                // border node count
                pbp->pbp_BranchCode[0].hc_Weight++;
            }
        }
    } while (++seqcode < m_as->alphabet_size);
}

/*!
 * \brief Get short edge statistics
 */
void PtpanTreeBuilder::pbt_getTreeStatsShortEdgesRec(
        struct PTPanBuildPartition *pbp, struct SfxNode *sfxnode) {
    // first increase edgecount (always, short or long edge doesn't matter!)
    pbp->pbp_EdgeCount++;
    // update short edge code histogram
    if (sfxnode->sn_EdgeLen == 1) {
        pbp->pbp_ShortEdgeCode[1].hc_Weight++; // also count length==1 edges
    } else {
        if (sfxnode->sn_EdgeLen <= m_as->short_edge_max) {
            // add stop bit to already calculated prefix
            pbp->pbp_ShortEdgeCode[(sfxnode->sn_Edge
                    | (1UL << m_as->bits_use_table[sfxnode->sn_EdgeLen]))].hc_Weight++;
        }
    }
    if (sfxnode->sn_AlphaMask & INNERNODEBIT) {
        // node is an inner node, so traverse children
        UWORD seqcode = 0;
        do {
            if (sfxnode->sn_AlphaMask & (1UL << seqcode)) {
                // there is a child for seqcode, so go down
                pbt_getTreeStatsShortEdgesRec(
                        pbp,
                        (struct SfxNode *) ((struct SfxInnerNode *) sfxnode)->sn_Children[seqcode]);
            }
        } while (++seqcode < m_as->alphabet_size);
    }
}

/*!
 * \brief Build long edge dictionary from gathered data ...
 */
void PtpanTreeBuilder::pbt_buildLongEdgeDictionary(
        struct PTPanBuildPartition *pbp) {

    STRPTR longDict;
    ULONG dictsize = 0;
    if (pbp->pbp_ShortEdgeCode[0].hc_Weight == 0) {
        // catch special case of no long edges
        pbp->pbp_LongEdgeLenSize = 1;
        pbp->pbp_LongEdgeLenCode = (struct HuffCode *) calloc(
                pbp->pbp_LongEdgeLenSize, sizeof(struct HuffCode));
        pbp->pbp_PTPanPartition->pp_LongDictSize = 1;
        longDict = (STRPTR) malloc(pbp->pbp_PTPanPartition->pp_LongDictSize);
        *longDict = 0;
        pbp->pbp_LongEdges = NULL;

    } else {

        // allocate long edge array
        // 'pbp->pbp_ShortEdgeCode[0].hc_Weight' contains long edge count
        pbp->pbp_LongEdges = (struct SfxNode **) calloc(
                pbp->pbp_ShortEdgeCode[0].hc_Weight, sizeof(struct SfxNode *));
        if (!pbp->pbp_LongEdges) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::pbt_buildLongEdgeDictionary() out of memory (pbp_LongEdges)");
        }
        pbp->pbp_LongEdgeLenSize = 0;
        pbp->pbp_LongEdgeCount = 0;

        struct SfxNode **unique_LongEdges = NULL;
        if (m_extended_long_edges) {
            // TODO FIXME
            // GetTreeStatsExtLongEdgesRec(pp, (struct SfxNode *) pp->pp_SfxRoot);
        } else {
            // hash map for edge as key and suffix node pointer as payload
            // to gather all unique long edges
            boost::unordered_map<ULONG, SfxNode*> unique_long_edges(
                    HASH_SIZE_SEED);
            {
                // root is the only node without ingoing edge,
                // so we do this here to avoid special case in method!
                struct SfxNode * sfxnode = (struct SfxNode *) pbp->pbp_SfxRoot;
                UWORD seqcode = 0;
                do {
                    if (sfxnode->sn_AlphaMask & (1UL << seqcode)) {
                        // there is a child for seqcode, so go down
                        pbt_getTreeStatsLongEdgesRec(
                                pbp,
                                (struct SfxNode *) ((struct SfxInnerNode *) sfxnode)->sn_Children[seqcode],
                                unique_long_edges);
                    }
                } while (++seqcode < m_as->alphabet_size);
            }
#ifdef DEBUG
            printf("unique edges count = '%ld'\n", unique_long_edges.size());
            printf("Long Edge Array filled with %ld entries - ",
                    pbp->pbp_LongEdgeCount);
#endif
            pbp->pbp_unique_long_edges = unique_long_edges.size();
            unique_LongEdges = (struct SfxNode **) calloc(
                    pbp->pbp_unique_long_edges, sizeof(struct SfxNode *));
            boost::unordered_map<ULONG, SfxNode*>::iterator unq_it;
            ULONG cnt = 0;
            for (unq_it = unique_long_edges.begin();
                    unq_it != unique_long_edges.end(); unq_it++) {
                unique_LongEdges[cnt++] = unq_it->second;
            }
        }

#ifdef DEBUG
        if (pbp->pbp_LongEdgeCount != pbp->pbp_ShortEdgeCode[0].hc_Weight) {
            printf("Counting long edges (%ld != %ld)!\n",
                    pbp->pbp_LongEdgeCount,
                    pbp->pbp_ShortEdgeCode[0].hc_Weight);
        } else {
            printf("Long edge count ok\n");
        }

        printf("Sorting (Pass 1)...\n");
#endif
        if (m_extended_long_edges) {
            // TODO FIXME
            // LongEdgeExtLabelCompare
        } else {
            std::sort(unique_LongEdges,
                    unique_LongEdges + pbp->pbp_unique_long_edges,
                    LongEdgeComparator(m_as, m_extended_long_edges));
            // some edges might now have been moved to the front after sorting,
            // but due to the sorting, these will be alternating, with edges of the
            // same length, so fix these in O(n)
            pbt_correctLongEdgeSorting(unique_LongEdges,
                    pbp->pbp_unique_long_edges);
        }

#ifdef DEBUG
        printf("Sorting (Pass 2)...\n");
#endif
        std::sort(unique_LongEdges,
                unique_LongEdges + pbp->pbp_unique_long_edges,
                LongEdgePosCompare);

        struct BuildConcLongEdge* longEdgeConc = pbt_initBuildConcLongEdge(
                pbp->pbp_unique_long_edges);

        // allocate a buffer for building the concatenated long edges
        ULONG tmp_buffer_size = 512UL << 10; // start with 512KB
        STRPTR tmp_buffer = (STRPTR) malloc(tmp_buffer_size);
        if (!tmp_buffer) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::pbt_buildLongEdgeDictionary() out of memory (concatenated long edges buffer)");
        }
        ULONG tmp_buffer_index = 0;
        tmp_buffer = pbt_buildConcatenatedLongEdges(unique_LongEdges,
                pbp->pbp_unique_long_edges, longEdgeConc, tmp_buffer,
                &tmp_buffer_size, &tmp_buffer_index);
        tmp_buffer[tmp_buffer_index] = 0;

#ifdef DEBUG
        printf("Additional intervals generated %ld (%ld KB)\n",
                longEdgeConc->bcll_ConcatenatedLongEdgeCount,
                tmp_buffer_index >> 10);
        printf(
                "Number of affected long edges %ld (%ld %%)\n",
                longEdgeConc->bcll_TouchedCount,
                (longEdgeConc->bcll_TouchedCount * 100)
                        / pbp->pbp_unique_long_edges);
#endif
        // we use our former tmp_buffer as it already contains some data
        pbp->pbp_PTPanPartition->pp_LongDictSize = tmp_buffer_size;
        dictsize = tmp_buffer_index;
        longDict = tmp_buffer;

#ifdef DEBUG
        printf("Temporary uncompressed dictionary size: %ld KB.\n",
                dictsize >> 10);
        printf("Longest edge: %ld\n", pbp->pbp_LongEdgeLenSize);
#endif
        // allocate long edge length histogram
        // first correct size as it must be longest-edge +1 !
        pbp->pbp_LongEdgeLenSize++;
        pbp->pbp_LongEdgeLenCode = (struct HuffCode *) calloc(
                pbp->pbp_LongEdgeLenSize, sizeof(struct HuffCode));
        if (!pbp->pbp_LongEdgeLenCode) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::pbt_buildLongEdgeDictionary() out of memory (pp_LongEdgeLenCode)");
        }
        // count lengths of long edges (now the non-unique ones!)
        for (ULONG cnt = 0; cnt < pbp->pbp_LongEdgeCount; cnt++) {
            pbp->pbp_LongEdgeLenCode[pbp->pbp_LongEdges[cnt]->sn_EdgeLen].hc_Weight++;
        }
#ifdef DEBUG
        printf("Generating huffman code for long edges length\n");
#endif
        // generate huffman code for edge lengths
        BuildHuffmanCode(pbp->pbp_LongEdgeLenCode, pbp->pbp_LongEdgeLenSize, 0);

        struct ConcatenatedLongEdge *cle;
        STRPTR seq_buffer = (STRPTR) calloc(
                (m_long_edge_max_size_factor * m_as->max_code_fit_long) + 1, 1);
        ULONG edgelen;
        // add rest of unique long edges or correct start position if already present
        for (ULONG count = 0; count < pbp->pbp_unique_long_edges; count++) {

            if (unique_LongEdges[count]->sn_AlphaMask
                    & (1UL << RELOFFSETBITS)) {
                cle =
                        (struct ConcatenatedLongEdge *) unique_LongEdges[count]->sn_StartPos;
                // now correct long edge start pos by adding offset to ConcatenatedLongEdge start pos
                unique_LongEdges[count]->sn_StartPos = cle->start_pos
                        + unique_LongEdges[count]->sn_Edge + 1;
                // correct by 1 as it is expected to omit the first char!

            } else {
                // non-concatenated unique edges
                if (m_extended_long_edges) {
                    // TODO FIXME
                } else {
                    edgelen = unique_LongEdges[count]->sn_EdgeLen;
                    // decompress edge
                    STRPTR buff_ptr = &seq_buffer[0];
                    do {
                        *buff_ptr++ =
                                m_as->decompress_table[(unique_LongEdges[count]->sn_Edge
                                        / m_as->power_table[--edgelen])
                                        % m_as->alphabet_size];
                    } while (edgelen);
                    *buff_ptr = 0;

                    // first check if dictionary size is sufficient
                    if (dictsize + unique_LongEdges[count]->sn_EdgeLen
                            >= pbp->pbp_PTPanPartition->pp_LongDictSize) {
                        STRPTR newptr;
                        // double the size of the buffer
                        pbp->pbp_PTPanPartition->pp_LongDictSize <<= 1;
                        if ((newptr = (STRPTR) realloc(longDict,
                                pbp->pbp_PTPanPartition->pp_LongDictSize))) {
                            longDict = newptr;
                            //printf( "Expanded Dictionary to %ld bytes.\n", pp->pp_LongDictSize);
                        } else {
                            free(longDict);
                            free(unique_LongEdges);
                            free(pbp->pbp_LongEdges);
                            throw std::runtime_error(
                                    "PtpanTreeBuilder::pbt_buildLongEdgeDictionary() out of memory (expand long edge dict 3)");
                        }
                    }
                    unique_LongEdges[count]->sn_StartPos = dictsize + 1;
                    memcpy(&longDict[dictsize], seq_buffer,
                            unique_LongEdges[count]->sn_EdgeLen);
                    dictsize += unique_LongEdges[count]->sn_EdgeLen;
                    // add 0 to end!
                    longDict[dictsize] = 0;
                }
            }
        }
        if (seq_buffer) {
            free(seq_buffer);
        }

#ifdef DEBUG
        printf("Temporary uncompressed dictionary size: %ld KB (%ld).\n",
                dictsize >> 10, dictsize);
#endif

        // afterwards update non-unique long edges?? TODO FIXME or can we do this when writing??
        struct SfxNode* unique_node = NULL;
        for (ULONG cnt = 0; cnt < pbp->pbp_LongEdgeCount; cnt++) {
            if (pbp->pbp_LongEdges[cnt]->sn_AlphaMask & NONUNIQUELONGEDGEBIT) {
                unique_node =
                        (struct SfxNode*) pbp->pbp_LongEdges[cnt]->sn_StartPos;
                pbp->pbp_LongEdges[cnt]->sn_StartPos = unique_node->sn_StartPos;
            }
        }

        pbt_freeBuildConcLongEdge(longEdgeConc);
        if (unique_LongEdges) {
            free(unique_LongEdges);
        }

    } // end else has long edges
    if (pbp->pbp_LongEdges) {
        free(pbp->pbp_LongEdges);
    }

    pbp->pbp_PTPanPartition->pp_LongDictSize = dictsize;

    // calculate bits usage
    pbp->pbp_PTPanPartition->pp_LongRelPtrBits = 1;
    while ((1UL << pbp->pbp_PTPanPartition->pp_LongRelPtrBits) < dictsize) {
        pbp->pbp_PTPanPartition->pp_LongRelPtrBits++;
    }
    // compress sequence to save memory
    if (!(pbp->pbp_PTPanPartition->pp_LongDictRaw = m_as->compressSequence(
            longDict))) {
        free(longDict);
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_buildLongEdgeDictionary() out of memory (pp_LongDictRaw)");
    }
    if (longDict) {
        free(longDict);
    }
#ifdef DEBUG
    printf("Final compressed dictionary size: %ld bytes.\n",
            ((dictsize / m_as->max_code_fit_long) + 1) * sizeof(ULONG));
#endif

}

/*!
 * \brief Get long edge statistics
 */
void PtpanTreeBuilder::pbt_getTreeStatsLongEdgesRec(
        struct PTPanBuildPartition *pbp, struct SfxNode *sfxnode,
        boost::unordered_map<ULONG, SfxNode*>& unique_long_edges) {
    // update short edge code histogram
    if (sfxnode->sn_EdgeLen == 1) {
        // replace sn_StartPos by huffman code index
        sfxnode->sn_Edge = 1;
        // it's a short edge (don't mix up with short label bit!)
        sfxnode->sn_AlphaMask |= SHORTEDGEBIT;

    } else {
        // edge length > 1 since we avoided root special case!
        // add stop bit to pre-calculated prefix
        ULONG prefix = sfxnode->sn_Edge
                | (1UL << m_as->bits_use_table[sfxnode->sn_EdgeLen]);
        if (sfxnode->sn_EdgeLen <= m_as->short_edge_max) {
            // check, if this edge doesn't have a huffman code
            if (!pbp->pbp_ShortEdgeCode[prefix].hc_CodeLength) {

                boost::unordered_map<ULONG, SfxNode*>::iterator uit =
                        unique_long_edges.find(prefix); // take 'prefix' to cover leading Ns
                if (uit != unique_long_edges.end()) {
                    sfxnode->sn_StartPos = (ULONG) uit->second;
                    sfxnode->sn_AlphaMask |= NONUNIQUELONGEDGEBIT;
                } else {
                    // insert as it is a new one!
                    unique_long_edges.insert(std::make_pair(prefix, sfxnode));
                }

                pbp->pbp_LongEdges[pbp->pbp_LongEdgeCount++] = sfxnode;
            } else {
                // replace sn_StartPos by huffman code index
                sfxnode->sn_Edge = prefix;
                // it's a short edge (don't mix up with short label bit!)
                sfxnode->sn_AlphaMask |= SHORTEDGEBIT;
            }
        } else {

            boost::unordered_map<ULONG, SfxNode*>::iterator uit =
                    unique_long_edges.find(prefix); // take 'prefix' to cover leading Ns
            if (uit != unique_long_edges.end()) {
                sfxnode->sn_StartPos = (ULONG) uit->second;
                sfxnode->sn_AlphaMask |= NONUNIQUELONGEDGEBIT;
            } else {
                // insert as it is a new one!
                unique_long_edges.insert(std::make_pair(prefix, sfxnode));
            }
            // this is a long edge anyway and has no huffman code
            pbp->pbp_LongEdges[pbp->pbp_LongEdgeCount++] = sfxnode;

            if (pbp->pbp_LongEdgeLenSize < sfxnode->sn_EdgeLen) {
                pbp->pbp_LongEdgeLenSize = sfxnode->sn_EdgeLen;
            }
        }
    }
    // recursion:
    if (sfxnode->sn_AlphaMask & INNERNODEBIT) {
        // node is an inner node, so traverse children
        UWORD seqcode = 0;
        do {
            if (sfxnode->sn_AlphaMask & (1UL << seqcode)) {
                // there is a child for seqcode, so go down
                pbt_getTreeStatsLongEdgesRec(
                        pbp,
                        (struct SfxNode *) ((struct SfxInnerNode *) sfxnode)->sn_Children[seqcode],
                        unique_long_edges);
            }
        } while (++seqcode < m_as->alphabet_size);
    }
}

/*!
 * \brief correct the sorting of long edges
 */
void PtpanTreeBuilder::pbt_correctLongEdgeSorting(struct SfxNode ** edges,
        ULONG edge_count) {
    ULONG edge_length_pred;
    ULONG edge_length_current;
    for (ULONG cnt = edge_count - 1; cnt > 0; cnt--) {

        if ((edges[cnt - 1]->sn_StartPos != edges[cnt]->sn_StartPos)) {

            edge_length_pred = edges[cnt - 1]->sn_EdgeLen;
            edge_length_current = edges[cnt]->sn_EdgeLen;

            if (edge_length_pred == edge_length_current) {
                if (edges[cnt - 1]->sn_Edge == edges[cnt]->sn_Edge) {
                    edges[cnt - 1]->sn_StartPos = edges[cnt]->sn_StartPos;
                }
            }
            if (edge_length_pred < edge_length_current) {
                // check if shorter edge is prefix of longer one
                if (edges[cnt - 1]->sn_Edge
                        == edges[cnt]->sn_Edge
                                / m_as->power_table[edge_length_current
                                        - edge_length_pred]) {
                    edges[cnt - 1]->sn_StartPos = edges[cnt]->sn_StartPos;
                }
            }
        }
    }
}

/*!
 * \brief Init supporting structure
 */
struct PtpanTreeBuilder::BuildConcLongEdge* PtpanTreeBuilder::pbt_initBuildConcLongEdge(
        ULONG edge_count) {
    struct BuildConcLongEdge *longEdgeConc =
            (struct BuildConcLongEdge *) calloc(sizeof(BuildConcLongEdge), 1);

    longEdgeConc->bcll_ConcatenatedLongEdges = (UBYTE *) calloc(
            (edge_count / 2) + 1, sizeof(struct ConcatenatedLongEdge));
    longEdgeConc->bcll_ConcatenatedLongEdgeCount = 0;
    longEdgeConc->bcll_ConcatenatedLongEdgeOffset = 0;
    longEdgeConc->bcll_TouchedCount = 0;

    return longEdgeConc;
}
/* \\\ */

/*!
 * \brief Free supporting structure
 */
void PtpanTreeBuilder::pbt_freeBuildConcLongEdge(
        struct BuildConcLongEdge *bcll) {
    if (bcll) {
        if (bcll->bcll_ConcatenatedLongEdges) {
            free(bcll->bcll_ConcatenatedLongEdges);
            bcll->bcll_ConcatenatedLongEdges = NULL;
        }
        free(bcll);
        bcll = NULL;
    }
}

/*!
 * \brief Build concatenated long edges
 */
STRPTR PtpanTreeBuilder::pbt_buildConcatenatedLongEdges(struct SfxNode ** edges,
        ULONG edge_count, struct BuildConcLongEdge *longEdgeConc,
        STRPTR tmp_buffer, ULONG *tmp_buffer_size, ULONG *tmp_buffer_index) {
    struct ConcatenatedLongEdge *current = NULL;
    ULONG ivalstart = 0;
    ULONG ivalend = 0;
    STRPTR tmp_buffer_local = tmp_buffer;
    // examine edges and create bigger intervals
    for (ULONG cnt = 0; cnt < edge_count; cnt++) {

        // check if buffer is big enough; if not, extend it
        if (*tmp_buffer_index + edges[cnt]->sn_EdgeLen >= *tmp_buffer_size) {
            STRPTR newptr;
            // double the size of the buffer
            *tmp_buffer_size <<= 1;
            if ((newptr = (STRPTR) realloc(tmp_buffer_local,
                    *tmp_buffer_size))) {
                tmp_buffer_local = newptr;
            } else {
                free(tmp_buffer_local);
                throw std::runtime_error(
                        "PtpanTreeBuilder::pbt_buildConcatenatedLongEdges() out of memory");
            }
        }

        if (edges[cnt]->sn_StartPos > ivalend) {
            // the next edge label is too far away, so if necessary update
            // length of temporary concatenated long edge and go to next
            ivalstart = edges[cnt]->sn_StartPos;
            ivalend = ivalstart + edges[cnt]->sn_EdgeLen - 1;
            current = NULL; // not present as it is a new one out of range of former one!

        } else {
            if (!current) {
                current =
                        (struct ConcatenatedLongEdge *) &longEdgeConc->bcll_ConcatenatedLongEdges[longEdgeConc->bcll_ConcatenatedLongEdgeOffset];
                longEdgeConc->bcll_ConcatenatedLongEdgeOffset +=
                        sizeof(struct ConcatenatedLongEdge);
                longEdgeConc->bcll_ConcatenatedLongEdgeCount++;
                current->start_pos = *tmp_buffer_index;
                current->length = edges[cnt - 1]->sn_EdgeLen;
                ULONG ccnt = edges[cnt - 1]->sn_EdgeLen;
                STRPTR buffer = &tmp_buffer_local[*tmp_buffer_index];
                if (m_extended_long_edges) {
                    // TODO FIXME
                    // extend edges...
                } else {
                    do {
                        *buffer++ =
                                m_as->decompress_table[(edges[cnt - 1]->sn_Edge
                                        / m_as->power_table[--ccnt])
                                        % m_as->alphabet_size];
                        (*tmp_buffer_index)++;
                    } while (ccnt);
                }
                *buffer = 0;
                // and don't forget to mark the SfxNode to prevent doing work twice
                edges[cnt - 1]->sn_Edge = 0; // the former edge contains the offset now!
                // the former StartPos contains the pointer to ConcatenatedLongEdge
                edges[cnt - 1]->sn_StartPos = (ULONG) current;
                edges[cnt - 1]->sn_AlphaMask |= 1UL << RELOFFSETBITS;
                longEdgeConc->bcll_TouchedCount++;
            }
            // check, if we have to enlarge this interval...
            if ((edges[cnt]->sn_StartPos + edges[cnt]->sn_EdgeLen - 1
                    > ivalend)) {
                ULONG count = edges[cnt]->sn_StartPos + edges[cnt]->sn_EdgeLen
                        - 1 - ivalend;
                ULONG tmp_pos = edges[cnt]->sn_EdgeLen - count;
                STRPTR buffer = &tmp_buffer_local[*tmp_buffer_index];
                if (m_extended_long_edges) {
                    // TODO FIXME
                    // extend edges...
                } else {
                    ULONG tmp = edges[cnt]->sn_EdgeLen - 1;
                    do {
                        *buffer++ = m_as->decompress_table[(edges[cnt]->sn_Edge
                                / m_as->power_table[tmp - tmp_pos])
                                % m_as->alphabet_size];
                        (*tmp_buffer_index)++;
                        tmp_pos++;
                        current->length++;
                    } while (--count);
                }
                *buffer = 0;
                ivalend = edges[cnt]->sn_StartPos + edges[cnt]->sn_EdgeLen - 1;
            }
            // update values to prevent making work twice; this will cover
            // edges which are part of former ones as well!
            // Edge -> offset
            edges[cnt]->sn_Edge = edges[cnt]->sn_StartPos - ivalstart; // the former edge contains the offset now!
            // the former StartPos contains the pointer to ConcatenatedLongEdge
            edges[cnt]->sn_StartPos = (ULONG) current;
            // set updated bit!
            edges[cnt]->sn_AlphaMask |= 1UL << RELOFFSETBITS;
            longEdgeConc->bcll_TouchedCount++;
        }
    } // end for all long edges
    return tmp_buffer_local;
}

/*!
 * \brief Write index tree for partition to disk
 */
void PtpanTreeBuilder::pbt_writeTreeToDisk(struct PTPanBuildPartition *pbp) {
#ifdef DEBUG
    printf(">>> Phase 3: Writing tree to secondary storage... <<<\n");
#endif
    /* after we have prepared all the codecs and compression tables, we have to modify
     the tree to be able to store it on the disk. Bottom up approach.
     a) count the space required for the cut off leaves. They are stored as compressed
     arrays, each array holding at least two leaves. The leaves in the upper levels
     of the tree are not saved there, as a pointer to the leaf array containing only
     one leaf would be a waste of space (instead they are stored directly as a child
     ptr).
     b) traverse the tree in DFS order from lowest level to the root, from right to the
     left, enter relative pointers in the ChildPtr[] array, count the space
     consumption.
     */
    /* calculate the relative pointers and the tree traversal */
    pbp->pbp_PTPanPartition->pp_DiskTreeSize = 0;

    // WriteNodesBuffer initialization!
    ULONG writeNodesBufferSize = pbp->pbp_NumInnerNodes
            + pbp->pbp_NumBorderNodes;
    ULONG *writeNodesBuffer = NULL;
    if (writeNodesBufferSize > 0) {
        writeNodesBuffer = (ULONG *) malloc(
                writeNodesBufferSize * sizeof(ULONG));
        if (!writeNodesBuffer) {
            throw std::runtime_error(
                    "PtpanTreeBuilder::pbt_writeTreeToDisk() out of memory (writeNodesBuffer)");
        }
        pbp->pbp_WriteNodesBufferPtr = writeNodesBuffer;
    }
    // intial value for write buffer; may increase during
    // size pre-calculation
    pbp->pbp_DiskBufferSize = 128UL << 10; // 128 Kb initial size

    ULONG tempDiskTreeSize = pbt_fixRelativePointersRec(pbp,
            (struct SfxNode *) pbp->pbp_SfxRoot);
    pbp->pbp_PTPanPartition->pp_DiskTreeSize += tempDiskTreeSize;
#ifdef DEBUG
    printf("Total size on disk: %ld KB\n",
            pbp->pbp_PTPanPartition->pp_DiskTreeSize >> 10);

#endif
    /* now finally write it to disk */
    FILE *partitionFile = fopen(pbp->pbp_PTPanPartition->pp_PartitionName, "w");
    if (!partitionFile) {
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_writeTreeToDisk() Couldn't open partition file for writing!");
    }
    pbt_writeTreeHeader(pbp, partitionFile);

    /* need disk buffer for writing suffix tree */
// buffer size is defined above and adopted if too small
    UBYTE *diskBuffer = (UBYTE *) calloc(1, pbp->pbp_DiskBufferSize);

//printf( "Diskbuffer: %ld KB\n", pp->pp_DiskBufferSize >> 10);
    ULONG diskPos = 0;
    ULONG bytessaved = 0;

#ifdef DEBUG
    printf("Writing (%ld KB)\n",
            pbp->pbp_PTPanPartition->pp_DiskTreeSize >> 10);
#endif
    struct SfxNode *sfxnode;
    ULONG count = 0;
    ULONG packsize;
    while (count < writeNodesBufferSize) {
        sfxnode = (struct SfxNode *) writeNodesBuffer[count];
        packsize = 0;
        /* it is always an inner or border node */
        packsize = pbt_writePackedNode(pbp, sfxnode, &diskBuffer[diskPos]);
        diskPos += packsize;
        bytessaved += packsize;

        if (!(sfxnode->sn_AlphaMask & INNERNODEBIT)) {
            // it is a border node, so write leafs as well
            packsize = pbt_writePackedLeaf(pbp,
                    (struct SfxBorderNode *) sfxnode, &diskBuffer[diskPos]);
            diskPos += packsize;
            bytessaved += packsize;
        }
        /* check if disk buffer is full enough to write a new chunk */
        if (diskPos > (pbp->pbp_DiskBufferSize >> 1)) {
            fwrite(diskBuffer, diskPos, 1, partitionFile);
            diskPos = 0;
        }
        count++;
    } // end while

    if (diskPos) {
        fwrite(diskBuffer, diskPos, 1, partitionFile);
    }

    fclose(partitionFile);
    free(diskBuffer);
    free(writeNodesBuffer);

    if (bytessaved != pbp->pbp_PTPanPartition->pp_DiskTreeSize) {
        throw std::runtime_error(
                "PtpanTreeBuilder::pbt_writeTreeToDisk() Calculated tree size did not match written data");
    }
}

/*!
 * \brief Write partition tree header to disk
 */
void PtpanTreeBuilder::pbt_writeTreeHeader(struct PTPanBuildPartition *pbp,
        FILE *fh) {
    /* write 16 bytes of ID */
    fputs("TUM PeTerPAN P3I", fh);
    /* checksum to verify */
    fwrite(&m_all_hash_sum, sizeof(m_all_hash_sum), 1, fh);
    struct PTPanPartition *ptpanPartition = pbp->pbp_PTPanPartition;
    /* write partition data */
    fwrite(&ptpanPartition->pp_ID, sizeof(ptpanPartition->pp_ID), 1, fh);
    fwrite(&ptpanPartition->pp_Prefix, sizeof(ptpanPartition->pp_Prefix), 1,
            fh);
    fwrite(&ptpanPartition->pp_PrefixLen, sizeof(ptpanPartition->pp_PrefixLen),
            1, fh);
    fwrite(&ptpanPartition->pp_Size, sizeof(ptpanPartition->pp_Size), 1, fh);
    fwrite(&ptpanPartition->pp_TreePruneDepth,
            sizeof(ptpanPartition->pp_TreePruneDepth), 1, fh);
    fwrite(&ptpanPartition->pp_TreePruneLength,
            sizeof(ptpanPartition->pp_TreePruneLength), 1, fh);
    fwrite(&ptpanPartition->pp_LongDictSize,
            sizeof(ptpanPartition->pp_LongDictSize), 1, fh);
    fwrite(&ptpanPartition->pp_LongRelPtrBits,
            sizeof(ptpanPartition->pp_LongRelPtrBits), 1, fh);

    /* branch code */
    WriteHuffmanTree(pbp->pbp_BranchCode, 1UL << m_as->alphabet_size, fh);

    /* short edge code */
    WriteHuffmanTree(pbp->pbp_ShortEdgeCode,
            1UL << (m_as->bits_use_table[m_as->short_edge_max] + 1), fh);

    /* long edge len code */
    WriteHuffmanTree(pbp->pbp_LongEdgeLenCode, pbp->pbp_LongEdgeLenSize, fh);

    /* long dictionary */
    pbp->pbp_LongDictRawSize = ((ptpanPartition->pp_LongDictSize
            / m_as->max_code_fit_long) + 1) * sizeof(ULONG);
    fwrite(&pbp->pbp_LongDictRawSize, sizeof(pbp->pbp_LongDictRawSize), 1, fh);
    fwrite(ptpanPartition->pp_LongDictRaw, pbp->pbp_LongDictRawSize, 1, fh);

    /* write tree length */
    fwrite(&ptpanPartition->pp_DiskTreeSize,
            sizeof(ptpanPartition->pp_DiskTreeSize), 1, fh);
}
/* \\\ */

/*!
 * \brief Fix relative pointer recursive
 */
ULONG PtpanTreeBuilder::pbt_fixRelativePointersRec(
        struct PTPanBuildPartition *pbp, struct SfxNode *sfxnode) {
    // first push into buffer for saving writing order
    // as we loose the tree structure afterwards
    *pbp->pbp_WriteNodesBufferPtr++ = (ULONG) sfxnode;

    ULONG nodesize = 0;
    bool inner_node = sfxnode->sn_AlphaMask & INNERNODEBIT ? true : false;
    if (inner_node) {
        struct SfxInnerNode * inner = (struct SfxInnerNode *) sfxnode;
        // node is an inner node, so traverse children
        UWORD seqcode = 0;
        ULONG tmp_nodesize = 0;
        do {
            if (sfxnode->sn_AlphaMask & (1UL << seqcode)) {
                // there is a child for seqcode, so go down
                tmp_nodesize = pbt_fixRelativePointersRec(pbp,
                        (struct SfxNode *) inner->sn_Children[seqcode]);
                inner->sn_Children[seqcode] = nodesize;
                nodesize += tmp_nodesize;
            }
        } while (++seqcode < m_as->alphabet_size);
    } else {
        // it is a border node
        nodesize += pbt_calcPackedLeafSize(pbp,
                (struct SfxBorderNode *) sfxnode);
    }
    nodesize += pbt_calcPackedNodeSize(pbp, sfxnode);
    if (!inner_node) {
        while (nodesize > (pbp->pbp_DiskBufferSize >> 1)) {
            // double the write buffer size if it would be too small
            pbp->pbp_DiskBufferSize = pbp->pbp_DiskBufferSize << 1;
#ifdef DEBUG
            printf("Doubled write buffer size to %ld KB\n",
                    pbp->pbp_DiskBufferSize >> 10);
#endif
        }
    }

    return (nodesize);
}

/*!
 * \brief Calculate packed node size
 */
inline ULONG PtpanTreeBuilder::pbt_calcPackedNodeSize(
        struct PTPanBuildPartition *pbp, struct SfxNode *sfxnode) {
    ULONG bitscnt = 0;
    if (sfxnode->sn_EdgeLen != 0) {
        /* huffman code for short / long edge */
        if (sfxnode->sn_AlphaMask & (1UL << (RELOFFSETBITS + 1))) {
            /* short edge */
            bitscnt += pbp->pbp_ShortEdgeCode[sfxnode->sn_Edge].hc_CodeLength;
        } else {
            /* long edge */
            bitscnt += pbp->pbp_ShortEdgeCode[0].hc_CodeLength;
            bitscnt +=
                    pbp->pbp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength;
            bitscnt += pbp->pbp_PTPanPartition->pp_LongRelPtrBits;
        }
    } else {
        /* special root handling */
        bitscnt += pbp->pbp_ShortEdgeCode[1].hc_CodeLength;
    }
    if (sfxnode->sn_AlphaMask & INNERNODEBIT) {
        // it's an inner node
        ULONG alphamask = sfxnode->sn_AlphaMask & RELOFFSETMASK;
        /* branch code */
        bitscnt += pbp->pbp_BranchCode[alphamask].hc_CodeLength;
        /* child pointers */
        /* msb = {0}   -> rel is next
         msb = {10}  ->  6 bit rel node ptr
         msb = {111} -> 11 bit rel node ptr
         msb = {1100} -> 27 bit rel node ptr (large enough for a jump over 128 MB)
         msb = {1101} -> 36 bit rel node ptr (large enough for a jump over 64 GB)
         add another opcode to allow longer jumps!
         */
        struct SfxInnerNode * inner = (struct SfxInnerNode *) sfxnode;
        UWORD seqcode = 0;
        ULONG newbase = 0;
        ULONG val;
        do {
            if (alphamask & (1UL << seqcode)) {
                val = inner->sn_Children[seqcode];
                val -= newbase;
                newbase += val;
                //printf("Dist: %6ld\n", val);
                if (val == 0) {
                    bitscnt++;
                } else {
                    if (val < (1UL << 6)) {
                        bitscnt += 2 + 6;
                    } else {
                        if (val < (1UL << 11)) {
                            bitscnt += 3 + 11;
                        } else {
                            if (val < (1UL << 27)) {
                                bitscnt += 4 + 27;
                            } else {
                                bitscnt += 4 + 36;
                            }
                        }
                    }
                }
            }
        } while (++seqcode < m_as->alphabet_size);
    } else {
        // it's a border node
        bitscnt += pbp->pbp_BranchCode[0].hc_CodeLength;
        //// write leaf bits to separate from SEQCODE_N branch!!
        //bitscnt += 3;
    }
    return ((bitscnt + 7) >> 3);
}

/*!
 * \brief Calculate packed leaf size
 */
inline ULONG PtpanTreeBuilder::pbt_calcPackedLeafSize(
        struct PTPanBuildPartition *pbp, struct SfxBorderNode *sfxnode) {
    /* how many leaves do we have? */
    ULONG leafcnt = sfxnode->sn_NumLeafs;
    /* enlarge memory, if too small */
    if (pbp->pbp_LeafBufferSize < leafcnt) {
        pbp->pbp_LeafBuffer = (LONG *) realloc(pbp->pbp_LeafBuffer,
                leafcnt * sizeof(LONG));
        pbp->pbp_LeafBufferSize = leafcnt;
    }
    LONG *leafBufferPtr = pbp->pbp_LeafBuffer;
    /* collect leafs */
    struct Leaf *current = (struct Leaf *) sfxnode->sn_FirstLeaf;
    for (ULONG i = 0; i < leafcnt; i++) {
        if (!current) {
            printf("An error occured during collecting of leafs!\n");
        }
        *leafBufferPtr++ = current->sn_position;
        current = current->next;
    }

    if (leafcnt > 1) {
        std::sort(pbp->pbp_LeafBuffer, pbp->pbp_LeafBuffer + leafcnt);
    }

    /* do delta compression */
    LONG oldval = pbp->pbp_LeafBuffer[0];
    ULONG cnt;
    LONG val;
    for (cnt = 1; cnt < leafcnt; cnt++) {
        pbp->pbp_LeafBuffer[cnt] -= oldval;
        oldval += pbp->pbp_LeafBuffer[cnt];
        //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
    }

#ifdef DOUBLEDELTALEAVES
#ifdef DOUBLEDELTAOPTIONAL
    for(cnt = 0; cnt < leafcnt; cnt++)
    {
        val = pbp->pbp_LeafBuffer[cnt];
        if((val < -63) || (val > 63))
        {
            if((val < -8192) || (val > 8191))
            {
                if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
                {
                    if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
                    {
                        /* five bytes */
                        sdleafsize += 5;
                    } else {
                        /* four bytes */
                        sdleafsize += 4;
                    }
                } else {
                    /* three bytes */
                    sdleafsize += 3;
                }
            } else {
                /* two bytes */
                sdleafsize += 2;
            }
        } else {
            /* one byte */
            sdleafsize++;
        }
    }
    *pbp->pbp_LeafBuffer = -(*pbp->pbp_LeafBuffer);
    (*pbp->pbp_LeafBuffer)--;
#endif
    /* do delta compression a second time */
    if(leafcnt > 2)
    {
        oldval = pbp->pbp_LeafBuffer[1];
        for(cnt = 2; cnt < leafcnt; cnt++)
        {
            pbp->pbp_LeafBuffer[cnt] -= oldval;
            oldval += pbp->pbp_LeafBuffer[cnt];
            //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
        }
    }

#endif
    /* compression:
     msb == {1}    -> 1 byte offset (-   63 -     63)
     msb == {01}   -> 2 byte offset (- 8192 -   8191)
     msb == {001}  -> 3 byte offset (- 2^20 - 2^20-1)
     msb == {0001} -> 4 byte offset (- 2^28 - 2^28-1)
     msb == {0000} -> 5 byte offset (- 2^35 - 2^35-1)
     special opcodes:
     0xff      -> end of array
     This means the upper limit is currently 1 GB for raw sequence data
     (this can be changed though: Just introduce another opcode)
     introduce other opcode which can handle more data!!
     someone started this with 5 byte offset (see ReadPackedLeaf)
     so currently the limit is 32 GB which is sufficient for now
     */
    ULONG leafsize = 1;

    for (cnt = 0; cnt < leafcnt; cnt++) {
        val = pbp->pbp_LeafBuffer[cnt];
        if ((val < -63) || (val > 63)) {
            if ((val < -8192) || (val > 8191)) {
                if ((val < -(1L << 20)) || (val > ((1L << 20) - 1))) {
                    if ((val < -(1L << 27)) || (val > ((1L << 27) - 1))) {
                        /* five bytes */
                        leafsize += 5;
                    } else {
                        /* four bytes */
                        leafsize += 4;
                    }
                } else {
                    /* three bytes */
                    leafsize += 3;
                }
            } else {
                /* two bytes */
                leafsize += 2;
            }
        } else {
            /* one byte */
            leafsize++;
        }
    }
#ifdef DOUBLEDELTAOPTIONAL
    return((leafsize < sdleafsize) ? leafsize : sdleafsize);
#else
    return (leafsize);
#endif
}

/* !
 * \brief Write packed node
 */
ULONG PtpanTreeBuilder::pbt_writePackedNode(struct PTPanBuildPartition *pbp,
        struct SfxNode *sfxnode, UBYTE *buf) {
    ULONG bpos = 0;
    if (sfxnode->sn_EdgeLen != 0) {
        /* huffman code for short / long edge */
        if (sfxnode->sn_AlphaMask & SHORTEDGEBIT) { // SHORTEDGEBIT = (1U << (RELOFFSETBITS + 1))
            /* short edge */
            bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                    pbp->pbp_ShortEdgeCode[sfxnode->sn_Edge].hc_Codec,
                    pbp->pbp_ShortEdgeCode[sfxnode->sn_Edge].hc_CodeLength);
        } else {
            /* long edge */
            bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                    pbp->pbp_ShortEdgeCode[0].hc_Codec,
                    pbp->pbp_ShortEdgeCode[0].hc_CodeLength);
            bpos =
                    AbstractAlphabetSpecifics::writeBits(
                            buf,
                            bpos,
                            pbp->pbp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_Codec,
                            pbp->pbp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength);
            bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                    sfxnode->sn_StartPos,
                    pbp->pbp_PTPanPartition->pp_LongRelPtrBits);
        }
    } else {
        /* special root handling */
        bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                pbp->pbp_ShortEdgeCode[1].hc_Codec,
                pbp->pbp_ShortEdgeCode[1].hc_CodeLength);
    }
    if (sfxnode->sn_AlphaMask & INNERNODEBIT) {
        // it's an inner node
        ULONG alphamask = sfxnode->sn_AlphaMask & RELOFFSETMASK;
        /* branch code */
        bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                pbp->pbp_BranchCode[alphamask].hc_Codec,
                pbp->pbp_BranchCode[alphamask].hc_CodeLength);
        /* child pointers */
        /* msb = {0}   -> rel is next
         msb = {10}  ->  6 bit rel node ptr
         msb = {111} -> 11 bit rel node ptr
         msb = {1100} -> 27 bit rel node ptr (large enough for a jump over 128 MB)
         msb = {1101} -> 36 bit rel node ptr (large enough for a jump over 64 GB)
         add another opcode to allow longer jumps!
         */
        struct SfxInnerNode * inner = (struct SfxInnerNode *) sfxnode;
        UWORD seqcode = 0;
        ULONG newbase = 0;
        ULONG val;
        do {
            if (alphamask & (1UL << seqcode)) {
                val = inner->sn_Children[seqcode];
                val -= newbase;
                newbase += val;
                //printf("Dist: %6ld\n", val);
                if (val == 0) {
                    bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos, 0x0,
                            1);
                } else {
                    if (val < (1UL << 6)) {
                        bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                                0x2, 2);
                        bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                                val, 6);
                    } else {
                        if (val < (1UL << 11)) {
                            bpos = AbstractAlphabetSpecifics::writeBits(buf,
                                    bpos, 0x7, 3);
                            bpos = AbstractAlphabetSpecifics::writeBits(buf,
                                    bpos, val, 11);
                        } else {
                            if (val < (1UL << 27)) {
                                bpos = AbstractAlphabetSpecifics::writeBits(buf,
                                        bpos, 0xc, 4);
                                bpos = AbstractAlphabetSpecifics::writeBits(buf,
                                        bpos, val, 27);
                            } else {
                                bpos = AbstractAlphabetSpecifics::writeBits(buf,
                                        bpos, 0xd, 4);
                                bpos = AbstractAlphabetSpecifics::writeBits(buf,
                                        bpos, val, 36);
                            }

                        }
                    }
                }
            }
        } while (++seqcode < m_as->alphabet_size);
    } else {
        // it's a border node
        bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos,
                pbp->pbp_BranchCode[0].hc_Codec,
                pbp->pbp_BranchCode[0].hc_CodeLength);
        //// write leaf bits to separate from SEQCODE_N branch!!
        //bpos = AbstractAlphabetSpecifics::writeBits(buf, bpos, 0x7, 3);
    }
    return ((bpos + 7) >> 3);
}

/*!
 * \brief Writee packed leaf
 */
ULONG PtpanTreeBuilder::pbt_writePackedLeaf(struct PTPanBuildPartition *pbp,
        struct SfxBorderNode *sfxnode, UBYTE *buf) {
    /* how many leaves do we have? */
    ULONG leafcnt = sfxnode->sn_NumLeafs;
    /* enlarge memory, if too small */
    if (pbp->pbp_LeafBufferSize < leafcnt) {
        pbp->pbp_LeafBuffer = (LONG *) realloc(pbp->pbp_LeafBuffer,
                leafcnt * sizeof(LONG));
        pbp->pbp_LeafBufferSize = leafcnt;
    }
    LONG *leafBufferPtr = pbp->pbp_LeafBuffer;
    /* collect leafs */
    struct Leaf *current = (struct Leaf *) sfxnode->sn_FirstLeaf;
    for (ULONG i = 0; i < leafcnt; i++) {
        if (!current) {
            printf("An error occured during collecting of leafs!\n");
        }
        *leafBufferPtr++ = current->sn_position;
        current = current->next;
    }

    if (leafcnt > 1) {
        std::sort(pbp->pbp_LeafBuffer, pbp->pbp_LeafBuffer + leafcnt);
    }

    /* do delta compression */
    LONG oldval = pbp->pbp_LeafBuffer[0];
    assert(pbp->pbp_LeafBuffer[0] < (LONG)m_total_raw_size);
    ULONG cnt;
    for (cnt = 1; cnt < leafcnt; cnt++) {
        assert(pbp->pbp_LeafBuffer[cnt] < (LONG)m_total_raw_size);
        pbp->pbp_LeafBuffer[cnt] -= oldval;
        oldval += pbp->pbp_LeafBuffer[cnt];
    }

#ifdef DOUBLEDELTALEAVES
#ifdef DOUBLEDELTAOPTIONAL
    for(cnt = 0; cnt < leafcnt; cnt++)
    {
        val = pbp->pbp_LeafBuffer[cnt];
        if((val < -63) || (val > 63))
        {
            if((val < -8192) || (val > 8191))
            {
                if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
                {
                    if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
                    {
                        /* five bytes */
                        sdleafsize += 5;
                    } else {
                        /* four bytes */
                        sdleafsize += 4;
                    }
                } else {
                    /* three bytes */
                    sdleafsize += 3;
                }
            } else {
                /* two bytes */
                sdleafsize += 2;
            }
        } else {
            /* one byte */
            sdleafsize++;
        }
    }
    *pbp->pbp_LeafBuffer = -(*pbp->pbp_LeafBuffer);
    (*pbp->pbp_LeafBuffer)--;
#endif
    /* do delta compression a second time */
    if(leafcnt > 2)
    {
        oldval = pbp->pbp_LeafBuffer[1];
        for(cnt = 2; cnt < leafcnt; cnt++)
        {
            pbp->pbp_LeafBuffer[cnt] -= oldval;
            oldval += pbp->pbp_LeafBuffer[cnt];
            //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
        }
    }
#endif

    ULONG leafsize = 1;
    /* compression:
     msb == {1}    -> 1 byte offset (-   63 -     63)
     msb == {01}   -> 2 byte offset (- 8192 -   8191)
     msb == {001}  -> 3 byte offset (- 2^20 - 2^20-1)
     msb == {0001} -> 4 byte offset (- 2^27 - 2^27-1)
     msb == {0000} -> 5 byte offset (- 2^35 - 2^35-1)
     special opcodes:
     0xff      -> end of array
     This means the upper limit is currently 512 MB for raw sequence data
     (this can be changed though: Just introduce another opcode)
     introduce other opcode which can handle more data!!
     someone started this with 5 byte offset (see ReadPackedLeaf)
     so currently the limit is 32 GB which is sufficient for now
     */
    LONG val;
    for (cnt = 0; cnt < leafcnt; cnt++) {
        val = pbp->pbp_LeafBuffer[cnt];
        if ((val < -63) || (val > 63)) {
            if ((val < -8192) || (val > 8191)) {
                if ((val < -(1L << 20)) || (val > ((1L << 20) - 1))) {
                    if ((val < -(1L << 27)) || (val > ((1L << 27) - 1))) {
                        /* five bytes */
                        if ((val < -(1L << 35)) || (val > ((1L << 35) - 1))) {
                            printf("ERROR: %ld: %ld out of range\n", cnt, val);
                        }
                        val += 1L << 35;
                        *buf++ = (val >> 32) & 0x0f;
                        *buf++ = (val >> 24);
                        *buf++ = (val >> 16);
                        *buf++ = (val >> 8);
                        *buf++ = val;
                        leafsize += 5;

                    } else {
                        /* four bytes */
                        val += 1L << 27;
                        *buf++ = ((val >> 24) & 0x0f) | 0x10;
                        *buf++ = (val >> 16);
                        *buf++ = (val >> 8);
                        *buf++ = val;
                        leafsize += 4;
                    }
                } else {
                    /* three bytes */
                    val += 1L << 20;
                    *buf++ = ((val >> 16) & 0x1f) | 0x20;
                    *buf++ = (val >> 8);
                    *buf++ = val;
                    leafsize += 3;
                }
            } else {
                /* two bytes */
                val += 8192;
                *buf++ = ((val >> 8) & 0x3f) | 0x40;
                *buf++ = val;
                leafsize += 2;
            }
        } else {
            /* one byte */
            val += 63;
            *buf++ = (val & 0x7f) | 0x80;
            leafsize++;
        }
    }
#ifdef DOUBLEDELTAOPTIONAL
    if(sdleafsize < leafsize)
    {
        /* undo double delta */
        for(cnt = 2; cnt < leafcnt; cnt++)
        {
            pbp->pbp_LeafBuffer[cnt] += pbp->pbp_LeafBuffer[cnt-1];
        }
        buf = origbuf;
        leafsize = sdleafsize;
        for(cnt = 0; cnt < leafcnt; cnt++)
        {
            val = pbp->pbp_LeafBuffer[cnt];
            if((val < -63) || (val > 63))
            {
                if((val < -8192) || (val > 8191))
                {
                    if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
                    {
                        if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
                        {
                            /* five bytes */
                            val += 1L << 35;
                            *buf++ = (val >> 32) & 0x0f;
                            *buf++ = (val >> 24);
                            *buf++ = (val >> 16);
                            *buf++ = (val >> 8);
                            *buf++ = val;
                        } else {
                            /* four bytes */
                            val += 1L << 27;
                            *buf++ = ((val >> 24) & 0x0f) | 0x10;
                            *buf++ = (val >> 16);
                            *buf++ = (val >> 8);
                            *buf++ = val;
                        }
                    } else {
                        /* three bytes */
                        val += 1L << 20;
                        *buf++ = ((val >> 16) & 0x1f) | 0x20;
                        *buf++ = (val >> 8);
                        *buf++ = val;
                    }
                } else {
                    /* two bytes */
                    val += 8192;
                    *buf++ = ((val >> 8) & 0x3f) | 0x40;
                    *buf++ = val;
                }
            } else {
                /* one byte */
                val += 63;
                *buf++ = (val & 0x7f) | 0x80;
            }
        }
    }
#endif
    *buf = 0xff;
    return (leafsize);
}

/*!
 * \brief Try to clean up if build failed
 */
void PtpanTreeBuilder::cleanFail() {
    if (!m_index_name.empty()) {
        if (m_threadpool) {
            m_threadpool->clear();
        }
        if (m_index_fh) {
            fclose(m_index_fh);
            m_index_fh = NULL;
        }
        if (remove(m_index_name.data()) != 0) {
            printf(
                    "PtpanTreeBuilder::cleanFail() could not remove index stuff.");
        }
    }
}

/*!
 * \brief Write bytes to file
 */
inline ULONG PtpanTreeBuilder::write_byte(ULONG value, UBYTE *buffer) {

    UWORD count;
    if (value < (1L << 16)) {
        // 00 16bit
        count = 2;
        *buffer++ = '0';
    } else if (value < (1L << 24)) {
        // 01 24bit
        count = 3;
        *buffer++ = '1';
    } else if (value < (1L << 32)) {
        // 10 32bit
        count = 4;
        *buffer++ = '2';
    } else {
        // 11 64bit
        count = sizeof(ULONG);
        *buffer++ = '3';
    }
    write_byte(value, count, buffer);
    // REMOVEfor (int i = 0; i < count; i++) {
    //*buffer++ = value >> (8 * i);
    //}
    return count + 1;
}

/*!
 * \brief Write bytes to file
 */
inline void PtpanTreeBuilder::write_byte(ULONG value, ULONG count,
        UBYTE *buffer) {
    for (ULONG i = 0; i < count; i++) {
        *buffer++ = value >> (8 * i);
    }
}

/*!
 * \brief Seek bytes in file stream
 */
void PtpanTreeBuilder::seek_byte(FILE *fileheader) {
    char prefix;
    fread(&prefix, 1, 1, fileheader);
    switch (prefix) {
    case '0':
        // 00 16bit
        fseek(fileheader, 2, SEEK_CUR);
        break;
    case '1':
        // 01 24bit
        fseek(fileheader, 3, SEEK_CUR);
        break;
    case '2':
        // 10 32bit
        fseek(fileheader, 4, SEEK_CUR);
        break;
    case '3':
        // 11 64bit
        fseek(fileheader, sizeof(ULONG), SEEK_CUR);
        break;
    default:
        break;
    }
}
