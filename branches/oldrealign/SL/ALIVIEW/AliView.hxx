// =============================================================== //
//                                                                 //
//   File      : AliView.hxx                                       //
//   Purpose   : Defines a filtered view onto an alignment         //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALIVIEW_HXX
#define ALIVIEW_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

class AP_filter;
class AP_weights;

class AliView {
    GBDATA     *gb_main;
    AP_filter  *filter;
    AP_weights *weights;
    char       *aliname;

    void create_real_view(const AP_filter *filter_, const AP_weights *weights_, const char *aliname_);

public:
    explicit AliView(GBDATA *gb_main_); // not really a view (used for AP_tree w/o sequence)
    AliView(GBDATA *gb_main_, const AP_filter& filter_, const AP_weights& weights_, const char *aliname_);
    AliView(const AliView& other);
    ~AliView();
    DECLARE_ASSIGNMENT_OPERATOR(AliView);

    const char *get_aliname() const { return aliname; }

    const AP_filter *get_filter() const { return filter; }
    AP_filter *get_filter() { return filter; }

    const AP_weights *get_weights() const { return weights; }
    GBDATA *get_gb_main() const { return gb_main; }

    size_t get_length() const;                      // length of (filtered) alignment seen through this view

    bool has_data() const { return aliname != NULL; }
};


#else
#error AliView.hxx included twice
#endif // ALIVIEW_HXX
