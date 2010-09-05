/*
 * mltree.c   Adachi, J.   1996.03.01
 * Copyright (C) 1992-1996 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "protml.h"

#define SLSDEBUG 0
#define MLSITE 0
#define USERTREEDEBUG 0


Node **
new_npvector(n)
int n;
/* memory allocate a node pointer vector */
{
	Node **v;

	v = (Node **) malloc((unsigned)n * sizeof(Node *));
	if (v == NULL) maerror("in new_npvector().");
	return v;
}

void
free_npvector(v)
Node **v;
{
	free((char *) v);
}


Tree *
new_tree(maxspc, maxibrnch, numptrn, seqconint)
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
	tr->rootp = NULL;

	return tr;
} /*_ new_tree */


int
getbuftree(numspc, identif)
int numspc;
cmatrix identif;
{
	int i, buf;

	buf = numspc * 3; /* - 3 */
	for (i = 0; i < numspc; i++)
		buf += strlen(identif[i]);
	return buf;
} /*_ getbuftree */


void
changedistan(distanmat, distanvec, numspc)
dmatrix distanmat;
dvector distanvec;
int numspc;
{
	int i, j, k;

	for (k = 0, i = 1; i < numspc; i++) {
		for (j = 0; j < i; j++, k++)
			distanvec[k] = distanmat[i][j];
	}
}


void
getproportion(proportion, distanvec, maxpair)
double *proportion;
dvector distanvec;
int maxpair;
{
	int i;
	double maxdis;

	maxdis = 0.0;
	for (i = 0; i < maxpair; i++) {
		if (distanvec[i] > maxdis) {
			maxdis = distanvec[i];
		}
	}
	*proportion = (double)MAXCOLUMN / (maxdis * 3.0);
	if (*proportion > 1.0) *proportion = 1.0;
	if (Debug) printf("Proportion: %.5f   maxdis: %.5f\n", *proportion,maxdis);
} /* getproportion */


Infotree *
newinfotrees(numtree, buftree)
int numtree;
int buftree;
{
	int i;
	Infotree *info;
	cvector m;

	info = (Infotree *) malloc((unsigned)numtree * sizeof(Infotree));
	if (info == NULL) maerror("in newinfotrees().");
	m = (cvector) malloc((unsigned)(numtree * buftree) * sizeof(char));
	if (m == NULL) maerror("2 in newinfotrees().");
	for (i = 0; i < numtree; i++, m+=buftree)
		info[i].ltplgy = m;

	return info;
} /*_ newinfotrees */


Infoaltree *
newinfoaltrees(numaltree, buftree)
int numaltree;
int buftree;
{
	int i;
	Infoaltree *info;
	cvector m;

	info = (Infoaltree *) malloc((unsigned)numaltree * sizeof(Infoaltree));
	if (info == NULL) maerror("in newinfoaltrees().");
	m = (cvector) malloc((unsigned)(numaltree * buftree) * sizeof(char));
	if (m == NULL) maerror("2 in newinfoaltrees().");
	for (i = 0; i < numaltree; i++, m+=buftree)
		info[i].ltplgy = m;

	return info;
} /*_ newinfoaltrees */


void
getnumtree(ifp, numtree)
FILE *ifp;
int *numtree;
{
	char *cp;
	char line[BUFLINE];

	while (fgets(line, BUFLINE, ifp) != NULL) {
		/* fputs(line, ofp); */
		if (sscanf(line, "%d", numtree) == 1) {
			if (Verbs_optn && User_optn && !Aprox_optn)
				fprintf(stderr,"%d trees\n", *numtree); /* stdout */
			return;
		} else {
			if (*line == '#')
				continue;
			for (cp = line; isspace(*cp) && *cp != '\0'; cp++)
			if (*cp != '\0') {
				fputs(line, stderr);
				fprintf(stderr,
					"\nCan't read number of user trees, bad format.\n");
				exit(1);
			} else {
				continue;
			}
		}
	}
	fprintf(stderr, "\nCan't read number of user trees.\n");
	exit(1);
} /*_ getnumtree */


void
getusertree(ifp, strtree, buftree)
FILE *ifp;
cvector strtree;
int buftree;
{
	char line[BUFLINE];
	char *cp, *np, *xp, *ap, *bp;
	int par, bra, nsp;
	boolean word;

	word = FALSE;
	par = bra = nsp = 0;
	np = strtree;
	strtree[buftree - 1] = '\0';
	while (fgets(line, BUFLINE, ifp) != NULL) {
		if (!isalnum(*line)) word = FALSE;
		/* printf("%s\n", line); */
		if (*line == '#') continue; /* comment line */
		for (cp = line; *cp != '\0'; ) {
			switch(*cp) {

			case ';':
					if (np == strtree) goto next;
					if (par == 0 && bra == 0) { /* tree */
						*np = '\0';
						if (nsp != Numspc) {
							fprintf(stderr,
								"ERROR: different number of OTU, %d/%d. \n",
								nsp, Numspc);
							fprintf(stderr, "%s\n", strtree);
							exit(1);
						}
						/* printf("%s\n", strtree); */
						return;
					} else { /* no tree */
						if (par != 0) {
							fprintf(stderr,
								"ERROR, bad match parenthesis \"()\" !\n");
							if (par > 0)
								fprintf(stderr,"')' is %d less than '('.\n",
									par);
							if (par < 0)
								fprintf(stderr,"')' is %d more than '('.\n",
									abs(par));
						}
						if (bra != 0) {
							fprintf(stderr,
								"ERROR, bad match brace \"{}\" !\n");
							if (bra > 0)
								fprintf(stderr,"'}' is %d less than '{'.\n",
									par);
							if (par < 0)
								fprintf(stderr,"'}' is %d more than '{'.\n",
									abs(par));
						}
						fprintf(stderr, "%s\n", strtree);
						exit(1);
					}
					break;
			case '(': *np++ = *cp++; par++; break;
			case ')': *np++ = *cp++; par--; break;
			case '{': *np++ = *cp++; bra++; break;
			case '}': *np++ = *cp++; bra--; break;
			default:
				if (isspace(*cp) || *cp == ',') {
					cp++;
				} else { /* identifier */
					if (isalnum(*cp) || *cp == '_' || *cp == '-') {
						if (!word) {
							np--;
							if (isalnum(*np) || *np == '_' || *np == '-') {
								np++;
								*np = ' ';
							}
							np++;
						}
						ap = cp; bp = np;
						while (isalnum(*cp) || *cp == '_' || *cp == '-') {
							*np++ = *cp++;
						}
						if (!word) nsp++;
						*cp == '\0' ? (word = TRUE) : (word = FALSE);
						if (Debug) printf("%3d %1d %-10.10s %-20.20s\n",
							nsp,word,bp,ap);
					} else {
						fprintf(stderr, "bad name: %s\n", cp);
						exit(1);
					}
				}

			} /* switch */
		} /* while */
		if (par == 0 && bra == 0 && np != strtree) {
			*np = '\0';
			/* printf("%s\n", strtree); */
			return;
		}
		if (np > (strtree + buftree - 2)) {
			fprintf(stderr, "Too long, users tree.\n");
			fprintf(stderr, "You may forget tree delimiter \";\".\n");
			fprintf(stderr, "\"%s\"\n", strtree);
			exit(1);
		}
#if USERTREEDEBUG
		printf("%s\n", strtree);
#endif
		next: ;
	}
	fprintf(stderr, "Can't read users trees of input file.\n");
	exit(1);
} /*_ getusertree */


Node *
internalnode(tr, cpp, ninode, st)
Tree *tr;
char **cpp;
int *ninode;
char *st;
{
	Node *xp, *np, *rp;
	int i, j, dvg;
	char ident[MAXWORD];
	char *idp;

	/* fprintf(stderr, "in1:%s\n", *cpp); */

	if (**cpp == ' ') (*cpp)++;
	if (**cpp == '(') {
		if (Debug) printf("internal1: %c\n", **cpp);
		(*cpp)++;
		xp = internalnode(tr, cpp, ninode, st);
		xp->isop = xp;
		dvg = 1;
		while (**cpp != ')') {
			dvg++;
			np = internalnode(tr, cpp, ninode, st);
			np->isop = xp->isop;
			xp->isop = np;
			xp = np;
		}
		if (Debug) printf("internal2: %c\n", **cpp);
		(*cpp)++;
		if (dvg < 2) {
			fputs("ERROR, redundancy or unnecessary \"()\" !\n", stderr);
			fprintf(stderr, "%s\n", st);
			exit(1);
		}
		/* printf("ninode %d\n", *ninode + 1); */
		rp = tr->ibrnchp[*ninode];
		rp->isop = xp->isop;
		xp->isop = rp;

		for (j = 0; j < Numspc; j++) rp->paths[j] = 0;
		for (xp = rp->isop; xp != rp; xp = xp->isop) {
			for (j = 0; j < Numspc; j++) {
				if (xp->paths[j] == 1) rp->paths[j] = 1;
			}
		}
		/*
		if (Debug) {
			for (j = 0; j < Numspc; j++) printf("%2d",rp->paths[j]);
			putchar('\n');
		}
		*/
		(*ninode)++;
		if (*ninode > Maxibrnch) {
			fputs("ERROR, too many number of internal branch!\n", stderr);
			fputs("This tree may be rooted tree.\n", stderr);
			fprintf(stderr, "%s\n", st);
			exit(1);
		}

		/* fprintf(stderr, "in2:%s\n", *cpp); */

		return rp->kinp;
	} else if (isalnum(**cpp)) {
		if (Debug) printf("external: %c\n", **cpp);
		for (idp = ident; **cpp!=' ' && **cpp!='(' && **cpp!=')'; (*cpp)++) {
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
		fprintf(stderr, "ERROR, abnormal identifier(OTU name): %s !\n", ident);
		exit(1);
	} else {
		fprintf(stderr, "ERROR, abnormal tree topology character: %s\n", *cpp);
		exit(1);
	}
	return NULL;
} /*_ internalnode */


void
constructtree(tr, strtree)
Tree *tr;
cvector strtree;
{
	char *cp;
	int ninode, dvg;
	Node *xp, *np;

#if USERTREEDEBUG
	printf("%s\n", strtree);
#endif
	ninode = 0;
	cp = strtree;
	if (*cp == '(') {
		if (Debug) printf("roottre0: %c\n", *cp);
		cp++;
		xp = internalnode(tr, &cp, &ninode, strtree);
		xp->isop = xp;
		dvg = 1;
		while (*cp != ')') {
			dvg++;
			np = internalnode(tr, &cp, &ninode, strtree);
			np->isop = xp->isop;
			xp->isop = np;
			xp = np;
		}
		if (Debug) printf("roottre0: %c\n", *cp);
		if (dvg < 3) {
			fputs("ERROR: this tree is not unroot tree!\n", stderr);
			fprintf(stderr, "%s\n", strtree);
			exit(1);
		}
		tr->rootp = xp;
		Numibrnch = ninode;
		Numbrnch = Numspc + ninode;
	} else {
		fprintf(stderr, "ERROR users tree:\n");
		fprintf(stderr, "%s\n", strtree);
		exit(1);
	}
} /*_ constructtree */


void
prcurtree(tr)
Tree *tr;
{
	Node *dp, *up, *cp;
	int i, j;
	double sum;

/*	printf("\nStructure of Tree\n"); */
	printf("\n%4s%5s%5s%5s%7s%7s%37s\n",
	"num","kinp","isop","isop","descen","length", "namesp");
	for (i = 0; i < Numspc; i++) {
		dp = tr->ebrnchp[i];
		up = dp->kinp;
		printf("%4d", dp->num+1);
		printf("%5d", up->num+1);
		if (dp->isop == NULL) printf("%5s", "null");
		else                  printf("%5d", dp->isop->num+1);
		if (up->isop == NULL) printf("%5s", "null");
		else                  printf("%5d", up->isop->num+1);
		printf("%3d", dp->descen);
		printf("%3d", up->descen);
		printf("%8.3f", dp->length);
		printf("%8.3f", up->length);
		printf("%5d%5d", dp->eprob[0],dp->eprob[Numptrn-1]);
		for (sum = 0.0, j = 0; j < Tpmradix; j++) sum += up->iprob[0][j];
		printf("%5.0f", sum * 1000.0);
		for (sum = 0.0, j = 0; j < Tpmradix; j++) sum += up->iprob[Numptrn-1][j];
		printf("%5.0f", sum * 1000.0);
		printf("   %s", Identif[dp->num]);
		putchar('\n');
	}
	for (i = 0; i < Numibrnch; i++) {
		dp = tr->ibrnchp[i];
		up = dp->kinp;
		printf("%4d", dp->num+1);
		printf("%5d", up->num+1);
		if (dp->isop == NULL) printf("%5s", "null");
		else                  printf("%5d", dp->isop->num+1);
		if (up->isop == NULL) printf("%5s", "null");
		else                  printf("%5d", up->isop->num+1);
		printf("%3d", dp->descen);
		printf("%3d", up->descen);
		printf("%8.3f", dp->length);
		printf("%8.3f", up->length);
		for (sum = 0.0, j = 0; j < Tpmradix; j++) sum += dp->iprob[0][j];
		printf("%5.0f", sum * 1000.0);
		for (sum = 0.0, j = 0; j < Tpmradix; j++) sum += dp->iprob[Numptrn-1][j];
		printf("%5.0f", sum * 1000.0);
		for (sum = 0.0, j = 0; j < Tpmradix; j++) sum += up->iprob[0][j];
		printf("%5.0f", sum * 1000.0);
		for (sum = 0.0, j = 0; j < Tpmradix; j++) sum += up->iprob[Numptrn-1][j];
		printf("%5.0f", sum * 1000.0);
		for (cp = dp->isop; cp != dp; cp = cp->isop) {
			printf("%5d", cp->num+1);
		}
		putchar('\n');
	}
		dp = tr->rootp->isop;
		printf("%4d", dp->num+1);
		for (cp = dp->isop; cp != dp; cp = cp->isop) {
			printf("%5d", cp->num+1);
		}
		putchar('\n');
} /*_ prcurtree */


void
pathing(tr)
Tree *tr;
{
	int i, j;
	ivector pthsd, pthsa;
	Node *rp, *cp, *xp, *dp;

	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			/*
			pthsd = cp->paths;
			*/
			cp = cp->kinp;
			/*
			pthsa = cp->paths;
			for (i = 0; i < Numspc; i++) {
				pthsd[i] == 1 ? (pthsa[i] = 0) : (pthsa[i] = 1);
			}
			*/
		} else { /* internal node */
			if (!cp->descen) { /* ascent */
				dp = cp->kinp;
				pthsd = dp->paths;
				for (i = 0; i < Numspc; i++) pthsd[i] = 0;
				for (xp = dp->isop; xp != dp; xp = xp->isop) {
					pthsa = xp->kinp->paths;
					for (i = 0; i < Numspc; i++) {
						if (pthsa[i]) pthsd[i] = 1;
					}
				}
				/*
				pthsa = cp->paths;
				for (i = 0; i < Numspc; i++) {
					pthsd[i] == 1 ? (pthsa[i] = 0) : (pthsa[i] = 1);
				}
				*/
			}
		}
	} while (cp != rp);
/* */
	if (Debug_optn) {
	puts("\npath of internal nodes");
	for (i = 0; i < Maxibrnch; i++) {
		pthsd = tr->ibrnchp[i]->paths;
		for (j = 0; j < Numspc; j++) printf("%2d", pthsd[j]); putchar('\n');
	}
	}
/* */
} /*_ pathing */


#if 0
static void
leastsquares(am, ym)
dmatrix am;
dvector ym;
{
	int i, j, k;
	double pivot, element;
	dmatrix im;

	im = new_dmatrix(Numbrnch, Numbrnch);
	for (i = 0; i < Numbrnch; i++) {
		for (j = 0; j < Numbrnch; j++)
			im[i][j] = 0.0;
		im[i][i] = 1.0;
	}
	for (k = 0; k < Numbrnch; k++) {
		pivot = am[k][k];
		ym[k] /= pivot;
		for (j = 0; j < Numbrnch; j++) {
			am[k][j] /= pivot;
			im[k][j] /= pivot;
		}
		for (i = 0; i < Numbrnch; i++) {
			if (k != i) {
				element = am[i][k];
				ym[i] -= element * ym[k];
				for (j = 0; j < Numbrnch; j++) {
					am[i][j] -= element * am[k][j];
					im[i][j] -= element * im[k][j];
				}
			}
		}
	}
/*
	putchar('\n');
	for (i = 0; i < Numbrnch; i++) {
		for (j = 0; j < Numbrnch; j++)
			printf("%10.6f", im[i][j]);
		putchar('\n');
	}
*/
	free_dmatrix(im);
} /*_ leastsquares */
#endif


static void
luequation(amat, yvec, size)
dmatrix amat;
dvector yvec;
int size;
{
	/* SOLVE THE LINEAR SET OF EQUATIONS ON LU DECOMPOSITION */
    double eps = 1.0e-20; /* ! */
	int i, j, k, l, maxi, idx;
	double sum, tmp, maxb, aw;
	dvector wk;
	ivector index;

	wk = new_dvector(size);
	index = new_ivector(size);
	aw = 1.0;
	for (i = 0; i < size; i++) {
		maxb = 0.0;
		for (j = 0; j < size; j++) {
			if (fabs(amat[i][j]) > maxb)
				maxb = fabs(amat[i][j]);
		}
		if (maxb == 0.0) {
			fprintf(stderr, "luequation: singular matrix\n");
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
			aw = -aw;
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

	l = -1;
	for (i = 0; i < size; i++) {
		idx = index[i];
		sum = yvec[idx];
		yvec[idx] = yvec[i];
		if (l != -1) {
			for (j = l; j < i; j++)
				sum -= amat[i][j] * yvec[j];
		} else if (sum != 0.0)
			l = i;
		yvec[i] = sum;
	}
	for (i = size - 1; i >= 0; i--) {
		sum = yvec[i];
		for (j = i + 1; j < size; j++)
			sum -= amat[i][j] * yvec[j];
		yvec[i] = sum / amat[i][i];
	}
	free_ivector(index);
	free_dvector(wk);
} /*_ luequation */


void
lslength(tr, distanvec, numspc)
Tree *tr;
dvector distanvec;
int numspc;
{
	int i, i1, j, j1, j2, k, numibrnch, numbrnch, numpair;
	double sum, leng, alllen, rss;
	ivector pths;
	dmatrix atmt, atamt;
	Node **ebp, **ibp;

	if (Debug) printf("numspc = %d\n", numspc);
	numibrnch = Numibrnch;
	numbrnch = numspc + numibrnch;
	numpair = (numspc * (numspc - 1)) / 2;
	atmt = new_dmatrix(numbrnch, numpair);
	atamt = new_dmatrix(numbrnch, numbrnch);
	ebp = tr->ebrnchp;
	ibp = tr->ibrnchp;
	if (Debug) {
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			for (j = 0; j < numspc; j++) printf("%2d",ebp[i]->paths[j]);
			putchar('\n');
		}
		for (i = 0; i < numibrnch; i++) {
			for (j = 0; j < numspc; j++) printf("%2d",ibp[i]->paths[j]);
			putchar('\n');
		}
	}

	for (i = 0; i < numspc; i++) {
		for (j1 = 1, j = 0; j1 < numspc; j1++) {
			if (j1 == i) {
				for (j2 = 0; j2 < j1; j2++, j++) {
					atmt[i][j] = 1.0;
				}
			} else {
				for (j2 = 0; j2 < j1; j2++, j++) {
					if (j2 == i)
						atmt[i][j] = 1.0;
					else
						atmt[i][j] = 0.0;
				}
			}
		}
	}
	for (i1 = 0, i = numspc; i1 < numibrnch; i1++, i++) {
		pths = ibp[i1]->paths;
		for (j1 = 1, j = 0; j1 < numspc; j1++) {
			for (j2 = 0; j2 < j1; j2++, j++) {
				if (pths[j1] != pths[j2])
					atmt[i][j] = 1.0;
				else
					atmt[i][j] = 0.0;
			}
		}
	}

	if (Debug) {
		putchar('\n');
		for (i = 0; i < numpair; i++) {
			for (j = 0; j < numbrnch; j++)
				printf("%2.0f",atmt[j][i]); /* !? */
			printf("%6.1f\n",distanvec[i]);
		}
	}
#ifdef DIST
		putchar('\n');
		for (i = 0; i < numpair; i++)
			printf("%5.1f",distanvec[i]);
		putchar('\n');
#endif
	for (i = 0; i < numbrnch; i++) {
		for (j = 0; j <= i; j++) {
			for (k = 0, sum = 0.0; k < numpair; k++)
				sum += atmt[i][k] * atmt[j][k];
			atamt[i][j] = sum;
			atamt[j][i] = sum;
		}
	}
	for (i = 0; i < numbrnch; i++) {
		for (k = 0, sum = 0.0; k < numpair; k++)
			sum += atmt[i][k] * distanvec[k];
		Brnlength[i] = sum;
	}
#if 0
	putchar('\n');
	for (i = 0; i < numbrnch; i++) {
		for (j = 0; j < numbrnch; j++)
			printf("%5.1f", atamt[i][j]);
		printf("%7.3f",Brnlength[i]);
		putchar('\n');
	}
#endif
	luequation(atamt, Brnlength, numbrnch);
#if 0
	putchar('\n');
	for (i = 0; i < numbrnch; i++) {
		for (j = 0; j < numbrnch; j++)
			printf("%5.1f", atamt[i][j]);
		printf("%7.3f",Brnlength[i]);
		putchar('\n');
	}
#endif
	if (Debug) {
		for (i = 0; i < numbrnch; i++) {
			printf("%7.3f",Brnlength[i]);
		} putchar('\n');
	}
	for (i = 0, rss = 0.0; i < numpair; i++) {
		sum = distanvec[i];
		for (j = 0; j < numbrnch; j++) {
			if (atmt[j][i] == 1.0 && Brnlength[j] > 0.0) /* caution */
				sum -= Brnlength[j];
		}
		rss += sum * sum;
	}
	tr->rssleast = sqrt(rss); /* / numpair !? */
	alllen = 0.0;
	for (i = 0; i < numspc; i++) {
		leng = Brnlength[i];
	/*	if (leng > 0.0) leng = 0.0   caution */
		alllen += leng;
		if (leng < Llimit) leng = Llimit;
		if (leng > Ulimit) leng = Ulimit;
		ebp[i]->length = leng;
		ebp[i]->kinp->length = leng;
		Brnlength[i] = leng;
	}
	for (i = 0, j = numspc; i < numibrnch; i++, j++) {
		leng = Brnlength[j];
	/*	if (leng > 0.0) leng = 0.0   caution */
		alllen += leng;
		if (leng < Llimit) leng = Llimit;
		if (leng > Ulimit) leng = Ulimit;
		ibp[i]->length = leng;
		ibp[i]->kinp->length = leng;
		Brnlength[j] = leng;
	}
	tr->tbldis = alllen;
	free_dmatrix(atmt);
	free_dmatrix(atamt);
} /*_ lslength */


void
slslength(tr, dmat, ns) /* simplify least square method (not equal LS) */
Tree *tr;
dmatrix dmat;
int ns;
{
	int i, j, k, m, ne, ni, l, r, ll, rr;
	int numibrnch, numbrnch;
	double sumc, suml, sumr, leng, alllen, rss, coef;
	ivector pths, lpaths, rpaths, npaths;
	Node **ebp, **ibp, *dp, *ap, *xp;

	lpaths = new_ivector(ns);
	rpaths = new_ivector(ns);
	npaths = new_ivector(ns);
	numibrnch = Numibrnch;
	numbrnch = ns + numibrnch;
	ebp = tr->ebrnchp;
	ibp = tr->ibrnchp;
	alllen = 0.0;

#if SLSDEBUG
	for (i = 0; i < ns; i++) {
		for (j = 0; j < ns; j++) printf("%6.0f", dmat[i][j]*1000);
		putchar('\n');
	} putchar('\n');
	for (i = 0; i < ns; i++) {
		for (j = 0, sumc = 0.0; j < ns; j++) sumc += dmat[i][j];
		printf("%6.0f", sumc*1000);
	} putchar('\n'); putchar('\n');
#endif /* SLSDEBUG */

	for ( ne = 0; ne < numbrnch; ne++) {

		for (k = 0; k < ns; k++) {
			npaths[k] = rpaths[k] = 0;
		}
		if (ne < ns) { /* internal */
			l = 1;
			ll = 1;
			dp = ebp[ne];
			ap = ebp[ne]->kinp;
			pths = dp->paths;
			for (k = 0; k < ns; k++) lpaths[k] = pths[k];
			npaths[ne] = 1;
		} else { /* external */
			ni = ne - ns;
			dp = ibp[ni];
			ap = ibp[ni]->kinp;
			pths = dp->paths;
			for (k = 0, l = 0; k < ns; k++) {
				lpaths[k] = 0;
				if (pths[k]) l++;
			}
			for (xp = dp->isop, j = 1; xp != dp; xp = xp->isop, j++) {
				pths = xp->paths;
				for (k = 0, m = 0; k < ns; k++) if (pths[k]) m++;
				if (xp->descen) {
					m = ns - m;
					for (k = 0; k < ns; k++) {
						if (!pths[k]) {
							lpaths[k] = j;
							npaths[k] = m;
						}
					}
				} else {
					for (k = 0; k < ns; k++) {
						if (pths[k]) {
							lpaths[k] = j;
							npaths[k] = m;
						}
					}
				}
			}
			ll = j - 1;
		}
		r = ns - l;
		for (xp = ap->isop, j = 1; xp != ap; xp = xp->isop, j++) {
			pths = xp->paths;
			for (k = 0, m = 0; k < ns; k++) if (pths[k]) m++;
			if (xp->descen) {
				m = ns - m;
				for (k = 0; k < ns; k++) {
					if (!pths[k]) {
						rpaths[k] = j;
						npaths[k] = m;
					}
				}
			} else {
				for (k = 0; k < ns; k++) {
					if (pths[k]) {
						rpaths[k] = j;
						npaths[k] = m;
					}
				}
			}
		}
		rr = j - 1;

#if SLSDEBUG
		for (k = 0; k < ns; k++) printf("%2d", lpaths[k]); putchar('\n');
		for (k = 0; k < ns; k++) printf("%2d", rpaths[k]); putchar('\n');
		for (k = 0; k < ns; k++) printf("%2d", npaths[k]); putchar('\n');
#endif /* SLSDEBUG */

		sumc = suml = sumr = 0.0;
		for (i = 0; i < ns - 1; i++) {
			for (j = i + 1; j < ns; j++) {
				coef = (double)npaths[i] * (double)npaths[j];
				if (lpaths[i]) {
					if (rpaths[j]) {
						sumc += dmat[i][j] / coef;
#if						SLSDEBUG
						printf("c1:%3d%3d%8.0f%7.0f\n",
							i+1,j+1,sumc*1000,dmat[i][j]/coef*1000);
#endif					/* SLSDEBUG */
					} else if (lpaths[i] != lpaths[j]) {
						suml += dmat[i][j] / coef;
#if						SLSDEBUG
						printf("l :%3d%3d%8.0f%7.0f\n",
							i+1,j+1,suml*1000,dmat[i][j]/coef*1000);
#endif					/* SLSDEBUG */
					}
				} else {
					if (!rpaths[j]) {
						sumc += dmat[i][j] / coef;
#if						SLSDEBUG
						printf("c2:%3d%3d%8.0f%7.0f\n",
							i+1,j+1,sumc*1000,dmat[i][j]/coef*1000);
#endif					/* SLSDEBUG */
					} else if (rpaths[i] != rpaths[j]) {
						sumr += dmat[i][j] / coef;
#if						SLSDEBUG
						printf("r :%3d%3d%8.0f%7.0f\n",
							i+1,j+1,sumr*1000,dmat[i][j]/coef*1000);
#endif					/* SLSDEBUG */
					}
				}
			}
		}

#if		SLSDEBUG
		printf("%3s:%3d",(ne < ns ? "ext" : "int"), ne+1);
		printf(" %3d%3d%3d%3d", l, r, ll, rr);
		printf(" %5.2f%5.2f", (ne < ns ? 0 : (double)rr / (double)(ll-1)),
			(double)ll / (double)(rr-1));
		printf("%7.0f%7.0f%7.0f", sumc*1000, suml*1000, sumr*1000);
#endif	/* SLSDEBUG */
		if (ne >= ns) suml *= (double)rr / (double)(ll-1);
		sumr *= (double)ll / (double)(rr-1);
		leng = ( sumc - sumr - suml ) / (double)(ll * rr);
#if		SLSDEBUG
		printf("   leng:%9.5f\n", leng);
#endif	/* SLSDEBUG */

		alllen += leng;

		if (leng < 0.0) leng = 0.0;  /* caution!? */
		if (ne < ns) /* external */
			pths = ebp[ne]->paths;
		else /* internal */
			pths = ibp[ni]->paths;
		for (i = 1; i < ns ; i++) {
			for (j = 0; j < i; j++) {
				if (pths[i] != pths[j]) dmat[i][j] -= leng;
			}
		}

		if (leng > Ulimit) leng = Ulimit;
		if (ne < ns)  { /* external */
			if (leng < LOWERLIMIT) leng = LOWERLIMIT;
			ebp[ne]->length = ebp[ne]->kinp->length = leng;
		} else { /* internal */
			if (leng < Llimit) leng = Llimit;
			ibp[ni]->length = ibp[ni]->kinp->length = leng;
		}
		Brnlength[ne] = leng; /* !? */

	} /* for ne */

	for (i = 1, rss = 0.0; i < ns ; i++) {
		for (j = 0; j < i; j++) {
			rss += dmat[i][j] * dmat[i][j];
#if			SLSDEBUG
			printf("%3d%3d%9.3f\n", i+1, j+1, dmat[i][j]);
#endif		/* SLSDEBUG */
		}
	}
	tr->rssleast = sqrt(rss); /* / numpair !? */
#if SLSDEBUG
	printf("rss: %9.3f%9.3f\n", rss, tr->rssleast);
#endif /* SLSDEBUG */
	for (i = 1; i < ns ; i++) {
		for (j = 0; j < i; j++) dmat[i][j] = dmat[j][i];
	}

	tr->tbldis = alllen;
	free_ivector(lpaths);
	free_ivector(rpaths);
	free_ivector(npaths);
} /*_ slslength */


#define DEBUGFM 0
#define LSLEN   0
void
fmlength(tr, dmat, ns)
Tree *tr;
dmatrix dmat;
int ns;
{
	int i, j, k, no, nx, ny, m;
	double sy, x, xav, alllen, rss;
	ivector otux, path;
	dvector xv, yv;
	dmatrix dism;
	Node **otup, *cp, *rp, *dp, *xp;
#if	LSLEN
	ivector otun;
#endif	/* LSLEN */

	dism = new_dmatrix(ns, ns);
	for (i = 0; i < ns; i++) {
		for (j = 0; j < ns; j++) dism[i][j] = dmat[i][j];
	}
	otux = new_ivector(ns);
	xv = new_dvector(ns);
	yv = new_dvector(ns);
	otup = (Node **)new_npvector(ns);
#if	LSLEN
	otun = new_ivector(ns);
#endif	/* LSLEN */
	for (i = 0; i < ns; i++) {
		otux[i] = 0;
		otup[i] = tr->ebrnchp[i]->kinp;
#if		LSLEN
		otun[i] = 1;
#endif	/* LSLEN */
	}
	alllen = 0.0;

	no = ns;
	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp = cp->kinp; /* not descen */
		} else { /* internal node */
			if (!cp->descen) {
				dp = cp->kinp;
				m = ns;
				for (xp = dp->isop, nx = 0; xp != dp; xp = xp->isop, nx++) {
					i = xp->num;
					otux[i] = i;
					if (i < m) m = i;
				}
				ny = no - nx;
				for (xp = dp->isop, sy = 0; xp != dp; xp = xp->isop) {
					i = xp->num;
					for (j = 0, xv[i] = 0, yv[i] = 0; j < ns; j++) {
						if (otup[j] != NULL) {
							if (otux[j])
								xv[i] += dism[i][j];
							else
								yv[i] += dism[i][j];
						}
					}
					yv[i] /= ny;
					sy += yv[i];
				}
#if				DEBUGFM
				printf("\nint. baranch %3d", cp->num+1);
				printf("   m:%3d  nx:%3d  ny:%3d  no:%3d\n", m+1, nx, ny, no);
				printf("%3s", "");
				for (j = 0; j < ns; j++) printf("%4d", j+1); putchar('\n');
				for (i = 0; i < ns; i++) {
					printf("%3d", i+1);
					for (j = 0; j < ns; j++) printf("%4.0f", dism[i][j] * 10);
					putchar('\n');
				}
				printf("\n%3s", "otp");
				for (j = 0; j < ns; j++) printf("%4d", otup[j] != NULL ? 1 : 0);
				putchar('\n');
				printf("%3s", "otx");
				for (j = 0; j < ns; j++) printf("%4d", otux[j]); putchar('\n');
				printf("%3s", "xv");
				for (j = 0; j < ns; j++) printf("%4.0f", xv[j]*10);
				putchar('\n');
				printf("%3s", "yv");
				for (j = 0; j < ns; j++) printf("%4.0f", yv[j]*10);
				putchar('\n');
#endif			/* DEBUGFM */
				for (xp = dp->isop, xav = 0; xp != dp; xp = xp->isop) {
					i = xp->num;
					x = (xv[i]+yv[i])/nx - (sy-yv[i])/(nx*(nx-1));
					alllen += x; /* caution!? */
					if (x < Llimit) x = Llimit;
					xp->length = xp->kinp->length = x;
					xav += x;
					Brnlength[xp->kinp->num] = x;
#if					DEBUGFM
					printf("b-len %3d%9.4f%9.4f%9.4f\n", xp->kinp->num+1
						x, (xv[i]+yv[i])/nx, (sy-yv[i])/(nx*(nx-1)));
#endif				/* DEBUGFM */
					if (i != m) {
						for (j = 0; j < ns; j++) {
							if (otup[j] != NULL && j != m) {
								dism[m][j] += dism[i][j];
								dism[j][m] += dism[j][i];
							}
#if							DEBUGFM
							dism[i][j] = dism[j][i] = 0.0;
#endif						/* DEBUGFM */
						}
						otup[i] = NULL;
					}
					otux[i] = 0;
					if (xp->kinp->isop != NULL) xp->num = xp->kinp->num;
				}

				for (j = 0; j < ns; j++) {
					if (otup[j] != NULL && j != m) {
						dism[m][j] = (dism[m][j] - xav)/nx;
						dism[j][m] = (dism[j][m] - xav)/nx;
					}
					xv[j] = yv[j] = 0.0;
				}
				cp->num = m;
				no -= (nx - 1);

			}
		}
	} while (cp != rp);

	xp = rp; nx = 0;
	do {
		xp = xp->isop;
		i = xp->num;
		otux[i] = 1;
		nx++;
	} while (xp != rp);
	ny = no - nx;
	xp = rp; sy = 0.0;
	do {
		xp = xp->isop;
		i = xp->num;
		for (j = 0, xv[i] = 0; j < ns; j++) {
			if (otup[j] != NULL && otux[j]) xv[i] += dism[i][j];
		}
		sy += xv[i];
	} while (xp != rp);
	sy /= 2;
#if	DEBUGFM
	printf("root   nx:%3d  ny:%3d  no:%3d\n", nx, ny, no);
	printf("%3s", "");
	for (j = 0; j < ns; j++) printf("%4d", j+1); putchar('\n');
	for (i = 0; i < ns; i++) {
		printf("%3d", i+1);
		for (j = 0; j < ns; j++) printf("%4.0f", dism[i][j] * 10);
		putchar('\n');
	}
	printf("\n%3s", "otp");
	for (j = 0; j < ns; j++) printf("%4d", otup[j] != NULL ? 1 : 0);
	putchar('\n');
	printf("%3s", "otx");
	for (j = 0; j < ns; j++) printf("%4d", otux[j]); putchar('\n');
	printf("%3s", "xv");
	for (j = 0; j < ns; j++) printf("%4.0f", xv[j] * 10); putchar('\n');
	printf("sy:%9.5f\n", sy);
#endif /* DEBUGFM */
	xp = rp;
	do {
		xp = xp->isop;
		i = xp->num;
		x = ( xv[i] - (sy-xv[i])/(nx-2) ) / (nx-1);
		if (xp->kinp->isop == NULL) {
			if (x < LOWERLIMIT) x = LOWERLIMIT;
		} else {
			if (x < Llimit) x = Llimit;
		}
		xp->length = xp->kinp->length = x;
		alllen += x;
		Brnlength[xp->kinp->num] = x;
#if		DEBUGFM
		printf("b-len %3d%9.4f%9.4f%9.4f%9.4f\n",
			xp->kinp->unm+1, x, xv[i], sy-xv[i], (sy-xv[i])/(nx-2) );
#endif	/* DEBUGFM */
		if (xp->kinp->isop != NULL) xp->num = xp->kinp->num;
	} while (xp != rp);

#if	DEBUGFM
	for (j = 0; j < Numbrnch; j++) printf("%3d %9.4f\n", j+1, Brnlength[j]);
#endif	/* DEBUGFM */

	for (i = 0; i < ns-1; i++) {
		for (j = i+1; j < ns; j++) dism[i][j] = dmat[i][j];
	}
	for (i = 0; i < ns; i++) {
		x = tr->ebrnchp[i]->length;
		path = tr->ebrnchp[i]->paths;
		for (j = 0; j < ns; j++) {
			if (path[j]) {
				for (k = 0; k < ns; k++) {
					if (path[k] == 0) {
						if (j < k) {
							dism[j][k] -= x;
#if 0
							printf("%3d%3d%3d%9.4f%9.4f%9.4f\n",
								i+1,j+1,k+1,x,dism[j][k],dmat[j][k]);
#endif
						} else {
							dism[k][j] -= x;
#if 0
							printf("%3d%3d%3d%9.4f%9.4f%9.4f\n",
								i+1,j+1,k+1,x,dism[k][j],dmat[k][j]);
#endif
						}
					}
				}
			}
		}
	}
	for (i = 0; i < Numibrnch; i++) {
		x = tr->ibrnchp[i]->length;
		if (x == Llimit) x = 0.0; /* caution */
		path = tr->ibrnchp[i]->paths;
		for (j = 0; j < ns; j++) {
			if (path[j]) {
				for (k = 0; k < ns; k++) {
					if (path[k] == 0) {
						if (j < k) {
							dism[j][k] -= x;
#if 0
							printf("%3d%3d%3d%9.4f%9.4f%9.4f\n",
								i+ns+1,j+1,k+1,x,dism[j][k],dmat[j][k]);
#endif
						} else {
							dism[k][j] -= x;
#if 0
							printf("%3d%3d%3d%9.4f%9.4f%9.4f\n",
								i+ns+1,j+1,k+1,x,dism[k][j],dmat[k][j]);
#endif
						}
					}
				}
			}
		}
	}
	for (i = 0, rss = 0.0; i < ns-1; i++) {
		for (j = i+1; j < ns; j++) {
			rss += dism[i][j] * dism[i][j];
#if 0
			printf("%3d%3d%9.4f%9.4f%9.4f\n",
				i+1, j+1, dism[i][j], dmat[i][j], rss);
#endif
		}
	}

	tr->rssleast = rss;
	tr->tbldis = alllen;
	free_dmatrix(dism);
	free_ivector(otux);
	free_dvector(xv);
	free_dvector(yv);
	free_npvector(otup);
#if	LSLEN
	free_ivector(otun);
#endif	/* LSLEN */
} /*_ fmlength */


extern double probnormal();

void
resulttree(tr)
Tree *tr;
{
	boolean llimit, rtrif;
	int n, ne;
	double reli;
	Node *ep, *ip;

	if (Relitrif != NULL) rtrif = TRUE; else rtrif = FALSE;
	printf("\nNo.%-3d", Cnotree + 1); /* offset */
	printf("%9s%7s%5s", "ext.","branch","S.E.");
	printf("%6s%7s%5s", "int.","branch","S.E.");
	if (Relia_optn) printf("  %5s%7s%6s", "LBP ", "2nd ", "pair");
	/* if (Info_optn) printf("%14s", "Likelihood"); */
	putchar('\n');
#if 0
	for (n = 0; n < 28; n++) putchar('-');
	for (n = 0; n <  5; n++) putchar(' ');
	for (n = 0; n < 16; n++) putchar('-'); putchar('\n');
#endif
	llimit = FALSE;
	for (n = 0; n < Numspc; n++) {
		ep = tr->ebrnchp[n];
		ne = ep->num;
		fputid(stdout, Identif[ne], 10);
		fputs(" ", stdout);
		printf("%3d", ne + 1); /* offset */
		if (ep->length == LOWERLIMIT)
			/* printf("%7s %-5s", "lower", "limit"); */
			printf("%7.2f%6.4s", ep->length, "----");
		else if (ep->length == Ulimit)
			printf("%7s %-5s", "UPPER", "LIMIT");
		else
			printf("%7.2f%6.2f", ep->length, sqrt(ep->kinp->lklhdl));
		if (n < Numibrnch) {
			ip = tr->ibrnchp[n];
			printf("%5d", n + 1 + Numspc); /* offset */
			if (ip->length == Llimit) {
				printf("%7s %-5s", "lower", "limit");
				llimit = TRUE;
			} else if (ip->length == Ulimit)
				printf("%7s %-5s", "UPPER", "LIMIT");
			else
				printf("%7.2f%6.2f", ip->length, sqrt(ip->kinp->lklhdl));
			if (Relia_optn && Relistat[n] >= 0) {
				if (Reliprob[n][0] != 1.0)
					printf("  %.3f", Reliprob[n][0]);
				else
					printf("  %.1f  ", Reliprob[n][0]);
				(Relistat[n] > 0) ? putchar('*') : putchar(' ');
				if (Reliprob[n][0] != 1.0)
					printf(" %.3f", Reliprob[n][1]);
				else
					printf(" %.1f  ", Reliprob[n][1]);
				printf(" %3d&%-3d", Relinum[n][0]+1, Relinum[n][1]+1);
			}
#if 0
			reli = probnormal( ip->length / sqrt(ip->kinp->lklhdl) );
			if (ip->length == Llimit)
				printf("%6s", "----");
			else
				printf("%6.2f", reli);
#endif
		/*	if (Info_optn) printf("%14.3f", ip->lklhdl); */
			putchar('\n');
		} else {
			if (n == Numspc - 3) {
				printf("%7s%11.2f", "TBL :",tr->tblength * 1.0);
				printf("%7s %d", "iter:", Numit);
				if (!Converg)
					printf("%s", "        non convergence!");
				else if (Converg == 2)
					printf("%s", " just before convergence");
				putchar('\n');
			} else if (n == Numspc - 2) {
				printf("%7s%11.2f +- %.2f\n", "ln L:",
					tr->lklhd, sqrt(tr->varilkl));
			} else if (n == Numspc - 1) {
				printf("%7s%11.2f", "AIC :",tr->aic);
				if (llimit) printf("%14s %.3f", "lower limit:",Llimit);
				putchar('\n');
			} else {
				putchar('\n');
			}
		}
	}
} /*_ resulttree */


void
bootstrap(infotrs, lklptrn)
Infotree *infotrs;
LPMATRIX lklptrn;
{
	int i, j, k, n, imax, maxtree, nsite, nptrn, same, allsame;
	double lklmax, coefrand;
	ivector addweight;
	dvector boots;

	addweight = new_ivector(Numsite);
	for ( j = 0, k = 0; k < Numptrn; k++ ) {
		for ( i = 0, imax = Weight[k]; i < imax; i++ )
			addweight[j++] = k;
	}
	if (j != Numsite) {
		fputs("ERROR in bootstrap(), sum Weight != Numsite\n", stderr);
		exit(1);
	}

	coefrand = (double)Numsite / ((double)RANDOM_MAX + 1.0);
	boots = new_dvector(Numtree);
	allsame = 0;
	for (n = 0; n < Numtree; n++) infotrs[n].bsprob = 0;

#if MLSITE
	for (n = 0; n < Numtree; n++) infotrs[n].mlsite = 0;
	for (k = 0; k < Numptrn; k++) {
		for (n = 1, maxtree = 0, lklmax = lklptrn[0][k]; n < Numtree; n++) {
			if (lklptrn[n][k] > lklmax) {
				lklmax = lklptrn[n][k];
				maxtree = n;
			}
		}
		infotrs[maxtree].mlsite += Weight[k];
	}
#endif

	for (i = 0; i < NUMBOOTS; i++) {
		if (Verbs_optn && i % 100 == 99) fprintf(stderr, " %d", i+1);
		for (n = 0; n < Numtree; n++)
			boots[n] = 0.0;
		for (k = 0; k < Numsite; k++) {
			nsite = (int)( coefrand * (double)rand() ); /* RANDOM */
			nptrn = addweight[nsite];
			for (n = 0; n < Numtree; n++)
				boots[n] += lklptrn[n][nptrn];
		}
		maxtree = 0;
		lklmax = boots[0];
		same = 0;
		for (n = 1; n < Numtree; n++) {
			if (boots[n] >= lklmax) {
				if (boots[n] > lklmax) {
					maxtree = n;
					lklmax = boots[n];
					same = 0;
				} else {
					same++;
				}
			}
		}
		allsame += same;
		infotrs[maxtree].bsprob++;
		if (Debug_optn && i < 10)
		printf("%3d%4d%10.2f%6d%6d\n", i, maxtree, lklmax, nptrn, nsite);
	}
	if (allsame > 0)
		printf("\nsame bootstrap likelihood occurred %d times\n", allsame);
	free_dvector(boots);
	free_ivector(addweight);
} /*_ bootstrap */


extern double probnormal();
extern double uprobnormal();

void
tabletree(infotrs, lklptrn)
Infotree *infotrs;
LPMATRIX lklptrn;
{
	int i, k, maxi, n;
	double ldiff, suml1, suml2, sdlkl, nn1, z;
	Infotree *info;
	LPVECTOR mlklptrn, alklptrn;

	maxi = 52;
	printf("\n%s %s %s", Prog_name, VERSION, Modelname);
	printf(" %d trees %d OTUs %d sites. %s\n\n",
		Numtree, Numspc, Numsite, Comment);
	printf("%4s%9s%11s%6s%6s%6s%10s",
	"Tree","ln L","Diff ln L","S.E.","#Para","AIC","Diff AIC");
	/* if (Mevol_optn) */
	printf("%7s", "   TBL ");
	if (Boots_optn) printf("%8s", "RELL-BP");
	putchar('\n');
	for (i = 0; i < maxi; i++) putchar('-');
	/* if (Mevol_optn) */
	{ for (i = 0; i < 7; i++) putchar('-'); }
	if (Boots_optn) { for (i = 0; i < 8; i++) putchar('-'); }
	putchar('\n');
	mlklptrn = lklptrn[Maxlkltree];
	nn1 = (double)Numsite / (double)(Numsite-1);
	for (n = 0; n < Numtree; n++) {
		info = &infotrs[n];
		alklptrn = lklptrn[n];
		printf("%-4d%11.1f%8.1f", n+1, info->lklhd, info->lklhd - Maxlkl); /* offset */
		if(n == Maxlkltree) {
			fputs(" <-best",stdout);
		} else {
			for (suml1 = suml2 = 0.0, k = 0; k < Numptrn; k++) {
				ldiff = alklptrn[k] - mlklptrn[k];
				suml1 += ldiff * Weight[k];
				suml2 += ldiff * ldiff * Weight[k];
			}
			suml1 /= Numsite;
			sdlkl = sqrt( nn1 * (suml2 - suml1*suml1*Numsite) );
			printf("%7.1f", sdlkl);
#if 0
			for (suml2 = 0.0, k = 0; k < Numptrn; k++) {
				ldiff = alklptrn[k] - mlklptrn[k] - suml1;
				suml2 += ldiff * ldiff * Weight[k];
			}
			sdlkl = sqrt( nn1 * suml2 );
			printf("%7.1f", sdlkl);
#endif
		}
		printf("%5d%10.1f%7.1f", info->npara, info->aic, info->aic - Minaic);
		/* if (Mevol_optn) { */
			if (n == Mintbltree) {
				printf("%6s ", "ME");
			} else {
				printf("%7.1f", info->tblength - Mintbl);
			}
		/* } */
		if (Boots_optn) printf("%8.4f", (double)info->bsprob / NUMBOOTS);
#if MLSITE
		if (Boots_optn) printf("%4d", info->mlsite);
#endif
#if 0
		if(n == Maxlkltree) {
			fputs("  Base",stdout);
		} else {
			z = (info->lklhd - Maxlkl) / sdlkl;
			printf(" %.3f", probnormal(z)); /* NORMAL */
		}
#endif
		putchar('\n');
	}
} /*_ tabletree */


void
tableinfo(infotrs)
Infotree *infotrs;
{
	int n;
	Infotree *info;

	putchar('\n');
	for (n = 0; n < Numtree; n++) {
		info = &infotrs[n];
		if (Boots_optn) printf("%.4f\t", (double) info->bsprob / NUMBOOTS);
		printf("%.1f\t%d\t", info->lklhd - Maxlkl, n+1); /* offset */
		puts(info->ltplgy);
	}
} /*_ tableinfo */

#if 0
void
rerootq(tr, numspc)
Tree *tr;
int numspc;
{
	Node *cp, *rp, *op, *xp, *yp;
	int i;

	for (i = 0; i < numspc; i++) {
		if (tr->bturn[i] == numspc - 1)
			 break;
	}
	rp = tr->ebrnchp[i]->kinp;
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
} /* rerootq */
#endif


void
outlklhd(lklptrn)
LPMATRIX lklptrn;
{
	int i, j, k, l, maxk;

#if 1
	fprintf(Lklfp, "%d %d ", Numtree, Numsite);
	header(Lklfp, &Maxspc, &Maxsite, &Comment);
	for (i = 0; i < Numtree; i++) {
		fprintf(Lklfp, "# %d %.1f %s;\n",
			i+1, Infotrees[i].lklhd, Infotrees[i].ltplgy);
		for (j = 0, l = 0; j < Numptrn; j++) {
			for (k = 0, maxk = Weight[j]; k < maxk; k++, l++) {
				fprintf(Lklfp, "%16.8e", lklptrn[i][j]);
				if ((l+1) % 5 == 0) fputc('\n', Lklfp);
			}
		}
		if (l % 5 != 0) fputc('\n', Lklfp);
	}
#else
	for (j = 0, l = 0; j < Numptrn; j++) {
		for (k = 0, maxk = Weight[j]; k < maxk; k++, l++) {
			for (i = 1; i < Numtree; i++) {
				fprintf(Lklfp, "%15.8f", lklptrn[0][j] - lklptrn[i][j]);
			}
			fputc('\n', Lklfp);
		}
	}
#endif
} /* outlklhd */


void
putsortseq(tr)
Tree *tr;
{
	Node *rp, *cp;
	int i, j, k;

	printf("%d %d %s\n", Maxspc, Maxsite, Comment);
	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			i = cp->num;
			printf("%s", Identif[i]);
			if (Sciname[i]) printf(" %s", Sciname[i]);
			putchar('\n');
			for (k = 0; k < Maxsite; k += 60) {
				for (j = k; j < k + 60 && j < Maxsite; j++)
					putchar(int2acid(Seqchar[i][j]));
				putchar('\n');
			}
			cp = cp->kinp;
		}
	} while (cp != rp);
} /* putsortseq */
