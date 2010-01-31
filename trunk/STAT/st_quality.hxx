// =============================================================== //
//                                                                 //
//   File      : st_quality.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ST_QUALITY_HXX
#define ST_QUALITY_HXX

#ifndef ST_WINDOW_HXX
#include "st_window.hxx"
#endif


class st_cq_stat {
    double *likelihoods;
    double *square_liks;
    int    *n_elems;
public:
    int     size;

    st_cq_stat(int size);
    ~st_cq_stat();
    
    void  add(int pos, double likelihood);
    char *generate_string();
    int   is_bad();                                 // 0 == ok, 1 strange, 2 bad, 3 very bad
};

class st_cq_info {
public:
    st_cq_stat ss2;                                 // begin end statistic
    st_cq_stat ss5;                                 // 5 pieces
    st_cq_stat ssu;                                 // user defined bucket size
    st_cq_stat sscon;                               // conserved / variable positions

public:
    st_cq_info(int seq_len, int bucket_size);

    ~st_cq_info();
};

GB_ERROR st_ml_check_sequence_quality(GBDATA *gb_main, const char *tree_name,
                                      const char *alignment_name, AWT_csp *awt_csp, int bucket_size,
                                      int marked_only, st_report_enum report, const char *dest_field);


#else
#error st_quality.hxx included twice
#endif // ST_QUALITY_HXX
