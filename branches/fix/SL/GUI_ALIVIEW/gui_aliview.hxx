// ================================================================ //
//                                                                  //
//   File      : gui_aliview.hxx                                    //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2009   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef GUI_ALIVIEW_HXX
#define GUI_ALIVIEW_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

struct adfiltercbstruct;
class  ColumnStat;
class  AW_root;
class  AW_awar;
class  AP_filter;
class  AP_weights;
class  AliView;

class WeightedFilter : virtual Noncopyable {
    adfiltercbstruct *adfilter;
    ColumnStat       *column_stat;

public:
    WeightedFilter(GBDATA *gb_main, AW_root *aw_root, const char *awar_filter_name, const char *awar_columnStat_name, AW_awar *awar_default_alignment);
    ~WeightedFilter();

    GBDATA *get_gb_main() const;
    AW_root *get_aw_root() const;

    adfiltercbstruct *get_adfiltercbstruct() { return adfilter; }
    ColumnStat *get_column_stat() { return column_stat; }

    // factory functions
    AP_filter *create_filter() const;
    AP_weights *create_weights(const AP_filter *filter, GB_ERROR& error) const;
    AliView *create_aliview(const char *aliname, GB_ERROR& error) const;
};


#else
#error gui_aliview.hxx included twice
#endif // GUI_ALIVIEW_HXX
