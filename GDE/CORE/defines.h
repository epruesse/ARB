/*

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/
#define USE_ARB
// #ifndef GB_INCLUDED
// typedef int *GBDATA;
// #endif

struct GBDATA;

#define OWTOOLKIT_WARNING_DISABLED
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/xview.h>

#if defined(DEBUG)
#define gde_assert(cond) do { if (!(cond)) (*(int*)0) = 0; } while(0) /* force SIGSEGV */
#else
#define gde_assert(cond)
#endif /* DEBUG */


#define TRUTH 1
#define JUSTICE 2
#define BEAUTY 3

/*
*	Edit modes
*/

#define INSERT 0
#define CHECK 1

/*
*	Cursor directions
*/
#define RIGHT 1
#define LEFT 0
#define UP 0
#define DOWN 1

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define GBUFSIZ 512
#define MAX_NA_DISPLAY_WIDTH 1024
#define MAX_NA_DISPLAY_HEIGHT 1024
#define MAX_STARTUP_CANVAS_HEIGHT 512
#define grey_height 8
#define grey_width 8

/*
*	Definable dialog types
*/
#define	TEXTFIELD	0x1
#define SLIDER		0x2
#define CHOOSER		0x3
#define CHOICE_MENU	0x4
#define CHOICE_LIST	0x5

/*
*	File Formats
*/
#define GDE		0x100
#define GENBANK		0x101
#define NA_FLAT		0x102
#define COLORMASK	0x103
#define STATUS_FILE	0x104


#ifdef USE_ARB
#define ARBDB		0x105
#endif
/*
*	Protection bits
*/

#define	PROT_BASE_CHANGES 	0x1	/* Allow base changes */
#define PROT_GREY_SPACE		0x2	/* Allow greyspace modification */
#define PROT_WHITE_SPACE	0x4	/* Allow whitespace modification */
#define PROT_TRANSLATION 	0x8	/* Allow translation  */
#define PROT_REORIENTATION	0x10	/* Allow reorientation */


/*
*	File loading methods (must be 'OR/AND' able)
*/

#define NONE		0x0
#define	DESTROY		0x1
#define LOAD		0x2
#define SAVE		0x4
#define SELECTED	0x8
#define ALL			0x10
#define SELECT_REGION	0x20
#define SELECT_ONE	0x30

/*
*	Sequence DISPLAY Types
*/
#define NASEQ_ALIGN	0x201
#define NASEQ		0x202

/*
*	Sequence Data Types
*/
#define DNA			0x300
#define RNA			0x301
#define TEXT		0x302
#define MASK		0x303
#define PROTEIN		0x304
/*
*	extended sequence attributes (true/false)
*/

#define IS_5_TO_3	0x01	/* 5prime to 3 prime */
#define IS_3_TO_5	0x02	/* 3 prime to 5 prime */
#define IS_CIRCULAR	0x04	/* circular dna		*/
#define IS_PRIMARY	0x10	/* on the primary strand */
#define	IS_SECONDARY	0x20	/* on the secondary strand */
#define IS_MODIFIED	0x40	/* modification flag */
#define IS_ORIG_PRIMARY 0x80	/* Original sequence was primary */
#define IS_ORIG_SECONDARY 0x100	/* Original sequence was secondary */
#define IS_ORIG_5_TO_3	0x200	/* Original sequence was 5_to_3 */
#define IS_ORIG_3_TO_5	0x400	/* Original sequence was 3_to_5 */

#ifdef HGL
#define DEFAULT_X_ATTR  0
#else
#define DEFAULT_X_ATTR  IS_5_TO_3+IS_PRIMARY;
#endif

/*
*	Other display attributed
*/
#define INVERTED 1
#define VSCROLL_LOCK 2
#define KEYCLICKS 4
#define GDE_MESSAGE_PANEL 8

/*
*	Coloring Methods
*/
#define COLOR_MONO 	0x40	/* no color, simple black and white */
#define COLOR_LOOKUP	0x41	/* Use a simple value->color lookup */
#define COLOR_ALN_MASK 	0x42	/* The alignment has a column by column color
				mask associated with it */
#define COLOR_SEQ_MASK	0x43	/* Each sequence has a color mask*/
#define COLOR_STRAND	0x44	/* Color based on original strandedness*/


/*
*	Data types
*/

typedef struct
{
	int *valu;
} NumList;


typedef struct
{
	struct
	{
		int yy;
		int mm;
		int dd;
		int hr;
		int mn;
		int sc;
	} origin,modify;
} TimeStamp;

typedef unsigned char NA_Base;

typedef struct
{
	char *name;
	int type;
	NumList *list;
	int listlen;
	int maxlen;
} GMask;


typedef struct NA_SeqStruct
{
	char id[80];			/* sequence id (ACCESSION)*/
	char seq_name[80];		/* Sequence name (ORGANISM) */
	char short_name[32];		/* Name (LOCUS) */
	char barcode[80];
	char contig[80];
	char membrane[80];
	char description[80];		/* Description (DEFINITION)*/
	char authority[80];		/* Author (or creator) */
	char *comments;			/* Stuff we can't parse */
	int comments_len, comments_maxlen;

	NA_Base *sequence;		/* List of bases */
	TimeStamp t_stamp;		/* Time stamp of origin/modification */
	Mask *mask;			/* List of masks(analysis/display) */
	int offset;			/* offset into alignment (left white)
						space	*/
	int seqlen;			/* Number of elements in sequence[] */
	int seqmaxlen;			/* Size sequence[] (for mem alloc) */
	unsigned int protect;		/* Protection mask */
	int attr;			/* Extended attributes */
	int groupid;			/* group id */
	int *col_lut;			/* character to color LUT */
	struct NA_SeqStruct *groupb;	/* Group link backward */
	struct NA_SeqStruct *groupf;	/* Group link forward */
	int *cmask;			/* color mask */
	int selected;			/* Selection flag */
	int subselected;		/* Sub selection flag */
	int format;			/* default file format */
	int elementtype;		/* what type of data are being aligned*/
	char *baggage;			/* unformatted comments*/
	int baggage_len,
	    baggage_maxlen;
	int *tmatrix;			/* translation matrix (code->char) */
	int *rmatrix;			/* reverse translation matrix
						(char->code)*/
#ifdef USE_ARB
	GBDATA	*gb_species;		/* ARB */
	long	transaction_nr;		/* Transaction number, be sure you
						are the last one who edited
						this sequence */
#endif
} NA_Sequence;

typedef struct
{
	char *id;			/* Alignment ID */
	char *description;		/* Description of the alignment*/
	char *authority;		/* Who generated the alignment*/
	int *cmask;			/* color mask */
	int cmask_offset;			/* color mask offset */
	int cmask_len;			/* color mask length */
	int ref;			/* reference sequence */
	int numelements;		/* number of data elements */
	int maxnumelements;		/* maximum number of data elements */
	int nummasks;			/* number of masks */
	int maxlen;			/* Maximum length of alignment */
	int rel_offset;			/* add this to every sequence offset */
					/* to orient it back to 0 */
	Mask *mask;			/* masks */
	NA_Sequence *element;		/* alignment elements */
	int numgroups;			/* number of groups */
	NA_Sequence **group;		/* link to array of pointers into
					   each group */
	char *na_ddata;			/* display data */
	int format;			/* default file format */
	char *selection_mask;		/* Sub sequence selection mask */
	int selection_mask_len;		/* Sub selection mask length */
	int min_subselect;		/* Leftmost coord of selection mask */
#ifdef USE_ARB
	GBDATA *gb_main;
	char	*use;
#endif
} NA_Alignment;


typedef struct
{
	int font_dx;			/* width of a character in this font*/
	int font_dy;			/* height of a character in this font*/
	int wid,ht;			/* width and height of edit win (in
					   characters */
	int top_seq;			/* Top sequence index shown */
	int lft_pos;			/* Leftmost column (in alignment
					   position coords) */
	int color_type;			/* Method of manipulating colors
					   (See above) */
	int depth;			/* number of color bits available */
	int num_colors;			/* Actual number of colors used */
	int *palette;			/* palette for display */
	int *col_lut;			/* character to color LUT */
	int black,white;		/* color indicies for blk,wht */
	int cursor_x,cursor_y;		/* Current cursor positions */
	int position;			/* Current position minus whitespace */
	int *jumptbl;			/* the jump table for fast access
					   into the sequence data */
	int jtsize;			/* its length */
	NA_Alignment *aln;		/* Pointer to the actual data set
					   (the alignment */
	Xv_font font;			/* The default font */
	Canvas seq_can,nam_can;		/* ties to the canvas for screen
					   updates. */
	Window seq_x,nam_x;		/* X versions of the above */
	int use_repeat;			/* Number keys set repeat count*/

} NA_DisplayData;

struct gde_time { int yy; int mm; int dd; int hr; int mn; int sc; };


#define getcmask(a,b) (b < ((a)->offset))?0:((a)->cmask[(b-(a)->offset)])



#include "functions.h"
