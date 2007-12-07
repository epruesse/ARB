#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
// #include <malloc.h>
#include <time.h>

#define const                /* const is not defined in non-ansi C,
			      * so it does not affect anything.
			      * Take this define off when using
			      * ANSI C compiler.
			      */

#define NUM_OF_FIELDS  25    /* number of fields in "struct Sequence" other
				than *len and *maxlen fields.
				update this number when changing at[] list. */

/* C style T/F definitions. */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX
#define MAX(a,b)   ( (a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)   ( (a) < (b) ? (a) : (b))
#endif

typedef struct
{
    char type[32];         /* DNA, RNA, AA/PROTEIN, TEXT, SCORE(0-F) */
    char status[32];       /* unmade, pending, unsolved, solved. */
    char name[64];
    char sequence_ID[32];

    char *c_elem;
    int  seqlen;
    int  seqmaxlen;

    int  creation_date[6]; /* yy/mm/dd/hh/mn/sc */
    char creator[32];
    char film[32];
    char membrane[32];
    int  laneset;
    char source_ID[32];
    char contig[32];
    int  strandedness;  /* 0: unspecified, 1:pri(default),  2:sec  */
    int  direction;     /* 0: unspecified, 1:5to3>(default), -1:3to5< */
    int  offset;
    char *comments;
    int  commentslen;
    int  commentsmaxlen;
    char *baggage;
    int  baglen;
    int  bagmaxlen;
    int  group_number;
    int  group_ID;
    char barcode[16];
    int  orig_direction;  /* 0: unknown, 1:5'->3',  0:3'->5'*/
    int  orig_strand;     /* 0: unknown, 1:primary, 0:secondary */
    int  probing_date[6];
    int  autorad_date[6];
    char walk[32]; /* "TRUE", "FALSE" or whatever */
} Sequence;


/***
 ***  Elements in at[] and e_tags should be IN THE SAME ORDER.
 ***/

static char *at[NUM_OF_FIELDS] = {
  "type",
  "status",
  "name",
  "sequence-ID",
  "sequence" ,
  "creation-date" ,
  "creator" ,
  "film" ,
  "membrane" ,
  "laneset" ,
  "source-ID" ,
  "contig" ,
  "strandedness" ,
  "direction" ,
  "offset" ,
  "comments" ,
  "baggage",
  "group-number",
  "barcode",
  "orig_direction",
  "orig_strand",
  "probing-date",
  "autorad-date",
  "group-ID",
  "walk"
};


enum e_tags { e_type,          /*0*/
	      e_status,
	      e_name,          /*2*/
	      e_sequence_ID ,  /*3*/
              e_c_elem      ,
              e_creation_date,
              e_creator,       /*6*/
              e_film,
              e_membrane,
              e_laneset,
	      e_source_ID,
              e_contig,
              e_strandedness,
	      e_direction,
              e_offset,        /*14*/
              e_comments,
	      e_baggage,
              e_group_number,  /*17*/
	      e_barcode,       /*18*/
	      e_orig_direction,
	      e_orig_strand,   /*20*/
	      e_probing_date,
	      e_autorad_date,
              e_group_ID,      /*23*/
	      e_walk
            };

typedef struct {
  char symbol[2];
  int field;
  char *value;
} str_cond ;

typedef struct {
    char prompt[20];
    int  optional;      /* 'T' or 'F' */
    char tag;
    char strvalue[100]; /* default value or fill in by this program. */
} Args;

