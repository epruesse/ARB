
#include <PT_com.h>
#include <arbdb.h>

#include <list.h>
#include <set.h>
#include <PT_server.h>

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define pt_assert(bed) arb_assert(bed)

#if defined(DEBUG)
#define PTM_DEBUG
#endif // DEBUG

typedef unsigned long ulong;
typedef unsigned int  uint;
typedef unsigned char uchar;

#define  min(a, b) ((a) < (b)) ? (a) : (b)
#define  max(a, b) ((a) > (b)) ? (a) : (b)

#define PT_CORE *(int *)0 = 0;

#define PT_MAX_MATCHES     100000
// #define PT_MAX_IDENTS      10000
#define PT_POS_TREE_HEIGHT 20
#define PT_POS_SECURITY    10
#define MIN_PROBE_LENGTH   6
#define PT_NAME_HASH_SIZE  10000

enum PT_MATCH_TYPE {
    PT_MATCH_TYPE_INTEGER           = 0,
    PT_MATCH_TYPE_WEIGHTED_PLUS_POS = 1,
    PT_MATCH_TYPE_WEIGHTED          = -1
};



#define MATCHANSWER 50  /* private msg type: match result answer */
#define CREATEANSWER 51 /* private msg type: create result answer */
#define FINDANSWER 52   /* private msg type: find result answer */

extern ulong physical_memory;
struct Hs_struct;
extern char *pt_error_buffer;

typedef enum type_types_type {
    t_int    = 1,
    t_string = 0,
    t_float  = 2
} type_types;

typedef enum PT_bases_enum  {
    PT_QU = 0,
    PT_N  = 1,
    PT_A,
    PT_C,
    PT_G,
    PT_T,
    PT_B_MAX
} PT_BASES;

/*  POS TREE */
typedef enum enum_PT_NODE_TYPE {
    PT_NT_LEAF        = 0,
    PT_NT_CHAIN       = 1,
    PT_NT_NODE        = 2,
    PT_NT_SINGLE_NODE = 3,      /* stage 3 */
    PT_NT_SAVED       = 3,      /* stage 1+2 */
    PT_NT_UNDEF       = 4
} PT_NODE_TYPE;

typedef struct POS_TREE_struct {
    uchar flags;
    char  data;
} POS_TREE;

typedef struct PTMM_struct {
    char *base;
    int   stage1;
    int   stage2;
    int   stage3;
    int   mode;
} PTM2;



/* Probe search */


struct probe_statistic {
    int match_count;        /* Counter for matches */
    double  rel_match_count;    /* match_count / (seq_len - probe_len + 1) */
};

struct probe_input_data {    /* every taxas own data */
    /******* name and sequence *********/
    char   *data;
    long    checksum;
    int     size;
    char   *name;
    char   *fullname;
    GBDATA *gbd;

    /********* probe design ************/
    int is_group;   /* -1:  nevermind
                        0:  no group
                        1:  group */

    /* probe design (match) */
    PT_probematch *match;       /* best hit for PT_new_design */

    /********* find family  ************/
    struct probe_statistic stat;

    /********* free pointer  ************/
    int next;
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
    GBDATA  *gb_main;           /* ARBDB interface */
    GBDATA  *gb_species_data;
    GBDATA  *gb_extended_data;
    char    *alignment_name;
    GB_HASH *namehash;          /* name to int */

    struct probe_input_data *data; /* the internal database */

    char *ecoli;                /* the ecoli sequenz */
    void *bi_ecoli;

    int  data_count;
    int  max_size;              /* maximum sequence len */
    long char_count;            /* number of all 'acgtuACGTU' */

    int    mismatches;          /* chain handle in match */
    double wmismatches;
    int    N_mismatches;
    int    w_N_mismatches[PT_POS_TREE_HEIGHT+PT_POS_SECURITY+1];

    int reversed;       /* tell the matcher whether probe is reversed */

    double *pos_to_weight;      /* position to weight */
    char    complement[256];    /* complement */

    int deep;                   /* for probe matching */
    int height;
    int length;
    int apos;

    int sort_by;

    char *probe;                /* probe design + chains */
    char *main_probe;

    char             *server_name; /* name of this server */
    aisc_com         *link;
    T_PT_MAIN         main;
    struct Hs_struct *com_so;   /* the communication socket */
    POS_TREE         *pt;
    PTM2             *ptmain;

    probe_statistic_struct stat;

} psg;

class gene_struct {
    char       *gene_name;
    const char *arb_species_name; // pointers into 'gene_name'
    const char *arb_gene_name;

    void init(const char *gene_name_, const char *arb_species_name_, const char *arb_gene_name_) {
        int gene_name_len        = strlen(gene_name_);
        int arb_species_name_len = strlen(arb_species_name_);
        int arb_gene_name_len    = strlen(arb_gene_name_);

        int fulllen      = gene_name_len+1+arb_species_name_len+1+arb_gene_name_len+1;
        gene_name        = new char[fulllen];
        strcpy(gene_name, gene_name_);
        arb_species_name = gene_name+(gene_name_len+1);
        strcpy((char*)arb_species_name, arb_species_name_);
        arb_gene_name    = arb_species_name+(arb_species_name_len+1);
        strcpy((char*)arb_gene_name, arb_gene_name_);
    }

public:
    gene_struct(const char *gene_name_, const char *arb_species_name_, const char *arb_gene_name_) {
        init(gene_name_, arb_species_name_, arb_gene_name_);
    }
    gene_struct(const gene_struct& other) {
        if (&other != this) {
            init(other.get_internal_gene_name(), other.get_arb_species_name(), other.get_arb_gene_name());
        }
    }
    gene_struct& operator = (const gene_struct& other) {
        if (&other != this) {
            delete [] gene_name;
            init(other.get_internal_gene_name(), other.get_arb_species_name(), other.get_arb_gene_name());
        }
        return *this;
    }

    ~gene_struct() {
        delete [] gene_name;
    }

    const char *get_internal_gene_name() const { return gene_name; }
    const char *get_arb_species_name() const { return arb_species_name; }
    const char *get_arb_gene_name() const { return arb_gene_name; }
};

extern int gene_flag;           // if 'gene_flag' == 1 -> we are a gene pt server

struct ltByArbName {
    bool operator()(const gene_struct *gs1, const gene_struct *gs2) const {
        int cmp           = strcmp(gs1->get_arb_species_name(), gs2->get_arb_species_name());
        if (cmp == 0) { cmp = strcmp(gs1->get_arb_gene_name(), gs2->get_arb_gene_name()); }
        return cmp<0;
    }
};
struct ltByInternalName {
    bool operator()(const gene_struct *gs1, const gene_struct *gs2) const {
        int cmp = strcmp(gs1->get_internal_gene_name(), gs2->get_internal_gene_name());
        return cmp<0;
    }
};

typedef list<gene_struct>                          gene_struct_list;
typedef set<const gene_struct *, ltByInternalName> gene_struct_index_internal;
typedef set<const gene_struct *, ltByArbName>      gene_struct_index_arb;

extern gene_struct_list           all_gene_structs; // stores all gene_structs
extern gene_struct_index_arb      gene_struct_arb2internal; // sorted by arb speces+gene name
extern gene_struct_index_internal gene_struct_internal2arb; // sorted by internal name

#define PT_base_string_counter_eof(str) (*(unsigned char *)(str) == 255)
