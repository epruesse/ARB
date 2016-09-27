// =============================================================== //
//                                                                 //
//   File      : AliView.cxx                                       //
//   Purpose   : Defines a filtered view onto an alignment         //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AliView.hxx"

#include <AP_filter.hxx>

#define av_assert(cond) arb_assert(cond)

using namespace std;

void AliView::create_real_view(const AP_filter *filter_, const AP_weights *weights_, const char *aliname_) {
    av_assert(!filter && !weights && !aliname);
    av_assert(filter_ && weights_ && aliname_);

    filter  = new AP_filter(*filter_);
    weights = new AP_weights(*weights_);
    aliname = strdup(aliname_);

    av_assert(filter->get_filtered_length() == weights->length());
    av_assert(filter->was_checked_for_validity());
}


AliView::AliView(GBDATA *gb_main_)
    : gb_main(gb_main_)
    , filter(NULL),  weights(NULL),  aliname(NULL)
{
    // this creates a fake-AliView (used for AP_tree w/o sequence)
}

AliView::AliView(GBDATA *gb_main_, const AP_filter& filter_, const AP_weights& weights_, const char *aliname_)
    : gb_main(gb_main_)
    , filter(NULL),  weights(NULL),  aliname(NULL)
{
    create_real_view(&filter_, &weights_, aliname_);
}

AliView::AliView(const AliView& other)
    : gb_main(other.gb_main)
    , filter(NULL),  weights(NULL),  aliname(NULL)
{
    create_real_view(other.filter, other.weights, other.aliname);
}

AliView::~AliView() {
    delete filter;
    delete weights;
    free(aliname);
}

size_t AliView::get_length() const {
    return filter->get_filtered_length();
}

