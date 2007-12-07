/*
 * njmtree.c   Adachi, J.   1995.10.09
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "protml.h"


void 
initsubplkl(op)
Node *op;
{
	/*      op
	 *     (a)----------(d)...
	 *    iprob <---...
	 */
	Node *cp;

	if (op->kinp->isop == NULL) { /* external branch */
		partelkl(op);
	} else { /* internal branch */
		/*
		 *     ( )   op        start  ( )---...
		 *          (a)----------(d)
		 *     ( )                    ( )---...
		 */
		cp = op->kinp;
		do {
			cp = cp->isop->kinp;
			if (cp->isop == NULL) { /* external node */
				cp = cp->kinp; /* not descen */
				if (Debug) printf("mse %3d\n", cp->num+1);
				partelkl(cp);
			} else { /* internal node */
				if (!cp->descen) {
					if (Debug) printf("msi %3d\n", cp->num+1);
					prodpart(cp->kinp->isop);
					partilkl(cp);
				}
			}
		} while (cp != op);
	}
} /*_ initsubplkl */


void
mlepartlen(ip, jp, rotup, nr, ns)
Node *ip, *jp;
Node **rotup;
int nr, ns;
{
	/* ...(a) - - - - - -      (a)-----(d)...
	 *          (a)-----(d)
	 * ...(a) - - - - - -      (a)-----(d)...
	 * ...(a) - - - - - -
	 */
	boolean conv;
	Node *cp, *kp, *xp;
	int k, l, m, maxm, nconv, nconv2;
	double lendiff, eps;

	kp = jp->isop;
	nconv = nconv2 = 0;
	Numit = 0;
	Converg = FALSE;
	for (l = 0; l < MAXIT; l++) { /* MAXIT */
		Numit++;
		if (Debug) printf("ml:%3d\n", l);
		if      (l == 0) eps = 1.0;
		else if (l == 1) eps = 0.2;
		else if (l == 2) eps = 0.1;
		else             eps = EPSILON;

		copypart1(kp, rotup[0]);
		for (k = 1; k < nr; k++) prodpart1(kp, rotup[k]);
		jp->isop = kp;
	/*	kp->isop = ip; */
		nconv = nconv2 = 0;
		if (l < 1) maxm = 5; else maxm = 1;
		for (m = 0; m < maxm; m++) {
			conv = TRUE;
			cp = kp;
			do {
				cp = cp->isop->kinp;
				if (maxm > 1) {
					m == 0 ? copypart1(kp->kinp, kp) : copypart1(kp, kp->kinp);
				}
				prodpart(cp->kinp->isop);
				if (cp->isop == NULL) { /* external node */
					cp = cp->kinp; /* not descen */
					lendiff = cp->length;
					mlebranch(cp, eps, 5);
					lendiff = fabs(lendiff - cp->length);
					lendiff < EPSILON ? (nconv++)  : (nconv = 0);
					lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
					if (lendiff > EPSILON) conv = FALSE;
				} else { /* internal node */
					if (cp->descen) {
						partilkl(cp);
					} else {
						lendiff = cp->length;
						mlibranch(cp, eps, 5);
						lendiff = fabs(lendiff - cp->length);
						lendiff < EPSILON ? (nconv++)  : (nconv = 0);
						lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
						if (lendiff > EPSILON) conv = FALSE;
					}
				}
			} while (cp != jp);
			if (conv) break;
		}
		if (Debug) putchar('\n');
		prodpart1(ip, jp);
		for (k = 0; k < nr; k++) {
			xp = rotup[k];
		/*	xp->isop = ip; */
			jp->isop = xp;
#if 1
			lendiff = xp->length;
			if (xp->kinp->isop == NULL) { /* external branch */
				mlebranch(xp, eps, 5);
			} else { /* internal branch */
				mlibranch(xp, eps, 5);
			}
			lendiff = fabs(lendiff - xp->length);
			lendiff < EPSILON ? (nconv++)  : (nconv = 0);
			lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
			if (lendiff > EPSILON) conv = FALSE;
#else
			cp = jp;
			do {
				cp = cp->isop->kinp;
				if (cp->kinp->isop != ip) prodpart(cp->kinp->isop);
				if (cp->isop == NULL) { /* external node */
					cp = cp->kinp; /* not descen */
					lendiff = cp->length;
					mlebranch(cp, eps, 5);
					lendiff = fabs(lendiff - cp->length);
					lendiff < EPSILON ? (nconv++)  : (nconv = 0);
					lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
					if (lendiff > EPSILON) conv = FALSE;
				} else { /* internal node */
					if (cp->descen) {
						partilkl(cp);
					} else {
						lendiff = cp->length;
						mlibranch(cp, eps, 5);
						lendiff = fabs(lendiff - cp->length);
						lendiff < EPSILON ? (nconv++)  : (nconv = 0);
						lendiff < 0.1     ? (nconv2++) : (nconv2 = 0);
						if (lendiff > EPSILON) conv = FALSE;
					}
				}
			} while (cp != xp);
#endif
		}
		jp->isop = kp;
		if (nconv >= Numbrnch) goto convergence;
		if (conv) goto convergence;

	}
	if (nconv2 >= Numbrnch) Converg = 2;
	return;

convergence:
	Converg = TRUE;
} /*_ mlepartlen */


void
remldmat(dmat, dij, psotu, rotup, otui, otuj, ns)
dmatrix dmat;
double dij;
Node **psotu, **rotup;
int otui, otuj, ns;
{
	int k, nr;
	double dis;
	Node *cp, *ip, *jp;

	ip = psotu[otui];
	jp = psotu[otuj];
	psotu[otui] = psotu[otuj] = NULL;
	for (k = 0, nr = 0; k < ns; k++) {
		cp = psotu[k];
		if (cp != NULL) {
			dis = (dmat[otui][k] + dmat[otuj][k] - dij) * 0.5;
			if (dis < Llimit) dis = Llimit;
			cp->length = cp->kinp->length = dis;
			cp->isop = ip;
			initsubplkl(cp);
			rotup[nr++] = cp;
		}
	}
	initsubplkl(ip);
	initsubplkl(jp);

	mlepartlen(ip, jp, rotup, nr, ns);

	for (k = 0; k < ns; k++) {
		if (psotu[k] != NULL) {
			dis = psotu[k]->length;
			dmat[otui][k] = dmat[k][otui] = dis;
		}
		dmat[otuj][k] = dmat[k][otuj] = 0.0;
	}

} /* remldmat */


void
njmtree(tr, distan, ns, flag)
Tree *tr;
dmatrix distan;
int ns;
boolean flag;
{
	int i, j, otui, otuj, otuk, nsp2, cinode, step, restsp, nitr;
	double dij, bix, bjx, bkx, sij, smax, q, dnsp2;
	dvector r;
	dmatrix dmat;
	Node **psotu, **rotup, *cp, *ip, *jp, *kp;

	dmat = new_dmatrix(ns, ns);
	for (i = 0; i < ns; i++) {
		for (j = 0; j < ns; j++) dmat[i][j] = distan[i][j];
	}
	nitr = 0;
	nsp2 = ns - 2;
	dnsp2 = 1.0 / nsp2;
	cinode = ns;
	r = new_dvector(ns);
	psotu = (Node **)new_npvector(ns);
	rotup = (Node **)new_npvector(ns);
	for (i = 0; i < ns; i++) psotu[i] = tr->ebrnchp[i]->kinp;
#if 0
	if (Debug) {
		for (i = 0; i < ns; i++) {
				printf("%3d", i+1);
				for (j = 0; j < ns; j++) printf("%8.3f",dmat[i][j]);
				putchar('\n');
		} putchar('\n');
	}
#endif
	for (step = 0, restsp = ns; restsp > 3; step++) {
		
		if (Verbs_optn) fprintf(stderr," %d %d", step+1, restsp);

		for (i = 0, q = 0.0; i < ns; i++) {
			if (psotu[i] != NULL) {
				for (j = 0, r[i] = 0.0; j < ns; j++) {
					if (psotu[j] != NULL) r[i] += dmat[i][j];
				}
				q += r[i];
			}
		}
		for (i = 0, smax = -1.0e300; i < ns-1; i++) {
			if (psotu[i] != NULL) {
				for (j = i+1; j < ns; j++) {
					if (psotu[j] != NULL) {
						sij = ( r[i] + r[j] ) * dnsp2 - dmat[i][j];
#if 0
						if (Debug)
							printf("%3d%3d %9.4f %9.4f %9.4f\n",
								i+1,j+1,sij,r[i],r[j]);
#endif
						if (!flag) sij = -sij;
						if (sij > smax) {
							smax = sij;
							otui = i;
							otuj = j;
						}
					}
				}
			}
		}
		dij = dmat[otui][otuj];
		bix = (dij + r[otui]/nsp2 - r[otuj]/nsp2) * 0.5;
		bjx = dij - bix;
		if (bix < Llimit) bix = Llimit;
		if (bjx < Llimit) bjx = Llimit;
		ip = psotu[otui];
		jp = psotu[otuj];
		ip->length += bix;
		jp->length += bjx;
		ip->kinp->length = ip->length;
		jp->kinp->length = jp->length;
		cp = tr->ibrnchp[cinode-ns];
		cp->isop = ip;
		ip->isop = jp;
		jp->isop = cp;

		if (Debug)
		printf ("%3d%3d %9.4f %9.4f %9.4f %9.4f\n", ip->num+1, jp->num+1,
			ip->length, jp->length, ip->kinp->length, jp->kinp->length);

		remldmat(dmat, dij, psotu, rotup, otui, otuj, ns);
		psotu[otui] = cp->kinp;
		psotu[otuj] = NULL;
		nitr += Numit;
#if 0
		if (Debug) {
			putchar('\n');
			for (i = 0; i < ns; i++) {
					printf("%3d", i+1);
					for (j = 0; j < ns; j++) printf("%8.3f",dmat[i][j]);
					putchar('\n');
			} putchar('\n');
		}
#endif
		cinode++;
		Numibrnch++;
		Numbrnch++;
		Numbrnch = cinode;
		restsp--;
		nsp2--;
		dnsp2 = 1.0 / nsp2;
		if (Verbs_optn) fprintf(stderr," %d\n", Numit);

	} /* for step restsp */

	otui = otuj = otuk = -1;
	for (i = 0; i < ns; i++) {
		if (psotu[i] != NULL) {
			if (otui == -1)
				otui = i;
			else if (otuj == -1)
				otuj = i;
			else
				otuk = i;
		}
	}
	bix = (dmat[otui][otuj] + dmat[otui][otuk] - dmat[otuj][otuk]) * 0.5;
	bjx = dmat[otui][otuj] - bix;
	bkx = dmat[otui][otuk] - bix;
	if (bix < Llimit) bix = Llimit;
	if (bjx < Llimit) bjx = Llimit;
	if (bkx < Llimit) bkx = Llimit;
	ip = psotu[otui];
	jp = psotu[otuj];
	kp = psotu[otuk];
	ip->isop = jp;
	jp->isop = kp;
	kp->isop = ip;
	ip->length += bix;
	jp->length += bjx;
	kp->length += bkx;
	ip->kinp->length = ip->length;
	jp->kinp->length = jp->length;
	kp->kinp->length = kp->length;

	reroot(tr, tr->ebrnchp[ns-1]->kinp);

	if (Verbs_optn) fprintf(stderr, "iter: %d\n", nitr);

	free_dvector(r);
	free_npvector(psotu);
	free_npvector(rotup);
	free_dmatrix(dmat);
} /*_ njmtree */
