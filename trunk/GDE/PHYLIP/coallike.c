#include "phylip.h"

/* version 3.52c.  (c) Copyright 1993 by Joseph Felsenstein.  Permission is
   granted to copy and use this program provided that no fee is charged for
   it and provided that this copyright notice is not removed */

#define epsilon         0.000001  /* a small number                          */

#define ansi0           true
#define ibmpc0          false
#define vt520           false

typedef struct node {             /* describes a tip species or an ancestor */
  struct node *ancestor, *leftdesc, *rtdesc;   /* pointers to nodes         */
  long index;                           /* number of the node               */
  boolean tip;                          /* present species are tips of tree */
  double v, tyme;
} node;

typedef node **pointarray;
typedef double *timearray;


FILE       *treefile, *outfile;
long        i, j, n, rep, spp, nonodes,maxsp;
double      b, b1, bnum, bdenom, mintheta, maxtheta, lnlike, summ, summax;
boolean     ansi, ibmpc, vt52, progress;
node       *root;
pointarray  treenode;   /* pointers to all nodes in tree */
timearray   thyme;
double      *sum;
double      *a;
char        ch;

openfile(fp,filename,mode,application,perm)
FILE **fp;
char *filename;
char *mode;
char *application;
char *perm;
{
  FILE *of;
  char file[100];
  strcpy(file,filename);
  while (1){
    of = fopen(file,mode);
    if (of)
      break;
    else {
      switch (*mode){
      case 'r':
        printf("%s:  can't read %s\n",application,file);
        file[0]='\0';
        while (file[0] =='\0'){
          printf("Please enter a new filename>");
          gets(file);}
        break;
      case 'w':
        printf("%s: can't write %s\n",application,file);
        file[0] = '\0';
        while (file[0] =='\0'){
          printf("Please enter a new filename>");
          gets(file);}
        break;
      }
    }
  }
  *fp=of;
  if (perm != NULL)
    strcpy(perm,file);
}
void preread(file,filename,species,trees)
     FILE **file;
     char *filename;
     long *species;
     long *trees;
{
 long colons,semicolons;
  char c;
  FILE *fp;
  colons     = 0;
  semicolons = 0;
  fp         = (*file);
  while (!feof(fp)){
    c = fgetc(fp);
    if (c == ':')
      colons++;
    else if (c == ';')
      semicolons++;
  }
(*species) = (colons/semicolons/2)+1;
(*trees)   = semicolons;
fclose(*file);
(*file)    = fopen(filename,"r");
}

void uppercase(ch)
Char *ch;
{
  /* convert ch to upper case -- either ASCII or EBCDIC */
  (*ch) = isupper(*ch) ? (*ch) : toupper(*ch);
}  /* uppercase */


void getoptions()
{
  /* interactively set options */
  Char ch;
  char in[100];
  boolean done, badoption;

  done = false;
  ch = ' ';
  mintheta = 0.0001;
  maxtheta = 100.0;
  ansi = ansi0;
  ibmpc = ibmpc0;
  vt52 = vt520;
  progress = false;
  badoption = false;
  do {
    printf(ansi ? "\033[2J\033[H" :
	   vt52 ? "\033E\033H"    : "\n");
    if (badoption)
      printf("\nNot a possible option!\n");
    if (!done && ch == 'Y') {
      putchar('\n');
      if (mintheta > maxtheta)
	printf("Minimum and maximum theta values are out of order\n");
    }
    printf("\nCoalescent likelihoods from sampled trees, version %s\n\n",
	   VERSION);
    printf("Settings for this run:\n");
    printf("  N   Minimum value of theta            %12.7f\n", mintheta);
    printf("  X   Maximum value of theta            %12.5f\n", maxtheta);
    printf("  0   Terminal type (IBM PC, VT52, ANSI)?  %s\n",
	   ibmpc ? "IBM PC" :
	   ansi  ? "ANSI"   :
	   vt52  ? "VT52"   : "(none)");

    printf("  1  Print indications of progress of run  %s\n",
	   (progress ? "Yes" : "No"));
    printf("\nAre these settings correct? (type Y or the letter for one to change)\n");
    in[0]=0;
    gets(in);
    ch = in[0];
    if (ch == '\n')
      ch = ' ';
    uppercase(&ch);
    done = (ch == 'Y' && mintheta < maxtheta);
    if (!done) {
      if (ch == 'N' || ch == 'X' || ch == '1' || ch == '0') {
	badoption = false;
	switch (ch) {

	case 'N':
	  do {
	    printf("Minimum value of theta?\n");
	    scanf("%lg%*[^\n]", &mintheta);
	    getchar();
	    if (mintheta <= 0.0)
	      printf("Must be positive\n");
	  } while (mintheta <= 0.0);
	  break;

	case 'X':
	  do {
	    printf("Maximum value of theta?\n");
	    scanf("%lg%*[^\n]", &maxtheta);
	    getchar();
	    if (maxtheta <= 0.0)
	      printf("Must be positive\n");
	  } while (mintheta <= 0.0);
	  break;

	case '0':
	  if (ibmpc) {
	    ibmpc = false;
	    vt52 = true;
	  } else {
	    if (vt52) {
	      vt52 = false;
	      ansi = true;
	    } else if (ansi)
	      ansi = false;
	    else
	      ibmpc = true;
	  }
	  break;

	case '1':
	  progress = !progress;
	  break;
	}
      } else
	badoption = true;
    }
  } while (!done);
}  /* getoptions */


void setup(p)
node **p;
{
  /* create an empty tree node */
  if (rep == 1)
    *p = (node *)Malloc(sizeof(node));
  (*p)->leftdesc = NULL;
  (*p)->rtdesc = NULL;
  (*p)->ancestor = NULL;
}  /* setup */



void getch(ch)
Char *ch;
{
  /* get next nonblank character */
  do {
    if (eoln(treefile)) {
      fscanf(treefile, "%*[^\n]");
      getc(treefile);
    }
    *ch = getc(treefile);
    if (*ch == '\n')
      *ch = ' ';
  } while (*ch == ' ');
}  /* getch */

void processlength(p)
node *p;
{
  long digit, ordzero;
  double valyew, divisor;
  boolean pointread;

  ordzero = '0';
  pointread = false;
  valyew = 0.0;
  divisor = 1.0;
  getch(&ch);
  digit = ch - ordzero;
  while ((unsigned long)digit <= 9 || ch == '.') {
    if (ch == '.')
      pointread = true;
    else {
      valyew = valyew * 10.0 + digit;
      if (pointread)
	divisor *= 10.0;
    }
    getch(&ch);
    digit = ch - ordzero;
  }
  p->v = valyew / divisor;
}  /* processlength */

void findch(c)
Char c;
{
  /* scan forward until find character c */
  if (ch == c)
    return;
  do {
    if (eoln(treefile)) {
      fscanf(treefile, "%*[^\n]");
      getc(treefile);
    }
    ch = getc(treefile);
    if (ch == '\n')
      ch = ' ';
  } while (ch != c);
}  /* findch */

void findeither(c1, c2)
Char c1, c2;
{  /* findch */
  /* scan forward until find either character c1 or c2 */
  if (ch == c2 || ch == c2)
    return;
  do {
    if (eoln(treefile)) {
      fscanf(treefile, "%*[^\n]");
      getc(treefile);
    }
    ch = getc(treefile);
    if (ch == '\n')
      ch = ' ';
  } while (ch != c1 && ch != c2);
}  /* findch */

void addelement(p, nextnode,nexttip)
node **p;
long *nextnode,*nexttip;
{
  /* recursive procedure adds nodes to user-defined tree */
  node *q;
  long n;

  getch(&ch);
  if (ch == '(') {
    (*nextnode)++;
    setup(&treenode[(*nextnode) - 1]);
    q = treenode[(*nextnode) - 1];
    addelement(&q->leftdesc, nextnode,nexttip);
    q->leftdesc->ancestor = q;
    findch(',');
    addelement(&q->rtdesc, nextnode,nexttip);
    q->rtdesc->ancestor = q;
    findeither(':', ';');
    *p = q;
  } else {
    n = 1;
    do {
      if (eoln(treefile)) {
	fscanf(treefile, "%*[^\n]");
	getc(treefile);
      }
      ch = getc(treefile);
      if (ch == '\n')
	ch = ' ';
      n++;
    } while (ch != ':' && ch != ',' && ch != ')' && ch != ';');
    (*nexttip)++;
    setup(&treenode[(*nexttip) - 1]);
    *p = treenode[(*nexttip) - 1];
  }
  if (ch == ':')
    processlength(*p);
}  /* addelement */


void treeread()
{
  /* read in user-defined tree and set it up */
/* Local variables for treeread */
  long i;
  long nextnode, nexttip;

  nexttip = 0;
  if (rep == 1)
    nextnode = maxsp;
  else
    nextnode = spp;
  addelement(&treenode[nextnode], &nextnode,&nexttip);
  if (rep == 1) {
    spp = nexttip;
    nonodes = spp + nextnode - maxsp;
    for (i = maxsp; i < nextnode; i++) {
      treenode[spp + i - maxsp] = treenode[i];
      treenode[i] = NULL;
    }
    for (i = 1; i <= nonodes; i++)
      treenode[i - 1]->tip = (i <= spp);
  }
  root = treenode[spp];
  root->ancestor = NULL;
}  /* treeread */


void gettymes(p)
node *p;
{
  if (p->tip) {
    p->tyme = 0.0;
    return;
  }
  gettymes(p->leftdesc);
  gettymes(p->rtdesc);
  p->tyme = p->leftdesc->tyme - p->leftdesc->v;
}  /* gettymes */


void shellsort(a, n)
double *a;
long n;
{
  /* Shell sort */
  long gap, i, j;
  double rtemp;

  gap = n / 2;
  while (gap > 0) {
    for (i = gap + 1; i <= n; i++) {
      j = i - gap;
      while (j > 0) {
	if (a[j - 1] < a[j + gap - 1]) {
	  rtemp = a[j - 1];
	  a[j - 1] = a[j + gap - 1];
	  a[j + gap - 1] = rtemp;
	}
	j -= gap;
      }
    }
    gap /= 2;
  }
}  /* shellsort */


void estimate()
{
  long i;
  double n, tt;

  sum[rep - 1] = 0.0;
  n = spp;
  for (i = 1; i < spp; i++) {
    if (i == 1)
      tt = -thyme[0];
    else
      tt = thyme[i - 2] - thyme[i - 1];
    sum[rep - 1] += (n - i) * (n - i + 1) * tt;
  }
}  /* estimate */



main(argc, argv)
int argc;
Char *argv[];
{  /* Coalescent log-likelihood curve */
  char outfilename[100],trfilename[100];
  long species,trees;
#ifdef MAC
  macsetup("Coallike","");
#endif
  openfile(&treefile,TREEFILE,"r",argv[0],trfilename);
  openfile(&outfile,OUTFILE,"w",argv[0],outfilename);
  preread(&treefile,trfilename,&species,&trees);
  thyme     =(double *)Malloc((2*species - 1) * sizeof(double));
  treenode =(node **)Malloc((2*species - 1) * sizeof(node *));
  sum      =(double *)Malloc(trees*sizeof(double));
    maxsp = species+1;
  spp      = species;
  getoptions();
  a = (double *)Malloc((1+(long)(log10(maxtheta/mintheta))*3)*sizeof(double));
  b = mintheta;
  b1 = exp(log(10.0) * (long)floor(log(b) / log(10.0) + 0.5));
  bdenom = (long)floor(1 / b1 + 0.5);
  bnum = (long)floor(b / b1 + 0.5);
  i = 0;
  do {
    i++;
    a[i - 1] = bnum / bdenom;
    if (fabs(bnum - 2.0) < epsilon)
      bnum = (long)floor(2.5 * bnum + 0.5);
    else
      bnum *= 2.0;
    if (fabs(bnum - 10.0) < epsilon) {
      bnum = 1.0;
      bdenom /= 10.0;
    }
  } while (bnum / bdenom <= maxtheta * (1.0 + epsilon));
  n = i;
  if (progress)
    putchar('\n');
  for (rep=1;rep<=trees;++i){
    treeread();
    if (progress && rep % 100 == 0)
      printf("Read tree number %6ld\n", rep);

    if (eoln(treefile) & (!eof(treefile))) {
      fscanf(treefile, "%*[^\n]");
      getc(treefile);
    }
    gettymes(root);
    for (i = spp + 1; i <= nonodes; i++)
      thyme[i - spp - 1] = treenode[i - 1]->tyme;
    shellsort(thyme, spp - 1);
    estimate();
    rep++;
  }
  fprintf(outfile, "\n    Read %5ld trees of %4ld  tips each\n\n",
	  rep - 1, spp);
  fprintf(outfile, "      theta        Ln(Likelihood)\n");
  fprintf(outfile, "      -----        --------------\n");
  for (i = 1; i <= n; i++) {
    summax = sum[0];
    for (j = 2; j < rep; j++) {
      if (sum[j - 1] < summax)
	summax = sum[j - 1];
    }
    summ = 0.0;
    for (j = 1; j < rep; j++) {
      if ((summax - sum[j - 1]) / a[i - 1] > -20.0)
	summ += exp((summax - sum[j - 1]) / a[i - 1]);
    }
    lnlike = (1 - spp) * log(a[i - 1]) - summax / a[i - 1] - log(rep - 1.0) +
	     log(summ);
    fprintf(outfile, "%12.5f%20.5f\n", a[i - 1], lnlike);
  }
  putc('\n', outfile);
  printf("\nResults written on output file\n\n");
  FClose(treefile);
  FClose(outfile);
#ifdef MAC
  fixmacfile(outfilename);
#endif
  exit(0);
}  /* coalescent likelihoods */


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


int eoln(f)
FILE *f;
{
    register int ch;

    ch = getc(f);
    if (ch == EOF)
        return 1;
    ungetc(ch, f);
    return (ch == '\n');
  }

void memerror()
{
printf("Error allocating memory\n");
exit(-1);
}

MALLOCRETURN *mymalloc(x)
long x;
{
MALLOCRETURN *mem;
mem = (MALLOCRETURN *)malloc(x);
if (!mem)
  memerror();
else
  return (MALLOCRETURN *)mem;
}


