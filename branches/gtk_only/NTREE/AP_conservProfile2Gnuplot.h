// ============================================================ //
//                                                              //
//   File      : AP_conservProfile2Gnuplot.h                    //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AP_CONSERVPROFILE2GNUPLOT_H
#define AP_CONSERVPROFILE2GNUPLOT_H

#define AP_AWAR_CONSPRO                  "tmp/ntree/conservProfile2Gnuplot"
#define AP_AWAR_CONSPRO_FILENAME         AP_AWAR_CONSPRO "/file_name"
#define AP_AWAR_CONSPRO_SMOOTH_GNUPLOT   AP_AWAR_CONSPRO "/smooth_gnuplot"
#define AP_AWAR_CONSPRO_GNUPLOT_DISP_POS AP_AWAR_CONSPRO "/gnuplot_display_positions"
#define AP_AWAR_CONSPRO_GNUPLOT_LEGEND   AP_AWAR_CONSPRO "/gnuplot_legend"
#define AP_AWAR_CONSPRO_GNUPLOT_MIN_X    AP_AWAR_CONSPRO "/gnuplot_xRange_min"
#define AP_AWAR_CONSPRO_GNUPLOT_MAX_X    AP_AWAR_CONSPRO "/gnuplot_xRange_max"
#define AP_AWAR_CONSPRO_GNUPLOT_MIN_Y    AP_AWAR_CONSPRO "/gnuplot_yRange_min"
#define AP_AWAR_CONSPRO_GNUPLOT_MAX_Y    AP_AWAR_CONSPRO "/gnuplot_yRange_max"
#define AP_AWAR_BASE_FREQ_FILTER_NAME    AP_AWAR_CONSPRO "/ap_filter/name"


#else
#error AP_conservProfile2Gnuplot.h included twice
#endif // AP_CONSERVPROFILE2GNUPLOT_H
