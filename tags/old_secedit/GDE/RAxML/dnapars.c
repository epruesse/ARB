#include "phylip2.h"


/* version 3.573c. (c) Copyright 1993-1998 by Joseph Felsenstein.
   Written by Joseph Felsenstein, Akiko Fuseki, Sean Lamont, and Andrew Keeffe.
   Permission is granted to copy and use this program provided no fee is
   charged for it and provided that this copyright notice is not removed. */

#define nmlngth         100   /* number of characters in species name    */
#define maxtrees        10   /* maximum number of tied trees stored     */
#define maxuser         8   /* maximum number of user-defined trees */
#define ibmpc0          false
#define ansi0           true
#define vt520           false
#define down            2

static char basechar[32]="ACMGRSVTWYHKDBNO???????????????";
typedef short *steptr;
typedef short *stepshortptr;
typedef enum {
  A, C, G, U, O
} bases;
typedef short *baseptr;
/* nodes will form a binary tree */

typedef struct node {           /* describes a tip species or an ancestor */
  struct node *next, *back;     /* pointers to nodes                      */
  short index;                   /* number of the node                     */
  boolean tip, bottom;          /* present species are tips of tree       */
  baseptr base;                 /* the sequence                           */
  stepshortptr numsteps;        /* bookkeeps steps                        */
  short xcoord, ycoord, ymin;    /* used by printree                       */
  short ymax;
} node;

typedef node **pointptr;
typedef short longer[6];
typedef char **sequence;



typedef struct gbases {
  baseptr base;
  struct gbases *next;
} gbases;

Static node *root, *p;
Static FILE *infile, *treefile;
Static short spp, nonodes, chars, endsite, outgrno, col, datasets, ith,
            i, j, l, jumb, njumble, inseed;
/* spp = number of species
   nonodes = number of nodes in tree
   chars = number of sites in actual sequences
   outgrno indicates outgroup */
Static boolean jumble, usertree, weights, thresh, trout, outgropt,
               printdata, progress, treeprint, stepbox, ancseq, mulsets,
               interleaved, ibmpc, vt52, ansi, firstset;
Static steptr weight = NULL, oldweight= NULL, alias= NULL, ally= NULL, location= NULL;
Static pointptr treenode= NULL;            /* pointers to all nodes in tree */
Static Char **nayme= NULL;                 /* names of species              */
Static sequence y= NULL;
Static gbases *garbage= NULL;
Static double threshold= 0;
Static longer seed;
Static short *enterorder= NULL;

/* local variables for Pascal maketree, propagated globally for C  version: */

short nextree, which, minwhich;
double like, minsteps, bestyet, bestlike, bstlike2;
boolean lastrearr, recompute;
double nsteps[maxuser];
short **fsteps = NULL;
node *there = NULL;
short *place = NULL;
short **bestrees = NULL;
long *threshwt = NULL;
baseptr nothing;
node *temp, *temp1;
Char ch;
boolean *names = NULL;
int ilvd;




Static Void gnu(p)
gbases **p;
{
  /* this and the following are do-it-yourself garbage collectors.
     Make a new node or pull one off the garbage list */
  if (garbage != NULL) {
    *p = garbage;
    garbage = garbage->next;
  } else {
    *p = (gbases *)malloc(sizeof(gbases));
    (*p)->base = (baseptr)malloc(endsite*sizeof(short));        
  }
  (*p)->next = NULL;
}  /* gnu */


void Rchuck(p)
gbases *p;
{
  /* collect garbage on p -- put it on front of garbage list */
  p->next = garbage;
  garbage = p;
}  /* Rchuck */


double Rrandum(seed)
short *seed;
{
  /* random number generator -- slow but machine independent */
  short i, j, k, sum;
  longer mult, newseed;
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
  memcpy(seed, newseed, sizeof(longer));
  seed[5] &= 3;
  x = 0.0;
  for (i = 0; i <= 5; i++)
    x = x / 64.0 + seed[i];
  x /= 4.0;
  return x;
}  /* randum */



Static Void uppercase(ch)
Char *ch;
{
  /* convert ch to upper case -- either ASCII or EBCDIC */
  *ch = (islower (*ch) ? toupper(*ch) : (*ch));
}  /* uppercase */


Local Void getoptions()
{
  /* interactively set options */
  short i, inseed0;
  Char ch;
  boolean done1;

  jumble = false;
  njumble = 1;
  outgrno = 1;
  outgropt = false;
  thresh = false;
  trout = true;
  usertree = false;
  weights = false;
  printdata = false;
  progress = false;
  treeprint = false;
  stepbox = false;
  ancseq = false;
  interleaved = ilvd;
  /*
  printf(ansi ? "\033[2J\033[H" :
	 vt52 ? "\033E\033H"    : "\n");
  printf("\nDNA parsimony algorithm, version %s\n\n",VERSION);
  printf("Setting for this run:\n");
  printf("  U                 Search for best tree?  %s\n",
	 (usertree ? "No, use user trees in input file" : "Yes"));
  if (!usertree) {
    printf("  J   Randomize input order of sequences?");
    if (jumble)
      printf("  Yes (seed =%8hd,%3hd times)\n", inseed0, njumble);
    else
      printf("  No. Use input order\n");
  }
   
  printf("  O                        Outgroup root?");
  if (outgropt)
    printf("  Yes, at sequence number%3hd\n", outgrno);
  else
    printf("  No, use as outgroup species%3hd\n", outgrno);
  printf("  T              Use Threshold parsimony?");
  if (thresh)
    printf("  Yes, count steps up to%4.1f per site\n", threshold);
  else
    printf("  No, use ordinary parsimony\n");
  printf("  M           Analyze multiple data sets?");
  if (mulsets)
    printf("  Yes, %2hd sets\n", datasets);
  else
    printf("  No\n");
  printf("  I          Input sequences interleaved?  %s\n",
	 (interleaved ? "Yes" : "No, sequential"));
  printf("  0   Terminal type (IBM PC, VT52, ANSI)?  %s\n",
	 ibmpc ? "IBM PC" :
	 ansi  ? "ANSI"   :
	 vt52  ? "VT52"    : "(none)");
  printf("  1    Print out the data at start of run  %s\n",
	 (printdata ? "Yes" : "No"));
  printf("  2  Print indications of progress of run  %s\n",
	 progress ? "Yes" : "No");
  printf("  3                        Print out tree  %s\n",
	 treeprint ? "Yes" : "No");
  printf("  4          Print out steps in each site  %s\n",
	 stepbox ? "Yes" : "No");
  printf("  5  Print sequences at all nodes of tree  %s\n",
	 ancseq ? "Yes" : "No");
  printf("  6       Write out trees onto tree file?  %s\n",
	 trout ? "Yes" : "No");
  printf("\nAre these settings correct? (type Y or the letter for one to change)\n");
  */
 
  if (jumble) 
    {	        
    }
  else 
    njumble = 1;
	
  
	
 
  if (thresh) 
    {
	  done1 = false;
	  do {
	    printf("What will be the threshold value?\n");
	    scanf("%lf%*[^\n]", &threshold);
              getchar();
	    done1 = (threshold >= 1.0);
	    if (!done1)
	      printf("BAD THRESHOLD VALUE:  it must be greater than 1\n");
	    else
	      threshold = (short)(threshold * 10.0 + 0.5) / 10.0;
	  } while (done1 != true);
    }
      
	
      
  if (mulsets) 
    {
      done1 = false;
      do {
	printf("How many data sets?\n");
	scanf("%hd%*[^\n]", &datasets);
	getchar();
	done1 = (datasets >= 1);
	if (!done1)
	  printf("BAD DATA SETS NUMBER:  it must be greater than 1\n");
      } while (done1 != true);
    }
      
	     
}  





Local Void inputweights()
{
  /* input the character weights, 0-9 and A-Z for weights 0 - 35 */
  Char ch;
  short i;

  for (i = 1; i < nmlngth; i++) {
    ch = getc(infile);
    if (ch == '\n')
      ch = ' ';
  }
  for (i = 0; i < chars; i++) {
    do {
      if (Reoln(infile)) {
        fscanf(infile, "%*[^\n]");
        getc(infile);
      }
      ch = getc(infile);
      if (ch == '\n')
        ch = ' ';
    } while (ch == ' ');
    weight[i] = 1;
    if (isdigit(ch))
      weight[i] = ch - '0';
    else if (isalpha(ch)) {
      uppercase(&ch);
      if (ch >= 'A' && ch <= 'I')
        weight[i] = ch - 55;
      else if (ch >= 'J' && ch <= 'R')
        weight[i] = ch - 55;
      else
        weight[i] = ch - 55;
    } else {
      printf("BAD WEIGHT CHARACTER: %c\n", ch);
      exit(-1);
    }
  }
  fscanf(infile, "%*[^\n]");
  getc(infile);
  weights = true;
}  /* inputweights */

Local Void printweights()
{
  /* print out the weights of sites */
  short i, j, k;

  
 
}  /* printweights */

Local Void inputoptions()
{
  /* input the information on the options */
  Char ch;
  short extranum, i, cursp, curchs;

  if (!firstset) {
    if (Reoln(infile)) {
      fscanf(infile, "%*[^\n]");
      getc(infile);
    }
    fscanf(infile, "%hd%hd", &cursp, &curchs);
    if (cursp != spp) {
      printf("\nERROR: INCONSISTENT NUMBER OF SPECIES IN DATA SET %4hd\n",
             ith);
      exit(-1);
    }
    chars = curchs;
  }
  extranum = 0;
  while (!(Reoln(infile))) {
    ch = getc(infile);
    if (ch == '\n')
      ch = ' ';
    uppercase(&ch);
    if (ch == 'W')
      extranum++;
    else if (ch != ' ') {
      printf("BAD OPTION CHARACTER: %c\n", ch);
      exit(-1);
    }
  }
  fscanf(infile, "%*[^\n]");
  getc(infile);
  for (i = 0; i < chars; i++)
    weight[i] = 1;
  for (i = 1; i <= extranum; i++) {
    ch = getc(infile);
    if (ch == '\n')
      ch = ' ';
      uppercase(&ch);
    if (ch == 'W')
      inputweights();
    else {
      printf("ERROR: INCORRECT AUXILIARY OPTIONS LINE WHICH STARTS WITH %c\n",
	     ch);
      exit(-1);}
  }
  if (weights)
    printweights();
}  /* inputoptions */




Local Void inputdata()
{
  /* input the names and sequences for each species */
  short i, j, k, l, basesread, basesnew;
  Char charstate;
  boolean allread, done;

  


  if (progress)
    putchar('\n');
  j = nmlngth + (chars + (chars - 1) / 10) / 2 - 5;
  if (j < nmlngth - 1)
    j = nmlngth - 1;
  if (j > 37)
    j = 37;
  if (printdata) {   
  }
  basesread = 0;
  allread = false;
  while (!(allread)) {
    allread = true;
    if (Reoln(infile)) {
      fscanf(infile, "%*[^\n]");
      getc(infile);
    }
    i = 1;
    while (i <= spp) 
      {
	if ((interleaved && basesread == 0) || !interleaved) 
	  {
	    for (j = 0; j < nmlngth; j++) 
	      {
		nayme[i - 1][j] = getc(infile);
		if ( eof(infile) | Reoln(infile)){
		  printf("ERROR: END-OF-LINE OR END-OF-FILE");
		  printf(" IN THE MIDDLE OF A SPECIES NAME\n");
		  exit(-1);
		}
	      }
	  }
      if (interleaved)
        j = basesread;
      else
        j = 0;
      done = false;
      while (((!done) & (!eof(infile)))) 
	{
	  if (interleaved)
	    done = true;
	    
	  while (((j < chars) & (!(Reoln(infile) | eof(infile)))))	 
	    {	    
	      charstate = getc(infile);
	      
	      if (charstate == '\n' || charstate == '\t' || charstate == '\0')
		charstate = ' ';
	      if (charstate == ' ' || (charstate >= '0' && charstate <= '9'))
		continue;
	      uppercase(&charstate);	      
	      if ((strchr("ABCDGHKMNRSTUVWXY?O-.",charstate)) == NULL){
		printf("ERROR: BAD BASE:%c AT POSITION%5hd OF SPECIES %3hd\n",
		       charstate, j, i);
		exit(-1);
	      }	      
	      j++;
	      if (charstate == '.')
		charstate = y[0][j - 1];
	      y[i - 1][j - 1] = charstate;	   
	    }
	  if (interleaved)
	    continue;
	  if (j < chars) 
	    {
	      fscanf(infile, "%*[^\n]");
	      getc(infile);
	    } 
	  else 
	    {
	      if (j == chars)
		done = true;
	    }
	}
      if (interleaved && i == 1)
        basesnew = j;
      fscanf(infile, "%*[^\n]");
      getc(infile);
      
      if ((interleaved && j != basesnew) || ((!interleaved) && j != chars))
	{
	 
	  printf("ERROR: SEQUENCES OUT OF ALIGNMENT\n");
	  exit(-1);
	}
      i++;
    }
    if (interleaved) {
      basesread = basesnew;
      allread = (basesread == chars);
    } else
      allread = (i > spp);
  }


  if (!printdata)
    return;

  for (i = 1; i <= ((chars - 1) / 60 + 1); i++) 
    {
    for (j = 1; j <= spp; j++) 
      {
	l = i * 60;
	if (l > chars)
	  l = chars;
	for (k = (i - 1) * 60 + 1; k <= l; k++) 
	  {
	    if (j > 1 && y[j - 1][k - 1] == y[0][k - 1])
	      charstate = '.';
	    else
	      charstate = y[j - 1][k - 1];
	  }
      
      }
    }
}  /* inputdata */












Local Void sitesort()
{
  /* Shell sort keeping sites, weights in same order */
  short gap, i, j, jj, jg, k, itemp;
  boolean flip, tied;

  gap = chars / 2;
  while (gap > 0) {
    for (i = gap + 1; i <= chars; i++) {
      j = i - gap;
      flip = true;
      while (j > 0 && flip) {
        jj = alias[j - 1];
        jg = alias[j + gap - 1];
        tied = true;
        k = 1;
        while (k <= spp && tied) {
          flip = (y[k - 1][jj - 1] > y[k - 1][jg - 1]);
          tied = (tied && y[k - 1][jj - 1] == y[k - 1][jg - 1]);
          k++;
        }
        if (!flip)
          break;
        itemp = alias[j - 1];
        alias[j - 1] = alias[j + gap - 1];
        alias[j + gap - 1] = itemp;
        itemp = weight[j - 1];
        weight[j - 1] = weight[j + gap - 1];
        weight[j + gap - 1] = itemp;
        j -= gap;
      }
    }
    gap /= 2;
  }
}  /* sitesort */

Local Void sitecombine()
{
  /* combine sites that have identical patterns */
  short i, j, k;
  boolean tied;

  i = 1;
  while (i < chars) {
    j = i + 1;
    tied = true;
    while (j <= chars && tied) {
      k = 1;
      while (k <= spp && tied) {
        tied = (tied &&
            y[k - 1][alias[i - 1] - 1] == y[k - 1][alias[j - 1] - 1]);
        k++;
      }
      if (tied) {
        weight[i - 1] += weight[j - 1];
        weight[j - 1] = 0;
        ally[alias[j - 1] - 1] = alias[i - 1];
      }
      j++;
    }
    i = j - 1;
  }
}  /* sitecombine */

Local Void sitescrunch()
{
  /* move so one representative of each pattern of
     sites comes first */
  short i, j, itemp;
  boolean done, found;

  done = false;
  i = 1;
  j = 2;
  while (!done) {
    if (ally[alias[i - 1] - 1] != alias[i - 1]) {
      if (j <= i)
        j = i + 1;
      if (j <= chars) {
        found = false;
        do {
          found = (ally[alias[j - 1] - 1] == alias[j - 1]);
          j++;
        } while (!(found || j > chars));
        if (found) {
          j--;
          itemp = alias[i - 1];
          alias[i - 1] = alias[j - 1];
          alias[j - 1] = itemp;
          itemp = weight[i - 1];
          weight[i - 1] = weight[j - 1];
          weight[j - 1] = itemp;
        } else
          done = true;
      } else
        done = true;
    }
    i++;
    done = (done || i >= chars);
  }
}  /* sitescrunch */

Local Void makeweights()
{
  /* make up weights vector to avoid duplicate computations */
  short i;

  for (i = 1; i <= chars; i++) {
    alias[i - 1] = i;
    oldweight[i - 1] = weight[i - 1];
    ally[i - 1] = i;
  }
  sitesort();
  sitecombine();
  sitescrunch();
  endsite = 0;
  for (i = 1; i <= chars; i++) {
    if (ally[i - 1] == i)
      endsite++;
  }
  for (i = 1; i <= endsite; i++)
    location[alias[i - 1] - 1] = i;
  if (!thresh)
    threshold = spp;
 
  threshwt = (long *)malloc(endsite*sizeof(long));
  
  for (i = 0; i < endsite; i++) {
    weight[i] *= 10;
    threshwt[i] = (long)(threshold * weight[i] + 0.5);
  }
}  /* makeweights */

Local Void makeweightsSubtree()
{
  /* make up weights vector to avoid duplicate computations */
  short i;

  for (i = 1; i <= chars; i++) {
    alias[i - 1] = i;
    oldweight[i - 1] = weight[i - 1];
    ally[i - 1] = i;
  }
  sitesort();
  sitecombine();
  sitescrunch();
  endsite = 0;
  for (i = 1; i <= chars; i++) {
    if (ally[i - 1] == i)
      endsite++;
  }
  for (i = 1; i <= endsite; i++)
    location[alias[i - 1] - 1] = i;
  if (!thresh)
    threshold = spp;
 
  /*  threshwt = (long *)malloc(endsite*sizeof(long));*/
  
  for (i = 0; i < endsite; i++) {
    weight[i] *= 10;
    threshwt[i] = (long)(threshold * weight[i] + 0.5);
  }
}  /* makeweights */

Local Void makevalues()
{
  /* set up fractional likelihoods at tips */
  short i, j;
  short ns;
  node *p;

  for (i = 1; i <= nonodes; i++) {
    treenode[i-1]->back = NULL;
    treenode[i-1]->tip = (i <= spp);
    treenode[i-1]->index = i;
    if (i > spp) {
      p = treenode[i-1]->next;
      while (p != treenode[i-1]) {
        p->back = NULL;
        p->tip = false;
        p->index = i;
        p = p->next;
      }
    }
  }
 
  for (i = 0; i < spp; i++) {
    treenode[i]->numsteps = (stepshortptr)malloc( endsite*sizeof(short));
    treenode[i]->base = (baseptr)malloc( endsite*sizeof(short));
  }
  for (i = spp; i < nonodes; i++) {
    p = treenode[i];
    for (j = 1; j <= 3; j++) {
      p->numsteps = (stepshortptr)malloc(endsite*sizeof(short));
      p->base = (baseptr)malloc(endsite*sizeof(short));
      p = p->next;
    }
  }
  
  for (j = 0; j < endsite; j++) {
    for (i = 0; i < spp; i++) {
      switch (y[i][alias[j] - 1]) {

      case 'A':
        ns = (short)(1 << A);
        break;

      case 'C':
        ns = (short)(1 << C);
        break;

      case 'G':
        ns = (short)(1 << G);
        break;

      case 'U':
        ns = (short)(1 << U);
        break;

      case 'T':
        ns = (short)(1 << U);
        break;

      case 'M':
        ns = ((short)(1 << A)) | ((short)(1 << C));
        break;

      case 'R':
        ns = ((short)(1 << A)) | ((short)(1 << G));
        break;

      case 'W':
        ns = ((short)(1 << A)) | ((short)(1 << U));
        break;

      case 'S':
        ns = ((short)(1 << C)) | ((short)(1 << G));
        break;

      case 'Y':
        ns = ((short)(1 << C)) | ((short)(1 << U));
        break;

      case 'K':
        ns = ((short)(1 << G)) | ((short)(1 << U));
        break;

      case 'B':
        ns = ((short)(1 << C)) | ((short)(1 << G)) | ((short)(1 << U));
        break;

      case 'D':
        ns = ((short)(1 << A)) | ((short)(1 << G)) | ((short)(1 << U));
        break;

      case 'H':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << U));
        break;

      case 'V':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G));
        break;

      case 'N':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G)) |
             ((short)(1 << U));
        break;

      case 'X':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G)) |
             ((short)(1 << U));
        break;

      case '?':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G)) |
             ((short)(1 << U)) | (short)(1 << O);
        break;

      case 'O':
        ns = (short)(1 << O);
        break;

      case '-':
        ns = (short)(1 << O);
        break;
      }
      treenode[i]->base[j] = ns;
      treenode[i]->numsteps[j] = 0;
    }
  }
}  /* makevalues */


Local Void makevaluesSubtree()
{
  /* set up fractional likelihoods at tips */
  short i, j;
  short ns;
  node *p;

  fprintf(stderr,"D1 %d\n", nonodes);
  for (i = 1; i <= nonodes; i++)
    {
    }
  for (i = 1; i <= nonodes; i++) {
    fprintf(stderr, "%d dd\n", i);
    treenode[i-1]->back = NULL;
    treenode[i-1]->tip = (i <= spp);
    treenode[i-1]->index = i;
    if (i > spp) 
      {
	/*fprintf(stderr,"the case\n");
	p = treenode[i-1]->next;
	while (p != treenode[i-1]) 
	  {
	    fprintf(stderr,"the case\n");
	    p->back = NULL;
	    p->tip = false;
	    p->index = i;
	    p = p->next;
	    }*/
      }
  }
  fprintf(stderr,"D2\n");
  /*for (i = 0; i < spp; i++) {
    treenode[i]->numsteps = (stepshortptr)malloc( endsite*sizeof(short));
    treenode[i]->base = (baseptr)malloc( endsite*sizeof(short));
    }
  for (i = spp; i < nonodes; i++) {
    p = treenode[i];
    for (j = 1; j <= 3; j++) {
      p->numsteps = (stepshortptr)malloc(endsite*sizeof(short));
	p->base = (baseptr)malloc(endsite*sizeof(short));
      p = p->next;
    }
    }*/
  fprintf(stderr,"D3\n");
  for (j = 0; j < endsite; j++) {
    for (i = 0; i < spp; i++) {
      switch (y[i][alias[j] - 1]) {

      case 'A':
        ns = (short)(1 << A);
        break;

      case 'C':
        ns = (short)(1 << C);
        break;

      case 'G':
        ns = (short)(1 << G);
        break;

      case 'U':
        ns = (short)(1 << U);
        break;

      case 'T':
        ns = (short)(1 << U);
        break;

      case 'M':
        ns = ((short)(1 << A)) | ((short)(1 << C));
        break;

      case 'R':
        ns = ((short)(1 << A)) | ((short)(1 << G));
        break;

      case 'W':
        ns = ((short)(1 << A)) | ((short)(1 << U));
        break;

      case 'S':
        ns = ((short)(1 << C)) | ((short)(1 << G));
        break;

      case 'Y':
        ns = ((short)(1 << C)) | ((short)(1 << U));
        break;

      case 'K':
        ns = ((short)(1 << G)) | ((short)(1 << U));
        break;

      case 'B':
        ns = ((short)(1 << C)) | ((short)(1 << G)) | ((short)(1 << U));
        break;

      case 'D':
        ns = ((short)(1 << A)) | ((short)(1 << G)) | ((short)(1 << U));
        break;

      case 'H':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << U));
        break;

      case 'V':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G));
        break;

      case 'N':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G)) |
             ((short)(1 << U));
        break;

      case 'X':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G)) |
             ((short)(1 << U));
        break;

      case '?':
        ns = ((short)(1 << A)) | ((short)(1 << C)) | ((short)(1 << G)) |
             ((short)(1 << U)) | (short)(1 << O);
        break;

      case 'O':
        ns = (short)(1 << O);
        break;

      case '-':
        ns = (short)(1 << O);
        break;
      }
      treenode[i]->base[j] = ns;
      treenode[i]->numsteps[j] = 0;
    }
  }
}  /* makevalues */


Static Void doinput()
{

  int i;
 
  

  for (i = 0; i < chars; i++)
    weight[i] = 1;


 
 
  inputdata();
  
  makeweights();

  makevalues();
}  /* doinput */



     
Local Void fillin(p, left, rt)
node *p, *left, *rt;
{
  /* sets up for each node in the tree the base sequence
     at that point and counts the changes.  The program
     spends much of its time in this PROCEDURE */
  short i;
  short ns, rs, ls;



#pragma omp parallel for private(ns, rs, ls) 
  for (i = 0; i < endsite; i++) 
    {
      if (left) ls = left->base[i];
      if (rt) rs = rt->base[i];
      if (!left) 
	{
	  ns = rs;
	  p->numsteps[i] = rt->numsteps[i];
	} 
      else if (!rt) 
	{
	  ns = ls;
	  p->numsteps[i] = left->numsteps[i];
	} 
      else 
	{
	  ns = ls & rs;
	  p->numsteps[i] = left->numsteps[i] + rt->numsteps[i];
	}
      if (ns == 0) 
	{
	  if (left && rt) ns = ls | rs;
	  p->numsteps[i] += weight[i];
	}
      p->base[i] = ns;
    }
 
 
}  /* fillin */

Local Void preorder(p)
node *p;
{
  /* recompute number of steps in preorder taking both ancestoral and
     descendent steps into account */

  if (p && !p->tip) {
    fillin (p->next, p->next->next->back, p->back);
    fillin (p->next->next, p->back, p->next->back);
    preorder (p->next->back);
    preorder (p->next->next->back);
  }
} /* preorder */

Local Void add(below, newtip, newfork)
node *below, *newtip, *newfork;
{
  /* inserts the nodes newfork and its left descendant, newtip,
     to the tree.  below becomes newfork's right descendant */

  if (below != treenode[below->index - 1])
    below = treenode[below->index - 1];
  if (below->back != NULL)
    below->back->back = newfork;
  newfork->back = below->back;
  below->back = newfork->next->next;
  newfork->next->next->back = below;
  newfork->next->back = newtip;
  newtip->back = newfork->next;
  if (root == below)
    root = newfork;
  root->back = NULL;
  if (!recompute)
    return;
  fillin(newfork, newfork->next->back, newfork->next->next->back);
  preorder(newfork);
  if (newfork != root)
    preorder(newfork->back);
}  /* add */

Local Void re_move(item, fork)
node **item, **fork;
{
  /* removes nodes item and its ancestor, fork, from the tree.
     the new descendant of fork's ancestor is made to be
     fork's second descendant (other than item).  Also
     returns pointers to the deleted nodes, item and fork */
  node *p, *q, *other;

  if ((*item)->back == NULL) {
    *fork = NULL;
    return;
  }
  *fork = treenode[(*item)->back->index - 1];
  if (*item == (*fork)->next->back)
    other = (*fork)->next->next->back;
  else
    other = (*fork)->next->back;
  if (root == *fork)
    root = other;
  p = (*item)->back->next->back;
  q = (*item)->back->next->next->back;
  if (p != NULL)
    p->back = q;
  if (q != NULL)
    q->back = p;
  (*fork)->back = NULL;
  p = (*fork)->next;
  while (p != *fork) {
    p->back = NULL;
    p = p->next;
    (*item)->back = NULL;
  }
  if (!recompute)
    return;
  preorder(other);
  if (other != root)
    preorder(other->back);
}  /* remove */

Local Void evaluate(r)
node *r;
{
  /* determines the number of steps needed for a tree. this is
     the minimum number of steps needed to evolve sequences on
     this tree */
  short i, steps;
  long term;
  double sum;

  sum = 0.0;
  for (i = 0; i < endsite; i++) {
    steps = r->numsteps[i];
    if ((long)steps <= threshwt[i])
      term = steps;
    else
      term = threshwt[i];
    sum += term;
    if (usertree && which <= maxuser)
      fsteps[which - 1][i] = term;
  }
  if (usertree && which <= maxuser) {
    nsteps[which - 1] = sum;
    if (which == 1) {
      minwhich = 1;
      minsteps = sum;
    } else if (sum < minsteps) {
      minwhich = which;
      minsteps = sum;
    }
  }
  like = -sum;
}  /* evaluate */

Local Void postorder(p)
node *p;
{
  /* traverses a binary tree, calling PROCEDURE fillin at a
     node's descendants before calling fillin at the node */
  if (p->tip)
    return;
  postorder(p->next->back);
  postorder(p->next->next->back);
  fillin(p, p->next->back, p->next->next->back);
}  /* postorder */

Local Void reroot(outgroup)
node *outgroup;
{
  /* reorients tree, putting outgroup in desired position. */
  node *p, *q;

  if (outgroup->back->index == root->index)
    return;
  p = root->next;
  q = root->next->next;
  p->back->back = q->back;
  q->back->back = p->back;
  p->back = outgroup;
  q->back = outgroup->back;
  outgroup->back->back = root->next->next;
  outgroup->back = root->next;
}  /* reroot */

Local Void savetraverse(p)
node *p;
{
  /* sets BOOLEANs that indicate which way is down */
  p->bottom = true;
  if (p->tip)
    return;
  p->next->bottom = false;
  savetraverse(p->next->back);
  p->next->next->bottom = false;
  savetraverse(p->next->next->back);
}  /* savetraverse */

Local Void savetree()
{
  /* record in place where each species has to be
     added to reconstruct this tree */
  short i, j;
  node *p;
  boolean done;

  reroot(treenode[outgrno - 1]);
  savetraverse(root);
  for (i = 0; i < nonodes; i++)
    place[i] = 0;
  place[root->index - 1] = 1;
  for (i = 1; i <= spp; i++) {
    p = treenode[i - 1];
    while (place[p->index - 1] == 0) {
      place[p->index - 1] = i;
      while (!p->bottom)
        p = p->next;
      p = p->back;
    }
    if (i > 1) {
      place[i - 1] = place[p->index - 1];
      j = place[p->index - 1];
      done = false;
      while (!done) {
        place[p->index - 1] = spp + i - 1;
        while (!p->bottom)
          p = p->next;
        p = p->back;
        done = (p == NULL);
        if (!done)
          done = (place[p->index - 1] != j);
      }
    }
  }
}  /* savetree */

Local Void findtree(found,pos)
boolean *found;
short *pos;
{
  /* finds tree given by ARRAY place in ARRAY
     bestrees by binary search */
  short i, lower, upper;
  boolean below, done;

  below = false;
  lower = 1;
  upper = nextree - 1;
  (*found) = false;
  while (!(*found) && lower <= upper) {
    (*pos) = (lower + upper) / 2;
    i = 3;
    done = false;
    while (!done) {
      done = (i > spp);
      if (!done)
        done = (place[i - 1] != bestrees[(*pos) - 1][i - 1]);
      if (!done)
        i++;
    }
    (*found) = (i > spp);
    below = (place[i - 1] <  bestrees[(*pos )- 1][i - 1]);
    if (*found)
      break;
    if (below)
      upper = (*pos) - 1;
    else
      lower = (*pos) + 1;
  }
  if (!(*found) && !below)
    (*pos)++;
}  /* findtree */

Local Void addtree(pos)
short pos;
{
  /* puts tree from ARRAY place in its proper position
     in ARRAY bestrees */
  short i;

  for (i = nextree - 1; i >= pos; i--)
    memcpy(bestrees[i], bestrees[i - 1], nonodes * sizeof(short));
  for (i = 0; i < spp; i++)
    bestrees[pos - 1][i] = place[i];
  nextree++;
}  /* addtree */

Local Void tryadd(p, item,nufork)
node *p,**item,**nufork;
{
  /* temporarily adds one fork and one tip to the tree.
     if the location where they are added yields greater
     "likelihood" than other locations tested up to that
     time, then keeps that location as there */
  short pos;
  boolean found;
  node *rute, *q;

  if (p == root)
    fillin(temp, *item, p);
  else {
    fillin(temp1, *item, p);
    fillin(temp, temp1, p->back);
  }
  evaluate(temp);
  if (lastrearr) {
    if (like < bestlike) {
      if ((*item) == (*nufork)->next->next->back) {
        q = (*nufork)->next;
        (*nufork)->next = (*nufork)->next->next;
        (*nufork)->next->next = q;
        q->next = (*nufork);
      }
    } else if (like >= bstlike2) {
      recompute = false;
      add(p, *item,*nufork);
      rute = root->next->back;
      savetree();
      reroot(rute);
      if (like > bstlike2) {
        bestlike = bstlike2 = like;
        pos = 1;
        nextree = 1;
        addtree(pos);
      } else {
        pos = 0;
        findtree(&found,&pos);
        if (!found) {
          if (nextree <= maxtrees)
            addtree(pos);
        }
      }
      re_move(item, nufork);
      recompute = true;
    }
  }
  if (like > bestyet) {
    bestyet = like;
    there = p;
  }
}  /* tryadd */

Local Void addpreorder(p, item, nufork)
node *p, *item, *nufork;
{
  /* traverses a binary tree, calling PROCEDURE tryadd
     at a node before calling tryadd at its descendants */

  if (p == NULL)
    return;
  tryadd(p, &item,&nufork);
  if (!p->tip) {
    addpreorder(p->next->back, item, nufork);
    addpreorder(p->next->next->back, item, nufork);
  }
}  /* addpreorder */



Local Void tryrearr(p, success)
node *p;
boolean *success;

{
  /* evaluates one rearrangement of the tree.
     if the new tree has greater "likelihood" than the old
     one sets success = TRUE and keeps the new tree.
     otherwise, restores the old tree */

  

  node *frombelow, *whereto, *forknode, *q;
  double oldlike;

  fprintf(stderr,"trying rearrange\n");
  if (p->back == NULL)
    return;
  forknode = treenode[p->back->index - 1];
  if (forknode->back == NULL)
    return;
  oldlike = bestyet;
  if (p->back->next->next == forknode)
    frombelow = forknode->next->next->back;
  else
    frombelow = forknode->next->back;
  whereto = treenode[forknode->back->index - 1];
  if (whereto->next->back == forknode)
    q = whereto->next->next->back;
  else
    q = whereto->next->back;
  fillin(temp1, frombelow, q);
  fillin(temp, temp1, p);
  fillin(temp1, temp, whereto->back);
  evaluate(temp1);
  if (like <= oldlike) {
    if (p != forknode->next->next->back)
      return;
    q = forknode->next;
    forknode->next = forknode->next->next;
    forknode->next->next = q;
    q->next = forknode;
    return;
  }
  recompute = false;
  re_move(&p, &forknode);
  fillin(whereto, whereto->next->back, whereto->next->next->back);
  recompute = true;
  add(whereto, p, forknode);
  (*success) = true;
  bestyet = like;
}  /* tryrearr */

Local Void repreorder(p, success)
node *p;
boolean *success;
{
  /* traverses a binary tree, calling PROCEDURE tryrearr
     at a node before calling tryrearr at its descendants */
  if (p == NULL)
    return;
  tryrearr(p, success);
  if (!p->tip) {
    repreorder(p->next->back,success);
    repreorder(p->next->next->back,success);
  }
}  /* repreorder */

Local Void rearrange(r)
node **r;
{
  /* traverses the tree (preorder), finding any local
     rearrangement which decreases the number of steps.
     if traversal succeeds in increasing the tree's
     "likelihood", PROCEDURE rearrange runs traversal again */
  boolean success=true;

  fprintf(stderr,"call rearrange\n");

  while (success) {
    success = false;
    repreorder(*r, &success);
  }
}  /* rearrange */


char *phylipTreeString;

Local Void findch(c)
Char c;
{
  /* scan forward until find character c */
  boolean done;

  done = false;
  while (!(done)) {
    if (c == ',') {
      if (ch == '(' || ch == ')' || ch == ';') {
        printf("\nERROR IN USER TREE%3hd: UNMATCHED PARENTHESIS OR MISSING COMMA\n",  which);
	exit(-1);
      } else if (ch == ',')
        done = true;
    } else if (c == ')') {
      if (ch == '(' || ch == ',' || ch == ';') {
        printf("\nERROR IN USER TREE%3hd: ",which);
	printf("UNMATCHED PARENTHESIS OR NON-BIFURCATED NODE\n");
	exit(-1);
      } else {
        if (ch == ')')
          done = true;
      }
    } else if (c == ';') {
      if (ch != ';') {
        printf("\nERROR IN USER TREE%3hd: ",which);
	printf("UNMATCHED PARENTHESIS OR MISSING SEMICOLON\n");
	exit(-1);
      } else
        done = true;
    }
    if (ch != ')' && done)
      continue;
    if (Reoln(infile)) {
      fscanf(infile, "%*[^\n]");
      getc(infile);
    }
    ch = getc(infile);
    if (ch == '\n')
      ch = ' ';
  }
}  /* findch */

Local Void addelement(p,nextnode,lparens,names)
node **p;
short *nextnode,*lparens;
boolean *names;

{
  /* recursive procedure adds nodes to user-defined tree */
  node *q;
  short i, n;
  boolean found;
  Char str[nmlngth];

  do {
    if (Reoln(infile)) {
      fscanf(infile, "%*[^\n]");
      getc(infile);
    }
    ch = getc(infile);
    if (ch == '\n')
      ch = ' ';
  } while (ch == ' ');
  if (ch == '(' ) {
    if ((*lparens) >= spp - 1) {
      printf("\nERROR IN USER TREE: TOO MANY LEFT PARENTHESES\n");
      exit(-1);
    }
    (*nextnode)++;
    (*lparens)++;
    q = treenode[(*nextnode) - 1];
    addelement(&q->next->back, nextnode,lparens,names);
    q->next->back->back = q->next;
    findch(',');
    addelement(&q->next->next->back,nextnode,lparens,names);
    q->next->next->back->back = q->next->next;
    findch(')');
    *p = q;
    return;
  }
  for (i = 0; i < nmlngth; i++)
    str[i] = ' ';
  n = 1;
  do {
    if (ch == '_')
      ch = ' ';
    str[n - 1] = ch;
    if (Reoln(infile)) {
      fscanf(infile, "%*[^\n]");
      getc(infile);
    }
    ch = getc(infile);
    if (ch == '\n')
     ch = ' ';
    n++;
  } while (ch != ',' && ch != ')' && ch != ':' && n <= nmlngth);
  n = 1;
  do {
    found = true;
    for (i = 0; i < nmlngth; i++)
      found = (found && str[i] == nayme[n - 1][i]);
    if (found) {
      if (names[n - 1] == false) {
        *p = treenode[n - 1];
        names[n - 1] = true;
      } else {
        printf("\nERROR IN USER TREE: DUPLICATE NAME FOUND -- ");
        for (i = 0; i < nmlngth; i++)
          putchar(nayme[n - 1][i]);
        putchar('\n');
	exit(-1);
      }
    } else
      n++;
  } while (!(n > spp || found));
  if (n <= spp)
    return;
  printf("\nCannot find species: ");
  for (i = 0; i < nmlngth; i++)
    putchar(str[i]);
  putchar('\n');
}  /* addelement */



Local Void treeread()
{
  /* read in user-defined tree and set it up */
  short i, nextnode,lparens;

  root = treenode[spp];
  nextnode = spp;
  root->back = NULL;
  for (i = 0; i < spp; i++)
    names[i] = false;
  lparens = 0;
  addelement(&root, &nextnode,&lparens,names);
  findch(';');
  fscanf(infile, "%*[^\n]");  
  getc(infile);
}  /* treeread */


Local Void coordinates(p,tipy)
node *p;
short *tipy;
{
  /* establishes coordinates of nodes */
  if (p->tip) {
    p->xcoord = 0;
    p->ycoord = (*tipy);
    p->ymin = (*tipy);
    p->ymax = (*tipy);
    (*tipy) += down;
    return;
  }
  coordinates(p->next->back, tipy);
  coordinates(p->next->next->back, tipy);
  p->xcoord = p->next->next->back->ymax - p->next->back->ymin;
  p->ycoord = (p->next->back->ycoord + p->next->next->back->ycoord) / 2;
  p->ymin = p->next->back->ymin;
  p->ymax = p->next->next->back->ymax;
}  /* coordinates */

Local Void drawline(i, scale)
short i;
double scale;
{
  /* draws one row of the tree diagram by moving up tree */
  node *p, *q;
  short n, j;
  boolean extra, done;

  p = root;
  q = root;
  extra = false;
  if (i == p->ycoord && p == root) {
    
    extra = true;
  } 
    
  do {
    if (!p->tip) {
      if (i >= p->next->back->ymin && i <= p->next->back->ymax)
        q = p->next->back;
      if (i >= p->next->next->back->ymin && i <= p->next->next->back->ymax)
        q = p->next->next->back;
    }
    done = (p == q);
    n = (short)(scale * (p->xcoord - q->xcoord) + 0.5);
    if (n < 3 && !q->tip)
      n = 3;
    if (extra) {
      n--;
      extra = false;
    }
    if (q->ycoord == i && !done) {
      if (!q->tip) {
       
        extra = true;
      } else {
      }
    } else if (!p->tip) {
      if (p->next->next->back->ycoord > i && p->next->back->ycoord < i
            && i != p->ycoord) {
      } else {
      }
    } else {
    }
    if (p != q)
      p = q;
  } while (!done);
  if (p->ycoord == i && p->tip) {
  }
}  /* drawline */

Local Void printree()
{
  /* prints out diagram of the tree */
  short tipy;
  double scale;
  short i;

  if (!treeprint)
    return;
  tipy = 1;
  coordinates(root, &tipy);
  scale = 1.5;
  for (i = 1; i <= (tipy - down); i++)
    drawline(i, scale);
}  /* printree */



/* Local variables for hypstates: */
struct LOC_hypstates {
  boolean bottom;
} ;

Local Void ancestset(a, b, c)
short *a, *b, *c;
{
  /* make the set of ancestral states below nodes
     whose base sets are a and b */
  *c = (*a) & (*b);
  if (*c == 0)
    *c = (*a) | (*b);
}  /* ancestset */

/* Local variables for hyptrav: */
struct LOC_hyptrav {
  node *r;
  short *hypset;
  boolean maybe, nonzero;
  short tempset, anc;
} ;

Local Void hyprint(b1, b2, htrav,hyps)
short b1, b2;
struct LOC_hyptrav   *htrav; /* variables from hyptrav */
struct LOC_hypstates *hyps;  /* variables from hypstates */
{
  /* print out states in sites b1 through b2 at node */
  short i, j, k, n;
  boolean dot;
  bases b;

  
   
  
 
}  /* hyprint */


Local Void hyptrav(r_, hypset_, b1, b2, hyps)
node *r_;
short *hypset_;
short b1, b2;
struct LOC_hypstates *hyps;
{
  /*  compute, print out states at one interior node */
  struct LOC_hyptrav HyptravV;
  short i, j;
  short left, rt;
  gbases *temparray, *ancset;

  HyptravV.r = r_;
  HyptravV.hypset = hypset_;
  gnu(&ancset);
  gnu(&temparray);
  HyptravV.maybe = false;
  HyptravV.nonzero = false;
  for (i = b1 - 1; i < b2; i++) {
    j = location[ally[i] - 1];
    HyptravV.anc = HyptravV.hypset[j - 1];
    if (!HyptravV.r->tip) {
      left = HyptravV.r->next->back->base[j - 1];
      rt = HyptravV.r->next->next->back->base[j - 1];
      HyptravV.tempset = left & rt & HyptravV.anc;
      if (HyptravV.tempset == 0) {
        HyptravV.tempset = (left & rt) | (left & HyptravV.anc) | (rt & HyptravV.anc);
        if (HyptravV.tempset == 0)
          HyptravV.tempset = left | rt | HyptravV.anc;
      }
      HyptravV.r->base[j - 1] = HyptravV.tempset;
    }
    if (!hyps->bottom)
      HyptravV.anc = treenode[HyptravV.r->back->index - 1]->base[j - 1];
    HyptravV.nonzero = (HyptravV.nonzero ||
                          (HyptravV.r->base[j - 1] & HyptravV.anc) == 0);
    HyptravV.maybe = (HyptravV.maybe || HyptravV.r->base[j - 1] != HyptravV.anc);
  }
  hyprint(b1, b2, &HyptravV,hyps);
  hyps->bottom = false;
  if (!HyptravV.r->tip) {
    memcpy(temparray->base, HyptravV.r->next->back->base, endsite*sizeof(short));
    for (i = b1 - 1; i < b2; i++) {
      j = location[ally[i] - 1];
      ancestset(&HyptravV.hypset[j-1],
                  &HyptravV.r->next->next->back->base[j-1],&ancset->base[j-1]);
    }
    hyptrav(HyptravV.r->next->back, ancset->base, b1, b2, hyps);
    for (i = b1 - 1; i < b2; i++) {
      j = location[ally[i] - 1];
      ancestset(&HyptravV.hypset[j-1], &temparray->base[j-1],&ancset->base[j-1]);
    }
    hyptrav(HyptravV.r->next->next->back, ancset->base, b1, b2, hyps);
  }
  Rchuck(temparray);
  Rchuck(ancset);
}  /* HyptravV */

Local Void hypstates()
{
  /* fill in and describestates at interior nodes */
  struct LOC_hypstates Vars;
  short i, n;

 
  nothing = (baseptr)malloc( endsite*sizeof(short));
  for (i = 0; i < endsite; i++)
    nothing[i] = 0;
  for (i = 1; i <= ((chars -1) / 40 + 1); i++) {
    Vars.bottom = true;
    n = i * 40;
    if (n > chars)
      n = chars;
    hyptrav(root, nothing, i * 40 - 39, n, &Vars);
  }
  free(nothing);
}  /* hypstates */

Local Void treeout(p)
node *p;
{
  /* write out file with representation of final tree */
  short i, n;
  Char c;

  if (p->tip) {
    n = 10;
    fprintf(treefile, "%s", nayme[p->index - 1]);
    /*
    for (i = 1; i <= nmlngth; i++) {
      if (nayme[p->index - 1][i - 1] != ' ')
        n = i;
    }
    for (i = 0; i < n; i++) {
      c = nayme[p->index - 1][i];
      if (c == ' ')
        c = '_';
      putc(c, treefile);
      }*/
    col += n;
  } else {
    putc('(', treefile);
    col++;
    treeout(p->next->back);
    putc(',', treefile);
    col++;
    if (col > 65) {
      putc('\n', treefile);
      col = 0;
    }
    treeout(p->next->next->back);
    putc(')', treefile);
    col++;
  }
  if (p != root)
    return;
  if (nextree > 2)
    fprintf(treefile, "[%6.4f];\n", 1.0 / (nextree - 1));
  else
    fprintf(treefile, ";\n");
}  /* treeout */


void treeout2(node *p)
{
  /* write out file with representation of final tree */
  short i;
  char *nameptr, c;
  

  if (p->tip) 
    {       
      i = 0; 
      *phylipTreeString++ = '\'';
      while(nayme[p->index - 1][i] != '\0')
	{
	  *phylipTreeString++= nayme[p->index - 1][i];
	  i++;
	}      
      *phylipTreeString++ = '\'';      
    }    
  else 
    {
      *phylipTreeString++ = '(';      
      treeout2(p->next->back);
      *phylipTreeString++= ',';           
      treeout2(p->next->next->back);          
      *phylipTreeString++ = ')';
    }
  if (p != root)
    {
      *phylipTreeString++ = ':';
      *phylipTreeString++ = ' ';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '.';
      *phylipTreeString++ = '9';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '0';
      return;
    }
  if (nextree > 2)
    {
      *phylipTreeString++ = ':';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '.';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = ';';
      *phylipTreeString++ = '\n';
    }
  else
    {
      *phylipTreeString++ = ':';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = '.';
      *phylipTreeString++ = '0';
      *phylipTreeString++ = ';';
      *phylipTreeString++ = '\n';
    }
}  /* treeout */


Local Void describe()
{
  /* prints ancestors, steps and table of numbers of steps in
     each site */
  short i, j, k, l;
 

  
  strcpy(phylipTreeString, "[&&AxMl: version = '1.1.0', likelihood = unlikely, ntaxa = 150, opt_level = 0, smoothed = 0]");
  phylipTreeString = &phylipTreeString[strlen("[&&AxMl: version = '1.1.0', likelihood = unlikely, ntaxa = 150, opt_level = 0, smoothed = 0]")];
  treeout2(root);  
  

}  /* describe */




Static Void maketree(int *perm)
{
  /* constructs a binary tree from the pointers in treenode.
     adds each node at location which yields highest "likelihood"
     then rearranges the tree for greatest "likelihood" */
  short i, j, k, numtrees, num;
  double gotlike, wt, sumw, sum, sum2, sd;
  node *item, *nufork, *dummy;
  double TEMP;

  temp = (node *)malloc( sizeof(node));
  temp->numsteps = (stepshortptr)malloc( endsite*sizeof(short));
  temp->base = (baseptr)malloc( endsite*sizeof(short));
  temp1 = (node *)malloc( sizeof(node));
  temp1->numsteps = (stepshortptr)malloc( endsite*sizeof(short));
  temp1->base = (baseptr)malloc( endsite*sizeof(short));

  /* RAxML */

  for(i = 1; i <= spp; i++)
    enterorder[i-1] = perm[i];

  /* RAxML */

  if (!usertree) 
    {
      recompute = true;
      for (i = 1; i <= spp; i++)
	enterorder[i - 1] = i;
      for(i = 1; i <= spp; i++)
	enterorder[i-1] = perm[i];
      if (jumble) {
	for (i = 0; i < spp; i++) {
	  j = (short)(Rrandum(seed) * spp) + 1;
	  k = enterorder[j - 1];
	  enterorder[j - 1] = enterorder[i];
	  enterorder[i] = k;
	}
      }
      root = treenode[enterorder[0] - 1];
      add(treenode[enterorder[0] - 1], treenode[enterorder[1] - 1],
	  treenode[spp]);   
      lastrearr = false;
      for (i = 3; i <= spp; i++) 
	{
	  bestyet = -10.0 * spp * chars;
	  item = treenode[enterorder[i - 1] - 1];
	  nufork = treenode[spp + i - 2];
	  there = root;
	  addpreorder(root, item, nufork);
	  add(there, item, nufork);
	  like = bestyet;     
	  lastrearr = (i == spp);
	  if (lastrearr) 
	    {	 	    
	        
	      bestlike = bestyet;
	      if (jumb == 1) 
		{
		  bstlike2 = bestlike;
		  nextree = 1;
		}
	     				 	      
	      gotlike = bestlike;
	      /*RAxML: das war frueher eine Schleife bis zur konvergenz*/

	      for (j = 0; j < nonodes; j++) 
		{
		  there = root;
		  bestyet = -10.0 * spp * chars;
		  item = treenode[j];
		  if (item != root) 
		    {
		      re_move(&item, &nufork);
		      there = root;
		      addpreorder(root, item, nufork);
		      add(there, item, nufork);
		    }
		}		  	    		  
	    }
	}    
      for (i = spp - 1; i >= 1; i--)
	re_move(&treenode[i], &dummy);
      if (jumb == njumble) 
	{     
	  if (nextree > maxtrees + 1) 
	    {	      
	      nextree = maxtrees + 1;
	    }
	  recompute = false;

	  /*RAxML: nur erster baum der liste*/
	  for (i = 0; i < 1/*<= (nextree - 2);*/; i++) 
	    {
	      root = treenode[0];
	      add(treenode[0], treenode[1], treenode[spp]);
	      for (j = 3; j <= spp; j++)
		add(treenode[bestrees[i][j - 1] - 1], treenode[j - 1],
		    treenode[spp + j - 2]);
	      reroot(treenode[outgrno - 1]);
	      postorder(root);
	      evaluate(root);
	      printree();	
	      describe();	      
	      for (j = 1; j < spp; j++)
		re_move(&treenode[j], &dummy);
	    }
	}
    }
  
  if (jumb == njumble) 
    {      
      free(temp->numsteps);
      free(temp->base);
      free(temp);
      free(temp1->numsteps);
      free(temp1->base);
      free(temp1);
    }
    

}  /* maketree */


Static Void makesubtree()
{
  /* constructs a binary tree from the pointers in treenode.
     adds each node at location which yields highest "likelihood"
     then rearranges the tree for greatest "likelihood" */
  short i, j, k, numtrees, num;
  double gotlike, wt, sumw, sum, sum2, sd;
  node *item, *nufork, *dummy;
  double TEMP;

  temp = (node *)malloc( sizeof(node));
  temp->numsteps = (stepshortptr)malloc( endsite*sizeof(short));
  temp->base = (baseptr)malloc( endsite*sizeof(short));
  temp1 = (node *)malloc( sizeof(node));
  temp1->numsteps = (stepshortptr)malloc( endsite*sizeof(short));
  temp1->base = (baseptr)malloc( endsite*sizeof(short));

  /* RAxML */

  /* RAxML */

  if (!usertree) 
    {
      recompute = true;
      for (i = 1; i <= spp; i++)
	enterorder[i - 1] = i;
      if (jumble) {
	for (i = 0; i < spp; i++) {
	  j = (short)(Rrandum(seed) * spp) + 1;
	  k = enterorder[j - 1];
	  enterorder[j - 1] = enterorder[i];
	  enterorder[i] = k;
	}
      }
      root = treenode[enterorder[0] - 1];
      add(treenode[enterorder[0] - 1], treenode[enterorder[1] - 1],
	  treenode[spp]);   
      lastrearr = false;
      for (i = 3; i <= spp; i++) 
	{
	  bestyet = -10.0 * spp * chars;
	  item = treenode[enterorder[i - 1] - 1];
	  nufork = treenode[spp + i - 2];
	  there = root;
	  addpreorder(root, item, nufork);
	  add(there, item, nufork);
	  like = bestyet;     
	  lastrearr = (i == spp);
	  if (lastrearr) 
	    {	 	    
	        
	      bestlike = bestyet;
	      if (jumb == 1) 
		{
		  bstlike2 = bestlike;
		  nextree = 1;
		}
	     				 	      
	      gotlike = bestlike;
	      /*RAxML: das war frueher eine Schleife bis zur konvergenz*/

	      for (j = 0; j < nonodes; j++) 
		{
		  there = root;
		  bestyet = -10.0 * spp * chars;
		  item = treenode[j];
		  if (item != root) 
		    {
		      re_move(&item, &nufork);
		      there = root;
		      addpreorder(root, item, nufork);
		      add(there, item, nufork);
		    }
		}		  	    		  
	    }
	}    
      for (i = spp - 1; i >= 1; i--)
	re_move(&treenode[i], &dummy);
      if (jumb == njumble) 
	{     
	  if (nextree > maxtrees + 1) 
	    {	      
	      nextree = maxtrees + 1;
	    }
	  recompute = false;

	  /*RAxML: nur erster baum der liste*/
	  for (i = 0; i < 1/*<= (nextree - 2);*/; i++) 
	    {
	      root = treenode[0];
	      add(treenode[0], treenode[1], treenode[spp]);
	      for (j = 3; j <= spp; j++)
		add(treenode[bestrees[i][j - 1] - 1], treenode[j - 1],
		    treenode[spp + j - 2]);
	      reroot(treenode[outgrno - 1]);
	      postorder(root);
	      evaluate(root);
	      printree();	
	      describe();	      
	      for (j = 1; j < spp; j++)
		re_move(&treenode[j], &dummy);
	    }
	}
    }
  
  if (jumb == njumble) 
    {      
      free(temp->numsteps);
      free(temp->base);
      free(temp);
      free(temp1->numsteps);
      free(temp1->base);
      free(temp1);
    }
    

}  /* maketree */









void RAxML_parsimony_init(int number_taxa, int number_patterns, char **y_axml, char **node_names)
{
  short i;
  node *p, *q;
  char inverseMap[21] = {'z', 'A', 'C','M','G','R','S','V', 'T', 'W', 'Y','H','K','D','B','N','O','X','?','-'};
  spp = number_taxa;
  chars = number_patterns;
  nonodes = spp * 2 - 1;

  getoptions();
    
  y = (Char **)malloc(spp*sizeof(Char *));
  for (i = 0; i < spp; i++)
    y[i] = (Char *)malloc(chars*sizeof(Char));
  treenode = (pointptr)malloc(nonodes*sizeof(node *));
  for (i = 0; i < spp; i++)
    treenode[i] = (node *)malloc(sizeof(node));
  for (i = spp; i < nonodes; i++) 
    {
      q = NULL;
      for (j = 1; j <= 3; j++) {
	p = (node *)malloc(sizeof(node));
	p->next = q;
	q = p;
      }
      p->next->next->next = p;
      treenode[i] = p;
    }

   ibmpc = ibmpc0;
   ansi = ansi0;
   vt52 = vt520;
   mulsets = false;
   datasets = 1;
   firstset = true;
   garbage = NULL;
   bestrees = (short **)malloc(maxtrees*sizeof(short *));
   for (j = 1; j <= maxtrees; j++)
     bestrees[j - 1] = (short *)malloc( nonodes*sizeof(short));
   nayme = (Char **)malloc( spp*sizeof(Char *));
   
   enterorder = (short *)malloc( spp*sizeof(short));
   names = (boolean *)malloc( spp*sizeof(boolean));
   place = (short *)malloc( nonodes*sizeof(short));
   weight = (short *)malloc( chars*sizeof(short));
   oldweight = (short *)malloc( chars*sizeof(short));
   alias = (short *)malloc( chars*sizeof(short));
   ally = (short *)malloc( chars*sizeof(short));
   location = (short *)malloc( chars*sizeof(short));
   
  
   for (i = 0; i < chars; i++)
     weight[i] = 1;

  for (i = 1; i <= spp; i++)    
    nayme[i-1] = node_names[i];     

  for(i = 0; i < spp; i++)
    for(j = 0; j < chars; j++)
      y[i][j] = inverseMap[y_axml[i+1][j+1]];

  makeweights();
  makevalues();
}  /* doinit*/


void RAxML_parsimony_Subtree_init(int number_taxa, int number_patterns, char **y_axml, 
				  char **node_names, int *filter, int all_taxa)
{
 short i;
 short k;
  node *p, *q;
  char inverseMap[21] = {'z', 'A', 'C','M','G','R','S','V', 'T', 'W', 'Y','H','K','D','B','N','O','X','?','-'};
  spp = number_taxa;
  chars = number_patterns;
  nonodes = spp * 2 - 1;
  
  getoptions();
    
  y = (Char **)malloc(spp*sizeof(Char *));
  for (i = 0; i < spp; i++)
    y[i] = (Char *)malloc(chars*sizeof(Char));
  treenode = (pointptr)malloc(nonodes*sizeof(node *));
  for (i = 0; i < spp; i++)
    treenode[i] = (node *)malloc(sizeof(node));
  for (i = spp; i < nonodes; i++) 
    {
      q = NULL;
      for (j = 1; j <= 3; j++) {
	p = (node *)malloc(sizeof(node));
	p->next = q;
	q = p;
      }
      p->next->next->next = p;
      treenode[i] = p;
    }

   ibmpc = ibmpc0;
   ansi = ansi0;
   vt52 = vt520;
   mulsets = false;
   datasets = 1;
   firstset = true;
   garbage = NULL;
   bestrees = (short **)malloc(maxtrees*sizeof(short *));
   for (j = 1; j <= maxtrees; j++)
     bestrees[j - 1] = (short *)malloc( nonodes*sizeof(short));
   nayme = (Char **)malloc( spp*sizeof(Char *));
   
   enterorder = (short *)malloc( spp*sizeof(short));
   names = (boolean *)malloc( spp*sizeof(boolean));
   place = (short *)malloc( nonodes*sizeof(short));
   weight = (short *)malloc( chars*sizeof(short));
   oldweight = (short *)malloc( chars*sizeof(short));
   alias = (short *)malloc( chars*sizeof(short));
   ally = (short *)malloc( chars*sizeof(short));
   location = (short *)malloc( chars*sizeof(short));
   
  
   for (i = 0; i < chars; i++)
     weight[i] = 1;

   k = 0;
   for (i = 1; i <= all_taxa; i++) 
     {
       if(filter[i])
	 {
	   nayme[k] = node_names[i];     
	   k++;
	 }
     }
   k = 0;
  for(i = 0; i < all_taxa; i++)
    {
      if(filter[i + 1])
	{
	  for(j = 0; j < chars; j++)
	    y[k][j] = inverseMap[y_axml[i+1][j+1]];
	  k++;
	}
    }

  makeweights();
  makevalues(); 
}  













void RAxML_parsimony_cleanup()
{
  for (i = 0; i < spp; i++) 
    {
      free(treenode[i]->numsteps);
      free(treenode[i]->base);
    }
  for (i = spp; i < nonodes; i++) 
    {
      p = treenode[i];
      for (j = 1; j <= 3; j++) 
	{
	  free(p->numsteps);
	  free(p->base);
	  p = p->next;
	}
    }
  free(threshwt);
  

  for (i = 0; i < spp; i++)
    free(y[i]);
  free(y);
  for (i = spp; i < nonodes; i++) 
    {
      free(treenode[i]->next->next);
      free(treenode[i]->next);
    }
  for (i = 0; i < nonodes; i++)
    {
      free(treenode[i]);
    }
  free(treenode);

  for(i = 1; i <= maxtrees; i++)
    {
      if(bestrees[i - 1]) free(bestrees[i - 1]);
    }
  /*free(bestrees);  
  free(nayme);
  free(enterorder);
  free(names);
  free(place);
  free(weight);
  free(oldweight);
  free(alias);
  free(ally);
  free(location);*/

}

int RAxML_parsimony(int *perm, int treeStringSize, char **parsimonyTree)
{  /* DNA parsimony by uphill search */
  

  phylipTreeString = (char *)malloc(sizeof(char) * treeStringSize); 
  *parsimonyTree = phylipTreeString;
 
  if (ith == 1)
    firstset = false;
  
  for (jumb = 1; jumb <= njumble; jumb++)
    maketree(perm);
   
  return nextree;
}  /* DNA parsimony by uphill search */

int RAxML_parsimony_Subtree(int treeStringSize, char **parsimonyTree)
{  /* DNA parsimony by uphill search */
  int *perm, i;

  phylipTreeString = (char *)malloc(sizeof(char) * treeStringSize); 

  *parsimonyTree = phylipTreeString;
  perm = (int *)malloc(sizeof(int) * (spp + 1));

  for(i = 0; i <= spp; i++)
    perm[i] = i;
  if (ith == 1)
    firstset = false;
  
  for (jumb = 1; jumb <= njumble; jumb++)
    maketree(perm);
   
  free(perm);

  return nextree;
}  /* DNA parsimony by uphill search */








int eof(f)
FILE *f;
{
    register int ch;

    if (feof(f))
        return 1;
    if (f == stdin)
        return 0;
    ch = getc(f);
    if (ch == EOF)
        return 1;
    ungetc(ch, f);
    return 0;
}


int Reoln(f)
FILE *f;
{
    register int ch;

    ch = getc(f);
    if (ch == EOF)
        return 1;
    ungetc(ch, f);
    return (ch == '\n');
}

void Rmemerror()
{
printf("Error allocating memory\n");
exit(-1);
}

MALLOCRETURN *Rmymalloc(x)
long x;
{
MALLOCRETURN *mem;
mem = (MALLOCRETURN *)malloc(x);
if (!mem)
  Rmemerror();
else
  return (MALLOCRETURN *)mem;
}

