/*
 * njproc.c   Adachi, J.   1995.10.30
 * Copyright (C) 1993-1995 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "tridist.h"

#define REV 0

#define DMIN -1.0e300

void
distantree(tr, dmat, numspc)
Tree *tr;
dmatrix dmat;
int numspc;
{
	int i, j, ii, jj, kk, otui, otuj, nsp2, cinode, restsp;
	double dij, bix, bjx, bkx, sij, smax, dnsp2, dij2;
	ivector otu;
	dvector r;
	Node **psotu, *cp, *ip, *jp, *kp;

	cinode = numspc;
	nsp2 = numspc - 2;
	dnsp2 = 1.0 / nsp2;
	r = new_dvector(numspc);
	otu = new_ivector(numspc);
	psotu = (Node **)new_npvector(numspc);
	for (i = 0; i < numspc; i++) {
		otu[i] = i;
		psotu[i] = tr->brnchp[i]->kinp;
	}

	for (restsp = numspc; restsp > 3; restsp--) {

		if (Upgma_optn) {
			for (i = 0, smax = DMIN; i < restsp-1; i++) {
				ii = otu[i];
				for (j = i + 1; j < restsp; j++) {
					jj = otu[j];
					sij = - dmat[ii][jj];
					/*	printf("%3d%3d %10.5f\n",ii+1,jj+1,sij); */
#if					REV
					sij = - sij;
#endif				/* REV */
					if (sij > smax) {
						smax = sij; otui = i; otuj = j;
					}
				}
			}
		} else { /* NJ */
			for (i = 0; i < restsp; i++) {
				ii = otu[i];
				for (j = 0, sij = 0.0; j < restsp; j++) sij += dmat[ii][otu[j]];
				r[ii] = sij;
			}
			for (i = 0, smax = DMIN; i < restsp-1; i++) {
				ii = otu[i];
				for (j = i + 1; j < restsp; j++) {
					jj = otu[j];
					sij = ( r[ii] + r[jj] ) * dnsp2 - dmat[ii][jj]; /* max */
					/* printf("%3d%3d %9.3f %9.3f %9.3f\n",
						ii+1,jj+1,sij,r[ii],r[jj]); */
#if					REV
					sij = - sij;
#endif				/* REV */
					if (sij > smax) {
						smax = sij; otui = i; otuj = j;
					}
				}
			}
		} /* Upgma_optn */

		ii = otu[otui];
		jj = otu[otuj];
		dij = dmat[ii][jj];
		dij2 = dij * 0.5;
		if (Upgma_optn) {
			bix = dij2;
			bjx = dij2;
		} else {
			bix = (dij + r[ii]/nsp2 - r[jj]/nsp2) * 0.5;
			bjx = dij - bix;
		}
		cp = tr->brnchp[cinode];
		ip = psotu[ii];
		jp = psotu[jj];
		cp->isop = ip;
		ip->isop = jp;
		jp->isop = cp;
		ip->length += bix;
		jp->length += bjx;
		ip->kinp->length = ip->length;
		jp->kinp->length = jp->length;
		cp = cp->kinp;
		cp->length = - dij2;
		psotu[ii] = cp;
		psotu[jj] = NULL;
		for (j = 0; j < restsp; j++) {
			kk = otu[j];
			if (kk != ii && kk != jj) {
				dij = (dmat[ii][kk] + dmat[jj][kk]) * 0.5;
				dmat[ii][kk] = dij;
				dmat[kk][ii] = dij;
			}
			dmat[jj][kk] = 0.0;
			dmat[kk][jj] = 0.0;
		}
		Numbrnch = ++cinode;
		dnsp2 = 1.0 / --nsp2;
		if (Debug_optn) {
			for (putchar('\n'), j = 0; j < restsp; j++) printf("%6d",otu[j]+1);
			for (putchar('\n'), i = 0; i < restsp; i++, putchar('\n')) {
				for (j = 0, ii = otu[i]; j < restsp; j++) {
					printf("%6.0f", dmat[ii][otu[j]]*100);
				}
			}
		}
		for (j = otuj; j < restsp - 1; j++) otu[j] = otu[j + 1];

	} /* for restsp */

	ii = otu[0];
	jj = otu[1];
	kk = otu[2];
	if (Upgma_optn) {
		if (dmat[ii][jj] < dmat[ii][kk]) {
			if (dmat[jj][kk] < dmat[ii][jj]) {
				i = ii; ii = jj; jj = kk; kk = i;
			}
		} else {
			if (dmat[ii][kk] < dmat[jj][kk]) {
				j = jj; jj = kk; kk = j;
			} else {
				i = ii; ii = jj; jj = kk; kk = i;
			}
		}
		bix = dmat[ii][jj] * 0.5;
		bjx = dmat[ii][jj] * 0.5;
		bkx = (dmat[ii][kk] + dmat[jj][kk] - dmat[ii][jj]) * 0.5;
	} else { /* NJ */
		bix = (dmat[ii][jj] + dmat[ii][kk] - dmat[jj][kk]) * 0.5;
		bjx = dmat[ii][jj] - bix;
		bkx = dmat[ii][kk] - bix;
	}
	ip = psotu[ii];
	jp = psotu[jj];
	kp = psotu[kk];
	ip->isop = jp;
	jp->isop = kp;
	kp->isop = ip;
	ip->length += bix;
	jp->length += bjx;
	kp->length += bkx;
	ip->kinp->length = ip->length;
	jp->kinp->length = jp->length;
	kp->kinp->length = kp->length;
	tr->ablength = sij;
	if (Upgma_optn) {
		tr->rootp = kp;
	} else {
		if (Outgr_optn) {
			reroot(tr, tr->brnchp[Outgroup]->kinp);
		} else {
			reroot(tr, tr->brnchp[numspc-1]->kinp); /* tr->rootp = kp; */
		}
	}
	free_dvector(r);
	free_ivector(otu);
	free_npvector(psotu);
} /*_ distantree */
