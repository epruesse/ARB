// =============================================================== //
//                                                                 //
//   File      : DI_protdist.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_protdist.hxx"
#include "di_matr.hxx"
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <cmath>

#define epsilon 0.000001        // a small number

double di_protdist::pameigs[20] = {
    -0.022091252, -0.019297602, 0.000004760, -0.017477817,
    -0.016575549, -0.015504543, -0.002112213, -0.002685727,
    -0.002976402, -0.013440755, -0.012926992, -0.004293227,
    -0.005356688, -0.011064786, -0.010480731, -0.008760449,
    -0.007142318, -0.007381851, -0.007806557, -0.008127024
};

double di_protdist::pamprobs[20][20] = {
    {
        -0.01522976, -0.00746819, -0.13934468, 0.11755315, -0.00212101,
        0.01558456, -0.07408235, -0.00322387, 0.01375826, 0.00448826,
        0.00154174, 0.02013313, -0.00159183, -0.00069275, -0.00399898,
        0.08414055, -0.01188178, -0.00029870, 0.00220371, 0.00042546
    },
    {
        -0.07765582, -0.00712634, -0.03683209, -0.08065755, -0.00462872,
        -0.03791039, 0.10642147, -0.00912185, 0.01436308, -0.00133243,
        0.00166346, 0.00624657, -0.00003363, -0.00128729, -0.00690319,
        0.17442028, -0.05373336, -0.00078751, -0.00038151, 0.01382718
    },
    {
        -0.08810973, -0.04081786, -0.04066004, -0.04736004, -0.03275406,
        -0.03761164, -0.05047487, -0.09086213, -0.03269598, -0.03558015,
        -0.08407966, -0.07970977, -0.01504743, -0.04011920, -0.05182232,
        -0.07026991, -0.05846931, -0.01016998, -0.03047472, -0.06280511
    },
    {
        0.02513756, -0.00578333, 0.09865453, 0.01322314, -0.00310665,
        0.05880899, -0.09252443, -0.02986539, -0.03127460, 0.01007539,
        -0.00360119, -0.01995024, 0.00094940, -0.00145868, -0.01388816,
        0.11358341, -0.12127513, -0.00054696, -0.00055627, 0.00417284
    },
    {
        0.16517316, -0.00254742, -0.03318745, -0.01984173, 0.00031890,
        -0.02817810, 0.02661678, -0.01761215, 0.01665112, 0.10513343,
        -0.00545026, 0.01827470, -0.00207616, -0.00763758, -0.01322808,
        -0.02202576, -0.07434204, 0.00020593, 0.00119979, -0.10827873
    },
    {
        0.16088826, 0.00056313, -0.02579303, -0.00319655, 0.00037228,
        -0.03193150, 0.01655305, -0.03028640, 0.01367746, -0.11248153,
        0.00778371, 0.02675579, 0.00243718, 0.00895470, -0.01729803,
        -0.02686964, -0.08262584, 0.00011794, -0.00225134, 0.09415650
    },
    {
        -0.01739295, 0.00572017, -0.00712592, -0.01100922, -0.00870113,
        -0.00663461, -0.01153857, -0.02248432, -0.00382264, -0.00358612,
        -0.00139345, -0.00971460, -0.00133312, 0.01927783, -0.01053838,
        -0.00911362, -0.01010908, 0.09417598, 0.01763850, -0.00955454
    },
    {
        0.01728888, 0.01344211, 0.01200836, 0.01857259, -0.17088517,
        0.01457592, 0.01997839, 0.02844884, 0.00839403, 0.00196862,
        0.01391984, 0.03270465, 0.00347173, -0.01940984, 0.01233979,
        0.00542887, 0.01008836, 0.00126491, -0.02863042, 0.00449764
    },
    {
        -0.02881366, -0.02184155, -0.01566086, -0.02593764, -0.04050907,
        -0.01539603, -0.02576729, -0.05089606, -0.00597430, 0.02181643,
        0.09835597, -0.04040940, 0.00873512, 0.12139434, -0.02427882,
        -0.02945238, -0.01566867, -0.01606503, 0.09475319, 0.02238670
    },
    {
        0.04080274, -0.02869626, -0.05191093, -0.08435843, 0.00021141,
        0.13043842, 0.00871530, 0.00496058, -0.02797641, -0.00636933,
        0.02243277, 0.03640362, -0.05735517, 0.00196918, -0.02218934,
        -0.00608972, 0.02872922, 0.00047619, 0.00151285, 0.00883489
    },
    {
        -0.02623824, 0.00331152, 0.03640692, 0.04260231, -0.00038223,
        -0.07480340, -0.01022492, -0.00426473, 0.01448116, 0.01456847,
        0.05786680, 0.03368691, -0.10126924, -0.00147454, 0.01275395,
        0.00017574, -0.01585206, -0.00015767, -0.00231848, 0.02310137
    },
    {
        -0.00846258, -0.01508106, -0.01967505, -0.02772004, 0.01248253,
        -0.01331243, -0.02569382, -0.04461524, -0.02207075, 0.04663443,
        0.19347923, -0.02745691, 0.02288515, -0.04883849, -0.01084597,
        -0.01947187, -0.00081675, 0.00516540, -0.07815919, 0.08035585
    },
    {
        -0.06553111, 0.09756831, 0.00524326, -0.00885098, 0.00756653,
        0.02783099, -0.00427042, -0.16680359, 0.03951331, -0.00490540,
        0.01719610, 0.15018204, 0.00882722, -0.00423197, -0.01919217,
        -0.02963619, -0.01831342, -0.00524338, 0.00011379, -0.02566864
    },
    {
        -0.07494341, -0.11348850, 0.00241343, -0.00803016, 0.00492438,
        0.00711909, -0.00829147, 0.05793337, 0.02734209, 0.02059759,
        -0.02770280, 0.14128338, 0.01532479, 0.00364307, 0.05968116,
        -0.06497960, -0.08113941, 0.00319445, -0.00104222, 0.03553497
    },
    {
        0.05948223, -0.08959930, 0.03269977, -0.03272374, -0.00365667,
        -0.03423294, -0.06418925, -0.05902138, 0.05746317, -0.02580596,
        0.01259572, 0.05848832, 0.00672666, 0.00233355, -0.05145149,
        0.07348503, 0.11427955, 0.00142592, -0.01030651, -0.04862799
    },
    {
        -0.01606880, 0.05200845, -0.01212967, -0.06824429, -0.00234304,
        0.01094203, -0.07375538, 0.08808629, 0.12394822, 0.02231351,
        -0.03608265, -0.06978045, -0.00618360, 0.00274747, -0.01921876,
        -0.01541969, -0.02223856, -0.00107603, -0.01251777, 0.05412534
    },
    {
        0.01688843, 0.05784728, -0.02256966, -0.07072251, -0.00422551,
        -0.06261233, -0.08502830, 0.08925346, -0.08529597, 0.01519343,
        -0.05008258, 0.10931873, 0.00521033, 0.02593305, -0.00717855,
        0.02291527, 0.02527388, -0.00266188, -0.00871160, 0.02708135
    },
    {
        -0.04233344, 0.00076379, 0.01571257, 0.04003092, 0.00901468,
        0.00670577, 0.03459487, 0.12420216, -0.00067366, -0.01515094,
        0.05306642, 0.04338407, 0.00511287, 0.01036639, -0.17867462,
        -0.02289440, -0.03213205, 0.00017924, -0.01187362, -0.03933874
    },
    {
        0.01284817, -0.01685622, 0.00724363, 0.01687952, -0.00882070,
        -0.00555957, 0.01676246, -0.05560456, -0.00966893, 0.06197684,
        -0.09058758, 0.00880607, 0.00108629, -0.08308956, -0.08056832,
        -0.00413297, 0.02973107, 0.00092948, 0.07010111, 0.13007418
    },
    {
        0.00700223, -0.01347574, 0.00691332, 0.03122905, 0.00310308,
        0.00946862, 0.03455040, -0.06712536, -0.00304506, 0.04267941,
        -0.10422292, -0.01127831, -0.00549798, 0.11680505, -0.03352701,
        -0.00084536, 0.01631369, 0.00095063, -0.09570217, 0.06480321
    }
};

void di_protdist::maketrans() {
    // Make up transition probability matrix from code and category tables
    long   i, j, k, m, n, s;
    double x, sum = 0;
    long   sub[3], newsub[3];
    double f[4], g[4];
    aas    b1, b2;
    double TEMP, TEMP1, TEMP2, TEMP3;

    for (i = 0; i <= 19; i++) {
        pi[i] = 0.0;
        for (j = 0; j <= 19; j++)
            prob[i][j] = 0.0;
    }
    f[0] = freqt;
    f[1] = freqc;
    f[2] = freqa;
    f[3] = freqg;
    g[0] = freqc + freqt;
    g[1] = freqc + freqt;
    g[2] = freqa + freqg;
    g[3] = freqa + freqg;
    TEMP = f[0];
    TEMP1 = f[1];
    TEMP2 = f[2];
    TEMP3 = f[3];
    fracchange = xi * (2 * f[0] * f[1] / g[0] + 2 * f[2] * f[3] / g[2]) +
        xv * (1 - TEMP * TEMP - TEMP1 * TEMP1 - TEMP2 * TEMP2 - TEMP3 * TEMP3);
    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 3; j++) {
            for (k = 0; k <= 3; k++) {
                if (trans[i][j][k] != STOP)
                    sum += f[i] * f[j] * f[k];
            }
        }
    }
    for (i = 0; i <= 3; i++) {
        sub[0] = i + 1;
        for (j = 0; j <= 3; j++) {
            sub[1] = j + 1;
            for (k = 0; k <= 3; k++) {
                sub[2] = k + 1;
                b1 = trans[i][j][k];
                for (m = 0; m <= 2; m++) {
                    s = sub[m];
                    for (n = 1; n <= 4; n++) {
                        memcpy(newsub, sub, sizeof(long) * 3L);
                        newsub[m] = n;
                        x = f[i] * f[j] * f[k] / (3.0 * sum);
                        if (((s == 1 || s == 2) && (n == 3 || n == 4)) ||
                            ((n == 1 || n == 2) && (s == 3 || s == 4)))
                            x *= xv * f[n - 1];
                        else
                            x *= xi * f[n - 1] / g[n - 1] + xv * f[n - 1];
                        b2 = trans[newsub[0] - 1][newsub[1] - 1][newsub[2] - 1];
                        if (b1 != STOP) {
                            pi[b1] += x;
                            if (b2 != STOP) {
                                if (cat[b1] != cat[b2]) {
                                    prob[b1][b2] += x * ease;
                                    prob[b1][b1] += x * (1.0 - ease);
                                } else
                                    prob[b1][b2] += x;
                            } else
                                prob[b1][b1] += x;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i <= 19; i++)
        prob[i][i] -= pi[i];
    for (i = 0; i <= 19; i++) {
        for (j = 0; j <= 19; j++)
            prob[i][j] /= sqrt(pi[i] * pi[j]);
    }
    // computes pi^(1/2)*B*pi^(-1/2)
}

void di_protdist::code()
{
    // make up table of the code 0 = u, 1 = c, 2 = a, 3 = g

    trans[0][0][0] = PHE;
    trans[0][0][1] = PHE;
    trans[0][0][2] = LEU;
    trans[0][0][3] = LEU;
    trans[0][1][0] = SER;
    trans[0][1][1] = SER;
    trans[0][1][2] = SER;
    trans[0][1][3] = SER;
    trans[0][2][0] = TYR;
    trans[0][2][1] = TYR;
    trans[0][2][2] = STOP;
    trans[0][2][3] = STOP;
    trans[0][3][0] = CYS;
    trans[0][3][1] = CYS;
    trans[0][3][2] = STOP;
    trans[0][3][3] = TRP;
    trans[1][0][0] = LEU;
    trans[1][0][1] = LEU;
    trans[1][0][2] = LEU;
    trans[1][0][3] = LEU;
    trans[1][1][0] = PRO;
    trans[1][1][1] = PRO;
    trans[1][1][2] = PRO;
    trans[1][1][3] = PRO;
    trans[1][2][0] = HIS;
    trans[1][2][1] = HIS;
    trans[1][2][2] = GLN;
    trans[1][2][3] = GLN;
    trans[1][3][0] = ARG;
    trans[1][3][1] = ARG;
    trans[1][3][2] = ARG;
    trans[1][3][3] = ARG;
    trans[2][0][0] = ILEU;
    trans[2][0][1] = ILEU;
    trans[2][0][2] = ILEU;
    trans[2][0][3] = MET;
    trans[2][1][0] = THR;
    trans[2][1][1] = THR;
    trans[2][1][2] = THR;
    trans[2][1][3] = THR;
    trans[2][2][0] = ASN;
    trans[2][2][1] = ASN;
    trans[2][2][2] = LYS;
    trans[2][2][3] = LYS;
    trans[2][3][0] = SER;
    trans[2][3][1] = SER;
    trans[2][3][2] = ARG;
    trans[2][3][3] = ARG;
    trans[3][0][0] = VAL;
    trans[3][0][1] = VAL;
    trans[3][0][2] = VAL;
    trans[3][0][3] = VAL;
    trans[3][1][0] = ALA;
    trans[3][1][1] = ALA;
    trans[3][1][2] = ALA;
    trans[3][1][3] = ALA;
    trans[3][2][0] = ASP;
    trans[3][2][1] = ASP;
    trans[3][2][2] = GLU;
    trans[3][2][3] = GLU;
    trans[3][3][0] = GLY;
    trans[3][3][1] = GLY;
    trans[3][3][2] = GLY;
    trans[3][3][3] = GLY;

    switch (whichcode) {
        case UNIVERSAL:
        case CILIATE:
            break; // use default code above

        case MITO:
            trans[0][3][2] = TRP;
            break;

        case VERTMITO:
            trans[0][3][2] = TRP;
            trans[2][3][2] = STOP;
            trans[2][3][3] = STOP;
            trans[2][0][2] = MET;
            break;

        case FLYMITO:
            trans[0][3][2] = TRP;
            trans[2][0][2] = MET;
            trans[2][3][2] = SER;
            break;

        case YEASTMITO:
            trans[0][3][2] = TRP;
            trans[1][0][2] = THR;
            trans[2][0][2] = MET;
            break;
    }
}

void di_protdist::transition() {
    // calculations related to transition-transversion ratio

    double freqr  = freqa + freqg;
    double freqy  = freqc + freqt;
    double freqgr = freqg / freqr;
    double freqty = freqt / freqy;

    double aa = ttratio * freqr * freqy - freqa * freqg - freqc * freqt;
    double bb = freqa * freqgr + freqc *                          freqty;

    xi = aa / (aa + bb);
    xv = 1.0 - xi;

    if (xi <= 0.0 && xi >= -epsilon) {
        xi = 0.0;
    }
    if (xi < 0.0) {
        GBK_terminate("This transition-transversion ratio is impossible with these base frequencies"); // @@@ should be handled better
    }
}

void di_protdist::givens(di_aa_matrix a, long i, long j, long n, double ctheta, double stheta, bool left)
{
    // Givens transform at i,j for 1..n with angle theta
    long            k;
    double          d;

    for (k = 0; k < n; k++) { // LOOP_VECTORIZED
        if (left) {
            d = ctheta * a[i - 1][k] + stheta * a[j - 1][k];
            a[j - 1][k] = ctheta * a[j - 1][k] - stheta * a[i - 1][k];
            a[i - 1][k] = d;
        }
        else {
            d = ctheta * a[k][i - 1] + stheta * a[k][j - 1];
            a[k][j - 1] = ctheta * a[k][j - 1] - stheta * a[k][i - 1];
            a[k][i - 1] = d;
        }
    }
}

void di_protdist::coeffs(double x, double y, double *c, double *s, double accuracy)
{
    // compute cosine and sine of theta
    double          root;

    root = sqrt(x * x + y * y);
    if (root < accuracy) {
        *c = 1.0;
        *s = 0.0;
    }
    else {
        *c = x / root;
        *s = y / root;
    }
}

void di_protdist::tridiag(di_aa_matrix a, long n, double accuracy)
{
    // Givens tridiagonalization
    long            i, j;
    double          s, c;

    for (i = 2; i < n; i++) {
        for (j = i + 1; j <= n; j++) {
            coeffs(a[i - 2][i - 1], a[i - 2][j - 1], &c, &s, accuracy);
            givens(a, i, j, n, c, s, true);
            givens(a, i, j, n, c, s, false);
            givens(eigvecs, i, j, n, c, s, true);
        }
    }
}

void di_protdist::shiftqr(di_aa_matrix a, long n, double accuracy)
{
    // QR eigenvalue-finder
    long            i, j;
    double          approx, s, c, d, TEMP, TEMP1;

    for (i = n; i >= 2; i--) {
        do {
            TEMP = a[i - 2][i - 2] - a[i - 1][i - 1];
            TEMP1 = a[i - 1][i - 2];
            d = sqrt(TEMP * TEMP + TEMP1 * TEMP1);
            approx = a[i - 2][i - 2] + a[i - 1][i - 1];
            if (a[i - 1][i - 1] < a[i - 2][i - 2])
                approx = (approx - d) / 2.0;
            else
                approx = (approx + d) / 2.0;
            for (j = 0; j < i; j++)
                a[j][j] -= approx;
            for (j = 1; j < i; j++) {
                coeffs(a[j - 1][j - 1], a[j][j - 1], &c, &s, accuracy);
                givens(a, j, j + 1, i, c, s, true);
                givens(a, j, j + 1, i, c, s, false);
                givens(eigvecs, j, j + 1, n, c, s, true);
            }
            for (j = 0; j < i; j++)
                a[j][j] += approx;
        } while (fabs(a[i - 1][i - 2]) > accuracy);
    }
}


void di_protdist::qreigen(di_aa_matrix proba, long n)
{
    // QR eigenvector/eigenvalue method for symmetric matrix
    double          accuracy;
    long            i, j;

    accuracy = 1.0e-6;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++)
            eigvecs[i][j] = 0.0;
        eigvecs[i][i] = 1.0;
    }
    tridiag(proba, n, accuracy);
    shiftqr(proba, n, accuracy);
    for (i = 0; i < n; i++)
        eig[i] = proba[i][i];
    for (i = 0; i <= 19; i++) {
        for (j = 0; j <= 19; j++)
            proba[i][j] = sqrt(pi[j]) * eigvecs[i][j];
    }
    // proba[i][j] is the value of U' times pi^(1/2)
}


void di_protdist::pameigen()
{
    // eigenanalysis for PAM matrix, precomputed
    memcpy(prob, pamprobs, sizeof(pamprobs));
    memcpy(eig, pameigs, sizeof(pameigs));
    fracchange = 0.01;
}

void di_protdist::build_exptteig(double tt) {
    int m;
    for (m = 0; m <= 19; m++) {
        exptteig[m] = exp(tt * eig[m]);
    }
}

void di_protdist::predict(double /* tt */, long nb1, long  nb2) {
    // make contribution to prediction of this aa pair
    for (long m = 0; m <= 19; m++) {
        double q = prob[m][nb1] * prob[m][nb2] * exptteig[m];
        p += q;
        double TEMP = eig[m];
        dp += TEMP * q;
        d2p += TEMP * TEMP * q;
    }
}

void di_protdist::build_predikt_table(int pos) {
    int             b1, b2;
    double tt = pos_2_tt(pos);
    build_exptteig(tt);
    akt_slopes = slopes[pos] = (di_paa_matrix *) calloc(sizeof(di_paa_matrix), 1);
    akt_curves = curves[pos] = (di_paa_matrix *) calloc(sizeof(di_paa_matrix), 1);
    akt_infs = infs[pos] = (di_bool_matrix *) calloc(sizeof(di_bool_matrix), 1);

    for (b1 = ALA; b1 < DI_MAX_PAA; b1++) {
        for (b2 = ALA; b2 <= b1; b2++) {
            if (b1 != STOP && b1 != DEL && b1 != QUEST && b1 != UNK &&
                b2 != STOP && b2 != DEL && b2 != QUEST && b2 != UNK) {
                p = 0.0;
                dp = 0.0;
                d2p = 0.0;
                if (b1 != ASX && b1 != GLX && b2 != ASX && b2 != GLX) {
                    predict(tt, b1, b2);
                }
                else {
                    if (b1 == ASX) {
                        if (b2 == ASX) {
                            predict(tt, 2L, 2L);
                            predict(tt, 2L, 3L);
                            predict(tt, 3L, 2L);
                            predict(tt, 3L, 3L);
                        }
                        else {
                            if (b2 == GLX) {
                                predict(tt, 2L, 5L);
                                predict(tt, 2L, 6L);
                                predict(tt, 3L, 5L);
                                predict(tt, 3L, 6L);
                            }
                            else {
                                predict(tt, 2L, b2);
                                predict(tt, 3L, b2);
                            }
                        }
                    }
                    else {
                        if (b1 == GLX) {
                            if (b2 == ASX) {
                                predict(tt, 5L, 2L);
                                predict(tt, 5L, 3L);
                                predict(tt, 6L, 2L);
                                predict(tt, 6L, 3L);
                            }
                            else {
                                if (b2 == GLX) {
                                    predict(tt, 5L, 5L);
                                    predict(tt, 5L, 6L);
                                    predict(tt, 6L, 5L);
                                    predict(tt, 6L, 6L);
                                }
                                else {
                                    predict(tt, 5L, b2);
                                    predict(tt, 6L, b2);
                                }
                            }
                        }
                        else {
                            if (b2 == ASX) {
                                predict(tt, b1, 2L);
                                predict(tt, b1, 3L);
                                predict(tt, b1, 2L);
                                predict(tt, b1, 3L);
                            }
                            else if (b2 == GLX) {
                                predict(tt, b1, 5L);
                                predict(tt, b1, 6L);
                                predict(tt, b1, 5L);
                                predict(tt, b1, 6L);
                            }
                        }
                    }
                }
                if (p > 0.0) {
                    akt_slopes[0][b1][b2] = dp / p;
                    akt_curves[0][b1][b2] = d2p / p - dp * dp / (p * p);
                    akt_infs[0][b1][b2] = 0;
                    akt_slopes[0][b2][b1] = akt_slopes[0][b1][b2];
                    akt_curves[0][b2][b1] = akt_curves[0][b1][b2];
                    akt_infs[0][b2][b1] = 0;
                }
                else {
                    akt_infs[0][b1][b2] = 1;
                    akt_infs[0][b2][b1] = 1;
                }
            }
        }
    }
}

int di_protdist::tt_2_pos(double tt) {
    int pos = (int)(tt * fracchange * DI_RESOLUTION);
    if (pos >= DI_RESOLUTION * DI_MAX_DIST)
        pos = DI_RESOLUTION * DI_MAX_DIST - 1;
    if (pos < 0)
        pos = 0;
    return pos;
}

double di_protdist::pos_2_tt(int pos) {
    double tt =  pos / (fracchange * DI_RESOLUTION);
    return tt+epsilon;
}

void di_protdist::build_akt_predikt(double tt)
{
    // take an actual slope from the hash table, else calculate a new one
    int             pos = tt_2_pos(tt);
    if (!slopes[pos]) {
        build_predikt_table(pos);
    }
    akt_slopes = slopes[pos];
    akt_curves = curves[pos];
    akt_infs = infs[pos];
    return;

}

GB_ERROR di_protdist::makedists(bool *aborted_flag) {
    /* compute the distances.
     * sets 'aborted_flag' to true, if it is non-NULL and user aborts the calculation
     */
    long   i, j, k, m, n, iterations;
    double delta, slope, curv;
    int    b1 = 0, b2=0;
    double tt = 0;
    int    pos;

    arb_progress progress("Calculating distances", matrix_halfsize(spp, false));
    GB_ERROR     error = NULL;

    for (i = 0; i < spp && !error; i++) {
        matrix->set(i, i, 0.0);
        {
            // move all unknown characters to del
            ap_pro *seq1 = entries[i]->get_prot_seq()->get_sequence();
            for (k = 0; k <chars;  k++) {
                b1 = seq1[k];
                if (b1 <= VAL) continue;
                if (b1 == ASX || b1 == GLX) continue;
                seq1[k] = DEL;
            }
        }

        for (j = 0; j < i && !error;  j++) {
            if (whichcat > KIMURA) {
                if (whichcat == PAM)
                    tt = 10.0;
                else
                    tt = 1.0;
                delta = tt / 2.0;
                iterations = 0;
                do {
                    slope = 0.0;
                    curv = 0.0;
                    pos = tt_2_pos(tt);
                    tt = pos_2_tt(pos);
                    build_akt_predikt(tt);
                    const ap_pro *seq1 = entries[i]->get_prot_seq()->get_sequence();
                    const ap_pro *seq2 = entries[j]->get_prot_seq()->get_sequence();
                    for (k = chars; k >0; k--) {
                        b1 = *(seq1++);
                        b2 = *(seq2++);
                        if (predict_infinity(b1, b2)) {
                            break;
                        }
                        slope += predict_slope(b1, b2);
                        curv += predict_curve(b1, b2);
                    }
                    iterations++;
                    if (!predict_infinity(b1, b2)) {
                        if (curv < 0.0) {
                            tt -= slope / curv;
                            if (tt > 10000.0) {
                                aw_message(GBS_global_string("Warning: infinite distance between species '%s' and '%s'\n", entries[i]->name, entries[j]->name));
                                tt = -1.0 / fracchange;
                                break;
                            }
                            int npos = tt_2_pos(tt);
                            int d = npos - pos; if (d<0) d=-d;
                            if (d<=1) { // cannot optimize
                                break;
                            }

                        }
                        else {
                            if ((slope > 0.0 && delta < 0.0) || (slope < 0.0 && delta > 0.0))
                                delta /= -2;
                            if (tt + delta < 0 && tt <= epsilon) {
                                break;
                            }
                            tt += delta;
                        }
                    }
                    else {
                        delta /= -2;
                        tt += delta;
                        if (tt < 0) tt = 0;
                    }
                } while (iterations < 20);
            }
            else {                    // cat < kimura
                m = 0;
                n = 0;
                const ap_pro *seq1 = entries[i]->get_prot_seq()->get_sequence();
                const ap_pro *seq2 = entries[j]->get_prot_seq()->get_sequence();
                for (k = chars; k >0; k--) {
                    b1 = *(seq1++);
                    b2 = *(seq2++);
                    if (b1 <= VAL && b2 <= VAL) {
                        if (b1 == b2)   m++;
                        n++;
                    }
                }
                if (n < 5) {            // no info
                    tt = -1.0;
                }
                else {
                    switch (whichcat) {
                        case KIMURA:
                            {
                                double rel = 1 - (double) m / n;
                                double drel = 1.0 - rel - 0.2 * rel * rel;
                                if (drel < 0.0) {
                                    aw_message(GBS_global_string("Warning: distance between sequences '%s' and '%s' is too large for kimura formula", entries[i]->name, entries[j]->name));
                                    tt = -1.0;
                                }
                                else {
                                    tt = -log(drel);
                                }
                            }
                            break;
                        case NONE:
                            tt = (n-m)/(double)n;
                            break;
                        case SIMILARITY:
                            tt = m/(double)n;
                            break;
                        default:
                            di_assert(0);
                            break;
                    }
                }
            }
            matrix->set(i, j, fracchange * tt);
            progress.inc_and_check_user_abort(error);
        }
    }
    if (aborted_flag && error) *aborted_flag = true;
    return error;
}


void di_protdist::clean_slopes() {
    for (int i=0; i<DI_RESOLUTION*DI_MAX_DIST; i++) {
        freenull(slopes[i]);
        freenull(curves[i]);
        freenull(infs[i]);
    }
    akt_slopes = 0;
    akt_curves = 0;
    akt_infs = 0;
}

di_protdist::~di_protdist() {
    clean_slopes();
}

di_protdist::di_protdist(di_codetype code_, di_cattype cat_, long nentries, DI_ENTRY **entries_, long seq_len, AP_smatrix *matrix_)
    : whichcode(code_),
      whichcat(cat_),
      spp(nentries),
      chars(seq_len),
      freqa(.25),
      freqc(.25),
      freqg(.25),
      freqt(.25),
      ttratio(2.0),
      ease(0.457),
      fracchange(0.0),
      entries(entries_),
      akt_slopes(NULL),
      akt_curves(NULL),
      akt_infs(NULL),
      matrix(matrix_),
      p(0.0),
      dp(0.0),
      d2p(0.0)
{
    memset(trans, 0, sizeof(trans));
    memset(pi, 0, sizeof(pi));

    for (int i = 0; i<DI_MAX_AA; ++i) {
        cat[i]      = 0;
        eig[i]      = 0;
        exptteig[i] = 0;

        for (int j = 0; j<DI_MAX_AA; ++j) {
            prob[i][j]    = 0;
            eigvecs[i][j] = 0;
        }
    }

    for (int i = 0; i<(DI_RESOLUTION*DI_MAX_DIST); ++i) {
        slopes[i] = NULL;
        curves[i] = NULL;
        infs[i]   = NULL;
    }

    transition(); // initializes members 'xi' and 'xv'

    switch (whichcat) {
        case NONE:
        case SIMILARITY:
        case KIMURA:
            fracchange = 1.0;
            break;
        case PAM:
            code();
            pameigen();
            break;
        default:
            code();
            maketrans();
            qreigen(prob, 20L);
            break;
    }
}
