/*
 * distan.c   Adachi, J.   1995.10.26
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa. All rights reserved.
 */

#define VARIOTU 0
#define PUTDISLKLHD 0
#define DISTAN_DEBUG 0
#define PUTDISPLOT 0
#define PUTDISNUM 2000

#include "protml.h"

#if PUTDISPLOT
static void
distan2(dislkl, initdist, seqi, seqj, seqw, nsite)
dmatrix dislkl;
double initdist;
ivector seqi, seqj, seqw;
{
	int i, j, k, l;
	double dist, distdiff, maxlkl, maxdis, coef;
	double lkl, ld1, lklhd, lkld1, lkld2;
	dmattpmty tprob, tdiff1, tdiff2;

	maxlkl = - DBL_MAX;
	for (l = 0, dist = 0.1; l < PUTDISNUM; l++) {
		coef = 0.5;
		dist = (l+1.0)/10.0;
		tdiffmtrx(dist, tprob, tdiff1, tdiff2);
		lklhd = lkld1 = lkld2 = 0.0;
		for (k = 0; k < nsite; k++) {
			i = seqi[k];
			j = seqj[k];
			lkl = tprob[i][j];
			ld1 = tdiff1[i][j] / lkl;
			lklhd += log(lkl) * (double)seqw[k];
			lkld1 += ld1 * (double)seqw[k];
			lkld2 += (tdiff2[i][j]/lkl - ld1*ld1) * (double)seqw[k];
		}
		distdiff = - (lkld1 / lkld2);
		dislkl[0][l] = lklhd;
		dislkl[1][l] = lkld1;
		dislkl[2][l] = lkld2;
		dislkl[3][l] = dist + distdiff;
		if (lkld2 < 0) {
			if (lkld1 > 0) {
				dislkl[4][l] = dist + distdiff;
				coef = 0.5;
			} else {
				if (dist + distdiff > dist * coef) {
					dislkl[4][l] = dist + distdiff;
				} else {
					dislkl[4][l] = dist * coef;
					coef *= 0.5;
				}
			}
		} else {
			dislkl[4][l] = dist * coef;
			coef *= 0.5;
		}
		if (dislkl[4][l] < LOWERLIMIT) dislkl[4][l] = LOWERLIMIT;
		if (dislkl[4][l] > UPPERLIMIT) dislkl[4][l] = UPPERLIMIT;
		if (lklhd > maxlkl) {
			maxlkl = lklhd;
			maxdis = dist;
		}
	}
	printf("# initdis: %.3f  dis: %.3f  ln L: %.3f\n",initdist,maxdis,maxlkl);
} /* distan2 */
#endif /* PUTDISPLOT */


static void
distan(dis, vdis, lklhd, ii, jj, seqi, seqj, seqw, nsite)
double *dis, *vdis, *lklhd;
int ii, jj;
ivector seqi, seqj, seqw;
int nsite;
{
	int i, j, k, l;
	int maxloop = 10;
	double dist, distold, distdiff, coef;
	double lkl, ld1, lkld1, lkld2;
	dmattpmty tprob, tdiff1, tdiff2;

	dist = *dis;
	coef = INITCOEFMLE;
	for (l = 0; l < maxloop; l++) {
		tdiffmtrx(dist, tprob, tdiff1, tdiff2);
		lkld1 = lkld2 = 0.0;
		for (k = 0; k < nsite; k++) {
			i = seqi[k];
			j = seqj[k];
			lkl = tprob[i][j];
			ld1 = tdiff1[i][j] / lkl;
			lkld1 += ld1 * (double)seqw[k];
			lkld2 += (tdiff2[i][j] / lkl - ld1*ld1 ) * (double)seqw[k];
		}
		distold = dist;
		distdiff = -(lkld1 / lkld2);
		if (lkld1 > 0) {
			dist += distdiff;
			coef  = INITCOEFMLE;
		} else {
			if (lkld2 < 0) {
				if (dist + distdiff > dist * coef) {
					dist += distdiff;
					coef  = INITCOEFMLE;
				} else {
					dist *= coef;
					coef *= INITCOEFMLE;
				}
			} else {
				dist *= coef;
				coef *= INITCOEFMLE;
			}
		}
		if (dist < LOWERLIMIT) dist = LOWERLIMIT;
		if (dist > UPPERLIMIT) dist = UPPERLIMIT;
#if DISTAN_DEBUG
		if (Debug)
		printf("%3d %8.5f %8.5f %8.5f %10.5f %8.5f\n",
			l, lkld1, lkld2, distdiff, dist, dist - distold);
#endif
		if (fabs(distold-dist) < DEPSILON && fabs(lkld1) < 1.0) /* EPSILON */
			break;
	}
	if (Debug_optn && Debug)
		printf("%3d %8.5f %8.5f %8.5f %10.5f %8.5f\n",
			l+1,lkld1,lkld2,distdiff,dist, dist - distold);
	if (fabs(distold - dist) > DEPSILON) {
		fprintf(stderr, "ERROR, distance(%s,%s), non convergence!\n",
			Identif[ii], Identif[jj]);
	}
	if (lkld2 > 0.0) {
		fprintf(stderr, "ERROR, distance(%s,%s) estimation!\n",
			Identif[ii], Identif[jj]);
		fprintf(stderr, "second derivative is negative!\n");
	}
	if (dist == UPPERLIMIT) {
		fprintf(stderr, "WARNING, distance(%s,%s) became upper limit!\n",
			Identif[ii], Identif[jj]);
	}
	*dis = dist;
	*vdis = 1.0 / fabs(lkld2);

	for (k = 0, lkl = 0.0; k < nsite; k++) {
		i = seqi[k];
		j = seqj[k];
		lkl += log(Freqtpm[i] * tprob[i][j]);
	}
	*lklhd = lkl;

} /* distan */


void
distance(distanmat, seqchar, maxspc, numsite)
dmatrix distanmat;
cmatrix seqchar;
int maxspc, numsite;
{
	int i, j, k, l, nsite, diff;
	double dist, vdis, lklhd;
	ivector seqi, seqj, seqw;
	cvector seqchi, seqchj;
	int gene[TPMRADIX + 1];
#if VARIOTU
	dvector variotu;
#endif

#if PUTDISLKLHD
	dmatrix lkldistan;
	dvector lkllinfotu;
	lkldistan = new_dmatrix(maxspc, maxspc);
	lkllinfotu = new_dvector(maxspc);
	for (i = 0; i < maxspc; i++) lkllinfotu[i] = 0.0;
#endif
#if PUTDISPLOT
	dmatrix dislkl;
	dislkl = new_dmatrix(5, PUTDISNUM);
#endif
#if VARIOTU
	variotu = new_dvector(maxspc);
	for (i = 0; i < maxspc; i++) variotu[i] = 0.0;
#endif

	if (Verbs_optn) fprintf(stderr,"distance matrix [%d][%d]\n",maxspc,maxspc);
	seqi = new_ivector(numsite);
	seqj = new_ivector(numsite);
	seqw = new_ivector(numsite);
	for (i = 0; i < maxspc-1; i++) {
		seqchi = seqchar[i];
		for (j = i+1; j < maxspc; j++) {
			seqchj = seqchar[j];
#if 0
			for (k=0; k<numsite; k++) putchar(chint2ami(seqchi[k]));
			putchar('\n');
			for (k=0; k<numsite; k++) putchar(chint2ami(seqchj[k]));
			putchar('\n');
#endif
			for ( k = 0; k < Tpmradix + 1; k++) gene[k] = 0;
			for ( k = 0; k < numsite; k++) {
				if (seqchi[k] == seqchj[k]) {
					gene[seqchi[k]]++;
					seqw[k] = FALSE;
				} else if (seqchi[k] == Tpmradix || seqchj[k] == Tpmradix) {
					seqw[k] = FALSE;
				} else {
					seqw[k] = TRUE;
				}
			}
			for ( k = 0, l = 0; k < numsite; k++) {
				if (seqw[k]) {
					seqi[l] = seqchi[k];
					seqj[l] = seqchj[k];
					seqw[l] = 1;
					l++;
				}
			}
			diff = l;
			for ( k = 0; k < Tpmradix; k++) {
				if (gene[k]) {
					seqi[l] = k;
					seqj[l] = k;
					seqw[l] = gene[k];
					l++;
				}
			}
			nsite = l;
#ifdef DISDEBUG
			for (k=0; k<nsite; k++) putchar(int2ami(seqi[k])); putchar('\n');
			for (k=0; k<nsite; k++) putchar(int2ami(seqj[k])); putchar('\n');
			for (k=0; k<nsite; k++) printf("%d",seqw[k]); putchar('\n');
#endif
			if (diff == 0) {
				dist = 0.0;
				vdis = 0.0;
			} else {
				if (diff < numsite)
					dist = -log(1.0 - (double)diff/(double)numsite)*100.0;
				else
					dist = -log(1.0 - (double)diff/(double)(numsite+1.0))*100.0;
				if (dist < LOWERLIMIT) dist = LOWERLIMIT;
				if (dist > UPPERLIMIT) dist = UPPERLIMIT;
#if PUTDISPLOT
				distan2(dislkl, dist, seqi, seqj, seqw, nsite);
#else
				distan(&dist, &vdis, &lklhd, i, j, seqi, seqj, seqw, nsite);
#endif
			}
			distanmat[i][j] = dist;
			if (Varia_optn)
				distanmat[j][i] = vdis;
			else
				distanmat[j][i] = dist;
#if VARIOTU
			variotu[i] += vdis; /* += vdis */
			variotu[j] += vdis;
#endif
#if PUTDISLKLHD
				lklhd *= -100.0 / (double)nsite;
				lkldistan[i][j] = lkldistan[j][i] = lklhd;
				lkllinfotu[i] += lklhd;
				lkllinfotu[j] += lklhd;
#endif
		}
		distanmat[i][i] = 0.0;
	}
	distanmat[maxspc-1][maxspc-1] = 0.0;
	free_ivector(seqw);
	free_ivector(seqj);
	free_ivector(seqi);

#if VARIOTU
	for (i = 0; i < maxspc; i++) {
		printf("%3d %-7s %10.1f\n", i+1, Identif[i], variotu[i]);
	}
#endif
 
	if (Debug)
	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < maxspc; j++) {
			printf("%5.1f", distanmat[i][j]);
		}
		printf("\n");
	}
#if PUTDISLKLHD
	for (j = 0; j < maxspc; j++) printf("%5d", j+1); printf("\n");
	for (j = 0; j < maxspc; j++) printf("%5.4s", Identif[j]); printf("\n");
	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < maxspc; j++) {
			printf("%5.0f", lkldistan[i][j]);
		} printf("\n");
	}
	for (j = 0; j < maxspc; j++) {
		lkllinfotu[j] /= maxspc;
		printf("%5.0f", lkllinfotu[j]);
	} printf("\n");
	for (i = 0; i < maxspc; i++) {
		for (j = 0, lklhd = 0.0; j < maxspc; j++) {
			lklhd += fabs(lkldistan[i][j] - lkllinfotu[i]);
		}
		printf("%3d %-7s %10.1f\n", i+1, Identif[i], lklhd);
	}
	free_dmatrix(lkldistan);
	free_dvector(lkllinfotu);
	exit(1);
#endif
#if PUTDISPLOT
	for (k = 0; k < PUTDISNUM; k++) {
		printf("%6.1f", (k+1.0) / 10.0);
		for (i = 0; i < 5; i++)
			printf(" %12.5f",dislkl[i][k]);
		putchar('\n');
	}
	free_dmatrix(dislkl);
	exit(1);
#endif
#if VARIOTU
	free_dvector(variotu);
#endif
} /* distance */


static double 
determinant(amat, size)
dmatrix amat;
int size;
{
	/* DETERMINANT ON LU DECOMPOSITION */
    double eps = 1.0e-20; /* ! */
	int i, j, k, maxi;
	double sum, tmp, maxb, det;
	dvector wk;
	ivector index;

	wk = new_dvector(size);
	index = new_ivector(size);
	det = 1.0;
	for (i = 0; i < size; i++) {
		maxb = 0.0;
		for (j = 0; j < size; j++) {
			if (fabs(amat[i][j]) > maxb)
				maxb = fabs(amat[i][j]);
		}
		if (maxb == 0.0) {
			fprintf(stderr, "determinant: singular matrix\n");
			exit(1);
		}
		wk[i] = 1.0 / maxb;
	}
	for (j = 0; j < size; j++) {
		for (i = 0; i < j; i++) {
			sum = amat[i][j];
			for (k = 0; k < i; k++)
				sum -= amat[i][k] * amat[k][j];
			amat[i][j] = sum;
		}
		maxb = 0.0;
		for (i = j; i < size; i++) {
			sum = amat[i][j];
			for (k = 0; k < j; k++)
				sum -= amat[i][k] * amat[k][j];
			amat[i][j] = sum;
			tmp = wk[i] * fabs(sum);
			if (tmp >= maxb) {
				maxb = tmp;
				maxi = i;
			}
		}
		if (j != maxi) {
			for (k = 0; k < size; k++) {
				tmp = amat[maxi][k];
				amat[maxi][k] = amat[j][k];
				amat[j][k] = tmp;
			}
			det = -det;
			wk[maxi] = wk[j];
		}
		index[j] = maxi;
		if (amat[j][j] == 0.0)
			amat[j][j] = eps;
		if (j != size - 1) {
			tmp = 1.0 / amat[j][j];
			for (i = j + 1; i < size; i++)
				amat[i][j] *= tmp;
		}
	}

	for (i = 0; i < size; i++) {
		det *= amat[i][i];
	}
	free_ivector(index);
	free_dvector(wk);
	return det;
} /*_ determinant */


void
lddistance(distanmat, seqchar, maxspc, numsite)
dmatrix distanmat;
cmatrix seqchar;
int maxspc, numsite;
{
	int i, j, k, x, y, nsite;
	double dist, comp;
	cvector seqchi, seqchj;
	dmatrix fxy, fxxyy;
	dvectpmty fx, fy;

	fxy = new_dmatrix(Tpmradix, Tpmradix);
	fxxyy = new_dmatrix(Tpmradix, Tpmradix);
	for (i = 0; i < maxspc-1; i++) {
		seqchi = seqchar[i];
		for (j = i+1; j < maxspc; j++) {
			seqchj = seqchar[j];
			for (x = 0; x < Tpmradix; x++) {
				for (y = 0; y < Tpmradix; y++) {
					fxy[x][y] = 0.0;
					fxxyy[x][y] = 0.0;
				}
				fx[x] = 0.0;
				fy[x] = 0.0;
			}
			nsite = 0;
			for (k = 0; k < numsite; k++) {
				if ( (x = seqchi[k]) != Tpmradix &&
					 (y = seqchj[k]) != Tpmradix ) {
					fxy[x][y] += 1.0;
					fx[x] += 1.0;
					fy[y] += 1.0;
					nsite++;
				}
			}
			for (x = 0; x < Tpmradix; x++) {
				for (y = 0; y < Tpmradix; y++) {
					fxy[x][y] /= nsite;
				}
				fxxyy[x][x] = fx[x] * fy[x];
			}
			if (Debug) {
				for (x = 0; x < Tpmradix; x++) {
					putchar('\n');
					for (y = 0; y < Tpmradix; y++) {
						printf("%10.5f", fxy[x][y]);
					};
				} putchar('\n');
			}
			dist = -log( determinant(fxy, Tpmradix) );
			comp =  log( determinant(fxxyy, Tpmradix) ) / 2.0;
			if (Debug) printf("%10.5f%10.5f\n", dist, comp);
			dist = ( dist + comp ) / Tpmradix;
			if (dist < LOWERLIMIT) dist = LOWERLIMIT;
			if (dist > UPPERLIMIT) dist = UPPERLIMIT;
			distanmat[i][j] = dist;
			distanmat[j][i] = dist;
		}
		distanmat[i][i] = 0.0;
	}
	distanmat[maxspc-1][maxspc-1] = 0.0;
	free_dmatrix(fxy);
	free_dmatrix(fxxyy);
 
	if (Debug)
	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < maxspc; j++) {
			printf("%5.1f", distanmat[i][j]);
		}
		printf("\n");
	}
} /* lddistance */


static void
pdistan(dis, seq, probk, weight, nptrn)
double *dis;
ivector seq;
dmatrix probk;
ivector weight;
int nptrn;
{
	int i, j, k, l;
	int maxloop = 30;
	double dist, distold, distdiff;
	double lkl, ld1, ld2, lkld1, lkld2, prod;
	dmattpmty tprob, tdiff1, tdiff2;

	dist = *dis;
	for (l = 0; l < maxloop; l++) {
		tdiffmtrx(dist, tprob, tdiff1, tdiff2);
		lkld1 = lkld2 = 0.0;
		for (k = 0; k < nptrn; k++) {
			j = seq[k];
		/*	printf(" %d", i); */
			lkl = ld1 = ld2 = 0.0;
			for (i = 0; i < Tpmradix; i++) {
				prod = Freqtpm[i] * probk[k][i];
				lkl += prod *  tprob[i][j];
				ld1 += prod * tdiff1[i][j];
				ld2 += prod * tdiff2[i][j];
			}
			ld1 /= lkl;
			lkld1 += ld1 * weight[k];
			lkld2 += (ld2 / lkl - ld1 * ld1 ) * weight[k];
		}
		distold = dist;
		distdiff = -(lkld1 / lkld2);
		if (distdiff > UPPERLIMIT - distold) {
			if (distold < UPPERLIMIT / 2.0)
				dist = LOWERLIMIT;
			else
				dist = UPPERLIMIT;
		} else
			dist += distdiff;
		if (dist < LOWERLIMIT) dist = LOWERLIMIT;
		if (dist > UPPERLIMIT) dist = UPPERLIMIT;
#if DISTAN_DEBUG
		if (Debug)
		printf("%3d %8.5f %8.5f %8.5f %8.5f\n",
			l, lkld1, lkld2, distdiff, dist);
#endif
		if (fabs(distold - dist) < DEPSILON)  /* EPSILON */
			break;
	}
	*dis = dist;
} /* pdistan */


void
tdistan(seqi, seqj, probk, weight, nptrn, len, lvari, triprob)
ivector seqi, seqj;
dmatrix probk;
ivector weight;
int nptrn;
double *len, *lvari;
dcube triprob;
{
	int i, j, k, l, m, nl, cnvrg, maxloop = 3;
	double lenm, lenold, lendiff, lenpre, lkl, ld1, ld2, lkld1, lkld2;
	double sum, prod1, prod2, minarc, maxarc;
	dmattpmty tprob, tdiff1, tdiff2;
	ivector triseq[3], seq;
	dmatrix mprob, cprob, dprob;

	minarc = LOWERLIMIT;
	maxarc = 100.0;

	triseq[1] = seqi;
	triseq[2] = seqj;
	tprobmtrx(len[0], tprob);
	mprob = triprob[0];
	for (k = 0; k < nptrn; k++) {
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0, sum = 0.0; j < Tpmradix; j++)
				sum += probk[k][j] * tprob[j][i];
			mprob[k][i] = sum;
		}
	}
	for (m = 1; m < 3; m++) {
		tprobmtrx(len[m], tprob);
		seq = triseq[m];
		mprob = triprob[m];
		for (k = 0; k < nptrn; k++) {
			j = seq[k];
			for (i = 0; i < Tpmradix; i++) mprob[k][i] = tprob[j][i];
		}
	}
#if DISTAN_DEBUG
	if (Debug)
		printf("               %10.5f %10.5f %10.5f\n", len[0],len[1],len[2]);
#endif
	cnvrg = 0;
	for (nl = 0; nl < 20; nl++) {

		mprob = triprob[0];
		cprob = triprob[1];
		dprob = triprob[2];
		for (k = 0; k < nptrn; k++) {
			for (i = 0; i < Tpmradix; i++)
				cprob[k][i] *= dprob[k][i];
		}
		lenm = len[0];
		lenpre = lenm;
		for (l = 0; l < maxloop; l++) {
			tdiffmtrx(lenm, tprob, tdiff1, tdiff2);
			lkld1 = lkld2 = 0.0;
			for (k = 0; k < nptrn; k++) {
				lkl = ld1 = ld2 = 0.0;
				for (i = 0; i < Tpmradix; i++) {
					prod1 = probk[k][i];
				/*	prod1 = Freqtpm[i] * probk[k][i]; */
					for (j = 0; j < Tpmradix; j++) {
						prod2 = prod1 * cprob[k][j];
						lkl += prod2 * tprob[i][j];
						ld1 += prod2 * tdiff1[i][j];
						ld2 += prod2 * tdiff2[i][j];
					}
				}
				ld1 /= lkl;
				lkld1 += ld1 * (double)weight[k];
				lkld2 += (ld2 / lkl - ld1 * ld1) * (double)weight[k];
			}
			lenold = lenm;
			lendiff = -(lkld1 / lkld2);
#if 0
			printf("%3d %10.5f %10.5f %10.5f %10.5f\n",
				l,lenm,lkld1,lkld2,lendiff);
#endif
			if (lendiff > maxarc - lenold) {
				if (lenold < maxarc / 2.0) lenm = minarc;
				else                       lenm = maxarc;
			} else {
				 lenm += lendiff;
			}
			if (lenm < minarc) lenm = minarc;
			if (lenm > maxarc) lenm = maxarc;
			len[0] = lenm;
			lvari[0] = fabs(lkld2);
#if DISTAN_DEBUG
			if (Debug)
				printf("%3d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f\n",
					l,lenm,len[0],len[1],len[2],lkld1,lkld2);
#endif
			if (fabs(lenold - lenm) < DEPSILON)
				break;
		}
		if (abs(lenm - lenpre) < DEPSILON) {
			cnvrg++;
			if (cnvrg >= 9)
				goto converge;
		} else {
			cnvrg = 0;
		}
		tprobmtrx(len[0], tprob);
		for (k = 0; k < nptrn; k++) {
			for (i = 0; i < Tpmradix; i++) {
				for (j = 0, sum = 0.0; j < Tpmradix; j++)
					sum += probk[k][j] * tprob[j][i];
				mprob[k][i] = sum;
			}
		}

		for (m = 1; m < 3; m++) {
			if (m == 1) {
				mprob = triprob[1];
				cprob = triprob[2];
				dprob = triprob[0];
			} else {
				mprob = triprob[2];
				cprob = triprob[0];
				dprob = triprob[1];
			}
			for (k = 0; k < nptrn; k++) {
				for (i = 0; i < Tpmradix; i++)
					cprob[k][i] *= dprob[k][i];
			}
			lenm = len[m];
			lenpre = lenm;
			seq = triseq[m];
			for (l = 0; l < maxloop; l++) {
				tdiffmtrx(lenm, tprob, tdiff1, tdiff2);
				lkld1 = lkld2 = 0.0;
				for (k = 0; k < nptrn; k++) {
					j = seq[k];
					lkl = ld1 = ld2 = 0.0;
					for (i = 0; i < Tpmradix; i++) {
						lkl += tprob[j][i]  * cprob[k][i];
						ld1 += tdiff1[j][i] * cprob[k][i];
						ld2 += tdiff2[j][i] * cprob[k][i];
					}
					ld1 /= lkl;
					lkld1 += ld1 * (double)weight[k];
					lkld2 += (ld2 / lkl - ld1 * ld1) * (double)weight[k];
				}
				lenold = lenm;
				lendiff = -(lkld1 / lkld2);
				if (lendiff > UPPERLIMIT - lenold) {
					if (lenold < UPPERLIMIT / 2.0) lenm = LOWERLIMIT;
					else                       lenm = UPPERLIMIT;
				} else {
					 lenm += lendiff;
				}
				if (lenm < LOWERLIMIT) lenm = LOWERLIMIT;
				if (lenm > UPPERLIMIT) lenm = UPPERLIMIT;
				len[m] = lenm;
				lvari[m] = fabs(lkld2);
#if DISTAN_DEBUG
				if (Debug)
					printf("%3d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f\n",
						l,lenm,len[0],len[1],len[2],lkld1,lkld2);
#endif
				if (fabs(lenold - lenm) < DEPSILON)
					break;
			}
			if (abs(lenm - lenpre) < DEPSILON) {
				cnvrg++;
				if (cnvrg >= 9)
					goto converge;
			} else {
				cnvrg = 0;
			}
			tprobmtrx(len[m], tprob);
			for (k = 0; k < nptrn; k++) {
				j = seq[k];
				for (i = 0; i < Tpmradix; i++)
					mprob[k][i] = tprob[j][i];
			}
		}
#if DISTAN_DEBUG
		if (Debug)
			printf("               %10.5f %10.5f %10.5f\n",
				len[0],len[1],len[2]);
#endif
	}

converge:
	if (Debug) putchar('\n');
	return;
} /* tdistan */


void
tdistan2(seqi, seqj, seqk, seqw, nsite, len, lvari, triprob)
ivector seqi, seqj, seqk, seqw;
int nsite;
double *len, *lvari;
dcube triprob;
{
	int i, j, k, l, m, nl, cnvrg, maxloop = 2;
	double lenm, lenold, lendiff, lenpre, lkl, ld1, ld2, lkld1, lkld2;
	dmattpmty tprob, tdiff1, tdiff2;
	ivector triseq[3], seq;
	dmatrix mprob, cprob, dprob;

	triseq[0] = seqi;
	triseq[1] = seqj;
	triseq[2] = seqk;
	for (m = 0; m < 3; m++) {
		tprobmtrx(len[m], tprob);
		seq = triseq[m];
		mprob = triprob[m];
		for (k = 0; k < nsite; k++) {
			j = seq[k];
			for (i = 0; i < Tpmradix; i++)
				mprob[k][i] = tprob[j][i];
		}
	}
#if DISTAN_DEBUG
	if (Debug)
		printf("               %10.5f %10.5f %10.5f\n", len[0],len[1],len[2]);
#endif
	cnvrg = 0;
	for (nl = 0; nl < 9; nl++) {
		for (m = 0; m < 3; m++) {
			if (m == 0) {
				mprob = triprob[0];
				cprob = triprob[1];
				dprob = triprob[2];
			} else if (m == 1) {
				mprob = triprob[1];
				cprob = triprob[2];
				dprob = triprob[0];
			} else {
				mprob = triprob[2];
				cprob = triprob[0];
				dprob = triprob[1];
			}
			for (k = 0; k < nsite; k++) {
				for (i = 0; i < Tpmradix; i++)
					cprob[k][i] *= dprob[k][i];
			}
			lenm = len[m];
			lenpre = lenm;
			seq = triseq[m];
			for (l = 0; l < maxloop; l++) {
				tdiffmtrx(lenm, tprob, tdiff1, tdiff2);
				lkld1 = lkld2 = 0.0;
				for (k = 0; k < nsite; k++) {
					j = seq[k];
					lkl = ld1 = ld2 = 0.0;
					for (i = 0; i < Tpmradix; i++) {
						lkl += tprob[j][i]  * cprob[k][i];
						ld1 += tdiff1[j][i] * cprob[k][i];
						ld2 += tdiff2[j][i] * cprob[k][i];
					}
					ld1 /= lkl;
					lkld1 += ld1 * (double)seqw[k];
					lkld2 += (ld2 / lkl - ld1 * ld1) * (double)seqw[k];
				}
				lenold = lenm;
				lendiff = -(lkld1 / lkld2);
				if (lendiff > UPPERLIMIT - lenold) {
					if (lenold < UPPERLIMIT / 2.0) lenm = LOWERLIMIT;
					else                       lenm = UPPERLIMIT;
				} else {
					 lenm += lendiff;
				}
				if (lenm < LOWERLIMIT) lenm = LOWERLIMIT;
				if (lenm > UPPERLIMIT) lenm = UPPERLIMIT;
				len[m] = lenm;
				lvari[m] = fabs(lkld2);
#if DISTAN_DEBUG
				if (Debug)
					printf("%3d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f\n",
						l,lenm,len[0],len[1],len[2],lkld1,lkld2);
#endif
				if (fabs(lenold - lenm) < DEPSILON)
					break;
			}
			if (abs(lenm - lenpre) < DEPSILON) {
				cnvrg++;
				if (cnvrg >= 9)
					goto converge;
			} else {
				cnvrg = 0;
			}

		/*	if (abs(lenm - lenpre) < DEPSILON) */
				tprobmtrx(len[m], tprob);
			for (k = 0; k < nsite; k++) {
				j = seq[k];
				for (i = 0; i < Tpmradix; i++)
					mprob[k][i] = tprob[j][i];
			}
		}
#if DISTAN_DEBUG
		if (Debug)
			printf("               %10.5f %10.5f %10.5f\n",
				len[0],len[1],len[2]);
#endif
	}

converge:
	if (Debug) putchar('\n');
	return;
} /* tdistan2 */


void
tridistance(distanmat, seqconint, weight, maxspc, numptrn)
dmatrix distanmat;
imatrix seqconint;
ivector weight;
int maxspc, numptrn;
{
	int i, j, k, kk, maxj, maxj2, maxj3;
	double dist, len[3], lvari[3], maxprob; /* vari */
	ivector seqi, seqj;
	dvector disvec;
	dmatrix probk;
	dcube triprob;

	disvec = new_dvector(Maxspc);
	probk = new_dmatrix(numptrn, Tpmradix);
	triprob = new_dcube(3, numptrn, Tpmradix);

	for (i = 0; i < numptrn; i++) {
		for (j = 0; j < Tpmradix; j++) probk[i][j] = 0.0;
	/*	for (j = 0; j < Tpmradix; j++) probk[i][j] = Freqtpm[j] * 1; */
		for (j = 0; j < maxspc; j++) {
			kk = seqconint[j][i];
			if (kk >= 0) {
				probk[i][kk] += 1.0;
			} else {
				for (k = 0; k < Tpmradix; k++) probk[i][k] += Freqtpm[k];
			}
		}
#if 0
		for (j = 0; j < Tpmradix; j++) probk[i][j] /= maxspc;
#endif
	/*	for (j = 0; j < Tpmradix; j++) probk[i][j] = Freqtpm[j]; */
#if 1
		for (j = 0, maxprob = 0.0; j < Tpmradix; j++) {
			if (probk[i][j] > maxprob) {
				maxj = j;
				maxprob = probk[i][j];
				maxj3 = maxj2 = -1;
			} else if (probk[i][j] == maxprob) {
				maxj3 = maxj2;
				maxj2 = maxj;
				maxj = j;
			}
			probk[i][j] = 0.0;
		}
		if (maxprob == 1.0) {
			for (j = 0; j < Tpmradix; j++) probk[i][j] = Freqtpm[j];
		} else {
			if (maxj3 != -1) {
				probk[i][maxj ] = 0.334;
				probk[i][maxj2] = 0.333;
				probk[i][maxj3] = 0.333;
			} else if (maxj2 != -1) {
				probk[i][maxj ] = 0.5;
				probk[i][maxj2] = 0.5;
			} else {
				probk[i][maxj ] = 1.0;
			}
		}
		if (maxprob <= (maxspc * 0.3)) {
			for (j = 0; j < Tpmradix; j++) probk[i][j] = Freqtpm[j];
		}
#endif
#if 0
		for (j=0;j<Tpmradix;j++) printf("%3.0f",probk[i][j]*100);putchar('\n');
#endif
	}
	for (i = 0; i < maxspc; i++) {
		dist = 10.0;
		pdistan(&dist, seqconint[i], probk, weight, numptrn);
		disvec[i] = dist;
	/*	printf("%20.10f\n", dist); */
	}
	for (i = 0; i < maxspc - 1; i++) {
		seqi = seqconint[i];
		for (j = i+1; j < maxspc; j++) {
			seqj = seqconint[j];
#if 0
			for (k = 0; k < numptrn; k++) {
				for (l = 0; l < Tpmradix; l++) probk[k][l] = 0.0;
				for (l = 0; l < maxspc; l++) {
					if (l == i || l == j) continue;
					kk = seqconint[l][k];
					if (kk >= 0) {
						probk[k][kk] += 1.0;
					} else {
						for (m=0; m<Tpmradix; m++) probk[k][m] += Freqtpm[m];
					}
				}
				for (l = 0; l < Tpmradix; l++) probk[k][l] /= (maxspc - 2);
			}
#endif
			len[0] = (disvec[i] + disvec[j] - distanmat[i][j]) * 0.5;
			len[1] = (disvec[i] - disvec[j] + distanmat[i][j]) * 0.5;
			len[2] = (disvec[j] - disvec[i] + distanmat[i][j]) * 0.5;
			tdistan(seqi, seqj, probk, weight, numptrn, len, lvari, triprob);
			/* vari = sqrt(lvari[0] + lvari[1] + lvari[2]); */
			dist = len[1] + len[2];
		/*	printf("%3d%3d%15.9f%15.9f%15.9f\n", i,j,len[0],len[1],len[2]); */
			if (Info_optn) {
				distanmat[i][j] = len[1];
				distanmat[j][i] = len[2];
			} else {
				distanmat[i][j] = dist;
				distanmat[j][i] = dist;
			}
		}
	}

	free_dcube(triprob);
	free_dmatrix(probk);
	free_dvector(disvec);
} /* tridistance */

#if 0
void
tridistance2(distanmat, seqchar, maxspc, numsite)
dmatrix distanmat;
cmatrix seqchar;
int maxspc, numsite;
{
	int i, j, k, l, m, xi, xj, xk, nsite;
	double dist, vari, len[3], lvari[3];
	dcube tridistan, triprob;
	double *ptr, *nptr;
	ivector seqi, seqj, seqk, seqw;
	cvector seqchi, seqchj, seqchk;
	int gene[TPMRADIX + 1];

	seqi = new_ivector(numsite);
	seqj = new_ivector(numsite);
	seqk = new_ivector(numsite);
	seqw = new_ivector(numsite);
	tridistan = new_dcube(maxspc, maxspc, maxspc);
	triprob = new_dcube(3, numsite, Tpmradix);
	nptr = **tridistan + maxspc*maxspc*maxspc;
	for (ptr = **tridistan; ptr < nptr; ptr++)
		*ptr = 0.0;

	for (i = 0; i < maxspc-2; i++) {
		seqchi = seqchar[i];
		for (j = i+1; j < maxspc-1; j++) {
			seqchj = seqchar[j];
			for (k = j+1; k < maxspc; k++) {
				seqchk = seqchar[k];

				for ( m = 0; m < Tpmradix + 1; m++) gene[m] = 0;
				for ( m = 0; m < numsite; m++) {
					xi = seqchi[m];
					xj = seqchj[m];
					xk = seqchk[m];
					if (xi == xj  && xi == xk) {
						gene[xi]++;
						seqw[m] = FALSE;
					} else if (xi == Tpmradix || xj == Tpmradix ||
							   xk == Tpmradix) {
						seqw[m] = FALSE;
					} else {
						seqw[m] = TRUE;
					}
				}
				for ( m = 0, l = 0; m < numsite; m++) {
					if (seqw[m]) {
						seqi[l] = seqchi[m];
						seqj[l] = seqchj[m];
						seqk[l] = seqchk[m];
						seqw[l] = 1;
						l++;
					}
				}
				for ( m = 0; m < Tpmradix; m++) {
					if (gene[m]) {
						seqi[l] = m;
						seqj[l] = m;
						seqk[l] = m;
						seqw[l] = gene[m];
						l++;
					}
				}
				nsite = l;
#ifdef DISDEBUG
				for (m=0;m<nsite;m++) putchar(int2ami(seqi[m])); putchar('\n');
				for (m=0;m<nsite;m++) putchar(int2ami(seqj[m])); putchar('\n');
				for (m=0;m<nsite;m++) putchar(int2ami(seqk[m])); putchar('\n');
				for (m=0;m<nsite;m++) printf("%d",seqw[m]); putchar('\n');
#endif
				len[0] = (distanmat[i][j]+distanmat[i][k]-distanmat[j][k])/2.0;
				len[1] = (distanmat[j][i]+distanmat[j][k]-distanmat[i][k])/2.0;
				len[2] = (distanmat[k][i]+distanmat[k][j]-distanmat[i][j])/2.0;
				tdistan2(seqi, seqj, seqk, seqw, nsite, len, lvari, triprob);
				vari = sqrt(lvari[0] + lvari[1] + lvari[2]);
				if (Varia_optn) {
					tridistan[i][j][k] = (len[0] + len[1]) / vari;
					tridistan[i][k][j] = (len[0] + len[2]) / vari;
					tridistan[j][i][k] = (len[1] + len[0]) / vari;
					tridistan[j][k][i] = (len[1] + len[2]) / vari;
					tridistan[k][i][j] = (len[2] + len[0]) / vari;
					tridistan[k][j][i] = (len[2] + len[1]) / vari;
				} else {
					tridistan[i][j][k] = (len[0] + len[1]);
					tridistan[i][k][j] = (len[0] + len[2]);
					tridistan[j][i][k] = (len[1] + len[0]);
					tridistan[j][k][i] = (len[1] + len[2]);
					tridistan[k][i][j] = (len[2] + len[0]);
					tridistan[k][j][i] = (len[2] + len[1]);
				}
			}
		}
	}

	if (Debug)
	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < maxspc; j++) {
			for (k = 0; k < maxspc; k++) {
				printf("%5.1f", tridistan[i][j][k]);
			}
			printf("\n");
		}
		printf("\n");
		printf("\n");
	}

	if (Debug)
	for (i = 0; i < maxspc; i++) {
		printf("%s\n", Identif[i]);
		for (j = 0; j < maxspc; j++) {
			for (k = 0; k < maxspc; k++) {
				printf("%10.6f", tridistan[i][j][k]);
				/* if ((k+1)%5 == 0)
					printf("\n"); */
			}
			printf("\n");
		}
		printf("\n");
	}

	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < maxspc; j++) {
			for (k = 0, dist = 0.0; k < maxspc; k++)
				dist += tridistan[i][j][k];
			dist /= (maxspc - 2);
			distanmat[i][j] = dist;
		}
	}

	free_dcube(triprob);
	free_dcube(tridistan);
	free_ivector(seqw);
	free_ivector(seqk);
	free_ivector(seqj);
	free_ivector(seqi);
} /* tridistance2 */
#endif

void
putdistance(identif, sciname, engname, distanmat, maxspc)
cmatrix identif;
cmatrix sciname;
cmatrix engname;
dmatrix distanmat;
int maxspc;
{
	int i, j, k, numk, maxk;

	if (!Info_optn) {

	for (i = 0; i < maxspc; i++) {
		printf("%s", identif[i]);
		if (sciname[i] != '\0') printf(" %s", sciname[i]);
		if (engname[i] != '\0') printf(" %s", engname[i]);
		putchar('\n');
		for (j = 0; j < maxspc; j++) {
			if (!Varia_optn) {
				printf("%15.12f", distanmat[i][j] / 100.0);
			} else {
				if (i < j) printf("%15.12f", distanmat[i][j] / 100.0);
				else       printf("%15.12f", distanmat[i][j] / 10000.0);
			}
			if ((j+1)%5 == 0) putchar('\n');
		}
		if (j%5 != 0) putchar('\n');
	}

	} else { /* Info_optn */

		numk = 23;
		for (k = 0; maxk = k + numk, k < maxspc; k += numk) {
			if (maxk > maxspc) maxk = maxspc;
			printf("\n%4s%-6s", "sub/","100");
			printf("%3d", k + 1);
			for (j = k + 1; j < maxk; j++) printf("%3d", (j+1) % 100);
			putchar('\n');
			printf("%4s%6s", "","");
			for (j = k; j < maxk; j++) printf("%3.2s", Identif[j]);
			putchar('\n');
			for (i = 0; i < maxspc; i++) {
				printf("%-4d%-6.6s", i + 1, Identif[i]);
				for (j = k; j < maxk; j++) {
					if (i != j)
						printf("%3.0f", distanmat[i][j]);
					else
						printf("%3.2s", Identif[i]);
				}
				putchar('\n');
			}
		}

	}
} /* putdistance */

void
checkseq(seqconint, maxspc, numptrn)
imatrix seqconint;
int maxspc;
int numptrn;
{
	int i, j, k;
	boolean same, sameotu;

	sameotu = FALSE;
	for (i = 0; i < maxspc - 1; i++) {
		for (j = i + 1; j < maxspc; j++) {
			for (k = 0, same = TRUE; k < numptrn; k++) {
				if (seqconint[i][k] != seqconint[j][k]) {
					same = FALSE;
					break;
				}
			}
			if (same) {
				fprintf(stderr,
					"ERROR: %s[%d] & %s[%d] are same sequences!\n",
					Identif[i], i+1, Identif[j], j+1);
				sameotu = TRUE;
			}
		}
	}
	if (sameotu) {
		fprintf(stderr,
			"ERROR: Please remove same sequences except for one!\n");
		/* exit(1); */
	}
} /* checkseq */
