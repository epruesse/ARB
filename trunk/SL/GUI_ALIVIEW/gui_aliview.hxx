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

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

struct adfiltercbstruct;
class  ColumnStat;
class  AW_root;
class  AW_awar;
class  AP_filter;
class  AP_weights;
class  AliView;

class WeightedFilter {
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
    AP_weights *create_weights(const AP_filter *filter) const;
    AliView *create_aliview(const char *aliname) const;
};


#else
#error gui_aliview.hxx included twice
#endif // GUI_ALIVIEW_HXX
