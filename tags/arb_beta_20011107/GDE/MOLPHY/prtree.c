/*
 * prtree.c   Adachi, J.   1995.02.05
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa, All rights reserved.
 */

#define PRINTLENGTH 1

#include "protml.h"


void
putctopology(tr)
Tree *tr;
{
	Node *cp, *rp;

	cp = rp = tr->rootp;
	putchar('(');
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			fputs(Identif[cp->num], stdout);
			cp = cp->kinp;
		} else { /* internal node */
			if (cp->descen)
				putchar('(');
			else
				putchar(')');
		}
		if (!cp->descen && !cp->isop->descen && cp != rp) /* not last subtree */
			putchar(',');
	} while (cp != rp);
	puts(");");

} /* putctopology */


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

} /* fputctopology */


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
	int i, nb, n, maxn, lim, nn;
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
		printf("-%d ", up->num + 1); /* offset */

#if PRINTLENGTH
		if (Info_optn) printf("%d ", (int)((up->length + 0.5) * 1.0));
#endif
		puts(Identif[up->num]);
		return;
	}

	nb = up->num + 1; /* offset, internal branch */
	nn = up->num - Maxspc;
	for (cp = up->isop, maxn = 0; cp != up; cp = cp->isop, maxn++)
		;
	for (cp = up->isop, n = 1; cp != up; cp = cp->isop, n++) {
		prbranch(cp->kinp, depth + 1, n, maxn, umbrella, column);
		if (m == 1 && n == maxn / 2) umbrella[depth - 1] = TRUE;
		if (n != maxn) {
			for (i = 0; i < depth - 1; i++) {
				if (umbrella[i])
					printf("%*c", column[i], ':');
				else
					printf("%*c", column[i], ' ');
			}

			i = depth - 1;
			if (umbrella[i]) {
				if (Relia_optn && Relistat[nn] > 0)
					printf("%*c", column[i], '*');
				else
					printf("%*c", column[i], ':');
			} else {
				printf("%*c", column[i], ' ');
			}

			if (n == maxn / 2) { /* internal branch */
				if (Relia_optn  && Relistat[nn] > 0) {
					for (i = 0, lim = column[depth] - 3; i < lim; i++)
						putchar('*');
					if (nb < 10)
						printf("**%d", nb);
					else if (nb < 100)
						printf("*%2d", nb);
					else
						printf("%3d", nb);
				} else {
					for (i = 0, lim = column[depth] - 3; i < lim; i++)
						putchar(bch);
					if (nb < 10)
						printf("--%d", nb);
					else if (nb < 100)
						printf("-%2d", nb);
					else
						printf("%3d", nb);
				}
#if PRINTLENGTH
				if (Info_optn) printf(" %d ", (int)((up->length + 0.5) * 1.0));
#endif
#if 1		
				if (Relia_optn) {
					if (Relistat[nn] != -1) {
						printf(" %.0f", Reliprob[nn][0] * 100.0);
						if (Relistat[nn] > 0) {
							printf(" %.0f %d&%d", Reliprob[nn][1] * 100.0,
								Relinum[nn][0]+1, Relinum[nn][1]+1);
						}
					}
				}
#endif
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
/*	putchar('\n'); */
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
strctree(tr, ltree)
Tree *tr;
char *ltree;
{
	Node *cp, *rp;
	char *idp;

	cp = rp = tr->rootp;
	*ltree++ = '(';
	do {
		cp = cp->isop->kinp;
		if (cp->isop == NULL) { /* external node */
			for (idp = Identif[cp->num]; *idp != '\0'; *ltree++ = *idp++)
				;
			cp = cp->kinp;
		} else { /* internal node */
			if (cp->descen)
				*ltree++ = '(';
			else
				*ltree++ = ')';
		}
		if (!cp->descen && !cp->isop->descen && cp != rp) /* not last subtree */
			*ltree++ = ',';
	} while (cp != rp);
	*ltree++ = ')';
	*ltree = '\0';
} /* strctree */
