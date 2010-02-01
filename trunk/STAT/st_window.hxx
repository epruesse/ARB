// =========================================================== //
//                                                             //
//   File      : st_window.hxx                                 //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ST_WINDOW_HXX
#define ST_WINDOW_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

#define ST_ML_AWAR "tmp/st_ml/"
#define ST_ML_AWAR_COLUMN_STAT          ST_ML_AWAR "name"
#define ST_ML_AWAR_ALIGNMENT            ST_ML_AWAR "alignment"
#define ST_ML_AWAR_CQ_BUCKET_SIZE       ST_ML_AWAR "bucket_size"

#define ST_ML_AWAR_CQ_MARKED_ONLY       ST_ML_AWAR "marked_only"
#define ST_ML_AWAR_CQ_DEST_FIELD        ST_ML_AWAR "dest_field"
#define ST_ML_AWAR_CQ_REPORT            ST_ML_AWAR "report"

class ST_ML;
class AP_tree;
class ColumnStat;

AW_window *STAT_create_main_window(AW_root *aw_root, ST_ML *st_ml, AW_CB0 refresh_func, AW_window *refreshed_win);
ST_ML     *STAT_create_ST_ML(GBDATA *gb_main);

typedef unsigned char ST_ML_Color;
enum st_report_enum {
    ST_QUALITY_REPORT_NONE,
    ST_QUALITY_REPORT_TEMP,
    ST_QUALITY_REPORT_YES
};

AP_tree     *STAT_find_node_by_name(ST_ML *st_ml, const char *species_name);
ST_ML_Color *STAT_get_color_string(ST_ML *st_ml, char *species_name, AP_tree *node, int start_ali_pos, int end_ali_pos);
int          STAT_update_ml_likelihood(ST_ML *st_ml, char *result[4], int *latest_update, char *species_name, AP_tree *node);
AW_window   *STAT_create_quality_check_window(AW_root *aw_root, GBDATA *gb_main);


#else
#error st_window.hxx included twice
#endif // ST_WINDOW_HXX
