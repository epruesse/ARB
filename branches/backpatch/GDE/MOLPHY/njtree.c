/*
 * njtree.c   Adachi, J.   1995.06.09
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "protml.h"
#include "prot_tml.h"

#define ENJ 0

Tree *
new_njtree(maxspc, maxibrnch, numptrn, seqconint)
int maxspc, maxibrnch, numptrn;
imatrix seqconint;
{
	int n, i;
	Tree *tr;
	Node *dp, *up;

	tr = (Tree *) malloc(sizeof(Tree));
	if (tr == NULL) maerror("tr in new_njtree().");
	tr->ebrnchp = (Node **) malloc((unsigned)maxspc * sizeof(Node *));
	if (tr->ebrnchp == NULL) maerror("ebrnchp in new_njtree().");
	tr->ibrnchp = (Node **) malloc((unsigned)maxibrnch * sizeof(Node *));
	if (tr->ibrnchp == NULL) maerror("ibrnchp in new_njtree().");
	tr->bturn = new_ivector(maxspc);
	for (n = 0; n < maxspc; n++) {
		tr->bturn[n] = n;
		dp = (Node *) malloc(sizeof(Node));
		if (dp == NULL) maerror("dp in new_njtree().");
		up = (Node *) malloc(sizeof(Node));
		if (up == NULL) maerror("up in new_njtree().");
		dp->isop = NULL;
		up->isop = NULL;
		dp->kinp = up;
		up->kinp = dp;
		dp->descen = TRUE;
		up->descen = FALSE;
		dp->num = n;
		up->num = n;
		dp->length = 0.0;
		up->length = 0.0;
		dp->lklhdl = 0.0;
		up->lklhdl = 0.0;
		dp->paths = new_ivector(maxspc);
		up->paths = dp->paths;
		for (i = 0; i < maxspc; i++) dp->paths[i] = 0;
		dp->paths[n] = 1;
		dp->eprob = seqconint[n];
		up->eprob = NULL;
		dp->iprob = NULL;
		up->iprob = new_dmatrix(numptrn, Tpmradix);
		tr->ebrnchp[n] = dp;
	}
	for (n = 0; n < maxspc - 1; n++) {
		tr->ebrnchp[n]->kinp->isop = tr->ebrnchp[n + 1]->kinp;
	}
	tr->ebrnchp[maxspc - 1]->kinp->isop = tr->ebrnchp[0]->kinp;
	for (n = 0; n < maxibrnch; n++) {
		dp = (Node *) malloc(sizeof(Node));
		if (dp == NULL) maerror("dp in new_njtree().");
		up = (Node *) malloc(sizeof(Node));
		if (up == NULL) maerror("up in new_njtree().");
		dp->isop = NULL;
		up->isop = NULL;
		dp->kinp = up;
		up->kinp = dp;
		dp->descen = TRUE;
		up->descen = FALSE;
		dp->num = n + maxspc;
		up->num = n + maxspc;
		dp->length = 0.0;
		up->length = 0.0;
		dp->lklhdl = 0.0;
		up->lklhdl = 0.0;
		dp->paths = new_ivector(maxspc);
		up->paths = dp->paths;
		for (i = 0; i < maxspc; i++) dp->paths[i] = 0;
		dp->eprob = NULL;
		up->eprob = NULL;
		dp->iprob = new_dmatrix(numptrn, Tpmradix);
		up->iprob = new_dmatrix(numptrn, Tpmradix);
		tr->ibrnchp[n] = dp;
	}
	tr->rootp = tr->ebrnchp[maxspc - 1]->kinp;

	return tr;
} /*_ new_njtree */


void
free_njtree(tr, maxspc, maxibrnch)
Tree *tr;
int maxspc, maxibrnch;
{
	int n;
	Node *dp, *up;

	for (n = 0; n < maxspc; n++) {
		dp = tr->ebrnchp[n];
		up = dp->kinp;
		free_ivector(dp->paths);
		free_dmatrix(up->iprob);
		free(up);
		free(dp);
	}
	for (n = 0; n < maxibrnch; n++) {
		dp = tr->ibrnchp[n];
		up = dp->kinp;
		free_ivector(dp->paths);
		free_dmatrix(up->iprob);
		free_dmatrix(dp->iprob);
		free(up);
		free(dp);
	}
	free(tr->bturn);
	free(tr->ibrnchp);
	free(tr->ebrnchp);
	free(tr);
} /*_ free_njtree */


double
emledis(dis, ip, kp)
double dis;
Node *ip, *kp;
{
	int i, j, k, it, numloop;
	double sumlk, sumd1, sumd2, lkld1, lkld2, prod, vari;
	double arc, arcold, arcdiff, arcpre;
	dmattpmty tprob, tdif1, tdif2;
	dmatrix oprob;
	ivector dseqi;
	dvector opb;
#ifdef NUC
	dvector tpb, td1, td2;
	double pn0, pn1, pn2, pn3;
#endif /* NUC */

	oprob = ip->iprob;
	dseqi = kp->eprob;
	arc = arcpre = dis;

	numloop = 30;
	for (it = 0; it < numloop; it++) {
		tdiffmtrx(arc, tprob, tdif1, tdif2);
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
		vari = 1.0 / fabs(lkld2);
		arcold = arc;
		arcdiff = - (lkld1 / lkld2);
		arc += arcdiff;
		if (arc > Mlimit && arcpre < 10.0) arc = Llimit;
		if (arc < LOWERLIMIT) arc = LOWERLIMIT;
		if (arc > Ulimit) arc = Ulimit;
		if (lkld2 > 0.0) {
			arc = Llimit;
			if (Debug || Debug_optn)
			fprintf(stderr,"mli: second derivative is positive! %8.3f\n",lkld2);
			break;
		}
		/*	printf("mle %3d %3d %8.3f %8.3f %12.5f %12.5f %10.1f\n",
			kp->num+1, it+1, arc, arcold, arcdiff, lkld1, lkld2); */
		if (fabs(arcold - arc) < DEPSILON) break;
	}
	/*	if (Debug) */
	if (fabs(arcdiff) > DEPSILON)
		printf("mle%4d%3d%8.3f%8.3f%8.3f%10.5f%9.5f%9.3f\n",
		kp->num+1, it+1, arc, arcpre, sqrt(vari), arcdiff, lkld1, lkld2);
	return arc;
} /* emledis */


double
imledis(dis, ip, kp)
double dis;
Node *ip, *kp;
{
	int i, j, k, it, numloop;
	double sumlk, sumd1, sumd2, lkld1, lkld2, prod1, prod2, vari;
	double arc, arcold, arcdiff, arcpre, slk, sd1, sd2;
	dmattpmty tprob, tdif1, tdif2;
	dmatrix oprob, cprob;
	dvector tpb, td1, td2, opb, cpb;
#ifdef NUC
	double cpb0, cpb1, cpb2, cpb3;
#endif /* NUC */

	oprob = ip->iprob;
	cprob = kp->iprob;
	arc = arcpre = dis;
/*	if (Debug) { prprob(oprob); prprob(cprob); } */
	numloop = 30;
	for (it = 0; it < numloop; it++) {
		tdiffmtrx(arc, tprob, tdif1, tdif2);
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
		vari = 1.0 / fabs(lkld2);
		arcold = arc;
		arcdiff = - (lkld1 / lkld2);
		arc += arcdiff;
		if (arc > Mlimit && arcpre < 10.0) arc = Llimit;
		if (arc < Llimit) arc = Llimit;
		if (arc > Ulimit) arc = Ulimit;
		if (lkld2 > 0.0) {
			arc = Llimit;
			if (Debug || Debug_optn)
			fprintf(stderr,"mli: second derivative is positive! %8.3f\n",lkld2);
			break;
		}
		/*	printf("mli %3d %3d %8.3f %8.3f %12.5f %12.5f %10.1f\n",
			kp->num+1, it+1, arc, arcold, arcdiff, lkld1, lkld2); */
		if (fabs(arcold - arc) < DEPSILON) break;
	}
	/*	if (Debug) */
	if (fabs(arcdiff) > DEPSILON)
		printf("mli%4d%3d%8.3f%8.3f%8.3f%10.5f%9.5f%9.3f\n",
		kp->num+1, it+1, arc, arcpre, sqrt(vari), arcdiff, lkld1, lkld2);
	return arc;
} /* imledis */


#if ENJ
void
redmat(dmat, dij, psotu, otu, restsp, ii, jj, ns)
dmatrix dmat;
double dij;
Node **psotu;
ivector otu;
int restsp, ii, jj, ns;
{
	int k, kk;
	double dis, predis;
	Node *ip, *jp, *kp;

	ip = psotu[ii];
	jp = psotu[jj];
	if (ip->kinp->isop == NULL) /* external */
		partelkl(ip);
	else /* internal */
		partilkl(ip);
	if (jp->kinp->isop == NULL) /* external */
		partelkl(jp);
	else /* internal */
		partilkl(jp);
	prodpart(ip);

	for (k = 0; k < restsp; k++) {
		kk = otu[k];
		if (kk != ii && kk != jj) {
			predis = (dmat[ii][kk] + dmat[jj][kk] - dij) * 0.5;
			kp = psotu[kk];
			if (kp->kinp->isop == NULL) { /* external */
				if (predis < LOWERLIMIT) predis = LOWERLIMIT;
				dis = emledis(predis, ip, kp->kinp);
			} else { /* internal */
				if (predis < Llimit) predis = Llimit;
				dis = imledis(predis, ip, kp->kinp->isop);
			}
			/*
			printf("%3d%3d%3d",ip->kinp->num+1,jp->kinp->num+1,kp->kinp->num+1);
			printf(" %3d%3d%9.4f%9.4f\n", ii+1, kk+1, predis, dis);
			*/
			dmat[ii][kk] = dmat[kk][ii] = dis;
		}
		dmat[jj][kk] = dmat[kk][jj] = 0.0;
	}
} /* redmat */
#endif /* ENJ */



void
enjtree(tr, distan, ns, flag)
Tree *tr;
dmatrix distan;
int ns;
boolean flag;
{
	int i, j, ii, jj, kk, otui, otuj, nsp2, cinode, restsp;
	double dij, bix, bjx, bkx, sij, smax, dnsp2, dij2;
	ivector otu;
	dvector r;
	dmatrix dmat;
	Node **psotu, *cp, *ip, *jp, *kp;

	dmat = new_dmatrix(ns, ns);
	for (i = 0; i < ns; i++) {
			for (j = 0; j < ns; j++) dmat[i][j] = distan[i][j];
	}
	cinode = ns;
	nsp2 = ns - 2;
	dnsp2 = 1.0 / nsp2;
	r = new_dvector(ns);
	otu = new_ivector(ns);
	psotu = (Node **)new_npvector(ns);
	for (i = 0; i < ns; i++) {
		otu[i] = i;
		psotu[i] = tr->ebrnchp[i]->kinp;
	}

	for (restsp = ns; restsp > 3; restsp--) {

		for (i = 0; i < restsp; i++) {
			ii = otu[i];
			for (j = 0, sij = 0.0; j < restsp; j++) sij += dmat[ii][otu[j]];
			r[ii] = sij;
		}
		for (i = 0, smax = - DBL_MAX; i < restsp-1; i++) {
			ii = otu[i];
			for (j = i + 1; j < restsp; j++) {
				jj = otu[j];
				sij = ( r[ii] + r[jj] ) * dnsp2 - dmat[ii][jj]; /* max */
				/* printf("%3d%3d %9.3f %9.3f %9.3f\n",
					ii+1,jj+1,sij,r[ii],r[jj]); */
				if (!flag) sij = - sij;
				if (sij > smax) {
					smax = sij; otui = i; otuj = j;
				}
			}
		}

		ii = otu[otui];
		jj = otu[otuj];
		dij = dmat[ii][jj];
		dij2 = dij * 0.5;
		bix = (dij + r[ii]/nsp2 - r[jj]/nsp2) * 0.5;
		bjx = dij - bix;
		cp = tr->ibrnchp[cinode - ns];
		ip = psotu[ii];
		jp = psotu[jj];
		cp->isop = ip;
		ip->isop = jp;
		jp->isop = cp;
		ip->length += bix;
		jp->length += bjx;
		if (ip->kinp->isop == NULL) {
			if (ip->length < LOWERLIMIT) ip->length = LOWERLIMIT;
		} else {
			if (ip->length < Llimit) ip->length = Llimit;
		}
		if (jp->kinp->isop == NULL) {
			if (jp->length < LOWERLIMIT) jp->length = LOWERLIMIT;
		} else {
			if (jp->length < Llimit) jp->length = Llimit;
		}
		ip->kinp->length = ip->length;
		jp->kinp->length = jp->length;
		cp = cp->kinp;

#if		ENJ
		cp->length = 0.0;
		redmat(dmat, dij, psotu, otu, restsp, ii, jj, ns);
#else	/* ENJ */
		cp->length = - dij2;
		for (j = 0; j < restsp; j++) {
			kk = otu[j];
			if (kk != ii && kk != jj) {
				dij = (dmat[ii][kk] + dmat[jj][kk]) * 0.5;
				dmat[ii][kk] = dmat[kk][ii] = dij;
			}
			dmat[jj][kk] = dmat[kk][jj] = 0.0;
		}
#endif	/* ENJ */

		psotu[ii] = cp;
		psotu[jj] = NULL;
		Numibrnch++;
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
	bix = (dmat[ii][jj] + dmat[ii][kk] - dmat[jj][kk]) * 0.5;
	bjx = dmat[ii][jj] - bix;
	bkx = dmat[ii][kk] - bix;
	ip = psotu[ii];
	jp = psotu[jj];
	kp = psotu[kk];
	ip->isop = jp;
	jp->isop = kp;
	kp->isop = ip;
	ip->length += bix;
	jp->length += bjx;
	kp->length += bkx;
	if (ip->kinp->isop == NULL) {
		if (ip->length < LOWERLIMIT) ip->length = LOWERLIMIT;
	} else {
		if (ip->length < Llimit) ip->length = Llimit;
	}
	if (jp->kinp->isop == NULL) {
		if (jp->length < LOWERLIMIT) jp->length = LOWERLIMIT;
	} else {
		if (jp->length < Llimit) jp->length = Llimit;
	}
	if (kp->kinp->isop == NULL) {
		if (kp->length < LOWERLIMIT) kp->length = LOWERLIMIT;
	} else {
		if (kp->length < Llimit) kp->length = Llimit;
	}
	ip->kinp->length = ip->length;
	jp->kinp->length = jp->length;
	kp->kinp->length = kp->length;

	reroot(tr, tr->ebrnchp[ns-1]->kinp); /* !? */

	free_dvector(r);
	free_ivector(otu);
	free_npvector(psotu);
	free_dmatrix(dmat);
} /*_ enjtree */
