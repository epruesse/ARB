/*!
 * \file ptpan_base.cxx
 *
 * \date 17.08.2011
 * \author Tilo Eissler
 */

#include <sys/mman.h>

#include "ptpan_base.h"

/*!
 * \brief constructor
 */
PtpanBase::PtpanBase() :
        m_pool_mutex(), m_threadpool(
                new boost::threadpool::pool(
                        boost::thread::hardware_concurrency())), m_verbose(
                false) {
}

/*!
 * \brief destructor
 */
PtpanBase::~PtpanBase() {
    try {
        m_threadpool.reset();
    } catch (...) {
        // do nothing
    }
}

/*!
 * \brief Function to free PTPanPartition
 *
 * \param partition PTPanPartition to free
 */
void PtpanBase::free_partition(PTPanPartition* partition) {

    FreeHuffmanTree(partition->pp_BranchTree);
    partition->pp_BranchTree = NULL;
    FreeHuffmanTree(partition->pp_ShortEdgeTree);
    partition->pp_ShortEdgeTree = NULL;
    FreeHuffmanTree(partition->pp_LongEdgeLenTree);
    partition->pp_LongEdgeLenTree = NULL;
    free(partition->pp_LongDictRaw);
    partition->pp_LongDictRaw = NULL;

    if (partition->pp_MapFileBuffer) {
        munmap(partition->pp_MapFileBuffer, partition->pp_MapFileSize);
        partition->pp_MapFileBuffer = NULL;
        partition->pp_DiskTree = NULL;
    } else {
        free(partition->pp_DiskTree);
        partition->pp_DiskTree = NULL;
    }

    if (partition->pp_PartitionName) {
        free(partition->pp_PartitionName);
    }

    free(partition->pp_PrefixSeq);
    free(partition);
    partition = NULL;
}

/*!
 * \brief Function to free all PTPanEntries of linked list
 *
 * \param list pointer to linked list
 */
void PtpanBase::free_all_ptpan_entries(struct List *list) {
    struct PTPanEntry *ps = (struct PTPanEntry *) list->lh_Head;
    while (ps->pe_Node.ln_Succ) {
        Remove(&ps->pe_Node);
        if (ps->pe_Name) {
            free(ps->pe_Name);
        }
        if (ps->pe_FullName) {
            free(ps->pe_FullName);
        }
        if (ps->pe_CompressedShortcuts) {
            free(ps->pe_CompressedShortcuts);
        }
        if (ps->pe_SeqDataCompressed) {
            free(ps->pe_SeqDataCompressed);
        }
        if (ps->pe_FeatureContainer) {
            if (ps->pe_FeatureContainer->normalMode()) {
                delete ps->pe_FeatureContainer;
            } else {
                PTPanFeatureContainer::freeSpecialMode(ps->pe_FeatureContainer);
                ps->pe_FeatureContainer = NULL;
            }
        }
        free(ps);
        ps = NULL;
        ps = (struct PTPanEntry *) list->lh_Head;
    }
}

/*!
 * \brief Free PTPanReferenceEntry structure
 *
 * \param pre Pointer to PTPanReferenceEntry
 */
void PtpanBase::free_ptpan_reference_entry(struct PTPanReferenceEntry* pre) {
    if (pre) {
        if (pre->pre_ReferenceBaseTable) {
            free(pre->pre_ReferenceBaseTable);
        }
        if (pre->pre_ReferenceSeq) {
            free(pre->pre_ReferenceSeq);
        }
        free(pre);
    }
}

/*!
 * \brief Returns the number of (extra) threads used
 *
 * Main process does work as well, so we need one thread less in pool!
 *
 * \return std::size_t
 */
std::size_t PtpanBase::numberOfThreads() const {
    boost::lock_guard<boost::mutex> lock(m_pool_mutex);
    return m_threadpool->size();
}

/*!
 * \brief Set the (maximum) number of threads to use
 *
 * If value is 0 or 1, the number is set to 1 also the main process
 * does work as well, so we won't need a thread in the pool!
 *
 * Default value is obtained by "boost::thread::hardware_concurrency()"
 * allowing the reset to system default value.
 *
 * \param num
 */
void PtpanBase::setNumberOfThreads(std::size_t num =
        boost::thread::hardware_concurrency()) {
    boost::lock_guard<boost::mutex> lock(m_pool_mutex);
    if (num > m_threadpool->size()) {
        m_threadpool->size_controller().resize(num);
    } else {
#ifndef DEBUG
        if (m_verbose) {
#endif
        printf(
                "TODO The utilized threadpool has currently an issue with down-sizing, so it is not done!");
#ifndef DEBUG
    }
#endif
        //        if (num <= 1) {
        //            printf("RESIZE TO 1 (from %ld)\n", m_threadpool->size());
        //            m_threadpool->size_controller().resize(1);
        //        } else {
        //            if (m_threadpool->size() > num) {
        //                m_threadpool->size_controller().resize(num);
        //            }
        //        }
    }
}

/*!
 * \brief Enable or disable verbose mode
 *
 * \param verbose
 */
void PtpanBase::setVerbose(bool verbose) {
    m_verbose = verbose;
}

/*!
 * \brief Returns true if in verbose mode
 *
 * \return bool
 */
bool PtpanBase::isVerbose() const {
    return m_verbose;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_nothing() {
    // TEST_ASSERT(0);
    MISSING_TEST(none);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

