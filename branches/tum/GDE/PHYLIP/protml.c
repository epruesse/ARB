/* Output from p2c, the Pascal-to-C translator */
/* From input file "protml.pas" */


#include "p2c.h"
/* p2c: protml.pas, line 2: 
 * Note: Unexpected name "seqfile" in program header [262] */
/* p2c: protml.pas, line 2: 
 * Note: Unexpected name "lklfile" in program header [262] */
/* p2c: protml.pas, line 2: 
 * Note: Unexpected name "tpmfile" in program header [262] */


/* Maximum Likelihood Inference of Protein Phylogeny */
/* (seqfile, output); */

/*            PROTML  Ver 1.01    January 6, 1993          */
/*               J.Adachi  and  M.Hasegawa                 */
/*        The Institute of Statistical Mathematics         */
/*     4-6-7 Minami-Azabu, Minato-ku, Tokyo 106, Japan     */

#define maxsp           20   /* maximum number of species                 */
#define maxnode         37   /* maxsp * 2 - 3                             */
#define maxpair         190   /* maxsp * (maxsp-1) / 2                     */
#define maxsite         500   /* maximum number of sites                   */
#define maxptrn         500   /* maximum number of different site patterns */
#define maxtree         15   /* maximum number of user trees              */
#define maxsmooth       30   /* number of smoothing algorithm             */
#define maxiterat       10   /* number of iterates of Newton method       */

#define epsilon         1.0e-5
    /* stopping value of error                   */
#define minarc          1.0e-3
    /* lower limit on number of subsutitutions   */
#define maxarc          3.0e+2
    /* upper limit on number of subsutitutions   */

#define prprtn          1   /* proportion of branch length               */
#define maxboot         10000
    /* number of bootstrap resamplings           */
#define maxexe          1   /* number of jobs                            */
#define maxline         60   /* length of sequence output per line        */
#define maxname         10   /* max. number of characters in species name */
#define maxami          20   /* number of amino acids                     */

#define minreal         1.0e-55
/* if job is in underflow error,
                        then increase this value                  */

#define seqfname        "seqfile"   /* input file of sequences data    */
#define tpmfname        "default_not_use"
    /* input file of trans probability */
#define lklfname        "default_not_use"
    /* output file of ln likelihood    */


/* maxnode   = maxsp*2 -3;            */
/* maxpair   = maxsp*(maxsp-1) DIV 2; */

typedef enum {
  ami
} nacidty;
typedef enum {
  AA, RR, NN, DD, CC, QQ, EE, GG, HH, II, LL, KK, MM, FF, PP_, SS, TT, WW, YY,
  VV
} amity;
typedef double aryamity[(long)VV - (long)AA + 1];
typedef long iaryamity[(long)VV - (long)AA + 1];
typedef aryamity daryamity[(long)VV - (long)AA + 1];
typedef daryamity taryamity[(long)VV - (long)AA + 1];
typedef long niaryamity[maxami];
typedef double naryamity[maxami];
typedef naryamity ndaryamity[maxami];
typedef aryamity probamity[maxptrn];
typedef Char charseqty[maxsp + 1][maxsite];
typedef Char namety[maxname];
typedef struct node *arynodepty[maxnode + 1];
typedef double lengthty[maxnode];
typedef long ilengthty[maxnode];
typedef lengthty dlengthty[maxnode];
typedef long sitety[maxsite];
typedef long itreety[maxtree + 1];
typedef double rtreety[maxtree + 1];
typedef double lptrnty[maxptrn];
typedef lptrnty ltrptty[maxtree + 1];
typedef long spity[maxsp];
typedef double rpairty[maxpair];
typedef double rnodety[maxnode];
typedef rnodety dpanoty[maxpair];
typedef rpairty dnopaty[maxnode];
typedef rnodety dnonoty[maxnode];
typedef boolean umbty[maxsp + 1];
typedef long lspty[maxsp + 1];
typedef long longintty[6];

typedef struct tree {
  /* tree topology data */
  arynodepty brnchp;   /* point to node                */
  double lklhd;   /* ln likelihood of this tree   */
  double vrilkl;   /* variance of likelihood       */
  lengthty vrilnga;   /* variance of length(branch)   */
  lengthty vrilnga2;   /* variance2 of length(branch)  */
  lengthty blklhd;   /* ln likelihood(branch)        */
  struct node *startp;   /* point to a basal node        */
  double aic;   /* Akaike Information Criterion */
  /* convergence of branch length */
  boolean cnvrgnc;
} tree;

typedef struct node {
  /* a node of tree */
  struct node *isop;   /* pointer to next subnode           */
  struct node *kinp;   /* pointer to an ascendant node      */
  long diverg;   /* number of divergences             */
  long number;   /* node number                       */
  namety namesp;   /* species name. if this node is tip */
  spity descen;   /* number of descenant nodes         */
  nacidty nadata;
  union {
    struct {
      probamity prba;   /* a partial likelihood */
      double lnga;   /* branch length        */
    } U0;
  } UU;
} node;


Static FILE *seqfile;   /* data file of amino acid sequences    */
Static FILE *lklfile;   /* output file of ln likelihood of site */
/* transition probability matrix data   */
Static FILE *tpmfile;
Static long numsp;   /* number of species                   */
Static long ibrnch1;   /* first numbering of internal branch  */
Static long ibrnch2;   /* last numbering of internal branch   */
Static long numpair;   /* number of species pairs             */
Static long endsite;   /* number of sites                     */
Static long endptrn;   /* max number of site patterns         */
Static long numnw;   /* curent numbering of Newton method   */
Static long numsm;   /* curent numbering of smoothing       */
Static long numexe;   /* curent numbering of executes        */
Static long numtrees;   /* total number of tree topologies     */
Static long notree;   /* current numbering of tree           */
Static long maxltr;   /* numbering of max ln likelihood tree */
Static long minatr;   /* numbering of min AIC tree           */
/* numbering of decomposable stage     */
Static long stage;
Static double maxlkl;   /* max ln likelihood of trees */
/* min AIC of trees           */
Static double minaic;
/* current character of seqfile */
Static Char chs;
Static boolean normal;   /* break out error                   */
Static boolean usertree;   /* U option  designate user trees    */
Static boolean semiaoptn;   /* S option  semi-auto decomposition */
Static boolean bootsoptn;   /* B option  bootstrap probability   */
Static boolean writeoptn;   /* W option  print output data       */
Static boolean debugoptn;   /* D option  print debug data        */
Static boolean putlkoptn;   /* P option */
Static boolean firstoptn;   /* F option */
Static boolean lastoptn;   /* L option */
Static boolean readtoptn;   /* R option */
/*          */
Static boolean smoothed;

Static tree ctree;   /* current tree                         */
Static aryamity freqa;   /* amino acid frequency(acid type)      */
Static charseqty chsequen;   /* acid sequence data(species,site)     */
Static sitety weight;   /* total number of a new site(new site) */
Static sitety alias;   /* number of old site(new site)         */
Static sitety weightw;   /* work area of weight                  */
Static aryamity freqdyhf, eval, eval2;
Static daryamity ev, iev;
Static taryamity coefp;
Static node *freenode;
Static itreety paratree;   /* number of parameters         */
Static rtreety lklhdtree;   /* ln likelihood                */
Static rtreety lklboottree;   /* ln likelihood (bootstrap)    */
Static rtreety aictree;   /* Akaike Information Criterion */
Static rtreety boottree;   /* bootstrap probability        */
Static ltrptty lklhdtrpt;   /* ln likelihood                */


/*** print line ***/
Static Void printline(num)
long num;
{
  /* print '-' line to standard output */
  long i;

  putchar(' ');
  for (i = 1; i <= num; i++)
    putchar('-');
  putchar('\n');
}  /* printline */


/*******************************************************/
/*****    READ DATA, INITIALIZATION AND SET UP     *****/
/*******************************************************/

/*** GET NUMBERS OF SPECIES AND SITES ***/
Static Void getnums()
{
  /* get species-number,sites-number from file*/
  /* input number of species, number of sites */
  fscanf(seqfile, "%ld%ld", &numsp, &endsite);
  if (usertree) {
    ibrnch1 = numsp + 1;
    ibrnch2 = numsp * 2 - 3;
  } else {
    ibrnch1 = numsp + 1;
    ibrnch2 = numsp;
  }
  /* numpair  := numsp*(numsp-1) DIV 2; */
  numpair = (long)((numsp * numsp - numsp) / 2.0);
  if (numsp > maxsp)
    printf("TOO MANY SPECIES: adjust CONSTants\n");
  if (endsite > maxsite)
    printf("TOO MANY SITES:   adjust CONSTants\n");
  normal = (numsp <= maxsp && endsite <= maxsite);
}  /* getnums */


Local boolean letter(chru)
Char chru;
{
  /* tests whether chru is a letter, (ASCII and EBCDIC) */
  return (chru >= 'A' && chru <= 'I' || chru >= 'J' && chru <= 'R' ||
	  chru >= 'S' && chru <= 'Z' || chru >= 'a' && chru <= 'i' ||
	  chru >= 'j' && chru <= 'r' || chru >= 's' && chru <= 'z');
}  /* letter */


/*** CONVERT CHARACTER TO UPPER CASE ***/
Static Void uppercase(chru)
Char *chru;
{
  /* convert character to upper case */
  /* convert chru to upper case -- either ASCII or EBCDIC */
  if (letter(*chru) &&
      (strcmp("a", "A") > 0 && *chru >= 'a' ||
       strcmp("a", "A") < 0 && *chru < 'A'))
    *chru = _toupper(*chru);
}  /* uppercase */


/*** GET OPERATIONAL OPTIONS ***/
Static Void getoptions()
{
  /* get option from sequences file */
  Char chrop;

  usertree = false;
  semiaoptn = false;
  firstoptn = false;
  lastoptn = false;
  putlkoptn = false;
  bootsoptn = false;
  writeoptn = false;
  debugoptn = false;
  readtoptn = false;
  while (!P_eoln(seqfile)) {
    chrop = getc(seqfile);
    if (chrop == '\n')
      chrop = ' ';
    uppercase(&chrop);
    if (chrop != 'U' && chrop != 'S' && chrop != 'F' && chrop != 'L' &&
	chrop != 'B' && chrop != 'P' && chrop != 'W' && chrop != 'D' &&
	chrop != 'R') {
      if (chrop != ' ') {
	printf(" BAD OPTION CHARACTER: %c\n", chrop);
	normal = false;
      }
      continue;
    }
    switch (chrop) {

    case 'U':
      usertree = true;
      break;

    case 'S':
      semiaoptn = true;
      break;

    case 'F':
      firstoptn = true;
      break;

    case 'L':
      lastoptn = true;
      break;

    case 'P':
      putlkoptn = true;
      break;

    case 'B':
      bootsoptn = true;
      break;

    case 'W':
      writeoptn = true;
      break;

    case 'D':
      debugoptn = true;
      break;

    case 'R':
      readtoptn = true;
      break;
    }
  }
  if (!(numexe == 1 && writeoptn))
    return;
  printf("\n%15s", "Users Option");
  printf("%14s%5s", "Usertree  : ", usertree ? "TRUE" : "FALSE");
  printf("%14s%5s", "Semiaoptn : ", semiaoptn ? "TRUE" : "FALSE");
  printf("%14s%5s\n", "Bootsoptn : ", bootsoptn ? "TRUE" : "FALSE");
  printf("%15c", ' ');
  printf("%14s%5s", "Putlkoptn : ", putlkoptn ? "TRUE" : "FALSE");
  printf("%14s%5s", "Writeoptn : ", writeoptn ? "TRUE" : "FALSE");
  printf("%14s%5s\n", "Debugoptn : ", debugoptn ? "TRUE" : "FALSE");
  printf("%15c", ' ');
  printf("%14s%5s", "Firstoptn : ", firstoptn ? "TRUE" : "FALSE");
  printf("%14s%5s\n\n", "Lastoptn  : ", lastoptn ? "TRUE" : "FALSE");
}  /* getoptions */


/*** GET AMINO ACID FREQENCY FROM DATASET ***/
Static Void readbasefreqs()
{
  /* get freqency from sequences file */
  amity ba;

  fscanf(seqfile, "%*[^\n]");
  getc(seqfile);
  for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    fscanf(seqfile, "%lg", &freqa[(long)ba - (long)AA]);
}  /* readbasefreqs */


/*** SETUP DATASTRUCTURE OF TREE ***/
Static Void setuptree(tr)
tree *tr;
{
  /* setup datastructure of tree */
  long i, k;
  node *p;
  long FORLIM, FORLIM1;

  FORLIM = numsp;
  for (i = 1; i <= FORLIM; i++) {
    p = (node *)Malloc(sizeof(node));
    p->number = i;
    p->UU.U0.lnga = 0.0;
    p->isop = NULL;
    p->diverg = 0;
    FORLIM1 = numsp;
    for (k = 0; k < FORLIM1; k++)
      p->descen[k] = 0;
    p->descen[i - 1] = 1;
    tr->brnchp[i] = p;
    p->nadata = ami;
  }
  tr->lklhd = -999999.0;
  tr->aic = 0.0;
  tr->startp = tr->brnchp[1];
  tr->cnvrgnc = false;
}  /* setuptree */


/*** GET SEQUENCES AND PRINTOUT SEQUENCES ***/
Static Void getsequence()
{
  /* get and print sequences from file */
  long is, js, ks, ls, ms, ns, ichr, numchr, maxchr;
  Char chrs;
  Char chrcnt[30];
  long ichrcnt[30];
  boolean namechar, existenced;
  long FORLIM, FORLIM1;

  fscanf(seqfile, "%*[^\n]");
  getc(seqfile);
  if (debugoptn)
    putchar('\n');
  FORLIM = numsp;
  for (is = 1; is <= FORLIM; is++) {
    /* readln(seqfile); */
    do {
      if (P_eoln(seqfile)) {
	fscanf(seqfile, "%*[^\n]");
	getc(seqfile);
      }
      chrs = getc(seqfile);
      if (chrs == '\n')
	chrs = ' ';
      if (debugoptn) {
	if (chrs == ' ')
	  putchar(chrs);
      }
    } while (chrs == ' ');
    if (debugoptn) {
      printf("<SPACE>\n");
      putchar(chrs);
    }
    namechar = true;
    ns = 0;
    do {
      ns++;
      if (ns <= maxname)
	ctree.brnchp[is]->namesp[ns - 1] = chrs;
      if (P_eoln(seqfile))
	namechar = false;
      else {
	chrs = getc(seqfile);
	if (chrs == '\n')
	  chrs = ' ';
	if (chrs == ' ')
	  namechar = false;
      }
      if (debugoptn) {
	if (namechar)
	  putchar(chrs);
      }
    } while (namechar);
    if (ns < maxname) {
      for (js = ns; js < maxname; js++)
	ctree.brnchp[is]->namesp[js] = ' ';
    }
    /*  FOR js := 1 TO maxname DO
        BEGIN
           read(seqfile, chrs);
           ctree.brnchp[is]^.namesp[js] := chrs;
        END;  */
    if (debugoptn) {
      printf("<NAME>\n");
      putchar(chrs);
    }
    FORLIM1 = endsite;
    for (js = 1; js <= FORLIM1; js++) {
      if (normal) {
	do {
	  if (P_eoln(seqfile)) {
	    fscanf(seqfile, "%*[^\n]");
	    getc(seqfile);
	  }
	  chrs = getc(seqfile);
	  if (chrs == '\n')
	    chrs = ' ';
	} while (chrs == ' ' || chrs >= '0' && chrs <= '9');
	uppercase(&chrs);
	if (debugoptn)
	  putchar(chrs);
	if (chrs != 'A' && chrs != 'C' && chrs != 'D' && chrs != 'E' &&
	    chrs != 'F' && chrs != 'G' && chrs != 'H' && chrs != 'I' &&
	    chrs != 'K' && chrs != 'L' && chrs != 'M' && chrs != 'N' &&
	    chrs != 'P' && chrs != 'Q' && chrs != 'R' && chrs != 'S' &&
	    chrs != 'T' && chrs != 'V' && chrs != 'W' && chrs != 'Y' &&
	    chrs != 'B' && chrs != 'Z' && chrs != 'V' && chrs != 'X' &&
	    chrs != '*' && chrs != '-') {
	  printf(
	    "\n WARNING -- BAD AMINO ACID \"%c\" AT POSITION%5ld OF SPECIES%3ld\n",
	    chrs, js, is);
	  normal = false;
	}
	chsequen[is][js - 1] = chrs;
      }
    }
    if (debugoptn)
      putchar('\n');
  }
  fscanf(seqfile, "%*[^\n]");
  getc(seqfile);

  FORLIM = endsite;
  for (ks = 0; ks < FORLIM; ks++) {
    ichr = 0;
    FORLIM1 = numsp;
    for (js = 0; js < FORLIM1; js++)
      ichrcnt[js] = 0;
    FORLIM1 = numsp;
    for (js = 0; js < FORLIM1; js++)
      chrcnt[js] = ' ';
    FORLIM1 = numsp;
    for (js = 1; js <= FORLIM1; js++) {
      chrs = chsequen[js][ks];
      existenced = false;
      for (ms = 0; ms < ichr; ms++) {
	if (chrs == chrcnt[ms]) {
	  ichrcnt[ms]++;
	  existenced = true;
	}
      }
      if (!existenced) {
	ichr++;
	chrcnt[ichr - 1] = chrs;
	ichrcnt[ichr - 1] = 1;
      }
    }
    maxchr = 0;
    numchr = 0;
    for (ms = 1; ms <= ichr; ms++) {
      if (ichrcnt[ms - 1] > maxchr) {
	maxchr = ichrcnt[ms - 1];
	numchr = ms;
      }
    }
    if (numchr != 0)
      chsequen[0][ks] = chrcnt[numchr - 1];
    else
      chsequen[0][ks] = '?';
  }

  if (numexe != 1)
    return;
  printf("\n%15s", "Sequences Data");
  printf("%5ld%9s%5ld%6s\n\n", numsp, "Species,", endsite, "Sites");
  printf(" Species%5cAmino Acid Sequences\n", ' ');
  printf(" -------%5c--------------------\n\n", ' ');
  FORLIM = (endsite - 1) / maxline + 1;
  for (is = 1; is <= FORLIM; is++) {
    FORLIM1 = numsp;
    for (js = 0; js <= FORLIM1; js++) {
      if (js == 0) {
	printf(" Consensus");   /* Consensus Consent */
	for (ks = 1; ks <= maxname - 7; ks++)
	  putchar(' ');

      } else {
	putchar(' ');
	for (ks = 0; ks < maxname; ks++)
	  putchar(ctree.brnchp[js]->namesp[ks]);
	printf("  ");
      }
      ls = maxline * is;
      if (ls > endsite)
	ls = endsite;
      for (ks = maxline * (is - 1) + 1; ks <= ls; ks++) {
	if (normal) {
	  if (js > 0 && chsequen[js][ks - 1] == chsequen[0][ks - 1])
	    putchar('.');
	  else
	    putchar(chsequen[js][ks - 1]);
	  if (ks % 10 == 0 && ks % maxline != 0)
	    putchar(' ');
/* p2c: protml.pas, line 466:
 * Note: Using % for possibly-negative arguments [317] */
/* p2c: protml.pas, line 466:
 * Note: Using % for possibly-negative arguments [317] */
	}
      }
      putchar('\n');
    }
    putchar('\n');
  }
}  /* getsequence */


/*************************************/
/***      READ SEQUENCES DATA      ***/
/*************************************/

Static Void getdata()
{
  /* read data from sequences file */
  if (numexe == 1) {
    printline(57L);
    printf("              PROTML Ver.1.00b  in MOLPHY\n");
    printf("    Maximum Likelihood Inference of Protein Phylogeny\n");
    printf("                based on Dayhoff model\n");
    printline(57L);
  }
  getnums();
  if (normal)
    getoptions();
  /* IF NOT freqsfrom THEN readbasefreqs; */
  if (numexe == 1) {
    if (normal)
      setuptree(&ctree);
  }
  if (normal)
    getsequence();
}  /* getdata */


/*** SORT OF SEQUENCES ***/
Static Void sitesort()
{
  /* sort sequences */
  /* Shell sort keeping sites, weights in same order */
  long gap, i, j, jj, jg, k, itemp;
  boolean flip, tied;
  long FORLIM;

  FORLIM = endsite;
  for (i = 1; i <= FORLIM; i++) {
    alias[i - 1] = i;
    weightw[i - 1] = 1;
  }
  gap = endsite / 2;
  while (gap > 0) {
    FORLIM = endsite;
    for (i = gap + 1; i <= FORLIM; i++) {
      j = i - gap;
      flip = true;
      while (j > 0 && flip) {
	jj = alias[j - 1];
	jg = alias[j + gap - 1];
	flip = false;   /* fel */
	tied = true;   /* fel */
	k = 1;
	while (k <= numsp && tied) {
	  flip = (chsequen[k][jj - 1] > chsequen[k][jg - 1]);
	  tied = (tied && chsequen[k][jj - 1] == chsequen[k][jg - 1]);
	  k++;
	}
	if (!flip)
	  break;
	itemp = alias[j - 1];
	alias[j - 1] = alias[j + gap - 1];
	alias[j + gap - 1] = itemp;
	itemp = weightw[j - 1];
	weightw[j - 1] = weightw[j + gap - 1];
	weightw[j + gap - 1] = itemp;
	j -= gap;
      }
    }
    gap /= 2;
  }
}  /* sitesort */


/*** COMBINATION OF SITE ***/
Static Void sitecombine()
{
  /* combine sites */
  /* combine sites that have identical patterns */
  long i, j, k;
  boolean tied;

  i = 1;
  while (i < endsite) {
    j = i + 1;
    tied = true;
    while (j <= endsite && tied) {
      k = 1;   /* fel */
      while (k <= numsp && tied) {
	tied = (tied && chsequen[k][alias[i - 1] - 1] == chsequen[k]
			[alias[j - 1] - 1]);
	k++;
      }
      if (tied && weightw[j - 1] > 0) {
	weightw[i - 1] += weightw[j - 1];
	weightw[j - 1] = 0;
	alias[j - 1] = alias[i - 1];
      }
      j++;
    }
    i = j - 1;
  }
}  /* sitecombine */


/*** SCRUNCH OF SITE ***/
Static Void sitescrunch()
{
  /* move so positively weighted sites come first */
  long i, j, itemp;
  boolean done, found;

  done = false;
  i = 1;
  j = 2;
  while (!done) {
    found = false;
    if (weightw[i - 1] > 0)
      i++;
    else {
      if (j <= i)
	j = i + 1;
      if (j <= endsite) {
	found = false;
	do {
	  found = (weightw[j - 1] > 0);
	  j++;
	} while (!(found || j > endsite));
	if (found) {
	  j--;
	  itemp = alias[i - 1];
	  alias[i - 1] = alias[j - 1];
	  alias[j - 1] = itemp;
	  itemp = weightw[i - 1];
	  weightw[i - 1] = weightw[j - 1];
	  weightw[j - 1] = itemp;
	} else
	  done = true;
      } else
	done = true;
    }
    done = (done || i >= endsite);
  }
}  /* sitescrunch */


/*** PRINT PATTERN ***/
Static Void printpattern(maxorder)
long maxorder;
{
  /* print patterned sequences */
  /* print pattern */
  long i, j, k, l, n, m, big, sml, kweight, FORLIM, FORLIM1;

  /* */
  if (debugoptn) {
    printf("\nalias  :\n");
    FORLIM = endptrn;
    for (i = 0; i < FORLIM; i++)
      printf("%4ld", alias[i]);
    printf("\n\nweight :\n");
    FORLIM = endptrn;
    for (i = 0; i < FORLIM; i++)
      printf("%4ld", weight[i]);
    printf("\n\nendptrn =%5ld\n\n\n", endptrn);
  }
  if (debugoptn) {
    printf("  num alias weight  pattern\n");
    FORLIM = endptrn;
    for (i = 0; i < FORLIM; i++) {
      printf("%4ld%6ld%7ld   ", i + 1, alias[i], weight[i]);
      FORLIM1 = numsp;
      for (j = 1; j <= FORLIM1; j++)
	printf("%2c", chsequen[j][alias[i] - 1]);
      putchar('\n');
    }
    putchar('\n');
  }
  /* */
  if (!writeoptn)
    return;

  printf("\n Species%5cPatternized Sequences\n", ' ');
  printf(" -------%5c---------------------\n\n", ' ');
  FORLIM = (endptrn - 1) / maxline;
  for (i = 0; i <= FORLIM; i++) {
    l = maxline * (i + 1);
    if (l > endptrn)
      l = endptrn;
    FORLIM1 = numsp;
    for (j = 1; j <= FORLIM1; j++) {
      putchar(' ');
      for (k = 0; k < maxname; k++)
	putchar(ctree.brnchp[j]->namesp[k]);
      printf("  ");
      for (k = maxline * i + 1; k <= l; k++) {
	if (normal) {
	  if (j > 1 &&
	      chsequen[j][alias[k - 1] - 1] == chsequen[1][alias[k - 1] - 1])
	    putchar('.');
	  else
	    putchar(chsequen[j][alias[k - 1] - 1]);
	  if (k % 10 == 0 && k % maxline != 0)
	    putchar(' ');
/* p2c: protml.pas, line 685:
 * Note: Using % for possibly-negative arguments [317] */
/* p2c: protml.pas, line 685:
 * Note: Using % for possibly-negative arguments [317] */
	}
      }
      putchar('\n');
    }
    putchar('\n');

    for (n = maxorder; n >= 1; n--) {
      printf("%3c", ' ');
      for (k = 1; k <= maxname; k++)
	putchar(' ');
      big = 1;
      for (m = 1; m <= n; m++)
	big *= 10;
      sml = big / 10;
      for (k = maxline * i + 1; k <= l; k++) {
	if (normal) {
	  kweight = weight[k - 1] % big / sml;
/* p2c: protml.pas, line 700:
 * Note: Using % for possibly-negative arguments [317] */
	  if (kweight > 0 && kweight < 10)
	    printf("%ld", kweight);
	  else if (kweight == 0) {
	    if (weight[k - 1] % big == weight[k - 1])
	      putchar(' ');
	    else if (weight[k - 1] % big < weight[k - 1])
	      putchar('0');
	    else
	      putchar('*');
/* p2c: protml.pas, line 705:
 * Note: Using % for possibly-negative arguments [317] */
	  } else
	    putchar('?');
	  if (k % 10 == 0 && k % maxline != 0)
	    putchar(' ');
/* p2c: protml.pas, line 714:
 * Note: Using % for possibly-negative arguments [317] */
/* p2c: protml.pas, line 714:
 * Note: Using % for possibly-negative arguments [317] */
	}
      }
      putchar('\n');
    }
    putchar('\n');
  }

/* p2c: protml.pas, line 707:
 * Note: Using % for possibly-negative arguments [317] */
}  /* printpattern */


/***********************************/
/***  ARRANGE SITES OF SEQUENCES  ***/
/***********************************/

Static Void makeweights()
{
  /* condense same site-pattern */
  /* make up weights vector to avoid duplicate computations */
  long iw, jw, kw, maxorder, maxweight, nweight, nw, FORLIM, FORLIM1;

  sitesort();
  sitecombine();
  sitescrunch();
  maxorder = 0;
  maxweight = 0;
  FORLIM = endsite;
  for (iw = 0; iw < FORLIM; iw++) {
    weight[iw] = weightw[iw];
    if (weight[iw] > 0)
      endptrn = iw + 1;
    if (weight[iw] > maxweight) {
      maxweight = weight[iw];
      nweight = weight[iw];
      nw = 0;
      do {
	nweight /= 10;
	nw++;
      } while (nweight != 0);
      if (nw > maxorder)
	maxorder = nw;
    }
  }
  if (endptrn > maxptrn) {
    printf(" TOO MANY PATTERNS: increase CONSTant maxptrn to at least %5ld\n",
	   endptrn);
    normal = false;
  }

  kw = 0;
  FORLIM = endptrn;
  for (iw = 1; iw <= FORLIM; iw++) {
    FORLIM1 = weight[iw - 1];
    for (jw = 1; jw <= FORLIM1; jw++) {
      kw++;
      weightw[kw - 1] = iw;
    }
  }

  printpattern(maxorder);

}  /* makeweights */


/*****************************************/
/***  SET PARTIAL LIKELIHOODS AT TIPS  ***/
/*****************************************/

Static Void makevalues()
{
  /* set up fractional likelihoods */
  /* set up fractional likelihoods at tips */
  long i, j, k;
  amity ba;
  long FORLIM, FORLIM1;

  FORLIM = endptrn;
  for (k = 0; k < FORLIM; k++) {
    j = alias[k];
    FORLIM1 = numsp;
    for (i = 1; i <= FORLIM1; i++) {
      for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
	ctree.brnchp[i]->UU.U0.prba[k][(long)ba - (long)AA] = 0.0;
      switch (chsequen[i][j - 1]) {

      case 'A':
	ctree.brnchp[i]->UU.U0.prba[k][0] = 1.0;
	break;

      case 'R':
	ctree.brnchp[i]->UU.U0.prba[k][(long)RR - (long)AA] = 1.0;
	break;

      case 'N':
	ctree.brnchp[i]->UU.U0.prba[k][(long)NN - (long)AA] = 1.0;
	break;

      case 'D':
	ctree.brnchp[i]->UU.U0.prba[k][(long)DD - (long)AA] = 1.0;
	break;

      case 'C':
	ctree.brnchp[i]->UU.U0.prba[k][(long)CC - (long)AA] = 1.0;
	break;

      case 'Q':
	ctree.brnchp[i]->UU.U0.prba[k][(long)QQ - (long)AA] = 1.0;
	break;

      case 'E':
	ctree.brnchp[i]->UU.U0.prba[k][(long)EE - (long)AA] = 1.0;
	break;

      case 'G':
	ctree.brnchp[i]->UU.U0.prba[k][(long)GG - (long)AA] = 1.0;
	break;

      case 'H':
	ctree.brnchp[i]->UU.U0.prba[k][(long)HH - (long)AA] = 1.0;
	break;

      case 'I':
	ctree.brnchp[i]->UU.U0.prba[k][(long)II - (long)AA] = 1.0;
	break;

      case 'L':
	ctree.brnchp[i]->UU.U0.prba[k][(long)LL - (long)AA] = 1.0;
	break;

      case 'K':
	ctree.brnchp[i]->UU.U0.prba[k][(long)KK - (long)AA] = 1.0;
	break;

      case 'M':
	ctree.brnchp[i]->UU.U0.prba[k][(long)MM - (long)AA] = 1.0;
	break;

      case 'F':
	ctree.brnchp[i]->UU.U0.prba[k][(long)FF - (long)AA] = 1.0;
	break;

      case 'P':
	ctree.brnchp[i]->UU.U0.prba[k][(long)PP_ - (long)AA] = 1.0;
	break;

      case 'S':
	ctree.brnchp[i]->UU.U0.prba[k][(long)SS - (long)AA] = 1.0;
	break;

      case 'T':
	ctree.brnchp[i]->UU.U0.prba[k][(long)TT - (long)AA] = 1.0;
	break;

      case 'W':
	ctree.brnchp[i]->UU.U0.prba[k][(long)WW - (long)AA] = 1.0;
	break;

      case 'Y':
	ctree.brnchp[i]->UU.U0.prba[k][(long)YY - (long)AA] = 1.0;
	break;

      case 'V':
	ctree.brnchp[i]->UU.U0.prba[k][(long)VV - (long)AA] = 1.0;
	break;

      case 'B':
	ctree.brnchp[i]->UU.U0.prba[k][(long)DD - (long)AA] = 0.5;
	ctree.brnchp[i]->UU.U0.prba[k][(long)NN - (long)AA] = 0.5;
	break;

      case 'Z':
	ctree.brnchp[i]->UU.U0.prba[k][(long)EE - (long)AA] = 0.5;
	ctree.brnchp[i]->UU.U0.prba[k][(long)QQ - (long)AA] = 0.5;
	break;

      case 'X':
	for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
	  ctree.brnchp[i]->UU.U0.prba[k][(long)ba - (long)AA] = 1.0;
	break;

      case '?':
	for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
	  ctree.brnchp[i]->UU.U0.prba[k][(long)ba - (long)AA] = 1.0;
	break;

      case '-':
	for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
	  ctree.brnchp[i]->UU.U0.prba[k][(long)ba - (long)AA] = 1.0;
	break;
      }
    }
  }
}  /* makevalues */


/*** EMPIRICAL FREQENCIES OF AMINO ACIDS ***/
Static Void empirifreqsA()
{
  /* calculate empirical frequencies */
  /* Get empirical frequencies of amino acids from the data */
  long ia, ja;
  amity ba;
  aryamity sfreqa;
  double sum;
  long FORLIM, FORLIM1;

  for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    sfreqa[(long)ba - (long)AA] = 0.0;
  sum = 0.0;
  FORLIM = numsp;
  for (ia = 1; ia <= FORLIM; ia++) {
    FORLIM1 = endptrn;
    for (ja = 0; ja < FORLIM1; ja++) {
      for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
	sfreqa[(long)ba - (long)AA] +=
	  weight[ja] * ctree.brnchp[ia]->UU.U0.prba[ja][(long)ba - (long)AA];
    }
  }
  for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    sum += sfreqa[(long)ba - (long)AA];
  for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    freqa[(long)ba - (long)AA] = sfreqa[(long)ba - (long)AA] / sum;
  if (!(numexe == 1 && writeoptn))
    return;
  printf("\n%13s%7.1f\n", " Total acid :", sum);
  printf("%13s", "  A,R,N,D,C :");
  for (ba = AA; (long)ba <= (long)CC; ba = (amity)((long)ba + 1))
    printf("%7.1f", sfreqa[(long)ba - (long)AA]);
  printf("\n%13s", "  Q,E,G,H,I :");
  for (ba = QQ; (long)ba <= (long)II; ba = (amity)((long)ba + 1))
    printf("%7.1f", sfreqa[(long)ba - (long)AA]);
  printf("\n%13s", "  L,K,M,F,P :");
  for (ba = LL; (long)ba <= (long)PP_; ba = (amity)((long)ba + 1))
    printf("%7.1f", sfreqa[(long)ba - (long)AA]);
  printf("\n%13s", "  S,T,W,Y,V :");
  for (ba = SS; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    printf("%7.1f", sfreqa[(long)ba - (long)AA]);
  putchar('\n');
}  /* empirifreqsA */


/*************************************/
/***      GET FREQUENCY            ***/
/*************************************/

Static Void getbasefreqs()
{
  /* print frequencies */
  amity ba;

  empirifreqsA();
  if (!(numexe == 1 && writeoptn))
    return;
  printf("\n Empirical");
  /* IF freqsfrom THEN */
  printf(" Amino Acid Frequencies:\n");
  printf("%13s", "A,R,N,D,C :");
  for (ba = AA; (long)ba <= (long)CC; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqa[(long)ba - (long)AA]);
  printf("\n%13s", "Q,E,G,H,I :");
  for (ba = QQ; (long)ba <= (long)II; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqa[(long)ba - (long)AA]);
  printf("\n%13s", "L,K,M,F,P :");
  for (ba = LL; (long)ba <= (long)PP_; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqa[(long)ba - (long)AA]);
  printf("\n%13s", "S,T,W,Y,V :");
  for (ba = SS; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqa[(long)ba - (long)AA]);
  putchar('\n');
}  /* getbasefreqs */


Static Void getinput()
{
  /* read data and set up data */
  /* read input file */
  normal = true;
  if (normal)
    getdata();
  if (normal)
    makeweights();
  if (normal)
    makevalues();
  if (normal)
    getbasefreqs();
}  /* getinput */


Static Void printnmtrx(nmtrx)
naryamity *nmtrx;
{
  long i, j;

  printf(" MATRIX(numtype)\n");
  for (i = 0; i <= 19; i++) {
    for (j = 1; j <= 20; j++) {
      printf("%7.3f", nmtrx[i][j - 1]);
      if (j == 10 || j == 20)
	putchar('\n');
    }
    putchar('\n');
  }
}  /* printnmtrx */


#define eps             1.0e-20


Static Void luinverse(omtrx, imtrx, nmtrx)
naryamity *omtrx, *imtrx;
long nmtrx;
{
  /* INVERSION OF MATRIX ON LU DECOMPOSITION */
  long i, j, k, l, maxi, idx, ix, jx;
  double sum, tmp, maxb, aw;
  niaryamity index;
  double *wk;

  wk = (double *)Malloc(sizeof(naryamity));
  aw = 1.0;
  for (i = 0; i < nmtrx; i++) {
    maxb = 0.0;
    for (j = 0; j < nmtrx; j++) {
      if (fabs(omtrx[i][j]) > maxb)
	maxb = fabs(omtrx[i][j]);
    }
    if (maxb == 0.0)
      printf("PROC. LUINVERSE:  singular\n");
    wk[i] = 1.0 / maxb;
  }
  for (j = 1; j <= nmtrx; j++) {
    for (i = 1; i < j; i++) {
      sum = omtrx[i - 1][j - 1];
      for (k = 0; k <= i - 2; k++)
	sum -= omtrx[i - 1][k] * omtrx[k][j - 1];
      omtrx[i - 1][j - 1] = sum;
    }
    maxb = 0.0;
    for (i = j; i <= nmtrx; i++) {
      sum = omtrx[i - 1][j - 1];
      for (k = 0; k <= j - 2; k++)
	sum -= omtrx[i - 1][k] * omtrx[k][j - 1];
      omtrx[i - 1][j - 1] = sum;
      tmp = wk[i - 1] * fabs(sum);
      if (tmp >= maxb) {
	maxb = tmp;
	maxi = i;
      }
    }
    if (j != maxi) {
      for (k = 0; k < nmtrx; k++) {
	tmp = omtrx[maxi - 1][k];
	omtrx[maxi - 1][k] = omtrx[j - 1][k];
	omtrx[j - 1][k] = tmp;
      }
      aw = -aw;
      wk[maxi - 1] = wk[j - 1];
    }
    index[j - 1] = maxi;
    if (omtrx[j - 1][j - 1] == 0.0)
      omtrx[j - 1][j - 1] = eps;
    if (j != nmtrx) {
      tmp = 1.0 / omtrx[j - 1][j - 1];
      for (i = j; i < nmtrx; i++)
	omtrx[i][j - 1] *= tmp;
    }
  }
  for (jx = 0; jx < nmtrx; jx++) {
    for (ix = 0; ix < nmtrx; ix++)
      wk[ix] = 0.0;
    wk[jx] = 1.0;
    l = 0;
    for (i = 1; i <= nmtrx; i++) {
      idx = index[i - 1];
      sum = wk[idx - 1];
      wk[idx - 1] = wk[i - 1];
      if (l != 0) {
	for (j = l - 1; j <= i - 2; j++)
	  sum -= omtrx[i - 1][j] * wk[j];
      } else if (sum != 0.0)
	l = i;
      wk[i - 1] = sum;
    }
    for (i = nmtrx - 1; i >= 0; i--) {
      sum = wk[i];
      for (j = i + 1; j < nmtrx; j++)
	sum -= omtrx[i][j] * wk[j];
      wk[i] = sum / omtrx[i][i];
    }
    for (ix = 0; ix < nmtrx; ix++)
      imtrx[ix][jx] = wk[ix];
  }
  Free(wk);
}  /* luinverse */

#undef eps


Static Void mproduct(am, bm, cm, na, nb, nc)
naryamity *am, *bm, *cm;
long na, nb, nc;
{
  long ia, ib, ic;
  double sum;

  for (ia = 0; ia < na; ia++) {
    for (ic = 0; ic < nc; ic++) {
      sum = 0.0;
      for (ib = 0; ib < nb; ib++)
	sum += am[ia][ib] * bm[ib][ic];
      cm[ia][ic] = sum;
    }
  }
}  /* mproduct */


Static Void convermtrx(amtrx, nmtrx)
aryamity *amtrx;
naryamity *nmtrx;
{
  amity ba1, ba2;

  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    for (ba2 = AA; (long)ba2 <= (long)VV; ba2 = (amity)((long)ba2 + 1))
      nmtrx[(int)ba1][(int)ba2] = amtrx[(long)ba1 - (long)AA]
	[(long)ba2 - (long)AA];
  }
}  /* convermtrx */


Static Void reconvermtrx(nmtrx, amtrx)
naryamity *nmtrx;
aryamity *amtrx;
{
  amity ba1, ba2;

  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    for (ba2 = AA; (long)ba2 <= (long)VV; ba2 = (amity)((long)ba2 + 1))
      amtrx[(long)ba1 - (long)AA][(long)ba2 - (long)AA] = nmtrx[(int)ba1]
	[(int)ba2];
  }
}  /* reconvermtrx */


Static Void printeigen()
{
  amity ba1, ba2;

  printf(" EIGEN VECTOR\n");
  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    for (ba2 = AA; (long)ba2 <= (long)VV; ba2 = (amity)((long)ba2 + 1)) {
      printf("%7.3f", ev[(long)ba1 - (long)AA][(long)ba2 - (long)AA]);
      if (ba2 == II || ba2 == VV)
	putchar('\n');
    }
    putchar('\n');
  }
  printf(" EIGEN VALUE\n");
  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    printf("%7.3f", eval[(long)ba1 - (long)AA]);
    if (ba1 == II || ba1 == VV)
      putchar('\n');
  }
}  /* printeigen */


Static Void checkevector(imtrx, nn)
naryamity *imtrx;
long nn;
{
  long i, j;

  for (i = 1; i <= nn; i++) {
    for (j = 1; j <= nn; j++) {
      if (i == j) {
	if (fabs(imtrx[i - 1][j - 1] - 1.0) > 1.0e-5)
	  printf(" error1 eigen vector (checkevector)\n");
      } else {
	if (fabs(imtrx[i - 1][j - 1]) > 1.0e-5)
	  printf(" error2 eigen vector (checkevector)\n");
      }
    }
  }
}  /* checkevector */


Static Void printamtrx(amtrx)
aryamity *amtrx;
{
  amity ba1, ba2;

  printf(" MATRIX(amitype)\n");
  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    for (ba2 = AA; (long)ba2 <= (long)VV; ba2 = (amity)((long)ba2 + 1)) {
      if (amtrx[(long)ba1 - (long)AA][(long)ba2 - (long)AA] > 0.1e-5)
	printf("%14.8f", amtrx[(long)ba1 - (long)AA][(long)ba2 - (long)AA]);
      else if (amtrx[(long)ba1 - (long)AA][(long)ba2 - (long)AA] <= 0.0)
	printf("%6c% .1E",
	       ' ', amtrx[(long)ba1 - (long)AA][(long)ba2 - (long)AA]);
      else
	printf("%3c% .4E",
	       ' ', amtrx[(long)ba1 - (long)AA][(long)ba2 - (long)AA]);
      if (ba2 == II || ba2 == VV || ba2 == CC || ba2 == PP_)
	putchar('\n');
    }
    putchar('\n');
  }
}  /* printamtrx */


Static Void printfreqdyhf()
{
  amity ba;

  printf("\n Dayhoff's Amino Acid Frequencies:\n");
  printf("%13s", "A,R,N,D,C :");
  for (ba = AA; (long)ba <= (long)CC; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqdyhf[(long)ba - (long)AA]);
  printf("\n%13s", "Q,E,G,H,I :");
  for (ba = QQ; (long)ba <= (long)II; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqdyhf[(long)ba - (long)AA]);
  printf("\n%13s", "L,K,M,F,P :");
  for (ba = LL; (long)ba <= (long)PP_; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqdyhf[(long)ba - (long)AA]);
  printf("\n%13s", "S,T,W,Y,V :");
  for (ba = SS; (long)ba <= (long)VV; ba = (amity)((long)ba + 1))
    printf("%7.3f", freqdyhf[(long)ba - (long)AA]);
  putchar('\n');
}  /* printfreqdyhf */


/*************************************/
/*** TRANSITION PROBABILITY MATRIX ***/
/*************************************/
/*** READ MATRIX ( EIGEN VALUE,VECTOR AND FREQUENCY ) ***/
Static Void readeigenv()
{
  amity ba1, ba2;

  for (ba2 = AA; (long)ba2 <= (long)VV; ba2 = (amity)((long)ba2 + 1)) {
    for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
      if (P_eoln(tpmfile)) {
	fscanf(tpmfile, "%*[^\n]");
	getc(tpmfile);
      }
      fscanf(tpmfile, "%lg", &ev[(long)ba1 - (long)AA][(long)ba2 - (long)AA]);
    }
  }
  fscanf(tpmfile, "%*[^\n]");
  getc(tpmfile);
  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    if (P_eoln(tpmfile)) {
      fscanf(tpmfile, "%*[^\n]");
      getc(tpmfile);
    }
    fscanf(tpmfile, "%lg", &eval[(long)ba1 - (long)AA]);
  }
  fscanf(tpmfile, "%*[^\n]");
  getc(tpmfile);
  for (ba1 = AA; (long)ba1 <= (long)VV; ba1 = (amity)((long)ba1 + 1)) {
    if (P_eoln(tpmfile)) {
      fscanf(tpmfile, "%*[^\n]");
      getc(tpmfile);
    }
    fscanf(tpmfile, "%lg%*[^\n]", &freqdyhf[(long)ba1 - (long)AA]);
    getc(tpmfile);
  }
}  /* readeigenv */


/*** DATA MATRIX ( EIGEN VALUE,VECTOR AND FREQUENCY ) ***/
Static Void dataeigenv()
{
  ev[0][0] = -2.18920e-01;
  ev[(long)RR - (long)AA][0] = -4.01968e-02;
  ev[(long)NN - (long)AA][0] = -2.27758e-01;
  ev[(long)DD - (long)AA][0] = -4.04699e-01;
  ev[(long)CC - (long)AA][0] = -3.49083e-02;
  ev[(long)QQ - (long)AA][0] = -2.38091e-01;
  ev[(long)EE - (long)AA][0] = 5.24114e-01;
  ev[(long)GG - (long)AA][0] = -2.34117e-02;
  ev[(long)HH - (long)AA][0] = 9.97954e-02;
  ev[(long)II - (long)AA][0] = -8.45249e-03;
  ev[(long)LL - (long)AA][0] = 5.28066e-03;
  ev[(long)KK - (long)AA][0] = 2.05284e-02;
  ev[(long)MM - (long)AA][0] = -1.23853e-02;
  ev[(long)FF - (long)AA][0] = -9.50275e-03;
  ev[(long)PP_ - (long)AA][0] = -3.24017e-02;
  ev[(long)SS - (long)AA][0] = 5.89404e-01;
  ev[(long)TT - (long)AA][0] = -1.99416e-01;
  ev[(long)WW - (long)AA][0] = -1.52404e-02;
  ev[(long)YY - (long)AA][0] = -1.81586e-03;
  ev[(long)VV - (long)AA][0] = 4.98109e-02;

  ev[0][(long)RR - (long)AA] = 4.00877e-02;
  ev[(long)RR - (long)AA][(long)RR - (long)AA] = 3.76303e-02;
  ev[(long)NN - (long)AA][(long)RR - (long)AA] = 7.25090e-01;
  ev[(long)DD - (long)AA][(long)RR - (long)AA] = -5.28042e-01;
  ev[(long)CC - (long)AA][(long)RR - (long)AA] = 1.46525e-02;
  ev[(long)QQ - (long)AA][(long)RR - (long)AA] = -8.66164e-02;
  ev[(long)EE - (long)AA][(long)RR - (long)AA] = 3.20299e-01;
  ev[(long)GG - (long)AA][(long)RR - (long)AA] = 7.74478e-03;
  ev[(long)HH - (long)AA][(long)RR - (long)AA] = -8.90059e-02;
  ev[(long)II - (long)AA][(long)RR - (long)AA] = -2.94900e-02;
  ev[(long)LL - (long)AA][(long)RR - (long)AA] = -3.50400e-03;
  ev[(long)KK - (long)AA][(long)RR - (long)AA] = -5.22374e-02;
  ev[(long)MM - (long)AA][(long)RR - (long)AA] = 1.94473e-02;
  ev[(long)FF - (long)AA][(long)RR - (long)AA] = 5.80870e-03;
  ev[(long)PP_ - (long)AA][(long)RR - (long)AA] = 1.62922e-02;
  ev[(long)SS - (long)AA][(long)RR - (long)AA] = -2.60397e-01;
  ev[(long)TT - (long)AA][(long)RR - (long)AA] = 4.22891e-02;
  ev[(long)WW - (long)AA][(long)RR - (long)AA] = 2.42879e-03;
  ev[(long)YY - (long)AA][(long)RR - (long)AA] = -1.46244e-02;
  ev[(long)VV - (long)AA][(long)RR - (long)AA] = -5.05405e-04;

  ev[0][(long)NN - (long)AA] = -4.62992e-01;
  ev[(long)RR - (long)AA][(long)NN - (long)AA] = 2.14018e-02;
  ev[(long)NN - (long)AA][(long)NN - (long)AA] = 1.71750e-01;
  ev[(long)DD - (long)AA][(long)NN - (long)AA] = 1.02440e-01;
  ev[(long)CC - (long)AA][(long)NN - (long)AA] = 3.17009e-03;
  ev[(long)QQ - (long)AA][(long)NN - (long)AA] = 1.50618e-01;
  ev[(long)EE - (long)AA][(long)NN - (long)AA] = -1.07848e-01;
  ev[(long)GG - (long)AA][(long)NN - (long)AA] = 5.62329e-02;
  ev[(long)HH - (long)AA][(long)NN - (long)AA] = -1.09388e-01;
  ev[(long)II - (long)AA][(long)NN - (long)AA] = -6.52572e-01;
  ev[(long)LL - (long)AA][(long)NN - (long)AA] = 1.46263e-02;
  ev[(long)KK - (long)AA][(long)NN - (long)AA] = -5.22977e-02;
  ev[(long)MM - (long)AA][(long)NN - (long)AA] = 4.61043e-02;
  ev[(long)FF - (long)AA][(long)NN - (long)AA] = 4.16223e-02;
  ev[(long)PP_ - (long)AA][(long)NN - (long)AA] = 6.60296e-02;
  ev[(long)SS - (long)AA][(long)NN - (long)AA] = 4.86194e-02;
  ev[(long)TT - (long)AA][(long)NN - (long)AA] = 3.28837e-01;
  ev[(long)WW - (long)AA][(long)NN - (long)AA] = -4.61826e-03;
  ev[(long)YY - (long)AA][(long)NN - (long)AA] = -8.60905e-03;
  ev[(long)VV - (long)AA][(long)NN - (long)AA] = 3.84867e-01;

  ev[0][(long)DD - (long)AA] = 2.47117e-02;
  ev[(long)RR - (long)AA][(long)DD - (long)AA] = -3.76030e-02;
  ev[(long)NN - (long)AA][(long)DD - (long)AA] = 5.56820e-01;
  ev[(long)DD - (long)AA][(long)DD - (long)AA] = 5.60598e-02;
  ev[(long)CC - (long)AA][(long)DD - (long)AA] = -2.51572e-02;
  ev[(long)QQ - (long)AA][(long)DD - (long)AA] = 3.40304e-01;
  ev[(long)EE - (long)AA][(long)DD - (long)AA] = -4.10919e-01;
  ev[(long)GG - (long)AA][(long)DD - (long)AA] = -7.16450e-02;
  ev[(long)HH - (long)AA][(long)DD - (long)AA] = -2.14248e-01;
  ev[(long)II - (long)AA][(long)DD - (long)AA] = 5.25788e-02;
  ev[(long)LL - (long)AA][(long)DD - (long)AA] = -1.22652e-02;
  ev[(long)KK - (long)AA][(long)DD - (long)AA] = -5.68214e-02;
  ev[(long)MM - (long)AA][(long)DD - (long)AA] = 1.75995e-02;
  ev[(long)FF - (long)AA][(long)DD - (long)AA] = -7.65321e-03;
  ev[(long)PP_ - (long)AA][(long)DD - (long)AA] = -5.79082e-02;
  ev[(long)SS - (long)AA][(long)DD - (long)AA] = 3.84128e-01;
  ev[(long)TT - (long)AA][(long)DD - (long)AA] = -4.36123e-01;
  ev[(long)WW - (long)AA][(long)DD - (long)AA] = -1.26346e-02;
  ev[(long)YY - (long)AA][(long)DD - (long)AA] = -3.19921e-03;
  ev[(long)VV - (long)AA][(long)DD - (long)AA] = 2.57284e-02;

  ev[0][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)RR - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)NN - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)DD - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)CC - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)QQ - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)EE - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)GG - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)HH - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)II - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)LL - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)KK - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)MM - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)FF - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)PP_ - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)SS - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)TT - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)WW - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)YY - (long)AA][(long)CC - (long)AA] = -2.23607e-01;
  ev[(long)VV - (long)AA][(long)CC - (long)AA] = -2.23607e-01;

  ev[0][(long)QQ - (long)AA] = 4.04873e-01;
  ev[(long)RR - (long)AA][(long)QQ - (long)AA] = 6.49103e-03;
  ev[(long)NN - (long)AA][(long)QQ - (long)AA] = -1.19891e-01;
  ev[(long)DD - (long)AA][(long)QQ - (long)AA] = -3.67766e-03;
  ev[(long)CC - (long)AA][(long)QQ - (long)AA] = -9.46397e-04;
  ev[(long)QQ - (long)AA][(long)QQ - (long)AA] = -1.89655e-01;
  ev[(long)EE - (long)AA][(long)QQ - (long)AA] = 5.91497e-02;
  ev[(long)GG - (long)AA][(long)QQ - (long)AA] = -8.40994e-02;
  ev[(long)HH - (long)AA][(long)QQ - (long)AA] = 8.85589e-02;
  ev[(long)II - (long)AA][(long)QQ - (long)AA] = -7.24798e-01;
  ev[(long)LL - (long)AA][(long)QQ - (long)AA] = 2.13078e-02;
  ev[(long)KK - (long)AA][(long)QQ - (long)AA] = 6.70215e-02;
  ev[(long)MM - (long)AA][(long)QQ - (long)AA] = 4.33730e-02;
  ev[(long)FF - (long)AA][(long)QQ - (long)AA] = 4.68020e-02;
  ev[(long)PP_ - (long)AA][(long)QQ - (long)AA] = -7.75734e-02;
  ev[(long)SS - (long)AA][(long)QQ - (long)AA] = -6.04936e-02;
  ev[(long)TT - (long)AA][(long)QQ - (long)AA] = -3.20011e-01;
  ev[(long)WW - (long)AA][(long)QQ - (long)AA] = 6.03313e-04;
  ev[(long)YY - (long)AA][(long)QQ - (long)AA] = -9.61127e-03;
  ev[(long)VV - (long)AA][(long)QQ - (long)AA] = 3.47473e-01;

  ev[0][(long)EE - (long)AA] = -4.99671e-02;
  ev[(long)RR - (long)AA][(long)EE - (long)AA] = 4.11185e-02;
  ev[(long)NN - (long)AA][(long)EE - (long)AA] = 1.47778e-01;
  ev[(long)DD - (long)AA][(long)EE - (long)AA] = 1.64073e-01;
  ev[(long)CC - (long)AA][(long)EE - (long)AA] = -1.66608e-03;
  ev[(long)QQ - (long)AA][(long)EE - (long)AA] = -3.40600e-01;
  ev[(long)EE - (long)AA][(long)EE - (long)AA] = -2.75784e-02;
  ev[(long)GG - (long)AA][(long)EE - (long)AA] = -6.14738e-03;
  ev[(long)HH - (long)AA][(long)EE - (long)AA] = 7.45280e-02;
  ev[(long)II - (long)AA][(long)EE - (long)AA] = 6.19257e-02;
  ev[(long)LL - (long)AA][(long)EE - (long)AA] = 8.66441e-02;
  ev[(long)KK - (long)AA][(long)EE - (long)AA] = 3.88639e-02;
  ev[(long)MM - (long)AA][(long)EE - (long)AA] = -8.97628e-01;
  ev[(long)FF - (long)AA][(long)EE - (long)AA] = -3.67076e-03;
  ev[(long)PP_ - (long)AA][(long)EE - (long)AA] = 4.14761e-02;
  ev[(long)SS - (long)AA][(long)EE - (long)AA] = 1.90444e-03;
  ev[(long)TT - (long)AA][(long)EE - (long)AA] = -4.52208e-02;
  ev[(long)WW - (long)AA][(long)EE - (long)AA] = -8.08550e-03;
  ev[(long)YY - (long)AA][(long)EE - (long)AA] = -1.28114e-02;
  ev[(long)VV - (long)AA][(long)EE - (long)AA] = 4.57149e-02;

  ev[0][(long)GG - (long)AA] = -6.76985e-02;
  ev[(long)RR - (long)AA][(long)GG - (long)AA] = 1.07693e-01;
  ev[(long)NN - (long)AA][(long)GG - (long)AA] = 1.83827e-01;
  ev[(long)DD - (long)AA][(long)GG - (long)AA] = 2.57799e-01;
  ev[(long)CC - (long)AA][(long)GG - (long)AA] = -8.80400e-04;
  ev[(long)QQ - (long)AA][(long)GG - (long)AA] = -5.07282e-01;
  ev[(long)EE - (long)AA][(long)GG - (long)AA] = -2.08116e-02;
  ev[(long)GG - (long)AA][(long)GG - (long)AA] = -9.91077e-03;
  ev[(long)HH - (long)AA][(long)GG - (long)AA] = 1.32778e-01;
  ev[(long)II - (long)AA][(long)GG - (long)AA] = 3.30619e-02;
  ev[(long)LL - (long)AA][(long)GG - (long)AA] = -5.54361e-02;
  ev[(long)KK - (long)AA][(long)GG - (long)AA] = -7.58113e-02;
  ev[(long)MM - (long)AA][(long)GG - (long)AA] = 7.67118e-01;
  ev[(long)FF - (long)AA][(long)GG - (long)AA] = -8.15430e-03;
  ev[(long)PP_ - (long)AA][(long)GG - (long)AA] = 5.85005e-02;
  ev[(long)SS - (long)AA][(long)GG - (long)AA] = 1.59381e-02;
  ev[(long)TT - (long)AA][(long)GG - (long)AA] = -6.67189e-02;
  ev[(long)WW - (long)AA][(long)GG - (long)AA] = -9.06354e-03;
  ev[(long)YY - (long)AA][(long)GG - (long)AA] = -6.80482e-03;
  ev[(long)VV - (long)AA][(long)GG - (long)AA] = -3.69299e-02;

  ev[0][(long)HH - (long)AA] = 2.37414e-02;
  ev[(long)RR - (long)AA][(long)HH - (long)AA] = -1.31704e-02;
  ev[(long)NN - (long)AA][(long)HH - (long)AA] = 1.93358e-02;
  ev[(long)DD - (long)AA][(long)HH - (long)AA] = 2.78387e-02;
  ev[(long)CC - (long)AA][(long)HH - (long)AA] = 6.35183e-02;
  ev[(long)QQ - (long)AA][(long)HH - (long)AA] = 1.94232e-02;
  ev[(long)EE - (long)AA][(long)HH - (long)AA] = 2.72120e-02;
  ev[(long)GG - (long)AA][(long)HH - (long)AA] = 3.14653e-02;
  ev[(long)HH - (long)AA][(long)HH - (long)AA] = 8.67994e-03;
  ev[(long)II - (long)AA][(long)HH - (long)AA] = 3.64294e-03;
  ev[(long)LL - (long)AA][(long)HH - (long)AA] = -1.65127e-02;
  ev[(long)KK - (long)AA][(long)HH - (long)AA] = 1.45011e-02;
  ev[(long)MM - (long)AA][(long)HH - (long)AA] = 1.53344e-04;
  ev[(long)FF - (long)AA][(long)HH - (long)AA] = -7.14500e-02;
  ev[(long)PP_ - (long)AA][(long)HH - (long)AA] = 2.47183e-02;
  ev[(long)SS - (long)AA][(long)HH - (long)AA] = 1.83958e-02;
  ev[(long)TT - (long)AA][(long)HH - (long)AA] = 2.03333e-02;
  ev[(long)WW - (long)AA][(long)HH - (long)AA] = -9.90001e-01;
  ev[(long)YY - (long)AA][(long)HH - (long)AA] = -6.85327e-02;
  ev[(long)VV - (long)AA][(long)HH - (long)AA] = 1.15883e-02;

  ev[0][(long)II - (long)AA] = 3.86884e-02;
  ev[(long)RR - (long)AA][(long)II - (long)AA] = 6.83612e-02;
  ev[(long)NN - (long)AA][(long)II - (long)AA] = 5.94583e-02;
  ev[(long)DD - (long)AA][(long)II - (long)AA] = 7.89097e-02;
  ev[(long)CC - (long)AA][(long)II - (long)AA] = -9.49564e-01;
  ev[(long)QQ - (long)AA][(long)II - (long)AA] = 7.75597e-02;
  ev[(long)EE - (long)AA][(long)II - (long)AA] = 7.93973e-02;
  ev[(long)GG - (long)AA][(long)II - (long)AA] = 5.96830e-02;
  ev[(long)HH - (long)AA][(long)II - (long)AA] = 5.15293e-02;
  ev[(long)II - (long)AA][(long)II - (long)AA] = 7.67383e-03;
  ev[(long)LL - (long)AA][(long)II - (long)AA] = 2.21331e-02;
  ev[(long)KK - (long)AA][(long)II - (long)AA] = 8.30887e-02;
  ev[(long)MM - (long)AA][(long)II - (long)AA] = 3.95955e-02;
  ev[(long)FF - (long)AA][(long)II - (long)AA] = -1.11612e-01;
  ev[(long)PP_ - (long)AA][(long)II - (long)AA] = 5.03976e-02;
  ev[(long)SS - (long)AA][(long)II - (long)AA] = 1.59569e-02;
  ev[(long)TT - (long)AA][(long)II - (long)AA] = 3.72414e-02;
  ev[(long)WW - (long)AA][(long)II - (long)AA] = -5.52876e-02;
  ev[(long)YY - (long)AA][(long)II - (long)AA] = -1.86929e-01;
  ev[(long)VV - (long)AA][(long)II - (long)AA] = 1.41872e-02;

  ev[0][(long)LL - (long)AA] = 5.80295e-02;
  ev[(long)RR - (long)AA][(long)LL - (long)AA] = 1.02103e-01;
  ev[(long)NN - (long)AA][(long)LL - (long)AA] = 7.00404e-02;
  ev[(long)DD - (long)AA][(long)LL - (long)AA] = 9.76903e-02;
  ev[(long)CC - (long)AA][(long)LL - (long)AA] = 2.47574e-01;
  ev[(long)QQ - (long)AA][(long)LL - (long)AA] = 7.38092e-02;
  ev[(long)EE - (long)AA][(long)LL - (long)AA] = 9.10580e-02;
  ev[(long)GG - (long)AA][(long)LL - (long)AA] = 1.00958e-01;
  ev[(long)HH - (long)AA][(long)LL - (long)AA] = 3.49872e-02;
  ev[(long)II - (long)AA][(long)LL - (long)AA] = -1.13481e-01;
  ev[(long)LL - (long)AA][(long)LL - (long)AA] = -2.12223e-01;
  ev[(long)KK - (long)AA][(long)LL - (long)AA] = 9.20232e-02;
  ev[(long)MM - (long)AA][(long)LL - (long)AA] = -1.06117e-01;
  ev[(long)FF - (long)AA][(long)LL - (long)AA] = -5.51278e-01;
  ev[(long)PP_ - (long)AA][(long)LL - (long)AA] = 8.17221e-02;
  ev[(long)SS - (long)AA][(long)LL - (long)AA] = 7.48437e-02;
  ev[(long)TT - (long)AA][(long)LL - (long)AA] = 4.59234e-02;
  ev[(long)WW - (long)AA][(long)LL - (long)AA] = 4.34903e-01;
  ev[(long)YY - (long)AA][(long)LL - (long)AA] = -5.43823e-01;
  ev[(long)VV - (long)AA][(long)LL - (long)AA] = -6.69564e-02;

  ev[0][(long)KK - (long)AA] = 1.68796e-01;
  ev[(long)RR - (long)AA][(long)KK - (long)AA] = 6.98733e-01;
  ev[(long)NN - (long)AA][(long)KK - (long)AA] = -3.64477e-02;
  ev[(long)DD - (long)AA][(long)KK - (long)AA] = 4.79840e-02;
  ev[(long)CC - (long)AA][(long)KK - (long)AA] = -2.83352e-02;
  ev[(long)QQ - (long)AA][(long)KK - (long)AA] = 2.07546e-03;
  ev[(long)EE - (long)AA][(long)KK - (long)AA] = 6.44875e-02;
  ev[(long)GG - (long)AA][(long)KK - (long)AA] = -1.24957e-01;
  ev[(long)HH - (long)AA][(long)KK - (long)AA] = -2.31511e-01;
  ev[(long)II - (long)AA][(long)KK - (long)AA] = -1.08720e-01;
  ev[(long)LL - (long)AA][(long)KK - (long)AA] = 6.27788e-02;
  ev[(long)KK - (long)AA][(long)KK - (long)AA] = -4.17790e-01;
  ev[(long)MM - (long)AA][(long)KK - (long)AA] = -1.98195e-01;
  ev[(long)FF - (long)AA][(long)KK - (long)AA] = -1.01464e-02;
  ev[(long)PP_ - (long)AA][(long)KK - (long)AA] = -2.34527e-01;
  ev[(long)SS - (long)AA][(long)KK - (long)AA] = 1.80480e-01;
  ev[(long)TT - (long)AA][(long)KK - (long)AA] = 2.62775e-01;
  ev[(long)WW - (long)AA][(long)KK - (long)AA] = -7.67023e-02;
  ev[(long)YY - (long)AA][(long)KK - (long)AA] = 8.53465e-03;
  ev[(long)VV - (long)AA][(long)KK - (long)AA] = -1.14869e-01;

  ev[0][(long)MM - (long)AA] = 1.14030e-02;
  ev[(long)RR - (long)AA][(long)MM - (long)AA] = 1.03453e-01;
  ev[(long)NN - (long)AA][(long)MM - (long)AA] = 1.03058e-01;
  ev[(long)DD - (long)AA][(long)MM - (long)AA] = 1.25170e-01;
  ev[(long)CC - (long)AA][(long)MM - (long)AA] = -7.83690e-02;
  ev[(long)QQ - (long)AA][(long)MM - (long)AA] = 8.15278e-02;
  ev[(long)EE - (long)AA][(long)MM - (long)AA] = 1.09160e-01;
  ev[(long)GG - (long)AA][(long)MM - (long)AA] = 9.53752e-02;
  ev[(long)HH - (long)AA][(long)MM - (long)AA] = 1.40899e-01;
  ev[(long)II - (long)AA][(long)MM - (long)AA] = -2.81849e-01;
  ev[(long)LL - (long)AA][(long)MM - (long)AA] = -4.92274e-01;
  ev[(long)KK - (long)AA][(long)MM - (long)AA] = 9.83942e-02;
  ev[(long)MM - (long)AA][(long)MM - (long)AA] = -3.14326e-01;
  ev[(long)FF - (long)AA][(long)MM - (long)AA] = 2.70954e-01;
  ev[(long)PP_ - (long)AA][(long)MM - (long)AA] = 4.05413e-02;
  ev[(long)SS - (long)AA][(long)MM - (long)AA] = 5.49397e-02;
  ev[(long)TT - (long)AA][(long)MM - (long)AA] = -2.30001e-04;
  ev[(long)WW - (long)AA][(long)MM - (long)AA] = -6.60170e-02;
  ev[(long)YY - (long)AA][(long)MM - (long)AA] = 5.64557e-01;
  ev[(long)VV - (long)AA][(long)MM - (long)AA] = -2.78943e-01;

  ev[0][(long)FF - (long)AA] = 1.68903e-01;
  ev[(long)RR - (long)AA][(long)FF - (long)AA] = -4.16455e-01;
  ev[(long)NN - (long)AA][(long)FF - (long)AA] = 1.75235e-01;
  ev[(long)DD - (long)AA][(long)FF - (long)AA] = -1.57281e-01;
  ev[(long)CC - (long)AA][(long)FF - (long)AA] = -3.02415e-02;
  ev[(long)QQ - (long)AA][(long)FF - (long)AA] = -1.99945e-01;
  ev[(long)EE - (long)AA][(long)FF - (long)AA] = -2.80839e-01;
  ev[(long)GG - (long)AA][(long)FF - (long)AA] = -1.65168e-01;
  ev[(long)HH - (long)AA][(long)FF - (long)AA] = 3.84179e-01;
  ev[(long)II - (long)AA][(long)FF - (long)AA] = -1.72794e-01;
  ev[(long)LL - (long)AA][(long)FF - (long)AA] = 4.08470e-02;
  ev[(long)KK - (long)AA][(long)FF - (long)AA] = 1.09632e-01;
  ev[(long)MM - (long)AA][(long)FF - (long)AA] = 3.10366e-02;
  ev[(long)FF - (long)AA][(long)FF - (long)AA] = 1.78854e-02;
  ev[(long)PP_ - (long)AA][(long)FF - (long)AA] = -2.43651e-01;
  ev[(long)SS - (long)AA][(long)FF - (long)AA] = 2.58272e-01;
  ev[(long)TT - (long)AA][(long)FF - (long)AA] = 4.85439e-01;
  ev[(long)WW - (long)AA][(long)FF - (long)AA] = 1.87008e-02;
  ev[(long)YY - (long)AA][(long)FF - (long)AA] = -8.19317e-02;
  ev[(long)VV - (long)AA][(long)FF - (long)AA] = -1.85319e-01;

  ev[0][(long)PP_ - (long)AA] = 1.89407e-01;
  ev[(long)RR - (long)AA][(long)PP_ - (long)AA] = -5.67638e-01;
  ev[(long)NN - (long)AA][(long)PP_ - (long)AA] = -2.92067e-02;
  ev[(long)DD - (long)AA][(long)PP_ - (long)AA] = 4.63283e-02;
  ev[(long)CC - (long)AA][(long)PP_ - (long)AA] = -7.50547e-02;
  ev[(long)QQ - (long)AA][(long)PP_ - (long)AA] = -1.77709e-01;
  ev[(long)EE - (long)AA][(long)PP_ - (long)AA] = 2.11012e-02;
  ev[(long)GG - (long)AA][(long)PP_ - (long)AA] = 4.57586e-01;
  ev[(long)HH - (long)AA][(long)PP_ - (long)AA] = -2.73917e-01;
  ev[(long)II - (long)AA][(long)PP_ - (long)AA] = 3.02761e-02;
  ev[(long)LL - (long)AA][(long)PP_ - (long)AA] = -8.55620e-02;
  ev[(long)KK - (long)AA][(long)PP_ - (long)AA] = -4.68228e-01;
  ev[(long)MM - (long)AA][(long)PP_ - (long)AA] = -1.49034e-01;
  ev[(long)FF - (long)AA][(long)PP_ - (long)AA] = 4.32709e-02;
  ev[(long)PP_ - (long)AA][(long)PP_ - (long)AA] = 1.13488e-01;
  ev[(long)SS - (long)AA][(long)PP_ - (long)AA] = 1.12224e-01;
  ev[(long)TT - (long)AA][(long)PP_ - (long)AA] = 8.54433e-02;
  ev[(long)WW - (long)AA][(long)PP_ - (long)AA] = 1.49702e-01;
  ev[(long)YY - (long)AA][(long)PP_ - (long)AA] = 1.44889e-02;
  ev[(long)VV - (long)AA][(long)PP_ - (long)AA] = 9.94179e-02;

  ev[0][(long)SS - (long)AA] = -4.51742e-02;
  ev[(long)RR - (long)AA][(long)SS - (long)AA] = 2.72843e-01;
  ev[(long)NN - (long)AA][(long)SS - (long)AA] = -6.34739e-02;
  ev[(long)DD - (long)AA][(long)SS - (long)AA] = -2.99613e-01;
  ev[(long)CC - (long)AA][(long)SS - (long)AA] = -1.26065e-02;
  ev[(long)QQ - (long)AA][(long)SS - (long)AA] = 6.62398e-02;
  ev[(long)EE - (long)AA][(long)SS - (long)AA] = -2.93927e-01;
  ev[(long)GG - (long)AA][(long)SS - (long)AA] = 2.07444e-01;
  ev[(long)HH - (long)AA][(long)SS - (long)AA] = 7.61271e-01;
  ev[(long)II - (long)AA][(long)SS - (long)AA] = 1.22763e-01;
  ev[(long)LL - (long)AA][(long)SS - (long)AA] = -9.12377e-02;
  ev[(long)KK - (long)AA][(long)SS - (long)AA] = -1.67370e-01;
  ev[(long)MM - (long)AA][(long)SS - (long)AA] = -7.69871e-02;
  ev[(long)FF - (long)AA][(long)SS - (long)AA] = 3.25168e-02;
  ev[(long)PP_ - (long)AA][(long)SS - (long)AA] = -8.69178e-02;
  ev[(long)SS - (long)AA][(long)SS - (long)AA] = -4.91352e-02;
  ev[(long)TT - (long)AA][(long)SS - (long)AA] = -9.28875e-02;
  ev[(long)WW - (long)AA][(long)SS - (long)AA] = -3.40158e-02;
  ev[(long)YY - (long)AA][(long)SS - (long)AA] = -9.70949e-02;
  ev[(long)VV - (long)AA][(long)SS - (long)AA] = 1.69233e-01;

  ev[0][(long)TT - (long)AA] = -6.57152e-02;
  ev[(long)RR - (long)AA][(long)TT - (long)AA] = -2.96127e-01;
  ev[(long)NN - (long)AA][(long)TT - (long)AA] = 1.40073e-01;
  ev[(long)DD - (long)AA][(long)TT - (long)AA] = 3.67827e-01;
  ev[(long)CC - (long)AA][(long)TT - (long)AA] = 3.88665e-02;
  ev[(long)QQ - (long)AA][(long)TT - (long)AA] = 3.54579e-01;
  ev[(long)EE - (long)AA][(long)TT - (long)AA] = 3.95304e-01;
  ev[(long)GG - (long)AA][(long)TT - (long)AA] = -1.30872e-01;
  ev[(long)HH - (long)AA][(long)TT - (long)AA] = 5.22294e-01;
  ev[(long)II - (long)AA][(long)TT - (long)AA] = -9.20181e-02;
  ev[(long)LL - (long)AA][(long)TT - (long)AA] = 1.35098e-01;
  ev[(long)KK - (long)AA][(long)TT - (long)AA] = -2.61406e-01;
  ev[(long)MM - (long)AA][(long)TT - (long)AA] = -5.72654e-02;
  ev[(long)FF - (long)AA][(long)TT - (long)AA] = -1.06748e-01;
  ev[(long)PP_ - (long)AA][(long)TT - (long)AA] = -1.86932e-01;
  ev[(long)SS - (long)AA][(long)TT - (long)AA] = -8.60432e-02;
  ev[(long)TT - (long)AA][(long)TT - (long)AA] = -1.17435e-01;
  ev[(long)WW - (long)AA][(long)TT - (long)AA] = 4.21809e-02;
  ev[(long)YY - (long)AA][(long)TT - (long)AA] = 3.66170e-02;
  ev[(long)VV - (long)AA][(long)TT - (long)AA] = -1.03287e-01;

  ev[0][(long)WW - (long)AA] = 8.12248e-02;
  ev[(long)RR - (long)AA][(long)WW - (long)AA] = -5.52327e-02;
  ev[(long)NN - (long)AA][(long)WW - (long)AA] = -5.60451e-02;
  ev[(long)DD - (long)AA][(long)WW - (long)AA] = -1.10967e-01;
  ev[(long)CC - (long)AA][(long)WW - (long)AA] = -4.36007e-02;
  ev[(long)QQ - (long)AA][(long)WW - (long)AA] = 7.49285e-02;
  ev[(long)EE - (long)AA][(long)WW - (long)AA] = -6.08685e-02;
  ev[(long)GG - (long)AA][(long)WW - (long)AA] = -3.69332e-01;
  ev[(long)HH - (long)AA][(long)WW - (long)AA] = 1.94411e-01;
  ev[(long)II - (long)AA][(long)WW - (long)AA] = 3.55141e-02;
  ev[(long)LL - (long)AA][(long)WW - (long)AA] = -8.63681e-02;
  ev[(long)KK - (long)AA][(long)WW - (long)AA] = -2.09830e-01;
  ev[(long)MM - (long)AA][(long)WW - (long)AA] = -9.78879e-02;
  ev[(long)FF - (long)AA][(long)WW - (long)AA] = -7.98604e-02;
  ev[(long)PP_ - (long)AA][(long)WW - (long)AA] = 8.35838e-01;
  ev[(long)SS - (long)AA][(long)WW - (long)AA] = 4.95931e-02;
  ev[(long)TT - (long)AA][(long)WW - (long)AA] = 7.86337e-02;
  ev[(long)WW - (long)AA][(long)WW - (long)AA] = 1.01576e-02;
  ev[(long)YY - (long)AA][(long)WW - (long)AA] = 8.79878e-02;
  ev[(long)VV - (long)AA][(long)WW - (long)AA] = 7.51898e-02;

  ev[0][(long)YY - (long)AA] = -3.45102e-02;
  ev[(long)RR - (long)AA][(long)YY - (long)AA] = 6.45521e-02;
  ev[(long)NN - (long)AA][(long)YY - (long)AA] = -2.22780e-02;
  ev[(long)DD - (long)AA][(long)YY - (long)AA] = -3.19792e-02;
  ev[(long)CC - (long)AA][(long)YY - (long)AA] = 5.97900e-02;
  ev[(long)QQ - (long)AA][(long)YY - (long)AA] = 4.40846e-02;
  ev[(long)EE - (long)AA][(long)YY - (long)AA] = -2.90400e-02;
  ev[(long)GG - (long)AA][(long)YY - (long)AA] = 1.32178e-01;
  ev[(long)HH - (long)AA][(long)YY - (long)AA] = 4.83456e-02;
  ev[(long)II - (long)AA][(long)YY - (long)AA] = -3.33680e-01;
  ev[(long)LL - (long)AA][(long)YY - (long)AA] = 1.96787e-01;
  ev[(long)KK - (long)AA][(long)YY - (long)AA] = -1.27120e-02;
  ev[(long)MM - (long)AA][(long)YY - (long)AA] = -1.01360e-02;
  ev[(long)FF - (long)AA][(long)YY - (long)AA] = 5.00463e-01;
  ev[(long)PP_ - (long)AA][(long)YY - (long)AA] = 2.65762e-01;
  ev[(long)SS - (long)AA][(long)YY - (long)AA] = 8.18628e-03;
  ev[(long)TT - (long)AA][(long)YY - (long)AA] = -1.15726e-01;
  ev[(long)WW - (long)AA][(long)YY - (long)AA] = -3.46187e-02;
  ev[(long)YY - (long)AA][(long)YY - (long)AA] = -5.64673e-01;
  ev[(long)VV - (long)AA][(long)YY - (long)AA] = -4.02511e-01;

  ev[0][(long)VV - (long)AA] = -2.37427e-02;
  ev[(long)RR - (long)AA][(long)VV - (long)AA] = 6.64165e-02;
  ev[(long)NN - (long)AA][(long)VV - (long)AA] = -3.07931e-02;
  ev[(long)DD - (long)AA][(long)VV - (long)AA] = -1.35114e-01;
  ev[(long)CC - (long)AA][(long)VV - (long)AA] = -1.26575e-02;
  ev[(long)QQ - (long)AA][(long)VV - (long)AA] = -4.34887e-02;
  ev[(long)EE - (long)AA][(long)VV - (long)AA] = -1.42949e-01;
  ev[(long)GG - (long)AA][(long)VV - (long)AA] = 1.85463e-01;
  ev[(long)HH - (long)AA][(long)VV - (long)AA] = 4.82054e-02;
  ev[(long)II - (long)AA][(long)VV - (long)AA] = -2.88402e-01;
  ev[(long)LL - (long)AA][(long)VV - (long)AA] = 2.93123e-01;
  ev[(long)KK - (long)AA][(long)VV - (long)AA] = 1.32034e-02;
  ev[(long)MM - (long)AA][(long)VV - (long)AA] = 6.77690e-02;
  ev[(long)FF - (long)AA][(long)VV - (long)AA] = -5.43584e-01;
  ev[(long)PP_ - (long)AA][(long)VV - (long)AA] = 1.46520e-01;
  ev[(long)SS - (long)AA][(long)VV - (long)AA] = 3.28990e-03;
  ev[(long)TT - (long)AA][(long)VV - (long)AA] = -7.67072e-02;
  ev[(long)WW - (long)AA][(long)VV - (long)AA] = -2.03106e-02;
  ev[(long)YY - (long)AA][(long)VV - (long)AA] = 5.89467e-01;
  ev[(long)VV - (long)AA][(long)VV - (long)AA] = -2.68367e-01;


  eval[0] = -2.03036e-02;
  eval[(long)RR - (long)AA] = -2.33761e-02;
  eval[(long)NN - (long)AA] = -1.71812e-02;
  eval[(long)DD - (long)AA] = -1.82705e-02;
  eval[(long)CC - (long)AA] = -1.55431e-15;
  eval[(long)QQ - (long)AA] = -1.60581e-02;
  eval[(long)EE - (long)AA] = -1.34008e-02;
  eval[(long)GG - (long)AA] = -1.38363e-02;
  eval[(long)HH - (long)AA] = -2.36636e-03;
  eval[(long)II - (long)AA] = -2.68394e-03;
  eval[(long)LL - (long)AA] = -2.91392e-03;
  eval[(long)KK - (long)AA] = -1.13524e-02;
  eval[(long)MM - (long)AA] = -4.34547e-03;
  eval[(long)FF - (long)AA] = -1.06999e-02;
  eval[(long)PP_ - (long)AA] = -5.48466e-03;
  eval[(long)SS - (long)AA] = -8.98371e-03;
  eval[(long)TT - (long)AA] = -7.23244e-03;
  eval[(long)WW - (long)AA] = -7.37540e-03;
  eval[(long)YY - (long)AA] = -7.91291e-03;
  eval[(long)VV - (long)AA] = -8.24441e-03;


  freqdyhf[0] = 0.8712669e-01;
  freqdyhf[(long)RR - (long)AA] = 0.4090396e-01;
  freqdyhf[(long)NN - (long)AA] = 0.4043229e-01;
  freqdyhf[(long)DD - (long)AA] = 0.4687196e-01;
  freqdyhf[(long)CC - (long)AA] = 0.3347348e-01;
  freqdyhf[(long)QQ - (long)AA] = 0.3825543e-01;
  freqdyhf[(long)EE - (long)AA] = 0.4953036e-01;
  freqdyhf[(long)GG - (long)AA] = 0.8861207e-01;
  freqdyhf[(long)HH - (long)AA] = 0.3361838e-01;
  freqdyhf[(long)II - (long)AA] = 0.3688570e-01;
  freqdyhf[(long)LL - (long)AA] = 0.8535736e-01;
  freqdyhf[(long)KK - (long)AA] = 0.8048168e-01;
  freqdyhf[(long)MM - (long)AA] = 0.1475275e-01;
  freqdyhf[(long)FF - (long)AA] = 0.3977166e-01;
  freqdyhf[(long)PP_ - (long)AA] = 0.5067984e-01;
  freqdyhf[(long)SS - (long)AA] = 0.6957710e-01;
  freqdyhf[(long)TT - (long)AA] = 0.5854172e-01;
  freqdyhf[(long)WW - (long)AA] = 0.1049367e-01;
  freqdyhf[(long)YY - (long)AA] = 0.2991627e-01;
  freqdyhf[(long)VV - (long)AA] = 0.6471762e-01;
}  /* dataeigenv */


/*** TRANSITION PROBABILITY MATRIX ***/
Static Void tprobmtrx(arc, tpr)
double arc;
aryamity *tpr;
{
  /* transition probability matrix */
  double sum;
  amity iba, jba, kba;
  aryamity exparc;

  /* negative : BOOLEAN; */
  /* negative := FALSE; */
  for (kba = AA; (long)kba <= (long)VV; kba = (amity)((long)kba + 1))
    exparc[(long)kba - (long)AA] = exp(arc * eval[(long)kba - (long)AA]);
  for (iba = AA; (long)iba <= (long)VV; iba = (amity)((long)iba + 1)) {
    for (jba = AA; (long)jba <= (long)VV; jba = (amity)((long)jba + 1)) {
      sum = coefp[(long)iba - (long)AA][(long)jba - (long)AA][0] * exparc[0] +
	  coefp[(long)iba - (long)AA][(long)jba - (long)AA][(long)RR - (long)
		  AA] * exparc[(long)RR - (long)AA] + coefp[(long)iba - (long)
		  AA][(long)jba - (long)AA][(long)NN - (long)AA] *
	    exparc[(long)NN - (long)AA] + coefp[(long)iba - (long)AA][(long)
		  jba - (long)AA][(long)DD - (long)AA] * exparc[(long)DD -
		(long)AA] + coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)CC - (long)AA] * exparc[(long)CC - (long)AA] + coefp[(long)
		  iba - (long)AA][(long)jba - (long)AA][(long)QQ - (long)AA] *
	    exparc[(long)QQ - (long)AA] + coefp[(long)iba - (long)AA][(long)
		  jba - (long)AA][(long)EE - (long)AA] * exparc[(long)EE -
		(long)AA] + coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)GG - (long)AA] * exparc[(long)GG - (long)AA] + coefp[(long)
		  iba - (long)AA][(long)jba - (long)AA][(long)HH - (long)AA] *
	    exparc[(long)HH - (long)AA] + coefp[(long)iba - (long)AA][(long)
		  jba - (long)AA][(long)II - (long)AA] * exparc[(long)II -
		(long)AA] + coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)LL - (long)AA] * exparc[(long)LL - (long)AA] + coefp[(long)
		  iba - (long)AA][(long)jba - (long)AA][(long)KK - (long)AA] *
	    exparc[(long)KK - (long)AA] + coefp[(long)iba - (long)AA][(long)
		  jba - (long)AA][(long)MM - (long)AA] * exparc[(long)MM -
		(long)AA] + coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)FF - (long)AA] * exparc[(long)FF - (long)AA] + coefp[(long)
		  iba - (long)AA][(long)jba - (long)AA]
	    [(long)PP_ - (long)AA] * exparc[(long)PP_ - (long)AA] +
	  coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)SS - (long)AA] * exparc[(long)SS - (long)AA] +
	  coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)TT - (long)AA] * exparc[(long)TT - (long)AA] +
	  coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)WW - (long)AA] * exparc[(long)WW - (long)AA] +
	  coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)YY - (long)AA] * exparc[(long)YY - (long)AA] +
	  coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)VV - (long)AA] * exparc[(long)VV - (long)AA];
/* p2c: protml.pas, line 1462: Note:
 * Line breaker spent 0.0+1.00 seconds, 5000 tries on line 1898 [251] */
      if (sum < minreal)   /* negative := TRUE; */
	tpr[(long)iba - (long)AA][(long)jba - (long)AA] = 0.0;
	    /* attention */
      else
	tpr[(long)iba - (long)AA][(long)jba - (long)AA] = sum;
    }
  }
  /* IF negative THEN
     IF debugoptn THEN
     BEGIN
        write(output,' Trans. prob. 1 is negative !');
        writeln(output,'  arc =',arc:10:5);
     END; */
}  /* tprobmtrx */


/*** TRANSITION PROBABILITY AND DIFFRENCE MATRIX ***/
Static Void tdiffmtrx(arc, tpr, td1, td2)
double arc;
aryamity *tpr, *td1, *td2;
{
  /* transition probability matrix */
  double sum, sumd1, sumd2, aaa, rrr, nnn, ddd, ccc, qqq, eee, ggg, hhh, iii,
	 lll, kkk, mmm, fff, ppp, sss, ttt, www, yyy, vvv;
  amity iba, jba, kba;
  aryamity exparc;

  for (kba = AA; (long)kba <= (long)VV; kba = (amity)((long)kba + 1))
    exparc[(long)kba - (long)AA] = exp(arc * eval[(long)kba - (long)AA]);
  for (iba = AA; (long)iba <= (long)VV; iba = (amity)((long)iba + 1)) {
    for (jba = AA; (long)jba <= (long)VV; jba = (amity)((long)jba + 1)) {
      aaa = coefp[(long)iba - (long)AA][(long)jba - (long)AA][0] * exparc[0];
      rrr = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)RR - (long)AA] * exparc[(long)RR - (long)AA];
      nnn = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)NN - (long)AA] * exparc[(long)NN - (long)AA];
      ddd = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)DD - (long)AA] * exparc[(long)DD - (long)AA];
      ccc = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)CC - (long)AA] * exparc[(long)CC - (long)AA];
      qqq = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)QQ - (long)AA] * exparc[(long)QQ - (long)AA];
      eee = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)EE - (long)AA] * exparc[(long)EE - (long)AA];
      ggg = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)GG - (long)AA] * exparc[(long)GG - (long)AA];
      hhh = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)HH - (long)AA] * exparc[(long)HH - (long)AA];
      iii = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)II - (long)AA] * exparc[(long)II - (long)AA];
      lll = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)LL - (long)AA] * exparc[(long)LL - (long)AA];
      kkk = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)KK - (long)AA] * exparc[(long)KK - (long)AA];
      mmm = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)MM - (long)AA] * exparc[(long)MM - (long)AA];
      fff = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)FF - (long)AA] * exparc[(long)FF - (long)AA];
      ppp = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)PP_ - (long)AA] * exparc[(long)PP_ - (long)AA];
      sss = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)SS - (long)AA] * exparc[(long)SS - (long)AA];
      ttt = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)TT - (long)AA] * exparc[(long)TT - (long)AA];
      www = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)WW - (long)AA] * exparc[(long)WW - (long)AA];
      yyy = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)YY - (long)AA] * exparc[(long)YY - (long)AA];
      vvv = coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	    [(long)VV - (long)AA] * exparc[(long)VV - (long)AA];

      sum = aaa + rrr + nnn + ddd + ccc + qqq + eee + ggg + hhh + iii + lll +
	    kkk + mmm + fff + ppp + sss + ttt + www + yyy + vvv;
      sumd1 = aaa * eval[0] + rrr * eval[(long)RR - (long)AA] +
	  nnn * eval[(long)NN - (long)AA] + ddd * eval[(long)DD - (long)AA] +
	  ccc * eval[(long)CC - (long)AA] + qqq * eval[(long)QQ - (long)AA] +
	  eee * eval[(long)EE - (long)AA] + ggg * eval[(long)GG - (long)AA] +
	  hhh * eval[(long)HH - (long)AA] + iii * eval[(long)II - (long)AA] +
	  lll * eval[(long)LL - (long)AA] + kkk * eval[(long)KK - (long)AA] +
	  mmm * eval[(long)MM - (long)AA] + fff * eval[(long)FF - (long)AA] +
	  ppp * eval[(long)PP_ - (long)AA] + sss * eval[(long)SS - (long)AA] +
	  ttt * eval[(long)TT - (long)AA] + www * eval[(long)WW - (long)AA] +
	  yyy * eval[(long)YY - (long)AA] + vvv * eval[(long)VV - (long)AA];
/* p2c: protml.pas, line 1542: Note:
 * Line breaker spent 0.0+1.00 seconds, 5000 tries on line 1983 [251] */
      sumd2 = aaa * eval2[0] + rrr * eval2[(long)RR - (long)AA] +
	  nnn * eval2[(long)NN - (long)AA] +
	  ddd * eval2[(long)DD - (long)AA] +
	  ccc * eval2[(long)CC - (long)AA] +
	  qqq * eval2[(long)QQ - (long)AA] +
	  eee * eval2[(long)EE - (long)AA] +
	  ggg * eval2[(long)GG - (long)AA] +
	  hhh * eval2[(long)HH - (long)AA] +
	  iii * eval2[(long)II - (long)AA] +
	  lll * eval2[(long)LL - (long)AA] +
	  kkk * eval2[(long)KK - (long)AA] +
	  mmm * eval2[(long)MM - (long)AA] +
	  fff * eval2[(long)FF - (long)AA] +
	  ppp * eval2[(long)PP_ - (long)AA] +
	  sss * eval2[(long)SS - (long)AA] +
	  ttt * eval2[(long)TT - (long)AA] +
	  www * eval2[(long)WW - (long)AA] +
	  yyy * eval2[(long)YY - (long)AA] + vvv * eval2[(long)VV - (long)AA];
/* p2c: protml.pas, line 1542: 
 * Note: Line breaker spent 0.0 seconds, 5000 tries on line 2003 [251] */
      if (sum < minreal) {   /* attention */
	tpr[(long)iba - (long)AA][(long)jba - (long)AA] = 0.0;
	td1[(long)iba - (long)AA][(long)jba - (long)AA] = 0.0;
	td2[(long)iba - (long)AA][(long)jba - (long)AA] = 0.0;
      } else {
	tpr[(long)iba - (long)AA][(long)jba - (long)AA] = sum;
	td1[(long)iba - (long)AA][(long)jba - (long)AA] = sumd1;
	td2[(long)iba - (long)AA][(long)jba - (long)AA] = sumd2;
      }
    }
  }
  /* IF negative THEN
     IF debugoptn THEN
     BEGIN
        write(output,' Trans. prob. 2 is negative !');
        writeln(output,'  arc =',arc:10:5);
     END; */
}  /* tdiffmtrx */


/*** MAKE TRANSITION PROBABILITY MATRIX ***/
Static Void maketransprob()
{
  /* make transition probability matrix */
  ndaryamity amatrix, bmatrix, cmatrix;
  amity iba, jba, kba;

  if (readtoptn)
    readeigenv();
  else
    dataeigenv();
  if (writeoptn)
    printfreqdyhf();

  if (readtoptn)
    printf("read trans. matrix\n");
  if (debugoptn)
    printeigen();

  convermtrx(ev, amatrix);
  memcpy(cmatrix, amatrix, sizeof(ndaryamity));
  luinverse(cmatrix, bmatrix, 20L);
  /* IF debugoptn printnmtrx (bmatrix); */
  mproduct(amatrix, bmatrix, cmatrix, 20L, 20L, 20L);
  checkevector(cmatrix, 20L);
  /* IF debugoptn printnmtrx (cmatrix); */
  reconvermtrx(bmatrix, iev);
  for (iba = AA; (long)iba <= (long)VV; iba = (amity)((long)iba + 1)) {
    for (jba = AA; (long)jba <= (long)VV; jba = (amity)((long)jba + 1)) {
      for (kba = AA; (long)kba <= (long)VV; kba = (amity)((long)kba + 1))
	coefp[(long)iba - (long)AA][(long)jba - (long)AA]
	  [(long)kba - (long)AA] = ev[(long)iba - (long)AA]
	    [(long)kba - (long)AA] * iev[(long)kba - (long)AA]
	    [(long)jba - (long)AA];
    }
  }
  for (kba = AA; (long)kba <= (long)VV; kba = (amity)((long)kba + 1))
    eval2[(long)kba - (long)AA] = eval[(long)kba - (long)AA] *
				  eval[(long)kba - (long)AA];
  /*
     tprobmtrx ( 100.0, tprobt );
     printamtrx ( tprobt );
  */
}  /* maketransprob */


Static double randum(seed)
long *seed;
{
  /* random number generator -- slow but machine independent */
  long i, j, k, sum;
  longintty mult, newseed;
  double x;

  mult[0] = 13;
  mult[1] = 24;
  mult[2] = 22;
  mult[3] = 6;
  for (i = 0; i <= 5; i++)
    newseed[i] = 0;
  for (i = 0; i <= 5; i++) {
    sum = newseed[i];
    k = i;
    if (i > 3)
      k = 3;
    for (j = 0; j <= k; j++)
      sum += mult[j] * seed[i - j];
    newseed[i] = sum;
    for (j = i; j <= 4; j++) {
      newseed[j + 1] += newseed[j] / 64;
      newseed[j] &= 63;
    }
  }
  memcpy(seed, newseed, sizeof(longintty));
  seed[5] &= 3;
  x = 0.0;
  for (i = 0; i <= 5; i++)
    x = x / 64.0 + seed[i];
  x /= 4.0;
  return x;
}  /* randum */


/*******************************************************/
/*****          ESTIMATE TREE TOPOLOGY             *****/
/*******************************************************/

/**************************************/
/***       READ TREE STRUCTURE      ***/
/**************************************/

/*** SERCH OF END CHARACTER ***/
Static Void serchchend()
{
  if (chs == ',' || chs == ')')
    return;
  do {
    if (P_eoln(seqfile)) {
      fscanf(seqfile, "%*[^\n]");
      getc(seqfile);
    }
    chs = getc(seqfile);
    if (chs == '\n')
      chs = ' ';
  } while (chs != ',' && chs != ')');
}  /* serchchend */


/*** NEXT CHARACTER ***/
Static Void nextchar(ch)
Char *ch;
{
  do {
    if (P_eoln(seqfile)) {
      fscanf(seqfile, "%*[^\n]");
      getc(seqfile);
    }
    *ch = getc(seqfile);
    if (*ch == '\n')
      *ch = ' ';
  } while (*ch == ' ');
}  /* nextchar */


/*** CREATE DATASTRUCTURE OF NODE ***/
Static Void clearnode(p)
node *p;
{
  long i, FORLIM;

  p->isop = NULL;
  p->kinp = NULL;
  p->diverg = 1;
  p->number = 0;
  for (i = 0; i < maxname; i++)
    p->namesp[i] = ' ';
  FORLIM = numsp;
  for (i = 0; i < FORLIM; i++)
    p->descen[i] = 0;
  p->nadata = ami;
  p->UU.U0.lnga = 0.0;
}  /* clearnode */


/*** JUDGMENT SPEICIES NAME ***/
Static Void judgspname(tr, num)
tree *tr;
long *num;
{
  long ie;   /* number of name strings */
  namety namestr;   /* current species name of seqfile */
  boolean found;

  for (ie = 0; ie < maxname; ie++)
    namestr[ie] = ' ';
  ie = 1;
  do {
    if (chs == '_')
      chs = ' ';
    namestr[ie - 1] = chs;
    if (P_eoln(seqfile)) {
      fscanf(seqfile, "%*[^\n]");
      getc(seqfile);
    }
    chs = getc(seqfile);
    if (chs == '\n')
      chs = ' ';
    ie++;
  } while (chs != ':' && chs != ',' && chs != ')' && ie <= maxname);
  *num = 1;
  do {
    found = true;
    for (ie = 0; ie < maxname; ie++)
      found = (found && namestr[ie] == tr->brnchp[*num]->namesp[ie]);
    if (!found)
      (*num)++;
  } while (!(*num > numsp || found));
  if (*num <= numsp)
    return;
  printf(" Cannot find species: ");
  for (ie = 0; ie < maxname; ie++)
    putchar(namestr[ie]);
  putchar('\n');
}  /* judgspname */


/*** ADD EXTERNAL NODE ***/
Static Void externalnode(tr, up)
tree *tr;
node *up;
{
  long num;   /* number of external nodes */

  judgspname(tr, &num);
  tr->brnchp[num]->kinp = up;
  up->kinp = tr->brnchp[num];
  if (tr->startp->number > num)
    tr->startp = tr->brnchp[num];
  tr->brnchp[num]->UU.U0.lnga = 0.0;
  up->UU.U0.lnga = 0.0;
}  /* externalnode */


/*** ADD INTERNAL NODE ***/
Static Void internalnode(tr, up, ninode)
tree *tr;
node *up;
long *ninode;
{
  node *np;   /* new pointer to internal node */
  node *cp;   /* current pointer to internal node */
  long i, nd, dvg;

  nextchar(&chs);
  if (chs != '(') {
    externalnode(tr, up);
    return;
  }
  (*ninode)++;
  nd = *ninode;
  if (freenode == NULL)
    np = (node *)Malloc(sizeof(node));
  else {
    np = freenode;
    freenode = np->isop;
    np->isop = NULL;
  }
  clearnode(np);
  np->isop = np;
  cp = np;
  np = NULL;
  cp->number = nd;
  tr->brnchp[*ninode] = cp;
  up->kinp = cp;
  cp->kinp = up;
  dvg = 0;
  while (chs != ')') {
    dvg++;
    if (freenode == NULL)
      np = (node *)Malloc(sizeof(node));
    else {
      np = freenode;
      freenode = np->isop;
      np->isop = NULL;
    }
    clearnode(np);
    np->isop = cp->isop;
    cp->isop = np;
    cp = np;
    np = NULL;
    cp->number = nd;
    internalnode(tr, cp, ninode);
    serchchend();
  }
  for (i = 0; i <= dvg; i++) {
    cp->diverg = dvg;
    cp = cp->isop;
  }
  nextchar(&chs);   /* */
}  /* internalnode */


/*** MAKE STRUCTURE OF TREE ***/
Static Void treestructure(tr)
tree *tr;
{
  long i, dvg, ninode;   /* number of internal nodes */
  node *np;   /* new pointer */
  node *cp;   /* current pointer to zero node(root) */

  ninode = numsp;
  nextchar(&chs);
  if (chs == '(') {
    dvg = -1;
    while (chs != ')') {
      if (freenode == NULL)
	np = (node *)Malloc(sizeof(node));
      else {
	np = freenode;
	freenode = np->isop;
	np->isop = NULL;
      }
      clearnode(np);
      internalnode(tr, np, &ninode);
      dvg++;
      if (dvg == 0)
	np->isop = np;
      else {
	np->isop = cp->isop;
	cp->isop = np;
      }
      cp = np;
      np = NULL;
      serchchend();
    }
    tr->brnchp[0] = cp->isop;
    tr->startp = tr->brnchp[numsp];
    for (i = 0; i <= dvg; i++) {
      cp->diverg = dvg;
      cp = cp->isop;
    }
  }
  fscanf(seqfile, "%*[^\n]");
  getc(seqfile);
  ibrnch2 = ninode;
}  /* treestructure */


/*** MAKE STAR STRUCTURE OF TREE ***/
Static Void starstructure(tr)
tree *tr;
{
  long i;
  /* */
  long dvg;
  node *np;   /* new pointer */
  node *cp;   /* current pointer to zero node(root) */
  long FORLIM;

  dvg = -1;
  FORLIM = numsp;
  for (i = 1; i <= FORLIM; i++) {
    if (freenode == NULL)
      np = (node *)Malloc(sizeof(node));
    else {
      np = freenode;
      freenode = np->isop;
      np->isop = NULL;
    }
    clearnode(np);

    tr->brnchp[i]->kinp = np;
    np->kinp = tr->brnchp[i];

    dvg++;
    if (dvg == 0)
      np->isop = np;
    else {
      np->isop = cp->isop;
      cp->isop = np;
    }
    cp = np;
    np = NULL;
  }
  tr->brnchp[0] = cp->isop;
  tr->startp = tr->brnchp[numsp];
  for (i = 0; i <= dvg; i++) {
    cp->diverg = dvg;
    cp = cp->isop;
  }
  ibrnch2 = numsp;
}  /* starstructure */


/* INSERT BRANCH IN TREE */
Static Void insertbranch(tr, cp1, cp2, bp1, bp2, np1, np2)
tree *tr;
node *cp1, *cp2, *bp1, *bp2, *np1, *np2;
{
  long i, num, dvg;
  node *ap;

  i = cp1->number;
  if (cp1 == tr->brnchp[i]) {
    if (i != 0) {
      ap = np1;
      np1 = np2;
      np2 = ap;
    } else
      tr->brnchp[i] = np2;
  }
  /*  np2^.number := bp1^.number;
      cp1^.number := np1^.number;
      cp2^.number := np1^.number;  */
  bp1->isop = np2;
  if (cp1 == bp2)
    np2->isop = cp2->isop;
  else
    np2->isop = cp1->isop;
  if (cp1 != bp2)
    bp2->isop = cp2->isop;
  np1->isop = cp1;
  if (cp1 != bp2)
    cp1->isop = cp2;
  cp2->isop = np1;
  tr->brnchp[ibrnch2]->kinp->number = tr->brnchp[ibrnch2]->kinp->isop->number;
  ap = tr->brnchp[ibrnch2];
  num = tr->brnchp[ibrnch2]->number;
  do {
    ap = ap->isop;
    ap->number = num;
  } while (ap->isop != tr->brnchp[ibrnch2]);
  dvg = 0;
  ap = np1;
  do {
    ap = ap->isop;
    dvg++;
  } while (ap->isop != np1);
  ap = np1;
  do {
    ap = ap->isop;
    ap->diverg = dvg;
  } while (ap != np1);
  dvg = 0;
  ap = np2;
  do {
    ap = ap->isop;
    dvg++;
  } while (ap->isop != np2);
  ap = np2;
  do {
    ap = ap->isop;
    ap->diverg = dvg;
  } while (ap != np2);
}  /* insertbranch */


/* DELETE BRANCH IN TREE */
Static Void deletebranch(tr, cnode, cp1, cp2, bp1, bp2, np1, np2)
tree *tr;
long cnode;
node *cp1, *cp2, *bp1, *bp2, *np1, *np2;
{
  /* i, */
  long dvg;
  node *ap;

  if (cnode != 0) {
    if (tr->brnchp[cnode] == cp1) {
      ap = np1;
      np1 = np2;
      np2 = ap;
    }
  } else {
    if (tr->brnchp[cnode] == np2)
      tr->brnchp[cnode] = cp1;
  }
  /* i := np2^.number;
     IF i = 0 THEN
        IF np2 = tr.brnchp[i] THEN tr.brnchp[i] := cp1
     ELSE
        IF cp1 = tr.brnchp[i] THEN
        BEGIN
           ap  := np1;
           np1 := np2;
           np2 := ap;
        END; */
  /* cp1^.number := np2^.number;
     cp2^.number := np2^.number; */
  if (cp1 == bp2)
    cp2->isop = np2->isop;
  else
    cp2->isop = bp2->isop;
  if (cp1 != bp2)
    cp1->isop = np2->isop;
  np1->isop = NULL;
  if (cp1 != bp2)
    bp2->isop = cp2;
  np2->isop = NULL;
  bp1->isop = cp1;
  dvg = 0;
  ap = cp1;
  do {
    ap = ap->isop;
    dvg++;
  } while (ap->isop != cp1);
  ap = cp1;
  do {
    ap = ap->isop;
    ap->diverg = dvg;
    ap->number = cnode;
  } while (ap != cp1);
  np1->diverg = 0;
  np2->diverg = 0;
}  /* deletebranch */


/*** PRINT STRUCTURE OF TREE ***/
Static Void printcurtree(tr)
tree *tr;
{
  node *ap;
  long i, j, k, num, FORLIM, FORLIM1;

  printf("\nStructure of Tree\n");
  printf("%7s%5s%5s%8s%9s%11s%13s\n",
	 "number", "kinp", "isop", "diverg", "namesp", "length",
	 "descendant");
  FORLIM = numsp;
  for (i = 1; i <= FORLIM; i++) {
    ap = tr->brnchp[i];
    printf("%5ld", ap->number);
    printf("%6ld", ap->kinp->number);
    if (ap->isop == NULL)
      printf("%6s", "nil");
    else
      printf("%6ld", ap->isop->number);
    printf("%7ld", ap->diverg);
    printf("%3c", ' ');
    for (j = 0; j < maxname; j++)
      putchar(ap->namesp[j]);
    printf("%8.3f", ap->UU.U0.lnga);
    printf("%3c", ' ');
    FORLIM1 = numsp;
    for (j = 0; j < FORLIM1; j++)
      printf("%2ld", ap->descen[j]);
    putchar('\n');
  }
  FORLIM = ibrnch2 + 1;
  for (i = ibrnch1; i <= FORLIM; i++) {
    if (i == ibrnch2 + 1)
      num = 0;
    else
      num = i;
    k = 0;
    ap = tr->brnchp[num];
    do {
      k++;
      if (ap->number == tr->brnchp[num]->number && k > 1)
	printf("%5c", '.');
      else
	printf("%5ld", ap->number);
      printf("%6ld", ap->kinp->number);
      if (ap->isop->number == tr->brnchp[num]->number && k > 0)   /* 1 */
	printf("%6c", '.');
      else
	printf("%6ld", ap->isop->number);
      printf("%7ld", ap->diverg);
      printf("%3c", ' ');
      for (j = 1; j <= maxname; j++)
	putchar(' ');
      printf("%8.3f", ap->UU.U0.lnga);
      printf("%3c", ' ');
      FORLIM1 = numsp;
      for (j = 0; j < FORLIM1; j++)
	printf("%2ld", ap->descen[j]);
      putchar('\n');
      ap = ap->isop;
    } while (ap != tr->brnchp[num]);
  }
}  /* printcurtree */


/**************************************/
/***  ESTIMATE LIKELIHOOD OF TREE   ***/
/**************************************/

Static Void initdescen(tr)
tree *tr;
{
  node *ap;
  long i, j, k, num, FORLIM, FORLIM1, FORLIM2;

  FORLIM = numsp;
  for (i = 1; i <= FORLIM; i++) {
    ap = tr->brnchp[i];
    FORLIM1 = numsp;
    for (j = 0; j < FORLIM1; j++)
      ap->descen[j] = 0;
    ap->descen[i - 1] = 1;
  }
  FORLIM = ibrnch2 + 1;
  for (i = ibrnch1; i <= FORLIM; i++) {
    if (i == ibrnch2 + 1)
      num = 0;
    else
      num = i;
    ap = tr->brnchp[num];
    FORLIM1 = tr->brnchp[num]->diverg + 1;
    for (k = 1; k <= FORLIM1; k++) {
      FORLIM2 = numsp;
      for (j = 0; j < FORLIM2; j++)
	ap->descen[j] = 0;
      ap = ap->isop;
    }
  }
}  /* initdescen */


Static Void initnodeA(p)
node *p;
{
  node *cp;
  long i, n;
  double sumprb;
  amity ba;
  long FORLIM, FORLIM2;

  if (p->isop == NULL)   /* TIP */
    return;
  cp = p->isop;
  FORLIM = p->diverg;
  for (n = 1; n <= FORLIM; n++) {
    initnodeA(cp->kinp);
    cp = cp->isop;
  }
  FORLIM = endptrn;
  for (i = 0; i < FORLIM; i++) {
    for (ba = AA; (long)ba <= (long)VV; ba = (amity)((long)ba + 1)) {
      sumprb = 0.0;
      cp = p->isop;
      FORLIM2 = p->diverg;
      for (n = 1; n <= FORLIM2; n++) {
	sumprb += cp->kinp->UU.U0.prba[i][(long)ba - (long)AA];
	cp = cp->isop;
      }
      if (sumprb != 0.0)
	p->UU.U0.prba[i][(long)ba - (long)AA] = sumprb / p->diverg;
      else
	p->UU.U0.prba[i][(long)ba - (long)AA] = 0.0;
    }
  }
  p->UU.U0.lnga = 10.0;   /* attention */
  p->kinp->UU.U0.lnga = 10.0;   /* attention */
  FORLIM = numsp;
  for (i = 0; i < FORLIM; i++) {
    cp = p->isop;
    FORLIM2 = p->diverg;
    for (n = 1; n <= FORLIM2; n++) {
      if (cp->kinp->descen[i] > 0)
	p->descen[i] = 1;
      cp = cp->isop;
    }
  }
  /*  IF debugoptn THEN
      BEGIN
         write(output,p^.kinp^.number:5,'-':2,p^.number:3,' ':3);
         '(':2,q^.number:3,r^.number:3,')':2,
         FOR i := 1 TO numsp DO  write(output,p^.descen[i]:2);
         writeln(output);
      END;  */
}  /* initnodeA */


Static Void initbranch(p)
node *p;
{
  long n;
  node *cp;
  long FORLIM;

  if (p->isop == NULL) {   /* TIP */
    initnodeA(p->kinp);
    return;
  }
  cp = p->isop;
  FORLIM = p->diverg;
  for (n = 1; n <= FORLIM; n++) {
    initbranch(cp->kinp);
    cp = cp->isop;
  }
}  /* initbranch */


Static Void evaluateA(tr)
tree *tr;
{
  double arc, lkl, sum, prod, lnlkl;
  long i;
  node *p, *q;
  aryamity xa1, xa2;
  amity ai, aj;
  daryamity tprobe;
  long FORLIM;

  p = tr->startp;
  q = p->kinp;
  arc = p->UU.U0.lnga;
  if (arc < minarc)
    arc = minarc;
  tprobmtrx(arc, tprobe);
  lkl = 0.0;
  FORLIM = endptrn;
  for (i = 0; i < FORLIM; i++) {
    memcpy(xa1, p->UU.U0.prba[i], sizeof(aryamity));
    memcpy(xa2, q->UU.U0.prba[i], sizeof(aryamity));
    sum = 0.0;
    for (ai = AA; (long)ai <= (long)VV; ai = (amity)((long)ai + 1)) {
      prod = freqdyhf[(long)ai - (long)AA] * xa1[(long)ai - (long)AA];
      for (aj = AA; (long)aj <= (long)VV; aj = (amity)((long)aj + 1))
	sum += prod * tprobe[(long)ai - (long)AA]
	       [(long)aj - (long)AA] * xa2[(long)aj - (long)AA];
    }
    if (sum > 0.0)
      lnlkl = log(sum);
    else
      lnlkl = 0.0;
    lkl += lnlkl * weight[i];
    lklhdtrpt[notree][i] = lnlkl;
  }
  tr->lklhd = lkl;
  tr->aic = ibrnch2 * 2 - 2.0 * lkl;
}  /* evaluateA */


Static Void sublklhdA(p)
node *p;
{
  long i, n;
  double arc;
  node *cp, *sp;
  amity ai;
  daryamity tprob;
  long FORLIM, FORLIM1;

  cp = p->isop;
  FORLIM = p->diverg;
  for (n = 1; n <= FORLIM; n++) {
    sp = cp->kinp;
    arc = sp->UU.U0.lnga;
    tprobmtrx(arc, tprob);
    FORLIM1 = endptrn;
    for (i = 0; i < FORLIM1; i++) {
      for (ai = AA; (long)ai <= (long)VV; ai = (amity)((long)ai + 1)) {
	if (n == 1)
	  p->UU.U0.prba[i][(long)ai - (long)AA] = 1.0;
	if (p->UU.U0.prba[i][(long)ai - (long)AA] < minreal)
	  p->UU.U0.prba[i][(long)ai - (long)AA] = 0.0;
	else  /* attention */
	  p->UU.U0.prba[i][(long)ai - (long)AA] *= tprob[(long)ai - (long)AA]
		[0] * sp->UU.U0.prba[i][0] + tprob[(long)ai - (long)AA][(long)
		      RR - (long)AA] * sp->UU.U0.prba[i]
		[(long)RR - (long)AA] + tprob[(long)ai - (long)AA][(long)NN -
		    (long)AA] * sp->UU.U0.prba[i][(long)NN - (long)AA] +
	      tprob[(long)ai - (long)AA][(long)DD - (long)AA] *
		sp->UU.U0.prba[i][(long)DD - (long)AA] + tprob[(long)ai -
		    (long)AA][(long)CC - (long)AA] * sp->UU.U0.prba[i][(long)
		      CC - (long)AA] + tprob[(long)ai - (long)AA][(long)QQ -
		    (long)AA] * sp->UU.U0.prba[i][(long)QQ - (long)AA] +
	      tprob[(long)ai - (long)AA][(long)EE - (long)AA] *
		sp->UU.U0.prba[i][(long)EE - (long)AA] + tprob[(long)ai -
		    (long)AA][(long)GG - (long)AA] * sp->UU.U0.prba[i][(long)
		      GG - (long)AA] + tprob[(long)ai - (long)AA][(long)HH -
		    (long)AA] * sp->UU.U0.prba[i][(long)HH - (long)AA] +
	      tprob[(long)ai - (long)AA][(long)II - (long)AA] *
		sp->UU.U0.prba[i][(long)II - (long)AA] + tprob[(long)ai -
		    (long)AA][(long)LL - (long)AA] * sp->UU.U0.prba[i][(long)
		      LL - (long)AA] + tprob[(long)ai - (long)AA][(long)KK -
		    (long)AA] * sp->UU.U0.prba[i][(long)KK - (long)AA] +
	      tprob[(long)ai - (long)AA][(long)MM - (long)AA] *
		sp->UU.U0.prba[i][(long)MM - (long)AA] +
	      tprob[(long)ai - (long)AA][(long)FF - (long)AA] *
		sp->UU.U0.prba[i]
		[(long)FF - (long)AA] + tprob[(long)ai - (long)AA]
		[(long)PP_ - (long)AA] * sp->UU.U0.prba[i]
		[(long)PP_ - (long)AA] + tprob[(long)ai - (long)AA]
		[(long)SS - (long)AA] * sp->UU.U0.prba[i]
		[(long)SS - (long)AA] + tprob[(long)ai - (long)AA]
		[(long)TT - (long)AA] * sp->UU.U0.prba[i]
		[(long)TT - (long)AA] + tprob[(long)ai - (long)AA]
		[(long)WW - (long)AA] * sp->UU.U0.prba[i]
		[(long)WW - (long)AA] + tprob[(long)ai - (long)AA]
		[(long)YY - (long)AA] * sp->UU.U0.prba[i]
		[(long)YY - (long)AA] + tprob[(long)ai - (long)AA]
		[(long)VV - (long)AA] * sp->UU.U0.prba[i]
		[(long)VV - (long)AA];
/* p2c: protml.pas, line 2226: Note:
 * Line breaker spent 0.0+1.00 seconds, 5000 tries on line 2781 [251] */
      }
    }
    cp = cp->isop;
  }
}  /* sublklhdA */


Static Void branchlengthA(p, it)
node *p;
long *it;
{
  long i, numloop;
  double arc, arcold, sum, sumd1, sumd2, lkl, lkld1, lkld2, prod1, prod2;
  boolean done;
  node *q;
  amity ai, aj;
  daryamity tprobl, tdiff1, tdiff2;
  long FORLIM;

  q = p->kinp;
  arc = p->UU.U0.lnga;
  done = false;
  *it = 0;
  if (numsm < 3)
    numloop = 3;
  else
    numloop = maxiterat;

  while (*it < numloop && !done) {
    (*it)++;
    if (arc < minarc)
      arc = minarc;
    if (arc > maxarc)   /* attention */
      arc = minarc;
    tdiffmtrx(arc, tprobl, tdiff1, tdiff2);
    lkl = 0.0;
    lkld1 = 0.0;
    lkld2 = 0.0;
    FORLIM = endptrn;
    for (i = 0; i < FORLIM; i++) {
      sum = 0.0;
      sumd1 = 0.0;
      sumd2 = 0.0;
      for (ai = AA; (long)ai <= (long)VV; ai = (amity)((long)ai + 1)) {
	prod1 = freqdyhf[(long)ai - (long)AA] * p->UU.U0.prba[i]
		[(long)ai - (long)AA];
	if (prod1 < minreal)   /* attention */
	  prod1 = 0.0;
	for (aj = AA; (long)aj <= (long)VV; aj = (amity)((long)aj + 1)) {
	  prod2 = prod1 * q->UU.U0.prba[i][(long)aj - (long)AA];
	  if (prod2 < minreal)   /* attention */
	    prod2 = 0.0;
	  sum += prod2 * tprobl[(long)ai - (long)AA][(long)aj - (long)AA];
	  sumd1 += prod2 * tdiff1[(long)ai - (long)AA][(long)aj - (long)AA];
	  sumd2 += prod2 * tdiff2[(long)ai - (long)AA][(long)aj - (long)AA];
	}
      }
      if (sum > minreal) {  /* attention */
	lkl += log(sum) * weight[i];
	lkld1 += sumd1 / sum * weight[i];
	if (sum * sum > minreal)
	  lkld2 += (sumd2 * sum - sumd1 * sumd1) / sum / sum * weight[i];
      } else {
	if (debugoptn)
	  printf(" *check branchlength1*%4ld%4ld\n", p->number, i + 1);
      }  /* attention */
    }
    arcold = arc;
    if (lkld1 != lkld2)
      arc -= lkld1 / lkld2;
    else {
      arcold = arc + epsilon * 0.1;
      if (debugoptn)
	printf(" *check branchlength2*");
    }
    if (arc > maxarc)   /* attention */
      arc = minarc;
    done = (fabs(arcold - arc) < epsilon);
    /* IF debugoptn THEN writeln(output,' ':10,arc:10:5,
       arcold:10:5, -(lkld1/lkld2):10:5); */
  }
  smoothed = (smoothed && done);
  if (arc < minarc)
    arc = minarc;
  if (arc > maxarc)   /* attention */
    arc = minarc;
  p->UU.U0.lnga = arc;
  q->UU.U0.lnga = arc;
}  /* branchlengthA */


Static Void printupdate(tr, p, vold, it)
tree *tr;
node *p;
double vold;
long it;
{
  if (p == tr->startp->kinp)
    printf("%4ld", numsm);
  else
    printf("%4c", ' ');
  if (p->isop == NULL)   /* TIP */
    printf("%12c", ' ');
  else {
    printf("%4c%3ld", '(', p->isop->kinp->number);
    printf("%3ld )", p->isop->isop->kinp->number);
  }
  printf("%3ld -%3ld", p->number, p->kinp->number);
  if (p->kinp->isop == NULL)   /* TIP */
    printf("%10c", ' ');
  else {
    printf(" (%3ld", p->kinp->isop->kinp->number);
    printf("%3ld )", p->kinp->isop->isop->kinp->number);
  }
  printf("%2c%10.5f", ' ', p->UU.U0.lnga);
  printf(" (%10.5f )", p->UU.U0.lnga - vold);
  printf("%4ld\n", it);
}  /* printupdate */


Static Void updateA(tr, p, it)
tree *tr;
node *p;
long *it;
{
  double vold;

  vold = p->UU.U0.lnga;
  if (p->isop != NULL)   /* TIP */
    sublklhdA(p);
  if (p->kinp->isop != NULL)   /* TIP */
    sublklhdA(p->kinp);
  branchlengthA(p, it);
  /* IF debugoptn AND ((numsm = 1) OR (numsm = maxsmooth)) THEN */
  if (debugoptn && numsm == 1)
    printupdate(tr, p, vold, *it);
}  /* updateA */


Static Void smooth2(tr, numit)
tree *tr;
long *numit;
{
  long n, it, FORLIM;

  FORLIM = numsp;
  for (n = 1; n <= FORLIM; n++) {
    updateA(tr, tr->brnchp[n], &it);
    *numit += it;
  }
  FORLIM = ibrnch1;
  for (n = ibrnch2; n >= FORLIM; n--) {
    updateA(tr, tr->brnchp[n], &it);
    *numit += it;
  }
}  /* smooth */


Static Void smooth(tr, p, numit)
tree *tr;
node *p;
long *numit;
{
  long n, it;
  double vold;
  node *cp;
  long FORLIM;

  vold = p->UU.U0.lnga;
  if (p->isop != NULL)   /* TIP */
    sublklhdA(p);
  if (p->kinp->isop != NULL)   /* TIP */
    sublklhdA(p->kinp);
  branchlengthA(p, &it);
  *numit += it;
  if (debugoptn && (numsm == 1 || numsm == maxsmooth))
    printupdate(tr, p, vold, it);
  if (p->isop == NULL)   /* TIP */
    return;
  /* smooth ( tr,p^.isop^.kinp,numit );
     smooth ( tr,p^.isop^.isop^.kinp,numit ); */
  cp = p->isop;
  FORLIM = p->diverg;
  for (n = 1; n <= FORLIM; n++) {
    smooth(tr, cp->kinp, numit);
    cp = cp->isop;
  }
}  /* smooth */


Static Void printsmooth(tr, numit)
tree *tr;
long numit;
{
  long i, FORLIM;

  if (debugoptn)
    return;
  printf(" %3ld", numsm);
  printf("%4ld", numit);
  FORLIM = ibrnch2;
  for (i = 1; i <= FORLIM; i++)
    printf("%5.1f", tr->brnchp[i]->UU.U0.lnga);
  putchar('\n');
}  /* printsmooth */


Static Void leastsquares(am_, ym)
rnodety *am_;
double *ym;
{
  dnonoty am;
  long i, j, k;
  double pivot, element;
  dnonoty im;
  long FORLIM, FORLIM1, FORLIM2;

  memcpy(am, am_, sizeof(dnonoty));
  FORLIM = ibrnch2;
  for (i = 1; i <= FORLIM; i++) {
    FORLIM1 = ibrnch2;
    for (j = 1; j <= FORLIM1; j++) {
      if (i == j)
	im[i - 1][j - 1] = 1.0;
      else
	im[i - 1][j - 1] = 0.0;
    }
  }
  FORLIM = ibrnch2;
  for (k = 0; k < FORLIM; k++) {
    pivot = am[k][k];
    ym[k] /= pivot;
    FORLIM1 = ibrnch2;
    for (j = 0; j < FORLIM1; j++) {
      am[k][j] /= pivot;
      im[k][j] /= pivot;
    }
    FORLIM1 = ibrnch2;
    for (i = 0; i < FORLIM1; i++) {
      if (k + 1 != i + 1) {
	element = am[i][k];
	ym[i] -= element * ym[k];
	FORLIM2 = ibrnch2;
	for (j = 0; j < FORLIM2; j++) {
	  am[i][j] -= element * am[k][j];
	  im[i][j] -= element * im[k][j];
	}
      }
    }
  }
}  /* leastsquares */


Static Void initlength(tr)
tree *tr;
{
  long ia, j1, j2, k, na, n1, n2;
  double suma, sumy;
  spity des;
  rpairty dfpair, ymt;
  rnodety atymt;
  dpanoty amt;
  dnopaty atmt;
  dnonoty atamt;
  long FORLIM, FORLIM1, FORLIM2, FORLIM3;

  FORLIM = ibrnch2;
  for (ia = 1; ia <= FORLIM; ia++) {
    memcpy(des, tr->brnchp[ia]->descen, sizeof(spity));
    na = 0;
    FORLIM1 = numsp;
    for (j1 = 1; j1 < FORLIM1; j1++) {
      FORLIM2 = numsp;
      for (j2 = j1 + 1; j2 <= FORLIM2; j2++) {
	na++;
	/* writeln(output,' ',ia:3,j1:3,j2:3,na:3); */
	if (des[j1 - 1] != des[j2 - 1])
	  amt[na - 1][ia - 1] = 1.0;
	else
	  amt[na - 1][ia - 1] = 0.0;
	if (ia == 1) {
	  dfpair[na - 1] = 0.0;
	  FORLIM3 = endsite;
	  for (k = 0; k < FORLIM3; k++) {
	    if (chsequen[j1][k] != chsequen[j2][k])
	      dfpair[na - 1] += 1.0;
	  }
	}
	if (dfpair[na - 1] > 0.0)
	  ymt[na - 1] = -log(1.0 - dfpair[na - 1] / endsite);
	else
	  ymt[na - 1] = -log(1.0);
      }
    }
  }

  if (debugoptn) {
    putchar('\n');
    FORLIM = numpair;
    for (na = 0; na < FORLIM; na++) {
      printf(" %3ld ", na + 1);
      FORLIM1 = ibrnch2;
      for (ia = 0; ia < FORLIM1; ia++)
	printf("%3ld", (long)amt[na][ia]);
      printf("%8.3f%5ld\n", ymt[na], (long)dfpair[na]);
    }
  }

  FORLIM = numpair;
  for (ia = 0; ia < FORLIM; ia++) {
    FORLIM1 = ibrnch2;
    for (na = 0; na < FORLIM1; na++)
      atmt[na][ia] = amt[ia][na];
  }
  FORLIM = ibrnch2;
  for (n1 = 0; n1 < FORLIM; n1++) {
    FORLIM1 = ibrnch2;
    for (n2 = 0; n2 < FORLIM1; n2++) {
      suma = 0.0;
      FORLIM2 = numpair;
      for (ia = 0; ia < FORLIM2; ia++)
	suma += atmt[n1][ia] * amt[ia][n2];
      atamt[n1][n2] = suma;
    }
  }
  FORLIM = ibrnch2;
  for (n1 = 0; n1 < FORLIM; n1++) {
    sumy = 0.0;
    FORLIM1 = numpair;
    for (ia = 0; ia < FORLIM1; ia++)
      sumy += atmt[n1][ia] * ymt[ia];
    atymt[n1] = sumy;
  }

  if (debugoptn) {
    putchar('\n');
    FORLIM = ibrnch2;
    for (n1 = 1; n1 <= FORLIM; n1++) {
      printf(" %3ld ", n1);
      FORLIM1 = ibrnch2;
      for (n2 = 0; n2 < FORLIM1; n2++)
	printf("%3ld", (long)atamt[n1 - 1][n2]);
      printf("%8.3f\n", atymt[n1 - 1]);
    }
  }

  leastsquares(atamt, atymt);
  FORLIM = ibrnch2;
  for (na = 0; na < FORLIM; na++)
    atymt[na] = 100.0 * atymt[na];

  if (!debugoptn) {
    if (writeoptn) {
      printf("\n%5s", " arc");
      printf("%4s", "  it");
      FORLIM = ibrnch2;
      for (na = 1; na <= FORLIM; na++)
	printf("%3ld%2c", na, ' ');
      printf("\n%4c", '0');
      printf("%4c", '0');
      FORLIM = ibrnch2;
      for (na = 0; na < FORLIM; na++)
	printf("%5.1f", atymt[na]);
      putchar('\n');
    }
  }

  FORLIM = ibrnch2;
  for (na = 1; na <= FORLIM; na++) {
    tr->brnchp[na]->UU.U0.lnga = atymt[na - 1];
    tr->brnchp[na]->kinp->UU.U0.lnga = atymt[na - 1];
  }

}  /* initlength */


Static Void judgconverg(oldarcs, newarcs, cnvrg)
double *oldarcs, *newarcs;
boolean *cnvrg;
{
  /* judgment of convergence */
  long i;
  boolean same;
  long FORLIM;

  same = true;
  FORLIM = ibrnch2;
  for (i = 0; i < FORLIM; i++) {
    if (fabs(newarcs[i] - oldarcs[i]) > epsilon)
      same = false;
  }
  *cnvrg = same;
}  /* judgconverg */


Static Void variance(tr)
tree *tr;
{
  long k, m;
  aryamity xa1, xa2;
  amity ai, aj;
  double arcm, alkl, lkl, lkl2, sarc, sarc2, sblklhd, ld1, ld2, prod, vlkl;
  lengthty varc, varc2, blklhdm;
  daryamity tprobm, tdifm1, tdifm2;
  long FORLIM;
  double TEMP;
  long FORLIM1;

  alkl = tr->lklhd / endsite;
  vlkl = 0.0;
  FORLIM = endptrn;
  for (k = 0; k < FORLIM; k++) {
    TEMP = lklhdtrpt[notree][k] - alkl;
    vlkl += TEMP * TEMP * weight[k];
  }
  tr->vrilkl = vlkl;

  FORLIM = ibrnch2;
  for (m = 1; m <= FORLIM; m++) {
    arcm = tr->brnchp[m]->UU.U0.lnga;
    tdiffmtrx(arcm, tprobm, tdifm1, tdifm2);
    sarc = 0.0;
    sarc2 = 0.0;
    sblklhd = 0.0;
    FORLIM1 = endptrn;
    for (k = 0; k < FORLIM1; k++) {
      memcpy(xa1, tr->brnchp[m]->UU.U0.prba[k], sizeof(aryamity));
      memcpy(xa2, tr->brnchp[m]->kinp->UU.U0.prba[k], sizeof(aryamity));
      lkl2 = 0.0;
      ld1 = 0.0;
      ld2 = 0.0;
      for (ai = AA; (long)ai <= (long)VV; ai = (amity)((long)ai + 1)) {
	prod = freqdyhf[(long)ai - (long)AA] * xa1[(long)ai - (long)AA];
	if (prod < minreal)   /* attention */
	  prod = 0.0;
	for (aj = AA; (long)aj <= (long)VV; aj = (amity)((long)aj + 1)) {
	  if (xa2[(long)aj - (long)AA] < minreal)   /* attention */
	    xa2[(long)aj - (long)AA] = 0.0;
	  lkl2 += prod * tprobm[(long)ai - (long)AA]
		  [(long)aj - (long)AA] * xa2[(long)aj - (long)AA];
	  ld1 += prod * tdifm1[(long)ai - (long)AA]
		 [(long)aj - (long)AA] * xa2[(long)aj - (long)AA];
	  ld2 += prod * tdifm2[(long)ai - (long)AA]
		 [(long)aj - (long)AA] * xa2[(long)aj - (long)AA];
	}
      }
      lkl = exp(lklhdtrpt[notree][k]);
      if (lkl * lkl > minreal)   /* attention */
	sarc += (ld2 * lkl - ld1 * ld1) / lkl / lkl * weight[k];
      if (lkl2 * lkl2 > minreal)   /* attention */
	sarc2 += (ld2 * lkl2 - ld1 * ld1) / lkl2 / lkl2 * weight[k];
      if (lkl2 > minreal)   /* attention */
	sblklhd += log(lkl2) * weight[k];
    }
    varc[m - 1] = sarc;
    varc2[m - 1] = sarc2;
    blklhdm[m - 1] = sblklhd;

    /*          IF debugoptn THEN BEGIN
                   write  (output,m:4);
                   FOR m := 1 TO ibrnch2 DO write(output,sarc[m]:9:6);
                   writeln(output);
                END; */

  }
  FORLIM = ibrnch2;
  for (m = 0; m < FORLIM; m++)
    varc[m] = 1.0 / varc[m];
  FORLIM = ibrnch2;
  for (m = 0; m < FORLIM; m++)
    varc2[m] = 1.0 / varc2[m];
  memcpy(tr->vrilnga, varc, sizeof(lengthty));
  memcpy(tr->vrilnga2, varc2, sizeof(lengthty));
  memcpy(tr->blklhd, blklhdm, sizeof(lengthty));
}  /* variance */


Static Void manuvaluate(tr)
tree *tr;
{
  long n, it;
  lengthty newarcs, oldarcs;
  boolean cnvrg;
  long FORLIM;

  if (writeoptn)
    putchar('\n');
  if (debugoptn)
    printf(" MANUALVALUATE\n");
  /*  IF debugoptn THEN printcurtree ( tr );  */
  if (debugoptn)
    putchar('\n');
  initdescen(tr);
  initbranch(tr->startp);
  initbranch(tr->startp->kinp);
  /*  IF debugoptn THEN printcurtree ( tr );  */
  initlength(tr);
  if (debugoptn)
    printcurtree(tr);

  if (debugoptn)
    printf(" SMOOTH\n");
  cnvrg = false;
  FORLIM = ibrnch2;
  for (n = 1; n <= FORLIM; n++)
    newarcs[n - 1] = tr->brnchp[n]->UU.U0.lnga;
  numnw = 0;
  numsm = 0;
  do {
    numsm++;
    it = 0;
    smooth(tr, tr->startp->kinp, &it);
    memcpy(oldarcs, newarcs, sizeof(lengthty));
    FORLIM = ibrnch2;
    for (n = 1; n <= FORLIM; n++)
      newarcs[n - 1] = tr->brnchp[n]->UU.U0.lnga;
    judgconverg(oldarcs, newarcs, &cnvrg);
    numnw += it;
    if (writeoptn)
      printsmooth(tr, it);
  } while (!(numsm >= maxsmooth || cnvrg));
  tr->cnvrgnc = cnvrg;

  /* IF debugoptn THEN printcurtree ( tr ); */
  evaluateA(tr);
  variance(tr);
  if (!usertree)
    return;
  lklhdtree[notree] = tr->lklhd;
  aictree[notree] = tr->aic;
  paratree[notree] = ibrnch2;
  if (notree == 1) {
    maxltr = 1;
    maxlkl = tr->lklhd;
    minatr = 1;
    minaic = tr->aic;
    return;
  }
  if (tr->lklhd > maxlkl) {
    maxltr = notree;
    maxlkl = tr->lklhd;
  }
  if (tr->aic < minaic) {
    minatr = notree;
    minaic = tr->aic;
  }
}  /* manuvaluate */


Static Void autovaluate(tr)
tree *tr;
{
  long n, it;
  lengthty newarcs, oldarcs;
  boolean cnvrg;
  long FORLIM;

  if (writeoptn)
    putchar('\n');
  if (debugoptn)
    printf(" AUTOVALUATE\n");
  /*
     IF debugoptn THEN printcurtree( tr );
     IF debugoptn THEN writeln(output);
     initdescen( tr );
     initbranch (tr.startp);
     initbranch (tr.startp^.kinp);
     IF debugoptn THEN printcurtree ( tr );
     initlength ( tr );
     IF debugoptn THEN printcurtree ( tr );
  */
  it = 0;
  for (n = 1; n <= 3; n++) {
    sublklhdA(tr->brnchp[ibrnch2]);
    sublklhdA(tr->brnchp[ibrnch2]->kinp);
    branchlengthA(tr->brnchp[ibrnch2], &it);
  }
  if (debugoptn)
    printcurtree(tr);

  if (debugoptn)
    printf(" SMOOTH\n");
  cnvrg = false;
  FORLIM = ibrnch2;
  for (n = 1; n <= FORLIM; n++)
    newarcs[n - 1] = tr->brnchp[n]->UU.U0.lnga;
  numnw = 0;
  numsm = 0;
  do {
    numsm++;
    it = 0;
    smooth(tr, tr->startp->kinp, &it);
    memcpy(oldarcs, newarcs, sizeof(lengthty));
    FORLIM = ibrnch2;
    for (n = 1; n <= FORLIM; n++)
      newarcs[n - 1] = tr->brnchp[n]->UU.U0.lnga;
    judgconverg(oldarcs, newarcs, &cnvrg);
    numnw += it;
    if (writeoptn)
      printsmooth(tr, it);
  } while (!(numsm >= maxsmooth || cnvrg));
  tr->cnvrgnc = cnvrg;

  /* IF debugoptn THEN printcurtree ( tr ); */
  evaluateA(tr);
  variance(tr);
  if (usertree)
    return;
  lklhdtree[notree] = tr->lklhd;
  aictree[notree] = tr->aic;
  paratree[notree] = ibrnch2;
  if (notree == 1) {
    maxltr = 1;
    maxlkl = tr->lklhd;
    minatr = 1;
    minaic = tr->aic;
    return;
  }
  if (tr->lklhd > maxlkl) {
    maxltr = notree;
    maxlkl = tr->lklhd;
  }
  if (tr->aic < minaic) {
    minatr = notree;
    minaic = tr->aic;
  }
}  /* autovaluate */


#define maxover         50
#define maxleng         30


/**************************************/
/***    OUTPUT OF TREE TOPOLOGY     ***/
/**************************************/

Static Void prbranch(up, depth, m, maxm, umbrella, length)
node *up;
long depth, m, maxm;
boolean *umbrella;
long *length;
{
  long i, j, n, d, maxn;
  node *cp;
  boolean over;
  long FORLIM, FORLIM1;

  over = false;
  d = depth + 1;
  if ((long)up->UU.U0.lnga >= maxover) {   /* +4 */
    over = true;
    length[d] = maxleng;
  } else
    length[d] = (long)up->UU.U0.lnga + 3;
  if (up->isop == NULL) {   /* TIP */
    if (m == 1)
      umbrella[d - 1] = true;
    for (j = 0; j < d; j++) {
      if (umbrella[j])
	printf("%*c", (int)length[j], ':');
      else
	printf("%*c", (int)length[j], ' ');
    }
    if (m == maxm)
      umbrella[d - 1] = false;
    if (over) {
      FORLIM = length[d] - 3;
      for (i = 1; i <= FORLIM; i++)
	putchar('+');
    } else {
      FORLIM = length[d] - 3;
      for (i = 1; i <= FORLIM; i++)
	putchar('-');
    }
    if (up->number < 10)
      printf("--%ld.", up->number);
    else if (up->number < 100)
      printf("-%2ld.", up->number);
    else
      printf("%3ld.", up->number);
    for (i = 0; i < maxname; i++)
      putchar(up->namesp[i]);
    /* write(output,d:36); */
    putchar('\n');
    return;
  }
  cp = up->isop;
  maxn = up->diverg;
  for (n = 1; n <= maxn; n++) {
    prbranch(cp->kinp, d, n, maxn, umbrella, length);
    cp = cp->isop;
    if (n == maxn / 2) {
      if (m == 1)
	umbrella[d - 1] = true;
      if (n == 1)
	umbrella[d - 1] = true;
      for (j = 0; j < d; j++) {
	if (umbrella[j])
	  printf("%*c", (int)length[j], ':');
	else
	  printf("%*c", (int)length[j], ' ');
      }
      if (n == maxn)
	umbrella[d - 1] = false;
      if (m == maxm)
	umbrella[d - 1] = false;
      if (over) {
	FORLIM1 = length[d] - 3;
	for (i = 1; i <= FORLIM1; i++)
	  putchar('+');
      } else {
	FORLIM1 = length[d] - 3;
	for (i = 1; i <= FORLIM1; i++)
	  putchar('-');
      }
      if (up->number < 10)   /* ,':' */
	printf("--%ld", up->number);
      else if (up->number < 100)
	printf("-%2ld", up->number);
      else
	printf("%3ld", up->number);
      /* write(output,d:50); */
      putchar('\n');
    } else if (n != maxn) {
      for (j = 0; j <= d; j++) {
	if (umbrella[j])
	  printf("%*c", (int)length[j], ':');
	else
	  printf("%*c", (int)length[j], ' ');
      }
      putchar('\n');
    }
  }

  /* ,':' */
  /* ,':' */
}  /* prbranch */

#undef maxover
#undef maxleng


Static Void prtopology(tr)
tree *tr;
{
  long n, maxn, depth;
  node *cp;
  umbty umbrella;
  lspty length;

  for (n = 0; n <= maxsp; n++) {
    umbrella[n] = false;
    if (n == 0)
      length[n] = 3;
    else
      length[n] = 6;
  }
  cp = tr->brnchp[0];
  maxn = cp->diverg + 1;
  putchar('\n');
  for (n = 1; n <= maxn; n++) {
    depth = 0;
    prbranch(cp->kinp, depth, n, maxn, umbrella, length);
    cp = cp->isop;
    if (n == maxn / 2)
      printf("%*s\n", (int)length[0], "0:");
    else if (n != maxn)
      printf("%*c\n", (int)length[0], ':');
  }
}  /* prtopology */


/*********************************/
/***  OUTPUT OF TREE TOPOLOGY  ***/
/*********************************/

Static Void charasubtoporogy(up, nsp, nch)
node *up;
long *nsp, *nch;
{
  long n, maxn;
  node *cp;

  if (up->isop == NULL) {   /* TIP */
    for (n = 0; n < maxname; n++) {
      if (up->namesp[n] != ' ') {
	putchar(up->namesp[n]);
	(*nch)++;
      }
    }
    (*nsp)++;
    return;
  }
  cp = up->isop;
  maxn = up->diverg;
  putchar('(');
  (*nch)++;
  for (n = 1; n <= maxn; n++) {
    charasubtoporogy(cp->kinp, nsp, nch);
    cp = cp->isop;
    if (n != maxn) {
      putchar(',');
      (*nch)++;
      if (*nch > maxline - 10) {
	printf("\n%3c", ' ');
	*nch = 3;
      }
    }
  }
  putchar(')');
  (*nch)++;
}  /* charasubtoporogy */


Static Void charatopology(tr)
tree *tr;
{
  long n, maxn, nsp, nch;
  node *cp;

  nsp = 0;
  nch = 3;
  cp = tr->brnchp[0];
  maxn = cp->diverg + 1;
  printf("\n%3s", " ( ");
  for (n = 1; n <= maxn; n++) {
    charasubtoporogy(cp->kinp, &nsp, &nch);
    cp = cp->isop;
    if (n != maxn) {
      printf("%2s", ", ");
      nch += 2;
      if (nch > maxline - 15) {
	printf("\n%3c", ' ');
	nch = 3;
      }
    }
  }
  printf(" )\n");
}  /* charatopology */


/**************************************/
/***   OUTPUT OF THE FINAL RESULT   ***/
/**************************************/

Static Void printbranch(tr)
tree *tr;
{
  long i, j;
  node *p, *q;
  long FORLIM;

  FORLIM = ibrnch2;
  for (i = 0; i < FORLIM; i++) {
    p = tr->brnchp[i + 1];
    q = p->kinp;
    printf("%5c", ' ');
    if (p->isop == NULL) {   /* TIP */
      for (j = 0; j < maxname; j++)
	putchar(p->namesp[j]);
    } else {
      for (j = 1; j <= maxname; j++)
	putchar(' ');
    }
    printf("%5ld", p->number);
    if (p->UU.U0.lnga >= maxarc)
      printf("%12s", " infinity");
    else if (p->UU.U0.lnga <= minarc)
      printf("%12s", " zero    ");
    else
      printf("%12.5f", p->UU.U0.lnga);
    printf("%3s%9.5f%2s", " (", sqrt(fabs(tr->vrilnga[i])), " )");
    if (debugoptn) {
      printf("%12.5f", sqrt(fabs(tr->vrilnga2[i])));
      printf("%13.5f", tr->blklhd[i]);
    }
    putchar('\n');
  }

}  /* printbranch */


Static Void summarize(tr)
tree *tr;
{
  putchar('\n');
  /* printline( 46 ); */
  if (usertree)
    printf("%4s%3ld%8c", " No.", notree, ' ');
  else
    printf("%4s%3ld%2s%3ld%3c", " No.", stage, " -", notree, ' ');
  printf("%7s%9s%11s", "number", "Length", "S.E.");
  if (!tr->cnvrgnc)
    printf("%21s", "non convergence");
  /* write  (output, 'convergence    ':21) */
  if (writeoptn)
    printf("%2c%3ld%2c%4ld", ':', numsm, ',', numnw);
  putchar('\n');
  printline(46L);
  printbranch(tr);
  printline(46L);
  printf("%8s", "ln L :");
  printf("%10.3f", tr->lklhd);
  printf("%2c%8.3f%2s", '(', sqrt(fabs(tr->vrilkl)), " )");
  printf("%7s%9.3f\n", "AIC :", tr->aic);
  printline(46L);
  /*       writeln(output); */
}  /* summarize */


Static Void cleartree(tr)
tree *tr;
{
  long i, n;
  node *cp, *sp, *dp;
  long FORLIM;

  FORLIM = ibrnch2;
  for (i = 1; i <= FORLIM; i++) {
    tr->brnchp[i]->kinp->kinp = NULL;
    tr->brnchp[i]->kinp = NULL;
  }
  FORLIM = ibrnch2 + 1;
  for (i = ibrnch1; i <= FORLIM; i++) {
    if (i == ibrnch2 + 1)
      n = 0;
    else
      n = i;
    sp = tr->brnchp[n];
    cp = sp->isop;
    sp->isop = NULL;
    while (cp != sp) {
      dp = cp;
      cp = cp->isop;
      dp->isop = freenode;
      freenode = dp;
      dp = NULL;
      /* dp^.isop := NIL;
         DISPOSE( dp ); */
    }
    sp = NULL;
    tr->brnchp[n] = NULL;
    cp->isop = freenode;
    freenode = cp;
    cp = NULL;
    /* DISPOSE( cp ); */
  }
}  /* cleartree */


Static Void bootstrap()
{
  long ib, jb, nb, bsite, maxboottree, inseed;
  double maxlklboot;
  longintty seed;
  long FORLIM, FORLIM1, FORLIM2;

  inseed = 12345;
  for (ib = 0; ib <= 5; ib++)
    seed[ib] = 0;
  ib = 0;
  do {
    seed[ib] = inseed & 63;
    inseed /= 64;
    ib++;
  } while (inseed != 0);

  FORLIM = numtrees;
  for (jb = 1; jb <= FORLIM; jb++)
    boottree[jb] = 0.0;
  for (nb = 1; nb <= maxboot; nb++) {
    FORLIM1 = numtrees;
    for (jb = 1; jb <= FORLIM1; jb++)
      lklboottree[jb] = 0.0;
    FORLIM1 = endsite;
    for (ib = 1; ib <= FORLIM1; ib++) {
      bsite = weightw[(int)((long)(randum(seed) * endsite + 1)) - 1];
      FORLIM2 = numtrees;
      for (jb = 1; jb <= FORLIM2; jb++)
	lklboottree[jb] += lklhdtrpt[jb][bsite - 1];
    }

    maxlklboot = lklboottree[1];
    maxboottree = 1;
    if (debugoptn)
      printf("%5ld", nb);
    FORLIM1 = numtrees;
    for (jb = 1; jb <= FORLIM1; jb++) {
      if (lklboottree[jb] > maxlklboot) {
	maxlklboot = lklboottree[jb];
	maxboottree = jb;
      }
      if (debugoptn)
	printf("%8.1f", lklboottree[jb]);
    }
    if (debugoptn)
      printf("%4ld\n", maxboottree);
    boottree[maxboottree] += 1.0;
  }
  if (maxboot == 0) {
    FORLIM = numtrees;
    for (jb = 1; jb <= FORLIM; jb++)
      boottree[jb] /= maxboot;
  }
}  /* bootstrap */


Static Void printlklhd()
{
  long i, j, cul;
  double suml;   /* difference of ln lklhd */
  double suml2;   /* absolute value of difference */
  double suma;   /* difference of AIC */
  double suma2;   /* absolute value of AIC */
  double lklsite;   /* likelihood of site */
  double aicsite;   /* AIC of site */
  double sdl;   /* standard error ln lklhd */
  double sda;   /* standard error of AIC */
  long FORLIM, FORLIM1;
  double TEMP;

  cul = 71;
  putchar('\n');
  if (usertree)
    printf("%10c%4ld user trees\n", ' ', numtrees);
  else
    printf("%10s%3ld%6s%5ld%6s\n", " NO.", stage, "stage", notree, "trees");
  if (numtrees <= 0 || endsite <= 1)
    return;
  printline(cul);
  printf("%6s%6s%12s%12s%6s%12s%17s\n",
	 " Tree", "ln L", "Diff ln L", "#Para", "AIC", "Diff AIC", "Boot P");
  printline(cul);
  FORLIM = numtrees;
  for (i = 1; i <= FORLIM; i++) {
    printf("%4ld%9.2f", i, lklhdtree[i]);
    if (maxltr == i)
      printf(" <-- best%9c", ' ');
    else {
      suml = 0.0;
      suml2 = 0.0;
      FORLIM1 = endptrn;
      for (j = 0; j < FORLIM1; j++) {
	lklsite = lklhdtrpt[maxltr][j] - lklhdtrpt[i][j];
	suml += weight[j] * lklsite;
	suml2 += weight[j] * lklsite * lklsite;
      }
      TEMP = suml / endsite;
      sdl = sqrt(endsite / (endsite - 1.0) * (suml2 - TEMP * TEMP));
      printf("%9.2f", lklhdtree[i] - maxlkl);
      printf("%3s%6.2f", "+-", sdl);
    }
    printf("%4ld", paratree[i]);
    printf("%9.2f", aictree[i]);
    if (minatr == i)
      printf(" <-- best%9c", ' ');
    else {
      suma = 0.0;
      suma2 = 0.0;
      FORLIM1 = endptrn;
      for (j = 0; j < FORLIM1; j++) {
	aicsite = -2.0 * (lklhdtrpt[maxltr][j] - lklhdtrpt[i][j]);
	suma += weight[j] * aicsite;
	suma2 += weight[j] * aicsite * aicsite;
      }
      TEMP = suma / endsite;
      sda = sqrt(endsite / (endsite - 1.0) * (suma2 - TEMP * TEMP));
      printf("%9.2f", aictree[i] - minaic);
      printf("%3s%6.2f", "+-", sda);
    }
    if (usertree)
      printf("%9.4f", boottree[i]);
    putchar('\n');
  }
  printline(cul);
}  /* printlklhd */


Static Void outlklhd()
{
  long i, j, k, nt, FORLIM, FORLIM1, FORLIM2;

  fprintf(lklfile, "%5ld%5ld\n", numsp, endsite);
  i = 0;
  FORLIM = numtrees;
  for (nt = 1; nt <= FORLIM; nt++) {
    fprintf(lklfile, "%5ld\n", nt);
    FORLIM1 = endptrn;
    for (j = 0; j < FORLIM1; j++) {
      FORLIM2 = weight[j];
      for (k = 1; k <= FORLIM2; k++) {
	fprintf(lklfile, "% .5E", lklhdtrpt[nt][j]);
	i++;
	if (i == 6) {
	  putc('\n', lklfile);
	  i = 0;
	}
      }
    }
    if (i != 0)
      putc('\n', lklfile);
    i = 0;
  }
}  /* outlklhd */


Static Void manutree()
{
  long ntree, FORLIM;

  /*PAGE(output);*/
  fscanf(seqfile, "%ld%*[^\n]", &numtrees);
  getc(seqfile);
  printf("\n %5ld user trees\n\n", numtrees);
  FORLIM = numtrees;
  for (ntree = 1; ntree <= FORLIM; ntree++) {
    notree = ntree;
    treestructure(&ctree);
    manuvaluate(&ctree);
    prtopology(&ctree);
    summarize(&ctree);
    cleartree(&ctree);
    /*PAGE(output);*/
  }
  if (bootsoptn)
    bootstrap();
  printlklhd();
  if (putlkoptn)
    outlklhd();
}  /* manutree */


Static Void newbranch(nbranch, np1, np2)
long nbranch;
node **np1, **np2;
{
  long j;
  node *np;

  for (j = 1; j <= 2; j++) {
    if (freenode == NULL)
      np = (node *)Malloc(sizeof(node));
    else {
      np = freenode;
      freenode = np->isop;
      np->isop = NULL;
    }
    clearnode(np);
    if (j == 1)
      *np1 = np;
    else
      *np2 = np;
    np = NULL;
  }
  (*np1)->kinp = *np2;
  (*np2)->kinp = *np1;
  ctree.brnchp[nbranch] = *np1;
  ctree.brnchp[nbranch]->number = ibrnch2;
}  /* newbranch */


Static Void decomposition(cnode)
long cnode;
{
  long i1, i2, maxdvg;
  node *cp1, *cp2, *bp1, *bp2, *lp1, *lp2, *np1, *np2;
  double maxlkls;

  if (ctree.brnchp[cnode]->diverg <= 2) {
    return;
  }  /* IF */
  /*
     PAGE(output);
  */
  stage++;
  notree = 0;
  ibrnch2++;
  newbranch(ibrnch2, &np1, &np2);
  cp1 = ctree.brnchp[cnode];
  cp2 = cp1;
  bp1 = cp1;
  do {
    bp1 = bp1->isop;
  } while (bp1->isop != cp1);
  bp2 = bp1;
  maxlkls = -999999.0;
  maxdvg = ctree.brnchp[cnode]->diverg;
  if (maxdvg == 3) {
    maxdvg--;
    cp1 = cp1->isop;
    cp2 = cp1;
    bp1 = bp1->isop;
    bp2 = bp1;
  }
  for (i1 = 1; i1 <= maxdvg; i1++) {
    for (i2 = i1 + 1; i2 <= maxdvg + 1; i2++) {
      if (debugoptn) {
	ibrnch2--;
	printcurtree(&ctree);
	ibrnch2++;
      }
      notree++;
      bp2 = cp2;
      cp2 = cp2->isop;
      if (debugoptn)
	printf(" AUTO-INS%3ld%3ld%4c%3ld%3ld%4c%3ld%3ld%4c%3ld%3ld\n",
	       i1, i2, 'c', cp1->kinp->number, cp2->kinp->number, 'b',
	       bp1->kinp->number, bp2->kinp->number, 'n', np1->number,
	       np2->number);
      insertbranch(&ctree, cp1, cp2, bp1, bp2, np1, np2);
      autovaluate(&ctree);
      prtopology(&ctree);
      charatopology(&ctree);
      summarize(&ctree);
      if (ctree.lklhd > maxlkls) {
	maxlkls = ctree.lklhd;
	lp1 = bp1;
	lp2 = bp2;
      }
      if (debugoptn)
	printf(" AUTO-DEL%3ld%3ld%4c%3ld%3ld%4c%3ld%3ld%4c%3ld%3ld\n",
	       i1, i2, 'c', cp1->kinp->number, cp2->kinp->number, 'b',
	       bp1->kinp->number, bp2->kinp->number, 'n', np1->number,
	       np2->number);
      deletebranch(&ctree, cnode, cp1, cp2, bp1, bp2, np1, np2);
    }
    bp1 = cp1;
    cp1 = bp1->isop;
    bp2 = bp1;
    cp2 = bp1->isop;
  }  /* FOR */
  /*
     PAGE(output);
  */
  numtrees = notree;
  printlklhd();
  notree = 0;
  cp1 = lp1->isop;
  cp2 = lp2->isop;
  bp1 = lp1;
  bp2 = lp2;
  if (debugoptn)
    printf(" AUTO-MAX%4c%3ld%3ld%4c%3ld%3ld%4c%3ld%3ld\n",
	   'c', cp1->kinp->number, cp2->kinp->number, 'b', bp1->kinp->number,
	   bp2->kinp->number, 'n', np1->number, np2->number);
  insertbranch(&ctree, cp1, cp2, bp1, bp2, np1, np2);
  autovaluate(&ctree);
  prtopology(&ctree);
  summarize(&ctree);
}  /* decomposition */


Static Void autotree()
{
  long i, j, dvg, cnode, FORLIM;

  stage = 0;
  notree = 1;
  if (semiaoptn)
    treestructure(&ctree);
  else
    starstructure(&ctree);
  manuvaluate(&ctree);
  prtopology(&ctree);
  summarize(&ctree);

  if (!semiaoptn) {
    do {
      decomposition(0L);
    } while (ctree.brnchp[0]->diverg >= 3);
    return;
  }

  /* printlklhd; */
  /* IF putlkoptn THEN outlklhd; */
  if (firstoptn) {
    do {
      decomposition(0L);
    } while (ctree.brnchp[0]->diverg >= 3);
    do {
      dvg = 2;   /* ctree.brnchp[0]^.diverg */
      cnode = 0;
      FORLIM = ibrnch2;
      for (i = ibrnch1; i <= FORLIM; i++) {
	if (ctree.brnchp[i]->diverg > dvg) {
	  dvg = ctree.brnchp[i]->diverg;
	  cnode = i;
	}
      }
      decomposition(cnode);
    } while (dvg >= 3);
    return;
  }
  if (lastoptn) {
    do {
      dvg = 2;   /* ctree.brnchp[0]^.diverg */
      cnode = 0;
      FORLIM = ibrnch2;
      for (i = ibrnch1; i <= FORLIM; i++) {
	if (ctree.brnchp[i]->diverg > dvg) {
	  dvg = ctree.brnchp[i]->diverg;
	  cnode = i;
	}
      }
      decomposition(cnode);
    } while (dvg >= 3);
    do {
      decomposition(0L);
    } while (ctree.brnchp[0]->diverg >= 3);
    return;
  }
  do {
    dvg = 2;   /* ctree.brnchp[0]^.diverg */
    cnode = 0;
    FORLIM = ibrnch2 + 1;
    for (i = ibrnch1; i <= FORLIM; i++) {
      if (i == ibrnch2 + 1)
	j = 0;
      else
	j = i;
      if (ctree.brnchp[j]->diverg > dvg) {
	dvg = ctree.brnchp[j]->diverg;
	cnode = j;
      }
    }
    decomposition(cnode);
  } while (dvg >= 3);

  /* auto mode */
}  /* autotree */


Static Void mainio()
{
  long nexe;

  freenode = NULL;
  for (nexe = 1; nexe <= maxexe; nexe++) {
    numexe = nexe;
    if (maxexe > 1)
      printf(" * JOB :%4ld *\n", numexe);
    getinput();
    if (normal && numexe == 1)
      maketransprob();
    if (normal) {
      if (usertree)
	manutree();
      else
	autotree();
    }
    /* IF maxexe > 1 THEN PAGE(output); */
  }
}  /* mainio */


main(argc, argv)
int argc;
Char *argv[];
{
  /* ASSIGN(seqfile,seqfname);      If TURBO Pascal, use */
  /* ASSIGN(tpmfile,tpmfname); */
  /* IF putlkoptn THEN
     ASSIGN (lklfile,lklfname); */

  PASCAL_MAIN(argc, argv);
  tpmfile = NULL;
  lklfile = NULL;
  seqfile = NULL;
  rewind(seqfile);
  /* If SUN Pascal, don't use */
  /* RESET (tpmfile); */
  /* IF putlkoptn THEN
     REWRITE(lklfile); */

  /* RESET (seqfile,seqfname);      If SUN Pascal, use */
  /* RESET (tpmfile,tpmfname); */
  /* IF putlkoptn THEN
     REWRITE(lklfile,lklfname); */

  mainio();

  /* CLOSE (seqfile);               If TURBO Pascal, use */
  /* CLOSE (tpmfile); */
  /* IF putlkoptn THEN
     CLOSE (lklfile); */

  if (seqfile != NULL)
    fclose(seqfile);
  if (lklfile != NULL)
    fclose(lklfile);
  if (tpmfile != NULL)
    fclose(tpmfile);
  exit(EXIT_SUCCESS);
}  /* PROTML */




/* End. */
