/*
 * seqproc.c   Adachi, J.   1994.01.21
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "protml.h"


void
convseq(seqconint, maxspc, numptrn)
imatrix seqconint;
int maxspc, numptrn;
{
	int i, j;

	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < numptrn; j++) {
			if (seqconint[i][j] == Tpmradix) {
				seqconint[i][j] = -1;
			}
		}
	}
#ifdef SEQ_DEBUG
	if (Debug) {
	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < numptrn; j++) {
			if ((j % 30) == 0) putchar('\n');
			printf("%2d",seqconint[i][j]);
		}
	} putchar('\n');
	}
#endif
} /*_ convseq */


void
getfreqepm(seqchar, freqemp, maxspc, maxsite)
cmatrix seqchar;
double *freqemp;
int maxspc, maxsite;
{
	int i, j, all;
	ivector gene;

	gene = new_ivector(Tpmradix + 1);
	for (i = 0; i < Tpmradix + 1; i++)
		gene[i] = 0;
	for (i = 0; i < maxspc; i++) {
		for (j = 0; j < maxsite; j++) {
			gene[(int)seqchar[i][j]]++;
		}
	}
	all = maxspc * maxsite - gene[Tpmradix];
	for (i = 0; i < Tpmradix; i++) {
		freqemp[i] = (double)gene[i] / (double)all;
		if (Debug) printf("%4d%8d%10.5f\n", i, gene[i], freqemp[i]);
	}
	if (Debug) printf("all:%8d   other:%5d\n", all, gene[Tpmradix]);
	free_ivector(gene);

} /*_ getfreqepm */


void
convfreq(freqemp)
double *freqemp;
{
	int i, maxi;
	double freq, maxfreq, sum;

	sum = 0.0;
	maxfreq = 0.0;
	for (i = 0; i < Tpmradix; i++) {
		freq = freqemp[i];
		if (freq < MINFREQ)
			freqemp[i] = MINFREQ;
		if (freq > maxfreq) {
			maxfreq = freq;
			maxi = i;
		}
		sum += freqemp[i];
	}
	freqemp[maxi] += 1.0 - sum;

	if (Debug) {
		putchar('\n');
		for (i = 0, sum = 0.0; i < Tpmradix; i++) {
			printf(" %2d %12.8f\n", i, freqemp[i]);
			sum += freqemp[i];
		}
		printf("sum %12.8f\n", sum);
	}
} /* convfreq */


void a_radixsort(seqchar, alias, maxspc, maxsite, numptrn)
cmatrix seqchar;
ivector alias;
int maxspc, maxsite;
int *numptrn;
{
	int i, j, k, l, n, pass;
	int *awork;
	int *count;

	awork = new_ivector(maxsite);
	count = new_ivector(Tpmradix+1);
	for (i = 0; i < maxsite; i++)
		alias[i] = i;
	for (pass = maxspc - 1; pass >= 0; pass--) {
		for (j = 0; j < Tpmradix+1; j++)
			count[j] = 0;
		for (i = 0; i < maxsite; i++)
			count[seqchar[pass][alias[i]]]++;
		for (j = 1; j < Tpmradix+1; j++)
			count[j] += count[j-1];
		for (i = maxsite-1; i >= 0; i--)
			awork[ --count[seqchar[pass][alias[i]]] ] = alias[i];
		for (i = 0; i < maxsite; i++)
			alias[i] = awork[i];
	}
	free_ivector(awork);
	free_ivector(count);
	n = 1;
	for (j = 1; j < maxsite; j++) {
		k = alias[j];
		l = alias[j-1];
		for (i = 0; i < maxspc; i++) {
			if (seqchar[i][l] != seqchar[i][k]) {
				n++;
				break;
			}
		}
	}
	*numptrn = n;
#ifdef SEQ_DEBUG
	if (Debug_optn) {
		printf("numpatrn =%4d", *numptrn);
		for (k = 0; k < maxsite; k += 30) {
			putchar('\n');
			for (i = 0; i < maxspc; i++) {
				for (j = k; j < maxsite && j < k+30; j++)
					printf("%2d", seqchar[i][alias[j]]);
				putchar('\n');
			}
		}
	}
#endif
} /*_ a_radixsort */


void
condenceseq(seqchar, alias, seqconint, weight, maxspc, maxsite, numptrn)
cmatrix seqchar;
ivector alias;
imatrix seqconint;
ivector weight;
int maxspc, maxsite, numptrn;
{
	int i, j, k, n;
	int agree_flag; /* boolean */

	n = 0;
	k = alias[n];
	for (i = 0; i < maxspc; i++) {
		seqconint[i][n] = seqchar[i][k];
	}
	weight[n] = 1;
	for (j = 1; j < maxsite; j++) {
		k = alias[j];
		agree_flag = TRUE;
		for (i = 0; i < maxspc; i++) {
			if (seqconint[i][n] != seqchar[i][k]) {
				agree_flag = FALSE;
				break;
			}
		}
		if (agree_flag == FALSE) {
			n++;
			for (i = 0; i < maxspc; i++) {
				seqconint[i][n] = seqchar[i][k];
			}
			weight[n] = 1;
		} else {
			weight[n]++;
		}
	}
	n++;
	if (numptrn != n) {
		printf("ERROR in condenceseq. numptrn != number of array of seqconint");
		exit(1);
	}
} /*_ condenceseq */


void
getnumsites(seqconint, numsites, weight, numspc, numptrn)
imatrix seqconint;
ivector numsites;
ivector weight;
int numspc;
int numptrn;
{
	 int i, j, n;
	 ivector seq;

	 for (i = 0; i < numspc; i++) {
		seq = seqconint[i];
		for (j = 0, n = 0; j < numptrn; j++) {
			if (seq[j] >= 0) n += weight[j];
		}
		numsites[i] = n;
	/*	printf("numsites %3d %5d\n", i ,n); */
	 }
} /* getnumsites */


void
prcondenceseq(identif, seqconint, weight, numspc, numsite, numptrn)
char **identif;
imatrix seqconint;
ivector weight;
int numspc;
int numsite;
int numptrn;
{
	int i, j, k, l, m, n, num, maxj, deci10;

	num = 0;
	while ((numsite /= 10) > 0) num++;
	for (k = 0; k < numptrn; k += LINESITES) {
		maxj = ((numptrn < k+LINESITES) ? numptrn : (k+LINESITES));
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			fputid(stdout, identif[i], 10);
			putchar(' ');
			for (j = k; j < maxj; j++) {
				if (i == 0 || seqconint[i][j] != seqconint[0][j])
					printf("%c", int2acid(seqconint[i][j]));
				else
					putchar('.');
			}
			putchar('\n');
		}
		n = num;
		do 	{
			fputs("           ", stdout);
			for (l = 0, deci10 = 1; l < n; l++) deci10 *= 10;
			for (j = k; j < maxj; j++) {
				if ((m = weight[j] / deci10) > 0) {
					if ((m %= 10) != 0)
						printf("%1d", m);
					else
						putchar('0');
				} else {
					putchar(' ');
				}
			}
			putchar('\n');
		} while (n-- > 0);
	}
} /*_ prcondenceseq */
