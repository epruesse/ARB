/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "arbdb.h"
#include "arbdbt.h"

/* -------------------------------------------------------------------------------- reverse complement functions */



char GBT_complementNucleotide(char c, char T_or_U)
{
    char n = c;

    switch (c)
    {
        case 'A': n = T_or_U; break;			/* A <-> TU */
        case 'a': n = tolower(T_or_U); break;
        case 'U':
        case 'T': n = 'A'; break;
        case 'u':
        case 't': n = 'a'; break;

        case 'C': n = 'G'; break;			/* C <-> G */
        case 'c': n = 'g'; break;
        case 'G': n = 'C'; break;
        case 'g': n = 'c'; break;

        case 'M': n = 'K'; break;			/* M=A/C <-> TU/G=K */
        case 'm': n = 'k'; break;
        case 'K': n = 'M'; break;
        case 'k': n = 'm'; break;

        case 'R': n = 'Y'; break;			/* R=A/G <-> TU/C=Y */
        case 'r': n = 'y'; break;
        case 'Y': n = 'R'; break;
        case 'y': n = 'r'; break;

        case 'V': n = 'B'; break;    			/* V=A/C/G <-> TU/G/C=B */
        case 'v': n = 'b'; break;
        case 'B': n = 'V'; break;
        case 'b': n = 'v'; break;

        case 'H': n = 'D'; break;    			/* H=A/C/TU <-> TU/G/A=D */
        case 'h': n = 'd'; break;
        case 'D': n = 'H'; break;
        case 'd': n = 'h'; break;

        case 'S':					/* S=C/G <-> G/C=S */
        case 's':
        case 'W': 					/* W=A/TU <-> TU/A=W */
        case 'w':
        case 'N':					/* N=A/C/G/TU */
        case 'n':
        case '.':
        case '-': break;

        default: break;
    }

    return n;
}

char *GBT_reverseNucSequence(const char *s, int len) {
    char *n = (char*)malloc(len+1);
    int p;

    if (!n) {
        GB_memerr();
        return 0;
    }

    len--;
    for (p=0; len>=0; p++,len--) {
        n[p] = s[len];
    }
    n[p] = 0;

    return n;
}
char *GBT_complementNucSequence(const char *s, int len, char T_or_U) {
    char *n = (char*)malloc(len+1);
    int p;

    for (p=0; p<len; p++) {
        n[p] = GBT_complementNucleotide(s[p], T_or_U);
    }
    n[p] = 0;

    return n;
}

GB_ERROR GBT_determine_T_or_U(GB_alignment_type alignment_type, char *T_or_U, const char *supposed_target) {
    switch (alignment_type)
    {
        case GB_AT_RNA: *T_or_U = 'U'; break;
        case GB_AT_DNA: *T_or_U = 'T'; break;
        default: {
            *T_or_U = 0;
            return GB_export_error("%s not available for alignment-type", supposed_target);
        }
    }
    return 0;
}

/* GB_ERROR GBT_reverseComplementNucSequence(char *seq, long length, GB_alignment_type alignment_type) */
void GBT_reverseComplementNucSequence(char *seq, long length, char T_or_U)
{
    long     i,l;
/*     char     T_or_U = 0; */
/*     GB_ERROR error  = GBT_determine_T_or_U(alignment_type, &T_or_U, "reverse-complement"); */

/*     if (!error) { */
        for (i=0,l=length-1; i <= l; i++,l--) {
            char c = seq[i];

            seq[i] = GBT_complementNucleotide(seq[l], T_or_U);
            seq[l] = GBT_complementNucleotide(c, T_or_U);
        }
/*     } */

/*     return error; */
}




