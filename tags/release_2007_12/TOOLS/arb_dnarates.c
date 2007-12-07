
#define programName     "DNAml_rates"
#define programVersion  "1.0.0"
#define programDate     "April 11, 1992"

/*  Maximum likelihood site rate calculation, Gary Olsen, 1991, 1992.
 *
 *  Portions based on the program dnaml version 3.3 by Joseph Felsenstein
 *
 *  Copyright notice from dnaml:
 *
 *  version 3.3. (c) Copyright 1986, 1990 by the University of Washington
 *  and Joseph Felsenstein.  Written by Joseph Felsenstein.  Permission is
 *  granted to copy and use this program provided no fee is charged for it
 *  and provided that this copyright notice is not removed.
 */

/* Conversion to C by Gary Olsen, 1991 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* #include <malloc.h> */
#include <string.h>
#include <math.h>
#include "DNAml_rates_1_0.h"

#include <aw_awars.hxx>
#include <arbdb.h>
#include <arbdbt.h>

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define assert(bed) arb_assert(bed)


/*  Global variables */

xarray
*usedxtip, *freextip;

double
dLidki[maxpatterns],      /*  change in pattern i likelihood with rate */
    bestki[maxpatterns],      /*  best rate for pattern i */
    bestLi[maxpatterns],      /*  best likelihood found for pattern i */
    patrate[maxpatterns];     /*  current rate for pattern i */

double
xi, xv, ttratio,          /*  transition/transversion info */
    freqa, freqc, freqg, freqt,  /*  base frequencies */
    freqr, freqy, invfreqr, invfreqy,
    freqar, freqcy, freqgr, freqty,
    fracchange;    /*  random matching fraquency (in a sense) */

int
info[maxsites+1],         /*  number of informative nucleotides */
    patsite[maxsites+1],      /*  site corresponding to pattern */
    pattern[maxsites+1],      /*  pattern number corresponding to site */
    patweight[maxsites+1],    /*  weight of given pattern */
    weight[maxsites+1];       /*  weight of sequence site */

int
categs,        /*  number of rate categories */
    endsite,       /*  number of unique sequence patterns */
    mininfo,       /*  minimum number of informative sequences for rate est */
    numsp,         /*  number of species */
    sites,         /*  number of input sequence positions */
    weightsum;     /*  sum of weights of positions in analysis */

boolean
anerror,       /*  error flag */
    freqsfrom,     /*  use empirical base frequencies */
    interleaved,   /*  input data are in interleaved format */
    printdata,     /*  echo data to output stream */
    writefile,     /*  put weight and rate data in file */
    userweights;   /*  use user-supplied position weight mask */

char
*y[maxsp+1];                    /*  sequence data array */


#if UseStdin       /*  Use standard input */
#  define  INFILE  stdin
#else              /*  Use file named "infile" */
#  define  INFILE  fp
FILE  *fp;
#endif

#if DebugData
FILE *debug;
#endif


/*=======================================================================*/
/*                              PROGRAM                                  */
/*=======================================================================*/

#if 0
void hang(msg) char *msg; {printf("Hanging around: %s\n", msg); while(1);}
#endif

void getnums ()         /* input number of species, number of sites */
{ /* getnums */
    printf("\n%s, version %s, %s\n\n",
           programName,
           programVersion,
           programDate);
    printf("Portions based on Joseph Felsenstein's Nucleic acid sequence\n");
    printf("Maximum Likelihood method, version 3.3\n\n");

    if (fscanf(INFILE, "%d %d", &numsp, &sites) != 2) {
        printf("ERROR: Problem reading number of species and sites\n");
        anerror = TRUE;
        return;
    }
    printf("%d Species, %d Sites\n\n", numsp, sites);

    if (numsp > maxsp) {
        printf("ERROR: Too many species; adjust program constants\n");
        anerror = TRUE;
    }
    else if (numsp < 4) {
        printf("ERROR: Too few species\n");
        anerror = TRUE;
    }

    if (sites > maxsites) {
        printf("ERROR: Too many sites; adjust program constants\n");
        anerror = TRUE;
    }
    else if (sites < 1) {
        printf("ERROR: Too few sites\n");
        anerror = TRUE;
    }
} /* getnums */


boolean digit (ch) int ch; {return (ch >= '0' && ch <= '9'); }


boolean white (ch) int ch; { return (ch == ' ' || ch == '\n' || ch == '\t'); }


void uppercase (chptr)
     int *chptr;
     /* convert character to upper case -- either ASCII or EBCDIC */
{ /* uppercase */
    int  ch;
    ch = *chptr;
    if ((ch >= 'a' && ch <= 'i') || (ch >= 'j' && ch <= 'r')
        || (ch >= 's' && ch <= 'z'))
        *chptr = ch + 'A' - 'a';
} /* uppercase */


int base36 (ch)
     int ch;
{ /* base36 */
    if      (ch >= '0' && ch <= '9') return (ch - '0');
    else if (ch >= 'A' && ch <= 'I') return (ch - 'A' + 10);
    else if (ch >= 'J' && ch <= 'R') return (ch - 'J' + 19);
    else if (ch >= 'S' && ch <= 'Z') return (ch - 'S' + 28);
    else if (ch >= 'a' && ch <= 'i') return (ch - 'a' + 10);
    else if (ch >= 'j' && ch <= 'r') return (ch - 'j' + 19);
    else if (ch >= 's' && ch <= 'z') return (ch - 's' + 28);
    else return -1;
} /* base36 */


int itobase36 (i)
     int  i;
{ /* itobase36 */
    if      (i <  0) return '?';
    else if (i < 10) return (i      + '0');
    else if (i < 19) return (i - 10 + 'A');
    else if (i < 28) return (i - 19 + 'J');
    else if (i < 36) return (i - 28 + 'S');
    else return '?';
} /* itobase36 */


int findch (c)
     int  c;
{
    int ch;

    while ((ch = getc(INFILE)) != EOF && ch != c) ;
    return  ch;
}


void inputweights ()
     /* input the character weights 0, 1, 2 ... 9, A, B, ... Y, Z */
{ /* inputweights */
    int i, ch, wi;

    for (i = 2; i <= nmlngth; i++)  (void) getc(INFILE);
    weightsum = 0;
    i = 1;
    while (i <= sites) {
        ch = getc(INFILE);
        wi = base36(ch);
        if (wi >= 0)
            weightsum += weight[i++] = wi;
        else if (! white(ch)) {
            printf("ERROR: Bad weight character: '%c'", ch);
            printf("       Weights must be a digit or a letter.\n");
            anerror = TRUE;
            return;
        }
    }

    if (findch('\n') == EOF) {      /* skip to end of line */
        printf("ERROR: Missing newline at end of weight data\n");
        anerror = TRUE;
    }
} /* inputweights */


void getoptions ()
{ /* getoptions */
    int  ch, i, extranum;

    categs      =     0;  /*  Number of rate categories */
    freqsfrom   = FALSE;  /*  Use empirical base frequencies */
    interleaved =  TRUE;  /*  By default, data format is interleaved */
    mininfo     = MIN_INFO; /*  Default minimum number of informative seqs */
    printdata   = FALSE;  /*  Don't echo data to output stream */
    ttratio     =   2.0;  /*  Transition/transversion rate ratio */
    userweights = FALSE;  /*  User-defined position weights */
    writefile   = FALSE;  /*  Do not write to file */
    extranum    =     0;

    while ((ch = getc(INFILE)) != '\n' && ch != EOF) {
        uppercase (& ch);
        switch (ch) {
            case '1' : printdata    = ! printdata; break;
            case 'C' : categs       = -1; extranum++; break;
            case 'F' : freqsfrom    = TRUE; break;
            case 'I' : interleaved  = ! interleaved; break;
            case 'L' : break;
            case 'M' : mininfo      = 0; extranum++; break;
            case 'T' : ttratio      = -1.0; extranum++; break;
            case 'U' : break;
            case 'W' : userweights  = TRUE; weightsum = 0; extranum++; break;
            case 'Y' : writefile    = ! writefile; break;
            case ' ' : break;
            case '\t': break;
            default  :
                printf("ERROR: Bad option character: '%c'\n", ch);
                anerror = TRUE;
                return;
        }
    }

    if (ch == EOF) {
        printf("ERROR: End-of-file in options list\n");
        anerror = TRUE;
        return;
    }

    /*  process lines with auxiliary data */

    while (extranum-- && ! anerror) {
        ch = getc(INFILE);
        uppercase (& ch);
        switch (ch) {

            case 'C':
                if (categs >= 0) {
                    printf("ERROR: Unexpected Categories data\n");
                    anerror = TRUE;
                }
                else if (fscanf(INFILE,"%d",&categs) != 1 || findch('\n')==EOF) {
                    printf("ERROR: Problem reading number of rate categories\n");
                    anerror = TRUE;
                }
                else if (categs < 1 || categs > maxcategories) {
                    printf("ERROR: Bad number of rate categories: %d\n", categs);
                    anerror = TRUE;
                }
                break;

            case 'M':   /*  Minimum infomative sequences  */
                if (mininfo > 0) {
                    printf("ERROR: Unexpected Min informative residues data\n");
                    anerror = TRUE;
                }
                else if (fscanf(INFILE,"%d",&mininfo)!=1 || findch('\n')==EOF) {
                    printf("ERROR: Problem reading min informative residues\n");
                    anerror = TRUE;
                }
                else if (mininfo < 2 || mininfo > numsp) {
                    printf("ERROR: Bad number for informative residues: %d\n",
                           mininfo);
                    anerror = TRUE;
                }
                break;

            case 'T':   /*  Transition/transversion ratio  */
                if (ttratio >= 0.0) {
                    printf("ERROR: Unexpected Transition/transversion data\n");
                    anerror = TRUE;
                }
                else if (fscanf(INFILE,"%lf",&ttratio)!=1 || findch('\n')==EOF) {
                    printf("ERROR: Problem reading transition/transversion data\n");
                    anerror = TRUE;
                }
                break;

            case 'W':    /*  Weights  */
                if (! userweights || weightsum > 0) {
                    printf("ERROR: Unexpected Weights data\n");
                    anerror = TRUE;
                }
                else {
                    inputweights();
                }
                break;

            default:
                printf("ERROR: Auxiliary data line starts with '%c'\n", ch);
                anerror = TRUE;
                break;
        }
    }

    if (anerror) return;

    if (categs < 0) {
        printf("ERROR: Category data missing from input\n");
        anerror = TRUE;
        return;
    }

    if (mininfo <= 0) {
        printf("ERROR: Minimum informative residues missing from input\n");
        anerror = TRUE;
        return;
    }
    else {
        printf("There must be at least %d informative residues per column\n\n", mininfo);
    }

    if (ttratio < 0.0) {
        printf("ERROR: Transition/transversion data missing from input\n");
        anerror = TRUE;
        return;
    }

    if (! userweights) {
        for (i = 1; i <= sites; i++)  weight[i] = 1;
        weightsum = sites;
    }
    else if (weightsum < 1) {
        printf("ERROR: Weight data invalid or missing from input\n");
        anerror = TRUE;
        return;
    }

} /* getoptions */


void getbasefreqs ()
{ /* getbasefreqs */
    double  suma, sumb;

    if (freqsfrom)  printf("Empirical ");
    printf("Base Frequencies:\n\n");

    if (! freqsfrom) {
        if (fscanf(INFILE, "%lf%lf%lf%lf", &freqa, &freqc, &freqg, &freqt) != 4
            || findch('\n') == EOF) {
            printf("ERROR: Problem reading user base frequencies\n");
            anerror = TRUE;
        }
    }

    printf("   A    %10.5f\n", freqa);
    printf("   C    %10.5f\n", freqc);
    printf("   G    %10.5f\n", freqg);
    printf("  T(U)  %10.5f\n\n", freqt);
    freqr = freqa + freqg;
    invfreqr = 1.0/freqr;
    freqar = freqa * invfreqr;
    freqgr = freqg * invfreqr;
    freqy = freqc + freqt;
    invfreqy = 1.0/freqy;
    freqcy = freqc * invfreqy;
    freqty = freqt * invfreqy;
    printf("Transition/transversion ratio = %10.6f\n\n", ttratio);
    suma = ttratio*freqr*freqy - (freqa*freqg + freqc*freqt);
    sumb = freqa*freqgr + freqc*freqty;
    xi = suma/(suma+sumb);
    xv = 1.0 - xi;
    ttratio = xi / xv;
    if (xi <= 0.0) {
        printf("WARNING: This transition/transversion ratio is\n");
        printf("         impossible with these base frequencies!\n");
        xi = 3/5;
        xv = 2/5;
        printf("Transition/transversion parameter reset\n\n");
    }
    printf("(Transition/transversion parameter = %10.6f)\n\n", xi/xv);
    fracchange = xi*(2*freqa*freqgr + 2*freqc*freqty)
        + xv*(1.0 - freqa*freqa - freqc*freqc
              - freqg*freqg - freqt*freqt);
} /* getbasefreqs */


void getyspace ()
{ /* getyspace */
    long size;
    int  i;
    char *y0;

    size = 4 * (sites/4 + 1);
    if (! (y0 = malloc((unsigned) (sizeof(char) * size * (numsp+1))))) {
        printf("ERROR: Unable to obtain space for data array\n");
        anerror = TRUE;
    }
    else {
        for (i = 0; i <= numsp; i++) {
            y[i] = y0;
            y0 += size;
        }
    }
} /* getyspace */


void setuptree (tr, numsp)
     tree  *tr;
     int    numsp;
{ /* setuptree */
    int     i, j;
    nodeptr p = 0, q;

    for (i = 1; i <= numsp; i++) {   /*  Set-up tips */
        if ((anerror = !(p = (nodeptr) malloc((unsigned) sizeof(node))))) break;
        p->x      = (xarray *) NULL;
        p->tip    = (char *) NULL;
        p->number = i;
        p->next   = (node *) NULL;
        p->back   = (node *) NULL;
        tr->nodep[i] = p;
    }

    for (i = numsp+1; i <= 2*numsp-1 && ! anerror; i++) { /* Internal nodes */  /* was : 2*numsp-2 (ralf) */
        q = (node *) NULL;
        for (j = 1; j <= 3; j++) {
            if ((anerror = !(p = (nodeptr) malloc((unsigned) sizeof(node))))) break;
            p->x      = (xarray *) NULL;
            p->tip    = (char *) NULL;
            p->number = i;
            p->next   = q;
            p->back   = (node *) NULL;
            q = p;
        }
        if (anerror) break;
        p->next->next->next = p;
        tr->nodep[i] = p;
    }

    tr->likelihood = unlikely;
    tr->start      = tr->nodep[1];
    tr->mxtips     = numsp;
    tr->ntips      = 0;
    tr->nextnode   = 0;
    tr->opt_level  = 0;
    tr->smoothed   = FALSE;
    if (anerror) printf("ERROR: Unable to obtain sufficient memory");
} /* setuptree */


void freeTreeNode(p)   /*  Free a tree node (sector) */
     nodeptr  p;
{ /* freeTreeNode */
    if (p) {
        if (p->x) {
            if (p->x->a) free((char *) p->x->a);
            free ((char *) p->x);
        }
        free ((char *) p);
    }
} /* freeTreeNode */


void freeTree (tr)
     tree  *tr;
{ /* freeTree */
    int  i;
    nodeptr  p, q;

    for (i = 1; i <= tr->mxtips; i++) freeTreeNode(tr->nodep[i]);

    for (i = tr->mxtips+1; i <= 2*(tr->mxtips)-2; i++) {
        if ((p = tr->nodep[i])) {
            if ((q = p->next)) {
                freeTreeNode(q->next);
                freeTreeNode(q);
            }
            freeTreeNode(p);
        }
    }
} /* freeTree */


void getdata (tr)
     tree  *tr;
     /* read sequences */
{ /* getdata */
    int  i, j, k, l, basesread, basesnew, ch;
    int  meaning[256];          /*  meaning of input characters */
    char *nameptr;
    boolean  allread, firstpass;

    for (i = 0; i <= 255; i++) meaning[i] = 0;
    meaning['A'] =  1;
    meaning['B'] = 14;
    meaning['C'] =  2;
    meaning['D'] = 13;
    meaning['G'] =  4;
    meaning['H'] = 11;
    meaning['K'] = 12;
    meaning['M'] =  3;
    meaning['N'] = 15;
    meaning['O'] = 15;
    meaning['R'] =  5;
    meaning['S'] =  6;
    meaning['T'] =  8;
    meaning['U'] =  8;
    meaning['V'] =  7;
    meaning['W'] =  9;
    meaning['X'] = 15;
    meaning['Y'] = 10;
    meaning['?'] = 15;
    meaning['-'] = 15;

    basesread = basesnew = 0;

    allread = FALSE;
    firstpass = TRUE;
    ch = ' ';

    while (! allread) {
        for (i = 1; i <= numsp; i++) {          /*  Read data line */
            if (firstpass) {                      /*  Read species names */
                j = 1;
                while (white(ch = getc(INFILE))) {  /*  Skip blank lines */
                    if (ch == '\n')  j = 1;  else  j++;
                }

                if (j > nmlngth) {
                    printf("ERROR: Blank name for species %d; ", i);
                    printf("check number of species,\n");
                    printf("       number of sites, and interleave option.\n");
                    anerror = TRUE;
                    return;
                }

                nameptr = tr->nodep[i]->name;
                for (k = 1; k < j; k++)  *nameptr++ = ' ';

                while (ch != '\n' && ch != EOF) {
                    if (ch == '_' || white(ch))  ch = ' ';
                    *nameptr++ = ch;
                    if (++j > nmlngth) break;
                    ch = getc(INFILE);
                }

                while (j++ <= nmlngth) *nameptr++ = ' ';
                *nameptr = '\0';                      /*  add null termination */

                if (ch == EOF) {
                    printf("ERROR: End-of-file in name of species %d\n", i);
                    anerror = TRUE;
                    return;
                }
            }    /* if (firstpass) */

            j = basesread;
            while ((j < sites) && ((ch = getc(INFILE)) != EOF)
                   && ((! interleaved) || (ch != '\n'))) {
                uppercase (& ch);
                if (meaning[ch] || ch == '.') {
                    j++;
                    if (ch == '.') {
                        if (i != 1) ch = y[1][j];
                        else {
                            printf("ERROR: Dot (.) found at site %d of sequence 1\n", j);
                            anerror = TRUE;
                            return;
                        }
                    }
                    y[i][j] = ch;
                }
                else if (white(ch) || digit(ch)) ;
                else {
                    printf("ERROR: Bad base (%c) at site %d of sequence %d\n",
                           ch, j, i);
                    anerror = TRUE;
                    return;
                }
            }

            if (ch == EOF) {
                printf("ERROR: End-of-file at site %d of sequence %d\n", j, i);
                anerror = TRUE;
                return;
            }

            if (! firstpass && (j == basesread)) i--;        /* no data on line */
            else if (i == 1) basesnew = j;
            else if (j != basesnew) {
                printf("ERROR: Sequences out of alignment\n");
                printf("       %d (instead of %d) residues read in sequence %d\n",
                       j - basesread, basesnew - basesread, i);
                anerror = TRUE;
                return;
            }

            while (ch != '\n' && ch != EOF) ch = getc(INFILE); /* flush line */
        }                                                  /* next sequence */
        firstpass = FALSE;
        basesread = basesnew;
        allread = (basesread >= sites);
    }

    /*  Print listing of sequence alignment */

    if (printdata) {
        j = nmlngth - 5 + ((sites + ((sites-1)/10))/2);
        if (j < nmlngth - 1) j = nmlngth - 1;
        if (j > 37) j = 37;
        printf("Name"); for (i=1;i<=j;i++) putchar(' '); printf("Sequences\n");
        printf("----"); for (i=1;i<=j;i++) putchar(' '); printf("---------\n");
        putchar('\n');

        for (i = 1; i <= sites; i += 60) {
            l = i + 59;
            if (l > sites) l = sites;

            if (userweights) {
                printf("Weights   ");
                for (j = 11; j <= nmlngth+3; j++) putchar(' ');
                for (k = i; k <= l; k++) {
                    putchar(itobase36(weight[k]));
                    if (((k % 10) == 0) && ((k % 60) != 0)) putchar(' ');
                }
                putchar('\n');
            }

            for (j = 1; j <= numsp; j++) {
                printf("%s   ", tr->nodep[j]->name);
                for (k = i; k <= l; k++) {
                    ch = y[j][k];
                    if ((j > 1) && (ch == y[1][k])) ch = '.';
                    putchar(ch);
                    if (((k % 10) == 0) && ((k % 60) != 0)) putchar(' ');
                }
                putchar('\n');
            }
            putchar('\n');
        }
    }

    /*  Convert characters to meanings */

    for (i = 1; i <= sites; i++)  info[i] = 0;

    for (j = 1; j <= numsp; j++) {
        for (i = 1; i <= sites; i++) {
            if ((y[j][i] = meaning[(int)y[j][i]]) != 15) info[i]++;
        }
    }

    for (i = 1; i <= sites; i++)  if (info[i] < MIN_INFO)  weight[i] = 0;

} /* getdata */


void sitesort ()
     /* Shell sort keeping sites, weights in same order */
{ /* sitesort */
    int  gap, i, j, jj, jg, k;
    boolean  flip, tied;

    for (gap = sites/2; gap > 0; gap /= 2) {
        for (i = gap + 1; i <= sites; i++) {
            j = i - gap;

            do {
                jj = patsite[j];
                jg = patsite[j+gap];
                flip = FALSE;
                tied = TRUE;
                for (k = 1; tied && (k <= numsp); k++) {
                    flip = (y[k][jj] >  y[k][jg]);
                    tied = (y[k][jj] == y[k][jg]);
                }
                if (flip) {
                    patsite[j]     = jg;
                    patsite[j+gap] = jj;
                    j -= gap;
                }
            } while (flip && (j > 0));

        }
    }
} /* sitesort */


void sitecombcrunch ()
     /* combine sites that have identical patterns (and nonzero weight) */
{ /* sitecombcrunch */
    int  i, sitei, j, sitej, k;
    boolean  tied;

    i = 0;
    patsite[0] = patsite[1];
    patweight[0] = 0;

    for (j = 1; j <= sites; j++) {
        tied = TRUE;
        sitei = patsite[i];
        sitej = patsite[j];

        for (k = 1; tied && (k <= numsp); k++)
            tied = (y[k][sitei] == y[k][sitej]);

        if (tied) {
            patweight[i] += weight[sitej];
        }
        else {
            if (patweight[i] > 0) i++;
            patweight[i] = weight[sitej];
            patsite[i] = sitej;
        }

        pattern[sitej] = i;
    }

    endsite = i;
    if (patweight[i] > 0) endsite++;
} /* sitecombcrunch */


void makeweights ()
     /* make up weights vector to avoid duplicate computations */
{ /* makeweights */
    int  i;

    for (i = 1; i <= sites; i++)  patsite[i] = i;
    sitesort();
    sitecombcrunch();
    if (endsite > maxpatterns) {
        printf("ERROR:  Too many patterns in data\n");
        printf("        Increase maxpatterns to at least %d\n", endsite);
        anerror = TRUE;
    }
    else {
        printf("Analyzing %d distinct data patterns (columns)\n\n", endsite);
    }
} /* makeweights */


void makevalues (tr)
     tree  *tr;
     /* set up fractional likelihoods at tips */
{ /* makevalues */
    nodeptr  p;
    int  i, j;

    for (i = 1; i <= numsp; i++) {    /* Pack and move tip data */
        for (j = 0; j < endsite; j++)
            y[i-1][j] = y[i][patsite[j]];

        p = tr->nodep[i];
        p->tip = &(y[i-1][0]);
    }

} /* makevalues */


void empiricalfreqs (tr)
     tree  *tr;
     /* Get empirical base frequencies from the data */
{ /* empiricalfreqs */
    double  sum, suma, sumc, sumg, sumt, wj, fa, fc, fg, ft;
    int  i, j, k, code;
    char *yptr;

    freqa = 0.25;
    freqc = 0.25;
    freqg = 0.25;
    freqt = 0.25;
    for (k = 1; k <= 8; k++) {
        suma = 0.0;
        sumc = 0.0;
        sumg = 0.0;
        sumt = 0.0;
        for (i = 1; i <= numsp; i++) {
            yptr = tr->nodep[i]->tip;
            for (j = 0; j < endsite; j++) {
                code = *yptr++;
                fa = freqa * ( code       & 1);
                fc = freqc * ((code >> 1) & 1);
                fg = freqg * ((code >> 2) & 1);
                ft = freqt * ((code >> 3) & 1);
                wj = patweight[j] / (fa + fc + fg + ft);
                suma += wj * fa;
                sumc += wj * fc;
                sumg += wj * fg;
                sumt += wj * ft;
            }
        }
        sum = suma + sumc + sumg + sumt;
        freqa = suma / sum;
        freqc = sumc / sum;
        freqg = sumg / sum;
        freqt = sumt / sum;
    }
} /* empiricalfreqs */


void getinput (tr)
     tree  *tr;
{ /* getinput */
    getnums();                        if (anerror) return;
    getoptions();                     if (anerror) return;
    if (! freqsfrom) getbasefreqs();  if (anerror) return;
    getyspace();                      if (anerror) return;
    setuptree(tr, numsp);             if (anerror) return;
    getdata (tr);                     if (anerror) return;
    makeweights();                    if (anerror) return;
    makevalues (tr);                  if (anerror) return;
    if (freqsfrom) {
        empiricalfreqs (tr);            if (anerror) return;
        getbasefreqs();
    }
} /* getinput */


xarray *setupxarray ()
{ /* setupxarray */
    xarray  *x;
    xtype  *data;

    x = (xarray *) malloc((unsigned) sizeof(xarray));
    if (x) {
        data = (xtype *) malloc((unsigned) (4 * endsite * sizeof(xtype)));
        if (data) {
            x->a = data;
            x->c = data += endsite;
            x->g = data += endsite;
            x->t = data +  endsite;
            x->prev = x->next = x;
            x->owner = (node *) NULL;
        }
        else {
            free ((char *) x);
            return (xarray *) NULL;
        }
    }
    return x;
} /* setupxarray */


void linkxarray (req, min, freexptr, usedxptr)
     int  req, min;
     xarray **freexptr, **usedxptr;
     /*  Link a set of xarrays */
{ /* linkxarray */
    xarray  *first, *prev, *x;
    int  i;

    first = prev = (xarray *) NULL;
    i = 0;

    do {
        x = setupxarray();
        if (x) {
            if (! first) first = x;
            else {
                prev->next = x;
                x->prev = prev;
            }
            prev = x;
            i++;
        }
        else {
            printf("ERROR: Failure to get xarray memory.\n");
            if (i < min) anerror = TRUE;
        }
    } while ((i < req) && x);

    if (first) {
        first->prev = prev;
        prev->next = first;
    }

    *freexptr = first;
    *usedxptr = (xarray *) NULL;
} /* linkxarray */


void setupnodex (tr)
     tree  *tr;
{ /* setupnodex */
    nodeptr  p;
    int  i;

    for (i = tr->mxtips + 1; (i <= 2*(tr->mxtips) - 2) && ! anerror; i++) {
        p = tr->nodep[i];
        if ((anerror = !(p->x = setupxarray()))) break;
    }
} /* setupnodex */


xarray *getxtip (p)
     nodeptr  p;
{ /* getxtip */
    xarray  *new;
    boolean  splice;

    if (! p) return (xarray *) NULL;

    splice = FALSE;

    if (p->x) {
        new = p->x;
        if (new == new->prev) ;             /* linked to self; leave it */
        else if (new == usedxtip) usedxtip = usedxtip->next; /* at head */
        else if (new == usedxtip->prev) ;   /* already at tail */
        else {                              /* move to tail of list */
            new->prev->next = new->next;
            new->next->prev = new->prev;
            splice = TRUE;
        }
    }

    else if (freextip) {
        p->x = new = freextip;
        new->owner = p;
        if (new->prev != new) {            /* not only member of freelist */
            new->prev->next = new->next;
            new->next->prev = new->prev;
            freextip = new->next;
        }
        else
            freextip = (xarray *) NULL;

        splice = TRUE;
    }

    else if (usedxtip) {
        usedxtip->owner->x = (xarray *) NULL;
        p->x = new = usedxtip;
        new->owner = p;
        usedxtip = usedxtip->next;
    }

    else {
        printf ("ERROR: Unable to locate memory for a tip.\n");
        anerror = TRUE;
        exit(0);
    }

    if (splice) {
        if (usedxtip) {                  /* list is not empty */
            usedxtip->prev->next = new;
            new->prev = usedxtip->prev;
            usedxtip->prev = new;
            new->next = usedxtip;
        }
        else
            usedxtip = new->prev = new->next = new;
    }

    return  new;
} /* getxtip */


xarray *getxnode (p)
     nodeptr  p;
     /* Ensure that internal node p has memory */
{ /* getxnode */
    nodeptr  s;

    if (! (p->x)) {  /*  Move likelihood array on this node to sector p */
        if ((s = p->next)->x || (s = s->next)->x) {
            p->x = s->x;
            s->x = (xarray *) NULL;
        }
        else {
            printf("ERROR in getxnode: Unable to locate memory at internal node.");
            exit(0);
        }
    }
    return  p->x;
} /* getxnode */


void newview (p)                      /*  Update likelihoods at node */
     nodeptr  p;
{ /* newview */
    double   z1, lz1, xvlz1, z2, lz2, xvlz2,
        zz1, zv1, fx1r, fx1y, fx1n, suma1, sumg1, sumc1, sumt1,
        zz2, zv2, fx2r, fx2y, fx2n, ki, tempi, tempj;
    nodeptr  q, r;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t,
        *x3a, *x3c, *x3g, *x3t;
    int  i;

    if (p->tip) {             /*  Make sure that data are at tip */
        int code;
        char *yptr;

        if (p->x) return;       /*  They are already there */
        (void) getxtip(p);      /*  They are not, so get memory */
        x3a = &(p->x->a[0]);    /*  Move tip data to xarray */
        x3c = &(p->x->c[0]);
        x3g = &(p->x->g[0]);
        x3t = &(p->x->t[0]);
        yptr = p->tip;
        for (i = 0; i < endsite; i++) {
            code = *yptr++;
            *x3a++ =  code       & 1;
            *x3c++ = (code >> 1) & 1;
            *x3g++ = (code >> 2) & 1;
            *x3t++ = (code >> 3) & 1;
        }
        return;
    }

    /*  Internal node needs update */

    q = p->next->back;
    r = p->next->next->back;

    while ((! p->x) || (! q->x) || (! r->x)) {
        if (! q->x) newview(q);
        if (! r->x) newview(r);
        if (! p->x)  (void) getxnode(p);
    }

    x1a = &(q->x->a[0]);
    x1c = &(q->x->c[0]);
    x1g = &(q->x->g[0]);
    x1t = &(q->x->t[0]);
    z1 = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);
    xvlz1 = xv * lz1;

    x2a = &(r->x->a[0]);
    x2c = &(r->x->c[0]);
    x2g = &(r->x->g[0]);
    x2t = &(r->x->t[0]);
    z2 = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);
    xvlz2 = xv * lz2;

    x3a = &(p->x->a[0]);
    x3c = &(p->x->c[0]);
    x3g = &(p->x->g[0]);
    x3t = &(p->x->t[0]);

    { double  *rptr;

    rptr = &(patrate[0]);
    for (i = 0; i < endsite; i++) {
        ki = *rptr++;

        zz1 = exp(ki *   lz1);
        zv1 = exp(ki * xvlz1);
        fx1r = freqa * *x1a + freqg * *x1g;
        fx1y = freqc * *x1c + freqt * *x1t;
        fx1n = fx1r + fx1y;
        tempi = fx1r * invfreqr;
        tempj = zv1 * (tempi-fx1n) + fx1n;
        suma1 = zz1 * (*x1a++ - tempi) + tempj;
        sumg1 = zz1 * (*x1g++ - tempi) + tempj;
        tempi = fx1y * invfreqy;
        tempj = zv1 * (tempi-fx1n) + fx1n;
        sumc1 = zz1 * (*x1c++ - tempi) + tempj;
        sumt1 = zz1 * (*x1t++ - tempi) + tempj;

        zz2 = exp(ki *   lz2);
        zv2 = exp(ki * xvlz2);
        fx2r = freqa * *x2a + freqg * *x2g;
        fx2y = freqc * *x2c + freqt * *x2t;
        fx2n = fx2r + fx2y;
        tempi = fx2r * invfreqr;
        tempj = zv2 * (tempi-fx2n) + fx2n;
        *x3a++ = suma1 * (zz2 * (*x2a++ - tempi) + tempj);
        *x3g++ = sumg1 * (zz2 * (*x2g++ - tempi) + tempj);
        tempi = fx2y * invfreqy;
        tempj = zv2 * (tempi-fx2n) + fx2n;
        *x3c++ = sumc1 * (zz2 * (*x2c++ - tempi) + tempj);
        *x3t++ = sumt1 * (zz2 * (*x2t++ - tempi) + tempj);
    }
    }
} /* newview */


void hookup (p, q, z)
     nodeptr  p, q;
     double   z;
{ /* hookup */
    p->back = q;
    q->back = p;
    p->z = q->z = z;
} /* hookup */


void initrav (p)
     nodeptr  p;
{ /* initrav */
    if (! p->tip) {
        initrav(p->next->back);
        initrav(p->next->next->back);
        newview(p);
    }
} /* initrav */


/*=======================================================================*/
/*                         Read a tree from a file                       */
/*=======================================================================*/

int treeFinishCom ()
{ /* treeFinishCom */
    int      ch;
    boolean  inquote;

    inquote = FALSE;
    while ((ch = getc(INFILE)) != EOF && (inquote || ch != ']')) {
        if (ch == '[' && ! inquote) {             /* comment; find its end */
            if ((ch = treeFinishCom()) == EOF)  break;
        }
        else if (ch == '\'') inquote = ! inquote;  /* start or end of quote */
    }

    return  ch;
} /* treeFinishCom */


int treeGetCh ()
     /* get next nonblank, noncomment character */
{ /* treeGetCh */
    int  ch;

    while ((ch = getc(INFILE)) != EOF) {
        if (white(ch)) ;
        else if (ch == '[') {                   /* comment; find its end */
            if ((ch = treeFinishCom()) == EOF)  break;
        }
        else  break;
    }

    return  ch;
} /* treeGetCh */


void treeFlushLabel()
{
    int     ch;
    boolean done;

    if ((ch = treeGetCh()) == EOF)  return;
    done = (ch == ':' || ch == ',' || ch == ')'  || ch == '[' || ch == ';');

    if (!done) {
        boolean quoted = (ch == '\'');
        if (quoted) ch = getc(INFILE);
        
        while (! done) {
            if (quoted) {
                if ((ch = findch('\'')) == EOF)  return;      /* find close quote */
                ch = getc(INFILE);                            /* check next char */
                if (ch != '\'') done = TRUE;                  /* not doubled quote */
            }
            else if (ch == ':' || ch == ',' || ch == ')'  || ch == '['
                     || ch == ';' || ch == '\n' || ch == EOF) {
                done = TRUE;
            }
            if (! done)  done = ((ch = getc(INFILE)) == EOF);
        }
    }

    if (ch != EOF)  (void) ungetc(ch, INFILE);
} /* treeFlushLabel */


int  findTipName (tr, ch)
     tree  *tr;
     int    ch;
{ /* findTipName */
    nodeptr  q;
    char  *nameptr, str[nmlngth+1];
    int  i, n;
    boolean  found, quoted, done;

    if ((quoted = (ch == '\'')))  ch = getc(INFILE);
    done = FALSE;
    i = 0;

    do {
        if (quoted) {
            if (ch == '\'') {
                ch = getc(INFILE);
                if (ch != '\'') done = TRUE;
            }
            else if (ch == EOF)
                done = TRUE;
            else if (ch == '\n' || ch == '\t')
                ch = ' ';
        }
        else if (ch == ':' || ch == ','  || ch == ')'  || ch == '['
                 || ch == '\n' || ch == EOF)
            done = TRUE;
        else if (ch == '_' || ch == '\t')
            ch = ' ';

        if (! done) {
            if (i < nmlngth)  str[i++] = ch;
            ch = getc(INFILE);
        }
    } while (! done);

    if (ch == EOF) {
        printf("ERROR: End-of-file in tree species name\n");
        return  0;
    }

    (void) ungetc(ch, INFILE);
    while (i < nmlngth)  str[i++] = ' ';     /*  Pad name */

    n = 1;
    do {
        q = tr->nodep[n];
        if (! (q->back)) {          /*  Only consider unused tips */
            i = 0;
            nameptr = q->name;
            do {found = str[i] == *nameptr++;}  while (found && (++i < nmlngth));
        }
        else
            found = FALSE;
    } while ((! found) && (++n <= tr->mxtips));

    if (! found) {
        i = nmlngth;
        do {str[i] = '\0';} while (i-- && (str[i] <= ' '));
        printf("ERROR: Cannot find data for tree species: %s\n", str);
    }

    return  (found ? n : 0);
} /* findTipName */


double processLength ()
{ /* processLength */
    double  branch;
    int     ch;
    char    string[41];

    ch = treeGetCh();                            /*  Skip comments */
    if (ch != EOF)  (void) ungetc(ch, INFILE);

    if (fscanf(INFILE, "%lf", &branch) != 1) {
        printf("ERROR: Problem reading branch length in processLength:\n");
        if (fscanf(INFILE, "%40s", string) == 1)  printf("%s\n", string);
        anerror = TRUE;
        branch = 0.0;
    }

    return  branch;
} /* processLength */


void  treeFlushLen ()
{ /* treeFlushLen */
    int  ch;

    if ((ch = treeGetCh()) == ':')
        (void) processLength();
    else if (ch != EOF)
        (void) ungetc(ch, INFILE);

} /* treeFlushLen */


void  treeNeedCh (c1, where)
     int    c1;
     char  *where;
{ /* treeNeedCh */
    int c2, i;

    if ((c2 = treeGetCh()) == c1)  return;
     /* assert(c2 == c1); */ /* stop here with debugger  */

    printf("ERROR: Missing '%c' %s tree; ", c1, where);
    if (c2 == EOF)
        printf("End-of-File");
    else {
        putchar('\'');
        for (i = 24; i-- && (c2 != EOF); c2 = getc(INFILE))  putchar(c2);
        putchar('\'');
    }
    printf(" found instead\n");
    anerror = TRUE;
} /* treeNeedCh */


void  addElementLen (tr, p)
     tree    *tr;
     nodeptr  p;
{ /* addElementLen */
    double   z, branch;
    nodeptr  q;
    int      n, ch;

    if ((ch = treeGetCh()) == '(') {     /*  A new internal node */
        n = (tr->nextnode)++;
        if (n > 2*(tr->mxtips) - 2) {
            if (tr->rooted || n > 2*(tr->mxtips) - 1) {
                printf("ERROR: Too many internal nodes.  Is tree rooted?\n");
                printf("       Deepest splitting should be a trifurcation.\n");
                anerror = TRUE;
                return;
            }
            else {
                tr->rooted = TRUE;
            }
        }
        q = tr->nodep[n];
        assert(q);
        addElementLen(tr, q->next);        if (anerror)  return;
        treeNeedCh(',', "in");             if (anerror)  return;
        assert(q->next);
        addElementLen(tr, q->next->next);  if (anerror)  return;
        treeNeedCh(')', "in");             if (anerror)  return;
        treeFlushLabel();                  if (anerror)  return;
    }

    else {                               /*  A new tip */
        n = findTipName(tr, ch);
        if (n <= 0) {anerror = TRUE; return; }
        q = tr->nodep[n];
        if (tr->start->number > n)  tr->start = q;
        (tr->ntips)++;
    }                                  /* End of tip processing */

    treeNeedCh(':', "in");             if (anerror)  return;
    branch = processLength();          if (anerror)  return;
    z = exp(-branch / fracchange);
    if (z > zmax)  z = zmax;
    hookup(p, q, z);
} /* addElementLen */


void uprootTree (tr, p)
     tree   *tr;
     nodeptr p;
{ /* uprootTree */
    nodeptr  q, r, s;
    int  n;

    if (p->tip || p->back) {
        printf("ERROR: Unable to uproot tree.\n");
        printf("       Inappropriate node marked for removal.\n");
        anerror = TRUE;
        return;
    }

    n = --(tr->nextnode);               /* last internal node added */
    if (n != tr->mxtips + tr->ntips - 1) {
        printf("ERROR: Unable to uproot tree.  Inconsistent\n");
        printf("       number of tips and nodes for rooted tree.\n");
        anerror = TRUE;
        return;
    }

    q = p->next->back;                  /* remove p from tree */
    r = p->next->next->back;
    hookup(q, r, q->z * r->z);

    q = tr->nodep[n];
    r = q->next;
    s = q->next->next;
    if (tr->ntips > 2 && p != q && p != r && p != s) {
        hookup(p,             q->back, q->z);   /* move connections to p */
        hookup(p->next,       r->back, r->z);
        hookup(p->next->next, s->back, s->z);
    }

    q->back = r->back = s->back = (nodeptr) NULL;
    tr->rooted = FALSE;
} /* uprootTree */


void treeReadLen (tr)
     tree  *tr;
{ /* treeReadLen */
    nodeptr  p;
    int  i, ch;

    for (i = 1; i <= tr->mxtips; i++) tr->nodep[i]->back = (node *) NULL;
    tr->start       = tr->nodep[tr->mxtips];
    tr->ntips       = 0;
    tr->nextnode    = tr->mxtips + 1;
    tr->opt_level   = 0;
    tr->smoothed    = FALSE;
    tr->rooted      = FALSE;

    p = tr->nodep[(tr->nextnode)++];
    treeNeedCh('(', "at start of");                  if (anerror)  return;
    addElementLen(tr, p);                            if (anerror)  return;
    treeNeedCh(',', "in");                           if (anerror)  return;
    addElementLen(tr, p->next);                      if (anerror)  return;
    if (! tr->rooted) {
        if ((ch = treeGetCh()) == ',') {        /*  An unrooted format */
            addElementLen(tr, p->next->next);            if (anerror)  return;
        }
        else {                                  /*  A rooted format */
            p->next->next->back = (nodeptr) NULL;
            tr->rooted = TRUE;
            if (ch != EOF)  (void) ungetc(ch, INFILE);
        }
    }
    treeNeedCh(')', "in");
    if (anerror)  {
        printf("(This error also happens if the last species in the tree is unmarked)\n");
        return;
    }


    treeFlushLabel();                                if (anerror)  return;
    treeFlushLen();                                  if (anerror)  return;
    treeNeedCh(';', "at end of");                    if (anerror)  return;

    if (tr->rooted)  uprootTree(tr, p->next->next);  if (anerror)  return;
    tr->start = p->next->next->back;  /* This is start used by treeString */

    initrav(tr->start);
    initrav(tr->start->back);
} /* treeReadLen */

/*=======================================================================*/
/*                           End of Tree Reading                         */
/*=======================================================================*/


double evaluate (tr, p)
     tree    *tr;
     nodeptr  p;
{ /* evaluate */
    double   sum, z, lz, xvlz,
        ki, zz, zv, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
        suma, sumb, sumc, term;
    double  *log_f, *rptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    nodeptr  q;
    int  i, *wptr;

    q = p->back;
    while ((! p->x) || (! q->x)) {
        if (! (p->x)) newview(p);
        if (! (q->x)) newview(q);
    }

    x1a = &(p->x->a[0]);
    x1c = &(p->x->c[0]);
    x1g = &(p->x->g[0]);
    x1t = &(p->x->t[0]);

    x2a = &(q->x->a[0]);
    x2c = &(q->x->c[0]);
    x2g = &(q->x->g[0]);
    x2t = &(q->x->t[0]);

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = xv * lz;

    wptr = &(patweight[0]);
    rptr = &(patrate[0]);
    log_f = tr->log_f;
    sum = 0.0;

    for (i = 0; i < endsite; i++) {
        fx1a  = freqa * *x1a++;
        fx1g  = freqg * *x1g++;
        fx1c  = freqc * *x1c++;
        fx1t  = freqt * *x1t++;
        suma  = fx1a * *x2a + fx1c * *x2c + fx1g * *x2g + fx1t * *x2t;
        fx2r  = freqa * *x2a++ + freqg * *x2g++;
        fx2y  = freqc * *x2c++ + freqt * *x2t++;
        fx1r  = fx1a + fx1g;
        fx1y  = fx1c + fx1t;
        sumc  = (fx1r + fx1y) * (fx2r + fx2y);
        sumb  = fx1r * fx2r * invfreqr + fx1y * fx2y * invfreqy;
        suma -= sumb;
        sumb -= sumc;

        ki = *rptr++;
        zz = exp(ki *   lz);
        zv = exp(ki * xvlz);

        term = log(zz * suma + zv * sumb + sumc);
        sum += *wptr++ * term;
        *log_f++ = term;
    }

    tr->likelihood = sum;
    return  sum;
} /* evaluate */


void dli_dki (p)  /*  d(Li)/d(ki) */
     nodeptr  p;
{ /* dli_dki */
    double   z, lz, xvlz;
    double   ki, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
        suma, sumb, sumc;
    double  *rptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    nodeptr  q;
    int  i, *wptr;


    q = p->back;
    while ((! p->x) || (! q->x)) {
        if (! p->x) newview(p);
        if (! q->x) newview(q);
    }

    x1a = &(p->x->a[0]);
    x1c = &(p->x->c[0]);
    x1g = &(p->x->g[0]);
    x1t = &(p->x->t[0]);

    x2a = &(q->x->a[0]);
    x2c = &(q->x->c[0]);
    x2g = &(q->x->g[0]);
    x2t = &(q->x->t[0]);

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = xv * lz;

    rptr = &(patrate[0]);
    wptr = &(patweight[0]);

    for (i = 0; i < endsite; i++) {
        fx1a  = freqa * *x1a++;
        fx1g  = freqg * *x1g++;
        fx1c  = freqc * *x1c++;
        fx1t  = freqt * *x1t++;
        suma  = fx1a * *x2a + fx1c * *x2c + fx1g * *x2g + fx1t * *x2t;
        fx2r  = freqa * *x2a++ + freqg * *x2g++;
        fx2y  = freqc * *x2c++ + freqt * *x2t++;
        fx1r  = fx1a + fx1g;
        fx1y  = fx1c + fx1t;
        sumc  = (fx1r + fx1y) * (fx2r + fx2y);
        sumb  = fx1r * fx2r * invfreqr + fx1y * fx2y * invfreqy;
        suma -= sumb;
        sumb -= sumc;

        ki    = *rptr++;
        suma *= exp(ki *   lz);
        sumb *= exp(ki * xvlz);
        dLidki[i] += *wptr++ * lz * (suma + sumb*xv);
    }
} /* dli_dki */


void spanSubtree (p)
     nodeptr  p;
{ /* spanSubtree */
    dli_dki (p);

    if (! p->tip) {
        spanSubtree(p->next->back);
        spanSubtree(p->next->next->back);
    }
} /* spanSubtree */


void findSiteRates (tr, ki_min, ki_max, d_ki, max_error)
     tree    *tr;
     double   ki_min, ki_max, d_ki, max_error;
{ /* findSiteRates */
    double  inv_d_ki, ki;
    int     i;

    if (ki_min <= 0.0 || ki_max <= ki_min) {
        printf("ERROR: Bad rate value limits to findSiteRates\n");
        anerror = TRUE;
        return;
    }
    else if (d_ki <= 1.0) {
        printf("ERROR: Bad rate step to findSiteRates\n");
        anerror = TRUE;
        return;
    }

    for (i = 0; i < endsite; i++) {
        bestki[i] = 1.0;                       /*  dummy initial rates */
        bestLi[i] = unlikely;
    }

    for (ki = ki_min; ki <= ki_max; ki *= d_ki) {
        for (i = 0; i < endsite; i++)  patrate[i] = ki;
        initrav(tr->start);
        initrav(tr->start->back);
        (void) evaluate(tr, tr->start->back);
        for (i = 0; i < endsite; i++) {
            if (tr->log_f[i] > bestLi[i]) {
                bestki[i] = ki;
                bestLi[i] = tr->log_f[i];
            }
        }
    }

    for (i = 0; i < endsite; i++)  patrate[i] = bestki[i];
    initrav(tr->start);
    initrav(tr->start->back);

    while (d_ki > 1.0 + max_error) {
        for (i = 0; i < endsite; i++)  dLidki[i] = 0.0;
        spanSubtree(tr->start->back);
        if (! tr->start->tip) {
            spanSubtree(tr->start->next->back);
            spanSubtree(tr->start->next->next->back);
        }

        d_ki = sqrt(d_ki);
        inv_d_ki = 1.0/d_ki;
        for (i = 0; i < endsite; i++) {
            if (dLidki[i] > 0.0) {
                patrate[i] *= d_ki;
                if (patrate[i] > ki_max) patrate[i] = ki_max;
            }
            else {
                patrate[i] *= inv_d_ki;
                if (patrate[i] < ki_min) patrate[i] = ki_min;
            }
        }

        initrav(tr->start);
        initrav(tr->start->back);
    }
} /* findSiteRates */


double  subtreeLength (p)
     nodeptr  p;
{ /* subtreeLength */
    double sum;

    sum = -fracchange * log(p->z);
    if (! p->tip) {
        sum += subtreeLength(p->next->back);
        sum += subtreeLength(p->next->next->back);
    }

    return sum;
} /* subtreeLength */


double  treeLength(tr)
     tree  *tr;
{ /* treeLength */
    double sum;

    sum = subtreeLength(tr->start->back);
    if (! tr->start->tip) {
        sum += subtreeLength(tr->start->next->back);
        sum += subtreeLength(tr->start->next->next->back);
    }

    return sum;
} /* treeLength */


void categorize (sites, categs, weight, pattern, patrate,
                 categrate, sitecateg)
     int     sites;
     int     categs;
     int     weight[];      /* one based */
     int     pattern[];     /* one based */
     double  patrate[];     /* zero based */
     double  categrate[];   /* zero based */
     int     sitecateg[];   /* one based */
{ /* categorize */
    double  ki, min_1, min_2, max_1, max_2, a, b;
    int  i, k;

    min_1 = 1.0E37;
    min_2 = 1.0E37;
    max_1 = 0.0;
    max_2 = 0.0;
    for (i = 1; i <= sites; i++) {
        if (weight[i] > 0) {
            ki = patrate[pattern[i]];
            if (ki < min_2) {
                if (ki < min_1) {
                    if (ki < 0.995 * min_1)  min_2 = min_1;
                    min_1 = ki;
                }
                else if (ki > 1.005 * min_1) {
                    min_2 = ki;
                }
            }
            else if (ki > max_2) {
                if (ki > max_1) {
                    if (ki > 1.005 * max_1)  max_2 = max_1;
                    max_1 = ki;
                }
                else if (ki < 0.995 * max_1) {
                    max_2 = ki;
                }
            }
        }
    }

    a = (categs - 3.0)/log(max_2/min_2);
    b = - a * log(min_2) + 2.0;

    categrate[0] = min_1;
    for (k = 1; k <= categs-2; k++)  categrate[k] = min_2 * exp((k-1)/a);
    categrate[categs-1] = max_1;

    for (i = 1; i <= sites; i++) {
        if (weight[i] > 0) {
            ki = patrate[pattern[i]];
            if      (ki < 0.99 * min_2) sitecateg[i] = 1;
            else if (ki > 1.00 * max_2) sitecateg[i] = categs;
            else sitecateg[i] = nint(a * log(patrate[pattern[i]]) + b);
        }
        else
            sitecateg[i] = categs;
    }
} /* categorize */


char *arb_filter;
char *alignment_name;

GBDATA *gb_main;
/** Get the calling filter, needed to expand weights afterwards */
void getArbFilter(){
    GB_begin_transaction(gb_main);
    arb_filter = GBT_read_string(gb_main,AWAR_GDE_EXPORT_FILTER);
    alignment_name = GBT_get_default_alignment(gb_main);
    GB_commit_transaction(gb_main);
}

void writeToArb(){
    char sai_name[1024];
    char type_info[1024];
    char category_string[1024];
    char *p;
    long ali_len;
    int i,k;
    int ali_pos;
    float *rates;		/* rates to export */
    char *cats;			/* categories */
    GBDATA *gb_sai;
    GBDATA *gb_data;


    GB_begin_transaction(gb_main);

    /* First create new SAI */
    sprintf(sai_name, "POS_VAR_BY_ML_%i",getpid());
    printf("Writing '%s'\n", sai_name);
    ali_len = GBT_get_alignment_len(gb_main,alignment_name);
    cats    = (char *)GB_calloc(ali_len,sizeof(char));
    rates   = (float *)GB_calloc(ali_len,sizeof(float));

    /* fill in rates and categories */
    {
        double  categrate[maxcategories]; /* rate of a given category */
        int     sitecateg[maxsites+1];    /* category of a given site */

        categorize(sites, categs, weight, pattern, patrate,categrate, sitecateg);

        i = 1;			/* thanks to pascal */
        for ( ali_pos = 0; ali_pos < ali_len; ali_pos++){
            if (arb_filter[ali_pos] == '0'){
                cats[ali_pos] = '.';
                rates[ali_pos] = KI_MAX;
                continue; /* filter says not written */
            }
            if (weight[i] > 0) {
                rates[ali_pos] = patrate[pattern[i]];
                cats[ali_pos] = itobase36(categs - sitecateg[i]);
            }else{
                rates[i] = KI_MAX;	/* infinite rate */
                cats[ali_pos] = '.';
            }
            i++;
        }

        /* write categories */
        p    = category_string;
        p[0] = 0; /* if no categs */
        for (k = 1; k <= categs; k ++) {
            sprintf(p," %G", categrate[categs-k]);
            p += strlen(p);
        }
    }


    sprintf(type_info, "PVML: Positional Variability by ML (Olsen)");
    gb_sai = GBT_create_SAI(gb_main,sai_name);
    if (!gb_sai){
        GB_print_error();
    }else{
        gb_data = GBT_add_data(gb_sai, alignment_name, "rates", GB_FLOATS);
        GB_write_floats(gb_data, rates,ali_len);
        gb_data = GBT_add_data(gb_sai, alignment_name, "data", GB_STRING);
        GB_write_string(gb_data, cats);
        gb_data = GBT_add_data(gb_sai, alignment_name, "_CATEGORIES", GB_STRING);
        GB_write_string(gb_data,category_string);
        gb_data = GBT_add_data(gb_sai, alignment_name, "_TYPE", GB_STRING);
        GB_write_string(gb_data, type_info);
    }

    GB_commit_transaction(gb_main);
}

void openArb(){
    gb_main = GB_open(":","rw");
    if (!gb_main){
        GB_warning("Cannot find ARB server");
        exit(-1);;
    }
}

void closeArb(){
    GB_close(gb_main);
}

void wrfile (outfile, sites, categs, weight, categrate, sitecateg)
     FILE   *outfile;
     int     sites;
     int     categs;
     int     weight[];      /* one based */
     double  categrate[];   /* zero based */
     int     sitecateg[];   /* one based */
{ /* wrfile */


    int  i, k, l;


    for (k = 1; k <= sites; k += 60) {
        l = k + 59;
        if (l > sites)  l = sites;
        fprintf(outfile, "%s  ", k == 1 ? "Weights   " : "          ");

        for (i = k; i <= l; i++) {
            putc(itobase36(weight[i]), outfile);
            if (((i % 10) == 0) && ((i % 60) != 0)) putc(' ', outfile);
        }

        putc('\n', outfile);
    }
    for (k = 1; k <= categs; k += 7) {
        l = k + 6;
        if (l > categs)  l = categs;
        if (k == 1)  fprintf(outfile, "C %2d", categs);
        else         fprintf(outfile, "    ");

        for (i = k-1; i < l; i++)  fprintf(outfile, " %9.5f", categrate[i]);

        putc('\n', outfile);
    }

    for (k = 1; k <= sites; k += 60) {
        l = k + 59;
        if (l > sites)  l = sites;
        fprintf(outfile, "%s  ", k == 1 ? "Categories" : "          ");

        for (i = k; i <= l; i++) {
            putc(itobase36(sitecateg[i]), outfile);
            if (((i % 10) == 0) && ((i % 60) != 0)) putc(' ', outfile);
        }

        putc('\n', outfile);
    }

} /* wrfile */


void summarize (treenum)
     int    treenum;
{ /* summarize */
    int  i;

    printf("  Site      Rate\n");
    printf("  ----   ---------\n");

    for (i = 1; i <= sites; i++) {
        if (weight[i] > 0) {
            printf("%6d %11.4f\n", i, patrate[pattern[i]]);
        }
        else
            printf("%6d   Undefined\n", i);
    }

    putchar('\n');

    if (categs > 1) {
        double  categrate[maxcategories]; /* rate of a given category */
        int     sitecateg[maxsites+1];    /* category of a given site */

        categorize(sites, categs, weight, pattern, patrate,categrate, sitecateg);

        wrfile(stdout, sites, categs, weight, categrate, sitecateg);
        putchar('\n');

        if (writefile) {
            char  filename[512];
            FILE *outfile;

            if (treenum <= 0)
                (void) sprintf(filename, "%s.%d", "weight_rate", getpid());
            else
                (void) sprintf(filename, "%s_%2d.%d", "weight_rate",
                               treenum, getpid());

            outfile = fopen(filename, "w");
            if (outfile) {
                wrfile(outfile, sites, categs, weight, categrate, sitecateg);
                (void) fclose(outfile);
                printf("Weights and categories also writen to %s\n\n", filename);
            }
        }
    }
} /* summarize */


void makeUserRates (tr)
     tree   *tr;
{ /* makeUserRates */
    double  tree_length;
    int     numtrees, which, i;

    if (fscanf(INFILE, "%d", &numtrees) != 1 || findch('\n') == EOF) {
        printf("ERROR: Problem reading number of user trees\n");
        anerror = TRUE;
        return;
    }

    printf("User-defined %s:\n\n", (numtrees == 1) ? "tree" : "trees");

    for (which = 1; which <= numtrees; which++) {
        for (i = 0; i < endsite; i++)  patrate[i] = 1.0;
        treeReadLen(tr);                                 if (anerror) break;
        tree_length = treeLength(tr);

        printf("%d taxon user-supplied tree read\n\n", tr->ntips);
        printf("Total length of tree branches = %8.6f\n\n", tree_length);

        findSiteRates(tr, 0.5/tree_length, KI_MAX, RATE_STEP, MAX_ERROR);
        if (anerror) break;

        summarize(numtrees == 1 ? 0 : which);            if (anerror) break;
    }

} /* makeUserRates */


int main ()
{ /* Maximum Likelihood Site Rate */
    tree   curtree, *tr;

#   if DebugData
    debug = fopen("ml_rates.debug","w");
#   endif

#   if ! UseStdin
    fp = fopen("infile", "rt");
#   endif

#if defined(DEBUG)
    {
        int i;
        for (i = 0; i<(2*maxsp-1); ++i) {
            curtree.nodep[i] = 0;
        }
    }
#endif /* DEBUG */

    openArb();
    getArbFilter();

    tr = & curtree;
    getinput(tr);                               if (anerror)  return 1;
    linkxarray(3, 3, & freextip, & usedxtip);   if (anerror)  return 1;
    setupnodex(tr);                             if (anerror)  return 1;
    makeUserRates(tr);                          if (anerror)  return 1;

    writeToArb();
    closeArb();

    freeTree(tr);

#   if ! UseStdin
    (void) fclose(fp);
#   endif

#   if DebugData
    fclose(debug);
#   endif

    return 0;
} /* Maximum Likelihood Site Rate */


