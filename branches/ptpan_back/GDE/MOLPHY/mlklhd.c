/*
 * mlklhd.c   Adachi, J.   1996.03.02
 * Copyright (C) 1992-1996 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "protml.h"

#define RELIDEBUG 0
#define QRELIDEBUG 0
#define MLEDEBUG 0
#define MLEDEBUG2 0
#define PRBCHECK 0


double
probnormal(z)  /* lower probability of normal distribution */
double z;
{
	int i;
	double z2, prev, p, t;

	z2 = z * z;
	t = p = z * exp(-0.5 * z2) / sqrt(2 * 3.14159265358979323846264);
	for (i = 3; i < 200; i += 2) {
		prev = p;  t *= z2 / i;  p += t;
		if (p == prev) return 0.5 + p;
	}
	return (z > 0);
} /* probnormal */


double
uprobnormal(z)  /* upper probability of normal distribution */
double z;
{
	return 1 - probnormal(z);
} /* uprobnormal */


#ifdef DEBUG
static void
prprob(xprob)
dmatrix xprob;
{
	int i, j;

	for (i = 0; i < Numptrn; i++) {
		for (j = 0; j < Tpmradix; j++) printf(" %3.0f", xprob[i][j]*100.0);
		putchar('\n');
	}
} /* prprob */
#endif


void 
copypart1(op, cp)
Node *op, *cp;
{
	/* op (o) opb  <---  cpb (c) cp */
	int i;
	dvector opb, cpb;

	opb = *(op->iprob);
	cpb = *(cp->iprob);
	for (i = 0; i < Numptrn; i++) {
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
#ifndef NUC
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
		*opb++ = *cpb++;
#endif	/* NUC */
	}
} /*_ copypart1 */


void 
prodpart1(op, cp)
Node *op, *cp;
{
	/* op (o) opb  <---  cpb (c) cp */
	int i;
	dvector opb, cpb;

	opb = *(op->iprob);
	cpb = *(cp->iprob);
	for (i = 0; i < Numptrn; i++) {
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
#ifndef NUC
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
		*opb++ *= *cpb++;
#endif	/* NUC */
	}
	/*	if (Debug) prprob(op->iprob); */
} /*_ prodpart1 */


void 
prodpart(op)
Node *op;
{
	/*                  (c) cp
	 *      opb  <----  cpb   
	 *   op (o)    |
	 *             |    (c) cp'
	 *              --  cpb'   
	 *
	 *                  (c) last
	 */
	Node *cp;
	int i;
	dvector opb, cpb;

	cp = op;
	while (cp->isop->isop != op) {
		cp = cp->isop;
		opb = *(op->iprob);
		cpb = *(cp->iprob);
		for (i = 0; i < Numptrn; i++) {
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
#ifndef NUC
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
			*opb++ *= *cpb++;
#endif		/* NUC */
		}
	}
	/*	if (Debug) prprob(op->iprob); */
} /*_ prodpart */


void 
partilkl(op)
Node *op;
{
	/*                   (i) iprob
	 *     op            /    /
	 *  --(a)----------(d)   /
	 *    oprob <------------             
	 */
	int i, k;
	double sum;
	dmattpmty tprob;
	dmatrix oprob, cprob;
	dvector opb, cpb, cpb2, tpb;

	tprobmtrx(op->length, tprob);
	oprob = op->iprob;
	cprob = op->kinp->isop->iprob;
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
		cpb = cprob[k];
		tpb = *tprob;
#ifndef NUC
		for (i = 0; i < Tpmradix; i++) {
			cpb2 = cpb;
			sum  = *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			sum += *tpb++ * *cpb2++;
			opb[i] = sum;
		}
#else	/* NUC */
		opb[0] = tpb[ 0] * cpb[0] + tpb[ 1] * cpb[1]
			   + tpb[ 2] * cpb[2] + tpb[ 3] * cpb[3];
		opb[1] = tpb[ 4] * cpb[0] + tpb[ 5] * cpb[1]
			   + tpb[ 6] * cpb[2] + tpb[ 7] * cpb[3];
		opb[2] = tpb[ 8] * cpb[0] + tpb[ 9] * cpb[1]
		       + tpb[10] * cpb[2] + tpb[11] * cpb[3];
		opb[3] = tpb[12] * cpb[0] + tpb[13] * cpb[1]
			   + tpb[14] * cpb[2] + tpb[15] * cpb[3];
#endif	/* NUC */
	}
	/*	if (Debug) prprob(oprob); */
} /*_ partilkl */


void 
partelkl(op)
Node *op;
{
	/*     op
	 *  --(a)----------(d)
	 *    oprob <---- dseqi
	 */
	int i, k;
	dmattpmty tprob;
	dmatrix oprob;
	ivector dseqi;
	dvector opb, tbp;

	tprobmtrxt(op->length, tprob);
	oprob = op->iprob;
	dseqi = op->kinp->eprob;
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
		if ((i = dseqi[k]) >= 0) {
			tbp = tprob[i];
			opb[ 0] = tbp[ 0];
			opb[ 1] = tbp[ 1];
			opb[ 2] = tbp[ 2];
			opb[ 3] = tbp[ 3];
#ifndef		NUC
			opb[ 4] = tbp[ 4];
			opb[ 5] = tbp[ 5];
			opb[ 6] = tbp[ 6];
			opb[ 7] = tbp[ 7];
			opb[ 8] = tbp[ 8];
			opb[ 9] = tbp[ 9];
			opb[10] = tbp[10];
			opb[11] = tbp[11];
			opb[12] = tbp[12];
			opb[13] = tbp[13];
			opb[14] = tbp[14];
			opb[15] = tbp[15];
			opb[16] = tbp[16];
			opb[17] = tbp[17];
			opb[18] = tbp[18];
			opb[19] = tbp[19];
#endif		/* NUC */
		} else {
			opb[ 0] = 1.0;
			opb[ 1] = 1.0;
			opb[ 2] = 1.0;
			opb[ 3] = 1.0;
#ifndef		NUC
			opb[ 4] = 1.0;
			opb[ 5] = 1.0;
			opb[ 6] = 1.0;
			opb[ 7] = 1.0;
			opb[ 8] = 1.0;
			opb[ 9] = 1.0;
			opb[10] = 1.0;
			opb[11] = 1.0;
			opb[12] = 1.0;
			opb[13] = 1.0;
			opb[14] = 1.0;
			opb[15] = 1.0;
			opb[16] = 1.0;
			opb[17] = 1.0;
			opb[18] = 1.0;
			opb[19] = 1.0;
#endif		/* NUC */
		}
	}
} /*_ partelkl */


void 
partelkl2(op)
Node *op;
{
	/*     op
	 *  --(a)----------(d)
	 *    oprob <---- dseqi
	 */
	int j, k;
	dmattpmty tprob;
	dmatrix oprob;
	ivector dseqi;
	dvector opb;

	tprobmtrx(op->length, tprob);
	oprob = op->iprob;
	dseqi = op->kinp->eprob;
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
		if ((j = dseqi[k]) >= 0) {
			opb[ 0] = tprob[ 0][j];
			opb[ 1] = tprob[ 1][j];
			opb[ 2] = tprob[ 2][j];
			opb[ 3] = tprob[ 3][j];
#ifndef		NUC
			opb[ 4] = tprob[ 4][j];
			opb[ 5] = tprob[ 5][j];
			opb[ 6] = tprob[ 6][j];
			opb[ 7] = tprob[ 7][j];
			opb[ 8] = tprob[ 8][j];
			opb[ 9] = tprob[ 9][j];
			opb[10] = tprob[10][j];
			opb[11] = tprob[11][j];
			opb[12] = tprob[12][j];
			opb[13] = tprob[13][j];
			opb[14] = tprob[14][j];
			opb[15] = tprob[15][j];
			opb[16] = tprob[16][j];
			opb[17] = tprob[17][j];
			opb[18] = tprob[18][j];
			opb[19] = tprob[19][j];
#endif		/* NUC */
		} else {
			opb[ 0] = 1.0;
			opb[ 1] = 1.0;
			opb[ 2] = 1.0;
			opb[ 3] = 1.0;
#ifndef		NUC
			opb[ 4] = 1.0;
			opb[ 5] = 1.0;
			opb[ 6] = 1.0;
			opb[ 7] = 1.0;
			opb[ 8] = 1.0;
			opb[ 9] = 1.0;
			opb[10] = 1.0;
			opb[11] = 1.0;
			opb[12] = 1.0;
			opb[13] = 1.0;
			opb[14] = 1.0;
			opb[15] = 1.0;
			opb[16] = 1.0;
			opb[17] = 1.0;
			opb[18] = 1.0;
			opb[19] = 1.0;
#endif		/* NUC */
		}
	}
} /*_ partelkl */


void 
initpartlkl(tr)
Tree *tr;
{
	Node *cp, *rp;

	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			/* if (Debug) printf("mle %3d %2d\n", cp->num+1, cp->descen); */
			cp = cp->kinp; /* not descen */
			partelkl(cp);
		} else { /* internal node */
			/* if (Debug) printf("mli %3d %2d\n", cp->num+1, cp->descen); */
			if (!cp->descen) {
				prodpart(cp->kinp->isop);
				partilkl(cp);
			}
		}
	} while (cp != rp);
} /*_ initpartlkl */


void 
regupartlkl(tr)
Tree *tr;
{
	/* work on star decomposition */
	Node *cp, *rp;
	int i, j, prmax;
	double pr;
	dvector prbcv, prbkv;
	dmatrix prbc, prbk;
	dmattpmty tprob;

	cp = rp = tr->rootp;
	do {
		cp = cp->isop;
		if (cp->kinp->isop != NULL) { /* internal node */
			prbc = cp->iprob;
			prbk = cp->kinp->isop->iprob;
			printf("length = %12.8f, %12.8f\n", cp->length, cp->kinp->length);
			tprobmtrx(cp->length, tprob);
			for (i = 0; i < Numptrn; i++) {
				prbcv = prbc[i];
				prbkv = prbk[i];
				for (j = 1, pr = prbkv[0], prmax = 0; j < Tpmradix; j++) {
					if (prbkv[j] > pr) {
						pr = prbkv[j];
						prmax = j;
					}
				}
				for (j = 0; j < Tpmradix; j++) {
					prbcv[j] = tprob[j][prmax];
				}
			}
		}
	} while (cp != rp);
} /*_ regupartlkl */


void 
mlibranch(op, eps, nloop)
Node *op; /* op->kinp == descen */
double eps;
int nloop;
{
	/*                       (i) cprob
	 *      op    dist      /
	 *      (a)----------(b)
	 *     / oprob'
	 *  (i)
	 *  oprob
	 */
	boolean conv;
	int i, j, k, it;
	double sumlk, sumd1, sumd2, lkld1, lkld2, prod1, prod2, vari;
	double dist, distold, distdiff, distpre, coef, slk, sd1, sd2;
	/* double d, d1, d2, diff, dis; */
	dmattpmty tprob, tdif1, tdif2;
	dmatrix oprob, cprob;
	dvector tpb, td1, td2, opb, cpb;
#ifdef NUC
	double cpb0, cpb1, cpb2, cpb3;
#endif /* NUC */

	oprob = op->isop->iprob;
	cprob = op->kinp->isop->iprob;
	distpre = dist = op->length;
/*
	if (1) {
		for (k = 0, diff = 0.0; k < Numptrn; k++) {
			opb = oprob[k];
			cpb = cprob[k];
			for (i = 0, d1 = d2 = 0.0; i < Tpmradix; i++) {
				for (j = 0; j < Tpmradix; j++) {
					d1 += opb[i] * cpb[j];
					if (i != j) d2 += Freqtpm[i] * opb[i] * cpb[j];
				}
			}
			d = d2 / d1;
			diff += d * Weight[k];
		}
		if (diff < 1) diff = 1.0;
		if (diff >= Maxsite) diff = Maxsite - 1.0;
		dis = -log(1.0 - diff / Maxsite)*100.0;
	}
*/
	for (it = 0, conv = FALSE, coef = INITCOEFMLE; it < nloop; it++) {
		tdiffmtrx(dist, tprob, tdif1, tdif2);
		lkld1 = lkld2 = 0.0;
		for (k = 0; k < Numptrn; k++) {
			sumlk = sumd1 = sumd2 = 0.0;
			opb = oprob[k];
			cpb = cprob[k];
#ifdef NUC
			cpb0 = cpb[0]; cpb1 = cpb[1]; cpb2 = cpb[2]; cpb3 = cpb[3];
#endif		/* NUC */
			for (i = 0; i < Tpmradix; i++) {
				tpb = tprob[i];
				td1 = tdif1[i];
				td2 = tdif2[i];
				prod1 = Freqtpm[i] * opb[i];
#ifndef NUC
				slk = sd1 = sd2 = 0.0;
				for (j = 0; j < Tpmradix; j++) {
					prod2 = cpb[j];
					slk += prod2 * tpb[j];
					sd1 += prod2 * td1[j];
					sd2 += prod2 * td2[j];
				}
				sumlk += prod1 * slk;
				sumd1 += prod1 * sd1;
				sumd2 += prod1 * sd2;
#else			/* NUC */
				slk = cpb0*tpb[0] + cpb1*tpb[1] + cpb2*tpb[2] + cpb3*tpb[3];
				sd1 = cpb0*td1[0] + cpb1*td1[1] + cpb2*td1[2] + cpb3*td1[3];
				sd2 = cpb0*td2[0] + cpb1*td2[1] + cpb2*td2[2] + cpb3*td2[3];
				sumlk += prod1 * slk;
				sumd1 += prod1 * sd1;
				sumd2 += prod1 * sd2;
#endif			/* NUC */
			}
			sumd1 /= sumlk;
			lkld1 += sumd1 * Weight[k];
			lkld2 += (sumd2 / sumlk - sumd1 * sumd1) * Weight[k];
		}

		distold = dist;
		distdiff = - (lkld1 / lkld2);
		if (lkld1 > 0) { /* NR internal */
			dist += distdiff;
			coef  = INITCOEFMLE;
			conv  = TRUE;
		} else {
			if (lkld2 < 0) {
				if (dist + distdiff > dist * coef) {
					dist += distdiff;
					coef  = INITCOEFMLE;
					conv  = TRUE;
				} else {
					if (dist < 1.0) {
						dist = Llimit;
						coef = INITCOEFMLE;
					} else {
						dist *= coef;
						coef *= INITCOEFMLE;
					}
					conv  = FALSE;
				}
			} else {
				if (dist < 1.0) {
					dist = Llimit;
					coef = INITCOEFMLE;
				} else {
					dist *= coef;
					coef *= INITCOEFMLE;
				}
				conv  = FALSE;
			}
		}
		if (fabs(lkld1) > 5.0) conv = FALSE;
		if (dist == distold) conv = TRUE;
		if (dist < Llimit) { dist = Llimit; conv = TRUE; }
		if (dist > Ulimit) { dist = Ulimit; conv = TRUE; }
#if		MLEDEBUG2
		printf("mli%3d%3d %7.3f %7.3f %9.4f %9.4f %12.5f %9.3f\n",
			op->num+1,it+1,dist,distold, dist-distold,distdiff,lkld1,lkld2);
#endif
		if (conv && fabs(distold - dist) < eps) break;
	}
	op->kinp->length = dist;
	op->length = dist;
	vari = 1.0 / fabs(lkld2);
	op->lklhdl = vari;
	if (Debug)
		printf("mli%4d%3d%8.3f%8.3f%12.7f%7.2f%14.6f%14.3f\n",
		op->num+1,it+1,dist,distpre,distpre-dist, sqrt(vari),lkld1,lkld2);

	if (distold != dist) tprobmtrx(dist, tprob);
	oprob = op->iprob;
#ifdef NUC
		tpb = tprob[0];
#endif	/* NUC */
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
		cpb = cprob[k];
#ifndef NUC
		for (i = 0; i < Tpmradix; i++) {
			tpb = tprob[i];
			for (j = 0, sumlk = 0.0; j < Tpmradix; j++)
				sumlk += tpb[j] * cpb[j];
			opb[i] = sumlk;
		}
#else	/* NUC */
		cpb0 = cpb[0]; cpb1 = cpb[1]; cpb2 = cpb[2]; cpb3 = cpb[3];
		opb[0] = tpb[ 0]*cpb0 + tpb[ 1]*cpb1 + tpb[ 2]*cpb2 + tpb[ 3]*cpb3;
		opb[1] = tpb[ 4]*cpb0 + tpb[ 5]*cpb1 + tpb[ 6]*cpb2 + tpb[ 7]*cpb3;
		opb[2] = tpb[ 8]*cpb0 + tpb[ 9]*cpb1 + tpb[10]*cpb2 + tpb[11]*cpb3;
		opb[3] = tpb[12]*cpb0 + tpb[13]*cpb1 + tpb[14]*cpb2 + tpb[15]*cpb3;
#endif	/* NUC */
	}
} /*_ mlibranch */


void 
mlebranch(op, eps, nloop)
Node *op; /* op->kinp == descen */
double eps;
int nloop;
{
	/*      op    dist
	 *      (a)----------(b)
	 *     / oprob'     dseqi
	 *  (i)
	 *  oprob
	 */
	boolean conv;
	int i, j, k, it;
	double sumlk, sumd1, sumd2, lkld1, lkld2, prod, vari;
	double dist, distold, distdiff, distpre, coef;
	dmattpmty tprob, tdif1, tdif2;
	dmatrix oprob;
	ivector dseqi;
	dvector opb, tpb, td1, td2;
#ifdef NUC
	double pn0, pn1, pn2, pn3;
#endif /* NUC */

	oprob = op->isop->iprob;
	dseqi = op->kinp->eprob;
	distpre = dist = op->length;

	for (it = 0, conv = FALSE, coef = INITCOEFMLE; it < nloop; it++) {
		tdiffmtrx(dist, tprob, tdif1, tdif2);
		lkld1 = lkld2 = 0.0;
		for (k = 0; k < Numptrn; k++) {
			if ((i = dseqi[k]) >= 0) {
				opb = oprob[k];
				tpb = tprob[i]; td1 = tdif1[i]; td2 = tdif2[i];
#ifndef NUC
				sumlk = sumd1 = sumd2 = 0.0;
				for (j = 0; j < Tpmradix; j++) {
					prod = opb[j];
					sumlk += prod * tpb[j];
					sumd1 += prod * td1[j];
					sumd2 += prod * td2[j];
				}
#else			/* NUC */
				pn0 = opb[0];
				pn1 = opb[1];
				pn2 = opb[2];
				pn3 = opb[3];
				sumlk = pn0 * tpb[0] + pn1 * tpb[1]
					  + pn2 * tpb[2] + pn3 * tpb[3];
				sumd1 = pn0 * td1[0] + pn1 * td1[1]
					  + pn2 * td1[2] + pn3 * td1[3];
				sumd2 = pn0 * td2[0] + pn1 * td2[1]
					  + pn2 * td2[2] + pn3 * td2[3];
#endif			/* NUC */
				sumd1 /= sumlk;
				lkld1 += sumd1 * Weight[k];
				lkld2 += (sumd2 / sumlk - sumd1 * sumd1) * Weight[k];
			}
		}

		distold = dist;
		distdiff = - (lkld1 / lkld2);
		if (lkld1 > 0) { /* NR external */
			dist += distdiff;
			coef  = INITCOEFMLE;
			conv  = TRUE;
		} else {
			if (lkld2 < 0) {
				if (dist + distdiff > dist * coef) {
					dist += distdiff;
					coef  = INITCOEFMLE;
					conv  = TRUE;
				} else {
					if (dist < 0.2) {
						dist = LOWERLIMIT;
						coef = INITCOEFMLE;
					} else {
						dist *= coef;
						coef *= INITCOEFMLE;
					}
					conv  = FALSE;
				}
			} else {
				if (dist < 0.2) {
					dist = LOWERLIMIT;
					coef = INITCOEFMLE;
				} else {
					dist *= coef;
					coef *= INITCOEFMLE;
				}
				conv  = FALSE;
			}
		}
		if (fabs(lkld1) > 5.0) conv = FALSE;
		if (dist == distold) conv = TRUE;
		if (dist < LOWERLIMIT) { dist = LOWERLIMIT; conv = TRUE; }
		if (dist > Ulimit) { dist = Ulimit; conv = TRUE; }
#if		MLEDEBUG2
		 printf("mle%3d%3d %7.3f %7.3f %9.4f %9.4f %12.5f %9.3f\n",
			op->num+1,it+1,dist,distold, dist-distold,distdiff,lkld1,lkld2);
#endif
		if (conv && fabs(distold - dist) < eps) break;
	}
	op->kinp->length = dist;
	op->length = dist;
	vari = 1.0 / fabs(lkld2);
	op->lklhdl = vari;
	if (Debug)
		printf("mle%4d%3d%8.3f%8.3f%12.7f%7.2f%14.6f%14.3f\n",
		op->num+1,it+1,dist,distpre,distpre-dist,sqrt(vari),lkld1,lkld2);

	tprobmtrxt(dist, tprob); /* if (distold != dist) */
	oprob = op->iprob;
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
#ifndef NUC
		if ((i = dseqi[k]) >= 0) {
			tpb = tprob[i];
			for (j = 0; j < Tpmradix; j++)
				opb[j] = tpb[j];
		} else {
			for (j = 0; j < Tpmradix; j++)
				opb[j] = 1.0; /* !? */
		}
#else	/* NUC */
		if ((i = dseqi[k]) >= 0) {
			tpb = tprob[i];
			opb[0] = tpb[0];
			opb[1] = tpb[1];
			opb[2] = tpb[2];
			opb[3] = tpb[3];
		} else {
			opb[0] = 1.0;
			opb[1] = 1.0;
			opb[2] = 1.0;
			opb[3] = 1.0;
		}
#endif	/* NUC */
	}
} /*_ mlebranch */


void 
mlebranch2(op, eps, nloop)
Node *op; /* op->kinp == descen */
double eps;
int nloop;
{
	/*      op    dist
	 *      (a)----------(b)
	 *     / oprob'     dseqi
	 *  (i)
	 *  oprob
	 */
	boolean conv;
	int i, j, k, it;
	double sumlk, sumd1, sumd2, lkld1, lkld2, prod, vari;
	double dist, distold, distdiff, distpre, coef;
	dmattpmty tprob, tdif1, tdif2;
	dmatrix oprob;
	ivector dseqi;
	dvector opb;
#ifdef NUC
	dvector tpb, td1, td2;
	double pn0, pn1, pn2, pn3;
#endif /* NUC */

	oprob = op->isop->iprob;
	dseqi = op->kinp->eprob;
	distpre = dist = op->length;

	for (it = 0, conv = FALSE, coef = INITCOEFMLE; it < nloop; it++) {
		tdiffmtrx(dist, tprob, tdif1, tdif2);
#ifdef NUC
		tpb = *tprob; td1 = *tdif1; td2 = *tdif2;
#endif	/* NUC */
		lkld1 = lkld2 = 0.0;
		for (k = 0; k < Numptrn; k++) {
			if ((j = dseqi[k]) >= 0) {
				opb = oprob[k];
#ifndef NUC
				sumlk = sumd1 = sumd2 = 0.0;
				for (i = 0; i < Tpmradix; i++) {
					prod = Freqtpm[i] * opb[i];
					sumlk += prod * tprob[i][j];
					sumd1 += prod * tdif1[i][j];
					sumd2 += prod * tdif2[i][j];
				}
#else			/* NUC */
				pn0 = Freqtpm[0] * opb[0];
				pn1 = Freqtpm[1] * opb[1];
				pn2 = Freqtpm[2] * opb[2];
				pn3 = Freqtpm[3] * opb[3];
				sumlk = pn0 * tpb[j  ] + pn1 * tpb[j+ 4]
					  + pn2 * tpb[j+8] + pn3 * tpb[j+12];
				sumd1 = pn0 * td1[j  ] + pn1 * td1[j+ 4]
					  + pn2 * td1[j+8] + pn3 * td1[j+12];
				sumd2 = pn0 * td2[j  ] + pn1 * td2[j+ 4]
					  + pn2 * td2[j+8] + pn3 * td2[j+12];
#endif			/* NUC */
				sumd1 /= sumlk;
				lkld1 += sumd1 * Weight[k];
				lkld2 += (sumd2 / sumlk - sumd1 * sumd1) * Weight[k];
			}
		}

		distold = dist;
		distdiff = - (lkld1 / lkld2);
		if (lkld1 > 0) { /* NR external */
			dist += distdiff;
			coef  = INITCOEFMLE;
			conv  = TRUE;
		} else {
			if (lkld2 < 0) {
				if (dist + distdiff > dist * coef) {
					dist += distdiff;
					coef  = INITCOEFMLE;
					conv  = TRUE;
				} else {
					if (dist < 0.2) {
						dist = LOWERLIMIT;
						coef = INITCOEFMLE;
					} else {
						dist *= coef;
						coef *= INITCOEFMLE;
					}
					conv  = FALSE;
				}
			} else {
				if (dist < 0.2) {
					dist = LOWERLIMIT;
					coef = INITCOEFMLE;
				} else {
					dist *= coef;
					coef *= INITCOEFMLE;
				}
				conv  = FALSE;
			}
		}
		if (fabs(lkld1) > 5.0) conv = FALSE;
		if (dist == distold) conv = TRUE;
		if (dist < LOWERLIMIT) { dist = LOWERLIMIT; conv = TRUE; }
		if (dist > Ulimit) { dist = Ulimit; conv = TRUE; }
#if		MLEDEBUG2
		 printf("mle%3d%3d %7.3f %7.3f %9.4f %9.4f %12.5f %9.3f\n",
			op->num+1,it+1,dist,distold, dist-distold,distdiff,lkld1,lkld2);
#endif
		if (conv && fabs(distold - dist) < eps) break;
	}
	op->kinp->length = dist;
	op->length = dist;
	vari = 1.0 / fabs(lkld2);
	op->lklhdl = vari;
	if (Debug)
		printf("mle%4d%3d%8.3f%8.3f%12.7f%7.2f%14.6f%14.3f\n",
		op->num+1,it+1,dist,distpre,distpre-dist,sqrt(vari),lkld1,lkld2);

	if (distold != dist) tprobmtrx(dist, tprob);
	oprob = op->iprob;
#ifdef NUC
	tpb = *tprob;
#endif /* NUC */
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
#ifndef NUC
		if ((j = dseqi[k]) >= 0) {
			for (i = 0; i < Tpmradix; i++)
				opb[i] = tprob[i][j];
		} else {
			for (i = 0; i < Tpmradix; i++)
				opb[i] = 1.0; /* !? */
		}
#else	/* NUC */
		if ((j = dseqi[k]) >= 0) {
			opb[0] = tpb[j];
			opb[1] = tpb[j+4];
			opb[2] = tpb[j+8];
			opb[3] = tpb[j+12];
		} else {
			opb[0] = 1.0;
			opb[1] = 1.0;
			opb[2] = 1.0;
			opb[3] = 1.0;
		}
#endif	/* NUC */
	}
} /*_ mlebranch */


void 
evallkl(op)
Node *op;
{
	/*      op
	 *      (a)----------( )
	 *     / oprob
	 *  (i)
	 *  iprob
	 */
	int i, k;
	double sumlk, lklhd;
	dvector opb, ipb;
	dmatrix oprob, iprob;

	oprob = op->iprob;
	iprob = op->isop->iprob;

	for (k = 0, lklhd = 0.0; k < Numptrn; k++) {
		opb = oprob[k];
		ipb = iprob[k];
		for (i = 0, sumlk = 0.0; i < Tpmradix; i++) {
			sumlk += Freqtpm[i] * opb[i] * ipb[i];
		}
		sumlk = log(sumlk);
		Alklptrn[k] = sumlk;
		lklhd += sumlk * Weight[k];
		/* printf("%3d %10.5f\n", k, sumlk); */
	}
	/* printf("branch:%3d ln L: %12.5f\n", op->kinp->num+1,lklhd); */
	Ctree->lklhd = lklhd;

} /*_ evallkl */


Node *
mlikelihood(tr)
Tree *tr;
{
	Node *cp, *rp;
	int l, nconv, nconv2;
	double eps, lendiff;
#if MLEDEBUG
	int i;
#endif

	nconv = nconv2 = 0;
	Converg = FALSE;
	for (l = 0; l < MAXIT; l++) {
		Numit = l + 1;
		if      (l == 0) eps = 1.0;
		else if (l == 1) eps = 0.2;
		else if (l == 2) eps = 0.1;
		else             eps = Epsilon;
		if (Debug) printf("ml:%3d\n", l);
#if MLEDEBUG2
		printf("ml:%3d\n", l+1);
#endif
#if MLEDEBUG
		if (l == 0) putchar('\n');
		printf("%2d", l+1);
		for (i = 0; i < Numspc; i++)
			printf("%5.0f",tr->ebrnchp[i]->length*100);
		for (i = 0; i < Numibrnch; i++)
			printf("%5.0f",tr->ibrnchp[i]->length*100);
		printf(" %d\n", nconv);
#endif

		cp = rp = tr->rootp;
		do {
			cp = cp->isop->kinp;
			prodpart(cp->kinp->isop);
			if (cp->isop == NULL) { /* external node */
				/* if (Debug) printf("mle %3d%3d\n",cp->num+1,cp->descen); */
				cp = cp->kinp; /* not descen */
				lendiff = cp->length;
				mlebranch(cp, eps, 5);
				/* evallkl(cp); */
				lendiff = fabs(lendiff - cp->length);
				lendiff < Epsilon ? (nconv++)  : (nconv = 0);
				lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
			} else { /* internal node */
				/* if (Debug) printf("mli %3d%3d\n",cp->num+1,cp->descen); */
				if (cp->descen) {
					partilkl(cp);
					/* mlibranch(cp, eps, 5); */
				} else {
					lendiff = cp->length;
					mlibranch(cp, eps, 5);
					/* evallkl(cp); */
					lendiff = fabs(lendiff - cp->length);
					lendiff < Epsilon ? (nconv++)  : (nconv = 0);
					lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
				}
			}
			/* if (nconv >= Numbrnch) goto convergence; */
		} while (cp != rp);
		if (nconv >= Numbrnch) goto convergence;

	}
	if (nconv2 >= Numbrnch) Converg = 2;
	evallkl(cp);
	return rp;

convergence:
	Converg = TRUE;
	evallkl(cp);
	return cp;
} /*_ mlikelihood */


void 
ribranch(op)
Node *op; /* op->kinp == descen */
{
	/*                       (i) cprob
	 *      op    dist      /
	 *      (a)----------(b)
	 *     / oprob'
	 *  (i)
	 *  oprob
	 */
	int i, j, k;
	double dist, sumlk, sumlk0, lsumlk, lsumlk0, slk, lklhd, lklhd0;
	double nn1, suml1, suml2, ldiff, sdlkl, dlkl, rel;
	dmattpmty tprob;
	dmatrix oprob, cprob;
	dvector tpb, opb, cpb;
#ifdef NUC
	double cpb0, cpb1, cpb2, cpb3;
#endif /* NUC */

	oprob = op->isop->iprob;
	cprob = op->kinp->isop->iprob;
	dist = op->length;

	tprobmtrx(dist, tprob);
	lklhd = lklhd0 = suml1 = suml2 = 0.0;
	for (k = 0; k < Numptrn; k++) {
		sumlk = sumlk0 = 0.0;
		opb = oprob[k];
		cpb = cprob[k];
#ifdef NUC
		cpb0 = cpb[0]; cpb1 = cpb[1]; cpb2 = cpb[2]; cpb3 = cpb[3];
#endif		/* NUC */
		for (i = 0; i < Tpmradix; i++) {
			tpb = tprob[i];
#ifndef NUC
			for (j = 0, slk = 0.0; j < Tpmradix; j++) {
				slk  += cpb[j] * tpb[j];
			}
#else			/* NUC */
			slk  = cpb0*tpb[0] + cpb1*tpb[1] + cpb2*tpb[2] + cpb3*tpb[3];
#endif			/* NUC */
			sumlk  += Freqtpm[i] * opb[i] * slk;
			sumlk0 += Freqtpm[i] * opb[i] * cpb[i];
		}
		lsumlk  = log(sumlk);
		lsumlk0 = log(sumlk0);
		lklhd  += lsumlk  * Weight[k];
		lklhd0 += lsumlk0 * Weight[k];
		ldiff = lsumlk - lsumlk0;
		suml1 += ldiff * Weight[k];
		suml2 += ldiff * ldiff * Weight[k];
#if PRBCHECK
		if (sumlk < 0.0 || sumlk0 < 0.0) {
			for (i = 0; i < Tpmradix; i++)
				printf("%3d %20.10e %20.10e\n", i, opb[i], cpb[i]);
			printf("%3d %15.3e %15.3e %9.3f %9.3f %9.3f\n",
				k, sumlk, sumlk0, lsumlk, lsumlk0, ldiff);
		}
#endif
	}
	dlkl = lklhd - lklhd0;
	suml1 /= Numsite;
	nn1 = (double)Numsite / (double)(Numsite-1);
	sdlkl = sqrt( nn1 * (suml2 - suml1*suml1*Numsite) );
	Relitrif[op->num - Maxspc] = dlkl / sdlkl;
	rel = probnormal(dlkl / sdlkl);
	/*
	printf("%3d %12.3f %12.3f %9.3f %7.3f %7.3f\n",
		op->num+1, lklhd, lklhd0, dlkl, sdlkl, rel);
	*/


	oprob = op->iprob;
#ifdef NUC
		tpb = tprob[0];
#endif	/* NUC */
	for (k = 0; k < Numptrn; k++) {
		opb = oprob[k];
		cpb = cprob[k];
#ifndef NUC
		for (i = 0; i < Tpmradix; i++) {
			tpb = tprob[i];
			for (j = 0, sumlk = 0.0; j < Tpmradix; j++)
				sumlk += tpb[j] * cpb[j];
			opb[i] = sumlk;
		}
#else	/* NUC */
		cpb0 = cpb[0]; cpb1 = cpb[1]; cpb2 = cpb[2]; cpb3 = cpb[3];
		opb[0] = tpb[ 0]*cpb0 + tpb[ 1]*cpb1 + tpb[ 2]*cpb2 + tpb[ 3]*cpb3;
		opb[1] = tpb[ 4]*cpb0 + tpb[ 5]*cpb1 + tpb[ 6]*cpb2 + tpb[ 7]*cpb3;
		opb[2] = tpb[ 8]*cpb0 + tpb[ 9]*cpb1 + tpb[10]*cpb2 + tpb[11]*cpb3;
		opb[3] = tpb[12]*cpb0 + tpb[13]*cpb1 + tpb[14]*cpb2 + tpb[15]*cpb3;
#endif	/* NUC */
	}
} /*_ ribranch */


Node *
relibranch(op)
Node *op;
{
	Node *cp;

	cp = op;
	do {
		cp = cp->isop->kinp;
		prodpart(cp->kinp->isop);
		if (cp->isop == NULL) { /* external node */
			cp = cp->kinp; /* not descen */
			partelkl(cp);
		} else { /* internal node */
			/* if (Debug) printf("mli %3d%3d\n",cp->num+1,cp->descen); */
			if (cp->descen) {
				partilkl(cp);
			} else {
				ribranch(cp);
			}
		}
	} while (cp != op);
	return op;
} /*_ relibranch */


void
mlvalue(tr, infotrs)
Tree *tr;
Infotree *infotrs;
{
	int n, k, npara;
	double lklhd, tbl, aic, lkl, varilkl, ldiff;

#ifdef DIST
	int i, j, i1, i2;
	double dist;
	dmatrix amt;
	imatrix pths;
	amt = new_dmatrix(Numpair, Numbrnch);
	pths = tr->paths;
	for (i = 0, i1 = 0; i1 < (Numspc - 1); i1++) {
		for (i2 = i1 + 1; i2 < Numspc; i2++, i++) {
			for (j = 0; j < Numbrnch; j++) {
				pths[j][i1] != pths[j][i2] ? (amt[i][j]=1.0) : (amt[i][j]=0.0);
			}
		}
	}
	for (i = 0, i1 = 0; i1 < (Numspc - 1); i1++) {
		for (i2 = i1 + 1; i2 < Numspc; i2++, i++) {
			for (j = 0, dist = 0.0; j < Numbrnch; j++) {
				if (amt[i][j]) dist += tr->brnchp[j]->length;
			} printf("%5.1f",dist);
		}
	} putchar('\n');
	free_dmatrix(amt);
#endif /* DIST */

	tbl = 0.0;
	for (n = 0; n < Numspc;    n++) tbl += tr->ebrnchp[n]->length;
	for (n = 0; n < Numibrnch; n++) tbl += tr->ibrnchp[n]->length;

	npara = Numspc + Numibrnch;
	if (Frequ_optn) npara += Tpmradix - 1;
#ifdef NUC
	if (AlphaBeta != 1.0) npara++;
	if (AlphaYR != 1.0) npara++;
	if (Beta12 != 1.0) npara++;
#endif /* NUC */

	lklhd = tr->lklhd;
	aic = npara * 2 - 2.0 * lklhd;
	lkl = lklhd / Numsite;
	for (k = 0, varilkl = 0.0; k < Numptrn; k++) {
		ldiff = Alklptrn[k] - lkl;
		varilkl += ldiff * ldiff * Weight[k];
	}
	tr->varilkl = varilkl;
	tr->npara = npara;
	tr->aic = aic;
	tr->tblength = tbl;
	infotrs[Cnotree].npara = npara;
	infotrs[Cnotree].lklhd = lklhd;
	infotrs[Cnotree].lklaprox = 0.0; /* !? */
	infotrs[Cnotree].aic = aic;
	infotrs[Cnotree].tblength = tbl;
	if (Cnotree == 0) {
		Maxlkltree = 0;
		Minaictree = 0;
		Mintbltree = 0;
		Maxlkl = lklhd;
		Minaic = aic;
		Mintbl = tbl;
	} else {
		if (lklhd > Maxlkl) {
			Maxlkltree = Cnotree;
			Maxlkl = lklhd;
		}
		if (aic < Minaic) {
			Minaictree = Cnotree;
			Minaic = aic;
		}
		if (tbl < Mintbl) {
			Mintbltree = Cnotree;
			Mintbl = tbl;
		}
	}
	return;
} /*_ mlvalue */


void
reroot(tr, rp)
Tree *tr;
Node *rp;
{
	Node *cp, *op, *xp, *bp, *ap;
	boolean exch_flag;

	tr->rootp = rp;
	cp = rp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp->descen = TRUE;
			cp = cp->kinp;
			cp->descen = FALSE;
		} else { /* internal node */
			if (cp->descen != -1) {
				cp->descen = TRUE;
				cp->kinp->descen = -1;
				tr->ibrnchp[cp->num - Maxspc] = cp;
			} else {
				/* printf("inode %3d\n", cp->kinp->num+1); */
				cp->descen = FALSE;
				cp = cp->kinp; /* reversed */
				op = cp;
				do {
					exch_flag = FALSE;
					/* printf("op:%3d\n", op->num+1); */
					for (bp = cp; bp->isop->isop != op; bp = bp->isop) {
						xp = bp->isop;
						ap = xp->isop;
						/* printf("bp:%3d xp:%3d ap:%3d\n",
							bp->num+1,xp->num+1,ap->num+1); */
						if (ap->num < xp->num) {
							xp->isop = ap->isop;
							ap->isop = xp;
							bp->isop = ap;
							exch_flag = TRUE;
						}
					}
					op = bp->isop;
				} while (exch_flag);
				cp->kinp->num = cp->isop->num;
				for (xp = cp->isop; xp != cp; xp = xp->isop) {
					xp->num = xp->kinp->num;
				}
				cp = cp->kinp; /* reversed */
			}
		}
	} while (cp != rp);

	/* printf("root: %d\n", cp->kinp->num+1); */
	op = cp;
	do {
		exch_flag = FALSE;
		/* printf("op:%3d\n", op->num+1); */
		for (bp = cp; bp->isop->isop != op; bp = bp->isop) {
			xp = bp->isop;
			ap = xp->isop;
			/* printf("bp:%3d xp:%3d ap:%3d\n",
				bp->num+1,xp->num+1,ap->num+1); */
			if (ap->num < xp->num) {
				xp->isop = ap->isop;
				ap->isop = xp;
				bp->isop = ap;
				exch_flag = TRUE;
			}
		}
		op = bp->isop;
	} while (exch_flag);

	cp->num = cp->kinp->num;
	for (xp = cp->isop; xp != cp; xp = xp->isop) {
		xp->num = xp->kinp->num;
	}
} /* reroot */


void
sorttree(tr, rp)
Tree *tr;
Node *rp;
{
	Node *cp, *op, *xp, *yp;

	tr->rootp = rp;
	cp = rp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp->descen = TRUE;
			cp = cp->kinp;
			cp->descen = FALSE;
			cp->num = 1;
		} else { /* internal node */
			if (cp->descen != -1) {
				cp->descen = TRUE;
				cp->kinp->descen = -1;
				tr->ibrnchp[cp->num - Maxspc] = cp;
			} else {
				cp->descen = FALSE;
			}
			if (!cp->descen) {
				op = cp->kinp;
				xp = op->isop;
				yp = xp->isop;
				if (xp->num < yp->num) {
					op->isop = yp;
					yp->isop = xp;
					xp->isop = op;
				}
				cp->num = xp->num + yp->num;
				xp->num = xp->kinp->num;
				yp->num = yp->kinp->num;
			}
		}
	} while (cp != rp);
	op = cp;
	xp = op->isop;
	yp = xp->isop;
	if (xp->num < yp->num) {
		op->isop = yp;
		yp->isop = xp;
		xp->isop = op;
	}
	xp->num = xp->kinp->num;
	yp->num = yp->kinp->num;
	op->num = op->kinp->num;
} /* sorttree */


void
chroot(tr, s1, s2)
Tree *tr;
int s1, s2;
{
	Node *rp, *cp, *op, *xp, *yp;

	if (Outgr_optn == 2) {
	} else { 
	}

	rp = tr->ebrnchp[s1]->kinp;
	cp = rp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp->descen = TRUE;
			cp = cp->kinp;
			cp->descen = FALSE;
		} else { /* internal node */
			if (cp->descen != -1) {
				cp->descen = TRUE;
				cp->kinp->descen = -1;
				tr->ibrnchp[cp->num - Maxspc] = cp;
			} else {
				cp->descen = FALSE;
			}
			if (!cp->descen) {
				op = cp->kinp;
				xp = op->isop;
				yp = xp->isop;
				if (xp->num > yp->num) {
					op->isop = yp;
					yp->isop = xp;
					xp->isop = op;
				}
				cp->num = op->isop->num;
				xp->num = xp->kinp->num;
				yp->num = yp->kinp->num;
			}
		}
	} while (cp != rp);
	op = cp;
	xp = op->isop;
	yp = xp->isop;
	if (xp->num > yp->num) {
		op->isop = yp;
		yp->isop = xp;
		xp->isop = op;
	}
	xp->num = xp->kinp->num;
	yp->num = yp->kinp->num;
	op->num = op->kinp->num;
} /* chroot */


void
noexch(rp, exchstate)
Node *rp;
ivector exchstate;
{
	Node *cp;

	cp = rp->kinp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp = cp->kinp;
		} else { /* internal node */
			if (exchstate[cp->num - Maxspc] == 1) {
				exchstate[cp->num - Maxspc] = 2;
			} else if (exchstate[cp->num - Maxspc] == 2) {
				exchstate[cp->num - Maxspc] = 0;
			} else {
				cp = cp->kinp;
			}
		}
	} while (cp != rp);
	exchstate[cp->num - Maxspc] = 1;
#if 0
	for (j = exchorder[i]; j < Numibrnch; j = exchorder[j]) {
		if (exchstate[j] && exchldiff[i] > exchldiff[j]) {
			cp = ibrn[j];
			if (rp == cp->isop->kinp ||
				rp == cp->isop->isop->kinp ||
				rp == cp->kinp->isop ||
				rp == cp->kinp->isop->kinp ||
				rp == cp->kinp->isop->isop ||
				rp == cp->kinp->isop->isop->kinp) {
				exchstate[j] = 0;
			}
		}
	}
#endif
} /* noexch */


void
reliml(tr, op, lklorg, mlklptrn, rel)
Tree *tr;
Node *op;
double lklorg;
LPVECTOR mlklptrn;
double *rel;
{
	Node *cp, *kp;
	int i, l, nconv, nconv2;
	double eps, lendiff, suml1, suml2, ldiff, sdlkl, lkldiff;

	/* prtopology(tr); */
	kp = op->kinp;
	cp = op;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			/* printf("rmle %3d%3d\n", cp->num+1,cp->descen); */
			cp = cp->kinp; /* not descen */
			partelkl(cp);
		} else { /* internal node */
			if (cp->kinp->descen != 2) {
				cp->descen = 2;
			} else {
				/* printf("rmli %3d%3d\n", cp->num+1,cp->descen); */
				prodpart(cp->kinp->isop);
				partilkl(cp);
				cp->descen ? (cp->kinp->descen = FALSE)
						   : (cp->kinp->descen = TRUE);
			}
		}
	} while (cp != op);

	prodpart(op->isop);
	mlibranch(op, 0.1, 5);
#if RELIDEBUG
	printf("\n%3s", "");
	for (i = 0; i < Numbrnch; i++) printf("%5d",i+1); putchar('\n');
#endif
	op->isop->kinp->descen = op->isop->isop->kinp->descen = 2;
	kp->isop->kinp->descen = kp->isop->isop->kinp->descen = 2;

	for (l = 0, nconv = 0; l < MAXIT; l++) {
		if      (l == 0) eps = 0.5;
		else             eps = 0.1;
#if RELIDEBUG
		printf("%3d", l+1);
		for (i = 0; i < Numspc; i++)
			printf("%5.0f",tr->ebrnchp[i]->length*100);
		for (i = 0; i < Numibrnch; i++)
			printf("%5.0f",tr->ibrnchp[i]->length*100);
		putchar('\n');
#endif
		cp = op;
		do {
			cp = cp->isop->kinp;
			prodpart(cp->kinp->isop);
			if (cp->isop == NULL) { /* external node */
				cp = cp->kinp; /* not descen */
				lendiff = cp->length;
				mlebranch(cp, eps, 5);
				lendiff = fabs(lendiff - cp->length);
				lendiff < 0.1 ? (nconv++) : (nconv = 0);
				/* printf("e%3d%9.3f%9.3f\n", cp->num+1,cp->length,lendiff); */
			} else { /* internal node */
				if (!cp->descen && cp->kinp->descen)
					partilkl(cp);
				if (cp->descen == 2 || (cp->descen && !cp->kinp->descen)) {
					if (cp->descen ==2 ) cp = cp->kinp;
					lendiff = cp->length;
					mlibranch(cp, eps, 5);
					lendiff = fabs(lendiff - cp->length);
					lendiff < 0.1 ? (nconv++) : (nconv = 0);
					/* printf("i%3d%9.3f%9.3f\n",
						cp->num+1,cp->length,lendiff); */
				}
			}
		} while (cp != op);
		if (nconv >= 5) break;
	}
	op->isop->descen ?
		(op->isop->kinp->descen = 0) : (op->isop->kinp->descen = 1);
	op->isop->isop->descen ?
		(op->isop->isop->kinp->descen = 0) : (op->isop->isop->kinp->descen = 1);
	kp->isop->descen ?
		(kp->isop->kinp->descen = 0) : (kp->isop->kinp->descen = 1);
	kp->isop->isop->descen ?
		(kp->isop->isop->kinp->descen = 0) : (kp->isop->isop->kinp->descen = 1);

	nconv = nconv2 = 0;
	Converg = FALSE;
	for (l = 0, Numit = 1; l < MAXIT; l++, Numit++) {
		if      (l == 0) eps = 0.5;
		else if (l == 1) eps = 0.1;
		else             eps = Epsilon;
#if RELIDEBUG
		printf("%3d", l+1);
		for (i = 0; i < Numspc; i++)
			printf("%5.0f",tr->ebrnchp[i]->length*100);
		for (i = 0; i < Numibrnch; i++)
			printf("%5.0f",tr->ibrnchp[i]->length*100);
		putchar('\n');
#endif
		cp = op;
		do {
			cp = cp->isop->kinp;
			prodpart(cp->kinp->isop);
			if (cp->isop == NULL) { /* external node */
				/* if (Debug) printf("mle %3d%3d\n",cp->num+1,cp->descen); */
				cp = cp->kinp; /* not descen */
				lendiff = cp->length;
				mlebranch(cp, eps, 5);
				lendiff = fabs(lendiff - cp->length);
				lendiff < Epsilon ? (nconv++)  : (nconv = 0);
				lendiff < 0.5     ? (nconv2++) : (nconv2 = 0);
			} else { /* internal node */
				/* if (Debug) printf("mli %3d%3d\n",cp->num+1,cp->descen); */
				if (cp->descen) {
					partilkl(cp);
				} else {
					lendiff = cp->length;
					mlibranch(cp, eps, 5);
					lendiff = fabs(lendiff - cp->length);
					lendiff < Epsilon ? (nconv++)  : (nconv = 0);
					lendiff < 0.5     ? (nconv2++) : (nconv2 = 0);
				}
			}
			if (nconv >= Numbrnch) {
				Converg = TRUE;
				break;
			}
		} while (cp != op);
		if (Converg) break;
	}
	if (!Converg && nconv2 >= Numbrnch) Converg = 2;
	evallkl(cp);

	for (i = 0, suml1 = suml2 = 0.0; i < Numptrn; i++) {
		ldiff = Alklptrn[i] - mlklptrn[i];
		suml1 += ldiff * Weight[i];
		suml2 += ldiff * ldiff * Weight[i];
	}
	suml1 /= Numsite;
	sdlkl = sqrt( (double)Numsite/(Numsite-1) * (suml2-suml1*suml1*Numsite) );
	lkldiff = lklorg - tr->lklhd;
	*rel = probnormal(lkldiff / sdlkl);
#if 0
	printf("%3d", op->num+1);
	printf(" %7.3f %7.3f %7.3f %7.3f", lkldiff,sdlkl,lkldiff/sdlkl,*rel);
	printf("  %3s%3d\n",
		(Converg == TRUE ? "con" : (Converg == 2 ? "jbc" : "Noc")), Numit);
#endif
	return;
} /*_ reliml */


void
localbp(reliprob, mlklptrn, rlklptrn, whichml, nb, ns)
dmatrix reliprob;
LPVECTOR mlklptrn;
LPCUBE rlklptrn;
ivector whichml;
int nb, ns;
{
	int i, j, k, ib, imax, nsite, nptrn;
	double coefrand;
	ivector addweight;
	imatrix bpn;
	dmatrix bpl;

	if (Verbs_optn) fprintf(stderr, "bootstraping\n");
	addweight = new_ivector(ns);
	bpn = new_imatrix(nb, 3);
	bpl = new_dmatrix(nb, 3);
	coefrand = (double)Numsite / ((double)RANDOM_MAX + 1.0);
	for ( j = 0, k = 0; k < Numptrn; k++ ) {
		for ( i = 0, imax = Weight[k]; i < imax; i++ ) addweight[j++] = k;
	}

	for (ib = 0; ib < nb; ib++)
		bpn[ib][0] = bpn[ib][1] = bpn[ib][2] = 0;
	for (i = 0; i < NUMBOOTSR; i++) {
		for (ib = 0; ib < nb; ib++)
			bpl[ib][0] = bpl[ib][1] = bpl[ib][2] = 0.0;
		for (k = 0; k < ns; k++) {
			nsite = (int)( coefrand * (double)rand() ); /* RANDOM */
			nptrn = addweight[nsite];
			for (ib = 0; ib < nb; ib++) {
				bpl[ib][0] += mlklptrn[nptrn];
				bpl[ib][1] += rlklptrn[ib][0][nptrn];
				bpl[ib][2] += rlklptrn[ib][1][nptrn];
			}
		}
		for (ib = 0; ib < nb; ib++) {
			bpl[ib][1] > bpl[ib][2]
			? ( bpl[ib][0] > bpl[ib][1] ? bpn[ib][0]++ : bpn[ib][1]++ )
			: ( bpl[ib][0] > bpl[ib][2] ? bpn[ib][0]++ : bpn[ib][2]++ );
		}
	}
	for (ib = 0; ib < nb; ib++) {
		reliprob[ib][0] = (double)bpn[ib][0] / (double)NUMBOOTSR;
		reliprob[ib][1] = (double)bpn[ib][whichml[ib]] / (double)NUMBOOTSR;
	}

	free_imatrix(bpn);
	free_dmatrix(bpl);
	free_ivector(addweight);
} /* localbp */


void
reliabranch(tr)
Tree *tr;
{
	boolean localconv, vibrate_flag;
	int ib, i, j, n, ll, forder, exn, exn0, exb, exb0;
	double lklorg, lklold, lklhd1, lklhd2, uldiff, muldiff, rel, minrel, iblen;
	Node *rp, *dp, *ap, *cp, *cp0, *cp1, *kp, *kp0, *kp1, *ncp;
	Node **exchbrnch1, **exchbrnch2, **ebrn, **ibrn;
	dvector elenvec, ilenvec, exchldiff, exchrelia;
	ivector exchstate, exchorder, whichml;
	LPVECTOR mlklptrn;
	LPCUBE   rlklptrn;

/*   root   kp1                 cp        [1] [2]
 * R   D ---(d)   ap   I  dp    (a)--- A   X   Y
 *                (a)-----(d)
 * Z   C ---(a)   kp0     cp0   (a)--- B   Y   X
 *          kp                  cp1
 *
 *   [1]: A(X) <--> C(Z) , [2]: B(X) <--> C(Z)
 */
	elenvec = new_dvector(Maxspc);
	ilenvec = new_dvector(Numibrnch);
	exchbrnch1 = (Node **)malloc((unsigned)Numibrnch * sizeof(Node *));
	if (exchbrnch1 == NULL) maerror("exchbrnch1 in reliabranch().");
	exchbrnch2 = (Node **)malloc((unsigned)Numibrnch * sizeof(Node *));
	if (exchbrnch2 == NULL) maerror("exchbrnch2 in reliabranch().");
	exchstate = new_ivector(Numibrnch);
	exchorder = new_ivector(Numibrnch);
	whichml   = new_ivector(Numibrnch);
	exchrelia = new_dvector(Numibrnch);
	exchldiff = new_dvector(Numibrnch+1);
	exchldiff[Numibrnch] = - DBL_MAX;
	rlklptrn  = NEW_LPCUBE(Numibrnch, 2, Numptrn);
	ebrn = tr->ebrnchp;
	ibrn = tr->ibrnchp;
	mlklptrn = Alklptrn;
	lklorg = tr->lklhd;
	exn = 0;
	exb = Numibrnch;

	for (ll = 0, localconv = vibrate_flag = FALSE; !localconv; ll++) {
		if (ll) printf("%%%d\n", ll);
		if (Verbs_optn) fprintf(stderr, "%2d:", ll+1);
		for (i = 0; i < Numspc; i++)    elenvec[i] = ebrn[i]->length;
		for (i = 0; i < Numibrnch; i++) ilenvec[i] = ibrn[i]->length;

		for (ib = 0, forder = Numibrnch, muldiff = 0.0; ib < Numibrnch; ib++) {
			if (Verbs_optn) fprintf(stderr, " %d", ib+1);
			dp = ibrn[ib];
			kp0 = ap = dp->kinp;
			if (dp->isop->isop->isop != dp || ap->isop->isop->isop != ap) {
				Relistat[ib] = -1;
				exchstate[ib] = 0;
				continue;
			}
			for (kp = ap->isop; kp->descen || kp == tr->rootp;
				kp0 = kp, kp = kp->isop)
				;
			kp1 = kp->isop;
	
			for (cp=dp->isop, cp0=dp, n=0; cp != dp; cp0=cp, cp=cp->isop, n++) {
				cp1 = cp->isop;
				kp0->isop = cp; cp->isop = kp1;
				cp0->isop = kp; kp->isop = cp1;
				for (i = 0; i < Numspc; i++)
					ebrn[i]->length = ebrn[i]->kinp->length = elenvec[i];
				for (i = 0; i < Numibrnch; i++)
					ibrn[i]->length = ibrn[i]->kinp->length = ilenvec[i];
				iblen = ap->length * 0.5;
				if (iblen < Llimit) iblen = Llimit;
				dp->length = ap->length = iblen;
				n == 0 ? (Alklptrn=rlklptrn[ib][0]):(Alklptrn=rlklptrn[ib][1]);
				reliml(tr, ap, lklorg, mlklptrn, &rel);
				n == 0 ? (lklhd1 = tr->lklhd) : (lklhd2 = tr->lklhd);
				if (n == 0) {
					Relinum[ib][0]  = dp->isop->num;
					Relinum[ib][1]  = dp->isop->isop->num;
					ncp = cp;
					minrel = rel;
				} else if (lklhd2 > lklhd1) {
					Relinum[ib][0]  = dp->isop->num;
					Relinum[ib][1]  = dp->isop->isop->num;
					ncp = cp;
					minrel = rel;
				}
				kp0->isop = kp; kp->isop = kp1;
				cp0->isop = cp; cp->isop = cp1;
			} /* for cp */

			if (lklhd1 > lklhd2) {
				lklorg > lklhd1 ? (Relistat[ib] = 0) : (Relistat[ib] = 1);
				uldiff = lklhd1 - lklorg;
				whichml[ib] = 1;
			} else {
				lklorg > lklhd2 ? (Relistat[ib] = 0) : (Relistat[ib] = 2);
				uldiff = lklhd2 - lklorg;
				whichml[ib] = 2;
			}
			if (uldiff > 0.0 && uldiff < 0.00001 &&
				minrel < 0.5 && minrel > 0.499   &&
				fabs(lklhd1 - lklhd2)  < 0.00001) { /* attention! */
					/*	printf("%3d %20.15f %20.15f %20.15f\n",
						ib+Numspc+1, uldiff, minrel, fabs(lklhd1-lklhd2)); */
					uldiff = 0.0;
					minrel = 0.5;
					Relistat[ib] = 0;
			}
			Reliprob[ib][0] = minrel;
			Reliprob[ib][1] = 1.0 - minrel;
			uldiff > 0 ? (exchstate[ib] = 1) : (exchstate[ib] = 0);
			exchorder[ib] = -Numspc-1; /* -1 */
			exchldiff[ib] = uldiff;
			exchrelia[ib] = 1.0 - minrel;
			exchbrnch1[ib] = kp;
			exchbrnch2[ib] = ncp;
			if (uldiff > muldiff) {
				exchorder[ib] = forder;
				forder = ib;
				muldiff = uldiff;
			} else if (uldiff > 0.0) {
				for (i = forder; ; i = j) {
					j = exchorder[i];
					/* printf("exchorder[%3d]:%3d\n",i+1,j+1); */
					if (uldiff > exchldiff[j]) {
						exchorder[ib] = j;
						exchorder[i] = ib;
						break;
					}
				}
			}
		} /* for ib */
		if (Verbs_optn) fprintf(stderr, "\n");
#if 0
		printf(" ib RS N1 N2  LBP1  LBP2 ES %3d   ldiff         relia\n",
			forder+Numspc+1);
		for (ib = 0; ib < Numibrnch; ib++) {
			printf("%3d%3d%3d%3d%6.3f%6.3f%3d%4d%15.10f%14.10f\n",
			ib+Numspc+1, Relistat[ib], Relinum[ib][0]+1, Relinum[ib][1]+1,
			Reliprob[ib][0], Reliprob[ib][1], exchstate[ib],
			exchorder[ib]+Numspc+1, exchldiff[ib], exchrelia[ib]);
		} putchar('\n');
#endif
		for (i = forder; i < Numibrnch; i = exchorder[i]) {
			if (exchstate[i]) noexch(ibrn[i], exchstate);
		}

		if (Xreli_optn) goto ONCE;

		if (muldiff <= 0.0) {
			localconv = TRUE;
		} else {
			prtopology(tr); putchar('\n');
			exn0 = exn;
			exb0 = exb;
			exn = 0;
			for (i = forder; i < Numibrnch; i = exchorder[i]) {
				if (exchstate[i]) {
					exn++;
					exb = i;
					kp = exchbrnch1[i];
					cp = exchbrnch2[i];
					/*
					printf("%%%3d %3d<->%-3d  ln L:%12.3f +%7.3f\n",i+Numspc+1,
						kp->kinp->num+1,cp->kinp->num+1,lklorg,exchldiff[i]);
					*/
					printf("%%%3d %3d<->%-3d  ln L:%12.3f +%17.10f\n",i+Numspc+1,
						kp->kinp->num+1,cp->kinp->num+1,lklorg,exchldiff[i]);
					dp = ibrn[i];
					ap = dp->kinp;
					kp->isop->isop->isop = cp;
					cp->isop->isop->isop = kp;
					ncp = cp->isop;
					cp->isop = kp->isop;
					kp->isop = ncp;
					if (vibrate_flag) break;
				}
			}
			fflush(stdout);
			Alklptrn = mlklptrn;
			pathing(tr);
			slslength(tr, Distanmat, Maxspc);
			initpartlkl(tr);
			rp = (Node *)mlikelihood(tr);
			lklold = lklorg;
			lklorg = tr->lklhd;
			/* printf("%%%3d %3d %3d %3d %20.10f\n",
				exn, exn0, exb, exb0, lklorg - lklold); */
			if (exn == 1 && exn0 == 1 && exb == exb0) { /* attention! */
				if (fabs(lklorg - lklold) < 0.00001) localconv = TRUE;
			}
			if (lklorg < lklold) vibrate_flag = TRUE;
		}
	} /* for ll (!localconv) */

ONCE: /* if (Xreli_optn) */

	Alklptrn = mlklptrn;
	for (i = 0; i < Numspc; i++)
		ebrn[i]->length = ebrn[i]->kinp->length = elenvec[i];
	for (i = 0; i < Numibrnch; i++)
		ibrn[i]->length = ibrn[i]->kinp->length = ilenvec[i];
	initpartlkl(tr);
	rp = (Node *)mlikelihood(tr);
	mlvalue(tr, Infotrees);
	prtopology(tr); putchar('\n');
	if (Verbs_optn) fputs("rerooting\n", stderr);
	reroot(tr, tr->rootp);
	putctopology(tr); putchar('\n');
	localbp(Reliprob, mlklptrn, rlklptrn, whichml, Numibrnch, Numsite);
	FREE_LPCUBE(rlklptrn);
	free_dvector(exchrelia);
	free_dvector(exchldiff);
	free_ivector(exchstate);
	free_ivector(exchorder);
	free_ivector(whichml);
	free(exchbrnch1);
	free(exchbrnch2);
	free_dvector(elenvec);
	free_dvector(ilenvec);
} /*_ reliabranch */


void
annealing(tr)
Tree *tr;
{
	int i, k;
	double lklorg, lklnew, lkldiff, ldiff, suml1, suml2, sdlkl, nn1, z, rel;
	double maxprob;
	Node *rp, *dp, *ap, *cp, *cp0, *cp1, *kp, *kp0, *kp1;
	LPVECTOR mlklptrn, lklptrn2, lkltemp;

	mlklptrn = NEW_LPVECTOR(Numptrn);
	lklptrn2 = NEW_LPVECTOR(Numptrn);
	for (k = 0; k < Numptrn; k++) mlklptrn[k] = Alklptrn[k];
	lkltemp = Alklptrn; Alklptrn = mlklptrn; mlklptrn = lkltemp;
	lklorg = tr->lklhd;
	nn1 = (double)Numsite / (Numsite-1);
	if (Write_optn) putchar('\n');
	for (i = 0; i < Numibrnch; i++) {
		maxprob = 1.1;
		dp = tr->ibrnchp[i];
		ap = dp->kinp;
		for (kp = ap->isop, kp0 = ap;
			kp->descen || kp == tr->rootp;
			kp0 = kp, kp = kp->isop)
			;
		kp1 = kp->isop;
	/*	printf(" kp:%3d%3d%3d", kp0->num+1, kp->num+1, kp1->num+1); */
		for (cp = dp->isop, cp0 = dp; cp != dp; cp0 = cp, cp = cp->isop) {
			cp1 = cp->isop;
			kp0->isop = cp; cp->isop = kp1;
			cp0->isop = kp; kp->isop = cp1;

		/*	putctopology(tr); */
		/*	prtopology(tr); */
#if 1
			pathing(tr);
			slslength(tr, Distanmat, Maxspc);
#endif
			initpartlkl(tr);
			rp = (Node *)mlikelihood(tr);
			lklnew = tr->lklhd;
			lkldiff = lklorg - lklnew;
			for (k = 0, suml1 = suml2 = 0.0; k < Numptrn; k++) {
				ldiff = Alklptrn[k] - mlklptrn[k];
				suml1 += ldiff * Weight[k];
				suml2 += ldiff * ldiff * Weight[k];
			}
			suml1 /= Numsite;
			sdlkl = sqrt( nn1 * (suml2 - suml1*suml1*Numsite) );
			z = lkldiff / sdlkl;
			rel = probnormal(z);
		/*	printf("%3d%3d%3d%3d",
				i+1+Numspc, cp0->num+1, cp->num+1, cp1->num+1); */
			if (Write_optn) {
				printf("%3d", i+1+Numspc);
				printf("%3d%3d ", kp->num+1, cp->num+1);
				printf("%3d%3d", dp->isop->num+1, dp->isop->isop->num+1);
				printf(" %7.3f %7.3f %7.3f  %.3f\n", i,lkldiff,sdlkl,z,rel);
			}
			if (rel < maxprob) {
				maxprob = rel;
				Reliprob[i][0] = rel;
				Relinum[i][0]  = dp->isop->num;
				Relinum[i][1]  = dp->isop->isop->num;
			}

			kp0->isop = kp; kp->isop = kp1;
			cp0->isop = cp; cp->isop = cp1;
		}
	}
	lkltemp = Alklptrn; Alklptrn = mlklptrn; mlklptrn = lkltemp;
#if 1
	pathing(tr);
	slslength(tr, Distanmat, Maxspc);
#endif
	initpartlkl(tr);
	rp = (Node *)mlikelihood(tr);
	mlvalue(tr, Infotrees);

	FREE_LPVECTOR(mlklptrn);
	FREE_LPVECTOR(lklptrn2);

} /*_ annealing */


void
qlrsearch(tr)
Tree *tr;
{
	boolean localconv;
	int ib, i, j, n, ll, forder;
	double lklorg, lklold, lklhd1, lklhd2, uldiff, muldiff, rel, minrel, iblen;
	Node *rp, *dp, *ap, *cp, *cp0, *cp1, *kp, *kp0, *kp1, *ncp;
	Node **exchbrnch1, **exchbrnch2, **ebrn, **ibrn;
	dvector elenvec, ilenvec, exchldiff, exchrelia;
	ivector exchstate, exchorder, whichml;
	LPVECTOR mlklptrn;
	LPCUBE   rlklptrn;

	boolean exch_flag;
	int mm, rnum;
	double rmin;

#define RMAX 100.0

/*   root   kp1                 cp        [1] [2]
 * R   D ---(d)   ap   I  dp    (a)--- A   X   Y
 *                (a)-----(d)
 * Z   C ---(a)   kp0     cp0   (a)--- B   Y   X
 *          kp                  cp1
 *
 *   [1]: A(X) <--> C(Z) , [2]: B(X) <--> C(Z)
 */
	elenvec = new_dvector(Maxspc);
	ilenvec = new_dvector(Numibrnch);
	exchbrnch1 = (Node **)malloc((unsigned)Numibrnch * sizeof(Node *));
	if (exchbrnch1 == NULL) maerror("exchbrnch1 in reliabranch().");
	exchbrnch2 = (Node **)malloc((unsigned)Numibrnch * sizeof(Node *));
	if (exchbrnch2 == NULL) maerror("exchbrnch2 in reliabranch().");
	exchstate = new_ivector(Numibrnch);
	exchorder = new_ivector(Numibrnch);
	whichml   = new_ivector(Numibrnch);
	exchrelia = new_dvector(Numibrnch);
	exchldiff = new_dvector(Numibrnch+1);
	exchldiff[Numibrnch] = - DBL_MAX;
	rlklptrn  = NEW_LPCUBE(Numibrnch, 2, Numptrn);
	ebrn = tr->ebrnchp;
	ibrn = tr->ibrnchp;
	mlklptrn = Alklptrn;

	for (ib = 0; ib < Numibrnch; ib++) {
		Relistat[ib] = -1;
		Relinum[ib][0] = Relinum[ib][1] = 0;
		Reliprob[ib][0] = Reliprob[ib][1] = 0.0;
	}

	for (ll = 0, localconv = FALSE; !localconv; ll++) {
		if (Verbs_optn) fprintf(stderr, "%2d:", ll+1);
		Alklptrn = mlklptrn;
		initpartlkl(tr);
		rp = (Node *)mlikelihood(tr);
		rp = (Node *)relibranch(rp);
		mlvalue(tr, Infotrees);
		reroot(tr, tr->rootp);
		mlklptrn = Alklptrn;
		lklold = lklorg;
		lklorg = tr->lklhd;
		putchar('\n');
		prtopology(tr);
#if QRELIDEBUG
		for (ib = 0; ib < Numibrnch; ib++) {
			printf("%3d%9.3f%9.3f\n",
				ib+Maxspc+1, Relitrif[ib], probnormal(Relitrif[ib]));
		}
#endif
		fflush(stdout);

		for (i = 0; i < Numspc; i++)    elenvec[i] = ebrn[i]->length;
		for (i = 0; i < Numibrnch; i++) ilenvec[i] = ibrn[i]->length;

		for (exch_flag = FALSE, mm = 0; !exch_flag; mm++) {
			if (Verbs_optn) fprintf(stderr, " %d", mm+1);
			for (ib = 0, rnum = -1, rmin = RMAX; ib < Numibrnch; ib++) {
				if (Relitrif[ib] < rmin) {
					rnum = ib;
					rmin = Relitrif[ib];
				}
			}
			if (rnum == -1) {
				/* printf("%3d %10.3f min\n", rnum+Maxspc+1, rmin); */
				localconv = TRUE;
				break;
			}

			ib = rnum;
			dp = ibrn[ib];
			kp0 = ap = dp->kinp;
			for (kp = ap->isop; kp->descen || kp == tr->rootp;
				kp0 = kp, kp = kp->isop)
				;
			kp1 = kp->isop;
	
			for (cp=dp->isop, cp0=dp, n=0; cp != dp; cp0=cp, cp=cp->isop, n++) {
				cp1 = cp->isop;
				kp0->isop = cp; cp->isop = kp1;
				cp0->isop = kp; kp->isop = cp1;
				for (i = 0; i < Numspc; i++)
					ebrn[i]->length = ebrn[i]->kinp->length = elenvec[i];
				for (i = 0; i < Numibrnch; i++)
					ibrn[i]->length = ibrn[i]->kinp->length = ilenvec[i];
				iblen = ap->length * 0.5;
				if (iblen < Llimit) iblen = Llimit;
				dp->length = ap->length = iblen;
				n == 0 ? (Alklptrn=rlklptrn[ib][0]):(Alklptrn=rlklptrn[ib][1]);
				reliml(tr, ap, lklorg, mlklptrn, &rel);
				n == 0 ? (lklhd1 = tr->lklhd) : (lklhd2 = tr->lklhd);
				if (n == 0) {
					if (lklhd1 > lklorg) {
					for (i = 0; i < Numspc; i++)    elenvec[i]=ebrn[i]->length;
					for (i = 0; i < Numibrnch; i++) ilenvec[i]=ibrn[i]->length;
					}
					Relinum[ib][0]  = dp->isop->num;
					Relinum[ib][1]  = dp->isop->isop->num;
					ncp = cp;
					minrel = rel;
				} else if (lklhd2 > lklhd1) {
					if (lklhd2 > lklorg) {
					for (i = 0; i < Numspc; i++)    elenvec[i]=ebrn[i]->length;
					for (i = 0; i < Numibrnch; i++) ilenvec[i]=ibrn[i]->length;
					}
					Relinum[ib][0]  = dp->isop->num;
					Relinum[ib][1]  = dp->isop->isop->num;
					ncp = cp;
					minrel = rel;
				}
				kp0->isop = kp; kp->isop = kp1;
				cp0->isop = cp; cp->isop = cp1;
			} /* for cp */

			if (lklhd1 > lklhd2) {
				lklorg > lklhd1 ? (Relistat[ib] = 0) : (Relistat[ib] = 1);
				uldiff = lklhd1 - lklorg;
				whichml[ib] = 1;
			} else {
				lklorg > lklhd2 ? (Relistat[ib] = 0) : (Relistat[ib] = 2);
				uldiff = lklhd2 - lklorg;
				whichml[ib] = 2;
			}
			Reliprob[ib][0] = minrel;
			Reliprob[ib][1] = 1.0 - minrel;
			uldiff > 0 ? (exchstate[ib] = 1) : (exchstate[ib] = 0);
			exchbrnch1[ib] = kp;
			exchbrnch2[ib] = ncp;
#if QRELIDEBUG
			printf("%3d %3d %8.3f", mm+1, ib+Maxspc+1, rmin);
			printf(" %3d%3d%3d%6.3f%6.3f\n", Relistat[ib], Relinum[ib][0]+1,
				Relinum[ib][1]+1, Reliprob[ib][0], Reliprob[ib][1]);
#endif
			if (exchstate[ib]) {
				kp = exchbrnch1[ib];
				cp = exchbrnch2[ib];
				printf("\n%%%-3d %3d %3d<->%-3d  ln L:%12.3f +%7.3f\n", ll+1,
					ib+Numspc+1, kp->kinp->num+1,cp->kinp->num+1,lklorg,uldiff);
				dp = ibrn[ib];
				ap = dp->kinp;
				kp->isop->isop->isop = cp;
				cp->isop->isop->isop = kp;
				ncp = cp->isop;
				cp->isop = kp->isop;
				kp->isop = ncp;
				Relistat[ib] = 0;
				Relinum[ib][0] = Relinum[ib][1] = 0;
				Reliprob[ib][0] = Reliprob[ib][1];
				Reliprob[ib][1] = 0.0;
				for (i = 0; i < Numspc; i++)
					ebrn[i]->length = ebrn[i]->kinp->length = elenvec[i];
				for (i = 0; i < Numibrnch; i++)
					ibrn[i]->length = ibrn[i]->kinp->length = ilenvec[i];
				exch_flag = TRUE;
			} else {
				Relitrif[ib] = RMAX;
			}

		} /* for ib */
		if (Verbs_optn) fprintf(stderr, "\n");

		if (Xreli_optn) goto ONCE;

	} /* for ll (!localconv) */

ONCE: /* if (Xreli_optn) */

	for (i = 0; i < Numspc; i++)
		ebrn[i]->length = ebrn[i]->kinp->length = elenvec[i];
	for (i = 0; i < Numibrnch; i++)
		ibrn[i]->length = ibrn[i]->kinp->length = ilenvec[i];
	Alklptrn = mlklptrn;
	initpartlkl(tr);
	rp = (Node *)mlikelihood(tr);
	mlvalue(tr, Infotrees);
	reroot(tr, tr->rootp);
	putchar('\n');
	putctopology(tr);
	localbp(Reliprob, mlklptrn, rlklptrn, whichml, Numibrnch, Numsite);
	FREE_LPCUBE(rlklptrn);
	free_dvector(exchrelia);
	free_dvector(exchldiff);
	free_ivector(exchstate);
	free_ivector(exchorder);
	free_ivector(whichml);
	free(exchbrnch1);
	free(exchbrnch2);
	free_dvector(elenvec);
	free_dvector(ilenvec);
} /*_ qlrsearch */
