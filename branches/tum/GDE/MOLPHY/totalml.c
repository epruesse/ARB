/*
 * totalml.c   Adachi, J.   1994.12.30
 * Copyright (C) 1994  J. Adachi & M. Hasegawa, All rights reserved.
 */

#define DEBUG 0

#define MAIN_MODULE 1
#include "totalml.h"

void
copyright()
{
	fprintf(stderr, "TotalML %s(%s) ", VERSION, DATE );
	fprintf(stderr, "Total ML Inference of Molecular Phylogeny\n");
	fprintf(stderr, "Copyright (C) 1994 J. Adachi & M. Hasegawa, ");
	fprintf(stderr, "All rights reserved.\n");
/*	fprintf(stderr, "  TotalML comes with NO WARRANTY\n"); */
}


void
usage()
{
	copyright();
	fprintf(stderr, "Usage: %s [switches] LLS_files\n", Prog_name);
	fprintf(stderr, " Help: %s -h\n", Prog_name);
}


void
helpinfo()
{
	copyright();
	fprintf(stderr, "Usage: %s [switches] LLS_files\n", Prog_name);
	fprintf(stderr, "switches:\n");
	fprintf(stderr, "-b  no Bootstrap probabilities\n");
	fprintf(stderr, "-v  verbose to stderr\n");
	fprintf(stderr, "-i, -w  output some information\n");
}


/* #if 0 */
/* void */
/* header(ofp, numseqs, allsite, commentp) */
/* FILE *ofp; */
/* int *numseqs; */
/* int *allsite; */
/* char **commentp; */
/* { */
/* 	time_t ct; */
/* 	char *datetime, *ip, *jp; */

/* 	fprintf(ofp, "%s %s  ", Prog_name, VERSION); */

/* 	ct = time(NULL); */
/* 	datetime = ctime(&ct); */
/* 	for (ip = datetime+11, jp = datetime+20; *jp != '\n'; ) *ip++ = *jp++; */
/* 	*ip = '\0'; */
/* 	fputs(datetime + 4, ofp); */
/* 	fprintf(ofp, "  %d data %d sites.\n", *numseqs, *allsite); */
/* /*	fprintf(ofp, "  %d data %d sites. %s\n", *numseqs, *allsite, *commentp); */ */
/* } /*_ header */ */
/* #endif */


 void
header(ofp, maxspc, numsite, commentp)
FILE *ofp;
int *maxspc;
int *numsite;
char **commentp;
{
	char date[32];

	strftime(date, 32, "%x", localtime(&Ct0));
	fprintf(ofp, "%s %s(%s)", Prog_name, VERSION, date);
	fprintf(ofp, " %d OTUs %d sites. %s\n", *maxspc, *numsite, *commentp);
} /*_ header */



void
getsize(ifp, ntree, nsite, commentp)
FILE *ifp;
int *ntree;
int *nsite;
char **commentp;
{
	char *cp, *np;
	char line[BUFLINE];

	if (fgets(line, BUFLINE, ifp) != NULL) {
		if (sscanf(line, "%d %d", ntree, nsite) == 2) {
			for (cp = line; isdigit(*cp) || isspace(*cp) && *cp != '\0'; cp++)
				;
			*commentp = new_cvector(strlen(cp) + 1);
			if (*cp != '\0') {
				for (np = *commentp; *cp != '\n' && *cp != '\0'; cp++)
					*np++ = *cp;
				*np = '\0';
			} else {
				**commentp = '\0';
			}
			if (Debug)
				fprintf(stdout, "%d trees, %d sites,  %s\n",
					*ntree, *nsite, *commentp);
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
getlnlklsite(ifp, alls, ntree, nsite)
FILE *ifp;
dmatrix alls;
int ntree, nsite;
{
	char line[BUFLINE];
	char **cpp, *cp, *sp;
	int i, j, nt;
	double x;

	cpp = &cp;

	for (i = 0; i < ntree; i++) {
		if (Debug) fprintf(stdout, "Read %d tree \n", i+1);
		if (fgets(line, BUFLINE, ifp) != NULL) {
			if (sscanf(line, "# %d", &nt)) {
				if (Debug_optn) fputs(line, stdout);
				if (nt != i + 1) {
					fprintf(stderr, "\nCan't read %d tree! \"%d\"\n", i+1, nt);
					exit(1);
				}
			} else {
				puts(line);
				fprintf(stderr, "\nCan't read header of %d tree! \"%d\"\n",
					i+1, nt);
				exit(1);
			}
		} else {
			fprintf(stderr, "\nCan't read %d tree!\n", i+1);
			exit(1);
		}
		j = 0;
		while (j < nsite) {
			if (fgets(line, BUFLINE, ifp) != NULL) {
				sp = line;
				while (x = strtod(sp, cpp)) {
					if (Debug) printf("%7.2f", x);
					if (Debug && (j+1) % 10 == 0) putchar('\n');
					alls[i][j] = x;
					sp = *cpp;
					j++;
				}
			} else {
				fprintf(stderr, "\nError: %d site!\n", j+1);
				exit(1);
			}
		}
		if (Debug && j % 10 != 0) putchar('\n');

	}
	return;
} /*_ getlnlklsite */


prlnlklsite(alls, ntree, nsite)
dmatrix alls;
int ntree, nsite;
{
	int i, j;

	for (i = 0; i < ntree; i++) {
		printf("#%d tree\n", i+1);
		for (j = 0; j < nsite; j++) {
			printf("%7.2f", alls[i][j]);
			if ((j+1) % 10 == 0) putchar('\n');
		}
		if (j % 10 != 0) putchar('\n');
	}
} /* prlnlklsite */


static int
getdata(ifp, cnoseq)
FILE *ifp;
int cnoseq;
{
	int ntree, nsite;

	getsize(ifp, &ntree, &nsite, &Comment);
	if (cnoseq == 0) {
		Numtree = ntree;
		Lnlklsite = new_dcube(Numseqs, 0, 0);
	} else {
		if (ntree != Numtree) {
			fprintf(stderr, "\nBad size, number of trees is %d\n", ntree);
			exit(1);
		}
	}
	if (nsite <= 0) {
		fprintf(stderr, "\nBad size, number of sites is %d\n", nsite);
		exit(1);
	}
	Alnlklsite = new_dmatrix(Numtree, nsite);
	getlnlklsite(ifp, Alnlklsite, Numtree, nsite);
	Lnlklsite[cnoseq] = Alnlklsite;
	Numsite[cnoseq] = nsite;
	if (Debug_optn) prlnlklsite(Alnlklsite, Numtree, nsite);
	return nsite;
}

void
prlls()
{
	int i, j, n, jmax;

	for (n = 0; n < Numseqs; n++) {
		printf("\nData: %d\n", n+1);
		for (i = 0; i < Numtree; i++) {
			printf("#%d tree\n", i+1);
			for (j = 0, jmax = Numsite[n]; j < jmax; j++) {
				printf("%7.2f", Lnlklsite[n][i][j]);
				if ((j+1) % 10 == 0) putchar('\n');
			} if (j % 10 != 0) putchar('\n');
		}
	}
}


void
total()
{
	int i, j, m, n, nsite, rs, nolltmax, same, allsame, mllbest;
	double lltmax, coefrand, coefboot;
	double sl1, sl2, suml1, suml2, ldiff, sdllt, nn1;
	ivector bspall, bsp, mllseq;
	imatrix bspseq;
	dvector lltall, llt, alls, mlls;
	dmatrix lltseq, alnlklsite;

	mllseq = new_ivector(Numseqs);
	bspall = new_ivector(Numtree);
	bspseq = new_imatrix(Numseqs, Numtree);
	lltall = new_dvector(Numtree);
	lltseq = new_dmatrix(Numseqs, Numtree);

	allsame = 0;
	for (i = 0; i < Numtree; i++) lltall[i] = 0.0;
	for (n = 0; n < Numseqs; n++) {
		llt = lltseq[n];
		for (i = 0; i < Numtree; i++) llt[i] = 0.0;
		nsite = Numsite[n];
		alnlklsite = Lnlklsite[n];
		for (i = 0; i < Numtree; i++) {
			for (j = 0; j < nsite; j++) {
				llt[i]    += alnlklsite[i][j];
				lltall[i] += alnlklsite[i][j];
			}
		}
		nolltmax = 0;
		lltmax = llt[0];
		same = 0;
		for (i = 1; i < Numtree; i++) {
			if (llt[i] > lltmax) {
				nolltmax = i;
				lltmax = llt[i];
				same = 0;
			} else if (llt[i] == lltmax) {
				same++;
			}
		}
		allsame += same;
		mllseq[n] = nolltmax;
	}
	nolltmax = 0;
	lltmax = lltall[0];
	same = 0;
	for (i = 1; i < Numtree; i++) {
		if (lltall[i] > lltmax) {
			nolltmax = i;
			lltmax = lltall[i];
			same = 0;
		} else if (lltall[i] == lltmax) {
			same++;
		}
	}
	allsame += same;
	mllbest = nolltmax;

	if (!Ctacit_optn) {
		printf("\n%-4s", "tree");
		for (n = 0; n < Numseqs; n++) printf("%7d ", n+1);
		printf(" %8s\n", "total");
		for (i = 0; i < Numtree; i++) {
			printf("%-4d", i+1);
			for (n = 0; n < Numseqs; n++) {
				if (i == mllseq[n])
				/*	printf("%8s", "ml"); */
					printf("%8.1f", -lltseq[n][i]);
				else
					printf("%8.1f", lltseq[n][mllseq[n]] - lltseq[n][i]);
			}
			if (i == mllbest) {
				printf(" %8.1f\n", -lltall[i]);
			} else {
				printf(" %8.1f\n", lltall[mllbest]-lltall[i]);
			}

			printf("%-4s", "");
			for (suml1 = suml2 = 0.0, n = 0; n < Numseqs; n++) {
				mlls = Lnlklsite[n][mllseq[n]];
				nsite = Numsite[n];
				nn1 = (double)(nsite / (nsite-1));
				if (i == mllseq[n]) {
					printf("%8s", "ml");
				} else {
					alls = Lnlklsite[n][i];
					for (sl1 = sl2 = 0.0, j = 0; j < nsite; j++) {
						ldiff = alls[j] - mlls[j];
						sl1 += ldiff;
						sl2 += ldiff * ldiff;
					}
					suml1 += sl1;
					suml2 += sl2;
					sl1 /= nsite;
					sdllt = sqrt( nn1 * (sl2 - sl1*sl1*nsite) );
					printf("%8.1f", sdllt);
				}
			}
			if (i == mllbest) {
				printf(" %8s\n", "ML");
			} else {
				suml1 /= Allsite;
				nn1 = (double)(Allsite / (Allsite-1));
				sdllt = sqrt( nn1 * (suml2 - suml1*suml1*Allsite) );
				printf(" %8.1f\n", sdllt);
			}

		}
		printf("%-4s", "sites");
		printf("%7d", Numsite[0]);
		for (n = 1; n < Numseqs; n++) printf("%8d", Numsite[n]);
		printf(" %8d\n", Allsite);
	}

	coefboot = 1.0 / (double)NUMBOOTS;
	allsame = 0;
	for (n = 0; n < Numseqs; n++) {
		for (i = 0; i < Numtree; i++) bspseq[n][i] = 0;
	}
	for (i = 0; i < Numtree; i++) bspall[i] = 0;
	for (m = 0; m < NUMBOOTS; m++) {
		for (i = 0; i < Numtree; i++) lltall[i] = 0.0;
		for (n = 0; n < Numseqs; n++) {
			llt = lltseq[n];
			bsp = bspseq[n];
			for (i = 0; i < Numtree; i++) llt[i] = 0.0;
			nsite = Numsite[n];
			alnlklsite = Lnlklsite[n];
			coefrand = (double)nsite / ((double)RANDOM_MAX + 1.0);
			for (j = 0; j < nsite; j++) {
				rs = (int)( coefrand * (double)rand() ); /* RANDOM */
				for (i = 0; i < Numtree; i++) {
					llt[i]    += alnlklsite[i][rs];
					lltall[i] += alnlklsite[i][rs];
				}
			}
			nolltmax = 0;
			lltmax = llt[0];
			same = 0;
			for (i = 1; i < Numtree; i++) {
				if (llt[i] > lltmax) {
					nolltmax = i;
					lltmax = llt[i];
					same = 0;
				} else if (llt[i] == lltmax) {
					same++;
				}
			}
			allsame += same;
			bsp[nolltmax]++;
		}
		nolltmax = 0;
		lltmax = lltall[0];
		same = 0;
		for (i = 1; i < Numtree; i++) {
			if (lltall[i] > lltmax) {
				nolltmax = i;
				lltmax = lltall[i];
				same = 0;
			} else if (lltall[i] == lltmax) {
				same++;
			}
		}
		allsame += same;
		bspall[nolltmax]++;
	}
	if (allsame > 0)
		printf("\nsame bootstrap likelihood occured %d times\n", allsame);

	if (Ctacit_optn) {
		for (i = 0; i < Numtree; i++) printf(" %6.4f", bspall[i]*coefboot);
		printf(" %2d\n", mllbest+1);
	} else {
		printf("\n%-4s", "tree");
		for (n = 0; n < Numseqs; n++) printf("%6d  ", n+1);
		printf(" %8s\n", "total");
		for (i = 0; i < Numtree; i++) {
			printf("%-4d", i+1);
			for (n = 0; n < Numseqs; n++) printf("%8.4f",bspseq[n][i]*coefboot);
			printf(" %8.4f\n", bspall[i]*coefboot);
		}
#if 0
		printf("%-4s", "sites");
		printf("%8d", Numsite[0]);
		for (n = 1; n < Numseqs; n++) printf("%8d", Numsite[n]);
		printf(" %6d\n", Allsite);
#endif
	}

	free_dmatrix(lltseq);
	free_dvector(lltall);
	free_imatrix(bspseq);
	free_ivector(bspall);
	free_ivector(mllseq);
}


main(argc, argv)
int argc;
char **argv;
{
	int i, ch;
	int cnoseq, nsite;
	extern int Optindex;   /* index of next argument */
	extern char *Optargp;  /* pointer to argument of current option */

	Ct0 = time(NULL);
	if((Prog_name = strrchr(argv[0], DIR_CHAR)) != NULL )
		Prog_name++;
	else
		Prog_name = argv[0];

	Boots_optn = TRUE; /* default with User_optn*/

	while((ch = mygetopt(argc, argv, SWITCHES)) != -1 ) {
		switch(ch) {
		case 'b': Boots_optn  = FALSE; break;
		case 'i': Info_optn   = TRUE;  break;
		case 'v': Verbs_optn  = TRUE;  break;
		case 'w': Write_optn  = TRUE;  break;
#if 1
		case 'z': Debug_optn  = TRUE;  break;
		case 'Z': Debug       = TRUE;  break;
#endif
		case 'C': Ctacit_optn = TRUE;  break;
		case 'h':
		case 'H': helpinfo(); exit(1);
		default : usage(); exit(1);
		}
	}

#ifdef DEBUG
	if (Debug) {
		printf("argc    = %d\n",argc);
		for(i = 0; i < argc; i++) printf("argv[%d] = %s\n",i,argv[i]);
		putchar('\n');
		printf("Optindex = %d\n",Optindex);
		printf("Optargp  = %s\n",Optargp);
	}
#endif

	if (Optindex == argc) {
		usage();
		exit(1);
	}
	Numseqs = argc - Optindex;
	if (Debug) printf("Numseqs: %d\n", Numseqs);
	Numsite = new_ivector(Numseqs);

	if (Verbs_optn) copyright();
	cnoseq = 0;
	Allsite = 0;
	while ((llsfp = fopen(argv[Optindex++], "r")) != NULL) {
		nsite = getdata(llsfp, cnoseq);
		cnoseq++;
		Allsite += nsite;
	}
	if (!Ctacit_optn) header(stdout, &Numseqs, &Allsite, &Comment);
	if (--Optindex != argc) {
		fprintf(stderr,"%s: can't open %s\n",Prog_name,argv[Optindex]);
		exit(1);
	}
	if (Debug) prlls();
	total();

	return 0;
}
