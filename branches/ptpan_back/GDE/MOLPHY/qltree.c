/*
 * qltree.c   Adachi, J.   1994.01.11
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "protml.h"


Infoqltree *
newinfoqltrees(n, maxbrnch)
int n;
int maxbrnch;
{
	int i;
	Infoqltree *info;
	double *v;

	info = (Infoqltree *)malloc((unsigned)n * sizeof(Infoqltree));
	if (info == NULL) maerror("1 in newinfoqltrees().");
	v = (double *)malloc((unsigned)(n * maxbrnch) * sizeof(double));
	if (v == NULL) maerror("2 in newinfoqltrees().");
	for (i = 0; i < n; i++, v+=maxbrnch)
		info[i].lengths = v;

	return info;
} /*_ newinfoqltrees */


Infoaddtree *
newinfoaddtree(buftree)
int buftree;
{
	Infoaddtree *info;
	cvector m;

	info = (Infoaddtree *) malloc(sizeof(Infoaddtree));
	if (info == NULL) maerror("1 in newinfoaddtree().");
	m = (cvector) malloc((unsigned)buftree * sizeof(char));
	if (m == NULL) maerror("2 in newinfoaddtree().");
	info->ltplgy = m;
	info->lklaprox = 0.0;
	info->frequency = 0;
	info->dp = NULL;

	return info;
} /*_ newinfoaddtree */


void initturn(tr)
Tree *tr;
{
	int i, j, mini, n;
	double dis, mindis;
	ivector turn;
	Node *dp, *ap;
	
	mindis = fabs(Distanmat[0][1] - Distanmat[Numspc-1][1]);
	mini = 1;
	for ( i = 2; i < Numspc - 1; i++) {
		dis = fabs(Distanmat[0][i] - Distanmat[Numspc-1][i]);
		if (dis < mindis) {
			mindis = dis;
			mini = i;
		}
	}
	turn = tr->bturn;
	turn[0] = 0;
	turn[1] = mini;
	turn[2] = Numspc - 1;
	for (i = 1, j = 3; i < Numspc - 1; i++) {
		if (i != mini)
			turn[j++] = i;
	}

	for (i = 0; i < Maxspc; i++) {
		n = turn[i];
		dp = tr->ebrnchp[i];
		ap = dp->kinp;
		dp->num = n;
		ap->num = n;
		dp->eprob = Seqconint[n];
	}
} /* initturn */


void randturn(tr)
Tree *tr;
{
	int i, j, temp, n;
	ivector turn;
	Node *dp, *ap;

	turn = tr->bturn;
	for (i = Maxspc - 1; i > 0; i--) {
		j = (int)(((i + 1) / (RANDOM_MAX + 1.0)) * rand());
		temp = turn[i];
		turn[i] = turn[j];
		turn[j] = temp;
	}
	if (turn[0] > turn[1]) { temp=turn[0]; turn[0]=turn[1]; turn[1]=temp; }
	if (turn[0] > turn[2]) { temp=turn[0]; turn[0]=turn[2]; turn[2]=temp; }
	if (turn[1] > turn[2]) { temp=turn[1]; turn[1]=turn[2]; turn[2]=temp; }

	for (i = 0; i < Maxspc; i++) {
		n = turn[i];
		dp = tr->ebrnchp[i];
		ap = dp->kinp;
		dp->num = n;
		ap->num = n;
		dp->eprob = Seqconint[n];
	}
} /* randturn */


void
convertdistan(tr, numspc, distanmat, distanvec)
dmatrix distanmat;
dvector distanvec;
Tree *tr;
int numspc;
{
	int i, j, k;
	ivector turn;

	turn = tr->bturn;
	for (i = 1, k = 0; i < numspc; i++) {
		for (j = 0; j < i; j++, k++)
			distanvec[k] =
				distanmat[turn[i]][turn[j]];
	}
}


void
praproxlkl2(fp, tr)
FILE *fp;
Tree *tr;
{
	fprintf(fp, "%.1f\t%.3f\t",tr->lklhd,tr->rssleast);
} /*_ praproxlkl2 */


int addotu(tr, cp, np, ip, cnspc)
Tree *tr;
Node *cp, *np, *ip;
int cnspc;
{
	Node *op;

	if (cp == tr->rootp) tr->rootp = ip;
	for (op = cp->isop->isop; op->isop != cp; op = op->isop)
		;
	ip->isop = cp->isop;
	op->isop = ip;
	ip->kinp->isop = cp;
	cp->isop = np;
	np->isop = ip->kinp;

	if (Debug) prcurtree(tr);
	pathing(tr);
	lslength(tr, Distanvec, cnspc+1);
	tr->lklhd = 0.0;
/*	initpartlkl(tr); */
/*	aproxlkl(tr); */
/*	if (Logfl_optn) {
		praproxlkl2(Logfp, tr);
		fputctopology(Logfp, tr);
	} */

	cp->isop = ip->isop;
	op->isop = cp;
	np->isop = NULL;
	ip->isop = NULL;
	ip->kinp->isop = NULL;
	if (ip == tr->rootp) tr->rootp = cp;
} /* addotu */


addotual(tr, cp, np, ip, lengs)
Tree *tr;
Node *cp, *np, *ip;
dvector lengs;
{
	Node *op, **bp;
	int i, j;

	if (cp == tr->rootp) tr->rootp = ip;
	for (op = cp->isop->isop; op->isop != cp; op = op->isop)
		;
	ip->isop = cp->isop;
	op->isop = ip;
	ip->kinp->isop = cp;
	cp->isop = np;
	np->isop = ip->kinp;

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
/*	for (i = 0; i < Numbrnch; i++) printf("%6.2f",lengs[i]); putchar('\n'); */
	if (Debug) prcurtree(tr);
	initpartlkl(tr);
	aproxlkl(tr);
	if (Logfl_optn) {
		praproxlkl2(Logfp, tr);
		fputctopology(Logfp, tr);
	}

	cp->isop = ip->isop;
	op->isop = cp;
	np->isop = NULL;
	ip->isop = NULL;
	ip->kinp->isop = NULL;
	if (ip == tr->rootp) tr->rootp = cp;
} /* addotual */


void
roundtree(tr, cnspc, infoqltrees, qhead, qtail)
Tree *tr;
int cnspc;
Infoqltree *infoqltrees, *qhead, *qtail;
{
	Node *cp, *rp, *np, *ip, *op, *ap, *maxlkp;
	Infoqltree *xp, *yp, *zp; 
	boolean added;
	int i, ntree;
	double res, minres, rate;
	dvector lengs;

	minres = 1e+36;
	qtail = qhead;
	np = tr->ebrnchp[cnspc]->kinp;
	ip = tr->ibrnchp[cnspc - 3]->kinp;
	ntree = 0;
	cp = rp = tr->rootp;
	do {
		added = FALSE;
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			addotu(tr, cp->kinp, np, ip, cnspc);
			added = TRUE;
			ap = cp->kinp;
			cp = cp->kinp; /* not descen */
		} else { /* internal node */
			if (cp->descen) {
				addotu(tr, cp->kinp, np, ip, cnspc); 
				ap = cp->kinp;
				added = TRUE;
			}
		}
		if (added) {
			res = tr->rssleast;
			if (res < qtail->residual || ntree < MAXQLBUF) {
				if (res < minres)
					minres = res;
				if (ntree < MAXQLBUF) {
					yp = &infoqltrees[ntree];
				} else if (res < qtail->residual) {
					yp = qtail;
					qtail = yp->up;
				}
				yp->residual = res;
				yp->ap = ap;
				lengs = yp->lengths;
				for (xp = qtail; res < xp->residual; zp = xp, xp = xp->up)
					;
				yp->up = xp;
				if (qtail == xp)
					qtail = yp;
				else
					zp->up = yp;
				for (i = 0; i < Numbrnch; i++)
					lengs[i] = Brnlength[i];
			/*	for (i = 0; i < Numbrnch; i++) printf("%6.2f",Brnlength[i]);
				putchar('\n');
				for (i = 0; i < Numbrnch; i++) printf("%6.2f",lengs[i]);
				putchar('\n'); */
			}
			ntree++;
		}
	} while (cp != rp);

	if (Logfl_optn) putc('\n', Logfp);
	if ((cnspc * 2 - 3) > MAXQLBUF)
		ntree = MAXQLBUF;
	else
		ntree = cnspc * 2 - 3;
	tr->lklmean = -1e+36;
	xp = qtail;
	while (xp != qhead) {
		ntree--;
		rate = xp->residual / minres; /* !? */
		if (rate < QRSRATE || ntree < 3) {
		/*	printf("%.3f %d\n", rate, ntree); */
			tr->rssleast = rate;
			addotual(tr, xp->ap, np, ip, xp->lengths);
			if (tr->lklhd > tr->lklmean) {
				maxlkp = xp->ap;
				tr->lklmean = tr->lklhd;
			}
		}
		xp = xp->up;
	}

	if (maxlkp == tr->rootp) tr->rootp = ip;
	for (op = maxlkp->isop->isop; op->isop != maxlkp; op = op->isop)
		;
	ip->isop = maxlkp->isop;
	op->isop = ip;
	ip->kinp->isop = maxlkp;
	maxlkp->isop = np;
	np->isop = ip->kinp;
#if 0
	if (Debug) prcurtree(tr);
	pathing(tr);
	lslength(tr, Distanvec, cnspc+1);
	if (Logfl_optn) fprintf(Logfp, "\t%.3f\n", tr->rssleast / minres); /* !? */
	initpartlkl(tr);
	aproxlkl(tr);
	if (Logfl_optn) praproxlkl2(Logfp, tr);
	if (Logfl_optn) fputctopology(Logfp, tr);
#endif
} /*_ roundtree */


void
qtreeinit(tr)
Tree *tr;
{
	Node *sp0, *sp1, *sp2;

	sp0 = tr->ebrnchp[0]->kinp;
	sp1 = tr->ebrnchp[1]->kinp;
	sp2 = tr->ebrnchp[2]->kinp;
	sp0->isop = sp1;
	sp1->isop = sp2;
	sp2->isop = sp0;
	tr->rootp = sp2;
} /*_ qtreeinit */


void
tableaddtree(head, numaddtree)
Infoaddtree *head;
int numaddtree;
{
	Infoaddtree *cp, *tail;

	printf("%d / %d  ", numaddtree, Cnotree);
	fputs(Modelname, stdout);
	fputs(" model", stdout);
	for (cp = head; cp != NULL; cp = cp->dp) {
		tail = cp;
	}
	printf("  approx ln L %.1f", head->lklaprox);
	printf(" ... %.1f", tail->lklaprox);
	printf("  diff %.1f\n", head->lklaprox - tail->lklaprox);
	for (cp = head; cp != NULL; cp = cp->dp) {
		if (Info_optn ) printf("%d\t%.1f ", cp->frequency, cp->lklaprox);
		fputs(cp->ltplgy, stdout);
		fputs(";\n", stdout);
	}
} /*_ tableaddtree */
