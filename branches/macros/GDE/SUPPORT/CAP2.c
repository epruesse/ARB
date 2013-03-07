/* CONTIG ASSEMBLY PROGRAM (CAP)

   copyright (c) 1991	Xiaoqiu Huang
   The distribution of the program is granted provided no charge
   is made and the copyright notice is included.

   Proper attribution of the author as the source of the software
   would be appreciated:
   "A Contig Assembly Program Based on Sensitive Detection of
   Fragment Overlaps" (submitted to Genomics, 1991)
	Xiaoqiu Huang
	Department of Computer Science
	Michigan Technological University
	Houghton, MI 49931
        E-mail: huang@cs.mtu.edu

   The CAP program uses a dynamic programming algorithm to compute
   the maximal-scoring overlapping alignment between two fragments.
   Fragments in random orientations are assembled into contigs by a
   greedy approach in order of the overlap scores. CAP is efficient
   in computer memory: a large number of arbitrarily long fragments
   can be assembled. The time requirement is acceptable; for example,
   CAP took 4 hours to assemble 1015 fragments of a total of 252 kb
   nucleotides on a Sun SPARCstation SLC. The program is written in C
   and runs on Sun workstations.

   Below is a description of the parameters in the #define section of CAP.
   Two specially chosen sets of substitution scores and indel penalties
   are used by the dynamic programming algorithm: heavy set for regions
   of low sequencing error rates and light set for fragment ends of high
   sequencing error rates. (Use integers only.)
	Heavy set:			 Light set:

	MATCH     =  2			 MATCH     =  2
	MISMAT    = -6			 LTMISM    = -3
	EXTEND    =  4			 LTEXTEN   =  2

    In the initial assembly, any overlap must be of length at least OVERLEN,
    and any overlap/containment must be of identity percentage at least
    PERCENT. After the initial assembly, the program attempts to join
    contigs together using weak overlaps. Two contigs are merged if the
    score of the overlapping alignment is at least CUTOFF. The value for
    CUTOFF is chosen according to the value for MATCH.

    DELTA is a parameter in necessary conditions for overlap/containment.
    Those conditions are used to quickly reject pairs of fragments that
    could not possibly have an overlap/containment relationship.
    The dynamic programming algorithm is only applied to pairs of fragments
    that pass the screening. A large value for DELTA means stringent
    conditions, where the value for DELTA is a real number at least 8.0.

    POS5 and POS3 are fragment positions such that the 5' end between base 1
    and base POS5, and the 3' end after base POS3 are of high sequencing
    error rates, say more than 5%. For mismatches and indels occurring in
    the two ends, light penalties are used.

    A file of input fragments looks like:
>G019uabh
ATACATCATAACACTACTTCCTACCCATAAGCTCCTTTTAACTTGTTAAA
GTCTTGCTTGAATTAAAGACTTGTTTAAACACAAAAATTTAGAGTTTTAC
TCAACAAAAGTGATTGATTGATTGATTGATTGATTGATGGTTTACAGTAG
GACTTCATTCTAGTCATTATAGCTGCTGGCAGTATAACTGGCCAGCCTTT
AATACATTGCTGCTTAGAGTCAAAGCATGTACTTAGAGTTGGTATGATTT
ATCTTTTTGGTCTTCTATAGCCTCCTTCCCCATCCCCATCAGTCTTAATC
AGTCTTGTTACGTTATGACTAATCTTTGGGGATTGTGCAGAATGTTATTT
TAGATAAGCAAAACGAGCAAAATGGGGAGTTACTTATATTTCTTTAAAGC
>G028uaah
CATAAGCTCCTTTTAACTTGTTAAAGTCTTGCTTGAATTAAAGACTTGTT
TAAACACAAAATTTAGACTTTTACTCAACAAAAGTGATTGATTGATTGAT
TGATTGATTGATGGTTTACAGTAGGACTTCATTCTAGTCATTATAGCTGC
TGGCAGTATAACTGGCCAGCCTTTAATACATTGCTGCTTAGAGTCAAAGC
ATGTACTTAGAGTTGGTATGATTTATCTTTTTGGTCTTCTATAGCCTCCT
TCCCCATCCCATCAGTCT
>G022uabh
TATTTTAGAGACCCAAGTTTTTGACCTTTTCCATGTTTACATCAATCCTG
TAGGTGATTGGGCAGCCATTTAAGTATTATTATAGACATTTTCACTATCC
CATTAAAACCCTTTATGCCCATACATCATAACACTACTTCCTACCCATAA
GCTCCTTTTAACTTGTTAAAGTCTTGCTTGAATTAAAGACTTGTTTAAAC
ACAAAATTTAGACTTTTACTCAACAAAAGTGATTGATTGATTGATTGATT
GATTGAT
>G023uabh
AATAAATACCAAAAAAATAGTATATCTACATAGAATTTCACATAAAATAA
ACTGTTTTCTATGTGAAAATTAACCTAAAAATATGCTTTGCTTATGTTTA
AGATGTCATGCTTTTTATCAGTTGAGGAGTTCAGCTTAATAATCCTCTAC
GATCTTAAACAAATAGGAAAAAAACTAAAAGTAGAAAATGGAAATAAAAT
GTCAAAGCATTTCTACCACTCAGAATTGATCTTATAACATGAAATGCTTT
TTAAAAGAAAATATTAAAGTTAAACTCCCCTATTTTGCTCGTTTTTGCTT
ATCTAAAATACATTCTGCACAATCCCCAAAGATTGATCATACGTTAC
>G006uaah
ACATAAAATAAACTGTTTTCTATGTGAAAATTAACCTANNATATGCTTTG
CTTATGTTTAAGATGTCATGCTTTTTATCAGTTGAGGAGTTCAGCTTAAT
AATCCTCTAAGATCTTAAACAAATAGGAAAAAAACTAAAAGTAGAAAATG
GAAATAAAATGTCAAAGCATTTCTACCACTCAGAATTGATCTTATAACAT
GAAATGCTTTTTAAAAGAAAATATTAAAGTTAAACTCCCC
   A string after ">" is the name of the following fragment.
   Only the five upper-case letters A, C, G, T and N are allowed
   to appear in fragment data. No other characters are allowed.
   A common mistake is the use of lower case letters in a fragment.

   To run the program, type a command of form

	cap  file_of_fragments  

   The output goes to the terminal screen. So redirection of the
   output into a file is necessary. The output consists of three parts:
   overview of contigs at fragment level, detailed display of contigs
   at nucleotide level, and consensus sequences.
   '+' = direct orientation; '-' = reverse complement
   The output of CAP on the sample input data looks like:

#Contig 1

#G022uabh+(0)
TATTTTAGAGACCCAAGTTTTTGACCTTTTCCATGTTTACATCAATCCTGTAGGTGATTG
GGCAGCCATTTAAGTATTATTATAGACATTTTCACTATCCCATTAAAACCCTTTATGCCC
ATACATCATAACACTACTTCCTACCCATAAGCTCCTTTTAACTTGTTAAAGTCTTGCTTG
AATTAAAGACTTGTTTAAACACAAAA-TTTAGACTTTTACTCAACAAAAGTGATTGATTG
ATTGATTGATTGATTGAT
#G028uaah+(145)
CATAAGCTCCTTTTAACTTGTTAAAGTCTTGCTTGAATTAAAGACTTGTTTAAACACAAA
A-TTTAGACTTTTACTCAACAAAAGTGATTGATTGATTGATTGATTGATTGATGGTTTAC
AGTAGGACTTCATTCTAGTCATTATAGCTGCTGGCAGTATAACTGGCCAGCCTTTAATAC
ATTGCTGCTTAGAGTCAAAGCATGTACTTAGAGTTGGTATGATTTATCTTTTTGGTCTTC
TATAGCCTCCTTCCCCATCCC-ATCAGTCT
#G019uabh+(120)
ATACATCATAACACTACTTCCTACCCATAAGCTCCTTTTAACTTGTTAAAGTCTTGCTTG
AATTAAAGACTTGTTTAAACACAAAAATTTAGAGTTTTACTCAACAAAAGTGATTGATTG
ATTGATTGATTGATTGATGGTTTACAGTAGGACTTCATTCTAGTCATTATAGCTGCTGGC
AGTATAACTGGCCAGCCTTTAATACATTGCTGCTTAGAGTCAAAGCATGTACTTAGAGTT
GGTATGATTTATCTTTTTGGTCTTCTATAGCCTCCTTCCCCATCCCCATCAGTCTTAATC
AGTCTTGTTACGTTATGACT-AATCTTTGGGGATTGTGCAGAATGTTATTTTAGATAAGC
AAAA-CGAGCAAAAT-GGGGAGTT-A-CTT-A-TATTT-CTTT-AAA--GC
#G023uabh-(426)
GTAACGT-ATGA-TCAATCTTTGGGGATTGTGCAGAATGT-ATTTTAGATAAGCAAAAAC
GAGCAAAATAGGGGAGTTTAACTTTAATATTTTCTTTTAAAAAGCATTTCATGTTATAAG
ATCAATTCTGAGTGGTAGAAATGCTTTGACATTTTATTTCCATTTTCTACTTTTAGTTTT
TTTCCTATTTGTTTAAGATCGTAGAGGATTATTAAGCTGAACTCCTCAACTGATAAAAAG
CATGACATCTTAAACATAAGCAAAGCATATTTTTAGGTTAATTTTCACATAGAAAACAGT
TTATTTTATGTGAAATTCTATGTAGATATACTATTTTTTTGGTATTTATT
#G006uaah-(496)
GGGGAGTTTAACTTTAATATTTTCTTTTAAAAAGCATTTCATGTTATAAGATCAATTCTG
AGTGGTAGAAATGCTTTGACATTTTATTTCCATTTTCTACTTTTAGTTTTTTTCCTATTT
GTTTAAGATCTTAGAGGATTATTAAGCTGAACTCCTCAACTGATAAAAAGCATGACATCT
TAAACATAAGCAAAGCATATNNT-AGGTTAATTTTCACATAGAAAACAGTTTATTTTATG
T



Slight modifications by S. Smith on Mon Feb 17 10:18:34 EST 1992.
These changes allow for command line arguments for several
of the hard coded parameters, as well as a slight modification to
the output routine to support GDE format.  Changes are commented
as:

Mod by S.S.

*/

#include   <stdio.h>

int OVERLEN;        /* Minimum length of any overlap */
float PERCENT;      /* Minimum identity percentage of any overlap */
#define  CUTOFF    50	        /* cutoff score for overlap or containment */
#define  DELTA     9.0	        /* Jump increment in check for overlap */
#define  MATCH     2	        /* score of a match */
#define  MISMAT   -6	        /* score of a mismatch */
#define  LTMISM   -3	        /* light score of a mismatch */
#define  OPEN      0	        /* gap open penalty */
#define  EXTEND    4	        /* gap extension penalty */
#define  LTEXTEN   2	        /* light gap extension penalty */
#define  POS5      30	        /* Sequencing errors often occur before base POS5 */
#define  POS3      475	        /* Sequencing errors often occur after base POS3 */
#define  LINELEN   60		/* length of one printed line */
#define  NAMELEN   20		/* length of printed fragment name */
#define  TUPLELEN  4	        /* Maximum length of words for lookup table */
#define  SEQLEN    2000	        /* initial size of an array for an output fragment */

static int over_len;
static float per_cent;
typedef struct OVERLAP		/* of 5' and 3' segments */
{  
	int    number;	/* index of 3' segment   */
	int    host;		/* index of 5' segment   */
	int    ind;		/* used in reassembly */
	int    stari;	/* start position of 5' suffix */
	int    endi;		/* end position of 5' suffix   */
	int    starj;	/* start position of 3' prefix */
	int    endj;		/* end position of 3' prefix   */
	short   orienti;	/* orientation of 5' segment: 0= rev. */
	short   orientj;	/* orientation of 3' segment: 0= rev. */
	int    score;	/* score of overlap alignment */
	int    length;	/* length of alignment      */
	int    match;	/* number of matches in alignment */
	short   kind;	/* 0 = containment; 1 = overlap   */
	int    *script;	/* script for constructing alignment */
	struct  OVERLAP  *next; 
} over, *overptr;
struct SEG
{  
	char    *name;	/* name string */
	int     len;		/* length of segment name */
	char    *seq;	/* segment sequence */
	char    *rev;	/* reverse complement */
	int     length;	/* length of sequence */
	short   kind;	/* 0 = contain; 1 = overlap */
	int     *lookup;	/* lookup table */
	int     *pos;	/* location list */
	overptr list;	/* list of overlapping edges */
} *segment;	/* array of segment records */
int   seg_num;                  /* The number of segments   */
overptr   *edge;		/* set of overlapping edges */
int   edge_num;                 /* The number of overlaps */
struct CONS			/* 1 = itself; 0 = reverse complement */
{  
	short   isfive[2];	/* is 5' end free? */
	short   isthree[2];	/* is 3' end free? */
	short   orient[2];	/* orientation of 3' segment */
	int     group;	/* contig serial number */
	int     next[2];	/* pointer to 3' adjacent segment */
	int     other;	/* the other end of the contig   */
	int     child;	/* for the containment tree   */
	int     brother;
	int     father;
	overptr node[2];	/* pointers to overlapping edges */
} *contigs;  /* list of contigs */
struct TTREE			/* multiple alignment tree */
{  
	short   head;	/* head = 1 means the head of a contig */
	short   orient;	/* orientation */
	int     begin;	/* start position of previous segment */
	int     *script;	/* alignment script */
	int     size;	/* length of script */
	int     next;	/* list of overlap segments */
	int     child;	/* list of child segments */
	int     brother;	/* list of sibling segments */
}  *mtree;
int   vert[128];		/* converted digits for 'A','C','G','T' */
int   vertc[128];		/* for reverse complement */
int   tuple;			/* tuple = TUPLELEN - 1 */
int  base[TUPLELEN];		/* locations of a lookup table */
int  power[TUPLELEN];		/* powers of 4 */
typedef struct OUT
{ 
	char  *line;		/* one print line */
	char  *a;		/* pointer to slot in line */
	char  c;		/* current char */
	char  *seq;		/* pointer to sequence */
	int   length;		/* length of segment */
	int   id;		/* index of segment */
	int   *s;		/* pointer to script vector */
	int   size;		/* size of script vector */
	int   op;		/* current operation */
	char  name[NAMELEN+2];/* name of segment */
	short done;		/* indicates if segment is done */
	int   loc;		/* position of next segment symbol */
	char  kind;		/* type of next symbol of segment */
	char  up;		/* type of upper symbol of operation */
	char  dw;		/* type of lower symbol of operation */
	int   offset;		/* relative to consensus sequence */
	int   linesize;	/* size of array line */
	struct  OUT *child;	/* pointer to child subtree */
	struct  OUT *brother;	/* pointer to brother node */
	struct  OUT *link;	/* for operation linked list */
	struct  OUT *father;	/* pointer to father node */
}  row, *rowptr;	/* node for segment */
rowptr *work;			/* a set of working segments */
rowptr head, tail;		/* first and last nodes of op list */
struct VX
{  
	int     id;			/* Segment index */
	short   kind;		/* overlap or containment */
	overptr list;		/* list of overlapping edges */
} *piece;		/* array of segment records */
char  *allconsen, *allconpt;		/* Storing consensus sequences */

main(argc, argv) int argc; 
char *argv[];
{ 
	int   M;        			/* Sequence length */
	int  V[128][128], Q,R;		/* Weights  */
	int  V1[128][128], R1;		/* Light weights  */
	int   total;			        /* Total of segment lengths */
	int   number;                         /* The number of segments   */
	char  *sequence;			/* Storing all segments     */
	char  *reverse;			/* Storing all reverse segments */
	int   symbol, prev;			/* The next character         */
	FILE *Ap, *ckopen();                  /* For the sequence file      */
	char *my_calloc(int);			/* space-allocating function  */
	register int  i, j, k;		/* index variables	      */
	/* Mod by S.S. */
	int jj;
	short  heading;			/* 1: heading; 0: sequence    */

	/*
*	Mod by S.S.
*
	if ( argc != 2 )
		fatalf("The proper form of command is: \n%s file_of_fragments",
			 argv[0]);
*/
	if ( argc != 4 )
		fatalf("usage: %s file_of_fragments MIN_OVERLAP PERCENT_MATCH",
		    argv[0]);
	sscanf(argv[2],"%d",&OVERLEN);
	sscanf(argv[3],"%d",&jj);
	PERCENT = (float)jj/100.0;
	if(PERCENT < 0.25) PERCENT = 0.25;
	if(PERCENT > 1.0) PERCENT = 1.0;
	if(OVERLEN < 1) OVERLEN = 1;
	if(OVERLEN > 100) OVERLEN = 100;



	/* determine number of segments and total lengths */
	j = 0;

	Ap = ckopen(argv[1], "r");
	prev = '\n';
	for (total = 3, number = 0; ( symbol = getc(Ap)) != EOF ; total++ )
	{ 
		if ( symbol == '>' && prev == '\n' )
			number++;
		prev = symbol;
	}
	(void)	fclose(Ap);

	total += number * 20;
	/* allocate space for segments */
	sequence = ( char * ) my_calloc( total * sizeof(char));
	reverse = ( char * ) my_calloc( total * sizeof(char));
	allconpt = allconsen = ( char * ) my_calloc( total * sizeof(char));
	segment = ( struct SEG * ) my_calloc( number * sizeof(struct SEG));

	/* read the segments into sequence */
	M = 0;
	Ap = ckopen(argv[1], "r");
	number = -1;
	heading = 0;
	prev = '\n';
	for ( i = 0, k = total ; ( symbol = getc(Ap)) != EOF ; )
	{ 
		if ( symbol != '\n' )
		{ 
			sequence[++i] = symbol;
			switch ( symbol )
			{ 
			case 'A' : 
				reverse[--k] = 'T'; 
				break;
			case 'a' : 
				reverse[--k] = 't'; 
				break;
			case 'T' : 
				reverse[--k] = 'A'; 
				break;
			case 't' : 
				reverse[--k] = 'a'; 
				break;
			case 'C' : 
				reverse[--k] = 'G'; 
				break;
			case 'c' : 
				reverse[--k] = 'g'; 
				break;
			case 'G' : 
				reverse[--k] = 'C'; 
				break;
			case 'g' : 
				reverse[--k] = 'c'; 
				break;
			default  : 
				reverse[--k] = symbol; 
				break;
			}
		}
		if ( symbol == '>' && prev == '\n' )
		{ 
			heading = 1;
			if ( number >= 0 )
			{ 
				segment[number].length = i - j - 1;
				segment[number].rev = &(reverse[k]);
				if ( i - j - 1 > M ) M = i - j -1;
			}
			number++;
			j = i;
			segment[number].name = &(sequence[i]);
			segment[number].kind = 1;
			segment[number].list = 0;
		}
		if ( heading && symbol == '\n' )
		{ 
			heading = 0;
			segment[number].len = i - j;
			segment[number].seq = &(sequence[i]);
			j = i;
		}

		prev = symbol;
	}
	segment[number].length = i - j;
	reverse[--k] = '>';
	segment[number].rev = &(reverse[k]);
	if ( i - j > M ) M = i - j;
	seg_num = ++number;
	(void)	fclose(Ap);

	Q = OPEN;
	R = EXTEND;
	R1 = LTEXTEN;
	/* set match and mismatch weights */
	for ( i = 0; i < 128 ; i++ )
		for ( j = 0; j < 128 ; j++ )
			if ((i|32) == (j|32) )
				V[i][j] = V1[i][j] = MATCH;
			else
			{ 
				V[i][j] = MISMAT;
				V1[i][j] = LTMISM;
			}
	for ( i = 0; i < 128 ; i++ )
		V['N'][i] = V[i]['N'] = MISMAT + 1;
	V1['N']['N'] = MISMAT + 1;

	over_len = OVERLEN;
	per_cent = PERCENT;
	edge_num = 0;
	INIT(M);
	MAKE();
	PAIR(V,V1,Q,R,R1);
	ASSEM();
	REPAIR();
	FORM_TREE();
	/* GRAPH(); */
	SHOW();
}

static int  (*v)[128];			/* substitution scores */
static int  q, r;			/* gap penalties */
static int  qr;				/* qr = q + r */
static int  (*v1)[128];			/* light substitution scores */
static int  r1;				/* light extension penalty */
static int  qr1;			/* qr1 = q + r1 */

static int   SCORE;
static int   STARI;
static int   STARJ;
static int   ENDI;
static int   ENDJ;

static int  *CC, *DD;			/* saving matrix scores */
static int  *RR, *SS;		 	/* saving start-points */
static int   *S;			/* saving operations for diff */

/* The following definitions are for function diff() */

int   diff();
static int   zero = 0;				/* int  type zero        */

#define gap(k)  ((k) <= 0 ? 0 : q+r*(k))	/* k-symbol indel score */

static int  *sapp;				/* Current script append ptr */
static int   last;				/* Last script op appended */

static int  no_mat; 				/* number of matches */

static int  no_mis; 				/* number of mismatches */

static int  al_len; 				/* length of alignment */
/* Append "Delete k" op */
#define DEL(k)				\
{ al_len += k;				\
  if (last < 0)				\
    last = sapp[-1] -= (k);		\
  else					\
    last = *sapp++ = -(k);		\
}
/* Append "Insert k" op */
#define INS(k)				\
{ al_len += k;				\
  if (last < 0)				\
    { sapp[-1] = (k); *sapp++ = last; }	\
  else					\
    last = *sapp++ = (k);		\
}

/* Append "Replace" op */
#define REP 				\
{ last = *sapp++ = 0; 			\
  al_len += 1;				\
}

INIT(M) int  M;
{ 
	register  int   j;			/* row and column indices */
	char *my_calloc();			/* space-allocating function */

	/* allocate space for all vectors */
	j = (M + 1) * sizeof(int );
	CC = ( int  * ) my_calloc(j);
	DD = ( int  * ) my_calloc(j);
	RR = ( int  * ) my_calloc(j);
	SS = ( int  * ) my_calloc(j);
	S = ( int  * ) my_calloc(2 * j);
}

/* Make a lookup table for words of lengths up to TUPLELEN in each sequence.
   The value of a word is used as an index to the table.  */
MAKE()
{ 
	int   hash[TUPLELEN];		/* values of words of lengths up to TUPLELEN */
	int   *table;			/* pointer to a lookup table */
	int   *loc;			/* pointer to a table of sequence locations */
	int   size;			/* size of a lookup table */
	int   limit, digit, step;	/* temporary variables  */
	register  int  i, j, k, p, q;	/* index varibles */
	char *my_calloc();		/* space-allocating function */
	char *A;			/* pointer to a sequence */
	int  M;			/* length of a sequence */

	tuple = TUPLELEN - 1;
	for ( i = 0; i < 128; i++ )
		vert[i] = 4;
	vert['A'] = vert['a'] = 0;
	vert['C'] = vert['c'] = 1;
	vert['G'] = vert['g'] = 2;
	vert['T'] = vert['t'] = 3;
	vertc['A'] = vertc['a'] = 3;
	vertc['C'] = vertc['c'] = 2;
	vertc['G'] = vertc['g'] = 1;
	vertc['T'] = vertc['t'] = 0;
	for ( i = 0, j = 1, size = 0; i <= tuple ; i++, j *= 4 )
	{  
		base[i] = size;
		power[i] = j;
		size = ( size + 1 ) * 4;
	}
	for ( j = 0; j <= tuple; j++ )
		hash[j] = 0;
	/* make a lookup table for each sequence */
	for ( i = 0; i < seg_num ; i++ )
	{ 
		A = segment[i].seq;
		M = segment[i].length;
		table = segment[i].lookup = (int  * ) my_calloc(size * sizeof(int ));
		loc = segment[i].pos = (int  * ) my_calloc((M + 1) * sizeof(int ));
		for ( j = 0; j < size; j++ )
			table[j] = 0;
		for ( k = 0, j = 1; j <= M; j++ )
			if ( ( digit = vert[A[j]] ) != 4 )
			{ 
				for ( p = tuple; p > 0; p-- )
					hash[p] = 4 * (hash[p-1] + 1) + digit;
				hash[0] = digit;
				step = j - k;
				limit = tuple <= step ? tuple : step;
				for ( p = 0; p < limit; p++ )
					if ( ! table[(q = hash[p])] )  table[q] = 1;
				if ( step > tuple )
				{ 
					loc[(p = j - tuple)] = table[(q = hash[tuple])];
					table[q] = p;
				}
			}
			else
				k = j;
	}
}

/* Perform pair-wise comparisons of sequences. The sequences not
   satisfying the necessary condition for overlap are rejected quickly.
   Those that do satisfy the condition are verified with a dynamic
   programming algorithm to see if overlaps exist. */
PAIR(V,V1,Q,R,R1) int  V[][128],V1[][128],Q,R,R1;
{ 
	int  endi, endj, stari, starj;	/* endpoint and startpoint */

	short orienti, orientj;		/* orientation of segments */
	short iscon;				/* containment condition   */
	int   score;   			/* the max score */
	int  count, limit;			/* temporary variables     */

	register  int   i, j, d;		/* row and column indices  */
	char *my_calloc();			/* space-allocating function */
	int  rl, cl;
	char *A, *B;
	int  M, N;
	overptr node1;
	int  total;				/* total number of pairs */
	int  hit;				/* number of pairs satisfying cond. */
	int  CHECK();

	v = V;
	q = Q;
	r = R;
	qr = q + r;
	v1 = V1;
	r1 = R1;
	qr1 = q + r1;
	total = hit = 0;
	limit = 2 * ( seg_num - 1 );
	for ( orienti = 0, d = 0; d < limit ; d++ )
	{ 
		i = d / 2;
		orienti = 1 - orienti;
		A = orienti ? segment[i].seq : segment[i].rev;
		M = segment[i].length;
		for ( j = i+1; j < seg_num ; j++ )
		{ 
			B = segment[j].seq;
			orientj = 1;
			N = segment[j].length;
			total += 1;
			if ( CHECK(orienti, i, j) )
			{ 
				hit += 1;
				SCORE = 0;
				big_pass(A,B,M,N,orienti,orientj);
				if ( SCORE )
				{ 
					score = SCORE;
					stari = ++STARI;
					starj = ++STARJ;
					endi = ENDI;
					endj = ENDJ;
					rl = endi - stari + 1;
					cl = endj - starj + 1;
					sapp = S;
					last = 0;
					al_len = no_mat = no_mis = 0;
					(void) diff(&A[stari]-1, &B[starj]-1,rl,cl,q,q);
					iscon = stari == 1 && endi == M || starj == 1 && endj == N;
					if ( no_mat >= al_len * per_cent &&
					    ( al_len >= over_len || iscon ) )
					{ 
						node1 = ( overptr ) my_calloc( (int ) sizeof(over));
						if ( iscon )
							node1->kind = 0;		/* containment */
						else
						{ 
							node1->kind = 1; 
							edge_num++; 
						}	/* overlap */
						if ( endi == M && ( endj != N || starj != 1 ) ) /*i is 5'*/
						{ 
							node1->number = j;
							node1->host = i;
							node1->stari = stari;
							node1->endi = endi;
							node1->orienti = orienti;
							node1->starj = starj;
							node1->endj = endj;
							node1->orientj = orientj;
						}
						else	/* j is 5' */
						{ 
							node1->number = i;
							node1->host = j;
							node1->stari = starj;
							node1->endi = endj;
							node1->orienti = orientj;
							node1->starj = stari;
							node1->endj = endi;
							node1->orientj = orienti;
						}
						node1->score = score;
						node1->length = al_len;
						node1->match = no_mat;
						count = node1->number == i ? j : i;
						node1->next = segment[count].list;
						segment[count].list = node1;
						if ( ! node1->kind )
							segment[count].kind = 0;
					}
				}
			}
		}
	}
}

/* Return 1 if two sequences satisfy the necessary condition for overlap,
   and 0 otherwise. Parameters first and second are the indices of segments,
   and parameter orient indicates the orientation of segment first.  */
int  CHECK(orient,first,second) short orient; 
int  first, second;
{ 
	int   limit, bound;		/* maximum number of jumps */
	int   small;			/* smaller of limit and bound */
	float  delta;			/* cutoff factor   */
	float cut;			/* cutoff score */
	register  int   i;		/* index variable */
	int  t, q;			/* temporary variables */
	int  JUMP();
	int  RJUMP();
	int  JUMPC();
	int  RJUMPC();

	delta = DELTA;
	if ( orient )
		limit = JUMP(CC, first, second, 1);
	else
		limit = JUMPC(CC, first, second);
	bound = RJUMP(DD, second, first, orient);
	small = limit <= bound ? limit : bound;
	cut = 0;
	for ( i = 1; i <= small; i++ )
	{ 
		if ( (t = DD[i] - 1) >= over_len && t >= cut
		    && (q = CC[i] - 1) >= over_len && q >= cut )
			return (1);
		cut += delta;
	}
	if (DD[bound] >= delta*(bound-1) || CC[limit] >= delta*(limit-1))
		return (1);
	limit = JUMP(CC, second, first, orient);
	if ( orient )
		bound = RJUMP(DD, first, second, 1);
	else
		bound = RJUMPC(DD, first, second);
	small = limit <= bound ? limit : bound;
	cut = 0;
	for ( i = 1; i <= small; i++ )
	{ 
		if ( (t = DD[i] - 1) >= over_len && t >= cut
		    && (q = CC[i] - 1) >= over_len && q >= cut )
			return (1);
		cut += delta;
	}
	return (0);
}

/* Compute a vector of lengths of jumps */
int  JUMP(H,one,two,orient) int  H[], one, two; 
short orient;
{ 
	char *A, *B;			/* pointers to two sequences */
	int  M, N;			/* lengths of two sequences */
	int  *table;			/* pointer to a lookup table */
	int  *loc;			/* pointer to a location table */
	int  value;			/* value of a word */
	int  maxd;			/* maximum length of an identical diagonal */
	int  d;			/* length of current identical diagonal */
	int  s;			/* length of jumps */
	int  k;			/* number of jumps */

	register int  i, j, p;	/* index variables */

	A = segment[one].seq;
	M = segment[one].length;
	table = segment[one].lookup;
	loc = segment[one].pos;
	B = orient ? segment[two].seq : segment[two].rev;
	N = segment[two].length;
	for ( s = 1, k = 1; s <= N ; k++ )
	{ 
		maxd = 0;
		for ( value = -1, d = 0, j = s; d <= tuple && j <= N; j++, d++)
		{ 
			if ( vert[B[j]] == 4 ) break;
			value = 4 * (value + 1) + vert[B[j]];
			if ( ! table[value] ) break;
		}
		if ( d > tuple )
		{ 
			for ( p = table[value]; p ; p = loc[p] )
			{ 
				d = tuple + 1;
				for ( i = p+d, j = s+d; i <= M && j <= N; i++, j++, d++ )
					if ( A[i] != B[j] && vert[A[i]] != 4 && vert[B[j]] != 4 ) break;
				if ( maxd < d )
					maxd = d;
				if ( j > N ) break;
			}
		}
		else 
			maxd = d;
		s += maxd + 1;
		H[k] = s;
	}
	return (k - 1);
}

/* Compute a vector of lengths of jumps for reverse complement of one */
int  JUMPC(H,one,two) int  H[], one, two;
{ 
	char *A, *B;			/* pointers to two sequences */
	int  M, N;			/* lengths of two sequences */
	int  *table;			/* pointer to a lookup table */
	int  *loc;		/* pointer to a location table */
	int  value;			/* value of a word */
	int  maxd;			/* maximum length of an identical diagonal */
	int  d;			/* length of current identical diagonal */
	int  s;			/* length of jumps */
	int  k;			/* number of jumps */

	register  int   i, j, p;	/* index variables */

	A = segment[one].rev;
	M = segment[one].length;
	table = segment[one].lookup;
	loc = segment[one].pos;
	B = segment[two].seq;
	N = segment[two].length;
	for ( s = 1, k = 1; s <= N ; k++ )
	{ 
		maxd = 0;
		for ( value = 0, d = 0, j = s; d <= tuple && j <= N; j++, d++)
		{ 
			if ( vert[B[j]] == 4 ) break;
			value += vertc[B[j]] * power[d];
			if ( ! table[value+base[d]] ) break;
		}
		if ( d > tuple )
		{ 
			for ( p = table[value+base[tuple]]; p ; p = loc[p] )
			{ 
				d = tuple + 1;
				for ( i = M+2-p, j = s+d; i <= M && j <= N; i++, j++, d++ )
					if ( A[i] != B[j] && vert[A[i]] != 4 && vert[B[j]] != 4 ) break;
				if ( maxd < d )
					maxd = d;
				if ( j > N ) break;
			}
		}
		else 
			maxd = d;
		s += maxd + 1;
		H[k] = s;
	}
	return (k - 1);
}

/* Compute a vector of lengths of reverse jumps */
int  RJUMP(H,one,two,orient) int  H[], one, two; 
short orient;
{ 
	char *A, *B;			/* pointers to two sequences */
	int  N;			/* length of a sequence */
	int  *table;			/* pointer to a lookup table */
	int  *loc;		/* pointer to a location table */
	int  value;			/* value of a word */
	int  maxd;			/* maximum length of an identical diagonal */
	int  d;			/* length of current identical diagonal */
	int  s;			/* length of jumps */
	int  k;			/* number of jumps */

	register  int   i, j, p;	/* index variables */

	A = segment[one].seq;
	table = segment[one].lookup;
	loc = segment[one].pos;
	B = orient ? segment[two].seq : segment[two].rev;
	N = segment[two].length;
	for ( s = 1, k = 1; s <= N ; k++ )
	{ 
		maxd = 0;
		for (value = 0, d = 0, j = N+1-s; d <= tuple && j >= 1; j--, d++)
		{ 
			if ( vert[B[j]] == 4 ) break;
			value += vert[B[j]] * power[d];
			if ( ! table[value+base[d]] ) break;
		}
		if ( d > tuple )
		{ 
			for ( p = table[value+base[tuple]]; p ; p = loc[p] )
			{ 
				d = tuple + 1;
				for ( i = p-1, j = N+1-s-d; i >= 1 && j >= 1; i--, j--, d++ )
					if ( A[i] != B[j] && vert[A[i]] != 4 && vert[B[j]] != 4 ) break;
				if ( maxd < d )
					maxd = d;
				if ( j < 1 ) break;
			}
		}
		else 
			maxd = d;
		s += maxd + 1;
		H[k] = s;
	}
	return (k - 1);
}

/* Compute a vector of lengths of reverse jumps for reverse complement */
int  RJUMPC(H,one,two) int  H[], one, two;
{ 
	char *A, *B;			/* pointers to two sequences */
	int  M, N;			/* lengths of two sequences */
	int  *table;			/* pointer to a lookup table */
	int  *loc;		/* pointer to a location table */
	int  value;			/* value of a word */
	int  maxd;			/* maximum length of an identical diagonal */
	int  d;			/* length of current identical diagonal */
	int  s;			/* length of jumps */
	int  k;			/* number of jumps */

	register  int   i, j, p;	/* index variables */

	A = segment[one].rev;
	M = segment[one].length;
	table = segment[one].lookup;
	loc = segment[one].pos;
	B = segment[two].seq;
	N = segment[two].length;
	for ( s = 1, k = 1; s <= N ; k++ )
	{ 
		maxd = 0;
		for (value = -1, d = 0, j = N+1-s; d <= tuple && j >= 1; j--, d++)
		{ 
			if ( vert[B[j]] == 4 ) break;
			value = 4 * (value + 1) + vertc[B[j]];
			if ( ! table[value] ) break;
		}
		if ( d > tuple )
		{ 
			for ( p = table[value]; p ; p = loc[p] )
			{ 
				d = tuple + 1;
				i = M - p - tuple;
				for ( j = N-s-tuple; i >= 1 && j >= 1; i--, j--, d++ )
					if ( A[i] != B[j] && vert[A[i]] != 4 && vert[B[j]] != 4 ) break;
				if ( maxd < d )
					maxd = d;
				if ( j < 1 ) break;
			}
		}
		else 
			maxd = d;
		s += maxd + 1;
		H[k] = s;
	}
	return (k - 1);
}

/* Construct contigs */
ASSEM()
{ 
	char *my_calloc();		/* space-allocating function */
	register  int   i, j, k;	/* index variables */
	overptr  node1, x, y;		/* temporary pointer */
	int   five, three;		/* indices of 5' and 3' segments */
	short   orienti;		/* orientation of 5' segment */
	short   orientj;		/* orientation of 3' segment */
	short   sorted;		/* boolean variable */

	contigs = ( struct CONS * ) my_calloc( seg_num * sizeof(struct CONS));
	for ( i = 0; i < seg_num; i++ )
	{ 
		contigs[i].isfive[0] = contigs[i].isthree[0] = 1;
		contigs[i].isfive[1] = contigs[i].isthree[1] = 1;
		contigs[i].other = i;
		contigs[i].group = contigs[i].child = -1;
		contigs[i].brother = contigs[i].father = -1;
	}
	for ( i = 0; i < seg_num; i++ )
		if ( ! segment[i].kind )
			for ( ; ; )
			{ 
				for ( y = segment[i].list; y->kind; y = y->next )
					;
				for ( x = y->next; x != 0; x = x->next )
					if ( ! x->kind && x->score > y->score )
						y = x;
				for ( j = y->number; (k = contigs[j].father) != -1; j = k )
					;
				if ( j != i )
				{ 
					contigs[i].father = j = y->number;
					contigs[i].brother = contigs[j].child;
					contigs[j].child = i;
					contigs[i].node[1] = y;
					break;
				}
				else
				{ 
					if ( segment[i].list->number == y->number )
						segment[i].list = y->next;
					else
					{ 
						for ( x = segment[i].list; x->next->number != y->number ; )
							x = x->next;
						x->next = y->next;
					}
					for ( x = segment[i].list; x != 0 && x->kind; x = x->next )
						;
					if ( x == 0 )
					{ 
						segment[i].kind = 1;
						break;
					}
				}
			}
	edge = ( overptr * ) my_calloc( edge_num * sizeof(overptr) );
	for ( j = 0, i = 0; i < seg_num; i++ )
		if ( segment[i].kind )
			for ( node1 = segment[i].list; node1 != 0; node1 = node1->next )
				if ( segment[node1->number].kind )
					edge[j++] = node1;
	edge_num = j;
	for ( i = edge_num - 1; i > 0; i-- )
	{ 
		sorted = 1;
		for ( j = 0; j < i; j++ )
			if ( edge[j]->score < edge[j+1]->score )
			{ 
				node1 = edge[j];
				edge[j] = edge[j+1];
				edge[j+1] = node1;
				sorted = 0;
			}
		if ( sorted )
			break;
	}
	for ( k = 0; k < edge_num; k++ )
	{ 
		five = edge[k]->host;
		three = edge[k]->number;
		orienti = edge[k]->orienti;
		orientj = edge[k]->orientj;
		if ( contigs[five].isthree[orienti] &&
		    contigs[three].isfive[orientj] && contigs[five].other != three )
		{ 
			contigs[five].isthree[orienti] = 0;
			contigs[three].isfive[orientj] = 0;
			contigs[five].next[orienti] = three;
			contigs[five].orient[orienti] = orientj;
			contigs[five].node[orienti] = edge[k];
			contigs[three].isthree[(j = 1 - orientj)] = 0;
			contigs[five].isfive[(i = 1 - orienti)] = 0;
			contigs[three].next[j] = five;
			contigs[three].orient[j] = i;
			contigs[three].node[j] = edge[k];
			i = contigs[three].other;
			j = contigs[five].other;
			contigs[i].other = j;
			contigs[j].other = i;
		}
	}
}

REPAIR()
{ 
	int  endi, endj, stari, starj;	/* endpoint and startpoint */

	short orienti, orientj;		/* orientation of segments */
	short isconi, isconj;			/* containment condition   */
	int   score;   			/* the max score */
	int   i, j, f, d, e;			/* row and column indices  */
	char *my_calloc();			/* space-allocating function */
	char *A, *B;
	int  M, N;
	overptr node1;
	int   piece_num;       	        /* The number of pieces */
	int   count, limit;
	int   number;
	int   hit;

	piece = ( struct VX * ) my_calloc( seg_num * sizeof(struct VX));
	for ( j = 0, i = 0; i < seg_num; i++ )
		if (segment[i].kind && (contigs[i].isfive[1] || contigs[i].isfive[0]))
			piece[j++].id = i;
	piece_num = j;
	for ( i = 0; i < piece_num; i++ )
	{ 
		piece[i].kind = 1;
		piece[i].list = 0;
	}
	limit = 2 * ( piece_num - 1 );
	hit = number = 0;
	for ( orienti = 0, d = 0; d < limit ; d++ )
	{ 
		i = piece[(e = d / 2)].id;
		orienti = 1 - orienti;
		A = orienti ? segment[i].seq : segment[i].rev;
		M = segment[i].length;
		for ( f = e+1; f < piece_num ; f++ )
		{ 
			j = piece[f].id;
			B = segment[j].seq;
			orientj = 1;
			N = segment[j].length;
			SCORE = 0;
			hit++;
			big_pass(A,B,M,N,orienti,orientj);
			if ( SCORE > CUTOFF )
			{ 
				score = SCORE;
				stari = ++STARI;
				starj = ++STARJ;
				endi = ENDI;
				endj = ENDJ;
				isconi = stari == 1 && endi == M;
				isconj = starj == 1 && endj == N;
				node1 = ( overptr ) my_calloc( (int ) sizeof(over));
				if ( isconi || isconj )
					node1->kind = 0;		/* containment */
				else
				{ 
					node1->kind = 1; 
					number++; 
				}	/* overlap */
				if ( endi == M && ! isconj )	 /*i is 5'*/
				{ 
					node1->number = j;
					node1->host = i;
					node1->ind = f;
					node1->stari = stari;
					node1->endi = endi;
					node1->orienti = orienti;
					node1->starj = starj;
					node1->endj = endj;
					node1->orientj = orientj;
				}
				else	/* j is 5' */
				{ 
					node1->number = i;
					node1->host = j;
					node1->ind = e;
					node1->stari = starj;
					node1->endi = endj;
					node1->orienti = orientj;
					node1->starj = stari;
					node1->endj = endi;
					node1->orientj = orienti;
				}
				node1->score = score;
				count = node1->number == i ? f : e;
				node1->next = piece[count].list;
				piece[count].list = node1;
				if ( ! node1->kind )
					piece[count].kind = 0;
			}
		}
	}
	REASSEM(piece_num, number);
}

/* Construct contigs */
REASSEM(piece_num, number) int  piece_num, number;
{ 
	char *my_calloc();		/* space-allocating function */
	int   i, j, k, d;		/* index variables */
	overptr  node1, x, y;		/* temporary pointer */
	int   five, three;		/* indices of 5' and 3' segments */
	short   orienti;		/* orientation of 5' segment */
	short   orientj;		/* orientation of 3' segment */
	short   sorted;		/* boolean variable */

	for ( d = 0; d < piece_num; d++ )
		if ( ! piece[d].kind )
			for ( i = piece[d].id ; ; )
			{ 
				for ( y = piece[d].list; y->kind; y = y->next )
					;
				for ( x = y->next; x != 0; x = x->next )
					if ( ! x->kind && x->score > y->score )
						y = x;
				for ( j = y->number; (k = contigs[j].father) != -1; j = k )
					;
				if ( j != i && RECONCILE(y,&piece_num,&number) )
				{ 
					contigs[i].father = j = y->number;
					contigs[i].brother = contigs[j].child;
					contigs[j].child = i;
					contigs[i].node[1] = y;
					segment[i].kind = 0;
					break;
				}
				else
				{ 
					if ( piece[d].list->number == y->number )
						piece[d].list = y->next;
					else
					{ 
						for ( x = piece[d].list; x->next->number != y->number ; )
							x = x->next;
						x->next = y->next;
					}
					for ( x = piece[d].list; x != 0 && x->kind; x = x->next )
						;
					if ( x == 0 )
					{ 
						piece[d].kind = 1;
						break;
					}
				}
			}
	if ( number > edge_num )
		edge = ( overptr * ) my_calloc( number * sizeof(overptr) );
	for ( j = 0, d = 0; d < piece_num; d++ )
		if ( piece[d].kind )
			for ( node1 = piece[d].list; node1 != 0; node1 = node1->next )
				if ( piece[node1->ind].kind )
					edge[j++] = node1;
	edge_num = j;
	for ( i = edge_num - 1; i > 0; i-- )
	{ 
		sorted = 1;
		for ( j = 0; j < i; j++ )
			if ( edge[j]->score < edge[j+1]->score )
			{ 
				node1 = edge[j];
				edge[j] = edge[j+1];
				edge[j+1] = node1;
				sorted = 0;
			}
		if ( sorted )
			break;
	}
	for ( k = 0; k < edge_num; k++ )
	{ 
		five = edge[k]->host;
		three = edge[k]->number;
		orienti = edge[k]->orienti;
		orientj = edge[k]->orientj;
		if ( contigs[five].isthree[orienti] &&
		    contigs[three].isfive[orientj] && contigs[five].other != three )
		{ 
			contigs[five].isthree[orienti] = 0;
			contigs[three].isfive[orientj] = 0;
			contigs[five].next[orienti] = three;
			contigs[five].orient[orienti] = orientj;
			contigs[five].node[orienti] = edge[k];
			contigs[three].isthree[(j = 1 - orientj)] = 0;
			contigs[five].isfive[(i = 1 - orienti)] = 0;
			contigs[three].next[j] = five;
			contigs[three].orient[j] = i;
			contigs[three].node[j] = edge[k];
			i = contigs[three].other;
			j = contigs[five].other;
			contigs[i].other = j;
			contigs[j].other = i;
		}
	}
}

RECONCILE(y, pp,nn) overptr y; 
int  *pp,*nn;
{ 
	short orienti, orientj;		/* orientation of segments */
	short orientk, orientd;		/* orientation of segments */
	int   i, j, k, d, f;			/* row and column indices  */
	char *my_calloc();			/* space-allocating function */
	char *A, *B;
	int  M, N;
	overptr node1;

	k = y->host;
	d = y->number;
	orientk = y->orienti;
	orientd = y->orientj;
	if ( ! contigs[k].isthree[orientk] )
	{ 
		if ( ! piece[y->ind].kind ) return (0);
		if ( contigs[d].isthree[orientd] )
		{ 
			orienti = orientd;
			i = d;
			orientj = contigs[k].orient[orientk];
			j = contigs[k].next[orientk];
		}
		else
			return (0);
	}
	else
		if ( ! contigs[k].isfive[orientk] )
		{ 
			if ( ! piece[y->ind].kind ) return (0);
			if ( contigs[d].isfive[orientd] )
			{ 
				orienti = contigs[k].orient[1-orientk];
				orienti = 1 - orienti;
				i = contigs[k].next[1-orientk];
				orientj = orientd;
				j = d;
			}
			else
				return (0);
		}
		else
			return (0);
	A = orienti ? segment[i].seq : segment[i].rev;
	M = segment[i].length;
	B = orientj ? segment[j].seq : segment[j].rev;
	N = segment[j].length;
	SCORE = 0;
	big_pass(A,B,M,N,orienti,orientj);
	if ( SCORE > CUTOFF && ENDI - STARI > over_len && ENDI == M && STARJ == 0 )
	{ 
		node1 = ( overptr ) my_calloc( (int ) sizeof(over));
		node1->kind = 1;
		node1->host = i;
		node1->number = j;
		node1->stari = ++STARI;
		node1->endi = ENDI;
		node1->orienti = orienti;
		node1->starj = ++STARJ;
		node1->endj = ENDJ;
		node1->orientj = orientj;
		node1->score = SCORE;
		piece[*pp].kind = 1;
		if ( i == d )
		{ 
			node1->ind = *pp;
			node1->next = piece[y->ind].list;
			piece[y->ind].list = node1;
			piece[*pp].id = j;
			piece[*pp].list = 0;
		}
		else
		{ 
			node1->ind = y->ind;
			piece[*pp].list = node1;
			node1->next = 0;
			piece[*pp].id = i;
		}
		(*nn)++;
		(*pp)++;
		f = contigs[k].other;
		if ( ! contigs[k].isthree[orientk] )
		{ 
			contigs[j].isfive[orientj] = 1;
			contigs[j].isthree[1 - orientj] = 1;
			contigs[k].isthree[orientk] = 1;
			contigs[k].isfive[1 - orientk] = 1;
			contigs[f].other = j;
			contigs[j].other = f;
		}
		else
		{ 
			contigs[i].isthree[orienti] = 1;
			contigs[i].isfive[1 - orienti] = 1;
			contigs[k].isfive[orientk] = 1;
			contigs[k].isthree[1 - orientk] = 1;
			contigs[f].other = i;
			contigs[i].other = f;
		}
		contigs[k].other = k;
		return (1);
	}
	return (0);
}

/* Construct a tree of overlapping-containment segments */
FORM_TREE()
{ 
	register  int   i, j, k;	/* index variables */
	char *my_calloc();		/* space-allocating function */
	overptr  node1;		/* temporary pointer */
	short   orient;		/* orientation of segment */
	int   group;			/* serial number of contigs  */
	char *A, *B;			/* pointers to segment sequences */
	int  stari, endi, starj, endj;/* positions where alignment begins */
	int  M, N;			/* lengths of segment sequences */
	int   count;			/* temporary variables */

	mtree = ( struct TTREE * ) my_calloc( seg_num * sizeof(struct TTREE));
	for ( i = 0; i < seg_num; i++ )
	{ 
		mtree[i].head = 0;
		mtree[i].next = mtree[i].child = mtree[i].brother = -1;
	}
	for ( group = 0, i = 0; i < seg_num; i++ )
		if ( segment[i].kind && contigs[i].group < 0 &&
		    ( contigs[i].isfive[1] || contigs[i].isfive[0] ) )
		{ 
			orient = contigs[i].isfive[1] ? 1 : 0;
			mtree[i].head = 1;
			for ( j = i; ;  )
			{ 
				contigs[j].group = group;
				mtree[j].orient = orient;
				SORT(j, orient);
				if ( contigs[j].isthree[orient] )
					break;
				else
				{ 
					k = contigs[j].next[orient];
					node1 = contigs[j].node[orient];
					if ( j == node1->host )
					{ 
						stari = node1->stari;
						endi  = node1->endi;
						starj = node1->starj;
						endj  = node1->endj;
						A = node1->orienti ? segment[j].seq : segment[j].rev;
						B = node1->orientj ? segment[k].seq : segment[k].rev;
					}
					else
					{ 
						M = segment[j].length;
						stari = M + 1 - node1->endj;
						endi = M + 1 - node1->starj;
						N = segment[k].length;
						starj = N + 1 - node1->endi;
						endj = N + 1 - node1->stari;
						A = node1->orientj ? segment[j].rev : segment[j].seq;
						B = node1->orienti ? segment[k].rev : segment[k].seq;
					}
					M = endi - stari + 1;
					N = endj - starj + 1;
					sapp = S;
					last = 0;
					al_len = no_mat = no_mis = 0;
					(void) diff(&A[stari]-1, &B[starj]-1,M,N,q,q);
					count = ( (N = sapp - S) + 1 ) * sizeof(int );
					mtree[k].script = ( int  * ) my_calloc( count );
					for ( M = 0; M < N; M++)
						mtree[k].script[M] = S[M];
					mtree[k].size = N;
					mtree[k].begin = stari;
					mtree[j].next = k;
					orient = contigs[j].orient[orient];
					j = k;
				}
			}
			group++;
		}
}

/* Sort the children of each node by the `begin' field */
SORT(seg, ort) int  seg; 
short ort;
{ 
	register  int   i, j, k;	/* index variables */
	char *my_calloc();		/* space-allocating function */
	overptr  node1;		/* temporary pointer */
	short   orient;		/* orientation of segment */
	char *A, *B;			/* pointers to segment sequences */
	int  stari, endi, starj, endj;/* positions where alignment begins */
	int  M, N;			/* lengths of segment sequences */
	int   count;			/* temporary variables */

	for ( j = contigs[seg].child; j != -1; j = contigs[j].brother )
	{ 
		node1 = contigs[j].node[1];
		if ( ort == node1->orientj )
		{ 
			stari = node1->starj;
			endi  = node1->endj;
			starj = node1->stari;
			endj  = node1->endi;
			A = node1->orientj ? segment[seg].seq : segment[seg].rev;
			B = node1->orienti ? segment[j].seq : segment[j].rev;
			orient = node1->orienti;
		}
		else
		{ 
			M = segment[seg].length;
			stari = M + 1 - node1->endj;
			endi = M + 1 - node1->starj;
			N = segment[j].length;
			starj = N + 1 - node1->endi;
			endj = N + 1 - node1->stari;
			A = node1->orientj ? segment[seg].rev : segment[seg].seq;
			B = node1->orienti ? segment[j].rev : segment[j].seq;
			orient = 1 -  node1->orienti;
		}
		M = endi - stari + 1;
		N = endj - starj + 1;
		sapp = S;
		last = 0;
		al_len = no_mat = no_mis = 0;
		(void) diff(&A[stari]-1, &B[starj]-1,M,N,q,q);
		count = ( (M = sapp - S ) + 1 ) * sizeof(int );
		mtree[j].script = ( int  * ) my_calloc( count );
		for ( k = 0; k < M; k++)
			mtree[j].script[k] = S[k];
		mtree[j].size = M;
		mtree[j].begin = stari;
		mtree[j].orient = orient;
		if ( mtree[seg].child == -1 )
			mtree[seg].child = j;
		else
		{ 
			i = mtree[seg].child;
			if ( mtree[i].begin >= stari )
			{ 
				mtree[j].brother = i;
				mtree[seg].child = j;
			}
			else
			{ 
				M = mtree[i].brother;
				for ( ; M != -1; i = M, M = mtree[M].brother )
					if ( mtree[M].begin >= stari ) break;
				mtree[j].brother = M;
				mtree[i].brother = j;
			}
		}
		SORT(j, orient);
	}
}

/* Display the alignments of segments */
SHOW()
{ 
	register  int   i, j, k;	/* index variables */
	char *my_calloc();		/* space-allocating function */
	int   n;			/* number of working segments */
	int   limit;			/* number of slots in work */
	int   col;			/* number of output columns prepared */
	short done;			/* tells if current group is done */
	rowptr root;			/* pointer to root of op tree */
	int   sym[6];			/* occurrence counts for six chars */
	char  c;			/* temp variable */
	rowptr t, w, yy;		/* temp pointer */
	int    x;			/* temp variables */
	int   group;			/* Contigs number */
	char  conlit[20], *a;		/* String form of contig number */
	char  *spt;			/* pointer to the start of consensus */

	work = ( rowptr * ) my_calloc( seg_num * sizeof(rowptr));
	group = 0;
	yy = 0;
	for ( j = 0; j < 6; j++ )
		sym[j] = 0;
	n = limit = col = 0;
	for ( i = 0; i < seg_num; i++ )
		if ( mtree[i].head )
		{ 
			(void) sprintf(conlit, ">Contig %d\n", group);
			for ( a = conlit; *a; )
				*allconpt++ = *a++;
			/* Mod by S.S.
     (void) printf("\n#Contig %d\n\n", group++);
*/
			group++;
			done = 0;
			ENTER(&limit, &n, i, col, yy);
			root = work[0];
			spt = allconpt;
			while ( ! done )
			{ 
				for ( j = 0; j < n; j++ )	/* get segments into work */
				{ 
					t = work[j];
					k = t->id;
					if ((x = mtree[k].next) != -1 && mtree[x].begin == t->loc)
					{ 
						ENTER(&limit, &n, x, col, t);
						mtree[k].next = -1;
					}
					for ( x = mtree[k].child; x != -1; x = mtree[x].brother )
						if ( mtree[x].begin == t->loc )
						{ 
							ENTER(&limit, &n, x, col, t);
							mtree[k].child = mtree[x].brother;
						}
						else
							break;
				}
				COLUMN(root);		/* determine next column */
				root->c = root->kind;
				for ( t = head; t != 0; t = t->link )
					t->c = t->kind;
				for ( j = 0; j < n; j++ )
				{ 
					t = work[j];
					if ( t->done )
						*t->a++ = ' ';
					else
					{ 
						if ( t->c == 'L' )
						{ 
							if ( t->loc == 1 )
								t->offset = allconpt - spt;
							c = *t->a++ = t->seq[t->loc++];
						}
						else
							if ( t->loc > 1 )
								c = *t->a++ = '-';
							else
								c = *t->a++ = ' ';
						if ( c != ' ' )
							if ( c == '-' )
								sym[5] += 1;
							else
								sym[vert[c]] += 1;
						t->c = ' ';
					}
				}
				/* determine consensus char */
				k = sym[0] + sym[1] + sym[2] + sym[3] + sym[4];
				if ( k < sym[5] )
					*allconpt++ = '-';
				else
					if ( sym[0] == sym[1] && sym[1] == sym[2] &&
					    sym[2] == sym[3] )
						*allconpt++ = 'N';
					else
					{ 
						k = sym[0];
						c = 'A';
						if ( k < sym[1] ) { 
							k = sym[1]; 
							c = 'C'; 
						}
						if ( k < sym[2] ) { 
							k = sym[2]; 
							c = 'G'; 
						}
						if ( k < sym[3] ) c = 'T';
						*allconpt++ = c;
					}
				for ( j = 0; j < 6; j++ )
					sym[j] = 0;
				for ( t = head; t != 0; t = t->link )
				{ 
					NEXTOP(t);
					if ( t->done )	/* delete it from op tree */
					{ 
						w = t->father;
						if ( w->child->id == t->id )
							w->child = t->brother;
						else
						{ 
							w = w->child;
							for ( ; w->brother->id != t->id; w = w->brother )
								;
							w->brother = t->brother;
						}
					}
				}
				if ( root->loc > root->length )	/* check root node */
				{ 
					root->done = 1;
					if ( (w = root->child) != 0 )
					{ 
						w->father = 0;
						root = w;
					}
					else
						done = 1;
				}
				col++;
				if ( col == LINELEN || done )	/* output */
				{ 
					col = 0;
					for ( j = 0; j < n; j++ )
					{ 
						t = work[j];
						if ( t->done )
						/*
		Mod by S.S.
		      { (void) printf("#");
			for ( a = t->name; *a; a++ )
			  (void) printf("%c", *a);
*/
						{
							int jj;
							(void) printf("{\nname	");
							for(jj=0;jj<strlen(t->name)-1;jj++)
								(void) printf("%c", t->name[jj]);
							printf("\nstrandedness	%c\n",
								t->name[strlen(t->name)] == '+'? '1':'2');

							printf("offset	%d\ntype  DNA\ngroup-ID	%d\nsequence	\"\n",
							    t->offset,group);
							for ( k = 0, a = t->line ; a != t->a; a++ )
								if ( *a != ' ' )
								{ 
									k++;
									(void)  printf("%c", *a);
									if ( k == LINELEN )
									{ 
										(void)  printf("\n");
										k = 0;
									}
								}
/*
	                if ( k )
*/
							(void)  printf("\"\n}\n");
						}
						if ( t->linesize - (t->a - t->line) < LINELEN + 3 )
							ALOC_SEQ(t);
					}
					if ( !done )
					{ 
						for ( k = j = n - 1; j >= 0; j-- )
							if ( work[j]->done )
							{ 
								t = work[j];
								for ( x = j; x < k; x++ )
									work[x] = work[x+1];
								work[k--] = t;
							}
						n = k + 1;
					}
					else
						n = 0;
				}
			}
		}
}

/* allocate more space for output fragment */
ALOC_SEQ(t) rowptr t;
{ 
	char *my_calloc();		/* space-allocating function */
	char  *start, *end, *p;
	t->linesize *= 2;
	start = t->line;
	end = t->a;
	t->line = ( char * ) my_calloc( t->linesize * sizeof(char));
	for ( t->a = t->line, p  = start ; p != end; )
		*t->a++ = *p++;
	free(start);
	return 0;
}

/* enter a segment into working set */
ENTER(b, d, id, pos, par) int  *b, *d, id, pos; 
rowptr par;
{ 
	int  i;
	char *my_calloc();		/* space-allocating function */
	rowptr  t;

	if ( *b <= *d )
	{ 
		work[*b] = ( rowptr ) my_calloc( (int ) sizeof(row));
		work[*b]->line = ( char * ) my_calloc( SEQLEN * sizeof(char));
		work[*b]->linesize = SEQLEN;
		*b += 1;
	}
	t = work[*d];
	*d += 1;
	t->a = t->line;
	for ( i = 0; i < pos; i++ )
		*t->a++ = ' ';
	t->c = ' ';
	t->seq = mtree[id].orient ? segment[id].seq : segment[id].rev;
	t->length = segment[id].length;
	t->id = id;
	if ( par != 0 )
	{ 
		t->s = mtree[id].script;
		t->size = mtree[id].size;
	}
	t->op = 0;
	for ( i = 1; i <= segment[id].len && i <= NAMELEN; i++ )
		t->name[i-1] = segment[id].name[i];
	if ( mtree[id].orient )
		t->name[i-1] = '+';
	else
		t->name[i-1] = '-';
	t->name[i] = '\0';
	t->done = 0;
	t->loc = 1;
	t->child = 0;
	t->father = par;
	if ( par != 0 )
	{ 
		t->brother = par->child;
		par->child = t;
		NEXTOP(t);
	}
}

/* get the next operation */
NEXTOP(t) rowptr  t;
{	
	if ( t->size || t->op )
		if ( t->op == 0 && *t->s == 0 )
		{ 
			t->op = *t->s++;
			t->size--;
			t->up = 'L';
			t->dw = 'L';
		}
		else
		{ 
			if ( t->op == 0 )
			{ 
				t->op = *t->s++;
				t->size--;
			}
			if ( t->op > 0 )
			{ 
				t->up = '-';
				t->dw = 'L';
				t->op--;
			}
			else
			{ 
				t->up = 'L';
				t->dw = '-';
				t->op++;
			}
		}
	else
		if ( t->loc > t->length )
			t->done = 1;
}

COLUMN(x) rowptr x;
{ 
	rowptr  y;
	rowptr  start, end;		/* first and last nodes for subtree */

	if ( x->child == 0 )
	{ 
		head = tail = 0;
		x->kind = 'L';
	}
	else
	{ 
		start = end = 0;
		x->kind = 'L';
		for ( y = x->child; y != 0; y = y->brother )
		{ 
			COLUMN(y);
			if ( x->kind == y->up )
				if ( y->kind == y->dw )
				{ 
					if ( head == 0 )
					{ 
						y->link = 0;
						head = tail = y;
					}
					else
					{ 
						y->link = head;
						head = y;
					}
					if ( end == 0 )
						start = head;
					else
						end->link = head;
					end = tail;
				}
				else
					if ( y->kind == '-' )
					{ 
						start = head;
						end = tail;
						x->kind = '-';
					}
					else
					{ 
						y->link = 0;
						y->kind = '-';
						if ( end == 0 )
							start = end = y;
						else
						{ 
							end->link = y;
							end = y;
						}
					}
			else
				if ( y->kind == y->dw )
					if ( x->kind == '-' )
						;
					else
					{ 
						if ( head == 0 )
						{ 
							y->link = 0;
							head = tail = y;
						}
						else
						{ 
							y->link = head;
							head = y;
						}
						start = head;
						end = tail;
						x->kind = '-';
					}
				else
					if ( x->kind == '-' )
						if ( y->kind == '-' )
						{ 
							if ( end == 0 )
							{ 
								start = head;
								end = tail;
							}
							else
								if ( head == 0 )
/* code folded from here */
	;
/* unfolding */
								else
								{ 
/* code folded from here */
	end->link = head;
	end = tail;
/* unfolding */
								}
						}
						else
							;
					else
					{ 
						start = head;
						end = tail;
						x->kind = '-';
					}
		}
		head = start;
		tail = end;
	}
}

/* Display a summary of contigs */
GRAPH()
{ 
	int   i, j, k;		/* index variables */
	int   group;			/* serial number of contigs  */
	char name[NAMELEN+2];		/* name of segment */
	char  *t;			/* temp var */
	int   length;			/* length of name */

	(void)	printf("\nOVERLAPS            CONTAINMENTS\n\n");
	group = 1;
	for ( i = 0; i < seg_num; i++ )
		if ( mtree[i].head )
		{ 
			(void) printf("******************* Contig %d ********************\n",
			    group++ );
			for ( j = i; j != -1; j = mtree[j].next )
			{ 
				length = segment[j].len;
				t = segment[j].name + 1;
				for ( k = 0; k < length && k < NAMELEN; k++ )
					name[k] = *t++;
				if ( mtree[j].orient )
					name[k] = '+';
				else
					name[k] = '-';
				name[k+1] = '\0';
				(void) printf("%s\n", name);
				CONTAIN(mtree[j].child, name);
			}
		}
}

CONTAIN(id, f) int  id; 
char *f;
{ 
	int   k;			/* index variable */
	char name[NAMELEN+2];		/* name of segment */
	char  *t;			/* temp var */
	int   length;			/* length of name */

	if ( id != -1 )
	{ 
		length = segment[id].len;
		t = segment[id].name + 1;
		for ( k = 0; k < length && k < NAMELEN; k++ )
			name[k] = *t++;
		if ( mtree[id].orient )
			name[k] = '+';
		else
			name[k] = '-';
		name[k+1] = '\0';
		(void) printf("                    %s is in %s\n", name,f);
		CONTAIN(mtree[id].child, name);
		CONTAIN(mtree[id].brother, f);
	}
}

big_pass(A,B,M,N,orienti,orientj) char A[],B[]; 
int  M,N; 
short orienti, orientj;
{ 
	register  int   i, j;			/* row and column indices */
	register  int   c;			/* best score at current point */
	register  int   f;			/* best score ending with insertion */
	register  int   d;			/* best score ending with deletion */
	register  int   p;			/* best score at (i-1, j-1) */
	register  int   ci;			/* end-point associated with c */

	register  int   di;			/* end-point associated with d */
	register  int   fi;			/* end-point associated with f */
	register  int   pi;			/* end-point associated with p */
	int   *va;				/* pointer to v(A[i], B[j]) */
	int   x1, x2;		/* regions of A before x1 or after x2 are lightly penalized */
	int   y1, y2;		/* regions of B before y1 or after y2 are lightly penalized */
	short  heavy;		/* 1 = heavy penalty */
	int   ex, gx;		/* current gap penalty scores */

	/* determine x1, x2, y1, y2 */
	if ( POS5 >= POS3 )
		fatal("The value for POS5 must be less than the value for POS3");
	if ( orienti )
	{ 
		x1 = POS5 >= M ? 1 : POS5;
		x2 = POS3 >= M ? M : POS3;
	}
	else
	{ 
		x1 = POS3 >= M ? 1 : M - POS3 + 1;
		x2 = POS5 >= M ? M : M - POS5 + 1;
	}
	if ( orientj )
	{ 
		y1 = POS5 >= N ? 1 : POS5;
		y2 = POS3 >= N ? N : POS3;
	}
	else
	{ 
		y1 = POS3 >= N ? 1 : N - POS3 + 1;
		y2 = POS5 >= N ? N : N - POS5 + 1;
	}
	if ( x1 + 1 <= x2 ) x1++;
	if ( y1 + 1 <= y2 ) y1++;
	heavy = 0;

	/* Compute the matrix.
	   CC : the scores of the current row
	   RR : the starting point that leads to score CC
	   DD : the scores of the current row, ending with deletion
	   SS : the starting point that leads to score DD        */
	/* Initialize the 0 th row */
	for ( j = 1; j <= N ; j++ )
	{  
		CC[j] = 0;
		DD[j] = - (q);
		RR[j] = SS[j] = -j;
	}
	for ( i = 1; i <= M; i++)
	{  
		if ( i == x1 ) heavy = 1 - heavy;
		if ( i == x2 ) heavy = 1 - heavy;
		ex = r1;
		gx = qr1;
		va = v1[A[i]];
		c = 0;				/* Initialize column 0 */
		f = - (q);
		ci = fi = i;
		p = 0;
		pi = i - 1;
		for ( j = 1 ; j <= N ; j++ )
		{  
			if ( j == y1 )
			{ 
				if ( heavy )
				{ 
					ex = r;
					gx = qr;
					/*
S.S.
	                va = v[A[i]];
*/
					va = v[A[i]];
				}
			}
			if ( j == y2 )
			{ 
				if ( heavy )
				{ 
					ex = r1;
					gx = qr1;
					va = v1[A[i]];
				}
			}
			if ( ( f = f - ex ) < ( c = c - gx ) )
			{ 
				f = c; 
				fi = ci; 
			}
			di = SS[j];
			if ( ( d = DD[j] - ex ) < ( c = CC[j] - gx ) )
			{ 
				d = c; 
				di = RR[j]; 
			}
			c = p+va[B[j]];		/* diagonal */
			ci = pi;
			if ( c < d )
			{ 
				c = d; 
				ci = di; 
			}
			if ( c < f )
			{ 
				c = f; 
				ci = fi; 
			}
			p = CC[j];
			CC[j] = c;
			pi = RR[j];
			RR[j] = ci;
			DD[j] = d;
			SS[j] = di;
			if ( ( j == N || i == M ) &&  c > SCORE )
			{ 
				SCORE = c;
				ENDI = i;
				ENDJ = j;
				STARI = ci;
			}
		}
	}
	if ( SCORE )
		if ( STARI < 0 )
		{ 
			STARJ = - STARI;
			STARI = 0;
		}
		else
			STARJ = 0;
}

/* diff(A,B,M,N,tb,te) returns the score of an optimum conversion between
   A[1..M] and B[1..N] that begins(ends) with a delete if tb(te) is zero
   and appends such a conversion to the current script.                   */

int  diff(A,B,M,N,tb,te) char *A, *B; 
int  M, N; 
int  tb, te;

{ 
	int    midi, midj, type;	/* Midpoint, type, and cost */
	int  midc;

	{ 
		register int    i, j;
		register int  c, e, d, s;
		int  t, *va;
		char  *my_calloc();

		/* Boundary cases: M <= 1 or N == 0 */

		if (N <= 0)
		{ 
			if (M > 0) DEL(M)
				return - gap(M);
		}
		if (M <= 1)
		{ 
			if (M <= 0)
			{ 
				INS(N);
				return - gap(N);
			}
			if (tb > te) tb = te;
			midc = - (tb + r + gap(N) );
			midj = 0;
			va = v[A[1]];
			for (j = 1; j <= N; j++)
			{ 
				c = va[B[j]] - ( gap(j-1) + gap(N-j) );
				if (c > midc)
				{ 
					midc = c;
					midj = j;
				}
			}
			if (midj == 0)
			{ 
				INS(N) DEL(1) 			}
			else
			{ 
				if (midj > 1) INS(midj-1)
					REP
					    if ( (A[1]|32) == (B[midj]|32) )
						no_mat += 1;
					else
						no_mis += 1;
				if (midj < N) INS(N-midj)
			}
			return midc;
		}

		/* Divide: Find optimum midpoint (midi,midj) of cost midc */

		midi = M/2;			/* Forward phase:                          */
		CC[0] = 0;			/*   Compute C(M/2,k) & D(M/2,k) for all k */
		t = -q;
		for (j = 1; j <= N; j++)
		{ 
			CC[j] = t = t-r;
			DD[j] = t-q;
		}
		t = -tb;
		for (i = 1; i <= midi; i++)
		{ 
			s = CC[0];
			CC[0] = c = t = t-r;
			e = t-q;
			va = v[A[i]];
			for (j = 1; j <= N; j++)
			{ 
				if ((c = c - qr) > (e = e - r)) e = c;
				if ((c = CC[j] - qr) > (d = DD[j] - r)) d = c;
				c = s+va[B[j]];
				if (c < d) c = d;
				if (c < e) c = e;
				s = CC[j];
				CC[j] = c;
				DD[j] = d;
			}
		}
		DD[0] = CC[0];

		RR[N] = 0;			/* Reverse phase:                          */
		t = -q;			/*   Compute R(M/2,k) & S(M/2,k) for all k */
		for (j = N-1; j >= 0; j--)
		{ 
			RR[j] = t = t-r;
			SS[j] = t-q;
		}
		t = -te;
		for (i = M-1; i >= midi; i--)
		{ 
			s = RR[N];
			RR[N] = c = t = t-r;
			e = t-q;
			va = v[A[i+1]];
			for (j = N-1; j >= 0; j--)
			{ 
				if ((c = c - qr) > (e = e - r)) e = c;
				if ((c = RR[j] - qr) > (d = SS[j] - r)) d = c;
				c =  s+va[B[j+1]];
				if (c < d) c = d;
				if (c < e) c = e;
				s = RR[j];
				RR[j] = c;
				SS[j] = d;
			}
		}
		SS[N] = RR[N];

		midc = CC[0]+RR[0];		/* Find optimal midpoint */
		midj = 0;
		type = 1;
		for (j = 0; j <= N; j++)
			if ((c = CC[j] + RR[j]) >= midc)
				if (c > midc || CC[j] != DD[j] && RR[j] == SS[j])
				{ 
					midc = c;
					midj = j;
				}
		for (j = N; j >= 0; j--)
			if ((c = DD[j] + SS[j] + q) > midc)
			{ 
				midc = c;
				midj = j;
				type = 2;
			}
	}

	/* Conquer: recursively around midpoint */

	if (type == 1)
	{
		(void) diff(A,B,midi,midj,tb,q);
		(void) diff(A+midi,B+midj,M-midi,N-midj,q,te);
	}
	else
	{
		(void) diff(A,B,midi-1,midj,tb,zero);
		DEL(2);
		(void) diff(A+midi+1,B+midj,M-midi-1,N-midj,zero,te);
	}
	return midc;
}

/* lib.c - library of C procedures. */

/* fatal - print message and die */
fatal(msg)
char *msg;
{
	(void) fprintf(stderr, "%s\n", msg);
	exit(1);
}

/* fatalf - format message, print it, and die */
fatalf(msg, val)
char *msg, *val;
{
	(void) fprintf(stderr, msg, val);
	(void)	putc('\n', stderr);
	exit(1);
}

/* ckopen - open file; check for success */
FILE *ckopen(name, mode)
char *name, *mode;
{
	FILE *fopen(), *fp;

	if ((fp = fopen(name, mode)) == NULL)
		fatalf("Cannot open %s.", name);
	return(fp);
}

/* my_calloc - allocate space; check for success */
char *my_calloc(amount)
int  amount;
{
	char *malloc(), *p;

	if ((p = malloc( (unsigned) amount)) == NULL)
		fatal("Ran out of memory.");
	return(p);
}

