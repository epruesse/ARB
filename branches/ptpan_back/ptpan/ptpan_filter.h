/*!
 * \file ptpan_filter.h
 *
 * \date 14.03.2011
 * \author Tilo Eissler
 */

#ifndef PTPAN_FILTER_H_
#define PTPAN_FILTER_H_

#include "ptpan.h"

/*!
 * \brief Class ...
 *
 * TODO
 */
class AbstractQueryFilter {
protected:
    AbstractQueryFilter();
    AbstractQueryFilter(const AbstractQueryFilter& filter);
    AbstractQueryFilter& operator=(const AbstractQueryFilter& filter);

public:
    virtual ~AbstractQueryFilter() = 0;

    virtual bool matchesFilter(const QueryHit& hit) const = 0;
    virtual ULONG removeNotMatching(struct List *query_hit_list) const = 0;
};

/*!
 * \brief Class ...
 *
 * TODO
 *
 * Each QueryHit must match all filter aggregated in this filter
 */
class AggregateQueryFilter: public AbstractQueryFilter {
public:
    AggregateQueryFilter();
    virtual ~AggregateQueryFilter();
    AggregateQueryFilter(const AggregateQueryFilter& filter);
    AggregateQueryFilter& operator=(const AggregateQueryFilter& filter);

    virtual bool matchesFilter(const QueryHit& hit) const;
    virtual ULONG removeNotMatching(struct List *query_hit_list) const;

    // TODO addFilter()
    // TODO removeFilter()
    // TODO countFilter()
};

/*!
 * \brief Class ...
 *
 * TODO
 */
class RichBaseRangeQueryFilter: public AbstractQueryFilter {
private:
    RichBaseRangeQueryFilter();

public:
    virtual ~RichBaseRangeQueryFilter();
    RichBaseRangeQueryFilter(const RichBaseRangeQueryFilter& filter);
    RichBaseRangeQueryFilter& operator=(const RichBaseRangeQueryFilter& filter);

    virtual bool matchesFilter(const QueryHit& hit) const;
    virtual ULONG removeNotMatching(struct List *query_hit_list) const;

    // TODO
};

/*!
 * \brief Class ...
 *
 * TODO
 */
class BareBaseRangeQueryFilter: public AbstractQueryFilter {
private:
    BareBaseRangeQueryFilter();

public:
    virtual ~BareBaseRangeQueryFilter();
    BareBaseRangeQueryFilter(const BareBaseRangeQueryFilter& filter);
    BareBaseRangeQueryFilter& operator=(const BareBaseRangeQueryFilter& filter);

    virtual bool matchesFilter(const QueryHit& hit) const;
    virtual ULONG removeNotMatching(struct List *query_hit_list) const;

    // TODO
};

#endif /* PTPAN_FILTER_H_ */
