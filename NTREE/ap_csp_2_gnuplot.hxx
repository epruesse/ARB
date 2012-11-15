#define AP_AWAR_CSP                         "tmp/ntree/csp_2_gnuplot"
#define AP_AWAR_CSP_NAME                    AP_AWAR_CSP "/name"
#define AP_AWAR_CSP_ALIGNMENT               AP_AWAR_CSP "/alignment"
#define AP_AWAR_CSP_SUFFIX                  AP_AWAR_CSP "/filter"
#define AP_AWAR_CSP_DIRECTORY               AP_AWAR_CSP "/directory"
#define AP_AWAR_CSP_FILENAME                AP_AWAR_CSP "/file_name"
#define AP_AWAR_CSP_SMOOTH                  AP_AWAR_CSP "/smooth"
#define AP_AWAR_CSP_SMOOTH_GNUPLOT          AP_AWAR_CSP "/smooth_gnuplot"
#define AP_AWAR_CSP_GNUPLOT_OVERLAY_PREFIX  AP_AWAR_CSP "/gnuplot_overlay_prefix"
#define AP_AWAR_CSP_GNUPLOT_OVERLAY_POSTFIX AP_AWAR_CSP "/gnuplot_overlay_postfix"

#define AP_AWAR_FILTER_NAME      AP_AWAR_CSP "/ap_filter/name"
#define AP_AWAR_FILTER_ALIGNMENT AP_AWAR_CSP "/ap_filter/alignment"
#define AP_AWAR_FILTER_FILTER    AP_AWAR_CSP "/ap_filter/filter"

AW_window *AP_open_csp_2_gnuplot_window( AW_root *root );
