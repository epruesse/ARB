/*!
 * \file ptpan_base.h
 *
 * \date 17.08.2011
 * \author Tilo Eissler
 */

#ifndef PTPAN_BASE_H_
#define PTPAN_BASE_H_

#include <boost/threadpool.hpp>
#include <boost/smart_ptr.hpp>

#include "ptpan.h"

class PtpanBase {
protected:
    PtpanBase();
    virtual ~PtpanBase() = 0;

public:
    virtual std::size_t numberOfThreads() const;
    virtual void setNumberOfThreads(std::size_t num);

    virtual void setVerbose(bool verbose);
    virtual bool isVerbose() const;

protected:
    virtual void free_partition(PTPanPartition* partition);
    virtual void free_all_ptpan_entries(struct List *list);
    virtual void free_ptpan_reference_entry(struct PTPanReferenceEntry* pre);

    mutable boost::mutex m_pool_mutex;
    typedef boost::shared_ptr<boost::threadpool::pool> pool_ptr;
    pool_ptr m_threadpool;

    bool m_verbose;

private:
    // anything??

};

#endif /* PTPAN_BASE_H_ */
