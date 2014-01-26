// ================================================================ //
//                                                                  //
//   File      : GUI_aliview.cxx                                    //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2009   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "gui_aliview.hxx"
#include "awt_filter.hxx"
#include "ColumnStat.hxx"

#include <AliView.hxx>
#include <AP_filter.hxx>

WeightedFilter::WeightedFilter(GBDATA *gb_main, AW_root *aw_root, const char *awar_filter_name, const char *awar_columnStat_name, AW_awar *awar_default_alignment) {
#if defined(WARN_TODO)
#warning let awt_create_select_filter create filter awars and automatically map it to awar_default_alignment \
    // (see [6476] for changes done for ColumnStat)
#endif
    adfilter    = awt_create_select_filter(aw_root, gb_main, awar_filter_name);
    column_stat = awar_columnStat_name ? new ColumnStat(gb_main, aw_root, awar_columnStat_name, awar_default_alignment) : NULL;
}

WeightedFilter::~WeightedFilter() {
    delete column_stat;
    // @@@ leak: adfilter (no destruction implemented)
}

AP_filter *WeightedFilter::create_filter() const {
    return awt_get_filter(adfilter);
}

AP_weights *WeightedFilter::create_weights(const AP_filter *filter) const {
    AP_weights *weights   = NULL;
    bool        haveRates = false;
    if (column_stat) {
        column_stat->calculate(0);
        haveRates = column_stat->has_rates();
    }
    if (haveRates) {
        column_stat->weight_by_inverseRates();
        weights = new AP_weights(column_stat->get_weights(), column_stat->get_length(), filter);
    }
    else {
        weights = new AP_weights(filter);
    }
    if (column_stat) column_stat->forget();

    return weights;
}

AliView *WeightedFilter::create_aliview(const char *aliname) const {
    AP_filter  *filter  = create_filter();
    AP_weights *weights = create_weights(filter);
    AliView    *aliview = new AliView(adfilter->gb_main, *filter, *weights, aliname);

    delete weights;
    delete filter;

    return aliview;
}

GBDATA *WeightedFilter::get_gb_main() const {
    return adfilter->gb_main;
}

AW_root *WeightedFilter::get_aw_root() const {
    return adfilter->awr;
}

