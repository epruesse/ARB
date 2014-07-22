#ifndef GDE_DEF_H
#define GDE_DEF_H

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

/* Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
 * All rights reserved.
 */

#define gde_assert(bed) arb_assert(bed)
 
#define TRUTH   1
#define JUSTICE 2
#define BEAUTY  3

// Cursor directions
#define RIGHT 1
#define LEFT  0
#define UP    0
#define DOWN  1

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#define GBUFSIZ                   4096
#define MAX_NA_DISPLAY_WIDTH      1024
#define MAX_NA_DISPLAY_HEIGHT     1024
#define MAX_STARTUP_CANVAS_HEIGHT 512
#define grey_height               8
#define grey_width                8

#define TEXTFIELDWIDTH 15

// Definable dialog types
#define TEXTFIELD      0x1
#define SLIDER         0x2
#define CHOOSER        0x3
#define CHOICE_MENU    0x4
#define CHOICE_LIST    0x5
#define CHOICE_TREE    0x6
#define CHOICE_SAI     0x7
#define CHOICE_WEIGHTS 0x8
#define FILE_SELECTOR  0x9

// File Formats
#define GDE         0x100
#define GENBANK     0x101
#define NA_FLAT     0x102

// Protection bits
#define PROT_BASE_CHANGES  0x1                      // Allow base changes
#define PROT_GREY_SPACE    0x2                      // Allow greyspace modification
#define PROT_WHITE_SPACE   0x4                      // Allow whitespace modification
#define PROT_TRANSLATION   0x8                      // Allow translation
#define PROT_REORIENTATION 0x10                     // Allow reorientation


// File loading methods (must be 'OR/AND' able)
#define NONE          0x0
#define DESTROY       0x1
#define LOAD          0x2
#define SAVE          0x4
#define ALL           0x10

// Sequence DISPLAY Types
#define NASEQ_ALIGN 0x201
#define NASEQ       0x202

// Sequence Data Types
#define DNA     0x300
#define RNA     0x301
#define TEXT    0x302
#define MASK    0x303
#define PROTEIN 0x304

// extended sequence attributes (true/false)
#define IS_5_TO_3         0x01                      // 5prime to 3 prime
#define IS_3_TO_5         0x02                      // 3 prime to 5 prime
#define IS_CIRCULAR       0x04                      // circular dna
#define IS_PRIMARY        0x10                      // on the primary strand
#define IS_SECONDARY      0x20                      // on the secondary strand
#define IS_MODIFIED       0x40                      // modification flag
#define IS_ORIG_PRIMARY   0x80                      // Original sequence was primary
#define IS_ORIG_SECONDARY 0x100                     // Original sequence was secondary
#define IS_ORIG_5_TO_3    0x200                     // Original sequence was 5_to_3
#define IS_ORIG_3_TO_5    0x400                     // Original sequence was 3_to_5

#ifdef HGL
#define DEFAULT_X_ATTR  0
#else
#define DEFAULT_X_ATTR  IS_5_TO_3+IS_PRIMARY;
#endif

// Other display attributed
#define INVERTED          1
#define VSCROLL_LOCK      2
#define KEYCLICKS         4
#define GDE_MESSAGE_PANEL 8

// Coloring Methods
#define COLOR_MONO      0x40    // no color, simple black and white
#define COLOR_LOOKUP    0x41    // Use a simple value->color lookup
#define COLOR_ALN_MASK  0x42    /* The alignment has a column by column color
                                   mask associated with it */
#define COLOR_SEQ_MASK  0x43    // Each sequence has a color mask
#define COLOR_STRAND    0x44    // Color based on original strandedness



// Data types

struct NumList {
    int *valu;
};


struct TimeStamp {
    struct {
        int yy;
        int mm;
        int dd;
        int hr;
        int mn;
        int sc;
    } origin, modify;
};

typedef unsigned char NA_Base;

struct GMask {
    char    *name;
    int      type;
    NumList *list;
    int      listlen;
    int      maxlen;
};


// sizes for fields (including terminating zero byte)
#define SIZE_FIELD_GENBANK 80
#define SIZE_ID            SIZE_FIELD_GENBANK
#define SIZE_SEQ_NAME      SIZE_FIELD_GENBANK
#define SIZE_SHORT_NAME    32
#define SIZE_DESCRIPTION   SIZE_FIELD_GENBANK
#define SIZE_AUTHORITY     SIZE_FIELD_GENBANK

struct NA_Sequence {
    char  id[SIZE_ID];                  // sequence id (ACCESSION)
    char  seq_name[SIZE_SEQ_NAME];      // Sequence name (ORGANISM)
    char  short_name[SIZE_SHORT_NAME];  // Name (LOCUS)
    char  barcode[80];
    char  contig[80];
    char  membrane[80];
    char  description[SIZE_DESCRIPTION];// Description (DEFINITION)
    char  authority[SIZE_AUTHORITY];    // Author (or creator)
    char *comments;                     // Stuff we can't parse
    int   comments_len, comments_maxlen;

    NA_Base      *sequence;     // List of bases
    TimeStamp     t_stamp;      // Time stamp of origin/modification
    GMask        *mask;         // List of masks(analysis/display)
    int           offset;       // offset into alignment (left white) space
    int           seqlen;       // Number of elements in sequence[]
    int           seqmaxlen;    // Size sequence[] (for mem alloc)
    unsigned int  protect;      // Protection mask
    int           attr;         // Extended attributes
    size_t        groupid;      // group id
    int          *col_lut;      // character to color LUT

    NA_Sequence *groupb;        // Group link backward
    NA_Sequence *groupf;        // Group link forward

    int  *cmask;                // color mask
    int   format;               // default file format
    int   elementtype;          // what type of data are being aligned
    char *baggage;              // unformatted comments
    int   baggage_len, baggage_maxlen;
    int  *tmatrix;              // translation matrix (code->char)
    int  *rmatrix;              // reverse translation matrix (char->code)

    GBDATA  *gb_species;
};

struct NA_Alignment {
    char         *id;           // Alignment ID
    char         *description;  // Description of the alignment
    char         *authority;    // Who generated the alignment
    int          *cmask;        // color mask
    int           cmask_offset; // color mask offset
    int           cmask_len;    // color mask length
    int           ref;          // reference sequence
    size_t        numelements;  // number of data elements
    int           maxnumelements; // maximum number of data elements
    int           nummasks;     // number of masks
    int           maxlen;       // Maximum length of alignment
    int           rel_offset;   // add this to every sequence offset
    // to orient it back to 0
    GMask        *mask;         // masks
    NA_Sequence  *element;      // alignment elements
    size_t        numgroups;    // number of groups
    NA_Sequence **group;            /* link to array of pointers into
                                       each group */
    char         *na_ddata;     // display data
    int           format;       // default file format
    char         *selection_mask; // Sub sequence selection mask
    int           selection_mask_len; // Sub selection mask length
    int           min_subselect; // Leftmost coord of selection mask

    GBDATA *gb_main;
    char   *alignment_name;
};

extern NA_Alignment *DataSet;

#define getcmask(a, b) (b < ((a)->offset)) ? 0 : ((a)->cmask[(b-(a)->offset)])

inline void strncpy_terminate(char *dest, const char *source, size_t dest_size) {
    // like strncpy, but also writes terminating zero byte
    // 'dest_size' is overall 'dest'-buffersize (incl. zero terminator)
    strncpy(dest, source, dest_size-1);
    dest[dest_size-1] = 0;
}

#else
#error GDE_def.h included twice
#endif // GDE_DEF_H
