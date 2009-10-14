// =============================================================== //
//                                                                 //
//   File      : di_mldist.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_MLDIST_HXX
#define DI_MLDIST_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif


const int DI_ML_RESOLUTION   = 1000; // max res
const int DI_ML_MAX_DIST     = 10; // max dist
const int DI_ML_MAX_MAT_SIZE = 16;


typedef double di_ml_matrix[DI_ML_MAX_MAT_SIZE][DI_ML_MAX_MAT_SIZE];
typedef double di_pml_matrix[DI_ML_MAX_MAT_SIZE][DI_ML_MAX_MAT_SIZE];
typedef char   di_bool_matrix[DI_ML_MAX_MAT_SIZE][DI_ML_MAX_MAT_SIZE];

class DI_ENTRY;
class AP_smatrix;

class di_mldist {
    long spp;                   // number of species
    long chars;                 // number of characters
    long n_states;              // << DI_ML_MAX_MAT_SIZE

    /*
     * spp = number of species chars = number of sites in actual sequences
     */

    double    fracchange;
    DI_ENTRY **entries;          // link to entries
    double    pi[20];

    double       eig[DI_ML_MAX_MAT_SIZE];
    double       exptteig[DI_ML_MAX_MAT_SIZE];
    di_ml_matrix prob, eigvecs;

    di_pml_matrix   *(slopes[DI_ML_RESOLUTION*DI_ML_MAX_DIST]);
    // huge cash for many slopes
    di_pml_matrix   *(curves[DI_ML_RESOLUTION*DI_ML_MAX_DIST]);
    di_bool_matrix  *(infs[DI_ML_RESOLUTION*DI_ML_MAX_DIST]);

    di_pml_matrix  *akt_slopes;
    di_pml_matrix  *akt_curves;
    di_bool_matrix *akt_infs;
    double          weight[2];  // weight akt slope 1 -> linear interpolation
    AP_smatrix     *matrix;     // link to output matrix

    /* Local variables for makedists, propagated globally for c version: */
    double p, dp, d2p;


    void givens(di_ml_matrix a,long i,long j,long n,double ctheta,double stheta,GB_BOOL left);
    void coeffs(double x,double y,double *c,double *s,double accuracy);
    void tridiag(di_ml_matrix a,long n,double accuracy);
    void shiftqr(di_ml_matrix a, long n, double accuracy);
    void qreigen(di_ml_matrix prob,long n);

    void predict(double tt,long nb1,long  nb2);
    int tt_2_pos(double tt);        // double to cash index
    double pos_2_tt(int pos);       // cash index to pos
    void build_exptteig(double tt);
    void build_predikt_table(int pos);      // build akt_slopes akt_curves
    void build_akt_predikt(double tt);      // build akt_slopes akt_curves

    double predict_slope(int b1,int b2) { return akt_slopes[0][b1][b2]; }
    double predict_curve(int b1,int b2) { return akt_curves[0][b1][b2]; }
    char predict_infinity(int b1,int b2) { return akt_infs[0][b1][b2]; }
    void clean_slopes();

public:
    di_mldist(long nentries, DI_ENTRY **entries, long seq_len, AP_smatrix *matrixi);
    ~di_mldist();
    
    const char *makedists();    // calculate the distance matrix
};

#else
#error di_mldist.hxx included twice
#endif // DI_MLDIST_HXX
