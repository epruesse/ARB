// =========================================================== //
//                                                             //
//   File      : ed4_colStat.hxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ED4_COLSTAT_HXX
#define ED4_COLSTAT_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

void ED4_activate_col_stat(AW_window *aww, AW_CL, AW_CL);
void ED4_disable_col_stat(AW_window *, AW_CL, AW_CL);
void ED4_toggle_detailed_column_stats(AW_window *aww, AW_CL, AW_CL);
void ED4_set_col_stat_threshold(AW_window *aww, AW_CL, AW_CL);

#else
#error ed4_colStat.hxx included twice
#endif // ED4_COLSTAT_HXX
