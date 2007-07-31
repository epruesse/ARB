/*
 * altree.c   Adachi, J.   1995.09.22
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "protml.h"

Tree *
new_atree(maxspc, maxibrnch, numptrn, seqconint)
int maxspc, maxibrnch, numptrn;
imatrix seqconint;
{
	int n, i;
	Tree *tr;
	Node *dp, *up;

	tr = (Tree *) malloc(sizeof(Tree));
	if (tr == NULL) maerror("tr in new_atree().");
	tr->ebrnchp  = (Node **) malloc((unsigned)maxspc * sizeof(Node *));
	if (tr->ebrnchp == NULL) maerror("ebrnchp in new_atree().");
	tr->ibrnchp  = (Node **) malloc((unsigned)maxibrnch * sizeof(Node *));
	if (tr->ibrnchp == NULL) maerror("ibrnchp in new_atree().");
	for (n = 0; n < maxspc; n++) {
		dp = (Node *) malloc(sizeof(Node));
		if (dp == NULL) maerror("dp in new_atree().");
		up = (Node *) malloc(sizeof(Node));
		if (up == NULL) maerror("up in new_atree().");
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
		dp->iprob = NULL;
		up->eprob = NULL;
		up->iprob = new_dmatrix(numptrn, Tpmradix);
		tr->ebrnchp[n] = dp;
	}
	tr->rootp = NULL;
	return tr;
} /*_ new_atree */


Node *
new_dnode()
{
	Node *dp;
	int i;

	dp = (Node *) malloc(sizeof(Node));
	if (dp == NULL) maerror("dp in new_dnode().");
	dp->isop = NULL;
	dp->kinp = NULL;
	dp->descen = TRUE;
	dp->num = Maxbrnch;
	dp->length = 0.0;
	dp->lklhdl = 0.0;
	dp->paths = new_ivector(Maxspc);
	for (i = 0; i < Maxspc; i++) dp->paths[i] = 0;
	dp->eprob = NULL;
	dp->iprob = new_dmatrix(Numptrn, Tpmradix);
	return dp;
} /*_ new_dnode */


Node *
new_anode()
{
	Node *ap;

	ap = (Node *) malloc(sizeof(Node));
	if (ap == NULL) maerror("ap in new_anode().");
	ap->isop = NULL;
	ap->kinp = NULL;
	ap->descen = FALSE;
	ap->num = Maxbrnch;
	ap->length = 0.0;
	ap->lklhdl = 0.0;
	ap->paths = NULL;
	ap->eprob = NULL;
	ap->iprob = new_dmatrix(Numptrn, Tpmradix);
	return ap;
} /*_ new_anode */


Node ***
new_nodematrix(nrow, ncol)
int nrow;
int ncol;
/* memory allocate a node matrix */
{
	int i;
	Node ***m;

	m = (Node ***) malloc((unsigned)nrow * sizeof(Node **));
	if (m == NULL) maerror("1 in nodematrix().");
	*m = (Node **) malloc((unsigned)(nrow * ncol) * sizeof(Node *));
	if (*m == NULL) maerror("2 in nodematrix().");
	for (i = 1; i < nrow; i++) m[i] = m[i-1] + ncol;
	return m;
}


void
free_nodematrix(m)
Node ***m;
{
	free((char *) *m);
	free((char *) m);
}


#if 1
void 
aproxlkl(tr)
Tree *tr;
{
	int i, k;
	Node *cp;
	double sumlk, lklhd;
	dmatrix prob1, prob2, prob3;

	cp = tr->rootp;
	lklhd = 0.0;
	if (cp->isop->isop->isop == cp) { /* binary tree */
		prob1 = cp->iprob;
		prob2 = cp->isop->iprob;
		prob3 = cp->isop->isop->iprob;
		for (k = 0; k < Numptrn; k++) {
			sumlk = 0.0;
			for (i = 0; i < Tpmradix; i++)
				sumlk += Freqtpm[i] * prob1[k][i] * prob2[k][i] * prob3[k][i];
		/*	printf("%3d%23.20f%10.3f%4d\n", k,sumlk,log(sumlk),Weight[k]); */
			lklhd += log(sumlk) * Weight[k];
		}
	} else { /* multiway tree */
		prodpart(cp->isop);
		prob1 = cp->iprob;
		prob2 = cp->isop->iprob;
		for (k = 0; k < Numptrn; k++) {
			sumlk = 0.0;
			for (i = 0; i < Tpmradix; i++)
				sumlk += Freqtpm[i] * prob1[k][i] * prob2[k][i];
		/*	printf("%3d%23.20f%10.3f%4d\n", k,sumlk,log(sumlk),Weight[k]); */
			lklhd += log(sumlk) * Weight[k];
		}
	}
	tr->aproxl = lklhd;
	tr->lklhd  = lklhd; /* ?!?!?! */
	return;
} /*_ aproxlkl */

#else
void 
aproxlkl(tr)
Tree *tr;
{
	int i, k;
	Node *cp, *rp;
	double sumlk, lklhd;
	dmatrix prob1, prob2, prob3;

	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp = cp->kinp; /* not descen */
			partelkl(cp);
		} else { /* internal node */
			if (!cp->descen) {
				prodpart(cp->kinp->isop);
				partilkl(cp);
			}
		}
	} while (cp != rp);

	lklhd = 0.0;
	if (cp->isop->isop->isop == cp) { /* binary tree */
		prob1 = cp->iprob;
		prob2 = cp->isop->iprob;
		prob3 = cp->isop->isop->iprob;
		for (k = 0; k < Numptrn; k++) {
			sumlk = 0.0;
			for (i = 0; i < Tpmradix; i++)
				sumlk += Freqtpm[i] * prob1[k][i] * prob2[k][i] * prob3[k][i];
		/*	printf("%3d%23.20f%10.3f%4d\n", k,sumlk,log(sumlk),Weight[k]); */
			lklhd += log(sumlk) * Weight[k];
		}
	} else { /* multiway tree */
		prodpart(cp->isop);
		prob1 = cp->iprob;
		prob2 = cp->isop->iprob;
		for (k = 0; k < Numptrn; k++) {
			sumlk = 0.0;
			for (i = 0; i < Tpmradix; i++)
				sumlk += Freqtpm[i] * prob1[k][i] * prob2[k][i];
		/*	printf("%3d%23.20f%10.3f%4d\n", k,sumlk,log(sumlk),Weight[k]); */
			lklhd += log(sumlk) * Weight[k];
		}
	}
	tr->lklhd = lklhd;
	return;
} /*_ aproxlkl */
#endif


void
praproxlkl(tr)
Tree *tr;
{
	printf("%.1f\t%.1f\t%d\t",tr->lklhd,tr->tbldis,Cnotree+1); /* offset */
} /*_ praproxlkl */


void
aproxtree(tr, ntr)
Tree *tr;
int ntr;
{
	int index;
	double lkl, tbl;
	Infoaltree *cp, *bp, *op;
	static num = 0;

	if (Verbs_optn) {
		if (ntr % Numverbs == 0) {
			fprintf(stderr, " %d", ntr+1);
#if __STDC__ && DIFFTIME
			Ct1 = time(NULL);
			fprintf(stderr, "(%.0fs)", difftime(Ct1, Ct0));
#endif
		}
	}
	pathing(tr);
	slslength(tr, Distanmat, Numspc);
	tbl = tr->tbldis;
	index = (int)((tbl - Mintbldm) * Tblcoef);
	if (index < 0) {
		Tblunder++;
	} else if (index < NUMTBLBIN) {
		Tblbin[index]++;
	} else {
		Tblover++;
	}

	if (!Mevol_optn) {
		if (tbl > Basetbldm) return;
		initpartlkl(tr);
		aproxlkl(tr);
	}
	if (Aprox_optn) {
		praproxlkl(tr);
		putctopology(tr);
	}
	if (Mevol_optn) { /* Minimum Evolution */
		lkl = -tbl;
	} else { /* approximate ln likelihood */
		lkl = tr->aproxl;
	}
	if (num < Maxaltree) {
		Numaltree = num + 1;
		Infoaltrees[num].lklaprox = lkl;
		Infoaltrees[num].tbl = tbl;
		strctree(tr, Infoaltrees[num].ltplgy);
		op = &Infoaltrees[num];
		if (Info_optn)
			printf("%.1f\t%d\t%d\t%s\n", op->lklaprox, ntr,num, op->ltplgy);
		for (bp = &Atail, cp = bp->up; lkl > cp->lklaprox; bp = cp, cp = cp->up)
			;
		op->up = cp;
		bp->up = op;
		num++;
	} else if (lkl > Atail.up->lklaprox) {
		op = Atail.up;
		Atail.up = op->up;
		op->lklaprox = lkl;
		op->tbl = tbl;
		strctree(tr, op->ltplgy);
		if (Info_optn)
			printf("%.1f\t%d\t%d\t%s\n", op->lklaprox, ntr,num, op->ltplgy);
		for (bp = &Atail, cp = bp->up; lkl > cp->lklaprox; bp = cp, cp = cp->up)
			;
		op->up = cp;
		bp->up = op;
	}
	/*	if (ntr % 10000 == 0) putctopology(tr); */
} /*_ aproxtree */


void autoconstruction();

void 
wedge(tr, onode, poolnode, addposition, poolorder, op)
Tree *tr;
int onode;
Node **poolnode, **addposition;
ivector poolorder;
Node *op;
{
	Node *cp, *dp;
	int i, istart;

	cp = poolnode[onode];
	if (addposition[onode] != NULL) { /* internal node */
		if (cp->isop != NULL) { /* binary tree */
			dp = op->kinp;
			op->kinp = cp->isop;
			cp->isop->kinp = op;
			dp->kinp = cp->isop->isop;
			cp->isop->isop->kinp = dp;
			op->paths = cp->isop->paths;
			cp->isop->isop->paths = dp->paths;
			for (istart = onode + 1; istart < Numspc; istart++) {
				if (poolorder[onode] != poolorder[istart])
					break;
			}
			for (i = istart; i < Numspc; i++) {
				if (op == addposition[i])
					addposition[i] = dp->kinp;
			}
		} else { /* multiway tree */
			cp->isop = op->isop;
			op->isop = cp;
			dp = op->kinp;
		}
	} else { /* root */
		dp = cp->isop->kinp;
		if (op == tr->rootp)
			tr->rootp = dp;
		dp->isop = op->isop;
		op->isop->isop->isop = dp;
		op->isop = cp;
		cp->isop->isop = op;
	}
	if (onode < Numspc - 1) {
#if 0
		/* evaluate each subtrees */
		if (onode < 6) putctopology(tr);
#endif
		/* printf("%3d%3d%3d%3d ",
			onode,dp->num+1,cp->kinp->num+1,cp->isop->num+1);
		putctopology(tr); */
		if (addposition[onode + 1] == NULL)
			autoconstruction(tr, onode + 1, poolnode, addposition, poolorder);
		else
			wedge(tr, onode+1, poolnode, addposition, poolorder, addposition[onode+1]);
	} else if (onode = Numspc - 1) {
	/*	Numibrnch = Maxibrnch;
		Numbrnch = Maxbrnch; */
		aproxtree(tr, Cnotree);
		Cnotree++;
	}
	if (addposition[onode] != NULL) { /* internal node */
		if (op->isop != cp) { /* binary tree */
			for (i = istart; i < Numspc; i++) {
				if (dp->kinp == addposition[i])
					addposition[i] = op;
			}
			cp->isop->kinp = NULL;
			cp->isop->isop->kinp = NULL;
			op->kinp = dp;
			dp->kinp = op;
			op->paths = dp->paths;
		} else { /* multiway tree */
			op->isop = cp->isop;
			cp->isop = NULL;
		}
	} else { /* root */
		if (dp == tr->rootp)
			tr->rootp = op;
		dp->isop->isop->isop = op;
		op->isop = dp->isop;
		cp->isop->isop = cp;
		dp->isop = NULL;
	}
	if (op->kinp->isop == NULL) {
		return;
	} else {
		cp = op->kinp->isop;
		while (cp != op->kinp) {
			wedge(tr, onode, poolnode, addposition, poolorder,  cp);
			cp = cp->isop;
		}
	}
} /*_ wedge */


void 
autoconstruction(tr, onode, poolnode, addposition, poolorder)
Tree *tr;
int onode;
Node **poolnode, **addposition;
ivector poolorder;
{
	Node *cp;

	cp = tr->rootp;
	do {
		cp = cp->isop;
		wedge(tr, onode, poolnode, addposition, poolorder, cp);
	} while (cp != tr->rootp);
} /*_ autoconstruction */


Node *
inbranode(tr, cpp, nenode, numorder, poolnode2, st)
Tree *tr;
char **cpp;
int *nenode;
int numorder;
Node ***poolnode2;
cvector st;
{
	Node *xp, *yp, *np;
	int i, k, n, dvg;
	char *idp, ident[MAXWORD];

	numorder++;
	if (**cpp == ' ') (*cpp)++;
	if (**cpp == '{') { /* internal node of binary tree */
		if (Debug) printf("inbranode'{': %c\n", **cpp);
		(*cpp)++;
		xp = inbranode(tr, cpp, nenode, numorder, poolnode2, st); /*first node*/
		dvg = 0;
		for (n = 0; poolnode2[numorder][n] != NULL; n++)
			;
		k = 1;
		while (**cpp != '}') { /* second node and others */
			np = inbranode(tr, cpp, nenode, numorder, poolnode2, st);
			np->isop = new_dnode();
			np->isop->isop = new_anode();
			np->isop->isop->isop = np;
			poolnode2[numorder][n + dvg] = np;
			np->isop->kinp = xp; /* add position */
			dvg++;
			Numtplgy *= k; /* printf("Numtplgy: %.1f\n", Numtplgy); */
			k += 2;
		}
		if (Debug) printf("inbranode'}': %c\n", **cpp);
		poolnode2[numorder][n + dvg] = NULL;
		if (dvg < 1) {
			fprintf(stderr, "ERROR, constrained tree:\n%s\n", st);
			fprintf(stderr, "redundancy or unnecessary \"{}\" !\n");
			exit(1);
		}
		(*cpp)++;
		(*nenode)++;
		return xp;
	} else if  (**cpp == '(') { /* internal node of multiway tree */
		if (Debug) printf("inbranode'(': %c\n", **cpp);
		(*cpp)++;
		xp = inbranode(tr, cpp, nenode, numorder, poolnode2, st); /*first node*/
		dvg = 0;
		for (n = 0; poolnode2[numorder][n] != NULL; n++)
			;
		if (**cpp != ')') { /* second node */
			np = inbranode(tr, cpp, nenode, numorder, poolnode2, st);
			np->isop = new_dnode();
			np->isop->isop = new_anode();
			np->isop->isop->isop = np;
			poolnode2[numorder][n + dvg] = np;
			np->isop->kinp = xp; /* add position */
			dvg++;
		}
		while (**cpp != ')') { /* third node and others */
			yp = np;
			np = inbranode(tr, cpp, nenode, numorder, poolnode2, st);
			np->isop = NULL;
			poolnode2[numorder][n + dvg] = np;
			np->kinp->isop = yp; /* add position (kinp) */
			dvg++;
		}
		if (Debug) printf("inbranode')': %c\n", **cpp);
		poolnode2[numorder][n + dvg] = NULL;
		if (dvg < 1) {
			fprintf(stderr, "ERROR, constrained tree:\n%s\n", st);
			fprintf(stderr, "redundancy or unnecessary \"()\" !\n");
			exit(1);
		}
		(*cpp)++;
		(*nenode)++;
		return xp;

	} else if (isalnum(**cpp)) { /* external node */
		if (Debug) printf("external: %c\n", **cpp);
		for (idp = ident; **cpp!=' ' && **cpp!='(' && **cpp!=')'
			&& **cpp!='{' && **cpp!='}'; (*cpp)++) {
			*idp++ = **cpp;
			if (Debug) putchar(**cpp);
		}
		*idp = '\0';
		if (Debug) putchar('\n');
		for (i = 0; i < Numspc; i++) {
			/* puts(Identif[i]); */
			if (!strcmp(ident, Identif[i])) {
				return tr->ebrnchp[i]->kinp;
			}
		}
		fprintf(stderr, "ERROR, constrained tree:\n%s\n", st);
		fprintf(stderr, "ERROR, abnormal identifier(OTU name): %s !\n", ident);
		exit(1);
	} else {
		fprintf(stderr, "ERROR, constrained tree:\n%s\n", st);
		fprintf(stderr, "ERROR, abnormal character: %s\n", **cpp);
		exit(1);
	}
	return NULL;
} /*_ inbranode */


void
streeinit(tr, strtree, poolnode, addposition, poolorder)
Tree *tr;
cvector strtree;
Node **poolnode, **addposition;
ivector poolorder;
{
	char *chp;
	int i, j, k, nenode, ninode, dvg, numorder;
	Node *sp0, *sp1, *sp2, *np, ***poolnode2;

	poolnode2 = new_nodematrix(Numspc, Numspc);
	nenode = 0;
	numorder = 0;
	for (i = 0; i < Numspc; i++) {
		poolnode2[i][0] = NULL;
		poolnode[i] = NULL;
		addposition[i] = NULL;
		poolorder[i] = 0;
	}

	chp = strtree;
	if (*chp == '{') { /* binary */
		chp++;
		sp0 = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
		sp1 = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
		dvg = 0;
		k = 3;
		while (*chp != '}') { /* fourth node and others */
			np = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
			if (*chp == '}') break;
			np->isop = new_dnode();
			np->isop->kinp = new_anode();
			np->isop->kinp->paths = np->isop->paths;
			np->isop->isop = np;
			np->isop->kinp->kinp = np->isop;
			poolnode2[0][dvg] = np;
			dvg++;
			Numtplgy *= k;
			k += 2;
		}
		poolnode2[0][dvg] = NULL;
		sp2 = np;
		sp0->isop = sp1;
		sp1->isop = sp2;
		sp2->isop = sp0;
		tr->rootp = sp2;
	} else if (*chp == '(') { /* multiway */
		chp++;
		sp0 = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
		sp1 = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
		sp2 = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
		sp0->isop = sp1;
		sp1->isop = sp2;
		sp2->isop = sp0;
		tr->rootp = sp2;
		dvg = 0;
		while (*chp != ')') { /* fourth node and others */
			np = inbranode(tr, &chp, &nenode, numorder, poolnode2, strtree);
			np->isop = NULL;
			poolnode2[0][dvg] = np;
			np->kinp->isop = sp2; /* add position (kinp) */
			dvg++;
		}
		poolnode2[0][dvg] = NULL;
	} else {
		fprintf(stderr, "ERROR, constrained tree:\n%s\n", strtree);
		fprintf(stderr, "ERROR, abnormal character: %s\n", *chp);
		exit(1);
	}

	if (Debug_optn) {
		putchar('\n');
		printf("    %4d%4d%4d\n", sp0->num+1, sp1->num+1, sp2->num+1);
		for (i = 0; i < Numspc; i++) {
			printf("%4d", i);
			if (poolnode2[i][0] != NULL) {
				for (j = 0; j < Numspc; j++) {
					if (poolnode2[i][j] != NULL)
						printf("%4d", poolnode2[i][j]->num+1);
					else
						break;
				} putchar('\n');
			} else {
				printf(" nul\n");
			}
		}
	}

	poolnode[0] = sp0;
	poolnode[1] = sp1;
	poolnode[2] = sp2;
	addposition[0] = NULL;
	addposition[1] = NULL;
	addposition[2] = NULL;
	for (k = 3, ninode = 0, i = 0; i < Numspc; i++) {
		for (j = 0; j < Numspc && poolnode2[i][j] != NULL; j++) {
			poolorder[k] = i;
			poolnode[k] = poolnode2[i][j];
			if (poolnode[k]->isop != NULL) { /* binary tree */
				poolnode[k]->isop->num = ninode + Maxspc;
				if (i == 0) { /* root */
					addposition[k] = NULL;
				} else { /* not root */
					addposition[k] = poolnode[k]->isop->kinp;
					poolnode[k]->isop->kinp = NULL;
				}
				tr->ibrnchp[ninode] = poolnode[k]->isop;
				ninode++;
			} else { /* multiway tree */
				addposition[k] = poolnode[k]->kinp->isop;
				poolnode[k]->kinp->isop = NULL;
			}
			k++;
		}
	}
	Numibrnch = ninode;
	Numbrnch = Numspc + ninode;

	if (Debug_optn) {
		putchar('\n');
		for (i = 0; i < k; i++) {
			printf("%4d ", i);
			fputid(stdout, Identif[poolnode[i]->kinp->num], 10);
			printf("%4d", poolnode[i]->kinp->num+1);
			printf("%4d", poolnode[i]->num+1);
			if (addposition[i] != NULL) {
				printf("%4d", addposition[i]->kinp->num+1);
				printf("%4d", addposition[i]->num+1);
			} else {
				fputs(" nul", stdout);
				fputs(" nul", stdout);
			}
			printf(" %4d ", poolorder[i]);
			if (poolnode[i]->isop != NULL) {
				np = poolnode[i];
				while((np = np->isop) != poolnode[i]) {
					printf("%4d", np->num+1);
				}
			} putchar('\n');
		}
		printf("Numbrnch =%4d\n", Numbrnch);
		printf("Numibrnch =%4d\n", Numibrnch);
	}
	free_nodematrix(poolnode2);
} /*_ streeinit */


void
atreeinit(tr, poolnode, addposition, poolorder)
Tree *tr;
Node **poolnode, **addposition;
ivector poolorder;
{
	char *chp;
	char linebuf[BUFLINE];
	int i, j, k, nenode, ninode, dvg, numorder, mini;
	double dis, mindis;
	Node *sp0, *sp1, *sp2, *np;
	Node ***poolnode2;

	mindis = fabs(Distanmat[0][1] - Distanmat[Numspc-1][1]);
	mini = 1;
	for ( i = 2; i < Numspc - 1; i++) {
		dis = fabs(Distanmat[0][i] - Distanmat[Numspc-1][i]);
		if (dis < mindis) {
			mindis = dis;
			mini = i;
		}
	}

	poolnode2 = new_nodematrix(Numspc, Numspc);
	nenode = 0;
	numorder = 0;
	for (i = 0; i < Numspc; i++) {
		poolnode2[i][0] = NULL;
		poolnode[i] = NULL;
		addposition[i] = NULL;
		poolorder[i] = 0;
	}

	strcpy(linebuf, Identif[0]);
	strcat(linebuf, " ");
	chp = linebuf;
	sp0 = inbranode(tr, &chp, &nenode, numorder, poolnode2, linebuf);
	strcpy(linebuf, Identif[mini]);
	strcat(linebuf, " ");
	chp = linebuf;
	sp1 = inbranode(tr, &chp, &nenode, numorder, poolnode2, linebuf);
	strcpy(linebuf, Identif[Numspc-1]);
	strcat(linebuf, " ");
	chp = linebuf;
	sp2 = inbranode(tr, &chp, &nenode, numorder, poolnode2, linebuf);
	sp0->isop = sp1;
	sp1->isop = sp2;
	sp2->isop = sp0;
	tr->rootp = sp2;
	dvg = 0;
	for ( i = 1; i < Numspc - 1; i++) {
		if (i != mini) {
			strcpy(linebuf, Identif[i]);
			strcat(linebuf, " ");
			chp = linebuf;
			np = inbranode(tr, &chp, &nenode, numorder, poolnode2, linebuf);
			np->isop = new_dnode();
			np->isop->kinp = new_anode();
			np->isop->kinp->paths = np->isop->paths;
			np->isop->isop = np;
			np->isop->kinp->kinp = np->isop;
			poolnode2[0][dvg] = np;
			dvg++;
		}
	}
	poolnode2[0][dvg] = NULL;

	if (Debug_optn) {
		putchar('\n');
		printf("%4d%4d%4d\n", sp0->num+1, sp1->num+1, sp2->num+1);
		for (j = 0; j < Numspc && poolnode2[0][j] != NULL; j++)
			printf("%4d", poolnode2[0][j]->num+1);
		putchar('\n');
	}

	poolnode[0] = sp0;
	poolnode[1] = sp1;
	poolnode[2] = sp2;
	addposition[0] = NULL;
	addposition[1] = NULL;
	addposition[2] = NULL;
	k = 3;
	ninode = 0;
	for (j = 0; j < Numspc && poolnode2[0][j] != NULL; j++) {
		poolorder[k] = 0;
		poolnode[k] = poolnode2[0][j];
		poolnode[k]->isop->num = ninode + Maxspc;
		addposition[k] = NULL;
		tr->ibrnchp[ninode] = poolnode[k]->isop;
		ninode++;
		k++;
	}
	Numbrnch = ninode;

	if (Debug_optn) {
		putchar('\n');
		for (i = 0; i < k; i++) {
			printf("%4d ", i);
			fputid(stdout, Identif[poolnode[i]->kinp->num], 10);
			printf("%4d", poolnode[i]->kinp->num+1);
			printf("%4d", poolnode[i]->num+1);
			if (addposition[i] != NULL) {
				printf("%4d", addposition[i]->kinp->num+1);
				printf("%4d", addposition[i]->num+1);
			} else {
				fputs(" nul", stdout);
				fputs(" nul", stdout);
			}
			printf(" %4d ", poolorder[i]);
			if (poolnode[i]->isop != NULL) {
				np = poolnode[i];
				while((np = np->isop) != poolnode[i]) {
					printf("%4d", np->num+1);
				}
			} putchar('\n');
		}
		printf("Numbrnch =%4d\n", Numbrnch);
	}
	free_nodematrix(poolnode2);
} /*_ atreeinit */


void
tablealtree(nt)
int nt;
{
	Infoaltree *cp, **info;
	int i, j, k, max;

	info = (Infoaltree **)malloc((unsigned)nt * sizeof(Infoaltree *));
	if (info == NULL) maerror("in tablealtree().");
	for (cp = Atail.up, i = nt; cp != &Ahead; cp = cp->up) {
		info[--i] = cp;
	/*	printf("%.1f %s\n", cp->lklaprox, cp->ltplgy); */
	}
	printf("%d / %d %s %s \"%s\"", nt, Cnotree, Prog_name, VERSION, Modelname);
	printf(" %d OTUs %d sites. %s\n", Maxspc, Numsite, Comment);

	if (Mevol_optn) { /* total branch length */
		printf("# top ranking %1d trees by TBL criterion (Minimum Evolution)\n",
			Maxaltree);
	} else { /* approximate ln likelihood */
		printf("# <= %1d trees (top ranking for approx. ln L)", Maxaltree);
		printf(" in the top %.1f%% range of TBL\n", Tblrate * 100.0);
	}

	for (i = 1, max = Tblbin[0]; i < NUMTBLBIN; i++) {
		if (Tblbin[i] > max) max = Tblbin[i];
	}
	printf("# %5s%8s %9s\n", "range", "TBL ", "trees");
	printf("#   <  %8.2f %9d\n", Mintbldm, Tblunder);
	for (i = 0; i < NUMTBLBIN; i++) {
		printf("# %3.0f%% %8.2f %9d ",
			(i+1)*100.0/NUMTBLBIN, ((i+1.0)/Tblcoef)+Mintbldm, Tblbin[i]);
		k = (int)(Tblbin[i] * 50.0 / max);
		for (j = 0; j < k; j++) putchar('*');
		putchar('\n');
	}
	printf("# %3s  %8s %9d\n", "", "over", Tblover);

	putchar('#');
	if (!Mevol_optn) { /*  approximate ln likelihood */
		printf(" approx. ln L %.1f", info[0]->lklaprox);
		printf(" ... %.1f", info[nt-1]->lklaprox);
		printf(" diff %.1f,", info[0]->lklaprox - info[nt-1]->lklaprox);
	}
	printf(" TBL %.1f", info[0]->tbl);
	printf(" ... %.1f", info[nt-1]->tbl);
	printf(" diff %.1f\n", info[nt-1]->tbl - info[0]->tbl);

	for (i = 0; i < nt; i++) {
		if (Info_optn ) printf("%.1f ", info[i]->lklaprox);
		fputs(info[i]->ltplgy, stdout);
		printf("; %.1f\n", info[0]->lklaprox - info[i]->lklaprox);
	}
} /*_ tablealtree */
