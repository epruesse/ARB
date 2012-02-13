// =============================================================== //
//                                                                 //
//   File      : arb_dnarates.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

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

// Conversion to C by Gary Olsen, 1991

#define programName     "DNAml_rates"
#define programVersion  "1.0.0"
#define programDate     "April 11, 1992"

#include "DNAml_rates_1_0.h"

#include <aw_awar_defs.hxx>
#include <arbdbt.h>

#include <unistd.h>
#include <cmath>

#define assert(bed) arb_assert(bed)

//  Global variables

static xarray
*usedxtip, *freextip;

static double
dLidki[maxpatterns],      //  change in pattern i likelihood with rate
    bestki[maxpatterns],      //  best rate for pattern i
    bestLi[maxpatterns],      //  best likelihood found for pattern i
    patrate[maxpatterns];     //  current rate for pattern i

static double
xi, xv, ttratio,          //  transition/transversion info
    freqa, freqc, freqg, freqt,  //  base frequencies
    freqr, freqy, invfreqr, invfreqy,
    freqar, freqcy, freqgr, freqty,
    fracchange;    //  random matching frequency (in a sense)

static int
info[maxsites+1],         //  number of informative nucleotides
    patsite[maxsites+1],      //  site corresponding to pattern
    pattern[maxsites+1],      //  pattern number corresponding to site
    patweight[maxsites+1],    //  weight of given pattern
    weight[maxsites+1];       //  weight of sequence site

static int
categs,        //  number of rate categories
    endsite,       //  number of unique sequence patterns
    mininfo,       //  minimum number of informative sequences for rate est
    numsp,         //  number of species
    sites,         //  number of input sequence positions
    weightsum;     //  sum of weights of positions in analysis

static bool
anerror,       //  error flag
    freqsfrom,     //  use empirical base frequencies
    interleaved,   //  input data are in interleaved format
    printdata,     //  echo data to output stream
    writefile,     //  put weight and rate data in file
    userweights;   //  use user-supplied position weight mask

static char
*y[maxsp+1];                    //  sequence data array


// =======================================================================
//                              PROGRAM
// =======================================================================

static void getnums(FILE *INFILE) {
    // input number of species, number of sites
    printf("\n%s, version %s, %s\n\n",
           programName,
           programVersion,
           programDate);
    printf("Portions based on Joseph Felsenstein's Nucleic acid sequence\n");
    printf("Maximum Likelihood method, version 3.3\n\n");

    if (fscanf(INFILE, "%d %d", &numsp, &sites) != 2) {
        printf("ERROR: Problem reading number of species and sites\n");
        anerror = true;
        return;
    }
    printf("%d Species, %d Sites\n\n", numsp, sites);

    if (numsp > maxsp) {
        printf("ERROR: Too many species; adjust program constants\n");
        anerror = true;
    }
    else if (numsp < 4) {
        printf("ERROR: Too few species (need at least 4)\n");
        anerror = true;
    }

    if (sites > maxsites) {
        printf("ERROR: Too many sites; adjust program constants\n");
        anerror = true;
    }
    else if (sites < 1) {
        printf("ERROR: Too few sites\n");
        anerror = true;
    }
}


static bool digit(int ch) { return (ch >= '0' && ch <= '9'); }


static bool white(int ch) { return (ch == ' ' || ch == '\n' || ch == '\t'); }


static void uppercase(int *chptr) {
    // convert character to upper case -- either ASCII or EBCDIC
    int  ch;
    ch = *chptr;
    if ((ch >= 'a' && ch <= 'i') || (ch >= 'j' && ch <= 'r')
        || (ch >= 's' && ch <= 'z'))
        *chptr = ch + 'A' - 'a';
}


static int base36(int ch) {
    if      (ch >= '0' && ch <= '9') return (ch - '0');
    else if (ch >= 'A' && ch <= 'I') return (ch - 'A' + 10);
    else if (ch >= 'J' && ch <= 'R') return (ch - 'J' + 19);
    else if (ch >= 'S' && ch <= 'Z') return (ch - 'S' + 28);
    else if (ch >= 'a' && ch <= 'i') return (ch - 'a' + 10);
    else if (ch >= 'j' && ch <= 'r') return (ch - 'j' + 19);
    else if (ch >= 's' && ch <= 'z') return (ch - 's' + 28);
    else return -1;
}


static int itobase36(int i) {
    if      (i <  0) return '?';
    else if (i < 10) return (i      + '0');
    else if (i < 19) return (i - 10 + 'A');
    else if (i < 28) return (i - 19 + 'J');
    else if (i < 36) return (i - 28 + 'S');
    else return '?';
}


static int findch(int c, FILE *INFILE) {
    int ch;

    while ((ch = getc(INFILE)) != EOF && ch != c) ;
    return  ch;
}


static void inputweights(FILE *INFILE) {
    // input the character weights 0, 1, 2 ... 9, A, B, ... Y, Z
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
            anerror = true;
            return;
        }
    }

    if (findch('\n', INFILE) == EOF) {      // skip to end of line
        printf("ERROR: Missing newline at end of weight data\n");
        anerror = true;
    }
}


static void getoptions(FILE *INFILE) {
    int  ch, i, extranum;

    categs      =     0;  //  Number of rate categories
    freqsfrom   = false;  //  Use empirical base frequencies
    interleaved =  true;  //  By default, data format is interleaved
    mininfo     = MIN_INFO; //  Default minimum number of informative seqs
    printdata   = false;  //  Don't echo data to output stream
    ttratio     =   2.0;  //  Transition/transversion rate ratio
    userweights = false;  //  User-defined position weights
    writefile   = false;  //  Do not write to file
    extranum    =     0;

    while ((ch = getc(INFILE)) != '\n' && ch != EOF) {
        uppercase (& ch);
        switch (ch) {
            case '1': printdata    = ! printdata; break;
            case 'C': categs       = -1; extranum++; break;
            case 'F': freqsfrom    = true; break;
            case 'I': interleaved  = ! interleaved; break;
            case 'L': break;
            case 'M': mininfo      = 0; extranum++; break;
            case 'T': ttratio      = -1.0; extranum++; break;
            case 'U': break;
            case 'W': userweights  = true; weightsum = 0; extranum++; break;
            case 'Y': writefile    = ! writefile; break;
            case ' ': break;
            case '\t': break;
            default:
                printf("ERROR: Bad option character: '%c'\n", ch);
                anerror = true;
                return;
        }
    }

    if (ch == EOF) {
        printf("ERROR: End-of-file in options list\n");
        anerror = true;
        return;
    }

    //  process lines with auxiliary data

    while (extranum-- && ! anerror) {
        ch = getc(INFILE);
        uppercase (& ch);
        switch (ch) {

            case 'C':
                if (categs >= 0) {
                    printf("ERROR: Unexpected Categories data\n");
                    anerror = true;
                }
                else if (fscanf(INFILE, "%d", &categs) != 1 || findch('\n', INFILE)==EOF) {
                    printf("ERROR: Problem reading number of rate categories\n");
                    anerror = true;
                }
                else if (categs < 1 || categs > maxcategories) {
                    printf("ERROR: Bad number of rate categories: %d\n", categs);
                    anerror = true;
                }
                break;

            case 'M':   //  Minimum informative sequences
                if (mininfo > 0) {
                    printf("ERROR: Unexpected Min informative residues data\n");
                    anerror = true;
                }
                else if (fscanf(INFILE, "%d", &mininfo)!=1 || findch('\n', INFILE)==EOF) {
                    printf("ERROR: Problem reading min informative residues\n");
                    anerror = true;
                }
                else if (mininfo < 2 || mininfo > numsp) {
                    printf("ERROR: Bad number for informative residues: %d\n",
                           mininfo);
                    anerror = true;
                }
                break;

            case 'T':   //  Transition/transversion ratio
                if (ttratio >= 0.0) {
                    printf("ERROR: Unexpected Transition/transversion data\n");
                    anerror = true;
                }
                else if (fscanf(INFILE, "%lf", &ttratio)!=1 || findch('\n', INFILE)==EOF) {
                    printf("ERROR: Problem reading transition/transversion data\n");
                    anerror = true;
                }
                break;

            case 'W':    //  Weights
                if (! userweights || weightsum > 0) {
                    printf("ERROR: Unexpected Weights data\n");
                    anerror = true;
                }
                else {
                    inputweights(INFILE);
                }
                break;

            default:
                printf("ERROR: Auxiliary data line starts with '%c'\n", ch);
                anerror = true;
                break;
        }
    }

    if (anerror) return;

    if (categs < 0) {
        printf("ERROR: Category data missing from input\n");
        anerror = true;
        return;
    }

    if (mininfo <= 0) {
        printf("ERROR: Minimum informative residues missing from input\n");
        anerror = true;
        return;
    }
    else {
        printf("There must be at least %d informative residues per column\n\n", mininfo);
    }

    if (ttratio < 0.0) {
        printf("ERROR: Transition/transversion data missing from input\n");
        anerror = true;
        return;
    }

    if (! userweights) {
        for (i = 1; i <= sites; i++)  weight[i] = 1;
        weightsum = sites;
    }
    else if (weightsum < 1) {
        printf("ERROR: Weight data invalid or missing from input\n");
        anerror = true;
        return;
    }

}


static void getbasefreqs(FILE *INFILE) {
    double  suma, sumb;

    if (freqsfrom)  printf("Empirical ");
    printf("Base Frequencies:\n\n");

    if (! freqsfrom) {
        if (fscanf(INFILE, "%lf%lf%lf%lf", &freqa, &freqc, &freqg, &freqt) != 4
            || findch('\n', INFILE) == EOF) {
            printf("ERROR: Problem reading user base frequencies\n");
            anerror = true;
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
}


static void getyspace() {
    long size;
    int  i;
    char *y0;

    size = 4 * (sites/4 + 1);
    if (! (y0 = (char*)malloc((unsigned) (sizeof(char) * size * (numsp+1))))) {
        printf("ERROR: Unable to obtain space for data array\n");
        anerror = true;
    }
    else {
        for (i = 0; i <= numsp; i++) {
            y[i] = y0;
            y0 += size;
        }
    }
}


static void setuptree(tree *tr, int numSp) {
    int     i, j;
    nodeptr p = 0, q;

    for (i = 1; i <= numSp; i++) {   //  Set-up tips
        if ((anerror = !(p = (nodeptr) malloc((unsigned) sizeof(node))))) break;
        p->x      = (xarray *) NULL;
        p->tip    = (char *) NULL;
        p->number = i;
        p->next   = (nodeptr) NULL;
        p->back   = (nodeptr) NULL;
        tr->nodep[i] = p;
    }

    for (i = numSp+1; i <= 2*numSp-1 && ! anerror; i++) { // Internal nodes    **  was : 2*numSp-2 (ralf)
        q = (nodeptr) NULL;
        for (j = 1; j <= 3; j++) {
            if ((anerror = !(p = (nodeptr) malloc((unsigned) sizeof(node))))) break;
            p->x      = (xarray *) NULL;
            p->tip    = (char *) NULL;
            p->number = i;
            p->next   = q;
            p->back   = (nodeptr) NULL;
            q = p;
        }
        if (anerror) break;
        p->next->next->next = p;
        tr->nodep[i] = p;
    }

    tr->likelihood = unlikely;
    tr->start      = tr->nodep[1];
    tr->mxtips     = numSp;
    tr->ntips      = 0;
    tr->nextnode   = 0;
    tr->opt_level  = 0;
    tr->smoothed   = false;
    if (anerror) printf("ERROR: Unable to obtain sufficient memory");
}


static void freeTreeNode(nodeptr p) {
    //  Free a tree node (sector)
    if (p) {
        if (p->x) {
            free(p->x->a);
            free(p->x);
        }
        free(p);
    }
}

static void freeTree(tree *tr) {
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
}


static void getdata(tree *tr, FILE *INFILE) {
     // read sequences
    int  i, j, k, l, basesread, basesnew, ch;
    int  meaning[256];          //  meaning of input characters
    char *nameptr;
    bool  allread, firstpass;

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

    allread = false;
    firstpass = true;
    ch = ' ';

    while (! allread) {
        for (i = 1; i <= numsp; i++) {          //  Read data line
            if (firstpass) {                      //  Read species names
                j = 1;
                while (white(ch = getc(INFILE))) {  //  Skip blank lines
                    if (ch == '\n')  j = 1;  else  j++;
                }

                if (j > nmlngth) {
                    printf("ERROR: Blank name for species %d; ", i);
                    printf("check number of species,\n");
                    printf("       number of sites, and interleave option.\n");
                    anerror = true;
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
                *nameptr = '\0';                      //  add null termination

                if (ch == EOF) {
                    printf("ERROR: End-of-file in name of species %d\n", i);
                    anerror = true;
                    return;
                }
            }

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
                            anerror = true;
                            return;
                        }
                    }
                    y[i][j] = ch;
                }
                else if (white(ch) || digit(ch)) ;
                else {
                    printf("ERROR: Bad base (%c) at site %d of sequence %d\n",
                           ch, j, i);
                    anerror = true;
                    return;
                }
            }

            if (ch == EOF) {
                printf("ERROR: End-of-file at site %d of sequence %d\n", j, i);
                anerror = true;
                return;
            }

            if (! firstpass && (j == basesread)) i--;        // no data on line
            else if (i == 1) basesnew = j;
            else if (j != basesnew) {
                printf("ERROR: Sequences out of alignment\n");
                printf("       %d (instead of %d) residues read in sequence %d\n",
                       j - basesread, basesnew - basesread, i);
                anerror = true;
                return;
            }

            while (ch != '\n' && ch != EOF) ch = getc(INFILE); // flush line
        }
        firstpass = false;
        basesread = basesnew;
        allread = (basesread >= sites);
    }

    //  Print listing of sequence alignment

    if (printdata) {
        j = nmlngth - 5 + ((sites + ((sites-1)/10))/2);
        if (j < nmlngth - 1) j = nmlngth - 1;
        if (j > 37) j = 37;
        printf("Name"); for (i=1; i<=j; i++) putchar(' '); printf("Sequences\n");
        printf("----"); for (i=1; i<=j; i++) putchar(' '); printf("---------\n");
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

    //  Convert characters to meanings

    for (i = 1; i <= sites; i++)  info[i] = 0;

    for (j = 1; j <= numsp; j++) {
        for (i = 1; i <= sites; i++) {
            if ((y[j][i] = meaning[(int)y[j][i]]) != 15) info[i]++;
        }
    }

    for (i = 1; i <= sites; i++)  if (info[i] < MIN_INFO)  weight[i] = 0;

}


static void sitesort() {
    // Shell sort keeping sites, weights in same order
    int  gap, i, j, jj, jg, k;
    bool  flip, tied;

    for (gap = sites/2; gap > 0; gap /= 2) {
        for (i = gap + 1; i <= sites; i++) {
            j = i - gap;

            do {
                jj = patsite[j];
                jg = patsite[j+gap];
                flip = false;
                tied = true;
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
}


static void sitecombcrunch() {
    // combine sites that have identical patterns (and nonzero weight)
    int  i, sitei, j, sitej, k;
    bool  tied;

    i = 0;
    patsite[0] = patsite[1];
    patweight[0] = 0;

    for (j = 1; j <= sites; j++) {
        tied = true;
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
}


static void makeweights() {
    // make up weights vector to avoid duplicate computations
    int  i;

    for (i = 1; i <= sites; i++)  patsite[i] = i;
    sitesort();
    sitecombcrunch();
    if (endsite > maxpatterns) {
        printf("ERROR:  Too many patterns in data\n");
        printf("        Increase maxpatterns to at least %d\n", endsite);
        anerror = true;
    }
    else {
        printf("Analyzing %d distinct data patterns (columns)\n\n", endsite);
    }
}


static void makevalues(tree *tr) {
    // set up fractional likelihoods at tips
    nodeptr  p;
    int  i, j;

    for (i = 1; i <= numsp; i++) {    // Pack and move tip data
        for (j = 0; j < endsite; j++)
            y[i-1][j] = y[i][patsite[j]];

        p = tr->nodep[i];
        p->tip = &(y[i-1][0]);
    }
}


static void empiricalfreqs(tree *tr) {
     // Get empirical base frequencies from the data
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
                fa = freqa * (code       & 1);
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
}


static void getinput(tree *tr, FILE *INFILE) {
    getnums(INFILE);                      if (anerror) return;
    getoptions(INFILE);                   if (anerror) return;
    if (!freqsfrom) getbasefreqs(INFILE); if (anerror) return;
    getyspace();                          if (anerror) return;
    setuptree(tr, numsp);                 if (anerror) return;
    getdata(tr, INFILE);                  if (anerror) return;
    makeweights();                        if (anerror) return;
    makevalues (tr);                      if (anerror) return;
    if (freqsfrom) {
        empiricalfreqs (tr);              if (anerror) return;
        getbasefreqs(INFILE);
    }
}


static xarray *setupxarray() {
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
            x->owner = (nodeptr) NULL;
        }
        else {
            free(x);
            return (xarray *) NULL;
        }
    }
    return x;
}


static void linkxarray(int req, int min, xarray **freexptr, xarray **usedxptr) {
    //  Link a set of xarrays
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
            if (i < min) anerror = true;
        }
    } while ((i < req) && x);

    if (first) {
        first->prev = prev;
        prev->next = first;
    }

    *freexptr = first;
    *usedxptr = (xarray *) NULL;
}


static void setupnodex(tree *tr) {
    nodeptr  p;
    int  i;

    for (i = tr->mxtips + 1; (i <= 2*(tr->mxtips) - 2) && ! anerror; i++) {
        p = tr->nodep[i];
        if ((anerror = !(p->x = setupxarray()))) break;
    }
}

static xarray *getxtip(nodeptr p) {
    xarray *new_xarray;
    bool  splice;

    if (! p) return (xarray *) NULL;

    splice = false;

    if (p->x) {
        new_xarray = p->x;
        if (new_xarray == new_xarray->prev) ;             // linked to self; leave it
        else if (new_xarray == usedxtip) usedxtip = usedxtip->next; // at head
        else if (new_xarray == usedxtip->prev) ;   // already at tail
        else {                              // move to tail of list
            new_xarray->prev->next = new_xarray->next;
            new_xarray->next->prev = new_xarray->prev;
            splice                 = true;
        }
    }

    else if (freextip) {
        p->x = new_xarray = freextip;
        new_xarray->owner = p;
        if (new_xarray->prev != new_xarray) {            // not only member of freelist
            new_xarray->prev->next = new_xarray->next;
            new_xarray->next->prev = new_xarray->prev;
            freextip               = new_xarray->next;
        }
        else
            freextip = (xarray *) NULL;

        splice = true;
    }

    else if (usedxtip) {
        usedxtip->owner->x = (xarray *) NULL;
        p->x               = new_xarray = usedxtip;
        new_xarray->owner  = p;
        usedxtip           = usedxtip->next;
    }

    else {
        printf ("ERROR: Unable to locate memory for a tip.\n");
        anerror = true;
        exit(0);
    }

    if (splice) {
        if (usedxtip) {                  // list is not empty
            usedxtip->prev->next = new_xarray;
            new_xarray->prev     = usedxtip->prev;
            usedxtip->prev       = new_xarray;
            new_xarray->next     = usedxtip;
        }
        else
            usedxtip = new_xarray->prev = new_xarray->next = new_xarray;
    }

    return  new_xarray;
}


static xarray *getxnode(nodeptr p) {
    // Ensure that internal node p has memory
    nodeptr  s;

    if (! (p->x)) {  //  Move likelihood array on this node to sector p
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
}


static void newview(nodeptr p) {
    //  Update likelihoods at node
    double   z1, lz1, xvlz1, z2, lz2, xvlz2,
        zz1, zv1, fx1r, fx1y, fx1n, suma1, sumg1, sumc1, sumt1,
        zz2, zv2, fx2r, fx2y, fx2n, ki, tempi, tempj;
    nodeptr  q, r;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t,
        *x3a, *x3c, *x3g, *x3t;
    int      i;

    if (p->tip) {             //  Make sure that data are at tip
        int code;
        char *yptr;

        if (p->x) return;       //  They are already there
        (void) getxtip(p);      //  They are not, so get memory
        x3a = &(p->x->a[0]);    //  Move tip data to xarray
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

    //  Internal node needs update

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
}


static void hookup(nodeptr p, nodeptr q, double z) {
    p->back = q;
    q->back = p;
    p->z = q->z = z;
}


static void initrav(nodeptr p) {
    if (! p->tip) {
        initrav(p->next->back);
        initrav(p->next->next->back);
        newview(p);
    }
}

// =======================================================================
//                         Read a tree from a file
// =======================================================================

static int treeFinishCom(FILE *INFILE) {
    int  ch;
    bool inquote;

    inquote = false;
    while ((ch = getc(INFILE)) != EOF && (inquote || ch != ']')) {
        if (ch == '[' && ! inquote) {             // comment; find its end
            if ((ch = treeFinishCom(INFILE)) == EOF)  break;
        }
        else if (ch == '\'') inquote = ! inquote;  // start or end of quote
    }

    return  ch;
}


static int treeGetCh(FILE *INFILE) {
    // get next nonblank, noncomment character
    int  ch;

    while ((ch = getc(INFILE)) != EOF) {
        if (white(ch)) ;
        else if (ch == '[') {                   // comment; find its end
            if ((ch = treeFinishCom(INFILE)) == EOF)  break;
        }
        else  break;
    }

    return  ch;
}

static void treeFlushLabel(FILE *INFILE) {
    int  ch;
    bool done;

    if ((ch = treeGetCh(INFILE)) == EOF)  return;
    done = (ch == ':' || ch == ',' || ch == ')'  || ch == '[' || ch == ';');

    if (!done) {
        bool quoted = (ch == '\'');
        if (quoted) ch = getc(INFILE);

        while (! done) {
            if (quoted) {
                if ((ch = findch('\'', INFILE)) == EOF)  return;      // find close quote
                ch = getc(INFILE);                            // check next char
                if (ch != '\'') done = true;                  // not doubled quote
            }
            else if (ch == ':' || ch == ',' || ch == ')'  || ch == '['
                     || ch == ';' || ch == '\n' || ch == EOF) {
                done = true;
            }
            if (! done)  done = ((ch = getc(INFILE)) == EOF);
        }
    }

    if (ch != EOF)  (void) ungetc(ch, INFILE);
}


static int findTipName(tree *tr, int ch, FILE *INFILE) {
    nodeptr  q;
    char    *nameptr, str[nmlngth+1];
    int      i, n;
    bool     found, quoted, done;

    if ((quoted = (ch == '\'')))  ch = getc(INFILE);
    done = false;
    i = 0;

    do {
        if (quoted) {
            if (ch == '\'') {
                ch = getc(INFILE);
                if (ch != '\'') done = true;
            }
            else if (ch == EOF)
                done = true;
            else if (ch == '\n' || ch == '\t')
                ch = ' ';
        }
        else if (ch == ':' || ch == ','  || ch == ')'  || ch == '['
                 || ch == '\n' || ch == EOF)
            done = true;
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
    while (i < nmlngth)  str[i++] = ' ';     //  Pad name

    n = 1;
    do {
        q = tr->nodep[n];
        if (! (q->back)) {          //  Only consider unused tips
            i = 0;
            nameptr = q->name;
            do { found = str[i] == *nameptr++; } while (found && (++i < nmlngth));
        }
        else
            found = false;
    } while ((! found) && (++n <= tr->mxtips));

    if (! found) {
        i = nmlngth;
        do { str[i] = '\0'; } while (i-- && (str[i] <= ' '));
        printf("ERROR: Cannot find data for tree species: %s\n", str);
    }

    return  (found ? n : 0);
}


static double processLength(FILE *INFILE) {
    double  branch;
    int     ch;
    char    string[41];

    ch = treeGetCh(INFILE);                            //  Skip comments
    if (ch != EOF)  (void) ungetc(ch, INFILE);

    if (fscanf(INFILE, "%lf", &branch) != 1) {
        printf("ERROR: Problem reading branch length in processLength:\n");
        if (fscanf(INFILE, "%40s", string) == 1)  printf("%s\n", string);
        anerror = true;
        branch = 0.0;
    }

    return  branch;
}


static void treeFlushLen(FILE *INFILE) {
    int  ch;

    if ((ch = treeGetCh(INFILE)) == ':')
        (void) processLength(INFILE);
    else if (ch != EOF)
        (void) ungetc(ch, INFILE);
}


static void treeNeedCh(int c1, const char *where, FILE *INFILE) {
    int c2, i;

    if ((c2 = treeGetCh(INFILE)) == c1)  return;

    printf("ERROR: Missing '%c' %s tree; ", c1, where);
    if (c2 == EOF)
        printf("End-of-File");
    else {
        putchar('\'');
        for (i = 24; i-- && (c2 != EOF); c2 = getc(INFILE))  putchar(c2);
        putchar('\'');
    }
    printf(" found instead\n");
    anerror = true;
}

static void addElementLen(tree *tr, nodeptr p, FILE *INFILE) {
    double   z, branch;
    nodeptr  q;
    int      n, ch;

    if ((ch = treeGetCh(INFILE)) == '(') {     //  A new internal node
        n = (tr->nextnode)++;
        if (n > 2*(tr->mxtips) - 2) {
            if (tr->rooted || n > 2*(tr->mxtips) - 1) {
                printf("ERROR: Too many internal nodes.  Is tree rooted?\n");
                printf("       Deepest splitting should be a trifurcation.\n");
                anerror = true;
                return;
            }
            else {
                tr->rooted = true;
            }
        }
        q = tr->nodep[n];
        assert(q);
        addElementLen(tr, q->next, INFILE);       if (anerror)  return;
        treeNeedCh(',', "in", INFILE);            if (anerror)  return;
        assert(q->next);
        addElementLen(tr, q->next->next, INFILE); if (anerror)  return;
        treeNeedCh(')', "in", INFILE);            if (anerror)  return;
        treeFlushLabel(INFILE);                   if (anerror)  return;
    }

    else {                               //  A new tip
        n = findTipName(tr, ch, INFILE);
        if (n <= 0) { anerror = true; return; }
        q = tr->nodep[n];
        if (tr->start->number > n)  tr->start = q;
        (tr->ntips)++;
    }

    treeNeedCh(':', "in", INFILE);     if (anerror)  return;
    branch = processLength(INFILE);    if (anerror)  return;
    z = exp(-branch / fracchange);
    if (z > zmax)  z = zmax;
    hookup(p, q, z);
}


static void uprootTree(tree *tr, nodeptr p) {
    nodeptr  q, r, s;
    int  n;

    if (p->tip || p->back) {
        printf("ERROR: Unable to uproot tree.\n");
        printf("       Inappropriate node marked for removal.\n");
        anerror = true;
        return;
    }

    n = --(tr->nextnode);               // last internal node added
    if (n != tr->mxtips + tr->ntips - 1) {
        printf("ERROR: Unable to uproot tree.  Inconsistent\n");
        printf("       number of tips and nodes for rooted tree.\n");
        anerror = true;
        return;
    }

    q = p->next->back;                  // remove p from tree
    r = p->next->next->back;
    hookup(q, r, q->z * r->z);

    q = tr->nodep[n];
    r = q->next;
    s = q->next->next;
    if (tr->ntips > 2 && p != q && p != r && p != s) {
        hookup(p,             q->back, q->z);   // move connections to p
        hookup(p->next,       r->back, r->z);
        hookup(p->next->next, s->back, s->z);
    }

    q->back = r->back = s->back = (nodeptr) NULL;
    tr->rooted = false;
}


static void treeReadLen(tree *tr, FILE *INFILE) {
    nodeptr  p;
    int  i, ch;

    for (i = 1; i <= tr->mxtips; i++) tr->nodep[i]->back = (nodeptr) NULL;
    tr->start       = tr->nodep[tr->mxtips];
    tr->ntips       = 0;
    tr->nextnode    = tr->mxtips + 1;
    tr->opt_level   = 0;
    tr->smoothed    = false;
    tr->rooted      = false;

    p = tr->nodep[(tr->nextnode)++];
    treeNeedCh('(', "at start of", INFILE);          if (anerror)  return;
    addElementLen(tr, p, INFILE);                    if (anerror)  return;
    treeNeedCh(',', "in", INFILE);                   if (anerror)  return;
    addElementLen(tr, p->next, INFILE);              if (anerror)  return;
    if (! tr->rooted) {
        if ((ch = treeGetCh(INFILE)) == ',') {        //  An unrooted format
            addElementLen(tr, p->next->next, INFILE); if (anerror) return;
        }
        else {                                  //  A rooted format
            p->next->next->back = (nodeptr) NULL;
            tr->rooted = true;
            if (ch != EOF)  (void) ungetc(ch, INFILE);
        }
    }
    treeNeedCh(')', "in", INFILE);
    if (anerror) {
        printf("(This error also happens if the last species in the tree is unmarked)\n");
        return;
    }


    treeFlushLabel(INFILE);               if (anerror)  return;
    treeFlushLen(INFILE);                 if (anerror)  return;
    treeNeedCh(';', "at end of", INFILE); if (anerror)  return;

    if (tr->rooted)  uprootTree(tr, p->next->next);  if (anerror)  return;
    tr->start = p->next->next->back;  // This is start used by treeString

    initrav(tr->start);
    initrav(tr->start->back);
}

// =======================================================================
//                           End of Tree Reading
// =======================================================================


static double evaluate(tree *tr, nodeptr p) {
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
}


static void dli_dki(nodeptr p) {
    //  d(Li)/d(ki)
    double   z, lz, xvlz;
    double   ki, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
        suma, sumb, sumc;
    double  *rptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    nodeptr  q;
    int      i, *wptr;


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
}

static void spanSubtree(nodeptr p) {
    dli_dki (p);

    if (! p->tip) {
        spanSubtree(p->next->back);
        spanSubtree(p->next->next->back);
    }
}


static void findSiteRates(tree *tr, double ki_min, double ki_max, double d_ki, double max_error) {
    double  inv_d_ki, ki;
    int     i;

    if (ki_min <= 0.0 || ki_max <= ki_min) {
        printf("ERROR: Bad rate value limits to findSiteRates\n");
        anerror = true;
        return;
    }
    else if (d_ki <= 1.0) {
        printf("ERROR: Bad rate step to findSiteRates\n");
        anerror = true;
        return;
    }

    for (i = 0; i < endsite; i++) {
        bestki[i] = 1.0;                       //  dummy initial rates
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
}


static double subtreeLength(nodeptr p) {
    double sum;

    sum = -fracchange * log(p->z);
    if (! p->tip) {
        sum += subtreeLength(p->next->back);
        sum += subtreeLength(p->next->next->back);
    }

    return sum;
}


static double treeLength(tree *tr) {
    double sum;

    sum = subtreeLength(tr->start->back);
    if (! tr->start->tip) {
        sum += subtreeLength(tr->start->next->back);
        sum += subtreeLength(tr->start->next->next->back);
    }

    return sum;
}


static void categorize(int    Sites,
                int    Categs,
                int    Weight[],                    // one based
                int    Pattern[],                   // one based
                double Patrate[],                   // zero based
                double categrate[],                 // zero based
                int    sitecateg[])                 // one based
{
    double  ki, min_1, min_2, max_1, max_2, a, b;
    int  i, k;

    min_1 = 1.0E37;
    min_2 = 1.0E37;
    max_1 = 0.0;
    max_2 = 0.0;
    for (i = 1; i <= Sites; i++) {
        if (Weight[i] > 0) {
            ki = Patrate[Pattern[i]];
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

    a = (Categs - 3.0)/log(max_2/min_2);
    b = - a * log(min_2) + 2.0;

    categrate[0] = min_1;
    for (k = 1; k <= Categs-2; k++)  categrate[k] = min_2 * exp((k-1)/a);
    categrate[Categs-1] = max_1;

    for (i = 1; i <= Sites; i++) {
        if (Weight[i] > 0) {
            ki = Patrate[Pattern[i]];
            if      (ki < 0.99 * min_2) sitecateg[i] = 1;
            else if (ki > 1.00 * max_2) sitecateg[i] = Categs;
            else sitecateg[i] = nint(a * log(Patrate[Pattern[i]]) + b);
        }
        else
            sitecateg[i] = Categs;
    }
}


static char   *arb_filter;
static char   *alignment_name;
static GBDATA *gb_main;

static void getArbFilter() {
    //! Get the calling filter, needed to expand weights afterwards
    GB_begin_transaction(gb_main);
    arb_filter     = GBT_read_string(gb_main, AWAR_GDE_EXPORT_FILTER);
    alignment_name = GBT_get_default_alignment(gb_main);
    GB_commit_transaction(gb_main);
}

static int get_next_SAI_count() {
    GBDATA *gb_sai_count = GB_search(gb_main, "arb_dnarates/sai_count", GB_FIND);
    if (!gb_sai_count) {
        gb_sai_count = GB_search(gb_main, "arb_dnarates/sai_count", GB_INT);
    }
    int count = GB_read_int(gb_sai_count);
    count++;
    GB_write_int(gb_sai_count, count);
    return count;
}

static GBDATA *create_next_SAI() {
    GBDATA *gb_sai = NULL;
    char    sai_name[100];

    while (1) {
        sprintf(sai_name, "POS_VAR_BY_ML_%i", get_next_SAI_count());
        gb_sai = GBT_find_SAI(gb_main, sai_name);
        if (!gb_sai) { // sai_name is not used yet
            gb_sai = GBT_find_or_create_SAI(gb_main, sai_name);
            printf("Writing '%s'\n", sai_name);
            break; 
        }
    }
    return gb_sai;
}

static void writeToArb() {
    char    type_info[1024];
    char    category_string[1024];
    char   *p;
    long    ali_len;
    int     i, k;
    int     ali_pos;
    float  *rates;              // rates to export
    char   *cats;               // categories
    GBDATA *gb_data;


    GB_begin_transaction(gb_main);

    ali_len = GBT_get_alignment_len(gb_main, alignment_name);
    cats    = (char *)GB_calloc(ali_len, sizeof(char));
    rates   = (float *)GB_calloc(ali_len, sizeof(float));

    // fill in rates and categories
    {
        double  categrate[maxcategories]; // rate of a given category
        int     sitecateg[maxsites+1];    // category of a given site

        categorize(sites, categs, weight, pattern, patrate, categrate, sitecateg);

        i = 1;                  // thanks to pascal
        for (ali_pos = 0; ali_pos < ali_len; ali_pos++) {
            if (arb_filter[ali_pos] == '0') {
                cats[ali_pos] = '.';
                rates[ali_pos] = KI_MAX;
                continue; // filter says not written
            }
            if (weight[i] > 0) {
                rates[ali_pos] = patrate[pattern[i]];
                cats[ali_pos] = itobase36(categs - sitecateg[i]);
            }
            else {
                rates[i] = KI_MAX;      // infinite rate
                cats[ali_pos] = '.';
            }
            i++;
        }

        // write categories
        p    = category_string;
        p[0] = 0; // if no categs
        for (k = 1; k <= categs; k ++) {
            sprintf(p, " %G", categrate[categs-k]);
            p += strlen(p);
        }
    }


    sprintf(type_info, "PVML: Positional Variability by ML (Olsen)");

    {
        GBDATA *gb_sai = create_next_SAI();
        if (!gb_sai) {
            fprintf(stderr, "Error: %s\n", GB_await_error());
        }
        else {
            gb_data = GBT_add_data(gb_sai, alignment_name, "rates", GB_FLOATS);
            GB_write_floats(gb_data, rates, ali_len);
            gb_data = GBT_add_data(gb_sai, alignment_name, "data", GB_STRING);
            GB_write_string(gb_data, cats);
            gb_data = GBT_add_data(gb_sai, alignment_name, "_CATEGORIES", GB_STRING);
            GB_write_string(gb_data, category_string);
            gb_data = GBT_add_data(gb_sai, alignment_name, "_TYPE", GB_STRING);
            GB_write_string(gb_data, type_info);
        }
    }

    GB_commit_transaction(gb_main);
    }

static void openArb(const char *dbname) {
    gb_main = GB_open(dbname, "rw");
    if (!gb_main) {
        if (strcmp(dbname, ":") == 0) {
            GB_warning("Cannot find ARB server");
        }
        else {
            GB_warningf("Cannot open DB '%s'", dbname); 
        }
        exit(EXIT_FAILURE);
    }
}

static void saveArb(const char *saveAs) {
    GB_ERROR error = GB_save(gb_main, saveAs, "a");
    if (error) {
        GB_warningf("Error saving '%s': %s", saveAs, error);
        exit(EXIT_FAILURE);
    }
}

static void closeArb() {
    GB_close(gb_main);
}

static void wrfile(FILE   *outfile,
            int     Sites,
            int     Categs,
            int     Weight[],   // one based
            double  categrate[], // zero based
            int     sitecateg[]) // one based
{


    int  i, k, l;


    for (k = 1; k <= Sites; k += 60) {
        l = k + 59;
        if (l > Sites)  l = Sites;
        fprintf(outfile, "%s  ", k == 1 ? "Weights   " : "          ");

        for (i = k; i <= l; i++) {
            putc(itobase36(Weight[i]), outfile);
            if (((i % 10) == 0) && ((i % 60) != 0)) putc(' ', outfile);
        }

        putc('\n', outfile);
    }
    for (k = 1; k <= Categs; k += 7) {
        l = k + 6;
        if (l > Categs)  l = Categs;
        if (k == 1)  fprintf(outfile, "C %2d", Categs);
        else         fprintf(outfile, "    ");

        for (i = k-1; i < l; i++)  fprintf(outfile, " %9.5f", categrate[i]);

        putc('\n', outfile);
    }

    for (k = 1; k <= Sites; k += 60) {
        l = k + 59;
        if (l > Sites)  l = Sites;
        fprintf(outfile, "%s  ", k == 1 ? "Categories" : "          ");

        for (i = k; i <= l; i++) {
            putc(itobase36(sitecateg[i]), outfile);
            if (((i % 10) == 0) && ((i % 60) != 0)) putc(' ', outfile);
        }

        putc('\n', outfile);
    }

}


static void summarize(int treenum) {
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
        double  categrate[maxcategories]; // rate of a given category
        int     sitecateg[maxsites+1];    // category of a given site

        categorize(sites, categs, weight, pattern, patrate, categrate, sitecateg);

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
                printf("Weights and categories also written to %s\n\n", filename);
            }
        }
    }
}


static void makeUserRates(tree *tr, FILE *INFILE) {
    double  tree_length;
    int     numtrees, which, i;

    if (fscanf(INFILE, "%d", &numtrees) != 1 || findch('\n', INFILE) == EOF) {
        printf("ERROR: Problem reading number of user trees\n");
        anerror = true;
        return;
    }

    printf("User-defined %s:\n\n", (numtrees == 1) ? "tree" : "trees");

    for (which = 1; which <= numtrees; which++) {
        for (i = 0; i < endsite; i++)  patrate[i] = 1.0;
        
        treeReadLen(tr, INFILE);
        if (anerror) break;

        tree_length = treeLength(tr);

        printf("%d taxon user-supplied tree read\n\n", tr->ntips);
        printf("Total length of tree branches = %8.6f\n\n", tree_length);

        findSiteRates(tr, 0.5/tree_length, KI_MAX, RATE_STEP, MAX_ERROR);
        if (anerror) break;

        summarize(numtrees == 1 ? 0 : which);            if (anerror) break;
    }

}

inline bool is_char(const char *name, char c) { return name[0] == c && !name[1]; }
inline bool wantSTDIN(const char *iname) { return is_char(iname, '-'); }

int ARB_main(int argc, const char *argv[]) {
    // Maximum Likelihood Site Rate
    const char *dbname     = ":";
    const char *dbsavename = NULL;
    bool        help       = false;
    const char *error      = NULL;
    const char *inputname  = NULL;
    FILE       *infile     = NULL;

    switch (argc) {
        case 3: error = "'dbname' may only be used together with 'dbsavename'"; break;
            
        case 4:
            dbname             = argv[2];
            dbsavename         = argv[3];
            // fall-through
        case 2:
            inputname          = argv[1];
            infile             = wantSTDIN(inputname) ? stdin : fopen(inputname, "rt");
            if (!infile) error = GB_IO_error("reading", inputname);
            break;

        case 0:
        case 1: error = "missing arguments"; break;

        default : error = "too many arguments"; break;
    }

    if (error) {
        fprintf(stderr, "arb_dnarates: Error: %s\n", error);
        help = true;
    }

    if (help) {
        fputs("Usage: arb_dnarates input [dbname dbsavename]\n"
              "       Expects phylip sequence data as 'input'.\n"
              "       Reads from STDIN if 'input' is '-'.\n"
              "       (e.g. cat data.phyl | arb_dnarates - ...\n"
              "          or arb_dnarates data.phyl ...)\n"
              "       Expects a 'dbname' or a running ARB DB server.\n"
              "       - Reads " AWAR_GDE_EXPORT_FILTER " from server.\n"
              "       - Saves calculated SAI to server (POS_VAR_BY_ML_...)\n"
              "       Using 'dbname' does only make sense for unittests!\n"
              , stderr);
        exit(EXIT_FAILURE);
    }

    // using arb_dnarates only makes sense with a running db server
    // (because result is written there)
    GB_shell shell;
    openArb(dbname);
    getArbFilter(); // Note: expects AWAR_GDE_EXPORT_FILTER in running db server

    {
        tree curtree;
        for (int i = 0; i<(2*maxsp-1); ++i) curtree.nodep[i] = 0;

        tree *tr = &curtree;
        getinput(tr, infile);                       if (anerror)  return 1;
        linkxarray(3, 3, & freextip, & usedxtip);   if (anerror)  return 1;
        setupnodex(tr);                             if (anerror)  return 1;
        makeUserRates(tr, infile);                  if (anerror)  return 1;

        writeToArb();
        if (dbsavename) saveArb(dbsavename);
        closeArb();

        freeTree(tr);
    }

    if (wantSTDIN(inputname)) fclose(infile);

    return EXIT_SUCCESS;
}
