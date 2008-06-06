const int PH_ML_RESOLUTION   = 1000; // max res
const int PH_ML_MAX_DIST     = 10; // max dist
const int PH_ML_MAX_MAT_SIZE = 16;


typedef double ph_ml_matrix[PH_ML_MAX_MAT_SIZE][PH_ML_MAX_MAT_SIZE];
typedef double ph_pml_matrix[PH_ML_MAX_MAT_SIZE][PH_ML_MAX_MAT_SIZE];
typedef char   ph_bool_matrix[PH_ML_MAX_MAT_SIZE][PH_ML_MAX_MAT_SIZE];

class ph_mldist {
    long spp;                   // number of species
    long chars;                 // number of characters
    long n_states;              // << PH_ML_MAX_MAT_SIZE

    /*
     * spp = number of species chars = number of sites in actual sequences
     */

    double    fracchange;
    PHENTRY **entries;          // link to entries
    double    pi[20];

    double       eig[PH_ML_MAX_MAT_SIZE];
    double       exptteig[PH_ML_MAX_MAT_SIZE];
    ph_ml_matrix prob, eigvecs;

    ph_pml_matrix   *(slopes[PH_ML_RESOLUTION*PH_ML_MAX_DIST]);
    // huge cash for many slopes
    ph_pml_matrix   *(curves[PH_ML_RESOLUTION*PH_ML_MAX_DIST]);
    ph_bool_matrix  *(infs[PH_ML_RESOLUTION*PH_ML_MAX_DIST]);

    ph_pml_matrix  *akt_slopes;
    ph_pml_matrix  *akt_curves;
    ph_bool_matrix *akt_infs;
    double          weight[2];  // weight akt slope 1 -> linear interpolation
    AP_smatrix     *matrix;     // link to output matrix

    /* Local variables for makedists, propagated globally for c version: */
    double p, dp, d2p;


    void givens(ph_ml_matrix a,long i,long j,long n,double ctheta,double stheta,GB_BOOL left);
    void coeffs(double x,double y,double *c,double *s,double accuracy);
    void tridiag(ph_ml_matrix a,long n,double accuracy);
    void shiftqr(ph_ml_matrix a, long n, double accuracy);
    void qreigen(ph_ml_matrix prob,long n);

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
    ph_mldist(long nentries, PHENTRY **entries, long seq_len, AP_smatrix *matrixi);
    ~ph_mldist();
    
    const char *makedists();    // calculate the distance matrix
};
