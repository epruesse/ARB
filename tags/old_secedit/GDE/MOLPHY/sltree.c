/*
 * sltree.c   Adachi, J.   1995.02.15
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa, All rights reserved.
 */

#define SLDEBUG 0

#include "protml.h"

Tree *
new_stree(maxspc, maxibrnch, numptrn, seqconint)
int maxspc, maxibrnch, numptrn;
imatrix seqconint;
{
	int n, i;
	Tree *tr;
	Node *dp, *up;

	tr = (Tree *) malloc(sizeof(Tree));
	if (tr == NULL) maerror("tr in new_tree().");
	tr->ebrnchp = (Node **) malloc((unsigned)maxspc * sizeof(Node *));
	if (tr->ebrnchp == NULL) maerror("ebrnchp in new_tree().");
	tr->ibrnchp = (Node **) malloc((unsigned)maxibrnch * sizeof(Node *));
	if (tr->ibrnchp == NULL) maerror("ibrnchp in new_tree().");
	tr->bturn = new_ivector(maxspc);
	for (n = 0; n < maxspc; n++) {
		tr->bturn[n] = n;
		dp = (Node *) malloc(sizeof(Node));
		if (dp == NULL) maerror("dp in new_tree().");
		up = (Node *) malloc(sizeof(Node));
		if (up == NULL) maerror("up in new_tree().");
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
		if (dp == NULL) maerror("dp in new_tree().");
		up = (Node *) malloc(sizeof(Node));
		if (up == NULL) maerror("up in new_tree().");
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
} /*_ new_stree */


Infosltree *
newinfosltrees(num, maxbrnch)
int num;
int maxbrnch;
{
	int i;
	Infosltree *info;
	double *v;

	info = (Infosltree *)malloc((unsigned)num * sizeof(Infosltree));
	if (info == NULL) maerror("1 in newinfosltrees().");
	v = (double *)malloc((unsigned)(num * maxbrnch) * sizeof(double));
	if (v == NULL) maerror("2 in newinfosltrees().");
	for (i = 0; i < num; i++, v+=maxbrnch)
		info[i].lengths = v;

	return info;
} /*_ newinfosltrees */


void
insertbranch(ibp, np)
Node *ibp, *np;
{
	np->isop = ibp->isop->isop;
	ibp->isop->isop = np->kinp;
	np->kinp->isop = ibp->isop;
	ibp->isop = np;
} /* insertbranch */


void
deletebranch(ibp, np)
Node *ibp, *np;
{
	np->kinp->isop->isop = np->isop;
	ibp->isop = np->kinp->isop;
	np->kinp->isop = NULL; /* redundancy */
	np->isop = NULL; /* redundancy */
} /* deletebranch */


void
movebranch(jbp, ip)
Node *jbp, *ip;
{
	Node *jp;

	jp = jbp->isop;
	jbp->isop = jp->isop;
	jp->isop = ip->isop;
	ip->isop = jp;
} /* movebranch */

void
removebranch(jbp, ip)
Node *jbp, *ip;
{
	Node *jp;

	jp = ip->isop;
	ip->isop = jp->isop;
	jp->isop = jbp->isop;
	jbp->isop = jp;
} /* removebranch */


void
subpathing(np)
Node *np;
{
	int i;
	Node *cp;
	boolean *npi, *cpi;

	for (i = 0, npi = np->paths; i < Maxspc; i++) *npi++ = FALSE;
	for (cp = np->isop; cp != np; cp = cp->isop) {
		npi = np->paths;
		cpi = cp->paths;
		for (i = 0; i < Maxspc; i++, npi++, cpi++) {
			if (*cpi == TRUE) *npi = *cpi;

		}
	}
/*	for (i=0; i<Maxspc; i++) printf("%1d",np->paths[i]);
	putchar('\n'); */
} /* subpathing */


void
copylength(tr, lengs)
Tree *tr;
dvector lengs;
{
	int i, j;
	Node **bp;

	bp = tr->ebrnchp;
	for (i = 0; i < Numspc; i++) {
		bp[i]->length = lengs[i];
		bp[i]->kinp->length = lengs[i];
	}
	bp = tr->ibrnchp;
	for (j = 0, i = Numspc; j < Numibrnch; j++, i++) {
		bp[j]->length = lengs[i];
		bp[j]->kinp->length = lengs[i];
	}
}


Node*
sdml(tr, op)
Tree *tr;
Node *op;
{
	Node *cp, *kp, *rp;
	int i, l, nconv, nconv2;
	double eps, lendiff;

	/* prtopology(tr); */
	kp = op->kinp;
	for (cp = op->isop; cp->isop != op; cp = cp->isop)
		;
	rp = cp;
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
	} while (cp != rp);


	prodpart(op->isop);
	mlibranch(op, 0.1, 5);
#if SLDEBUG
	printf("\n%2s", "");
	for (i = 0; i < Numbrnch; i++) printf("%5d",i+1); putchar('\n');
#endif
	kp->isop->kinp->descen = kp->isop->isop->kinp->descen = 2;

	for (l = 0, nconv = 0; l < MAXIT; l++) {
		if      (l == 0) eps = 1.0;
		else if (l == 1) eps = 0.5;
		else             eps = 0.1;
#if SLDEBUG
		printf("%2d", l+1);
		for (i = 0; i < Numspc; i++)
			printf("%5.0f",tr->ebrnchp[i]->length*100);
		for (i = 0; i < Numibrnch; i++)
			printf("%5.0f",tr->ibrnchp[i]->length*100);
		putchar('\n');
#endif
		cp = rp;
		do {
			cp = cp->isop->kinp;
			if (!(l == 0 && cp->kinp == op)) prodpart(cp->kinp->isop);
			if (cp->isop == NULL) { /* external node */
				cp = cp->kinp; /* not descen */
				lendiff = cp->length;
				mlebranch(cp, eps, 5);
				lendiff = fabs(lendiff - cp->length);
				lendiff < 0.1 ? (nconv++) : (nconv = 0);
				/* printf("e%3d%9.3f%9.3f\n", cp->num+1,cp->length,lendiff); */
			} else { /* internal node */
				if (cp->descen == 1) {
					partilkl(cp);
				} else if (cp->descen == 2 || !cp->descen) {
					if (cp->descen == 2 ) cp = cp->kinp;
					lendiff = cp->length;
					mlibranch(cp, eps, 5);
					lendiff = fabs(lendiff - cp->length);
					lendiff < 0.1 ? (nconv++) : (nconv = 0);
					/* printf("i%3d%9.3f%9.3f\n",
						cp->num+1,cp->length,lendiff); */
				}
			}
		} while (cp != rp);
		if (nconv >= 3) break;
	}
	kp->isop->descen ?
		(kp->isop->kinp->descen = 0) : (kp->isop->kinp->descen = 1);
	kp->isop->isop->descen ?
		(kp->isop->isop->kinp->descen = 0) : (kp->isop->isop->kinp->descen = 1);

	nconv = nconv2 = 0;
	Converg = FALSE;
	for (l = 0, Numit = 1; l < MAXIT; l++, Numit++) {
		if      (l == 0) eps = 1.0;
		else if (l == 1) eps = 0.5;
		else if (l == 2) eps = 0.1;
		else             eps = REPSILON;
#if SLDEBUG
		printf("%2d", l+1);
		for (i = 0; i < Numspc; i++)
			printf("%5.0f",tr->ebrnchp[i]->length*100);
		for (i = 0; i < Numibrnch; i++)
			printf("%5.0f",tr->ibrnchp[i]->length*100);
		putchar('\n');
#endif
		cp = rp;
		do {
			cp = cp->isop->kinp;
			prodpart(cp->kinp->isop);
			if (cp->isop == NULL) { /* external node */
				/* if (Debug) printf("mle %3d%3d\n",cp->num+1,cp->descen); */
				cp = cp->kinp; /* not descen */
				lendiff = cp->length;
				mlebranch(cp, eps, 5);
				lendiff = fabs(lendiff - cp->length);
				lendiff < REPSILON ? (nconv++)  : (nconv = 0);
				lendiff < 0.5      ? (nconv2++) : (nconv2 = 0);
			} else { /* internal node */
				/* if (Debug) printf("mli %3d%3d\n",cp->num+1,cp->descen); */
				if (cp->descen) {
					partilkl(cp);
				} else {
					lendiff = cp->length;
					mlibranch(cp, eps, 5);
					lendiff = fabs(lendiff - cp->length);
					lendiff < REPSILON ? (nconv++)  : (nconv = 0);
					lendiff < 0.5      ? (nconv2++) : (nconv2 = 0);
				}
			}
			if (nconv >= Numbrnch) goto convergence;
		} while (cp != rp);
	}
	if (nconv2 >= Numbrnch) Converg = 2;
	evallkl(cp);
	return rp;

convergence:
	Converg = TRUE;
	evallkl(cp);
	return cp;
} /*_ sdml */


void
decomposition(tr, n, infosltrees)
Tree *tr;
int n;
Infosltree *infosltrees;
{
	Node *rp, *np, *ibp, *jbp, *ip, *jp, *ibp2, *jbp2, *maxibp, *maxjbp, *op;
	double maxlklhd, maxaprlkl, res, rate, minres;
	Infosltree *head, *tail, *xp, *yp, *zp;
	int i, npair, ntree;
	dvector lengs;

/*	putchar('\n'); */

	rp = tr->rootp;
	np = tr->ibrnchp[n]->kinp;
	npair = 0;
	minres = 1.0e+37;
	head = &infosltrees[MAXSLBUF];
	tail = head;
	for(ibp = rp; ibp->isop != rp; ibp = ibp->isop) {
		ibp2 = ibp;
		ip = ibp->isop;
	/*	printf("ip:%3d\n", ip->num+1); */
		insertbranch(ibp, np);
		for(jbp = np; jbp != rp; jbp = jbp->isop) {
			jbp2 = jbp;
			jp = jbp->isop;
			if (jbp->isop == rp) tr->rootp = jbp;
			movebranch(jbp, ip);

			subpathing(np->kinp);
		/*	pathing(tr); */
			slslength(tr, Distanmat, Maxspc);
		/*	initpartlkl(tr);
			aproxlkl(tr);
			praproxlkl(tr);
			putctopology(tr); */

			res = tr->rssleast;
#if 0
			printf("%4d%4d%4d%4d %.3f\n",
				n, npair, ip->num+1, jp->num+1, res);
#endif
			if (npair < MAXSLBUF || res < tail->residual) {
				if (res < minres) minres = res;
				if (npair < MAXSLBUF) {
					yp = &infosltrees[npair];
				} else if (res < tail->residual) {
					yp = tail;
					tail = yp->up;
				}
				yp->residual = res;
				yp->ibp = ibp2;
				yp->jbp = jbp2;
				lengs = yp->lengths;
				for (i = 0; i < Numbrnch; i++) lengs[i] = Brnlength[i];
				for (xp = tail; res < xp->residual; zp = xp, xp = xp->up)
					;
				yp->up = xp;
				if (tail == xp)
					tail = yp;
				else
					zp->up = yp;
			}

			removebranch(jbp, ip);
			if (jbp->isop == rp) tr->rootp = rp;
			if (jbp == rp) rp = jbp->isop;
			npair++;
		}
		deletebranch(ibp, np);
	}

	if (npair < MAXSLBUF)
		ntree = npair;
	else
		ntree = MAXSLBUF;
	maxaprlkl = -1.0e+37;
	for (xp = tail; xp != head; xp = xp->up) {
		ntree--;
		rate = xp->residual / minres; /* !? */
		if (rate < SRSRATE || ntree < MAXSLBUF / 2) {
			tr->rssleast = rate;
			ibp = xp->ibp;
			ip = ibp->isop;
			insertbranch(ibp, np);
			jbp = xp->jbp;
			jp = jbp->isop;
			if (jbp->isop == rp) tr->rootp = jbp;
			movebranch(jbp, ip);
			subpathing(np->kinp);
		/*	pathing(tr); */
			copylength(tr, xp->lengths);
		/*	lslength(tr, Distanvec, Maxspc); */
			initpartlkl(tr);
			aproxlkl(tr);
			xp->lklaprox = tr->lklhd;
#if 0
			printf("%4d%4d%4d ", ntree, ip->num+1, jp->num+1);
			praproxlkl(tr);
			putctopology(tr);
#endif
			removebranch(jbp, ip);
			if (jbp->isop == rp) tr->rootp = rp;
			if (jbp == rp) rp = jbp->isop;
			deletebranch(ibp, np);

			if (tr->lklhd > maxaprlkl) maxaprlkl = tr->lklhd;
		}
	}

/*	putchar('\n'); */

	maxlklhd = -1.0e+37;
	for (xp = tail; xp != head; xp = xp->up) {
		rate = maxaprlkl - xp->lklaprox; /* !? */
		if (rate < 20) {
			tr->rssleast = rate;
			ibp = xp->ibp;
			ip = ibp->isop;
			insertbranch(ibp, np);
			jbp = xp->jbp;
			jp = jbp->isop;
			if (jbp->isop == rp) tr->rootp = jbp;
			movebranch(jbp, ip);
			subpathing(np->kinp);
		/*	pathing(tr); */
			copylength(tr, xp->lengths);
		/*	lslength(tr, Distanvec, Maxspc); */
			initpartlkl(tr);
			op = (Node *)mlikelihood(tr);
			mlvalue(tr, Infotrees);
			xp->lklaprox = tr->lklhd;
#if 0
			printf("%4d%4d\t", ip->num+1, jp->num+1);
			praproxlkl(tr);
			putctopology(tr);
#endif
		/*	prtopology(tr);
			resulttree(tr); */
			removebranch(jbp, ip);
			if (jbp->isop == rp) tr->rootp = rp;
			if (jbp == rp) rp = jbp->isop;
			deletebranch(ibp, np);

			if (tr->lklhd > maxlklhd) {
				maxibp = xp->ibp;
				maxjbp = xp->jbp;
				maxlklhd = tr->lklhd;
			}
		}
	}


	ip = maxibp->isop;
	insertbranch(maxibp, np);
	jp = maxjbp->isop;
/*	printf("%4d%4d%4d\t", n, ip->num + 1, jp->num + 1); */
	if (maxjbp->isop == rp) tr->rootp = maxjbp;
	movebranch(maxjbp, ip);
	subpathing(np->kinp);
/*	lslength(tr, Distanvec, Maxspc);
	initpartlkl(tr);
	op = (Node *)mlikelihood(tr);
	mlvalue(tr, Infotrees);
	praproxlkl(tr); */
#if 1
	putctopology(tr);
#endif
} /* decomposition */


void
stardecomp(tr, maxibrnch)
Tree *tr;
int maxibrnch;
{
	int i;
	Infosltree *infosltrees;

	infosltrees = (Infosltree *)newinfosltrees(MAXSLBUF + 1, Maxbrnch);
	infosltrees[MAXSLBUF].lklaprox = -1.0e+36;
	infosltrees[MAXSLBUF].residual = 0.0;
	for (i = 0; i < maxibrnch; i++) {
		Numibrnch++;
		Numbrnch++;
		decomposition(tr, i, infosltrees);
	}
} /* stardecomp */


dcube
new_dcubesym(nrow, ncol)
int nrow;
int ncol;
/* memory allocate a double cube */
{
	int i, j;
	dcube c;

	c = (dcube) malloc((unsigned)nrow * sizeof(dmatrix));
	if (c == NULL) maerror("1 in dcubesym().");
	*c = (dmatrix) malloc((unsigned)(nrow * nrow) * sizeof(dvector));
	if (*c == NULL) maerror("2 in dcubesym().");
	**c = (dvector) malloc((unsigned)(nrow*(nrow+1)/2 * ncol) * sizeof(double));
	if (**c == NULL) maerror("3 in dcubesym().");
	for (j = 1; j < nrow; j++) c[0][j] = c[0][j-1] + ncol;
	for (i = 1; i < nrow; i++) {
		c[i] = c[i-1] + nrow;
		for (j = 0; j < i; j++) c[i][j] = c[j][i];
		c[i][i] = c[i-1][nrow-1] + ncol;
		for (j = i + 1; j < nrow; j++) c[i][j] = c[i][j-1] + ncol;
	}
	return c;
}


void
free_dcubesym(c)
dcube c;
{
	free((char *) **c);
	free((char *) *c);
	free((char *) c);
}


void
ystardecomp(tr)
Tree *tr;
{
	Node *rp, *np, *op, *ibp, *jbp, *ip, *jp, *ibp2, *jbp2, *maxibp, *maxjbp;
	double lkl, maxlkl, minlkl, suml1, suml2, ldiff, sdlkl, z, minz, maxz, eps;
	double lk, llk;
	int i, j, k, l, ii, jj, npair, notu, notu2, nn1, maxi;
	Node **nodevec;
	ivector pairnum;
	dvector pairlkl, pairrel, alklptrn, mlklptrn;
	dmatrix lklmat, relmat, iprb, jprb;
	dcube lklcube;

	eps = 1.0; /* !? */
	rp = tr->rootp;
	for(ibp = rp, notu = 1; ibp->isop != rp; ibp = ibp->isop, notu++) ;
	nodevec = (Node **) malloc((unsigned)notu * sizeof(Node *));
	if (nodevec == NULL) maerror("nodevec in xstardecomp().");
	pairnum = new_ivector(notu);
	pairlkl = new_dvector(notu);
	pairrel = new_dvector(notu);
	lklmat = new_dmatrix(notu, notu);
	relmat = new_dmatrix(notu, notu);
	lklcube = new_dcubesym(notu, Numptrn);
	nn1 = (double)(Numsite / (Numsite-1));

	while (notu > 3) {

/*	printf("notu: %d\n", notu); */
/*
	for (i = 0, k = 1; i < notu; i++) {
		for (j = i; j < notu; j++) lklcube[i][j][Numptrn-1] = k++;
	}
	putchar('\n');
	for (i = 0; i < notu; i++) {
		for (j = 0; j < notu; j++) printf("%4.0f", lklcube[i][j][Numptrn-1]);
		putchar('\n');
	}
*/
	Numibrnch++; Numbrnch++;
	for (i = 0; i < notu; i++) pairlkl[i] = -1.0e+37;
	rp = tr->rootp;
	for(ip = rp->isop, i = 0; ip != rp; ip = ip->isop, i++) nodevec[i] = ip;
/*	printf("i: %d\n", i); */
	nodevec[i] = rp;
	np = tr->ibrnchp[Numibrnch - 1]->kinp;
	npair = 0;
	maxlkl = -1.0e+37;
	minlkl =  1.0e+37;

	initpartlkl(tr);
	if (notu < Numspc) {
		Alklptrn = lklcube[0][1];
		op = (Node *)mlikelihood(tr);
		mlvalue(tr, Infotrees);
		initpartlkl(tr);
	}
	regupartlkl(tr); /* !? */

	rp = tr->rootp;
	for(ibp = rp, ii = 0; ibp->isop != rp; ibp = ibp->isop, ii++) {
		ibp2 = ibp;
		ip = ibp->isop;
		for(jbp = ip, jj = ii + 1; jbp != rp; jbp = jbp->isop, jj++) {
			jbp2 = jbp;
			jp = jbp->isop;
			Alklptrn = lklcube[ii][jj];
			iprb = ip->iprob;
			jprb = jp->iprob;
			for (k = 0, lkl = 0.0; k < Numptrn;  k++) {
				for (l = 0, lk = 0.0; l < Tpmradix; l++) {
				/*
					if (notu < Numspc) printf("%3d%3d %3d %3d %20.7e%20.7e\n",
						ii,jj,k,l,iprb[k][l],jprb[k][l]);
				*/
					lk += Freqtpm[l] * iprb[k][l] * jprb[k][l];
				}
				/*
				if (notu < Numspc)
					printf("%3d%3d %3d %15.10f\n", ii,jj,k,lk);
				*/
				llk = log(lk);
				Alklptrn[k]  = llk;
				lkl += llk * Weight[k];
			}
		/*	lkl /= Numsite; */
			lklmat[ii][jj] = lkl;
			lklmat[jj][ii] = lkl;
			if (lkl > maxlkl) maxlkl = lkl;
			if (lkl < minlkl) minlkl = lkl;
			if (lkl > pairlkl[ii]) { pairnum[ii] = jj; pairlkl[ii] = lkl; }
			if (lkl > pairlkl[jj]) { pairnum[jj] = ii; pairlkl[jj] = lkl; }
			/*
			printf("%-4d%4d%3d  %.3f\n", npair,ip->num+1,jp->num+1,lkl);
			*/
			npair++;
		}
	}

	for (i = 0; i < notu; i++) {
		mlklptrn = lklcube[i][pairnum[i]];
		minz = 1.0e+37;
		for (j = 0; j < notu; j++) {
			if (i != j) {
				if (j == pairnum[i]) {
					relmat[i][j] = 0.0;
				} else {
					alklptrn = lklcube[i][j];
					for (suml1 = suml2 = 0.0, k = 0; k < Numptrn; k++) {
						ldiff = alklptrn[k] - mlklptrn[k];
						suml1 += ldiff * Weight[k];
						suml2 += ldiff * ldiff * Weight[k];
					}
					suml1 /= Numsite;
					sdlkl = sqrt( nn1 * (suml2 - suml1*suml1*Numsite) );
					z = (lklmat[i][pairnum[i]] - lklmat[i][j]) / sdlkl;
					relmat[i][j] = z;
					if (z < minz) minz = z;
				}
			}
		}
		pairrel[i] = minz;
	}
	printf("\n%2s", "");
	for (j = 0; j < notu; j++) printf("%4d", nodevec[j]->num+1); putchar('\n');
/*	printf("\n%2s", "");
	for (j = 0; j < notu; j++) printf("%4d", j + 1); putchar('\n'); */
	for (i = 0; i < notu; i++) {
		printf("%-2d", nodevec[i]->num+1);
		for (j = 0; j < notu; j++) {
			i != j ? printf("%4.0f", lklmat[i][j]-minlkl) : printf("%4s", "");
		} putchar('\n');
	}
	printf("\n%2s", "");
	for (j = 0; j < notu; j++) printf("%4d", nodevec[j]->num+1); putchar('\n');
	for (j = 0; j < notu; j++) {
		printf("%-2d", nodevec[j]->num+1);
		for (i = 0; i < notu; i++) {
			if (j == i) {
				printf("%4s", "");
			} else if (j == pairnum[i]) {
				if (i == pairnum[j])
					printf("%4s", "ML");
				else
					printf("%4s", "ml");
			} else {
				printf("%4.1f", relmat[i][j]);
			}
		} putchar('\n');
	}
	printf("%2s", "");
	for (i = 0; i < notu; i++) printf("%4.1f", pairrel[i]); putchar('\n');
	Numibrnch--; Numbrnch--;

	for (i = 0, maxz = 0.0; i < notu; i++) {
		j = pairnum[i];
		if ( i == pairnum[j] && i < j ) {
			z = (pairrel[i] > pairrel[j] ? pairrel[j] : pairrel[i]);
			printf("%3d", nodevec[i]->num+1);
			printf("%3d", nodevec[j]->num+1);
			printf("%6.2f%6.2f\n", pairrel[i], pairrel[j]);
			if (z > maxz) {
				maxz = z;
				maxi = i;
			}
		}
	}
	if ( notu == 4  && (maxi == 1 || maxi == 2) ) maxi = 0;
	for (i = 0, notu2 = notu; i < notu2; i++) {
		j = pairnum[i];
	/*	if ( i == pairnum[j] && i < j &&
			((pairrel[i] > eps && pairrel[j] > eps) ||
			(i == maxi && maxz < eps)) ) { */
		if ( i == pairnum[j] && i<j && (pairrel[i]>eps && pairrel[j]>eps) ) {
			Numibrnch++; Numbrnch++;
			np = tr->ibrnchp[Numibrnch - 1]->kinp;
			ip = nodevec[i];
			rp = tr->rootp;
			for(ibp = rp; ibp->isop != ip; ibp = ibp->isop) ;
			insertbranch(ibp, np);
			jp = nodevec[j];
			for(jbp = np; jbp->isop != jp; jbp = jbp->isop) ;
		/*	printf("%3d%3d%4d%3d\t", i, j, ip->num+1, jp->num+1); */
			if (jbp->isop == rp) tr->rootp = jbp;
			movebranch(jbp, ip);
			subpathing(np->kinp);
		/*	lslength(tr, Distanvec, Maxspc);
			initpartlkl(tr);
			op = (Node *)mlikelihood(tr);
			mlvalue(tr, Infotrees);
			praproxlkl(tr); */
			putctopology(tr);
			notu--;
			if (notu == 3) break;
		}
	/*	} */
	}
	if (notu == notu2) break;
	if (notu != 3) { putchar('\n'); prtopology(tr); }
	fflush(stdout);

	} /* while */

	free_dcubesym(lklcube);
	free_dmatrix(relmat);
	free_dmatrix(lklmat);
	free_dvector(pairrel);
	free_dvector(pairlkl);
	free_ivector(pairnum);
	free((Node *) nodevec);
} /* ystardecomp */


void
xstardecomp(tr)
Tree *tr;
{
	Node *rp, *np, *op, *ibp, *jbp, *ip, *jp, *ibp2, *jbp2, *maxibp, *maxjbp;
	double lkl, maxlkl, minlkl, suml1, suml2, ldiff, sdlkl, z, minz, maxz, eps;
	int nstep, i, j, k, k0, ii, jj, npair, notu, notu2, nn1, maxi, pairord0;
	Node **nodevec;
	ivector pairnum, pairord;
	dvector pairlkl, pairrel, alklptrn, mlklptrn, elenvec, ilenvec;
	dmatrix lklmat, relmat;
	dcube lklcube;

	elenvec = new_dvector(Maxspc);
	ilenvec = new_dvector(Maxibrnch);
	eps = 0.5; /* !? */
	rp = tr->rootp;
	for(ibp = rp, notu = 1; ibp->isop != rp; ibp = ibp->isop, notu++) ;
	nodevec = (Node **) malloc((unsigned)notu * sizeof(Node *));
	if (nodevec == NULL) maerror("nodevec in xstardecomp().");
	pairnum = new_ivector(notu);
	pairord = new_ivector(notu);
	pairlkl = new_dvector(notu);
	pairrel = new_dvector(notu);
	lklmat = new_dmatrix(notu, notu);
	relmat = new_dmatrix(notu, notu);
	lklcube = new_dcubesym(notu, Numptrn);
	nn1 = (double)(Numsite / (Numsite-1));
	nstep = 0;

	while (notu > 3) {

	if (Verbs_optn) fprintf(stderr, "%d OTUs:", notu);
	fmlength(tr, Distanmat, Maxspc);
	initpartlkl(tr);
	rp = (Node *)mlikelihood(tr);
	mlvalue(tr, Infotrees);
	if (Debug_optn) putctopology(tr);
	printf("#%d\n", nstep);
	prtopology(Ctree);
	/* resulttree(Ctree); */

	for (i = 0; i <  Numspc; i++)    elenvec[i] = tr->ebrnchp[i]->length;
	for (i = 0; i <= Numibrnch; i++) ilenvec[i] = tr->ibrnchp[i]->length;
	Numibrnch++; Numbrnch++;
	for (i = 0; i < notu; i++) pairlkl[i] = -1.0e+37;
	rp = tr->rootp;
	for(ip = rp->isop, i = 0; ip != rp; ip = ip->isop, i++) nodevec[i] = ip;
	nodevec[i] = rp;
	np = tr->ibrnchp[Numibrnch - 1]->kinp;
	npair = 0;
	maxlkl = -1.0e+37;
	minlkl =  1.0e+37;
	rp = tr->rootp;
	for(ibp = rp, ii = 0; ibp->isop != rp; ibp = ibp->isop, ii++) {
		ibp2 = ibp;
		ip = ibp->isop;
		insertbranch(ibp, np);
		for(jbp = np, jj = ii + 1; jbp != rp; jbp = jbp->isop, jj++) {
			if (Verbs_optn) fprintf(stderr, " %d", npair+1);
			jbp2 = jbp;
			jp = jbp->isop;
			if (jbp->isop == rp) tr->rootp = jbp;
			movebranch(jbp, ip);

			subpathing(np->kinp);
			for (i = 0; i < Numspc; i++) {
				tr->ebrnchp[i]->length = elenvec[i];
				tr->ebrnchp[i]->kinp->length = elenvec[i];
			}
			for (i = 0; i < Numibrnch; i++) {
				tr->ibrnchp[i]->length       = ilenvec[i];
				tr->ibrnchp[i]->kinp->length = ilenvec[i];
			}
			ip->length = ip->kinp->length = ip->length * 0.9;
			jp->length = jp->kinp->length = jp->length * 0.9;
			np->length = np->kinp->length = 1.0;
			Alklptrn = lklcube[ii][jj];
			op = (Node *)sdml(tr, np);
			mlvalue(tr, Infotrees);
		/*	putctopology(tr); */

			lkl = tr->lklhd;
			lklmat[ii][jj] = lkl;
			lklmat[jj][ii] = lkl;
			if (lkl > maxlkl) maxlkl = lkl;
			if (lkl < minlkl) minlkl = lkl;
			if (lkl > pairlkl[ii]) { pairnum[ii] = jj; pairlkl[ii] = lkl; }
			if (lkl > pairlkl[jj]) { pairnum[jj] = ii; pairlkl[jj] = lkl; }
			removebranch(jbp, ip);
			if (jbp->isop == rp) tr->rootp = rp;
			if (jbp == rp) rp = jbp->isop;
			npair++;
		}
		deletebranch(ibp, np);
	}

	for (i = 0; i < notu; i++) {
		mlklptrn = lklcube[i][pairnum[i]];
		minz = 1.0e+37;
		for (j = 0; j < notu; j++) {
			if (i != j) {
				if (j == pairnum[i]) {
					relmat[i][j] = 0.0;
				} else {
					alklptrn = lklcube[i][j];
					for (suml1 = suml2 = 0.0, k = 0; k < Numptrn; k++) {
						ldiff = alklptrn[k] - mlklptrn[k];
						suml1 += ldiff * Weight[k];
						suml2 += ldiff * ldiff * Weight[k];
					}
					suml1 /= Numsite;
					sdlkl = sqrt( nn1 * (suml2 - suml1*suml1*Numsite) );
					z = (lklmat[i][pairnum[i]] - lklmat[i][j]) / sdlkl;
					relmat[i][j] = z;
					if (z < minz) minz = z;
				}
			}
		}
		pairrel[i] = minz;
	}

	if (Info_optn) {
	printf("\n%2s", "");
	for (j = 0; j < notu; j++) printf("%4d", nodevec[j]->num+1); putchar('\n');
	for (i = 0; i < notu; i++) {
		printf("%-2d", nodevec[i]->num+1);
		for (j = 0; j < notu; j++) {
			i != j ? printf("%4.0f", lklmat[i][j]-minlkl) : printf("%4s", "");
		} putchar('\n');
	}
	} /* Info_optn */

	if (Info_optn) {
	printf("\n%2s", "");
	for (j = 0; j < notu; j++) printf("%4d", nodevec[j]->num+1); putchar('\n');
	for (j = 0; j < notu; j++) {
		printf("%-2d", nodevec[j]->num+1);
		for (i = 0; i < notu; i++) {
			if (j == i) {
				printf("%4s", "");
			} else if (j == pairnum[i]) {
				if (i == pairnum[j])
					printf("%4s", "ML");
				else
					printf("%4s", "ml");
			} else {
				printf("%4.0f", relmat[i][j] * 10.0);
			}
		} putchar('\n');
	}
	printf("%2s", "");
	for (i = 0; i < notu; i++) printf("%4.0f",pairrel[i]*10.0); putchar('\n');
	} /* Info_optn */

	Numibrnch--; Numbrnch--;

	for (i = 0, maxz = 0.0, pairord0 = notu; i < notu; i++) {
		j = pairnum[i];
		if ( i == pairnum[j] && i < j ) {
			z = (pairrel[i] > pairrel[j] ? pairrel[j] : pairrel[i]);
			if (Info_optn) {
				printf("%3d", nodevec[i]->num+1);
				printf("%3d", nodevec[j]->num+1);
				printf("%6.2f%6.2f\n", pairrel[i], pairrel[j]);
			}
			pairrel[i] = pairrel[j] = z;
			if (z > maxz) {
				maxz = z;
				maxi = i;
			}
			if (pairord0 == notu) {
				pairord0 = i;
				pairord[i] = notu;
			} else {
				for (k = k0 = pairord0; k < notu; k0 = k, k = pairord[k]) {
					if (z > pairrel[k]) {
						if (k == pairord0) {
							pairord[i] = k;
							pairord0 = i;
						} else {
							pairord[i] = k;
							pairord[k0] = i;
						}
						break;
					}
				}
				if (k == notu) {
					pairord[i] = k;
					pairord[k0] = i;
				}
			}
		}
	}

	/*
	for (k = pairord0; k < notu; k = pairord[k]) {
		printf("%3d%3d%9.3f\n", k+1, pairnum[k]+1, pairrel[k]);
	}
	printf("%3s%3s%9s%3d\n", "", "", "", pairord0+1);
	for (i = 0; i < notu; i++) {
		if (i < pairnum[i])
		printf("%3d%3d%9.3f%3d\n", i+1,pairnum[i]+1,pairrel[i],pairord[i]+1);
	}
	*/
	if ( notu == 4  && (maxi == 1 || maxi == 2) ) maxi = 0;
	for (i = pairord0, notu2 = notu; i < notu2; i = pairord[i]) {
		j = pairnum[i];
		if (pairrel[i] > eps || i == pairord0) {
			np = tr->ibrnchp[Numibrnch]->kinp;
			Numibrnch++; Numbrnch++;
			np->length = np->kinp->length = 10.0;
			ip = nodevec[i];
			rp = tr->rootp;
			for(ibp = rp; ibp->isop != ip; ibp = ibp->isop) ;
			insertbranch(ibp, np);
			jp = nodevec[j];
			for(jbp = np; jbp->isop != jp; jbp = jbp->isop) ;
		/*	printf("%3d%3d%4d%3d\t", i, j, ip->num+1, jp->num+1); */
			if (jbp->isop == rp) tr->rootp = jbp;
			movebranch(jbp, ip);
			subpathing(np->kinp);
			/* putctopology(tr); */
			notu--;
			if (notu == 3) break;
		}
	}
	if (notu == notu2) break;
	fflush(stdout);
	if (Verbs_optn) fprintf(stderr, "\n", notu);
	nstep++;

	} /* while */

	reroot(tr, tr->ebrnchp[Maxspc-1]->kinp);
	free_dcubesym(lklcube);
	free_dmatrix(relmat);
	free_dmatrix(lklmat);
	free_dvector(pairrel);
	free_dvector(pairlkl);
	free_ivector(pairord);
	free_ivector(pairnum);
	free((Node *) nodevec);
	free_dvector(elenvec);
	free_dvector(ilenvec);
} /* xstardecomp */
