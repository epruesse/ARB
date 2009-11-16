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
#include "awt_csp.hxx"

#include <AliView.hxx>
#include <AP_filter.hxx>

WeightedFilter::WeightedFilter(GBDATA *gb_main, AW_root *aw_root, const char *awar_filter_name, const char *awar_columnStat_name) {
    adfilter = awt_create_select_filter(aw_root, gb_main, awar_filter_name);
    csp      = awar_columnStat_name ? new AWT_csp(gb_main, aw_root, awar_columnStat_name) : NULL;
}

WeightedFilter::~WeightedFilter() {
    delete csp;
    // @@@ leak: adfilter (no destruction implemented)
}

AP_filter *WeightedFilter::create_filter() const {
    return awt_get_filter(adfilter->awr, adfilter);
}

AP_weights *WeightedFilter::create_weights(const AP_filter *filter) const {
    AP_weights *weights   = NULL;
    bool        haveRates = false;
    if (csp) {
        csp->go(0);
        haveRates = csp->rates != NULL;
    }
    if (haveRates) {
        long ali_len = filter->get_length();

        for (long i=0; i<ali_len; i++) {
            if (csp->rates[i]>0.0000001) {
                csp->weights[i] *= (int)(2.0/ csp->rates[i]);
            }
        }

        weights = new AP_weights(csp->weights, csp->seq_len, filter);
    }
    else {
        weights = new AP_weights(filter);
    }
    if (csp) csp->exit();
    
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

