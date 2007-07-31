#ifndef st_window_hxx_included
#define st_window_hxx_included

#define ST_ML_AWAR "tmp/st_ml/"
#define ST_ML_AWAR_CSP			ST_ML_AWAR "name"
#define ST_ML_AWAR_ALIGNMENT		ST_ML_AWAR "alignment"
#define ST_ML_AWAR_CQ_BUCKET_SIZE	ST_ML_AWAR "bucket_size"

#define ST_ML_AWAR_CQ_FILTER_NAME	ST_ML_AWAR "filter/name"
#define ST_ML_AWAR_CQ_FILTER_ALIGNMENT	ST_ML_AWAR "filter/alignment"
#define ST_ML_AWAR_CQ_FILTER_FILTER	ST_ML_AWAR "filter/filter"
#define ST_ML_AWAR_CQ_MARKED_ONLY	ST_ML_AWAR "marked_only"
#define ST_ML_AWAR_CQ_DEST_FIELD	ST_ML_AWAR "dest_field"
#define ST_ML_AWAR_CQ_REPORT		ST_ML_AWAR "report"

class ST_ML;
class AP_tree;
class AWT_csp;

AW_window *st_create_main_window(AW_root * aw_root, ST_ML * st_ml,
                                 AW_CB0 refresh_func, AW_window * win);
ST_ML *new_ST_ML(GBDATA * gb_main);
int st_is_inited(ST_ML * st_ml);

typedef unsigned char ST_ML_Color;
enum st_report_enum {
    ST_QUALITY_REPORT_NONE,
    ST_QUALITY_REPORT_TEMP,
    ST_QUALITY_REPORT_YES
};


AP_tree *st_ml_convert_species_name_to_node(ST_ML * st_ml,
                                            const char *species_name);

ST_ML_Color *st_ml_get_color_string(ST_ML * st_ml, char *species_name,
                                    AP_tree * node, int start_ali_pos,
                                    int end_ali_pos);
int st_ml_update_ml_likelihood(ST_ML * st_ml, char *result[4],
                               int *latest_update, char *species_name,
                               AP_tree * node);

AW_window *st_create_quality_check_window(AW_root * aw_root,
                                          GBDATA * gb_main);

GB_ERROR st_ml_check_sequence_quality(GBDATA * gb_main,
                                      const char *tree_name,
                                      const char *alignment_name,
                                      AWT_csp * awt_csp,
                                      int bucket_size, int marked_only,
                                      st_report_enum report,
                                      const char *filter_string,
                                      const char *dest_field);


#endif
