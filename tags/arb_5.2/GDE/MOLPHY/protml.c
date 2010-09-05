/*
 * protml.c   Adachi, J.   1995.12.01
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa, All rights reserved.
 */

#define NJMLD  0
#define BRANCHULIMIT  0
#define PTNLKL  0
#define TPGRAPH 0
#define DISLKL  0
#define TREECHECK 0
#define FMLEN 0
#define COMPCRITERION 0
#define TPMZEROCHECK 0

#define MAIN_MODULE 1
#include "protml.h"

void
copyright()
{
#ifndef NUC
	fprintf(stderr, "ProtML %s(%s) ", VERSION, DATE );
	fprintf(stderr, "Maximum Likelihood Inference of Protein Phylogeny\n");
#else  /* NUC */
	fprintf(stderr, "NucML %s(%s) ", VERSION, DATE );
	fprintf(stderr, "Maximum Likelihood Inference of Nucleic Acid Phylogeny\n");
#endif /* NUC */
	fprintf(stderr, "Copyright (C) 1992-1996 J. Adachi & M. Hasegawa. ");
	fprintf(stderr, "All rights reserved.\n");
/*	fprintf(stderr, "  ProtML comes with NO WARRANTY\n"); */
}


void
usage()
{
	copyright();
	fprintf(stderr, "Usage: %s [switches] sequence_file [topology_file]\n",
		Prog_name);
	fprintf(stderr, " Help: %s -h\n", Prog_name);
}


void
helpinfo()
{
	copyright();
	fprintf(stderr,
		"Usage: %s [switches] sequence_file [topology_file]\n",Prog_name);
	fprintf(stderr,
		"sequence_file = MOLPHY_format | Sequential(-S) | Interleaved(-I)\n");
	fprintf(stderr,
		"topology_file = users_trees(-u) | constrained_tree(-e)\n");
	fprintf(stderr, "Model:\n");
#ifndef NUC
	fprintf(stderr, "-j  JTT (default)   -jf  JTT-F         Jones, Taylor & Thornton(1992)\n");
	fprintf(stderr, "-d  Dayhoff         -df  Dayhoff-F     Dayhoff et al.(1978)\n");
	fprintf(stderr, "-m  %-7s         -mf  %-7s-F     Adachi & Hasegawa(1995)\n", MTREVNAME, MTREVNAME);
#else  /* NUC */
	fprintf(stderr,
		"-t n1     n1: Alpha/Beta ratio    (default:%.1f)  Hasegawa, Kishino & Yano(1985)\n", ALPHABETA);
	fprintf(stderr,
		"-t n1,n2  n2: AlphaY/AlphaR ratio (default:%.1f)  Tamura & Nei(1993)\n", ALPHAYR);
#endif /* NUC */
#ifndef NUC
	fprintf(stderr, "-p  Poisson         -pf  Proportional  (-f: with data Frequencies)\n");
	fprintf(stderr, "-r  users RSR       -rf  users RSR-F   (Relative Substitution Rate)\n");
#else  /* NUC */
	fprintf(stderr, "-p  Proportional    -pf  Poisson       (-f  withOUT data Frequencies)\n");
	fprintf(stderr, "-r  users RSR-F     -rf  users RSR     (Relative Substitution Rate)\n");
#endif /* NUC */
	fprintf(stderr, "Search strategy or Mode:\n");
	fprintf(stderr, "-u  Users trees (need users_trees file)\n");
	fprintf(stderr, "-R  local Rearrangement search   -RX  LBP only\n");
	fprintf(stderr, "-e  Exhaustive search (with/without constrained_tree file)\n");
	fprintf(stderr, "-s  Star decomposition search (may not be the ML tree)\n");
	fprintf(stderr, "-q  Quick add OTUs search (may not be the ML tree)\n");
	fprintf(stderr, "-D  maximum likelihood Distance matrix --> NJDIST\n");
	fprintf(stderr, "Others:\n");
	fprintf(stderr, "-n num  retained top ranking trees (default -e:%d,-q:%d)\n", MAXALTREES, NUMQLTREES);
	fprintf(stderr, "-P num  Per cent (default -e:%.0f, if possible trees<=945 : %.0f)\n", TBLRATE*100, TBLRATE7*100);
	fprintf(stderr, "-b  no RELL-BP          -M  Minimum evolution (with -e)\n");
	fprintf(stderr, "-S  Sequential format   -I  Interleaved format\n");
	fprintf(stderr, "-v  verbose to stderr   -i, -w  output some information\n");
}

static void prologue();
void pml();


main(argc, argv)
int argc;
char **argv;
{
	FILE *seqfp, *tplfp;
	int i, ch, num;
	double x;
	char **cpp, *cp;
	char buf[64];
	boolean num_flag;
	extern int Optindex;   /* index of next argument */
	extern char *Optargp;  /* pointer to argument of current option */

	Ct0 = time(NULL);
	if((Prog_name = strrchr(argv[0], DIR_CHAR)) != NULL )
		Prog_name++;
	else
		Prog_name = argv[0];

	Rrsr_optn  = FALSE;
	Boots_optn = TRUE; /* default with User_optn*/
	Exhau_optn = FALSE;
	Const_optn = FALSE;
	User_optn  = FALSE;
	Aneal_optn = FALSE;
	Njoin_optn = FALSE;
	num_flag   = FALSE;
	Percnt_optn = FALSE;
	Maxaltree = MAXALTREES;
	Numqltree = NUMQLTREES;
#ifndef NUC
	Jtt_optn   = TRUE;
	Frequ_optn = FALSE;
#else  /* NUC */
	Jtt_optn   = FALSE;
	Frequ_optn = TRUE;
	Tstv_optn  = FALSE;
	AlphaBeta = ALPHABETA;
	AlphaYR   = ALPHAYR;
	Beta12    = BETA12;
	Topting = FALSE;
#endif /* NUC */
	Dayhf_optn = FALSE;
	Mtrev_optn = FALSE;
	Linesites = LINESITES;
	while((ch = mygetopt(argc, argv, SWITCHES)) != -1 ) {
		switch(ch) {
#ifndef NUC
		case 'j':
				Jtt_optn    = TRUE;
				Dayhf_optn  = FALSE;
				Mtrev_optn  = FALSE;
				Poisn_optn  = FALSE;
				Rrsr_optn   = FALSE;
				break;
		case 'd':
				Dayhf_optn  = TRUE;
				Jtt_optn    = FALSE;
				Mtrev_optn  = FALSE;
				Poisn_optn  = FALSE;
				Rrsr_optn   = FALSE;
				break;
		case 'm':
				Mtrev_optn  = TRUE;
				Dayhf_optn  = FALSE;
				Jtt_optn    = FALSE;
				Poisn_optn  = FALSE;
				Rrsr_optn   = FALSE;
				break;
		case 'f':
				Frequ_optn  = TRUE;
				break;
		case 'p':
				Poisn_optn  = TRUE;
				Jtt_optn    = FALSE;
				Dayhf_optn  = FALSE;
				Mtrev_optn  = FALSE;
				Rrsr_optn   = FALSE;
				break;
		case 'r':
				Rrsr_optn   = TRUE;
				Jtt_optn    = FALSE;
				Dayhf_optn  = FALSE;
				Mtrev_optn  = FALSE;
				Poisn_optn  = FALSE;
				break;
#else  /* NUC */
		case 't':
				cpp = &Optargp;
				Tstv_optn   = TRUE;
				if (x = strtod(Optargp, cpp)) {
					AlphaBeta = x;
					if (**cpp == ',') {
						Optargp = ++(*cpp);
						if (x = strtod(Optargp, cpp)) {
							AlphaYR = x;
							if (**cpp == ',') {
								Optargp = ++(*cpp);
								if (x = strtod(Optargp, cpp)) {
									Beta12 = x;
								}
							}
						}
					}
				} else {
					Toptim_optn = TRUE;
					if (cp = strchr(Optargp, ',')) {
						Optargp = ++cp;
						if (x = strtod(Optargp, cpp)) {
							AlphaYR = x;
						} else {
							Toptim_optn += 2;
							AlphaYR = 1.0;
						}
						if (strchr(cp, ',')) Toptim_optn += 4;
						Beta12 = 1.0;
					}
				}
				break;
		case 'f':
				Frequ_optn  = FALSE;
				break;
		case 'p':
				Poisn_optn  = TRUE;
				break;
		case 'r':
				Rrsr_optn   = TRUE;
				Poisn_optn  = FALSE;
				break;
#endif /* NUC */
		case 'F':
				Logdet_optn = TRUE;
				break;


		case 'a': Aneal_optn  = TRUE;
				  Njoin_optn  = FALSE;
				  Exhau_optn  = FALSE;
		          User_optn   = FALSE;
				  Stard_optn  = FALSE;
				  Quick_optn  = FALSE;
		          Boots_optn  = FALSE; break;
		case 'e': Exhau_optn  = TRUE;
				  Njoin_optn  = FALSE;
				  Aneal_optn  = FALSE;
		          User_optn   = FALSE;
				  Stard_optn  = FALSE;
				  Quick_optn  = FALSE;
		          Boots_optn  = FALSE; break;
		case 'u': User_optn   = TRUE;
				  Njoin_optn  = FALSE;
				  Aneal_optn  = FALSE;
		          Exhau_optn  = FALSE;
				  Stard_optn  = FALSE;
				  Quick_optn  = FALSE; break;
		case 'q': Quick_optn  = TRUE;
				  Njoin_optn  = FALSE;
				  Aneal_optn  = FALSE;
		          User_optn   = FALSE;
				  Stard_optn  = FALSE;
		          Exhau_optn  = FALSE; break;
		case 'Q': Quick1_optn = TRUE;
				  Njoin_optn  = FALSE;
				  Aneal_optn  = FALSE;
		          User_optn   = FALSE;
				  Stard_optn  = FALSE;
		          Exhau_optn  = FALSE; break;
		case 's': Stard_optn  = TRUE;
				  Njoin_optn  = FALSE;
				  Aneal_optn  = FALSE;
				  Quick_optn  = FALSE;
				  Quick1_optn = FALSE;
		          User_optn   = FALSE;
		          Exhau_optn  = FALSE; break;
		case 'N': Njoin_optn  = TRUE;
				  Stard_optn  = FALSE;
				  Aneal_optn  = FALSE;
				  Quick_optn  = FALSE;
				  Quick1_optn = FALSE;
		          User_optn   = FALSE;
		          Exhau_optn  = FALSE; break;

		case 'n': cpp = &Optargp;
				  num_flag    = TRUE;
				  if (i = strtol(Optargp, cpp, 10)) num = i; break;
		case 'o':
			Outgr_optn = TRUE;
			cpp = &Optargp;
			if (i = strtol(Optargp, cpp, 10)) {
				Outgroup1 = i - 1;
				printf("Outgroup1 %3d\n", Outgroup1+1);
				if (**cpp == ',') {
					Optargp = ++(*cpp);
					if (i = strtol(Optargp, cpp, 10)) {
						Outgr_optn = 2;
						Outgroup2 = i - 1;
						printf("Outgroup2 %3d\n", Outgroup2+1);
					} else {
						fprintf(stderr,"%s: bad out group option \"%s\"\n",
							Prog_name, Optargp);
						exit(1);
					}
				}
			} else {
				fprintf(stderr,"%s: bad out group option \"%s\"\n",
					Prog_name, Optargp);
				exit(1);
			}
			break;
		case 'P':
				cpp = &Optargp;
				Percnt_optn = TRUE;
				if (x = strtod(Optargp, cpp)) {
					Percent = x;
					/* printf("%8.3f\n", x); */
					if (x < 0.0) {
						fprintf(stderr,"%s: bad par cent(-P) open \"%.1f\"\n",
							Prog_name, x);
						exit(1);
					}
				} else {
					fprintf(stderr,"%s: bad par cent(-P) open \"%s\"\n",
						Prog_name, Optargp);
					exit(1);
				}
				break;
		case 'b': Boots_optn  = FALSE; break;
		case 'R': Relia_optn  = TRUE;  break;
		case 'T': Triad_optn  = TRUE;  break;
		case 'D': Distn_optn  = TRUE;  break;
		case 'A': Aprox_optn  = TRUE;  break;
		case 'l':
				Lklhd_optn  = TRUE;
				Llsfile = new_cvector(strlen(Optargp) + strlen(LLSFILEEXT) + 1);
				*Llsfile = '\0';
				if (strcat(Llsfile, Optargp) && strcat(Llsfile, LLSFILEEXT)) {
				/*	printf("Llsfile is \"%s\"\n", Llsfile); */
					break;
				}
		case 'L': Logfl_optn  = TRUE;  break;
		case 'i': Info_optn   = TRUE;  break;
		case 'M': Mevol_optn  = TRUE;  break;
		case 'v': Verbs_optn  = TRUE;  break;
		case 'V': Varia_optn  = TRUE;  break;
		case 'w': Write_optn  = TRUE;  break;
		case 'X': Xreli_optn  = TRUE;  break;
		case 'S': Seque_optn  = TRUE;
		          Inlvd_optn  = FALSE; break;
		case 'I': Inlvd_optn  = TRUE;
		          Seque_optn  = FALSE; break;
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
	if (Outgr_optn == 1) {
		printf("Outgroup1 %3d\n", Outgroup1+1);
	} else if (Outgr_optn == 2) {
		printf("Outgroup1 %3d  Outgroup2 %3d\n", Outgroup1+1, Outgroup2+1);
	} else {
	}
	if (Exhau_optn) {
		if (Optindex + 2 == argc) Const_optn  = TRUE;
	}
#if TPGRAPH
#else /* TPGRAPH */
	if (!Aneal_optn && !Exhau_optn && !User_optn && !Quick_optn &&
		!Quick1_optn && !Stard_optn && !Distn_optn && !Njoin_optn ) {
		if (Optindex == argc) {
			helpinfo(); exit(1); /* default  !? */
		} else if (Optindex + 1 == argc) {
			helpinfo(); exit(1); /* default  !? */
		} else if (Optindex + 2 == argc) {
			User_optn  = TRUE; /* default  !? */
		}
	}
#endif /* TPGRAPH */
	if (num_flag) {
		if (Quick_optn || Quick1_optn) Numqltree = num;
		if (Exhau_optn) Maxaltree = num;
	}
	*Modelname = '\0';
#ifndef NUC
	if (Poisn_optn) {
		if (!Frequ_optn) strcat(Modelname, "Poisson");
		else             strcat(Modelname, "Proportional");
	} else {
		if (Rrsr_optn) {
			strcat(Modelname, "Read RSR");
		} else if (Jtt_optn) {
			strcat(Modelname, "JTT");
		} else if (Dayhf_optn) {
			strcat(Modelname, "Dayhoff");
		} else if (Mtrev_optn) {
			strcat(Modelname, MTREVNAME);
		} else {
			strcat(Modelname, "unknown");
		}
		if (Frequ_optn) strcat(Modelname, "-F");
	}
#else  /* NUC */
	if (Poisn_optn) {
		if (!Frequ_optn) strcat(Modelname, "Poisson");
		else             strcat(Modelname, "Proportional");
		AlphaBeta = 1.0;
		AlphaYR = 1.0;
	} else if (Rrsr_optn) {
		strcat(Modelname, "Read RSR");
		if (Frequ_optn) strcat(Modelname, "-F");
	} else {
		if (Toptim_optn) {
			strcat(Modelname, "A/B:opt");
			if (Toptim_optn == 3 || Toptim_optn == 7)
				strcat(Modelname, " Ay/Ar:opt");
			if (Toptim_optn >= 4)
				strcat(Modelname, " B1/B2:opt");
		} else {
			sprintf(buf, "A/B:%.2f\0", AlphaBeta);
			strcat(Modelname, buf);
			if (AlphaYR != 1.0) {
				sprintf(buf, " Ay/Ar:%.2f", AlphaYR);
				strcat(Modelname, buf);
			}
			if (Beta12 != 1.0) {
				sprintf(buf, " B1/B2:%.2f", Beta12);
				strcat(Modelname, buf);
			}
		}
		if (Frequ_optn) strcat(Modelname, " F");
	}
#endif /* NUC */
/*	printf("\"%s\"\n", Modelname); */

#ifdef DEBUG
	if (Debug) {
		printf("argc = %d\n",argc);
		for(i = 0; i < argc; i++) printf("argv[%d] = %s\n",i,argv[i]);
		putchar('\n');
		printf("\nOptindex = %d\n",Optindex);
		printf("Optargp = %s\n",Optargp);
	}
#endif

	Aneal_mode = 0;
	if (Optindex == argc) {
		seqfp = stdin;
		tplfp = stdin;
		Aneal_mode = 0;
	} else if (Optindex + 1 == argc) {
		if ((seqfp = fopen(argv[Optindex++], "r")) == NULL) {
			fprintf(stderr,"%s: can't open %s\n",Prog_name,argv[--Optindex]);
			exit(1);
		} else {
			tplfp = seqfp;
			Aneal_mode = 0;
		}
	} else if (Optindex + 2 == argc) {
		if ((seqfp = fopen(argv[Optindex++], "r")) == NULL) {
			fprintf(stderr,"%s: can't open %s\n",Prog_name,argv[--Optindex]);
			exit(1);
		} else if ((tplfp = fopen(argv[Optindex], "r")) == NULL) {
			if (*argv[Optindex] == '-') {
				tplfp = stdin;
				Aneal_mode = 1;
			} else {
				fprintf(stderr,"%s: can't open %s\n",Prog_name,argv[Optindex]);
				exit(1);
			}
		}
	} else {
		fprintf(stderr, "%s: Inconsistent number of operand in command line!\n",
			Prog_name);
		usage();
		exit(1);
	}

	if (Rrsr_optn) {
		if ((Rtfifp = fopen(RSRINFILE, "r")) == NULL) {
			fprintf(stderr,
				"%s: can't open RSR file: %s\n",Prog_name,RSRINFILE);
			exit(1);
		}
	}
	if (Logfl_optn) {
		if ((Logfp = fopen(LOGFILE, "w")) == NULL) {
			fprintf(stderr,
				"%s: can't open log file: %s\n",Prog_name,LOGFILE);
			exit(1);
		}
	}
	if (Lklhd_optn) {
		if ((Lklfp = fopen(Llsfile, "w")) == NULL) {
			fprintf(stderr,
				"%s: can't open log likelihood file: %s\n",Prog_name,Llsfile);
			exit(1);
		}
	}

	Epsfile = new_cvector(strlen(EPSFILE) + 1);
	*Epsfile = '\0';
	strcat(Epsfile, EPSFILE);
	if ((Epsfp = fopen(Epsfile, "w")) == NULL) {
		fprintf(stderr,"%s: can't open eps tree file: %s\n",Prog_name,Epsfile);
		exit(1);
	}
	if (Relia_optn || Njoin_optn) {
		if ((Tplfp = fopen(TPLFILE, "w")) == NULL) {
			fprintf(stderr,"%s: can't open tree topology file: %s\n",
				Prog_name,TPLFILE);
			exit(1);
		}
	}
	if (User_optn || Stard_optn || Njoin_optn) {
		if ((Trefp = fopen(TREFILE, "w")) == NULL) {
			fprintf(stderr,"%s: can't open tree file: %s\n",Prog_name,TREFILE);
			exit(1);
		}
	}

	if (Verbs_optn) { fputc('\n', stderr); copyright(); }
	Numexe = 1;
	for (Cnoexe = 0; Cnoexe < Numexe; Cnoexe++) {
		prologue(seqfp, stdout);
		if (!Distn_optn) pml(tplfp, stdout);
	}
#if __STDC__ && DIFFTIME
	if (Verbs_optn) {
		Ct1 = time(NULL);
		x = difftime(Ct1, Ct0);
		fprintf(stderr, "elapse time %02d:%02d:%02d\n",
			(int)(x/3600), (int)(x/60), (int)x % 60);
	}
#endif
	if (tplfp != stdin && tplfp != seqfp) fclose(tplfp);
	if (seqfp != stdin) fclose(seqfp);
	fclose(Epsfp);
	if (Relia_optn || Njoin_optn) fclose(Tplfp);
	if (User_optn || Stard_optn || Njoin_optn) fclose(Trefp);
	if (Lklhd_optn) fclose(Lklfp);
	if (Logfl_optn) fclose(Logfp);
	if (Rrsr_optn) fclose(Rtfifp);

	return 0;
}


 void
header(ofp, maxspc, numsite, commentp)
FILE *ofp;
int *maxspc;
int *numsite;
char **commentp;
{
	char date[32];

	strftime(date, 32, "%x", localtime(&Ct0));
	fprintf(ofp, "%s %s (%s) %s", Prog_name, VERSION, date, Modelname);
	fprintf(ofp, " %d OTUs %d sites. %s\n", *maxspc, *numsite, *commentp);
} /*_ header */


void
headerd(ofp, maxspc, numsite, commentp)
FILE *ofp;
int *maxspc;
int *numsite;
char **commentp;
{

	fprintf(ofp, "%d %d sites %s %s\n",
		*maxspc, *numsite, Modelname, *commentp);
} /*_ headerd */


#ifdef NUC
#include "optimtpm.c"
#include "abratio.c"
#endif /* NUC */

#if COMPCRITERION
#include "compcrit.c"
#endif /* COMPCRITERION */


static void
prologue(ifp, ofp)
FILE *ifp;
FILE *ofp;
{
	int i, j, k;
	ivector alias;
	double maxdis;
#if TPMZEROCHECK
	int mini, minj;
	double dis, minprob;
	dmattpmty tpm;
#endif
#if 0
	dmattpmty tpm;
	dvectpmty x;
	double y;
#endif

	getsize(ifp, &Maxspc, &Maxsite, &Comment);
	Numsites = new_ivector(Maxspc);
	if (Distn_optn) {
		if (Maxspc < 2) exit(1);
	} else {
		if (Maxspc < 3) exit(1);
	}
	if ((User_optn || Aneal_optn || Stard_optn || Njoin_optn) && !Ctacit_optn)
		header(ofp, &Maxspc, &Maxsite, &Comment);
	if (Distn_optn && Maxspc > 2) headerd(ofp, &Maxspc, &Maxsite, &Comment);
	Identif = (char **)calloc((unsigned)Maxspc, sizeof(char *));
	if (Identif == NULL) maerror("in prologue, Identif");
	Sciname = (char **)calloc((unsigned)Maxspc, sizeof(char *));
	if (Sciname == NULL) maerror("in prologue, Sciname");
	Engname = (char **)calloc((unsigned)Maxspc, sizeof(char *));
	if (Engname == NULL) maerror("in prologue, Engname");
	Seqchar = new_cmatrix(Maxspc, Maxsite);
	if (Seque_optn)
		getseqs(ifp, Identif, Seqchar, Maxspc, Maxsite);
	else if (Inlvd_optn)
		getseqi(ifp, Identif, Seqchar, Maxspc, Maxsite);
	else
		getseq(ifp, Identif, Sciname, Engname, Seqchar, Maxspc, Maxsite);
	if (Debug_optn) prsequence(ofp, Identif, Seqchar, Maxspc, Maxsite);
	getfreqepm(Seqchar, Freqemp, Maxspc, Maxsite);
	alias = new_ivector(Maxsite);
	a_radixsort(Seqchar, alias, Maxspc, Maxsite, &Numptrn);
	Seqconint = new_imatrix(Maxspc, Numptrn);
	Weight = new_ivector(Numptrn);
	condenceseq(Seqchar, alias, Seqconint, Weight, Maxspc, Maxsite, Numptrn);
	convseq(Seqconint, Maxspc, Numptrn);
	getnumsites(Seqconint, Numsites, Weight, Maxspc, Numptrn);
	if (Debug_optn)
		prcondenceseq(Identif, Seqconint, Weight, Maxspc, Maxsite, Numptrn);
	free_ivector(alias);
	checkseq(Seqconint, Maxspc, Numptrn);
	if (Frequ_optn) convfreq(Freqemp);
	if (Toptim_optn) Topting = TRUE;
#ifdef NUCxxxxxxx
	if (!Toptim_optn && (AlphaBeta == ALPHABETA)) abratio();
#endif /* NUC */
	tranprobmat();
#if TPMZEROCHECK
	for (k = 1, dis = 1.0; k < 100; k++) {
		dis = dis / (double)k;
		tprobmtrx(dis, tpm);
		for (i = 1, minprob = 2.0; i < Tpmradix; i++) {
			for (j = 0; j < i; j++) {
				if (tpm[i][j] < minprob) {
					minprob = tpm[i][j];
					mini = i;
					minj = j;
				}
				if (tpm[i][j] < 0.0)
					printf("%3d %20.15f %2d %2d %25.20f\n",
						k, dis, i, j, tpm[i][j]);
			}
		}
		printf("%3d %20.15f %2d %2d %25.20f\n", k, dis, mini, minj, minprob);
	}
	exit(1);
#endif
	if (Toptim_optn) Topting = FALSE;
	if (Write_optn) prfreq();

	Distanmat = new_dmatrix(Maxspc, Maxspc);
	if (Logdet_optn)
		lddistance(Distanmat, Seqchar, Maxspc, Maxsite);
	else
		distance(Distanmat, Seqchar, Maxspc, Maxsite);
#if 1
	if (Triad_optn) tridistance(Distanmat, Seqconint, Weight, Maxspc, Numptrn);
#else
	if (Triad_optn) tridistance2(Distanmat, Seqchar, Maxspc, Maxsite);
#endif
	if (Distn_optn) putdistance(Identif, Sciname, Engname, Distanmat, Maxspc);
	Maxbrnch = 2 * Maxspc - 3;
	Maxibrnch = Maxspc - 3;
	Maxpair = (Maxspc * (Maxspc - 1)) / 2;
#if BRANCHULIMIT
	Llimit = 100.0 / Maxsite;
	for (i = 1, maxdis = Llimit; i < Maxspc; i++) {
		for (j = 0; j < i; j++) {
			if (Distanmat[i][j] > maxdis) maxdis = Distanmat[i][j];
		}
	}
	Ulimit = maxdis;
	Mlimit = (Ulimit - Llimit) * 0.5;
#else /* BRANCHULIMIT */
	Llimit = LOWERLIMIT;
	Ulimit = UPPERLIMIT;
	Mlimit = MIDLIMIT;
#endif /* BRANCHULIMIT */
#if 0
	tprobmtrx(1.0, tpm); /* 1 - 10000 */
	for (i = 0, y = 0.0; i < Tpmradix; i++) {
		for (j = 0, x[i] = 0.0; j < Tpmradix; j++) {
			x[i] += tpm[i][j];
			if (i != j) y += Freqtpm[i] * tpm[i][j];
		}
	}
	putchar('\n');
	if (Tpmradix == NUMAMI) {
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < 10; j++) {
				printf("%7d", (int)(tpm[i][j]*1.0e6));
			} putchar('\n');
		}
		for (j = 0; j < 10; j++) printf("%7.4f", x[j]); putchar('\n');
		putchar('\n');
		for (i = 0; i < Tpmradix; i++) {
			for (j = 10; j < Tpmradix; j++) {
				printf("%7d", (int)(tpm[i][j]*1.0e6));
			} putchar('\n');
		}
		for (j = 10; j < Tpmradix; j++) printf("%7.4f", x[j]); putchar('\n');
	} else {
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < Tpmradix; j++) {
				printf("%7d", (int)(tpm[i][j]*1.0e6));
			} putchar('\n');
		}
		for (j = 0; j < Tpmradix; j++) printf("%7.4f", x[j]); putchar('\n');
	}
	printf("%20.10f\n\n", y);
	exit(1);
#endif
}


#ifdef BDATE
#include "bdate.c"
#endif /* BDATE */

#if PTNLKL
#include "ptnlkl.c"
#endif /* PTNLKL */

#if TPGRAPH
#include "tpgraph.c"
#endif /* TPGRAPH */


#ifndef TPM
void
pml(ifp, ofp)
FILE *ifp;
FILE *ofp;
{
	int buftree, cnospc, i;
	Node *rp, **poolnode, **addposition;
	Infoaddtree *topaddtree, *curp, *insp;
	char *cltree;
	double lkla, nt, mt;
#if DISLKL
	int  k, l;
	double dlkl, dislkl;
	dmatrix dislklhd, iprb, jprb;
#endif /* DISLKL */

	buftree =  getbuftree(Maxspc, Identif);
	if (Debug) fprintf(ofp, "buftree: %d\n", buftree);
	Distanvec = new_dvector(Maxpair);
	changedistan(Distanmat, Distanvec, Maxspc);
	Brnlength = new_dvector(Maxbrnch);
	if (!Cnoexe) getproportion(&Proportion, Distanvec, Maxpair);
	Epsilon = EPSILON;
	Numspc = Maxspc;
	Numbrnch = Maxbrnch;
	Numpair = Maxpair;
	Numsite = Maxsite;
	Numibrnch = Numspc - 3;
	Converg = TRUE;
	Numit = 0;

#if TPGRAPH
	tpgraph();
	exit(1);
#endif /* TPGRAPH */

	if (User_optn) { /* Users trees MODE */

		if (!Cnoexe)
			Ctree = (Tree *) new_tree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		getnumtree(ifp, &Numtree);
		Infotrees = (Infotree *) newinfotrees(Numtree, buftree);
		Lklptrn = NEW_LPMATRIX(Numtree, Numptrn);
		Strtree = new_cvector(buftree);
		if (Relia_optn) {
			Relistat = new_ivector(Numibrnch);
			Reliprob = new_dmatrix(Numibrnch, 2);
			Relinum  = new_imatrix(Numibrnch, 2);
			fprintf(Tplfp, "%d\n", Numtree);
		}
		if (Numtree != 1) fprintf(Trefp, "%d\n", Numtree);
		for (Cnotree = 0; Cnotree < Numtree; Cnotree++) {
			if (!Ctacit_optn && !Aprox_optn) printf("#%d\n", Cnotree + 1);
			getusertree(ifp, Strtree, buftree);
			if (Debug) puts(Strtree);
			constructtree(Ctree, Strtree);
#if TREECHECK
			prtopology(Ctree);
			exit(1);
#endif
			if (Debug) prcurtree(Ctree); /* */
			Alklptrn = Lklptrn[Cnotree];
#ifdef NUC
			if (Toptim_optn) {
				Epsilon = REPSILON;
				optimtpm();
				tranprobmat();
				Epsilon = EPSILON;
			}
#endif /* NUC */

#if FMLEN
			fmlength(Ctree, Distanmat, Maxspc);
#else
			slslength(Ctree, Distanmat, Maxspc);
		/*	lslength(Ctree, Distanvec, Maxspc); */
#endif
			if (Debug) prcurtree(Ctree);
			if (Debug_optn) prcurtree(Ctree);
			if (Aprox_optn) {
				initpartlkl(Ctree);
				aproxlkl(Ctree);
				praproxlkl(Ctree);
				putctopology(Ctree);
			} else {
				initpartlkl(Ctree);
				if (Ctacit_optn) aproxlkl(Ctree);
				rp = (Node *)mlikelihood(Ctree);
				/* rp = (Node *)relibranch(rp); */
				mlvalue(Ctree, Infotrees);
				if (Debug_optn) putctopology(Ctree);
				if (Relia_optn) {
					Epsilon = REPSILON;
					reliabranch(Ctree);
					fputctopology(Tplfp, Ctree);
					Epsilon = EPSILON;
				}
				if (!Ctacit_optn) prtopology(Ctree);
				strctree(Ctree, Infotrees[Cnotree].ltplgy);
				if (Debug) puts(Infotrees[Cnotree].ltplgy);
				if (!Ctacit_optn) resulttree(Ctree);
				if (Cnotree == 0) pstree(Epsfp, Ctree);
				fputcphylogeny(Trefp, Ctree);
#if VARITPM
				varitpm(Ctree);
#endif /* VARITPM */
#if COMPCRITERION
				if (Ctacit_optn) compcrit(Ctree);
#endif /* COMPCRITERION */
#ifdef BDATE
				branchdate(Ctree);
#endif /* BDATE */
#if PTNLKL
				xpatternlklhd(Ctree);
			/*	patternlklhd(Ctree); */
#endif /* PTNLKL */
			}
			if (Verbs_optn) {
				fprintf(stderr," %d", Cnotree + 1);
#if __STDC__ && DIFFTIME
				Ct1 = time(NULL);
				fprintf(stderr, "(%.0fs)", difftime(Ct1, Ct0));
#endif
			}
		}
		if (Ctacit_optn && Numtree == 1) printf("%.10f\n", Ctree->lklhd);
		if (Relia_optn) {
			free_ivector(Relistat);
			free_dmatrix(Reliprob);
			free_imatrix(Relinum);
		}
		if (Lklhd_optn) outlklhd(Lklptrn);
		if (!Aprox_optn && Numtree > 1 && (!COMPCRITERION || !Ctacit_optn)) {
			if (Boots_optn && Verbs_optn) fputs("\nbootstraping\n", stderr);
			if (Boots_optn) bootstrap(Infotrees, Lklptrn);
			tabletree(Infotrees, Lklptrn);
			if (Info_optn) tableinfo(Infotrees);
		}
		if (Verbs_optn) fputs("\n", stderr);
		FREE_LPMATRIX(Lklptrn);

	} else if (Aneal_optn) { /* Annealing MODE */

		Relia_optn = TRUE;
		Ctree = (Tree *) new_tree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		if (!Aneal_mode) getnumtree(ifp, &Numtree);
		if (!Numtree) Numtree = 1; /* !? */
		Infotrees = (Infotree *) newinfotrees(Numtree, buftree);
		Lklptrn = NEW_LPMATRIX(Numtree, Numptrn);
		Strtree = new_cvector(buftree);
		if (Relia_optn) {
			Reliprob = new_dmatrix(Numibrnch, 2);
			Relinum  = new_imatrix(Numibrnch, 2);
		}
		Cnotree = 0;

		if (!Ctacit_optn && !Aprox_optn) printf("\n");
		getusertree(ifp, Strtree, buftree);
		if (Debug) puts(Strtree);
		constructtree(Ctree, Strtree);
		if (Debug) prcurtree(Ctree); /* */
		Alklptrn = Lklptrn[Cnotree];
#ifdef NUC
		if (Toptim_optn) {
			optimtpm();
			tranprobmat();
		}
#endif /* NUC */
#if FMLEN
		fmlength(Ctree, Distanmat, Maxspc);
#else
		lslength(Ctree, Distanvec, Maxspc);
#endif
		if (Debug) prcurtree(Ctree);
		if (Debug_optn) prcurtree(Ctree);
		initpartlkl(Ctree);
		rp = (Node *)mlikelihood(Ctree);
		mlvalue(Ctree, Infotrees);
		if (Debug_optn) putctopology(Ctree);

		if (Relia_optn) annealing(Ctree);

		if (!Ctacit_optn) prtopology(Ctree);
		strctree(Ctree, Infotrees[Cnotree].ltplgy);
		if (Debug) puts(Infotrees[Cnotree].ltplgy);
		if (!Ctacit_optn) resulttree(Ctree);

		if (Ctacit_optn && Numtree == 1) printf("%.10f\n", Ctree->lklhd);
		if (Verbs_optn) fputs("\n", stderr);
		if (Lklhd_optn) outlklhd(Lklptrn);
		if (Relia_optn) {
			free_dmatrix(Reliprob);
			free_imatrix(Relinum);
		}
		FREE_LPMATRIX(Lklptrn);

	} else if (Exhau_optn) { /* Exhaustive search */

		Numtree = 1;
		Cnotree = 0;
		Numspc   = Maxspc;
		Numbrnch = Maxspc;
		Maxibrnch = Maxspc - 3;
		/* Infotrees = (Infotree *) newinfotrees(1, buftree); */
		/* Alklptrn = NEW_LPVECTOR(Numptrn); */
		if (Maxspc < 4)  {
			fprintf(stderr,"%s: number of OTUs is \"%d\".\n",Prog_name,Maxspc);
			exit(1);
		}
		if (!Const_optn) {
			for (i = Maxspc * 2 - 5, nt = 1.0; i > 0; i--) nt *= i;
			for (i = Maxspc - 3, mt = 1.0; i > 0; i--) mt *= i;
			for (i = Maxspc - 3; i > 0; i--) mt *= 2;
			nt /= mt;
			Numtplgy = nt;
		} else {
			Numtplgy = 1.0;
		}

		Ctree = (Tree *) new_njtree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		enjtree(Ctree, Distanmat, Maxspc, TRUE);
		pathing(Ctree);
		Numibrnch = Maxibrnch;
		slslength(Ctree, Distanmat, Maxspc);
		Mintbldm = Ctree->tbldis;
		/* printf("TBL: %7.3f ", Mintbldm); putctopology(Ctree); */
		free_njtree(Ctree, Maxspc, Maxibrnch);

		Ctree = (Tree *) new_njtree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		enjtree(Ctree, Distanmat, Maxspc, FALSE);
		pathing(Ctree);
		Numibrnch = Maxibrnch;
		slslength(Ctree, Distanmat, Maxspc);
		Maxtbldm = Ctree->tbldis;
		/* printf("TBL: %7.3f ", Maxtbldm); putctopology(Ctree); */
		free_njtree(Ctree, Maxspc, Maxibrnch);

		Poolorder = new_ivector(Maxspc);
		Ctree = (Tree *) new_atree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		poolnode = (Node **)malloc((unsigned)Maxspc * sizeof(Node *));
		if (poolnode == NULL) maerror("poolnode.");
		addposition = (Node **)malloc((unsigned)Maxspc * sizeof(Node *));
		if (addposition == NULL) maerror("addposition.");
		if (!Const_optn) { /* without constrained tree */
			atreeinit(Ctree, poolnode, addposition, Poolorder);
		} else { /* with constrained tree */
			Strtree = new_cvector(buftree);
			getusertree(ifp, Strtree, buftree);
			if (Debug) puts(Strtree);
			streeinit(Ctree, Strtree, poolnode, addposition, Poolorder);
		}
		Infoaltrees = (Infoaltree *) newinfoaltrees(Maxaltree, buftree);

		if (Percnt_optn) {
			if (Percent > 100.0)
				Tblrate = 2.0;
			else
				Tblrate = Percent / 100.0;
		} else {
			if (Numtplgy <= 945)
				Tblrate = TBLRATE7;
			else
				Tblrate = TBLRATE;
		}
		Basetbldm = (Maxtbldm - Mintbldm) * Tblrate + Mintbldm;
		/* printf("TBL: Min%7.3f max%7.3f bese%7.3f\n",
			Mintbldm, Maxtbldm, Basetbldm); */
		if (Verbs_optn) {
			fprintf(stderr," %.0f possible trees,  TBL limit %.1f%%\n",
				Numtplgy, Tblrate*100);
			if (Maxspc > 9) Numverbs = 10000; else Numverbs = 1000;
		}
		if (Numtplgy > 655e6)  {
			fprintf(stderr,"%s: too many possible trees \"%.0f\".\n",
				Prog_name, Numtplgy);
			exit(1);
		}

		Ahead.lklaprox = 0.0;
		Atail.up = &Ahead;
		Tblbin = new_ivector(NUMTBLBIN);
		for (i = 0; i < NUMTBLBIN; i++) Tblbin[i] = 0;
		Tblunder = Tblover = 0;
		Tblcoef = NUMTBLBIN / (Maxtbldm - Mintbldm + 0.00001);
		if (addposition[3] == NULL)
			autoconstruction(Ctree, 3, poolnode, addposition, Poolorder);
		else
			wedge(Ctree, 3, poolnode, addposition, Poolorder, addposition[3]);
		/* if (Cnotree < Maxaltree) Numaltree = Cnotree; */
		if (Verbs_optn) fputc('\n', stderr);
		if (!Aprox_optn) tablealtree(Numaltree);
		free_ivector(Poolorder);
		free_ivector(Tblbin);

	} else if (Stard_optn) { /* Star Decomposition MODE */

		Numspc = Maxspc;
		Numibrnch = 0;
		Numbrnch = Maxspc;
		Cnotree = 0;
		Numtree = MAXSLBUF;
		Infotrees = (Infotree *) newinfotrees(Numtree, buftree);
		Lklptrn = NEW_LPMATRIX(Numtree, Numptrn);
		Ctree = (Tree *) new_stree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		if (Debug) prcurtree(Ctree); /* */
		Alklptrn = Lklptrn[Cnotree];
#if 0
		fmlength(Ctree, Distanmat, Maxspc);
		if (Aprox_optn) {
			initpartlkl(Ctree);
			aproxlkl(Ctree);
			praproxlkl(Ctree);
			putctopology(Ctree);
		} else {
			initpartlkl(Ctree);
			rp = (Node *)mlikelihood(Ctree);
			mlvalue(Ctree, Infotrees);
			if (Debug_optn) putctopology(Ctree);
			prtopology(Ctree);
			resulttree(Ctree);
		}
#endif

#if DISLKL
		initpartlkl(Ctree);
		dislklhd = new_dmatrix(Numspc, Numspc);
		for (i = 0; i < Numspc - 1; i++) {
			iprb = Ctree->ebrnchp[i]->kinp->iprob;
			for (j = i + 1; j < Numspc; j++) {
				jprb = Ctree->ebrnchp[j]->kinp->iprob;
				for (k = 0, dislkl = 0.0; k < Numptrn;  k++) {
					for (l = 0, dlkl = 0.0; l < Tpmradix; l++) {
					/*	dlkl += fabs(iprb[k][l] - jprb[k][l]); */
						dlkl += Freqtpm[l] * iprb[k][l] * jprb[k][l];
					}
				/*	dislkl += dlkl * Weight[k]; */
					dislkl += log(dlkl) * Weight[k];
				}
				dislkl = -dislkl;
				dislkl /= Numsite;
				dislklhd[i][j] = dislkl;
				dislklhd[j][i] = dislkl;
			}
			dislklhd[i][i] = 0.0;
		}
		dislklhd[Numspc-1][Numspc-1] = 0.0;
/*
		for (i = 0; i < Numspc; i++) {
			for (j = 0; j < Numspc; j++) {
				if (i != j) printf("%5.0f", dislklhd[i][j] * 1000.0);
				else        printf("%5.3s", Identif[i]);
			} putchar('\n');
		} putchar('\n');
*/
		printf("\n%d %d\n", Numspc, Numsite);
		for (i = 0; i < Numspc; i++) {
			printf("%s\n", Identif[i]);
			for (j = 0; j < Numspc; j++) {
				printf(" %15.12f", dislklhd[i][j]);
				if ((j + 1) % 5 == 0) putchar('\n');
			} if (j % 5 != 0) putchar('\n');
		} putchar('\n');

		free_dmatrix(dislklhd);
		exit(1);
#endif /* DISLKL */

		if (Relia_optn) { // bugfix : Reliprob/Relinum is used inside xstardecomp()
			Relistat = new_ivector(Maxibrnch); 
			Reliprob = new_dmatrix(Maxibrnch, 2);
			Relinum  = new_imatrix(Maxibrnch, 2); 
		}
                
#if 1
		xstardecomp(Ctree);
#else
		stardecomp(Ctree, Maxibrnch);
#endif
		pathing(Ctree);
		fmlength(Ctree, Distanmat, Maxspc);
		initpartlkl(Ctree);
		Alklptrn = Lklptrn[Cnotree];
		rp       = (Node *)mlikelihood(Ctree);
		mlvalue(Ctree, Infotrees);
		if (Debug_optn) putctopology(Ctree);
		putchar('\n');
		prtopology(Ctree);
		resulttree(Ctree);
		pstree(Epsfp, Ctree);
		fputcphylogeny(Trefp, Ctree);
		FREE_LPMATRIX(Lklptrn);
		if (Relia_optn) {
                    free_ivector(Relistat); Relistat = 0;
                    free_dmatrix(Reliprob); Reliprob = 0;
                    free_imatrix(Relinum); Relinum   = 0; 
		}

	} else if (Njoin_optn) { /* NJ MODE */

		Numspc = Maxspc;
		Numbrnch = Maxspc;
		Maxibrnch = Maxspc - 3;
		Numibrnch = 0;
		Cnotree = 0;
		Numtree = 1;
		Infotrees = (Infotree *) newinfotrees(Numtree, buftree);
		Lklptrn = NEW_LPMATRIX(Numtree, Numptrn);
		Alklptrn = Lklptrn[0];
		puts("NJ");

		Ctree = (Tree *) new_njtree(Maxspc, Maxibrnch, Numptrn, Seqconint);
#if NJMLD
		njmtree(Ctree, Distanmat, Numspc, TRUE);
#else
		enjtree(Ctree, Distanmat, Numspc, TRUE);
#endif
		Alklptrn = Lklptrn[Cnotree];
		initpartlkl(Ctree);
		rp = (Node *)mlikelihood(Ctree);
		mlvalue(Ctree, Infotrees);
		if (Relia_optn) {
			Relitrif = new_dvector(Maxibrnch);
			Relistat = new_ivector(Numibrnch);
			Reliprob = new_dmatrix(Numibrnch, 2);
			Relinum  = new_imatrix(Numibrnch, 2);
			qlrsearch(Ctree);
			putchar('\n');
		}
		prtopology(Ctree);
		resulttree(Ctree);
	/*
		initpartlkl(Ctree);
		rp = (Node *)mlikelihood(Ctree);
		mlvalue(Ctree, Infotrees);
		if (Debug_optn) putctopology(Ctree);
		putchar('\n');
		prtopology(Ctree);
		resulttree(Ctree);
		putchar('\n');
		putctopology(Ctree);
	*/
		pstree(Epsfp, Ctree);
		fprintf(Tplfp, "%d\n", Numtree);
		fputctopology(Tplfp, Ctree);
		fprintf(Trefp, "%d\n", Numtree);
		fputcphylogeny(Trefp, Ctree);
		FREE_LPMATRIX(Lklptrn);
		if (Relia_optn) {
			free_dvector(Relitrif);
			free_ivector(Relistat);
			free_dmatrix(Reliprob);
			free_imatrix(Relinum);
		}
	/*
		sorttree(Ctree, Ctree->rootp);
		prtopology(Ctree);
		putsortseq(Ctree);
	*/

	} else { /* quick MODE */

		Numtree = 2;
		Lklptrn = NEW_LPMATRIX(Numqltree, Numptrn);
		Ctree = (Tree *) new_tree(Maxspc, Maxibrnch, Numptrn, Seqconint);
		Strtree = new_cvector(buftree);
		Infoqltrees = (Infoqltree *) newinfoqltrees(MAXQLBUF, Maxbrnch);
		Qhead = (Infoqltree *) malloc(sizeof(Infoqltree));
		if (Qhead == NULL) maerror("Qhead in pml().");
		Qhead->lklaprox = 0.0;
		Qhead->residual = 0.0;
		for (Cnotree = 0; Cnotree < Numqltree; Cnotree++) {
			Alklptrn = Lklptrn[Cnotree];
			if (Cnotree == 0)
				initturn(Ctree);
			else
				randturn(Ctree);
			qtreeinit(Ctree);
			for (cnospc = 3; cnospc < Maxspc; cnospc++) {
				Numspc = cnospc + 1;
				Numibrnch = Numspc - 3;
				Numbrnch = Numspc + Numibrnch;
				Numpair = (Numspc * (Numspc - 1)) / 2;
				convertdistan(Ctree, cnospc+1, Distanmat, Distanvec);
				Qtail = Qhead;
				roundtree(Ctree, cnospc, Infoqltrees, Qhead, Qtail);
			}
#if 0
			pathing(Ctree);
			fmlength(Ctree, Distanmat, Maxspc);
			initpartlkl(Ctree);
			rp = (Node *)mlikelihood(Ctree);
		/*	mlvalue(Ctree, Infotrees); */
			prtopology(Ctree);
			resulttree(Ctree);
#endif
		/*	rerootq(Ctree, Numspc); */

			for (i = 0; Ctree->bturn[i] != Numspc-1 && i < Numspc; i++)
				;
			reroot(Ctree, Ctree->ebrnchp[i]->kinp);

			if (Aprox_optn) initpartlkl(Ctree);
			if (Aprox_optn) praproxlkl(Ctree);
			if (Aprox_optn) putctopology(Ctree);
			if (Cnotree == 0) {
				topaddtree = (Infoaddtree *)newinfoaddtree(buftree);
				strctree(Ctree, topaddtree->ltplgy);
				topaddtree->lklaprox = Ctree->lklhd;
				topaddtree->frequency = 1;
				cltree = (char *)malloc((unsigned)buftree * sizeof(char));
				if (cltree == NULL) maerror("cltree in protml.c");
				if (Debug) puts(topaddtree->ltplgy);
				Numaddtree = 1;
			} else {
				strctree(Ctree, cltree);
				lkla = Ctree->lklhd;
				insp = topaddtree;
				for (curp = topaddtree; curp != NULL; curp = curp->dp) {
					if (strcmp(cltree, curp->ltplgy) == 0) { /* same */
						curp->frequency++;
						break;
					}
					if (lkla < curp->lklaprox) insp = curp;
				}
				if (curp == NULL) {
					curp = (Infoaddtree *)newinfoaddtree(buftree);
					curp->dp = insp->dp;
					insp->dp = curp;
					(void)strcpy(curp->ltplgy, cltree);
					curp->lklaprox = lkla;
					curp->frequency = 1;
					Numaddtree++;
				}
			}
		}
		if (!Aprox_optn) tableaddtree(topaddtree, Numaddtree);
		FREE_LPMATRIX(Lklptrn);
	}

	free_cvector(Comment);
	free_cmatrix(Identif);
	free_cmatrix(Sciname);
	free_cmatrix(Engname);
	free_imatrix(Seqconint);
	free_ivector(Weight);
	free_cmatrix(Seqchar);
	free_dvector(Distanvec);
	free_dmatrix(Distanmat);
	free_dvector(Brnlength);
}
#endif /* TPM */
