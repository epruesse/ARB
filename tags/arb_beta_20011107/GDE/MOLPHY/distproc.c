/*
 * distproc.c   Adachi, J.   1994.07.22
 * Copyright (C) 1993, 1994 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "tridist.h"

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

void
getsize(ifp, numspc, commentp)
FILE *ifp;
int *numspc;
char **commentp;
{
	char *cp, *np;
	char line[BUFLINE];

	if (fgets(line, BUFLINE, ifp) != NULL) {
		if (sscanf(line, "%d", numspc) == 1) {
			for (cp = line; isdigit(*cp); cp++)
				;
			for ( ; isspace(*cp) && *cp != '\0'; cp++)
				;
			*commentp = new_cvector(strlen(cp) + 1);
			if (*cp != '\0') {
				for (np = *commentp; *cp != '\n' && *cp != '\0'; cp++)
					*np++ = *cp;
				*np = '\0';
			} else {
				**commentp = '\0';
			}
#if DEBUG
			if (Debug) printf("%d OTUs,  %s\n\n", *numspc, *commentp);
#endif
		} else {
			fputs(line, stderr);
			fprintf(stderr, "\nBad format, first line of input file.\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "\nCan't read input file.\n");
		exit(1);
	}
	return;
} /*_ getsize */


 void
header(ofp, numspc, commentp)
FILE *ofp;
int *numspc;
char **commentp;
{
	char date[32];

	strftime(date, 32, "%x", localtime(&Ct0));
	fprintf(ofp, "%s %s (%s)", Prog_name, VERSION, date);
	fprintf(ofp, " %d OTUs %s\n", *numspc, *commentp);
} /*_ header */


/* getid */
void
getid(ifp, identif, sciname, engname, nl, notu)
FILE *ifp;
char **identif;
char **sciname;
char **engname;
int *nl;
int *notu;
{
	char line[BUFLINE];
	char idbuf[BUFLINE];
	char *cp, *np;

	while (fgets(line, BUFLINE, ifp) != NULL) {
		(*nl)++; /* line counter */
		if (line[0] == '#') /* comment line skip */
			continue;
		for (cp = line; isspace(*cp); cp++) /* white line skip */
			;
		if (*cp == '\0')
			continue;
		for (np = idbuf; !isspace(*cp); *np++ = *cp++) /* identifier */
			;
		*np = '\0';
		identif[*notu] =
			(char *)malloc((unsigned)(strlen(idbuf) + 1) * sizeof(char));
		if (identif[*notu] == NULL) maerror("in getid, identif");
		strcpy(identif[*notu], idbuf);

		for ( ; isspace(*cp); cp++) /* white char skip */
			;
		/* science name */
		if (*cp != '(' && *cp != '\0' && *cp != '#' && *cp != '\n') {
			for (np = idbuf; *cp!='(' && *cp!='#' && *cp!='\0' && *cp!='\n'; *np++ = *cp++)
				;
			for ( ; isspace(*(np-1)); np--)
				;
			*np = '\0';
		} else {
			idbuf[0] = '\0';
		}
		sciname[*notu] =
			(char *)malloc((unsigned)(strlen(idbuf) + 1) * sizeof(char));
		if (identif[*notu] == NULL) maerror("in getid, sciname");
		strcpy(sciname[*notu], idbuf);

		/* english name */
		if (*cp != '\0' && *cp != '#' && *cp != '\n') { /* science name */
			for (np = idbuf; *cp!='#' && *cp!='\0' && *cp!='\n'; *np++ = *cp++)
				;
			for ( ; isspace(*(np-1)); np--)
				;
			*np = '\0';
		} else {
			idbuf[0] = '\0';
		}
		engname[*notu] =
			(char *)malloc((unsigned)(strlen(idbuf) + 1) * sizeof(char));
		if (identif[*notu] == NULL) maerror("in getid, engname");
		strcpy(engname[*notu], idbuf);

#ifdef DEBUG
		if (Debug) {
			printf("%3d spc. \"%s\"", *notu+1, identif[*notu]);
			if (sciname[*notu] != '\0') {
				putchar(' ');
				fputs(sciname[*notu], stdout);
			}
			if (engname[*notu] != '\0') {
				putchar(' ');
				fputs(engname[*notu], stdout);
			}
			putchar('\n');
		}
#endif
		break;
	}
	return;
} /*_ getid */


/* getdist */
void
getdist(ifp, distanmat, numspc, nl, notu)
FILE *ifp;
dmatrix distanmat;
int numspc, *nl, *notu;
{
	char line[BUFLINE];
	char *cp, *np, **npp;
	int motu;
	double dis;

	motu = 0;
	npp = &np;
	while (fgets(line, BUFLINE, ifp) != NULL) {
		(*nl)++; /* line counter */
		if (line[0] == '#') continue; /* comment line skip */
		for (cp = line; (dis = strtod(cp,npp)) || np != cp; cp = np, motu++) {
			if (*notu < motu) {
				distanmat[*notu][motu] = dis;
			} else if (*notu == motu) {
				if (dis == 0.0) {
					distanmat[*notu][motu] = 0.0;
				} else {
					fprintf(stderr, "error! distanmat[%d][%d] = %f.5\n",
						*notu, motu, dis);
					exit(1);
				}
			} else {
				if (distanmat[motu][*notu] != dis) {
					distanmat[*notu][motu] = dis * 10000.0;
				} else {
					distanmat[*notu][motu] = dis * 100.0;
				}
				distanmat[motu][*notu] *= 100.0;
			}
			if (Debug_optn) printf("%8.3f", dis);
			if (motu > numspc) { /* || (np == cp && np != NULL) */
				fputs(cp, stderr);
				exit(1);
			}
		}
		if (motu >= numspc)
			break;
	}
	if (Debug_optn) putchar('\n');
	return;
} /* getdist */


void getdata(ifp, identif, sciname, engname, distanmat, numspc)
FILE *ifp;
char **identif, **sciname, **engname;
dmatrix distanmat;
int numspc;
{
	int notu;
	int nl = 0;

	for (notu = 0; notu < numspc; notu++) {
		getid(ifp, identif, sciname, engname, &nl, &notu);
		getdist(ifp, distanmat, numspc, &nl, &notu);
	}
	return;
} /*_ getdistan */


void getdatas(ifp, identif, distanmat, numspc)
FILE *ifp;
char **identif;
dmatrix distanmat;
int numspc;
{
	int notu, motu, i;
	double dis;
	char line[BUFLINE];
	char idbuf[BUFLINE];
	char *cp, *np, **npp;

	for (notu = 0; notu < numspc; notu++) {
		motu = -1;
		while (fgets(line, BUFLINE, ifp) != NULL) {
			if (line[0] == '#') /* comment line skip */
				continue;
			for (cp = line; isspace(*cp); cp++) /* white line skip */
				;
			if (*cp == '\0')
				continue;
			if (motu == -1) {
				for (np=idbuf, i=0; !isspace(*cp) && i<NMLNGTH; *np++ = *cp++, i++)
					; /* identifier */
				*np = '\0';
				identif[notu] =
					(char *)malloc((unsigned)(strlen(idbuf)+1) * sizeof(char));
				if (identif[notu] == NULL) maerror("in getid, identif");
				strcpy(identif[notu], idbuf);
				motu = 0;
			}
			npp = &np;
			for ( ; (dis = strtod(cp,npp)) || np != cp; cp = np, motu++) {
				distanmat[notu][motu] = dis * 100.0;
				if (Debug_optn) printf("%8.3f", dis);
				if (motu > numspc) { /* || (np == cp && np != NULL) */
					fputs(cp, stderr);
					exit(1);
				}
			}
			if (motu >= numspc)
				break;
		}
		if (Debug_optn) putchar('\n');
	}
	return;
} /*_ getdistan */


void
copydist(distanmat2, distanmat, numspc)
dmatrix distanmat2, distanmat;
int numspc;
{
	int i, j;

	for (i = 0; i < numspc; i++)
		for (j = 0; j < numspc; j++)
			distanmat2[i][j] = distanmat[i][j];
} /*_ copydist */


void
changedistan(distanmat, distanvec, numspc)
dmatrix distanmat;
dvector distanvec;
int numspc;
{
	int i, j, k;

	for (k = 0, i = 0; i < (numspc - 1); i++) {
		for (j = i + 1; j < numspc; j++, k++)
			distanvec[k] = distanmat[i][j];
	}
}


Tree *
newtree(maxbrnch)
int maxbrnch;
{
	int n, i;
	Tree *tr;
	Node *dp, *up;

	tr = (Tree *) malloc(sizeof(Tree));
	if (tr == NULL) maerror("tr in newtree().");
	tr->brnchp  = (Node **) malloc((unsigned) (maxbrnch * sizeof(Node *)));
	if (tr->brnchp == NULL) maerror("brnchp in newtree().");
	tr->paths = new_imatrix(maxbrnch, Numspc);
	for (n = 0; n < maxbrnch; n++) {
		dp = (Node *) malloc((unsigned) sizeof(Node));
		if (dp == NULL) maerror("dp in newtree().");
		up = (Node *) malloc((unsigned) sizeof(Node));
		if (up == NULL) maerror("up in newtree().");
		dp->isop = NULL;
		up->isop = NULL;
		dp->descen = TRUE;
		up->descen = FALSE;
		dp->num = n;
		up->num = n;
		dp->length = 0.0;
		up->length = 0.0;
		dp->kinp = up;
		up->kinp = dp;
		tr->brnchp[n] = dp;
		for (i = 0; i < Numspc; i++) {
			if (i == n)
				tr->paths[n][i] = TRUE;
			else
				tr->paths[n][i] = FALSE;
		}
	}
	tr->rootp = NULL;
	tr->firstp = tr->brnchp[0]->kinp; /* !? */

	return tr;
} /*_ newtree */


#ifdef DEBUG
void 
prcurtree(tr)
Tree *tr;
{
	Node *dp, *up, *cp;
	int i;

/*	printf("\nStructure of Tree\n"); */
	printf("\n%4s%5s%5s%5s%7s%11s%13s\n",
	"num","kinp","isop","isop","descen","length", "namesp");
	for (i = 0; i < Numbrnch; i++) {
		dp = tr->brnchp[i];
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
		if (i < Numspc) {
			for (cp = up->isop; cp != up; cp = cp->isop) {
				printf("%5d", cp->num+1);
			}
		}
		putchar('\n');
	}
		dp = tr->rootp;
		printf("%4d", dp->num+1);
		for (cp = dp->isop; cp != dp; cp = cp->isop) {
			printf("%5d", cp->num+1);
		}
		putchar('\n');
} /*_ prcurtree */
#endif


void
pathing(tr)
Tree *tr;
{
	Node *cp, *rp, *xp, *yp;
	int i;
	imatrix pths;

	pths = tr->paths;
	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			cp = cp->kinp;
		} else { /* internal node */
			if (!cp->descen) {
				for (yp = cp->kinp, xp = yp->isop; xp != yp; xp = xp->isop) {
					for (i = 0; i < Numspc; i++)
						if (pths[xp->num][i])
							pths[yp->num][i] = TRUE;
				}
			}
		}
	} while (cp != rp);
} /* pathing */


void
getproportion(proportion, distanmat, maxspc)
double *proportion;
dmatrix distanmat;
int maxspc;
{
	int i, j;
	double maxdis;

	maxdis = 0.0;
	for (i = 1; i < maxspc; i++) {
		for (j = 0; j < i; j++) {
			if (distanmat[i][j] > maxdis) {
				maxdis = distanmat[i][j];
			}
		}
	}
	*proportion = (double)MAXCOLUMN / (maxdis * 3.0);
	if (*proportion > 1.0) *proportion = 1.0;
	if (Debug) printf("Proportion: %.5f   maxdis: %.5f\n", *proportion,maxdis);
} /* getproportion */


void
copylengths(tr, lengths, numbrnch)
Tree *tr;
dvector lengths;
int numbrnch;
{
	int i;
	double leng;

	for (i = 0; i < numbrnch; i++) {
		leng = lengths[i];
		tr->brnchp[i]->length = leng;
		tr->brnchp[i]->kinp->length = leng;
	}
} /*_ copylengths */


void
prdistanmat(distanmat, numspc)
dmatrix distanmat;
int numspc;
{
	int i, j, k, n, m;

	for (k = 0; k < numspc; k = m) {
		fputs("\n     ", stdout);
		for (n = 0, j = k; n < 10 && j < numspc; j++, n++) {
			printf("%7.5s", Identif[j]);
		}
		m = j;
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			printf("%-5.5s", Identif[i]);
			for (j = k; j < m && j < numspc; j++) {
				if (i == j)
					printf("%7.5s", Identif[i]);
				else
					printf("%7.2f", distanmat[i][j]);
			}
			printf("\n");
		}
	}
} /* prdistanmat */


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
lslength(tr, distanvec, lengths)
Tree *tr;
dvector distanvec;
dvector lengths;
{
	int i, i1, i2, j, k;
	double sum, leng, alllen, rss;
	imatrix pths;
	dmatrix amt, atamt;

	amt = new_dmatrix(Numpair, Numbrnch);
	atamt = new_dmatrix(Numbrnch, Numbrnch);
	pths = tr->paths;
	if (Debug) {
		putchar('\n');
		for (i = 0; i < Numbrnch; i++) {
			for (j = 0; j < Numspc; j++) {
				printf("%2d",pths[i][j]);
			} putchar('\n');
		}
	}
	for (i = 0, i1 = 0; i1 < (Numspc - 1); i1++) {
		for (i2 = i1 + 1; i2 < Numspc; i2++, i++) {
			for (j = 0; j < Numbrnch; j++) {
				if (pths[j][i1] != pths[j][i2])
					amt[i][j] = 1.0;
				else
					amt[i][j] = 0.0;
			}
		}
	}
	if (Debug) {
		putchar('\n');
		for (i = 0; i < Numpair; i++) {
			for (j = 0; j < Numbrnch; j++)
				printf("%3.0f",amt[i][j]);
			printf("%6.1f\n",distanvec[i]);
		}
	}
#ifdef DIST
		putchar('\n');
		for (i = 0; i < Numpair; i++)
			printf("%5.1f",distanvec[i]);
		putchar('\n');
#endif
	for (i = 0; i < Numbrnch; i++) {
		for (j = 0; j < Numbrnch; j++) {
			for (k = 0, sum = 0.0; k < Numpair; k++)
				sum += amt[k][i] * amt[k][j];
			atamt[i][j] = sum;
		}
		for (k = 0, sum = 0.0; k < Numpair; k++)
			sum += amt[k][i] * distanvec[k];
		lengths[i] = sum;
	}

	luequation(atamt, lengths, Numbrnch);
/*
	putchar('\n');
	for (i = 0; i < Numbrnch; i++) {
		for (j = 0; j < Numbrnch; j++)
			printf("%5.1f", atamt[i][j]);
		printf("%7.3f",lengths[i]);
		putchar('\n');
	}
*/
	if (Debug) {
		for (i = 0; i < Numbrnch; i++) {
			printf("%7.3f",lengths[i]);
		} putchar('\n');
	}
	for (i = 0, rss = 0.0; i < Numpair; i++) {
		sum = distanvec[i];
		for (j = 0; j < Numbrnch; j++) {
			if (amt[i][j] == 1.0 && lengths[j] > 0.0)
				sum -= lengths[j];
		}
		rss += sum * sum;
	}
	tr->rssleast = sqrt(rss / Numpair); /* !? */
	for (i = 0, alllen = 0.0; i < Numbrnch; i++) {
		leng = lengths[i];
	/*	if (leng > 0.0) */
			alllen += leng;
		if (leng < 0.0) leng = 0.0;
	/*	tr->brnchp[i]->length = leng;
		tr->brnchp[i]->kinp->length = leng; */
	}
	tr->ablength = alllen;
	free_dmatrix(amt);
	free_dmatrix(atamt);
	free_dvector(lengths);
} /*_ lslength */


void
fputctopology(fp, tr)
FILE *fp;
Tree *tr;
{
	Node *cp, *rp;

	cp = rp = tr->rootp;
	putc('(', fp);
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			fputs(Identif[cp->num], fp);
			cp = cp->kinp;
		} else { /* internal node */
			if (cp->descen)
				putc('(', fp);
			else
				putc(')', fp);
		}
		if (!cp->descen && !cp->isop->descen && cp != rp) /* not last subtree */
			putc(',', fp);
	} while (cp != rp);
	fputs(");\n", fp);

} /* putctopology */


void
fputcphylogeny(fp, tr)
FILE *fp;
Tree *tr;
{
	Node *cp, *rp;
	int n;

	cp = rp = tr->rootp;
	putc('(', fp);
	n = 1;
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			if (n > 60) { putc('\n', fp); n = 0; }
			fputs(Identif[cp->num], fp);
			n += strlen(Identif[cp->num]);
			fprintf(fp, ":%.3f", cp->length);
			n += 6;
			cp = cp->kinp;
		} else { /* internal node */
			if (cp->descen) {
				if (n > 60) { putc('\n', fp); n = 0; }
				putc('(', fp);
				n++;
			} else {
				putc(')', fp);
				n++;
				if (n > 70) { putc('\n', fp); n = 0; }
				fprintf(fp, ":%.3f", cp->length);
				n += 6;
			}
		}
		if (!cp->descen && !cp->isop->descen && cp != rp) {
			putc(',', fp); /* not last subtree */
			n++;
		}
	} while (cp != rp);
	fputs(");\n", fp);
} /* fputcphylogeny */


static void 
prbranch(up, depth, m, maxm, umbrella, column)
Node *up;
int depth, m, maxm;
ivector umbrella;
ivector column;
{
	int i, nn, n, maxn, lim;
	Node *cp;
	char bch;

	if ((int)(up->length * Proportion) >= MAXOVER) {
		column[depth] = MAXLENG;
		bch = '+';
	} else {
		column[depth] = (int)(up->length * Proportion) + 3;
		bch = '-';
	}

	if (up->isop == NULL) { /* external branch */
		nn = up->num + 1; /* offset */
		if (m == 1) umbrella[depth - 1] = TRUE;
		for (i = 0; i < depth; i++) {
			if (umbrella[i])
				printf("%*c", column[i], ':');
			else
				printf("%*c", column[i], ' ');
		}
		if (m == maxm) umbrella[depth - 1] = FALSE;
		for (i = 0, lim = column[depth] - 3; i < lim; i++)
			putchar(bch);
		printf("-%d ", nn);
		puts(Identif[up->num]);
		return;
	}

	nn = up->num + 1; /* + Numspc offset, internal branch */
	for (cp = up->isop, maxn = 0; cp != up; cp = cp->isop, maxn++)
		;
	for (cp = up->isop, n = 1; cp != up; cp = cp->isop, n++) {
		prbranch(cp->kinp, depth + 1, n, maxn, umbrella, column);
		if (m == 1 && n == maxn / 2) umbrella[depth - 1] = TRUE;
		if (n != maxn) {
			for (i = 0; i < depth; i++) {
				if (umbrella[i])
					printf("%*c", column[i], ':');
				else
					printf("%*c", column[i], ' ');
			}
			if (n == maxn / 2) { /* internal branch */
				for (i = 0, lim = column[depth] - 3; i < lim; i++)
					putchar(bch);
				if (nn < 10)
					printf("--%d", nn);
				else if (nn < 100)
					printf("-%2d", nn);
				else
					printf("%3d", nn);
			} else {
				if (umbrella[depth])
					printf("%*c", column[depth], ':');
				else
					printf("%*c", column[depth], ' ');
			}
			putchar('\n');
		}
		if (m == maxm) umbrella[depth - 1] = FALSE;
	}
	return;
} /*_ prbranch */


void 
prtopology(tr)
Tree *tr;
{
	int n, maxn, depth;
	ivector umbrella;
	ivector column;
	Node *cp, *rp;

	umbrella = new_ivector(Numspc);
	column = new_ivector(Numspc);

	for (n = 0; n < Numspc; n++) {
		umbrella[n] = FALSE;
		column[n] = 3;
	}
	column[0] = 1;
	rp = tr->rootp;
	for (maxn = 1, cp = rp->isop; cp != rp; cp = cp->isop, maxn++)
		;
	depth = 1;
	n = 0;
	putchar('\n');
	cp = rp;
	do {
		cp = cp->isop;
		n++;
		prbranch(cp->kinp, depth, n, maxn, umbrella, column);
		if (cp != rp) printf("%*c\n", column[0], ':');
	} while (cp != rp);

	free_ivector(umbrella);
	free_ivector(column);
} /*_ prtopology */


void
resulttree(tr)
Tree *tr;
{
	int ne, ni;
	Node *ep, *ip;

/*	printf("\nNo.%-3d", Cnotree + 1);  offset */
	printf("\n%6s", "");
	if (Least_optn)
		printf("%9s%7s%7s%8s%7s%7s\n", "num","NJ ","L.S.","num","NJ ","L.S.");
	else
		printf("%9s%7s%8s%7s\n", "num","length","num","length");
	for (ne = 0, ni = Numspc; ne < Numspc; ne++, ni++) {
		ep = tr->brnchp[ne];
		printf("%-10.10s", Identif[ne]);
		fputs("  ", stdout);
		printf("%3d%7.2f", ne + 1, ep->length); /* offset */
		if (Least_optn)
			printf("%7.2f", Lengths[ne]);
		if (ni < Numbrnch) {
			ip = tr->brnchp[ni];
			printf("%8d%7.2f", ni + 1, ip->length); /* offset */
			if (Least_optn)
				printf("%7.2f", Lengths[ni]);
			putchar('\n');
		} else {
#if 0
			if (ne == Numspc - 2 && Info_optn && Least_optn)
				printf("%12s%10.1f", "RS    :",tr->rssleast);
			if (ne == Numspc - 1 && Info_optn)
				printf("%12s%10.1f", "Total :",tr->ablength);
#endif
			putchar('\n');
		}
	}
} /*_ resulttree */


void
reroot(tr, rp)
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
		} else { /* internal node */
			if (cp->descen != -1) {
				cp->descen = TRUE;
				cp->kinp->descen = -1;
				tr->brnchp[cp->num] = cp;
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
} /* reroot */
