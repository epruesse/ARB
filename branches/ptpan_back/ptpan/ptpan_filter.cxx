/*!
 * \file ptpan_filter.cxx
 *
 * \date 14.03.2011
 * \author Tilo Eissler
 */

#include "ptpan_filter.h"

/*!
 * \brief constructor
 */
AbstractQueryFilter::AbstractQueryFilter() {
}

/*!
 * \brief copy constructor
 */
AbstractQueryFilter::AbstractQueryFilter(
        const AbstractQueryFilter& /*filter*/) {
}

/*!
 * \brief assignment operator
 */
AbstractQueryFilter& AbstractQueryFilter::operator=(
        const AbstractQueryFilter& filter) {
    if (this != &filter) {
        // nothing at the moment ...
    }
    return *this;
}

/*!
 * \brief destructor
 */
AbstractQueryFilter::~AbstractQueryFilter() {
}

/*!
 * \brief constructor
 */
AggregateQueryFilter::AggregateQueryFilter() :
        AbstractQueryFilter() {
}

/*!
 * \brief destructor
 */
AggregateQueryFilter::~AggregateQueryFilter() {
}

/*!
 * \brief copy constructor
 */
AggregateQueryFilter::AggregateQueryFilter(const AggregateQueryFilter& filter) :
        AbstractQueryFilter(filter) {
    // TODO FIXME
}

/*!
 * \brief assignment operator
 */
AggregateQueryFilter& AggregateQueryFilter::operator=(
        const AggregateQueryFilter& filter) {
    if (this != &filter) {
        AggregateQueryFilter::operator=(filter);
        // TODO
    }
    return *this;
}

/*!
 * \brief Check if given hit matches filter
 *
 * \param QueryHit *
 *
 * \return bool
 */
bool AggregateQueryFilter::matchesFilter(const QueryHit& hit) const {
    // TODO FIXME
    return false;
}

/*!
 * \brief Remove all hits not matching filter from the list
 *
 * \param struct List *
 *
 * \return ULONG Number of hits removed
 */
ULONG AggregateQueryFilter::removeNotMatching(
        struct List *query_hit_list) const {
    if (query_hit_list) {
        // TODO FIXME
    }
    return 0;
}

/*!
 * \brief constructor
 */
RichBaseRangeQueryFilter::RichBaseRangeQueryFilter() :
        AbstractQueryFilter() {
}

/*!
 * \brief destructor
 */
RichBaseRangeQueryFilter::~RichBaseRangeQueryFilter() {
}

/*!
 * \brief copy constructor
 */
RichBaseRangeQueryFilter::RichBaseRangeQueryFilter(
        const RichBaseRangeQueryFilter& filter) :
        AbstractQueryFilter(filter) {
    // TODO FIXME
}

/*!
 * \brief assignment operator
 */
RichBaseRangeQueryFilter& RichBaseRangeQueryFilter::operator=(
        const RichBaseRangeQueryFilter& filter) {
    if (this != &filter) {
        AbstractQueryFilter::operator=(filter);
        // TODO
    }
    return *this;
}

/*!
 * \brief Check if given hit matches filter
 *
 * \param QueryHit *
 *
 * \return bool
 */
bool RichBaseRangeQueryFilter::matchesFilter(const QueryHit& hit) const {
    // TODO FIXME
    return false;
}

/*!
 * \brief Remove all hits not matching filter from the list
 *
 * \param struct List *
 *
 * \return ULONG Number of hits removed
 */
ULONG RichBaseRangeQueryFilter::removeNotMatching(
        struct List *query_hit_list) const {
    if (query_hit_list) {
        // TODO FIXME
    }
    return 0;
}

/*!
 * \brief constructor
 */
BareBaseRangeQueryFilter::BareBaseRangeQueryFilter() :
        AbstractQueryFilter() {
}

/*!
 * \brief destructor
 */
BareBaseRangeQueryFilter::~BareBaseRangeQueryFilter() {
}

/*!
 * \brief copy constructor
 */
BareBaseRangeQueryFilter::BareBaseRangeQueryFilter(
        const BareBaseRangeQueryFilter& filter) :
        AbstractQueryFilter(filter) {
    // TODO FIXME
}

/*!
 * \brief assignment operator
 */
BareBaseRangeQueryFilter& BareBaseRangeQueryFilter::operator=(
        const BareBaseRangeQueryFilter& filter) {
    if (this != &filter) {
        AbstractQueryFilter::operator=(filter);
        // TODO
    }
    return *this;
}

/*!
 * \brief Check if given hit matches filter
 *
 * \param QueryHit *
 *
 * \return bool
 */
bool BareBaseRangeQueryFilter::matchesFilter(const QueryHit& hit) const {
    // TODO FIXME
    return false;
}

/*!
 * \brief Remove all hits not matching filter from the list
 *
 * \param struct List *
 *
 * \return ULONG Number of hits removed
 */
ULONG BareBaseRangeQueryFilter::removeNotMatching(
        struct List *query_hit_list) const {
    if (query_hit_list) {
        // TODO FIXME
    }
    return 0;
}
