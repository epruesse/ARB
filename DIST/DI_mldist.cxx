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
#include "di_mldist.hxx"

#define epsilon         0.000001/* a small number */


void ph_mldist::givens(ph_ml_matrix a,long i,long j,long n,double ctheta,double stheta,GB_BOOL left)
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

void ph_mldist::coeffs(double x,double y,double *c,double *s,double accuracy)
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

void ph_mldist::tridiag(ph_ml_matrix a,long n,double accuracy)
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

void ph_mldist::shiftqr(ph_ml_matrix a, long n, double accuracy)
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


void ph_mldist::qreigen(ph_ml_matrix proba,long n)
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
	for (i = 0; i < n_states; i++) {
		for (j = 0; j < n_states; j++)
			proba[i][j] = sqrt(pi[j]) * eigvecs[i][j];
	}
	/* proba[i][j] is the value of U' times pi^(1/2) */
}				/* qreigen */


			/* pameigen */

void ph_mldist::build_exptteig(double tt){
	int m;
	for (m = 0; m < n_states; m++) {
		exptteig[m] = exp(tt * eig[m]);
	}
}

void ph_mldist::predict(double /*tt*/, long nb1,long  nb2)
{
	/* make contribution to prediction of this aa pair */
	long            m;
	double		q;
	double          TEMP;
	for (m = n_states-1; m >=0; m--) {
		q = prob[m][nb1] * prob[m][nb2] * exptteig[m];
		p += q;
		TEMP = eig[m];
		dp += TEMP * q;
		d2p += TEMP * TEMP * q;
	}
}				/* predict */

void            ph_mldist::build_predikt_table(int pos){
	int             b1, b2;
	double tt = pos_2_tt(pos);
	build_exptteig(tt);
	akt_slopes = slopes[pos] = (ph_pml_matrix *) calloc(sizeof(ph_pml_matrix), 1);
	akt_curves = curves[pos] = (ph_pml_matrix *) calloc(sizeof(ph_pml_matrix), 1);
	akt_infs = infs[pos] = (ph_bool_matrix *) calloc(sizeof(ph_bool_matrix), 1);

	for (b1 = 0; b1 < this->n_states; b1++) {
		for (b2 = 0; b2 <= b1; b2++) {
		    p = 0.0;
		    dp = 0.0;
		    d2p = 0.0;
		    predict(tt, b1, b2);

		    if (p > 0.0){
			double ip = 1.0/p;
			akt_slopes[0][b1][b2] = dp * ip;
			akt_curves[0][b1][b2] = d2p * ip - dp * dp * (ip * ip);
			akt_infs[0][b1][b2] = 0;
			akt_slopes[0][b2][b1] = akt_slopes[0][b1][b2];
			akt_curves[0][b2][b1] = akt_curves[0][b1][b2];
			akt_infs[0][b2][b1] = 0;
		    }else{
			akt_infs[0][b1][b2] = 1;
			akt_infs[0][b2][b1] = 1;
		    }
		}
	}
}

int ph_mldist::tt_2_pos(double tt) {
	int pos = (int)(tt * fracchange * PH_ML_RESOLUTION);
	if (pos >= PH_ML_RESOLUTION * PH_ML_MAX_DIST )
		pos = PH_ML_RESOLUTION * PH_ML_MAX_DIST - 1;
	if (pos < 0)
		pos = 0;
	return pos;
}

double ph_mldist::pos_2_tt(int pos) {
	double tt =  pos / (fracchange * PH_ML_RESOLUTION);
	return tt+epsilon;
}

void            ph_mldist::build_akt_predikt(double tt)
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

const char *ph_mldist::makedists()
{
    /* compute the distances */
    long            i, j, k, iterations;
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
	    }
	    matrix->set(i,j,fracchange * tt);
    }
    return 0;
}				/* makedists */


void ph_mldist::clean_slopes(){
	int i;
	if (slopes) {
		for (i=0;i<PH_ML_RESOLUTION*PH_ML_MAX_DIST;i++) {
			delete slopes[i]; slopes[i] = 0;
			delete curves[i]; curves[i] = 0;
			delete infs[i]; infs[i] = 0;
		}
	}
	akt_slopes = 0;
	akt_curves = 0;
	akt_infs = 0;
}

ph_mldist::~ph_mldist(){
	clean_slopes();
}

ph_mldist::ph_mldist(long nentries, PHENTRY	**entriesi, long seq_len, AP_smatrix *matrixi){
	memset((char *)this,0,sizeof(ph_mldist));
	entries = entriesi;
	matrix = matrixi;

	spp = nentries;
	chars = seq_len;

	//maketrans();
	qreigen(prob, 20L);
}

