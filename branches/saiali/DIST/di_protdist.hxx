// =============================================================== //
//                                                                 //
//   File      : di_protdist.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_PROTDIST_HXX
#define DI_PROTDIST_HXX

#ifndef AP_SEQ_SIMPLE_PRO_HXX
#include <AP_seq_simple_pro.hxx>
#endif

const int di_max_aa     = stop; // must be 20
const int di_max_paa    = unk;  // includes virtual aa
const int di_resolution = 1000; // max res
const int di_max_dist   = 10;   // max dist

// stop is first non real AA == 20

enum di_codetype {
    universal, ciliate, mito, vertmito, flymito, yeastmito
};
enum di_cattype {
    none, similarity, kimura, pam, chemical, hall, george
};


typedef double di_aa_matrix[di_max_aa][di_max_aa];
typedef double di_paa_matrix[di_max_paa][di_max_paa];
typedef char   di_bool_matrix[di_max_paa][di_max_paa];

class DI_ENTRY;
class AP_smatrix;

class di_protdist : virtual Noncopyable {
    static double pameigs[20];
    static double pamprobs[20][20];
    di_codetype   whichcode;
    di_cattype    whichcat;

    long spp;                   // number of species
    long chars;                 // number of characters

    // spp = number of species chars = number of sites in actual sequences

    double         freqa, freqc, freqg, freqt, ttratio, xi, xv, ease, fracchange;
    DI_ENTRY      **entries;                                                        // link to entries
    aas            trans[4][4][4];
    double         pi[20];
    long           cat[di_max_aa];
    double         eig[di_max_aa];
    double         exptteig[di_max_aa];
    di_aa_matrix   prob, eigvecs;

    di_paa_matrix   *(slopes[di_resolution*di_max_dist]);
    // huge cash for many slopes
    di_paa_matrix   *(curves[di_resolution*di_max_dist]);
    di_bool_matrix  *(infs[di_resolution*di_max_dist]);

    di_paa_matrix  *akt_slopes;
    di_paa_matrix  *akt_curves;
    di_bool_matrix *akt_infs;
    double          weight[2];  // weight akt slope 1 -> linear interpolation
    AP_smatrix     *matrix;     // link to output matrix

    // Local variables for makedists, propagated globally for c version:
    double p, dp, d2p;


    void cats(di_cattype wcat);
    void maketrans();
    void code();
    void transition();
    void givens(di_aa_matrix a, long i, long j, long n, double ctheta, double stheta, bool left);
    void coeffs(double x, double y, double *c, double *s, double accuracy);
    void tridiag(di_aa_matrix a, long n, double accuracy);
    void shiftqr(di_aa_matrix a, long n, double accuracy);
    void qreigen(di_aa_matrix prob, long n);
    void pameigen();

    void predict(double tt, long nb1, long  nb2);
    int tt_2_pos(double tt);        // double to cash index
    double pos_2_tt(int pos);       // cash index to pos
    void build_exptteig(double tt);
    void build_predikt_table(int pos);      // build akt_slopes akt_curves
    void build_akt_predikt(double tt);      // build akt_slopes akt_curves

    double predict_slope(int b1, int b2) { return akt_slopes[0][b1][b2]; }
    double predict_curve(int b1, int b2) { return akt_curves[0][b1][b2]; }
    char predict_infinity(int b1, int b2) { return akt_infs[0][b1][b2]; }

    void clean_slopes();

public:
    di_protdist(di_codetype codei, di_cattype cati, long nentries, DI_ENTRY  **entries, long seq_len, AP_smatrix *matrixi);
    ~di_protdist();

    GB_ERROR makedists(bool *aborted_flag);    // calculate the distance matrix
};

#else
#error di_protdist.hxx included twice
#endif // DI_PROTDIST_HXX
