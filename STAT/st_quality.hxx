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

#else
#error st_quality.hxx included twice
#endif // ST_QUALITY_HXX
