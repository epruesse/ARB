/*
 * triproc.c   Adachi, J.   1995.09.22
 * Copyright (C) 1993-1995 J. Adachi & M. Hasegawa. All rights reserved.
 */

#define PAIR

#include "tridist.h"


void
redistmat(distan, psotu, underotu, i, j, ip, jp)
dmatrix distan;
Node **psotu;
ivector underotu;
int i, j;
Node *ip, *jp;
{
	int k;
	double dis;

	for (k = 0; k < Numspc; k++) {
		if (psotu[k] != NULL) {
#if 1
			dis = ( distan[i][k] + distan[j][k] ) * 0.5;
#else
			dis = ( distan[i][k] * underotus[i]
				  + distan[j][k] * underotus[j] )
				  / (underotus[i] + underotus[j]);
#endif
			distan[i][k] = dis;
			distan[k][i] = dis;
		}
		distan[j][k] = 0.0;
		distan[k][j] = 0.0;
	}
} /* redistmat */


triadmat(brnchmat, brnchvari, distan, psotu, numspc, nsp2)
dmatrix brnchmat;
dmatrix brnchvari;
dmatrix distan;
Node **psotu;
int numspc, nsp2;
{
	int i, j, k;
	double disij, disik, disjk, brni, brnj, brnk;

	for (i = 0; i < numspc; i++) {
		for (j = 0; j < numspc; j++) {
			brnchmat[i][j] = 0.0;
			brnchvari[i][j] = 0.0;
		}
	}
	for (i = 0; i < numspc - 2; i++) {
		if (psotu[i] != NULL) {
			for (j = i + 1; j < numspc - 1; j++) {
				if (psotu[j] != NULL) {
					disij = distan[i][j];
					for (k = j + 1; k < numspc; k++) {
						if (psotu[k] != NULL) {
							disik = distan[i][k];
							disjk = distan[j][k];
							brni =  (disij + disik - disjk) * 0.5;
							brnj =  disij - brni;
							brnk =  disik - brni;
							brnchmat[i][j] += brni;
							brnchmat[i][k] += brni;
							brnchmat[j][k] += brnj;
							brnchmat[j][i] += brnj;
							brnchmat[k][i] += brnk;
							brnchmat[k][j] += brnk;
						}
					}
				}
			}
		}
	}
	for (i = 0; i < numspc; i++) {
		for (j = 0; j < numspc; j++) brnchmat[i][j] /= nsp2;
	}

	for (i = 0; i < numspc - 2; i++) {
		if (psotu[i] != NULL) {
			for (j = i + 1; j < numspc - 1; j++) {
				if (psotu[j] != NULL) {
					disij = distan[i][j];
					for (k = j + 1; k < numspc; k++) {
						if (psotu[k] != NULL) {
							disik = distan[i][k];
							disjk = distan[j][k];
							brni =  (disij + disik - disjk) * 0.5;
							brnj =  disij - brni;
							brnk =  disik - brni;
							brnchvari[i][j] +=
								(brnchmat[i][j]-brni)*(brnchmat[i][j]-brni);
							brnchvari[i][k] +=
								(brnchmat[i][k]-brni)*(brnchmat[i][k]-brni);
							brnchvari[j][k] +=
								(brnchmat[j][k]-brnj)*(brnchmat[j][k]-brnj);
							brnchvari[j][i] +=
								(brnchmat[j][i]-brnj)*(brnchmat[j][i]-brnj);
							brnchvari[k][i] +=
								(brnchmat[k][i]-brnk)*(brnchmat[k][i]-brnk);
							brnchvari[k][j] +=
								(brnchmat[k][j]-brnk)*(brnchmat[k][j]-brnk);
						}
					}
				}
			}
		}
	}
	for (i = 0; i < numspc; i++) {
		for (j = 0; j < numspc; j++) brnchvari[i][j] = sqrt(brnchvari[i][j]);
	}
} /* triadmat */


triadmat2(brnchmat, brnchvari, distan, psotu, underotus, masterotu, numspc)
dmatrix brnchmat;
dmatrix brnchvari;
dmatrix distan;
Node **psotu;
ivector underotus;
ivector masterotu;
{
	int i, j, k, ii, jj, kk;
	double disij, disik, disjk, brni, brnj, brnk;

	for (i = 0; i < numspc; i++) {
		for (j = 0; j < numspc; j++) {
			brnchmat[i][j] = 0.0;
			brnchvari[i][j] = 0.0;
		}
	}
	for (i = 0; i < numspc - 2; i++) {
		ii = masterotu[i];
			for (j = i + 1; j < numspc - 1; j++) {
				jj = masterotu[j];
				if (jj != ii) {
					disij = distan[i][j];
					for (k = j + 1; k < numspc; k++) {
						kk = masterotu[k];
						if ((kk != jj) && (kk != ii)) {
							disik = distan[i][k];
							disjk = distan[j][k];
							brni =  (disij + disik - disjk) * 0.5;
							brnj =  disij - brni;
							brnk =  disik - brni;
							brnchmat[i][jj] += brni;
							brnchmat[i][kk] += brni;
							brnchmat[j][kk] += brnj;
							brnchmat[j][ii] += brnj;
							brnchmat[k][ii] += brnk;
							brnchmat[k][jj] += brnk;
						}
					}
				}
			}
	}
	for (i = 0; i < numspc; i++) {
			ii = underotus[i];
			for (j = 0; j < numspc; j++) {
				if (psotu[j] != NULL) {
					jj = underotus[j];
					brnchmat[i][j] /= ((numspc - ii - jj) * jj);
				}
			}
	}

	for (i = 0; i < numspc - 2; i++) {
		ii = masterotu[i];
			for (j = i + 1; j < numspc - 1; j++) {
				jj = masterotu[j];
				if (jj != ii) {
					disij = distan[i][j];
					for (k = j + 1; k < numspc; k++) {
						kk = masterotu[k];
						if ((kk != jj) && (kk != ii)) {
							disik = distan[i][k];
							disjk = distan[j][k];
							brni =  (disij + disik - disjk) * 0.5;
							brnj =  disij - brni;
							brnk =  disik - brni;
							brnchvari[i][j] +=
								(brnchmat[i][j]-brni)*(brnchmat[i][j]-brni);
							brnchvari[i][k] +=
								(brnchmat[i][k]-brni)*(brnchmat[i][k]-brni);
							brnchvari[j][k] +=
								(brnchmat[j][k]-brnj)*(brnchmat[j][k]-brnj);
							brnchvari[j][i] +=
								(brnchmat[j][i]-brnj)*(brnchmat[j][i]-brnj);
							brnchvari[k][i] +=
								(brnchmat[k][i]-brnk)*(brnchmat[k][i]-brnk);
							brnchvari[k][j] +=
								(brnchmat[k][j]-brnk)*(brnchmat[k][j]-brnk);
						}
					}
				}
			}
	}
	for (i = 0; i < numspc; i++) {
			ii = underotus[i];
			for (j = 0; j < numspc; j++) {
					jj = underotus[j];
					brnchvari[i][j] /= (numspc - ii - jj);
					brnchvari[i][j] = sqrt(brnchvari[i][j]);
			}
	}
} /* triadmat2 */


void
minpair(brnchmat, brnchvari, brndiff, brndiff2, minbrnch, minbrnch2,  psotu, numspc)
dmatrix brnchmat;
dmatrix brnchvari;
dvector brndiff;
dvector brndiff2;
ivector minbrnch;
ivector minbrnch2;
Node **psotu;
int numspc;
{
	int i, j, k, minj, minj2, minv, minv2;
	double brni, minbrn, minbrn2, temp, vari, minvari, minvari2;

	for (i = 0; i < numspc; i++) {
		if (psotu[i] != NULL) {
			minj = minj2 = -1;
			minbrn = minbrn2 = UPPERLIMIT;
			minv = minv2 = -1;
			minvari = minvari2 = UPPERLIMIT;
			for (j = 0; j < numspc; j++) {
				if (psotu[j] != NULL && j != i) {
					brni = brnchmat[i][j];
					if (brni < minbrn2) {
						if (brni < minbrn) {
							minj2 = minj;
							minbrn2 = minbrn;
						} else {
							minj2 = j;
							minbrn2 = brni;
						}
					}
					if (brni < minbrn) {
						minj = j;
						minbrn = brni;
					}

					vari = brnchvari[i][j];
					if (vari < minvari2) {
						if (vari < minvari) {
							minv2 = minv;
							minvari2 = minvari;
						} else {
							minv2 = j;
							minvari2 = vari;
						}
					}
					if (vari < minvari) {
						minv = j;
						minvari = vari;
					}
				}
			}
#if 0
			brndiff[i] = minbrn2 - minbrn; /* !? */
#endif
#if 1
			brndiff[i] = ( minbrn2 - minbrn ) / minbrn; /* !? */
#endif
			minbrnch[i] = minj;

		/*	brndiff2[i] = minvari2 - minvari; */
			brndiff2[i] = minvari;
			minbrnch2[i] = minv;
		}
	}
} /* minpair */


void
minpair2(brnchmat, brnchvari, brndiff, brndiff2, minbrnch, minbrnch2,  psotu, masterotu)
dmatrix brnchmat;
dmatrix brnchvari;
dvector brndiff;
dvector brndiff2;
ivector minbrnch;
ivector minbrnch2;
Node **psotu;
ivector masterotu;
{
	int i, j, k, ii, jj, kk, minj, minj2, minv, minv2;
	double brni, minbrn, minbrn2, temp, vari, minvari, minvari2;

	for (i = 0; i < Numspc; i++) {
		ii = masterotu[i];
			minj = minj2 = -1;
			minbrn = minbrn2 = UPPERLIMIT;
			for (j = 0; j < Numspc; j++) {
				jj = masterotu[j];
				if (psotu[j] != NULL && jj != ii) {
					brni = brnchmat[i][j];
					if (brni < minbrn2) {
						if (brni < minbrn) {
							minj2 = minj;
							minbrn2 = minbrn;
						} else {
							minj2 = j;
							minbrn2 = brni;
						}
					}
					if (brni < minbrn) {
						minj = j;
						minbrn = brni;
					}
				}
			}
			for (j = 0; j < Numspc; j++) {
				jj = masterotu[j];
				if (psotu[j] != NULL && jj != ii) {
				}
			}
#if 0
			brndiff[i] = minbrn2 - minbrn; /* !? */
#endif
#if 1
			brndiff[i] = ( minbrn2 - minbrn ) / minbrn; /* !? */
#endif
			minbrnch[i] = masterotu[minj];

			brndiff2[i] = 0.0;
			minbrnch2[i] = masterotu[minj];
	}
} /* minpair2 */


void
prbrnmat(distanmat, minbrnch, underotus, psotu, identif, numspc)
dmatrix distanmat;
ivector minbrnch, underotus;
Node **psotu;
char **identif;
int numspc;
{
	int i, j, k, n, m;

	for (k = 0; k < numspc; k = m) {
		fputs("\n        ", stdout);
		for ( n = 0, j = k; n < 10 && j < numspc; j++) {
			if (psotu[j] != NULL) {
				printf("%7.5s", identif[j]);
				n++;
			}
		}
		m = j;
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			if (psotu[i] != NULL) {
				printf("%-5.5s", identif[i]);
				printf("%3d", underotus[i]);
				for (j = k; j < m && j < numspc; j++) {
					if (psotu[j] != NULL) {
						if (i == j)
							printf("%7.5s", identif[i]);
						else if (psotu[i] == NULL || psotu[j] == NULL)
							printf("%7s", "");
#if 0
						else if (minbrnch[j] == i)
							printf("%+7.2f", distanmat[j][i]);
						else
							printf("%7.2f", distanmat[j][i]);
#else
						else
							printf("%7.2f", distanmat[j][i]
								- distanmat[j][minbrnch[j]]);
#endif
					}
				}
				printf("\n");
			}
		}
	}
} /* prbrnmat */


void
prbrnmat2(distanmat, minbrnch, underotus, psotu, identif, numspc)
dmatrix distanmat;
ivector minbrnch, underotus;
Node **psotu;
char **identif;
int numspc;
{
	int i, j, k, n, m;

	for (k = 0; k < numspc; k = m) {
		fputs("\n        ", stdout);
		for ( n = 0, j = k; n < 10 && j < numspc; j++) {
			if (1) {
				printf("%7.5s", identif[j]);
				n++;
			}
		}
		m = j;
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			if (psotu[i] != NULL) {
				printf("%-5.5s", identif[i]);
				printf("%3d", underotus[i]);
				for (j = k; j < m && j < numspc; j++) {
					if (1) {
						if (i == j)
							printf("%7.5s", identif[i]);
						else if (distanmat[j][i] == 0.0)
							printf("%7s", "");
#if 0
						else if (minbrnch[j] == i)
							printf("%+7.2f", distanmat[j][i]);
						else
							printf("%7.2f", distanmat[j][i]);
#else
						else
							printf("%7.2f", distanmat[j][i]
								- distanmat[j][minbrnch[j]]);
#endif
					}
				}
				printf("\n");
			}
		}
	}
} /* prbrnmat2 */


void
prbrnvari(distanvari, minbrnch2, underotus, psotu, identif, numspc)
dmatrix distanvari;
ivector minbrnch2, underotus;
Node **psotu;
char **identif;
int numspc;
{
	int i, j, k, n, m;

	for (k = 0; k < numspc; k = m) {
		fputs("\n        ", stdout);
		for ( n = 0, j = k; n < 10 && j < numspc; j++) {
			if (psotu[j] != NULL) {
				printf("%7.5s", identif[j]);
				n++;
			}
		}
		m = j;
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			if (psotu[i] != NULL) {
				printf("%-5.5s", identif[i]);
				printf("%3d", underotus[i]);
				for (j = k; j < m && j < numspc; j++) {
					if (psotu[j] != NULL) {
						if (i == j)
							printf("%7.5s", identif[i]);
						else if (psotu[i] == NULL || psotu[j] == NULL)
							printf("%7s", "");
						else if (minbrnch2[j] == i)
							printf("%+7.2f", distanvari[j][i]);
						else
							printf("%7.2f", distanvari[j][i]);
					}
				}
				printf("\n");
			}
		}
	}
} /* prbrnvari */


void
pairorder(brndiff, brndiff2, minbrnch, minbrnch2, brnorder, psotu, numspc)
dvector brndiff, brndiff2;
ivector minbrnch, minbrnch2, brnorder;
Node **psotu;
int numspc;
{
	int i, j, k, maxi;
	double diff, maxdiff;

	maxdiff = -100.0;
	maxi = -1;
	for (i = 0; i < numspc; i++) {
		j = minbrnch[i];
		if (i == minbrnch[j] && i < j) {
			if (Write_optn) {
				printf("    %8.3f%8.3f  (%-5.5s,%-5.5s)",
					brndiff[i], brndiff[j], Identif[i], Identif[j]);
				printf("\t%8.4s%8.3f%8.3f\n",
					(i == minbrnch2[j] && j == minbrnch2[i]) ? "vari" : "----",
					brndiff2[i], brndiff2[j]);
			}
#if 1
			(brndiff[i] < brndiff[j]) ? (diff=brndiff[i]) : (diff=brndiff[j]);
#endif
#if 0
			diff = ( brndiff[i] + brndiff[j] ) * 0.5;
#endif
		/*	if (i != minbrnch2[j] || j != minbrnch2[i]) diff -= 10.0; */
			if (diff > maxdiff) { /* > */
				maxdiff = diff;
				maxi = i;
			}
		}
	}
	brnorder[0] = maxi;
	
} /* pairorder */


void
pairorder2(brndiff, brndiff2, minbrnch, minbrnch2, brnorder, psotu, numspc)
dvector brndiff, brndiff2;
ivector minbrnch, minbrnch2, brnorder;
Node **psotu;
int numspc;
{
	int i, j, k, maxi;
	double diff, maxdiff;

	maxdiff = -100.0;
	maxi = -1;
	for (i = 0; i < numspc; i++) {
		j = minbrnch[i];
		if (i == minbrnch[j] && i < j) {
			if (Write_optn) {
				printf("    %8.3f%8.3f  (%-5.5s,%-5.5s)",
					brndiff[i], brndiff[j], Identif[i], Identif[j]);
				printf("\t%8.4s%8.3f%8.3f\n",
					(i == minbrnch2[j] && j == minbrnch2[i]) ? "vari" : "----",
					brndiff2[i], brndiff2[j]);
			}
#if 1
			(brndiff[i] < brndiff[j]) ? (diff=brndiff[i]) : (diff=brndiff[j]);
#endif
#if 0
			diff = ( brndiff[i] + brndiff[j] ) * 0.5;
#endif
		/*	if (i != minbrnch2[j] || j != minbrnch2[i]) diff -= 10.0; */
			if (diff > maxdiff) { /* > */
				maxdiff = diff;
				maxi = i;
			}
		}
	}
	brnorder[0] = maxi;
	
} /* pairorder2 */


void
distantree(tr, distan, numspc)
Tree *tr;
dmatrix distan;
int numspc;
{
	int i, j, k, otui, otuj, otuk, nsp2, cinode, step, restsp;
	double dij, bix, bjx, bkx, sij, smin;
	dmatrix brnchmat, brnchvari;
	dvector brndiff, brndiff2;
	ivector minbrnch, minbrnch2, brnorder, underotus, masterotu;
	Node **psotu, *cp, *ip, *jp, *kp;

	nsp2 = numspc - 2;
	cinode = numspc;
	psotu = (Node **)new_npvector(numspc);
	brnchmat = new_dmatrix(numspc, numspc);
	brnchvari = new_dmatrix(numspc, numspc);
	brndiff = new_dvector(numspc);
	brndiff2 = new_dvector(numspc);
	minbrnch = new_ivector(numspc);
	minbrnch2 = new_ivector(numspc);
	brnorder = new_ivector(numspc);
	underotus = new_ivector(numspc);
	masterotu = new_ivector(numspc);
	for (i = 0; i < numspc; i++) {
		psotu[i] = tr->brnchp[i]->kinp;
		underotus[i] = 1;
		masterotu[i] = i;
	}
	restsp = numspc;

	for (step = 0; restsp > 3; step++) {

#ifndef PAIR
		triadmat(brnchmat, brnchvari, distan, psotu, numspc, nsp2);
		minpair(brnchmat, brnchvari, brndiff, brndiff2, minbrnch, minbrnch2,
			psotu, numspc);
#else
		triadmat2(brnchmat, brnchvari, distan, psotu,
			underotus, masterotu, numspc);
		minpair2(brnchmat, brnchvari, brndiff, brndiff2, minbrnch, minbrnch2,
			psotu, masterotu);
#endif
		if (Write_optn && Info_optn)
#ifndef PAIR
			prbrnmat(brnchmat, minbrnch, underotus, psotu, Identif, numspc);
#else
			prbrnmat2(brnchmat, minbrnch, underotus, psotu, Identif, numspc);
#endif
/*
		if (Write_optn && Info_optn)
			prbrnvari(brnchvari, minbrnch2, underotus, psotu, Identif, numspc);
*/
#ifndef PAIR
		pairorder(brndiff, brndiff2, minbrnch, minbrnch2,
			brnorder, psotu, numspc);
#else
		pairorder2(brndiff, brndiff2, minbrnch, minbrnch2,
			brnorder, psotu, numspc);
#endif

		for (i = brnorder[0]; i < brnorder[0]+1; i++) { /* numspc */
			if (restsp == 3) break;
			j = minbrnch[i];
			if (i == minbrnch[j] && i < j) { /* i == minbrnch[j] */
				if (Write_optn)
					printf("%-4d%8.3f%8.3f  (%s,%s)\n",
					cinode+1, brndiff[i], brndiff[j], Identif[i], Identif[j]);

				bix = brnchmat[i][j];
				bjx = brnchmat[j][i];
				dij = bix + bjx;
				cp = tr->brnchp[cinode];
				ip = psotu[i];
				jp = psotu[j];
				cp->isop = ip;
				ip->isop = jp;
				jp->isop = cp;
				ip->length += bix; /* += */
				jp->length += bjx; /* += */
				if (ip->length < 0.0) ip->length = 0.0;
				if (jp->length < 0.0) jp->length = 0.0;
				ip->kinp->length = ip->length;
				jp->kinp->length = jp->length;
				cp = cp->kinp;
				cp->length = dij * -0.5;
				psotu[i] = NULL;
				psotu[j] = NULL;
				masterotu[j] = i;
#if 0
				redistmat(distan, psotu, underotus, i, j, ip, jp);
#endif
				psotu[i] = cp;
				underotus[i] += underotus[j];
				underotus[j] = 0;
				cinode++;
				Numbrnch = cinode;
				restsp--;
				nsp2 -= 1.0;
				if (Debug_optn) {
					for (putchar('\n'), i = 0; i < numspc; i++) {
						for (j = 0; j < numspc; j++)
							printf("%8.3f", distan[i][j]);
						putchar('\n');
					}
				}
			}
		}

	}

	otui = otuj = otuk = -1;
	for (i = 0; i < numspc; i++) {
		if (psotu[i] != NULL) {
			if (otui == -1)
				otui = i;
			else if (otuj == -1)
				otuj = i;
			else
				otuk = i;
		}
	}
	if (Write_optn && Info_optn) printf("%4d%4d%4d\n", otui, otuj, otuk);
	bix = ( brnchmat[otui][otuj] + brnchmat[otui][otuj] ) * 0.5;
	bjx = ( brnchmat[otuj][otui] + brnchmat[otuj][otuk] ) * 0.5;
	bkx = ( brnchmat[otuk][otui] + brnchmat[otuk][otuj] ) * 0.5;
	ip = psotu[otui];
	jp = psotu[otuj];
	kp = psotu[otuk];
	ip->isop = jp;
	jp->isop = kp;
	kp->isop = ip;
	ip->length += bix;
	jp->length += bjx;
	kp->length += bkx;
	if (ip->length < 0.0) ip->length = 0.0;
	if (jp->length < 0.0) jp->length = 0.0;
	if (kp->length < 0.0) kp->length = 0.0;
	ip->kinp->length = ip->length;
	jp->kinp->length = jp->length;
	kp->kinp->length = kp->length;
	tr->ablength = 0.0;
	if (Outgr_optn) {
		reroot(tr, tr->brnchp[Outgroup]->kinp);
	} else {
		tr->rootp = kp;
	}
	free_npvector(psotu);
	free_ivector(masterotu);
	free_ivector(underotus);
	free_ivector(brnorder);
	free_ivector(minbrnch2);
	free_ivector(minbrnch);
	free_dvector(brndiff2);
	free_dvector(brndiff);
	free_dmatrix(brnchvari);
	free_dmatrix(brnchmat);
} /*_ distantree */
