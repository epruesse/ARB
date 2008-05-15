#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <math.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>

#include <awt_tree.hxx>
#include "dist.hxx"
#include <awt_csp.hxx>

#include "di_matr.hxx"
#include "di_protdist.hxx"

#define epsilon         0.000001/* a small number */

double   ph_protdist::pameigs[20] = {
	-0.022091252, -0.019297602, 0.000004760, -0.017477817,
	-0.016575549, -0.015504543, -0.002112213, -0.002685727,
	-0.002976402, -0.013440755, -0.012926992, -0.004293227,
	-0.005356688, -0.011064786, -0.010480731, -0.008760449,
	-0.007142318, -0.007381851, -0.007806557, -0.008127024
};

double   ph_protdist::pamprobs[20][20] =
{
	{	-0.01522976, -0.00746819, -0.13934468, 0.11755315, -0.00212101,
		0.01558456, -0.07408235, -0.00322387, 0.01375826, 0.00448826,
		0.00154174, 0.02013313, -0.00159183, -0.00069275, -0.00399898,
		0.08414055, -0.01188178, -0.00029870, 0.00220371, 0.00042546},
	{	-0.07765582, -0.00712634, -0.03683209, -0.08065755, -0.00462872,
		-0.03791039, 0.10642147, -0.00912185, 0.01436308, -0.00133243,
		0.00166346, 0.00624657, -0.00003363, -0.00128729, -0.00690319,
		0.17442028, -0.05373336, -0.00078751, -0.00038151, 0.01382718},
	{	-0.08810973, -0.04081786, -0.04066004, -0.04736004, -0.03275406,
		-0.03761164, -0.05047487, -0.09086213, -0.03269598, -0.03558015,
		-0.08407966, -0.07970977, -0.01504743, -0.04011920, -0.05182232,
		-0.07026991, -0.05846931, -0.01016998, -0.03047472, -0.06280511},
	{	0.02513756, -0.00578333, 0.09865453, 0.01322314, -0.00310665,
		0.05880899, -0.09252443, -0.02986539, -0.03127460, 0.01007539,
		-0.00360119, -0.01995024, 0.00094940, -0.00145868, -0.01388816,
		0.11358341, -0.12127513, -0.00054696, -0.00055627, 0.00417284},
	{	0.16517316, -0.00254742, -0.03318745, -0.01984173, 0.00031890,
		-0.02817810, 0.02661678, -0.01761215, 0.01665112, 0.10513343,
		-0.00545026, 0.01827470, -0.00207616, -0.00763758, -0.01322808,
		-0.02202576, -0.07434204, 0.00020593, 0.00119979, -0.10827873},
	{	0.16088826, 0.00056313, -0.02579303, -0.00319655, 0.00037228,
		-0.03193150, 0.01655305, -0.03028640, 0.01367746, -0.11248153,
		0.00778371, 0.02675579, 0.00243718, 0.00895470, -0.01729803,
		-0.02686964, -0.08262584, 0.00011794, -0.00225134, 0.09415650},
	{	-0.01739295, 0.00572017, -0.00712592, -0.01100922, -0.00870113,
		-0.00663461, -0.01153857, -0.02248432, -0.00382264, -0.00358612,
		-0.00139345, -0.00971460, -0.00133312, 0.01927783, -0.01053838,
		-0.00911362, -0.01010908, 0.09417598, 0.01763850, -0.00955454},
	{	0.01728888, 0.01344211, 0.01200836, 0.01857259, -0.17088517,
		0.01457592, 0.01997839, 0.02844884, 0.00839403, 0.00196862,
		0.01391984, 0.03270465, 0.00347173, -0.01940984, 0.01233979,
		0.00542887, 0.01008836, 0.00126491, -0.02863042, 0.00449764},
	{	-0.02881366, -0.02184155, -0.01566086, -0.02593764, -0.04050907,
		-0.01539603, -0.02576729, -0.05089606, -0.00597430, 0.02181643,
		0.09835597, -0.04040940, 0.00873512, 0.12139434, -0.02427882,
		-0.02945238, -0.01566867, -0.01606503, 0.09475319, 0.02238670},
	{	0.04080274, -0.02869626, -0.05191093, -0.08435843, 0.00021141,
		0.13043842, 0.00871530, 0.00496058, -0.02797641, -0.00636933,
		0.02243277, 0.03640362, -0.05735517, 0.00196918, -0.02218934,
		-0.00608972, 0.02872922, 0.00047619, 0.00151285, 0.00883489},
	{	-0.02623824, 0.00331152, 0.03640692, 0.04260231, -0.00038223,
		-0.07480340, -0.01022492, -0.00426473, 0.01448116, 0.01456847,
		0.05786680, 0.03368691, -0.10126924, -0.00147454, 0.01275395,
		0.00017574, -0.01585206, -0.00015767, -0.00231848, 0.02310137},
	{	-0.00846258, -0.01508106, -0.01967505, -0.02772004, 0.01248253,
		-0.01331243, -0.02569382, -0.04461524, -0.02207075, 0.04663443,
		0.19347923, -0.02745691, 0.02288515, -0.04883849, -0.01084597,
		-0.01947187, -0.00081675, 0.00516540, -0.07815919, 0.08035585},
	{	-0.06553111, 0.09756831, 0.00524326, -0.00885098, 0.00756653,
		0.02783099, -0.00427042, -0.16680359, 0.03951331, -0.00490540,
		0.01719610, 0.15018204, 0.00882722, -0.00423197, -0.01919217,
		-0.02963619, -0.01831342, -0.00524338, 0.00011379, -0.02566864},
	{	-0.07494341, -0.11348850, 0.00241343, -0.00803016, 0.00492438,
		0.00711909, -0.00829147, 0.05793337, 0.02734209, 0.02059759,
		-0.02770280, 0.14128338, 0.01532479, 0.00364307, 0.05968116,
		-0.06497960, -0.08113941, 0.00319445, -0.00104222, 0.03553497},
	{	0.05948223, -0.08959930, 0.03269977, -0.03272374, -0.00365667,
		-0.03423294, -0.06418925, -0.05902138, 0.05746317, -0.02580596,
		0.01259572, 0.05848832, 0.00672666, 0.00233355, -0.05145149,
		0.07348503, 0.11427955, 0.00142592, -0.01030651, -0.04862799},
	{	-0.01606880, 0.05200845, -0.01212967, -0.06824429, -0.00234304,
		0.01094203, -0.07375538, 0.08808629, 0.12394822, 0.02231351,
		-0.03608265, -0.06978045, -0.00618360, 0.00274747, -0.01921876,
		-0.01541969, -0.02223856, -0.00107603, -0.01251777, 0.05412534},
	{	0.01688843, 0.05784728, -0.02256966, -0.07072251, -0.00422551,
		-0.06261233, -0.08502830, 0.08925346, -0.08529597, 0.01519343,
		-0.05008258, 0.10931873, 0.00521033, 0.02593305, -0.00717855,
		0.02291527, 0.02527388, -0.00266188, -0.00871160, 0.02708135},
	{	-0.04233344, 0.00076379, 0.01571257, 0.04003092, 0.00901468,
		0.00670577, 0.03459487, 0.12420216, -0.00067366, -0.01515094,
		0.05306642, 0.04338407, 0.00511287, 0.01036639, -0.17867462,
		-0.02289440, -0.03213205, 0.00017924, -0.01187362, -0.03933874},
	{	0.01284817, -0.01685622, 0.00724363, 0.01687952, -0.00882070,
		-0.00555957, 0.01676246, -0.05560456, -0.00966893, 0.06197684,
		-0.09058758, 0.00880607, 0.00108629, -0.08308956, -0.08056832,
		-0.00413297, 0.02973107, 0.00092948, 0.07010111, 0.13007418},
	{	0.00700223, -0.01347574, 0.00691332, 0.03122905, 0.00310308,
		0.00946862, 0.03455040, -0.06712536, -0.00304506, 0.04267941,
		-0.10422292, -0.01127831, -0.00549798, 0.11680505, -0.03352701,
		-0.00084536, 0.01631369, 0.00095063, -0.09570217, 0.06480321}
};

void ph_protdist::cats(ph_cattype      wcat)
{
	/* define categories of amino acids */
	aas             b;

	/* fundamental subgroups */
	cat[(long) cys - (long) ala] = 1;
	cat[(long) met - (long) ala] = 2;
	cat[(long) val - (long) ala] = 3;
	cat[(long) leu - (long) ala] = 3;
	cat[(long) ileu - (long) ala] = 3;
	cat[(long) gly - (long) ala] = 4;
	cat[0] = 4;
	cat[(long) ser - (long) ala] = 4;
	cat[(long) thr - (long) ala] = 4;
	cat[(long) pro - (long) ala] = 5;
	cat[(long) phe - (long) ala] = 6;
	cat[(long) tyr - (long) ala] = 6;
	cat[(long) trp - (long) ala] = 6;
	cat[(long) glu - (long) ala] = 7;
	cat[(long) gln - (long) ala] = 7;
	cat[(long) asp - (long) ala] = 7;
	cat[(long) asn - (long) ala] = 7;
	cat[(long) lys - (long) ala] = 8;
	cat[(long) arg - (long) ala] = 8;
	cat[(long) his - (long) ala] = 8;
	if (wcat == george) {
		/*
		 * George, Hunt and Barker: sulfhydryl, small hydrophobic,
		 * small hydrophilic, aromatic, acid/acid-amide/hydrophilic,
		 * basic
		 */
		for (b = ala; (long) b <= (long) val; b = (aas) ((long) b + 1)) {
			if (cat[(long) b - (long) ala] == 3)
				cat[(long) b - (long) ala] = 2;
			if (cat[(long) b - (long) ala] == 5)
				cat[(long) b - (long) ala] = 4;
		}
	}
	if (wcat == chemical) {
		/*
		 * Conn and Stumpf:  monoamino, aliphatic, heterocyclic,
		 * aromatic, dicarboxylic, basic
		 */
		for (b = ala; (long) b <= (long) val; b = (aas) ((long) b + 1)) {
			if (cat[(long) b - (long) ala] == 2)
				cat[(long) b - (long) ala] = 1;
			if (cat[(long) b - (long) ala] == 4)
				cat[(long) b - (long) ala] = 3;
		}
	}
	/* Ben Hall's personal opinion */
	if (wcat != hall)
		return;
	for (b = ala; (long) b <= (long) val; b = (aas) ((long) b + 1)) {
		if (cat[(long) b - (long) ala] == 3)
			cat[(long) b - (long) ala] = 2;
	}
}				/* cats */


void ph_protdist::maketrans()
{
	/*
	 * Make up transition probability matrix from code and category
	 * tables
	 */
	long            i, j, k, m, n, s;
	double          x, sum = 0;
	long            sub[3], newsub[3];
	double          f[4], g[4];
	aas             b1, b2;
	double          TEMP, TEMP1, TEMP2, TEMP3;

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
				if (trans[i][j][k] != stop)
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
						if (b1 != stop) {
							pi[b1] += x;
							if (b2 != stop) {
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
	/* computes pi^(1/2)*B*pi^(-1/2)  */
}				/* maketrans */

void ph_protdist::code()
{
	/* make up table of the code 0 = u, 1 = c, 2 = a, 3 = g */

	trans[0][0][0] = phe;
	trans[0][0][1] = phe;
	trans[0][0][2] = leu;
	trans[0][0][3] = leu;
	trans[0][1][0] = ser;
	trans[0][1][1] = ser;
	trans[0][1][2] = ser;
	trans[0][1][3] = ser;
	trans[0][2][0] = tyr;
	trans[0][2][1] = tyr;
	trans[0][2][2] = stop;
	trans[0][2][3] = stop;
	trans[0][3][0] = cys;
	trans[0][3][1] = cys;
	trans[0][3][2] = stop;
	trans[0][3][3] = trp;
	trans[1][0][0] = leu;
	trans[1][0][1] = leu;
	trans[1][0][2] = leu;
	trans[1][0][3] = leu;
	trans[1][1][0] = pro;
	trans[1][1][1] = pro;
	trans[1][1][2] = pro;
	trans[1][1][3] = pro;
	trans[1][2][0] = his;
	trans[1][2][1] = his;
	trans[1][2][2] = gln;
	trans[1][2][3] = gln;
	trans[1][3][0] = arg;
	trans[1][3][1] = arg;
	trans[1][3][2] = arg;
	trans[1][3][3] = arg;
	trans[2][0][0] = ileu;
	trans[2][0][1] = ileu;
	trans[2][0][2] = ileu;
	trans[2][0][3] = met;
	trans[2][1][0] = thr;
	trans[2][1][1] = thr;
	trans[2][1][2] = thr;
	trans[2][1][3] = thr;
	trans[2][2][0] = asn;
	trans[2][2][1] = asn;
	trans[2][2][2] = lys;
	trans[2][2][3] = lys;
	trans[2][3][0] = ser;
	trans[2][3][1] = ser;
	trans[2][3][2] = arg;
	trans[2][3][3] = arg;
	trans[3][0][0] = val;
	trans[3][0][1] = val;
	trans[3][0][2] = val;
	trans[3][0][3] = val;
	trans[3][1][0] = ala;
	trans[3][1][1] = ala;
	trans[3][1][2] = ala;
	trans[3][1][3] = ala;
	trans[3][2][0] = asp;
	trans[3][2][1] = asp;
	trans[3][2][2] = glu;
	trans[3][2][3] = glu;
	trans[3][3][0] = gly;
	trans[3][3][1] = gly;
	trans[3][3][2] = gly;
	trans[3][3][3] = gly;
	if (whichcode == mito)
		trans[0][3][2] = trp;
	if (whichcode == vertmito) {
		trans[0][3][2] = trp;
		trans[2][3][2] = stop;
		trans[2][3][3] = stop;
		trans[2][0][2] = met;
	}
	if (whichcode == flymito) {
		trans[0][3][2] = trp;
		trans[2][0][2] = met;
		trans[2][3][2] = ser;
	}
	if (whichcode == yeastmito) {
		trans[0][3][2] = trp;
		trans[1][0][2] = thr;
		trans[2][0][2] = met;
	}
}				/* code */

void ph_protdist::transition()
{
	/* calculations related to transition-transversion ratio */
	double          aa, bb, freqr, freqy, freqgr, freqty;

	freqr = freqa + freqg;
	freqy = freqc + freqt;
	freqgr = freqg / freqr;
	freqty = freqt / freqy;
	aa = ttratio * freqr * freqy - freqa * freqg - freqc * freqt;
	bb = freqa * freqgr + freqc * freqty;
	xi = aa / (aa + bb);
	xv = 1.0 - xi;
	if (xi <= 0.0 && xi >= -epsilon)
		xi = 0.0;
	if (xi < 0.0) {
		printf("THIS TRANSITION-TRANSVERSION RATIO IS IMPOSSIBLE WITH");
		printf(" THESE BASE FREQUENCIES\n");
		exit(-1);
	}
}				/* transition */

void ph_protdist::givens(ph_aa_matrix a,long i,long j,long n,double ctheta,double stheta,GB_BOOL left)
{
	/* Givens transform at i,j for 1..n with angle theta */
	long            k;
	double          d;

	for (k = 0; k < n; k++) {
		if (left) {
			d = ctheta * a[i - 1][k] + stheta * a[j - 1][k];
			a[j - 1][k] = ctheta * a[j - 1][k] - stheta * a[i - 1][k];
			a[i - 1][k] = d;
		} else {
			d = ctheta * a[k][i - 1] + stheta * a[k][j - 1];
			a[k][j - 1] = ctheta * a[k][j - 1] - stheta * a[k][i - 1];
			a[k][i - 1] = d;
		}
	}
}				/* givens */

void ph_protdist::coeffs(double x,double y,double *c,double *s,double accuracy)
{
	/* compute cosine and sine of theta */
	double          root;

	root = sqrt(x * x + y * y);
	if (root < accuracy) {
		*c = 1.0;
		*s = 0.0;
	} else {
		*c = x / root;
		*s = y / root;
	}
}				/* coeffs */

void ph_protdist::tridiag(ph_aa_matrix a,long n,double accuracy)
{
	/* Givens tridiagonalization */
	long            i, j;
	double          s, c;

	for (i = 2; i < n; i++) {
		for (j = i + 1; j <= n; j++) {
			coeffs(a[i - 2][i - 1], a[i - 2][j - 1], &c, &s, accuracy);
			givens(a, i, j, n, c, s, GB_TRUE);
			givens(a, i, j, n, c, s, GB_FALSE);
			givens(eigvecs, i, j, n, c, s, GB_TRUE);
		}
	}
}				/* tridiag */

void ph_protdist::shiftqr(ph_aa_matrix a, long n, double accuracy)
{
	/* QR eigenvalue-finder */
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
				givens(a, j, j + 1, i, c, s, GB_TRUE);
				givens(a, j, j + 1, i, c, s, GB_FALSE);
				givens(eigvecs, j, j + 1, n, c, s, GB_TRUE);
			}
			for (j = 0; j < i; j++)
				a[j][j] += approx;
		} while (fabs(a[i - 1][i - 2]) > accuracy);
	}
}				/* shiftqr */


void ph_protdist::qreigen(ph_aa_matrix proba,long n)
{
	/* QR eigenvector/eigenvalue method for symmetric matrix */
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
	/* proba[i][j] is the value of U' times pi^(1/2) */
}				/* qreigen */


void ph_protdist::pameigen()
{
	/* eigenanalysis for PAM matrix, precomputed */
	memcpy(prob, pamprobs, sizeof(pamprobs));
	memcpy(eig, pameigs, sizeof(pameigs));
	fracchange = 0.01;
}				/* pameigen */

void ph_protdist::build_exptteig(double tt){
	int m;
	for (m = 0; m <= 19; m++) {
		exptteig[m] = exp(tt * eig[m]);
	}
}

void ph_protdist::predict(double /*tt*/, long nb1,long  nb2)
{
	/* make contribution to prediction of this aa pair */
	long            m;
	double		q;
	double          TEMP;
	for (m = 0; m <= 19; m++) {
		q = prob[m][nb1] * prob[m][nb2] * exptteig[m];
		p += q;
		TEMP = eig[m];
		dp += TEMP * q;
		d2p += TEMP * TEMP * q;
	}
}				/* predict */

void            ph_protdist::build_predikt_table(int pos){
	int             b1, b2;
	double tt = pos_2_tt(pos);
	build_exptteig(tt);
	akt_slopes = slopes[pos] = (ph_paa_matrix *) calloc(sizeof(ph_paa_matrix), 1);
	akt_curves = curves[pos] = (ph_paa_matrix *) calloc(sizeof(ph_paa_matrix), 1);
	akt_infs = infs[pos] = (ph_bool_matrix *) calloc(sizeof(ph_bool_matrix), 1);

	for (b1 = ala; b1 < ph_max_paa; b1++) {
		for (b2 = ala; b2 <= b1; b2++) {
			if (b1 != stop && b1 != del && b1 != quest && b1 != unk &&
			    b2 != stop && b2 != del && b2 != quest && b2 != unk) {
				p = 0.0;
				dp = 0.0;
				d2p = 0.0;
				if (b1 != asx && b1 != glx && b2 != asx && b2 != glx){
					predict(tt, b1, b2);
				} else {
					if (b1 == asx) {
						if (b2 == asx) {
							predict(tt, 2L, 2L);
							predict(tt, 2L, 3L);
							predict(tt, 3L, 2L);
							predict(tt, 3L, 3L);
						} else {
							if (b2 == glx) {
								predict(tt, 2L, 5L);
								predict(tt, 2L, 6L);
								predict(tt, 3L, 5L);
								predict(tt, 3L, 6L);
							} else {
								predict(tt, 2L, b2);
								predict(tt, 3L, b2);
							}
						}
					} else {
						if (b1 == glx) {
							if (b2 == asx) {
								predict(tt, 5L, 2L);
								predict(tt, 5L, 3L);
								predict(tt, 6L, 2L);
								predict(tt, 6L, 3L);
							} else {
								if (b2 == glx) {
									predict(tt, 5L, 5L);
									predict(tt, 5L, 6L);
									predict(tt, 6L, 5L);
									predict(tt, 6L, 6L);
								} else {
									predict(tt, 5L, b2);
									predict(tt, 6L, b2);
								}
							}
						} else {
							if (b2 == asx) {
								predict(tt, b1, 2L);
								predict(tt, b1, 3L);
								predict(tt, b1, 2L);
								predict(tt, b1, 3L);
							} else if (b2 == glx) {
								predict(tt, b1, 5L);
								predict(tt, b1, 6L);
								predict(tt, b1, 5L);
								predict(tt, b1, 6L);
							}
						}
					}
				}
				if (p > 0.0){
					akt_slopes[0][b1][b2] = dp / p;
					akt_curves[0][b1][b2] = d2p / p - dp * dp / (p * p);
					akt_infs[0][b1][b2] = 0;
					akt_slopes[0][b2][b1] = akt_slopes[0][b1][b2];
					akt_curves[0][b2][b1] = akt_curves[0][b1][b2];
					akt_infs[0][b2][b1] = 0;
				}else{
					akt_infs[0][b1][b2] = 1;
					akt_infs[0][b2][b1] = 1;
				}
			}//if
		}// b2

	} //for b1
}

int ph_protdist::tt_2_pos(double tt) {
	int pos = (int)(tt * fracchange * ph_resolution);
	if (pos >= ph_resolution * ph_max_dist )
		pos = ph_resolution * ph_max_dist - 1;
	if (pos < 0)
		pos = 0;
	return pos;
}

double ph_protdist::pos_2_tt(int pos) {
	double tt =  pos / (fracchange * ph_resolution);
	return tt+epsilon;
}

void            ph_protdist::build_akt_predikt(double tt)
{
	/* take an aktual slope from the hash table, else calculate a new one */
	int             pos = tt_2_pos(tt);
	if (!slopes[pos]){
	 	build_predikt_table(pos);
	}
	akt_slopes = slopes[pos];
	akt_curves = curves[pos];
	akt_infs = infs[pos];
	return;

}

const char *ph_protdist::makedists()
{
    /* compute the distances */
    long            i, j, k, m, n, iterations;
    double          delta, slope, curv;
    int             b1=0, b2=0;
    double		tt=0;
    int		pos;

    for (i = 0; i < spp; i++) {
	matrix->set(i,i,0.0);
	{
	    double gauge = (double)i/(double)spp;
	    if (aw_status(gauge*gauge)) return "Aborted";
	}
	{
	    /* move all unknown characters to del */
	    ap_pro *seq1 = entries[i]->sequence_protein->sequence;
	    for (k = 0; k <chars ; k++) {
		b1 = seq1[k];
		if (b1 <=val) continue;
		if (b1 == asx || b1 == glx) continue;
		seq1[k] = del;
	    }
	}

	for (j = 0; j < i ; j++) {
	    if (whichcat > kimura ) {
		if (whichcat == pam)
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
		    ap_pro *seq1 = entries[i]->sequence_protein->sequence;
		    ap_pro *seq2 = entries[j]->sequence_protein->sequence;
		    for (k = chars; k >0; k--) {
			b1 = *(seq1++);
			b2 = *(seq2++);
			if (predict_infinity(b1,b2)){
			    break;
			}
			slope += predict_slope(b1,b2);
			curv += predict_curve(b1,b2);
		    }
		    iterations++;
		    if (!predict_infinity(b1,b2)) {
			if (curv < 0.0) {
			    tt -= slope / curv;
			    if (tt > 10000.0) {
				aw_message(GB_export_error("INFINITE DISTANCE BETWEEN SPECIES %ld AND %ld; -1.0 WAS WRITTEN\n", i, j));
				tt = -1.0 / fracchange;
				break;
			    }
			    int npos = tt_2_pos(tt);
			    int d = npos - pos; if (d<0) d=-d;
			    if (d<=1){	// cannot optimize
				break;
			    }

			} else {
			    if ((slope > 0.0 && delta < 0.0) || (slope < 0.0 && delta > 0.0))
				delta /= -2;
			    if (tt + delta < 0 && tt<= epsilon) {
				break;
			    }
			    tt += delta;
			}
		    } else {
			delta /= -2;
			tt += delta;
			if (tt < 0) tt = 0;
		    }
		} while (iterations < 20);
	    } else {			// cat < kimura
		m = 0;
		n = 0;
		ap_pro *seq1 = entries[i]->sequence_protein->sequence;
		ap_pro *seq2 = entries[j]->sequence_protein->sequence;
		for (k = chars; k >0; k--) {
		    b1 = *(seq1++);
		    b2 = *(seq2++);
		    if (b1 <= val && b2 <= val) {
			if (b1 == b2)	m++;
			n++;
		    }
		}
		if (n < 5) {		// no info
		    tt = -1.0;
		}else{
		    switch (whichcat) {
			case kimura:
			{
			    double rel = 1 - (double) m / n;
			    double drel = 1.0 - rel - 0.2 * rel * rel;
			    if (drel < 0.0) {
				aw_message(GB_export_error("DISTANCE BETWEEN SEQUENCES %3ld AND %3ld IS TOO LARGE FOR KIMURA FORMULA", i, j));
				tt = -1.0;
			    }else{
				tt = -log(drel);
			    }
			}
			break;
			case none:
			    tt = (n-m)/(double)n;
			    break;
			case similarity:
			    tt = m/(double)n;
			    break;
			default:
			    GB_CORE;
			    break;
		    }
		}
	    }
	    matrix->set(i,j,fracchange * tt);
	}
    }
    return 0;
}				/* makedists */


void ph_protdist::clean_slopes(){
	int i;
	if (slopes) {
		for (i=0;i<ph_resolution*ph_max_dist;i++) {
			delete slopes[i]; slopes[i] = 0;
			delete curves[i]; curves[i] = 0;
			delete infs[i]; infs[i] = 0;
		}
	}
	akt_slopes = 0;
	akt_curves = 0;
	akt_infs = 0;
}

ph_protdist::~ph_protdist(){
	clean_slopes();
}

ph_protdist::ph_protdist(ph_codetype codei, ph_cattype cati, long nentries, PHENTRY	**entriesi, long seq_len, AP_smatrix *matrixi){
	memset((char *)this,0,sizeof(ph_protdist));
	entries = entriesi;
	matrix = matrixi;
	freqa = .25;
	freqc = .25;
	freqg = .25;
	freqt = .25;
	ttratio = 2.0;
	ease = 0.457;
	spp = nentries;
	chars = seq_len;
	transition();
	whichcode = codei;
	whichcat = cati;
	switch(cati){
		case none:
		case similarity:
		case kimura:
			fracchange = 1.0;
			break;
		case pam:
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

