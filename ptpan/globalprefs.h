/************************************************************************
 Global prefs
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 02.02.04
 ************************************************************************/

#ifndef GLOBALPREFS_H
#define GLOBALPREFS_H

/* size of alphabet */
#define ALPHASIZE 5

/* How many codes can be stored in a LONG
   -> ALPHASIZE^MAXCODEFITLONG < 2^31 */
#define MAXCODEFITLONG 13

/* index building prefs */

/* how many characters of a prefix will be observed */
#define MAXPREFIXSIZE 5

/* how big should the quick prefix table be? */
#define MAXQPREFIXLOOKUPSIZE 7

/* ratio between bignodes and small nodes, based on experimental values --
   if you ever run out of nodes buffer, increase this value. If you see
   messages about lots of memory wasted, decrease these values. */
#define SMALLNODESPERCENT 105
#define BIGNODESPERCENT   10

/* short edge maximum length (maximum length of edge stored implicitely
   and not in the dictionary) */
#define SHORTEDGEMAX 6

/* double delta leaf compression */
//#define DOUBLEDELTALEAVES
//#define DOUBLEDELTAOPTIONAL

/* size of hash table for duplicates detection during query */
#define QUERYHITSHASHSIZE 12800009

/* size of the hash table for species lookup by name */
#define SPECIESNAMEHASHSIZE 10000

/* minimum length of probe for searching */
#define MIN_PROBE_LENGTH 8

/* error decrease for mismatches in probe design */
#define PROBE_MISM_DEC 0.2


// Allow dots in match. When building the tree, up to MAXDOTSINMATCH dots 
// in succession will be converted to SEQCODE_N. In ps_SeqDataCompressed,
// they will be saved as dots. One replaced dot will add the same mismatch
// as a replaced N. (So 0.1 at the moment)
//#define ALLOWDOTSINMATCH

// Number of allowed dots in succession
#define MAXDOTSINMATCH 5





#endif /* GLOBALPREFS_H */
