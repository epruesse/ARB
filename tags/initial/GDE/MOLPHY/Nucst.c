/*
 * protst.c   Adachi, J.   1996.03.12
 * Copyright (C) 1993-1996 J. Adachi & M. Hasegawa, All rights reserved.
 */

#define MAIN_MODULE 1
#include "protst.h"


void
copyright()
{
#ifndef NUC
	fprintf(stderr, "ProtST %s (%s) ", VERSION, DATE );
	fprintf(stderr, "Basic Statistics of Protein Sequences\n");
#else
	fprintf(stderr, "NucST %s (%s) ", VERSION, DATE );
	fprintf(stderr, "Basic Statistics of Nucleic Acid Sequences\n");
#endif
	fprintf(stderr, "Copyright (C) 1993-1996 J. Adachi & M. Hasegawa. ");
	fprintf(stderr, "All rights reserved.\n");
/*	fprintf(stderr, "  ProtST comes with NO WARRANTY\n"); */
}


void
usage()
{
	copyright();
	fprintf(stderr, "Usage: %s [-%s] sequence_file\n", Prog_name, SWITCHES);
	fprintf(stderr, " Help: %s -h\n", Prog_name);
}


void
helpinfo()
{
	copyright();
	fprintf(stderr,
		"Usage: %s [switches] sequence_file\n",Prog_name);
	fprintf(stderr, "Switches:\n");
	fprintf(stderr, "-a      Alignments viewer\n");
	fprintf(stderr, "-c num  column size\n");
	fprintf(stderr, "-S      Sequential input format (PHYLIP)\n");
	fprintf(stderr, "-I      Interleaved input format (other packages)\n");
}

static void prologue();
static void pml();


main(argc, argv)
int argc;
char **argv;
{
	FILE *seqfp;
	int i, ch;
	char **cpp;
	extern int Optindex;   /* index of next argument */
	extern char *Optargp;  /* pointer to argument of current option */

	Tpmradix = TPMRADIX;
	Ct0 = time(NULL);
	if((Prog_name = strrchr(argv[0], DIR_CHAR)) != NULL )
		Prog_name++;
	else
		Prog_name = argv[0];

	while((ch = mygetopt(argc, argv, SWITCHES)) != -1 ) {
		switch(ch) {
		case 'a': Align_optn  = TRUE;  break;
		case 'c': Colmn_optn = TRUE;
				cpp = &Optargp;
				if (i = strtol(Optargp, cpp, 10)) Maxcolumn = i;
				break;
		case 'i': Info_optn   = TRUE;  break;
		case 'I': Inlvd_optn  = TRUE;
		          Seque_optn  = FALSE; break;
		case 'L': Logfl_optn  = TRUE;  break;
		case 'S': Seque_optn  = TRUE;
		          Inlvd_optn  = FALSE; break;
		case 'v': Verbs_optn  = TRUE;  break;
		case 'w': Write_optn  = TRUE;  break;
		case 'z': Debug_optn  = TRUE;  break;
		case 'Z': Debug       = TRUE;  break;
		case 'h':
		case 'H': helpinfo(); exit(1);
		default : usage(); exit(1);
		}
	}
	if (Colmn_optn) {
		Linesites  = ((Maxcolumn - 10) / 11) * 10;
		Maxelement = ((Maxcolumn -  9) /  5);
	} else {
		Linesites  = LINESITES;
		Maxelement = MAXELEMENT;
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
		seqfp = stdin;
	} else if (Optindex + 1 == argc) {
		if ((seqfp = fopen(argv[Optindex++], "r")) == NULL) {
			fprintf(stderr,"%s: can't open %s\n",Prog_name,argv[--Optindex]);
			exit(1);
		}
	} else {
		fprintf(stderr, "%s: Inconsistent number of file in command line\n",
			Prog_name);
		usage();
		exit(1);
	}
	if (Logfl_optn) {
		if ((Logfp = fopen(LOGFILE, "w")) == NULL) {
			fprintf(stderr,
				"%s: can't open log file: %s\n",Prog_name,LOGFILE);
			exit(1);
		}
	}

	if (Verbs_optn) copyright();
	prologue(seqfp, stdout);
	if (seqfp != stdin) fclose(seqfp);
	if (Logfl_optn) fclose(Logfp);

	return 0;
}

/*  */
/* #if 0 */
/* void */
/* header(ofp, maxspc, numsite, commentp) */
/* FILE *ofp; */
/* int *maxspc; */
/* int *numsite; */
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
/* 	fputs("  ", ofp); */
/* 	fprintf(ofp, "  %d OTUs %d sites  %s\n", *maxspc, *numsite, *commentp); */
/* 	free_cvector(*commentp); */
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


static void
prologue(ifp, ofp)
FILE *ifp;
FILE *ofp;
{
	ivector alias;
	char *comment;

	getsize(ifp, &Maxspc, &Maxsite, &comment);

	header(ofp, &Maxspc, &Maxsite, &comment);
	Identif = (char **)malloc((unsigned)Maxspc * sizeof(char *));
	if (Identif == NULL) maerror("in prologue, Identif");
	Sciname = (char **)malloc((unsigned)Maxspc * sizeof(char *));
	if (Sciname == NULL) maerror("in prologue, Sciname");
	Engname = (char **)malloc((unsigned)Maxspc * sizeof(char *));
	if (Engname == NULL) maerror("in prologue, Engname");
	Seqchar = new_cmatrix(Maxspc, Maxsite);
	if (Seque_optn)
		getseqs(ifp, Identif, Seqchar, Maxspc, Maxsite);
	else if (Inlvd_optn)
		getseqi(ifp, Identif, Seqchar, Maxspc, Maxsite);
	else
		getseq(ifp, Identif, Sciname, Engname, Seqchar, Maxspc, Maxsite);

	getfreqepm(Seqchar, Freqemp, Maxspc, Maxsite);
	alias = new_ivector(Maxsite);
	radixsort(Seqchar, alias, Maxspc, Maxsite, &Numptrn);
	Seqconint = new_imatrix(Maxspc, Numptrn);
	Weight = new_ivector(Numptrn);
	condenceseq(Seqchar, alias, Seqconint, Weight, Maxspc, Maxsite, Numptrn);
	convseq(Seqconint, Maxspc, Numptrn);
	free_ivector(alias);

	Insdel = new_ivector(Maxsite);
	seqinsdel(Seqchar, Maxspc, Maxsite, Insdel, &Numnoindel);

	seqdiff(Seqchar, Maxspc, Maxsite, Insdel, Numnoindel);
	if (!Align_optn && Maxspc > 28) putchar('\f');

	seqfreq(Seqchar, Maxspc, Maxsite, Insdel, Numnoindel);
/*	if (!Align_optn && Maxspc > 28) putchar('\f'); */

	if (Info_optn) seqtran(Seqchar, Maxspc, Maxsite, Insdel, Numnoindel);
/*	if (!Align_optn && Info_optn && Maxspc > 14) putchar('\f'); */

	if (Align_optn) prsequence(ofp, Identif, Seqchar, Maxspc, Maxsite);
	if (Write_optn && Info_optn && (Maxspc * Maxsite > 3600)) putchar('\f');

	if (Write_optn && Info_optn)
		prcondenceseq(Identif, Seqconint, Weight, Maxspc, Maxsite, Numptrn);

	free_ivector(Insdel);
}
