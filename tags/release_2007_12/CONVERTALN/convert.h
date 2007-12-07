/* data structure for each file format and sequences */

#ifndef FILEBUFFER_H
#include <FileBuffer.h>
#endif

#ifdef SGI
#define char signed char
#endif

#define INITSEQ 6000
#define RNA	0
#define NONRNA	1
#define EQ	1
#define NONEQ	0

#define GENBANK	'g'
#define MACKE	'm'
#define PROTEIN	't'
#define PHYLIP	'y'
#define PHYLIP2	'2'
#define PAUP	'p'
#define EMBL	'e'
#define GCG	'c'
#define PRINTABLE	'r'
#define ALMA	'l'
#define NBRF	'n'
#define STADEN	's'

#define UNKNOWN (-1)
#define AUTHOR	1
#define JOURNAL	2
#define TITLE	3
#define STANDARD	4
#define PROCESS	5
#define REF	6
#define ALL	0

#define SEQLINE 60
/* GenBank data structure */
typedef struct	{
	char	*ref;
	char	*author;
	char	*title;
	char	*journal;
	char	*standard;
} Reference;
typedef struct	{
	int	exist;
	char	*source;
	char	*cc;
	char	*formname;
	char	*nickname;
	char	*commname;
	char	*hostorg;
} Oinf;
typedef struct	{
	int	exist;
	char	*RDPid;
	char	*gbkentry;
	char	*methods;
	char	comp5;		/* yes or no, y/n */
	char	comp3;		/* yes or no, y/n */
} Sinf;
typedef struct	{
	Oinf	orginf;
	Sinf	seqinf;
	char	*others;
} Comment;
typedef struct	{
	char	*locus;
	char	*definition;
	char	*accession;
	char	*keywords;
	char	*source;
	char	*organism;
	int	numofref;
	Reference	*reference;
	Comment	comments;
} GenBank;
/* Macke data structure */
typedef	struct	{
	char  *seqabbr;	            /* seq. abbrev. */
	char  *name;		        /* seq. full name */
	int	   rna_or_dna;	        /* rna or dna */
	char  *atcc;		        /* CC# of seq. */
	char  *rna;		            /* Sequence methods, old version entry */
	char  *date;		        /* date of modification */
	char  *nbk;		            /* GenBank information -old version entry */
	char  *acs;		            /* accession number */
	char  *author;	            /* author of the first reference */
	char  *journal;	            /* journal of the first reference */
	char  *title;		        /* title of the first reference */
	char  *who;		            /* who key in the data */
	char  *strain;	            /* strain */
	char  *subspecies;	        /* subspecies */
	int	   numofrem;	        /* num. of remarks */
	char **remarks;	            /* remarks */
} Macke;
/* Paup data structure */
typedef struct	{
	int	        ntax;		    /* number of sequences */
	int	        nchar;		    /* max number of chars per seq. */
	int	        labelpos;	    /* Label start from left or right */
	int	        missing;
	const char *equate;	        /* equal meaning char */
	int	        interleave;	    /* interleave or sequential */
	int	        datatype;	    /* rna in this case */
	char	    gap;		    /* char of gap, default is '-' */
	int	        gapmode;
	/* if sequence data is not too large, read into memory at once */
	/* otherwise, read in one by one. */
} Paup;
typedef struct	{
	char	*author;
	char	*title;
	char	*journal;
	char	*processing;
} Emblref;
typedef struct	{
	char	*id;		/* entry name */
	char	*dateu;		/* date of last updated */
	char	*datec;		/* date of created */
	char	*description;	/* description line (DE) */
	char	*os;		/* Organism species */
	char	*accession;	/* accession number(s) */
	char	*keywords;	/* keyword */
	int	numofref;	/* num. of reference */
	Emblref	*reference;
	char	*dr;		/* database cross-reference */
	Comment	comments;	/* comments */
} Embl;
typedef struct	{
	char	*id;
	char	*filename;
	int	format;		/* could be NBRF, UWGCG, RMBL, STADEN */
	char	defgap;
	int	num_of_sequence;/* num of sequence after merging with gaps */
	char	*sequence;	/* sequence data after mergin with gaps */
} Alma;
typedef struct	{
	char	*id;		/* locus */
	char	*description;
} Nbrf;

/* one sequence entry */
struct	{
	int	      numofseq;		    /* number of sequences */
	int	      seq_length;		/* sequence length */
	int	      max;
	char	 *sequence;		    /* sequence data */
	/* to read all the sequences into memory at one time */
	char	**ids;			    /* array of ids. */
	char	**seqs;			    /* array of sequence data */
    int      *lengths;          /* array of sequence lengths */
    int       allocated;        /* for how many sequences space has been allocated */
	/* PAUP, PHYLIP, GCG, and PRINTABLE */
	GenBank	  gbk;			    /* one GenBank entry */
	Macke 	  macke;			/* one Macke entry */
	Paup	  paup;			    /* one Paup entry */
	Embl	  embl;
	Alma	  alma;
	Nbrf	  nbrf;
} data;

