/*
 * tridist.c   Adachi, J.   1995.02.11
 * Copyright (C) 1993-1995 J. Adachi & M. Hasegawa, All rights reserved.
 */

#define MAIN_MODULE 1
#include "tridist.h"


void
copyright()
{

#ifdef SD
	fprintf(stderr, "SDdist %s (%s) ", VERSION, DATE );
	fprintf(stderr, "Star Division Phylogeny from Distance Matrix\n");
#else  /* SD */
#ifdef NJ
	fprintf(stderr, "NJdist %s (%s) ", VERSION, DATE );
	fprintf(stderr, "Neighbor Joining Phylogeny from Distance Matrix\n");
#else  /* NJ */
	fprintf(stderr, "Tridist %s (%s) ", VERSION, DATE );
	fprintf(stderr, "Triad Distance Phylogeny from Distance Matrix\n");
#endif /* NJ */
#endif /* SD */

	fprintf(stderr, "Copyright (C) 1993-1995 J. Adachi & M. Hasegawa. ");
	fprintf(stderr, "All rights reserved.\n");

#ifdef NJ
	fprintf(stderr, "Ref: N. Saitou & M. Nei 1987.");
	fprintf(stderr, " Molecular Biology and Evolution 4:406-425\n");
#endif /* NJ */
}


void
usage()
{
	copyright();
	fprintf(stderr, "Usage: %s [-%s] distance_matrix_file    -h : help\n",
		Prog_name, SWITCHES);
	fprintf(stderr, "  or : protml -D sequence_file | %s [-%s]\n",
		Prog_name, SWITCHES);
}


void
helpinfo()
{
	copyright();
	fprintf(stderr,
		"Usage: %s [switches] distance_matrix_file\n",Prog_name);
	fprintf(stderr, "Switches:\n");
	fprintf(stderr, "-u      UPGMA\n");
	fprintf(stderr, "-w      branch length\n");
	fprintf(stderr, "-l      Least squares\n");
	fprintf(stderr, "-S      Sequential input format (PHYLIP)\n");
	fprintf(stderr, "-o num  branch number of Out group \n");
	fprintf(stderr, "-T str  output Tree file name\n");
	fprintf(stderr, "-t str  output Topology file name\n");
}

static void prologue();
static void distmethod();


main(argc, argv)
int argc;
char **argv;
{
	FILE *distfp, *treefp, *tplgyfp;
	int i, ch;
	char **cpp;
	char *treefname, *tplgyfname;
	extern int Optindex;   /* index of next argument */
	extern char *Optargp;  /* pointer to argument of current option */

	Ct0 = time(NULL);
	if((Prog_name = strrchr(argv[0], DIR_CHAR)) != NULL )
		Prog_name++;
	else
		Prog_name = argv[0];
	Tplgy_optn = FALSE;

	while((ch = mygetopt(argc, argv, SWITCHES)) != -1 ) {
		switch(ch) {
		case 'u': Upgma_optn = TRUE;  break;
		case 'l': Least_optn = TRUE;  break;
		case 'i': Info_optn  = TRUE;  break;
		case 'm': cpp = &Optargp;
				Multi_optn = TRUE;
				if (i = strtol(Optargp, cpp, 10)) Numexe = i;
				break;
		case 'v': Verbs_optn = TRUE;  break;
		case 'w': Write_optn = TRUE;  break;
		case 'S': Seque_optn = TRUE;  break;
		case 'o': Outgr_optn = TRUE;
				cpp = &Optargp;
				if (i = strtol(Optargp, cpp, 10)) Outgroup = i-1;
				break;
		case 'T': Tfile_optn = TRUE;
				treefname = new_cvector(strlen(Optargp)+strlen(TREEFEXT)+1);
				*treefname = '\0';
				strcat(treefname, Optargp);
				strcat(treefname, TREEFEXT);
				break;
		case 't': Tplgy_optn = TRUE;
				tplgyfname = new_cvector(strlen(Optargp)+strlen(TPLGYFEXT)+1);
				*tplgyfname = '\0';
				strcat(tplgyfname, Optargp);
				strcat(tplgyfname, TPLGYFEXT);
				break;
		case 'z': Debug_optn = TRUE;  break;
		case 'Z': Debug      = TRUE;  break;
		case 'h':
		case 'H': helpinfo(); exit(1);
		default : usage(); exit(1);
		}
	}

	if (!Tplgy_optn) {
		Tplgy_optn =  TRUE;
		tplgyfname = new_cvector(strlen(TPLGYFILE)+1);
		*tplgyfname = '\0';
		strcat(tplgyfname, TPLGYFILE);
	}

#ifdef DEBUG
	if (Debug) {
		printf("argc = %d\n",argc);
		for(i = 0; i < argc; i++) printf("argv[%d] = %s\n",i,argv[i]);
		putchar('\n');
		printf("\nOptindex = %d\n",Optindex);
		printf("Optargp = %s\n",Optargp);
	}
#endif
	if (Optindex == argc) {
		distfp = stdin;
	} else if (Optindex + 1 == argc) {
		if ((distfp = fopen(argv[Optindex++], "r")) == NULL) {
			fprintf(stderr,"%s: can't open %s\n",Prog_name,argv[--Optindex]);
			exit(1);
		}
	} else {
		fprintf(stderr, "%s: Inconsistent number of file in command line\n",
			Prog_name);
		usage();
		exit(1);
	}

	Cnoexe = 0;
	if (Verbs_optn) copyright();
	if (!Multi_optn) Numexe = 1;
	for (Cnoexe = 0; Cnoexe < Numexe; Cnoexe++) {
		prologue(distfp, stdout);
		distmethod(distfp, stdout);
	}
	if (distfp != stdin) fclose(distfp);

	if (Tfile_optn) {
		if ((treefp = fopen(treefname, "w")) == NULL) {
			fprintf(stderr,"%s: can't open tree file: %s\n",
				Prog_name, treefname);
			exit(1);
		} else {
			fputcphylogeny(treefp, Ctree);
			fclose(treefp);
		}
	}
	if (Tplgy_optn) {
		if ((tplgyfp = fopen(tplgyfname, "w")) == NULL) {
			fprintf(stderr,"%s: can't open topology file: %s\n",
				Prog_name, tplgyfname);
			exit(1);
		} else {
			fputs("1 ", tplgyfp);
			header(tplgyfp, &Numspc, &Comment);
			fputctopology(tplgyfp, Ctree);
			fclose(tplgyfp);
		}
	}

	Relia_optn = FALSE;
	Epsfile = new_cvector((unsigned)strlen(EPSFILE) + 1);
	*Epsfile = '\0';
	strcat(Epsfile, EPSFILE);
	if (1) {
		if ((Epsfp = fopen(Epsfile, "w")) == NULL) {
			fprintf(stderr,
				"%s: can't open eps tree file: %s\n",Prog_name,Epsfile);
			exit(1);
		} else {
			pstree(Epsfp, Ctree);
			fclose(Epsfp);
		}
	}

	free_cmatrix(Identif);
	free_cmatrix(Sciname);
	free_cmatrix(Engname);
	free_cvector(Comment);
	return 0;
}


static void
prologue(ifp, ofp)
FILE *ifp;
FILE *ofp;
{

	getsize(ifp, &Numspc, &Comment);
	if (!Multi_optn) header(ofp, &Numspc, &Comment);
	Identif = (char **)malloc((unsigned)Numspc * sizeof(char *));
	if (Identif == NULL) maerror("in prologue, Identif");
	Sciname = (char **)malloc((unsigned)Numspc * sizeof(char *));
	if (Sciname == NULL) maerror("in prologue, Sciname");
	Engname = (char **)malloc((unsigned)Numspc * sizeof(char *));
	if (Engname == NULL) maerror("in prologue, Engname");
	Distanmat = new_dmatrix(Numspc, Numspc);
	if (Seque_optn)
		getdatas(ifp, Identif, Distanmat, Numspc);
	else
		getdata(ifp, Identif, Sciname, Engname, Distanmat, Numspc);
	Maxbrnch = 2 * Numspc - 3;
	Numpair = (Numspc * (Numspc - 1)) / 2;
}


static void
distmethod(ifp, ofp)
FILE *ifp;
FILE *ofp;
{
	int n;

	if (Write_optn && Info_optn) prdistanmat(Distanmat, Numspc);
	if (Least_optn) Distanvec = new_dvector(Numpair);
	if (Least_optn) changedistan(Distanmat, Distanvec, Numspc);
	if (!Cnoexe) Ctree = (Tree *)newtree(Maxbrnch);
	if (Cnoexe) {
		for (n = 0; n < Maxbrnch; n++) {
			Ctree->brnchp[n]->length = 0.0;
			Ctree->brnchp[n]->kinp->length = 0.0;
		}
	}
	getproportion(&Proportion, Distanmat, Numspc);
	distantree(Ctree, Distanmat, Numspc);
	free_dmatrix(Distanmat);
	if (!Multi_optn) prtopology(Ctree);
	if (Least_optn) pathing(Ctree);
	if (Least_optn) Lengths = new_dvector(Numbrnch);
	if (Least_optn) lslength(Ctree, Distanvec, Lengths);
	if (!Multi_optn && Write_optn) resulttree(Ctree);
	if (!Multi_optn && Debug_optn) putchar('\n');
	if (Debug_optn) fputctopology(stdout, Ctree);
	if (Least_optn) copylengths(Ctree, Lengths, Numbrnch);
	if (Debug_optn) putchar('\n');
	if (Debug_optn) fputcphylogeny(stdout, Ctree);
	if (Least_optn) free_dvector(Lengths);
	if (Least_optn) free_dvector(Distanvec);
} /* distmethod */
