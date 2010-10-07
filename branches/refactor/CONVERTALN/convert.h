/* data structure for each file format and sequences */

#define INITSEQ 6000
#define RNA     0
#define NONRNA  1

#define GENBANK   'g'
#define MACKE     'm'
#define SWISSPROT 't'
#define PHYLIP    'y'
#define PHYLIP2   '2'
#define NEXUS     'p'
#define EMBL      'e'
#define GCG       'c'
#define PRINTABLE 'r'
#define NBRF      'n'
#define STADEN    's'

#define UNKNOWN  (-1)
#define AUTHOR   1
#define JOURNAL  2
#define TITLE    3
#define STANDARD 4
#define PROCESS  5
#define REF      6
#define ALL      0

#define SEQLINE 60

// -------------------------------
//      Comment (Embl+GenBank)

struct OrgInfo {
    int   exist;
    char *source;
    char *cc;
    char *formname;
    char *nickname;
    char *commname;
    char *hostorg;
};

struct SeqInfo {
    int   exist;
    char *RDPid;
    char *gbkentry;
    char *methods;
    char  comp5;                /* yes or no, y/n */
    char  comp3;                /* yes or no, y/n */
};

struct Comment {
    OrgInfo  orginf;
    SeqInfo  seqinf;
    char    *others;
};

// -------------
//      Embl

struct Emblref {
    char *author;
    char *title;
    char *journal;
    char *processing;
};

struct Embl {
    char    *id;                /* entry name */
    char    *dateu;             /* date of last updated */
    char    *datec;             /* date of created */
    char    *description;       /* description line (DE) */
    char    *os;                /* Organism species */
    char    *accession;         /* accession number(s) */
    char    *keywords;          /* keyword */
    int      numofref;          /* num. of reference */
    Emblref *reference;
    char    *dr;                /* database cross-reference */
    Comment  comments;          /* comments */
};

// -----------------
//      GenBank

struct GenbankRef {
    char *ref;
    char *author;
    char *title;
    char *journal;
    char *standard;
};

struct GenBank {
    char       *locus;
    char       *definition;
    char       *accession;
    char       *keywords;
    char       *source;
    char       *organism;
    int         numofref;
    GenbankRef *reference;
    Comment     comments;
};

// --------------
//      Macke

struct Macke {
    char  *seqabbr;             /* seq. abbrev. */
    char  *name;                /* seq. full name */
    int    rna_or_dna;          /* rna or dna */
    char  *atcc;                /* CC# of seq. */
    char  *rna;                 /* Sequence methods, old version entry */
    char  *date;                /* date of modification */
    char  *nbk;                 /* GenBank information -old version entry */
    char  *acs;                 /* accession number */
    char  *author;              /* author of the first reference */
    char  *journal;             /* journal of the first reference */
    char  *title;               /* title of the first reference */
    char  *who;                 /* who key in the data */
    char  *strain;              /* strain */
    char  *subspecies;          /* subspecies */
    int    numofrem;            /* num. of remarks */
    char **remarks;             /* remarks */
};

// -------------
//      Paup

struct Paup {
    int         ntax;           /* number of sequences */
    int         nchar;          /* max number of chars per seq. */
    int         labelpos;       /* Label start from left or right */
    int         missing;
    const char *equate;         /* equal meaning char */
    int         interleave;     /* interleave or sequential */
    int         datatype;       /* rna in this case */
    char        gap;            /* char of gap, default is '-' */
    int         gapmode;
    /* if sequence data is not too large, read into memory at once */
    /* otherwise, read in one by one. */
};

// -------------
//      Nbrf

struct Nbrf {
    char *id;                   /* locus */
    char *description;
};


