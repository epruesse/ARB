
#include <PT_com.h>
#include <arbdb.h>

#ifdef DEVEL_IDP
#include <list.h>
#include <PT_server.h>
#endif

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define pt_assert(bed) arb_assert(bed)

#define PTM_DEBUG
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned char uchar;

#define  min(a, b) ((a) < (b)) ? (a) : (b)
#define  max(a, b) ((a) > (b)) ? (a) : (b)
#define PT_CORE *(int *)0=0;
#define PT_MAX_MATCHES 10000
#define PT_MAX_IDENTS 10000
#define PT_POS_TREE_HEIGHT 20
#define PT_POS_SECURITY 10
#define MIN_PROBE_LENGTH 8
#define PT_NAME_HASH_SIZE 10000

enum PT_MATCH_TYPE {
    PT_MATCH_TYPE_INTEGER = 0,
    PT_MATCH_TYPE_WEIGHTED_PLUS_POS = 1,
    PT_MATCH_TYPE_WEIGHTED = -1
};



#define MATCHANSWER 50  /* private msg type: match result answer */
#define CREATEANSWER 51 /* private msg type: create result answer */
#define FINDANSWER 52   /* private msg type: find result answer */

extern ulong physical_memory;
struct Hs_struct;
extern char *pt_error_buffer;

typedef enum type_types_type {
    t_int = 1,
    t_string=0,
    t_float=2
    } type_types;

typedef enum PT_bases_enum  {
    PT_QU = 0,
    PT_N = 1,
    PT_A, PT_C, PT_G, PT_T, PT_B_MAX } PT_BASES;

/*  POS TREE */
typedef enum enum_PT_NODE_TYPE {
    PT_NT_LEAF = 0,
    PT_NT_CHAIN = 1,
    PT_NT_NODE = 2,
    PT_NT_SINGLE_NODE = 3,  /* stage 3 */
    PT_NT_SAVED = 3,    /* stage 1+2 */
    PT_NT_UNDEF = 4
    } PT_NODE_TYPE;

typedef struct POS_TREE_struct  {
    uchar       flags;
    char        data;
    } POS_TREE;

typedef struct PTMM_struct {
    char        *base;
    int             stage1;
    int             stage2;
    int             stage3;
    int     mode;
}               PTM2;



/* Probe search */


struct probe_statistic {
    int match_count;        /* Counter for matches */
    double  rel_match_count;    /* match_count / (seq_len - probe_len + 1) */
};

struct probe_input_data {    /* every taxas own data */
                /******* name and sequence *********/
     char   *data;
     long       checksum;
     int    size;
     char   *name;
     char   *fullname;
     GBDATA *gbd;

                /********* probe design ************/
     int    is_group;   /* -1:  nevermind
                    0:  no group
                    1:  group */
                /* probe design (match) */
     PT_probematch *match;  /* best hit for PT_new_design */

                /********* find family  ************/
     struct probe_statistic stat;

                /********* free pointer  ************/
     int    next;
};

struct probe_statistic_struct {
    int cut_offs;       /* statistic of chains */
    int single_node;
    int short_node;
    int long_node;
    int longs;
    int shorts;
    int shorts2;
    int chars;

};

extern struct probe_struct_global   {
    GBDATA  *gb_main;       /* ARBDB interface */
    GBDATA  *gb_species_data;
    GBDATA  *gb_extended_data;
    char    *alignment_name;
    GB_HASH *namehash;      /* name to int */

    struct probe_input_data *data;  /* the internal database */
    char    *ecoli;         /* the ecoli sequenz */
    void    *bi_ecoli;

    int data_count;
    int max_size;       /* maximum sequence len */
    long    char_count;     /* number of all 'acgtuACGTU' */

    int mismatches;     /* chain handle in match */
    double  wmismatches;
    int N_mismatches;
    int w_N_mismatches[PT_POS_TREE_HEIGHT+PT_POS_SECURITY+1];

    int reversed;       /* tell the matcher whether probe is
                        reversed */

    double  *pos_to_weight;         /* position to weight */
    char complement[256];       /* complement */

    int deep;           /* for probe matching */
    int height;
    int length;
    int apos;

    int sort_by;

    char    *probe;         /* probe design + chains */
    char    *main_probe;

    char    *server_name;       /* name of this server */
    aisc_com *link;
    T_PT_MAIN main;
    struct Hs_struct *com_so;           /* the communication socket */
    POS_TREE *pt;
    PTM2    *ptmain;
    probe_statistic_struct  stat;
} psg;

#ifdef DEVEL_IDP
struct gene_struct {
  char gene_name[9];
  char full_name[128];
};

extern list<gene_struct*> names_list;
extern GBDATA *map_ptr_idp;
extern int gene_flag;
#endif

#define PT_base_string_counter_eof(str) (*(unsigned char *)(str) == 255)
