/*
 * seqstat.c   Adachi, J.   1995.03.18
 * Copyright (C) 1993-1995 J. Adachi & M. Hasegawa, All rights reserved.
 */

#define STGRAPH 0
#define RRTEST 0
#define RRTESTNUM 1000 /* 100000 */

#include "protst.h"


void
seqinsdel(seqchar, numspc, numsite, insdel, numnoinsdel)
cmatrix seqchar;
int numspc, numsite;
ivector insdel;
int *numnoinsdel;
{
	int i, k, ninsdel;

	ninsdel = 0;
	for (k = 0; k < numsite; k++) {
		insdel[k] = FALSE;
		for (i = 0; i < numspc; i++) {
			if (seqchar[i][k] == Tpmradix) {
				insdel[k] = TRUE;
				ninsdel++;
				break;
			}
		}
	}
	*numnoinsdel = numsite - ninsdel;
} /* seqinsdel */


#if RRTEST
void
rrtest(seqchar, numspc, numsite, insdel, numnoinsdel)
cmatrix seqchar;
int numspc, numsite;
ivector insdel;
int numnoinsdel;
{
	int i, j, k, dif, nsite, nsp1, comdiff;
	double coefrand;
	cvector seqchi, seqchj;
	ivector diff;
	imatrix hist;

	nsp1 = numspc - 1;
	coefrand = (double)numsite / ((double)RANDOM_MAX + 1.0);
	diff = new_ivector(numspc);
	hist = new_imatrix(numspc, numsite);
	for (i = 0; i < numspc; i++) {
		for (j = 0; j < numsite; j++) hist[i][j] = 0;
	}
	seqchi = seqchar[numspc - 1];

	/* putchar('\n'); */
	for (i = 0; i < RRTESTNUM; i++) {
		for (j = 0; j < nsp1; j++) {
			seqchj = seqchar[j];
			for (k = 0, dif = 0; k < numsite; k++) {
				nsite = (int)( coefrand * (double)rand() ); /* RANDOM */
				if (!insdel[nsite]) {
					if (seqchi[nsite] != seqchj[nsite]) dif++;
				}
			}
			diff[j] = dif;
			hist[j][dif]++;
		}
		diff[numspc - 1] = 0;
		/*
		printf("%5d", i+1);
		for (j = 0; j < nsp1; j++) printf(" %4d",diff[j]); putchar('\n');
		*/
	}

	putchar('\n');
	for (i = 0; i < nsp1 - 1; i++) {
		for (j = i + 1; j < nsp1; j++) {
			comdiff = 0;
			for (k = 0; k < numsite; k++) {
				if (hist[i][k] <= hist[j][k]) {
					dif = hist[i][k];
				} else {
					dif = hist[j][k];
				}
				comdiff += dif;
			}
			printf("%3d %3d %8d %9.5f\n",
				i+1, j+1, comdiff, (double)comdiff/RRTESTNUM);
		}
	}

	printf("\n%5s", "diff");
	for (i = 0; i < nsp1; i++) printf(" %4d",i+1); putchar('\n');
	for (j = 0; j < numsite; j++) {
		printf("%5d", j);
		for (i = 0; i < nsp1; i++) printf(" %4d",hist[i][j]); putchar('\n');
	}

	free_ivector(diff);
	free_imatrix(hist);
	exit(1);
} /* rrtest */
#endif /*RRTEST*/


void
seqdiff(seqchar, numspc, numsite, insdel, numnoinsdel)
cmatrix seqchar;
int numspc, numsite;
ivector insdel;
int numnoinsdel;
{
	int i, j, k, x, y, z, dif1, dif2, maxk, numk;
	cvector seqchi, seqchj;
	imatrix diff;
#if STGRAPH
	double v, s, vsd2, ssd2;
#endif

	diff = new_imatrix(numspc, numspc);
	for (i = 0; i < numspc-1; i++) {
		seqchi = seqchar[i];
		for (j = i+1; j < numspc; j++) {
			seqchj = seqchar[j];
		/*	for (k=0;k<numsite;k++) printf("%2d",seqchj[k]); putchar('\n'); */
			for (k = 0, dif1 = 0, dif2 = 0; k < numsite; k++) {
				if (!insdel[k]) {
					if ((x = seqchi[k]) != (y = seqchj[k])) {
#ifndef NUC
						dif1++;
#else
						z = x + y;
						if ((z == 1) || (z == 5))
							dif1++;
						else
							dif2++;
#endif /* NUC */
					}
				}
			}
#ifndef NUC
			diff[i][j] = dif1;
			diff[j][i] = dif1;
#else
			diff[i][j] = dif1;
			diff[j][i] = dif2;
#endif /* NUC */
		}
		diff[i][i] = 0;
	}
	diff[numspc-1][numspc-1] = 0;
 
	if (!Align_optn) {
		if (numnoinsdel != numsite)
			printf("\nexcluding ins/del sites: %d\n", numnoinsdel);
		numk = Maxelement;
		for (k = 0; maxk = k + numk, k < numspc; k += numk) {
			if (maxk > numspc) maxk = numspc;
			if (Tpmradix == NUMAMI)
				printf("\n%4s%6s", "Diff","");
			else
				printf("\n%4s%-6s", " ","Ts");
			for (j = k; j < maxk; j++) printf("%4d", j + 1);
			putchar('\n');
			if (Tpmradix == NUMAMI)
				printf("%4s%6s", "","");
			else
				printf("%-4s%6s", "Tv","");
			for (j = k; j < maxk; j++) printf("%4.3s", Identif[j]);
			putchar('\n');
			for (i = 0; i < numspc; i++) {
				printf("%-4d%-6.6s", i + 1, Identif[i]);
				for (j = k; j < maxk; j++) {
					if (i != j)
						printf("%4d", diff[i][j]);
					else
						printf("%4.3s", Identif[i]);
				}
				putchar('\n');
			}
		}
	} /* Align_optn */

#if STGRAPH
	putchar('\n');
	for (i = 0; i < numspc - 1; i++) {
		for (j = i + 1; j < numspc; j++) {
			v = (double)diff[j][i];
			s =	(double)diff[i][j];
			vsd2 = 2 * sqrt(v * (1 - v/numsite)) / numsite;
			ssd2 = 2 * sqrt(s * (1 - s/numsite)) / numsite;
			printf("%9.5f%9.5f%9.5f%9.5f%9.5f%9.5f\n",
				v/numsite, v/numsite - vsd2, v/numsite + vsd2,
				s/numsite, s/numsite - ssd2, s/numsite + ssd2);
		}
	}
#endif /* STGRAPH */

	free_imatrix(diff);

#if RRTEST
	rrtest(seqchar, numspc, numsite, insdel, numnoinsdel);
#endif /* RRTEST */

} /* seqdiff */


void
seqfreq(seqchar, numspc, numsite, insdel, numnoinsdel)
cmatrix seqchar;
int numspc, numsite;
ivector insdel;
int numnoinsdel;
{
	int i, j, k, l, numfreq, numk, maxk;
	double temp, bias, skew, nid1, nid2;
	cvector seqchi;
	imatrix freqs;
	ivector freq;
	ivector freqall;

	nid1 = 1.0 / numnoinsdel;
	nid2 = 1.0 / (numnoinsdel * numnoinsdel);
	freqs = new_imatrix(numspc, Tpmradix + 1);
	freqall = new_ivector(Tpmradix + 1);
	for (j = 0; j < Tpmradix + 1; j++) freqall[j] = 0;
	for (i = 0; i < numspc; i++) {
		seqchi = seqchar[i];
		freq = freqs[i];
		for (j = 0; j < Tpmradix + 1; j++) freq[j] = 0;
		for (k = 0; k < numsite; k++) {
			if (!insdel[k])
				freq[seqchi[k]]++;
		}
		for (j = 0; j < Tpmradix + 1; j++) freqall[j] += freq[j];
	}

	if (Tpmradix == NUMAMI)
		numfreq = NUMAMI / 2;
	else
		numfreq = NUMNUC;

	if (!Align_optn) {
		printf("\n%-10s", ""); /* Frequencies, Composition */
		if (Tpmradix == NUMAMI) {
			for (j = 0; j < numfreq; j++)
				printf("%3s%4s", Cacid1[j], Cacid3[j]);
		} else {
			for (j = 0; j < numfreq; j++) printf("%5s%2s", Cacid1[j], " ");
			printf("%7.4s%7.4s%7.4s%7.4s", "A+T ", "G+C ", "Bias", "Skew");
		}
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			freq = freqs[i];
			printf("%-4d%-6.6s", i + 1, Identif[i]);
			for (j = 0; j < numfreq ; j++) {
				if (freq[j] != 0)
					printf("%7.3f", (double)freq[j] / numnoinsdel);
				else
					printf("%5.1f  ", (double)freq[j] / numnoinsdel);
			}
			if (Tpmradix == NUMNUC) {
				printf("%7.3f", (double)(freq[0]+freq[2]) / numnoinsdel);
				printf("%7.3f", (double)(freq[1]+freq[3]) / numnoinsdel);
				for (l = 0, bias = 0.0; l < Tpmradix; l++) {
					temp = (freq[l] * nid1 - 0.25);
					bias += temp * temp;
				}
				bias = bias * 4 / 3;
				printf("%7.3f", bias);
				skew = fabs(freq[0] - freq[2]) + fabs(freq[1] - freq[3]);
				skew /= numnoinsdel;
				printf("%7.3f", skew);
			}
			putchar('\n');
		}
		printf("%4s%-6s", " ", "mean");
		for (j = 0; j < numfreq ; j++) {
			if (freqall[j] != 0)
				printf("%7.3f", (double)freqall[j] / (numspc * numnoinsdel));
			else
				printf("%5.1f  ", (double)freqall[j] / (numspc * numnoinsdel));
		}
		if (Tpmradix == NUMNUC) {
			printf("%7.3f",
				(double)(freqall[0]+freqall[2])/(numspc*numnoinsdel));
			printf("%7.3f",
				(double)(freqall[1]+freqall[3])/(numspc*numnoinsdel));
			for (l = 0, bias = 0.0; l < Tpmradix; l++) {
				temp = (freqall[l] * nid1 / numspc - 0.25);
				bias += temp * temp;
			}
			bias = bias * 4 / 3;
			printf("%7.3f", bias);
			skew = fabs(freqall[0] - freqall[2])
				 + fabs(freqall[1] - freqall[3]);
			skew /= (numspc * numnoinsdel);
			printf("%7.3f", skew);
		}
		putchar('\n');
	
		if (Tpmradix == NUMAMI) {
		printf("\n%-10s", ""); /* Frequencies, Composition */
		if (Tpmradix == NUMAMI) {
			for (j = numfreq; j < Tpmradix; j++)
				printf("%3s%4s", Cacid1[j], Cacid3[j]);
		} else {
			for (j = numfreq; j < Tpmradix; j++)
				printf("%5s%2s", Cacid1[j], " ");
		}
		putchar('\n');
		for (i = 0; i < numspc; i++) {
			freq = freqs[i];
			printf("%-4d%-6.6s", i + 1, Identif[i]);
			for (j = numfreq; j < Tpmradix ; j++) {
				if (freq[j] != 0)
					printf("%7.3f", (double)freq[j] / numnoinsdel);
				else
					printf("%5.1f  ", (double)freq[j] / numnoinsdel);
			}
			putchar('\n');
		}
		printf("%4s%-6s", " ", "mean");
		for (j = numfreq; j < Tpmradix ; j++) {
			if (freqall[j] != 0)
				printf("%7.3f", (double)freqall[j] / (numspc * numnoinsdel));
			else
				printf("%5.1f  ", (double)freqall[j] / (numspc * numnoinsdel));
		} putchar('\n');
		}
	} /* Align_optn */

	if (!Align_optn) {
		if (Maxspc > 28) putchar('\f');
		numk = Maxelement;
		for (k = 0; maxk = k + numk, k < numspc; k += numk) {
			if (maxk > numspc) maxk = numspc;
#if 0
			printf("\n%4s%6s", "Bias"," x1e5");
#else
			printf("\n%4s%6s", "Bias"," x1e3");
#endif
			for (j = k; j < maxk; j++) printf("%4d", j + 1); putchar('\n');
			printf("%4s%6s", "","");
			for (j = k; j < maxk; j++) printf("%4.3s", Identif[j]);
			putchar('\n');
			for (i = 0; i < numspc; i++) {
				printf("%-4d%-6.6s", i + 1, Identif[i]);
				for (j = k; j < maxk; j++) {
					if (i != j) {
#if 0
						for (l = 0, bias = 0.0; l < Tpmradix; l++) {
							temp = (freqs[i][l] - freqs[j][l]);
							bias += temp * temp;
						}
						bias = bias * nid2 * 0.5;
						printf("%4.0f", bias * 100000);
#else
						for (l = 0, bias = 0.0; l < Tpmradix; l++) {
							bias += abs(freqs[i][l] - freqs[j][l]);
						}
						bias = bias * 0.5 / numnoinsdel;
						printf("%4.0f", bias * 1000);
#endif
					} else {
						printf("%4.3s", Identif[i]);
					}
				}
				putchar('\n');
			}
		}
#if 0
		printf("\n%d\n", numspc);
		for (i = 0; i < numspc; i++) {
			printf("%s\n", Identif[i]);
			for (j = 0; j < numspc; j++) {
					for (l = 0, bias = 0.0; l < Tpmradix; l++) {
						temp = (freqs[i][l] - freqs[j][l]);
						bias += temp * temp;
					}
					bias = bias * nid2 * 0.5;
					printf(" %7.3f", bias * 100000);
			}
			putchar('\n');
		}
#endif
	} /* Align_optn */

	free_ivector(freqall);
	free_imatrix(freqs);
} /* seqfreq */


void
seqtran(seqchar, numspc, numsite, insdel, numnoinsdel)
cmatrix seqchar;
int numspc, numsite;
ivector insdel;
int numnoinsdel;
{
	int i, j, k, numradix, ii, jj;
	cvector seqchi, seqchj;
	imatrix trans;
	ivector tranall;

	trans = new_imatrix(Tpmradix + 1, Tpmradix + 1);
	tranall = new_ivector(Tpmradix + 1);
	for (i = 0; i < Tpmradix + 1; i++) {
		for (j = 0; j < Tpmradix + 1; j++) trans[i][j] = 0;
		tranall[j] = 0;
	}
	for (i = 0; i < numspc - 1; i++) {
		seqchi = seqchar[i];
		for (j = i + 1; j < numspc; j++) {
			seqchj = seqchar[j];
			for (k = 0; k < numsite; k++) {
				if (!insdel[k]) {
					trans[seqchi[k]][seqchj[k]]++;
					trans[seqchj[k]][seqchi[k]]++;
				}
			}
		}
	}

	if (Tpmradix == NUMAMI)
		numradix = NUMAMI / 2;
	else
		numradix = NUMNUC;

	printf("\n%7s", " ");
	if (Tpmradix == NUMAMI) {
		for (j = 0; j < numradix; j++) printf("%3s%4s", Cacid1[j], Cacid3[j]);
	} else {
		for (j = 0; j < numradix; j++) printf("%5s%2s", Cacid1[j], " ");
	}
	putchar('\n');
	for (i = 0; i < Tpmradix; i++) {
		if (Tpmradix == NUMAMI) {
			printf("%3s%4s", Cacid1[i], Cacid3[i]);
		} else {
			printf("%5s%2s", Cacid1[i], " ");
		}
		for (j = 0; j < numradix ; j++) {
			printf("%7d", trans[i][j]);
		}
		putchar('\n');
	}

	if (Tpmradix == NUMAMI) {
	printf("\n%7s", " ");
	if (Tpmradix == NUMAMI) {
		for (j = numradix; j < Tpmradix; j++)
			printf("%3s%4s", Cacid1[j], Cacid3[j]);
	} else {
		for (j = numradix; j < Tpmradix; j++)
			printf("%5s%2s", Cacid1[j], " ");
	}
	putchar('\n');
	for (i = 0; i < Tpmradix; i++) {
		if (Tpmradix == NUMAMI) {
			printf("%3s%4s", Cacid1[i], Cacid3[i]);
		} else {
			printf("%5s%2s", Cacid1[i], " ");
		}
		for (j = numradix; j < Tpmradix ; j++) {
			printf("%7d", trans[i][j]);
		}
		putchar('\n');
	}
	}

#if 0
	for (i = 0; i < numspc - 1; i++) {
		seqchi = seqchar[i];
		for (j = i + 1; j < numspc; j++) {
			seqchj = seqchar[j];
			for (ii = 0; ii < Tpmradix + 1; ii++) {
				for (jj = 0; jj < Tpmradix + 1; jj++) trans[ii][jj] = 0;
			}
			for (k = 0; k < numsite; k++) {
				if (!insdel[k]) {
					trans[seqchi[k]][seqchj[k]]++;
					trans[seqchj[k]][seqchi[k]]++;
				}
			}

			printf("\n%-3d %-3d  %s  %s\n", i+1, j+1, Identif[i], Identif[j]);
			for (jj = 0; jj < Tpmradix; jj++) printf("%4s", Cacid1[jj]);
			putchar('\n');
			for (ii = 0; ii < Tpmradix; ii++) {
				printf("%1s", Cacid1[ii]);
				printf("%3d", trans[ii][0]);
				for (jj = 1; jj < Tpmradix ; jj++) {
					printf("%4d", trans[ii][jj]);
				}
				putchar('\n');
			}

		}
	}
#endif

	free_ivector(tranall);
	free_imatrix(trans);
} /* seqtran */
