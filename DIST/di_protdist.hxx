
const int ph_max_aa     = stop; // mussed be 20
const int ph_max_paa    = unk;  // includes virtual aa
const int ph_resolution = 1000; // max res
const int ph_max_dist   = 10;   // max dist

// stop is first non real AA == 20

typedef enum {
    universal, ciliate, mito, vertmito, flymito, yeastmito
} ph_codetype;
typedef enum {
    none,similarity,kimura,pam,chemical, hall, george
} ph_cattype;


typedef double ph_aa_matrix[ph_max_aa][ph_max_aa];
typedef double ph_paa_matrix[ph_max_paa][ph_max_paa];
typedef char   ph_bool_matrix[ph_max_paa][ph_max_paa];

class ph_protdist {
    static double pameigs[20];
    static double pamprobs[20][20];
    ph_codetype   whichcode;
    ph_cattype    whichcat;

    long spp;                   // number of species
    long chars;                 // number of characters

    /*
     * spp = number of species chars = number of sites in actual sequences
     */

    double         freqa, freqc, freqg, freqt, ttratio, xi, xv, ease, fracchange;
    PHENTRY      **entries;                                                        // link to entries
    aas            trans[4][4][4];
    double         pi[20];
    long           cat[ph_max_aa];
    double         eig[ph_max_aa];
    double         exptteig[ph_max_aa];
    ph_aa_matrix   prob, eigvecs;

    ph_paa_matrix   *(slopes[ph_resolution*ph_max_dist]);
    // huge cash for many slopes
    ph_paa_matrix   *(curves[ph_resolution*ph_max_dist]);
    ph_bool_matrix  *(infs[ph_resolution*ph_max_dist]);

    ph_paa_matrix  *akt_slopes;
    ph_paa_matrix  *akt_curves;
    ph_bool_matrix *akt_infs;
    double          weight[2];  // weight akt slope 1 -> linear interpolation
    AP_smatrix     *matrix;     // link to output matrix

    /* Local variables for makedists, propagated globally for c version: */
    double p, dp, d2p;


    void cats(ph_cattype wcat);
    void maketrans();
    void code();
    void transition();
    void givens(ph_aa_matrix a,long i,long j,long n,double ctheta,double stheta,GB_BOOL left);
    void coeffs(double x,double y,double *c,double *s,double accuracy);
    void tridiag(ph_aa_matrix a,long n,double accuracy);
    void shiftqr(ph_aa_matrix a, long n, double accuracy);
    void qreigen(ph_aa_matrix prob,long n);
    void pameigen();

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
    ph_protdist(ph_codetype codei, ph_cattype cati, long nentries, PHENTRY  **entries, long seq_len, AP_smatrix *matrixi);
    ~ph_protdist();
    
    const char *makedists();    // calculate the distance matrix
};
