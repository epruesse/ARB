#ifndef ARBDB_H
#define ARBDB_H

#ifndef _STDIO_H
#error "arbdb.h needs stdio.h included"
#endif

#define NOT4PERL
/* function definitions starting with NOT4PERL are not included into the ARB-perl-interface */

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define gb_assert(bed) arb_assert(bed)

typedef const char *GB_CSTR;        /* local memory mgrment */
typedef const char *GB_ERROR;       /* memory management is controlled by the ARBDB lib */
typedef char    *GB_CPNTR;      /* points into a piece of mem */

#define GB_PATH_MAX 1024
#define GBS_GLOBAL_STRING_SIZE 64000

#define GBS_SPECIES_HASH_SIZE 100000L

// #ifndef GB_INCLUDED
// #define GB_INCLUDED

#define GB_CORE *(long *)0 =0
#define GB_KEY_LEN_MAX  64 /* max. length of a key (a whole key path may be longer) */
#define GB_KEY_LEN_MIN  2

/* ---------------------------------------- need some stuff if adlocal.h is not included */

#ifndef GBL_INCLUDED
typedef enum gb_call_back_type {
    GB_CB_DELETE = 1,
    GB_CB_CHANGED = 2,
    GB_CB_SON_CREATED = 4,
    GB_CB_ALL  = 7
} GB_CB_TYPE;

typedef struct gb_data_base_type GBDATA;
typedef void (*GB_CB)(GBDATA *,int *clientdata, GB_CB_TYPE gbtype);
/*#define  GB_CB void (*)(GBDATA *,int *clientdata, GB_CB_TYPE gbtype)*/

#endif /*GBL_INCLUDED*/


#define GBUSE(a) a=a

typedef char *MALLOC_T;

typedef unsigned long GB_ULONG;

/* 4 Bytes */
typedef unsigned int GB_UINT4;
typedef const unsigned int GB_CUINT4;
typedef const float GB_CFLOAT;

/* ---------------------------------------- if adlocal.h not included */

#ifndef GBL_INCLUDED

#if defined(DEBUG)
#define MEMORY_TEST 1
#else
#define MEMORY_TEST 0
#endif


#if (MEMORY_TEST==1)

typedef char                        *GB_REL_STRING;
typedef struct gb_data_base_type    *GB_REL_GBDATA;
typedef struct gb_data_base_type2   *GB_REL_CONTAINER;

#else

typedef long    GB_REL_STRING;      /* relative adress */
typedef long    GB_REL_GBDATA;      /* relative adress */
typedef long    GB_REL_CONTAINER;       /* relative adress */

#endif /*MEMORY_TEST==1*/

typedef void GB_MAIN_TYPE;
typedef struct gbs_hash_struct GB_HASH;

struct gb_flag_types {      /* public flags */
    unsigned int        type:4;
    unsigned int        security_delete:3;
    unsigned int        security_write:3;
    unsigned int        security_read:3;
    unsigned int        compressed_data: 1;
    unsigned int        unused: 1;  /* last bit saved */
    unsigned int        user_flags:8;
    unsigned int        temporary:1;    /* ==1 -> dont save entry */
    unsigned int        saved_flags:8;
};

struct gb_flag_types2 {     /* private flags */
    unsigned int        intern0: 16;
    unsigned int        intern1: 16;
};



/********************* public ******************/


typedef int GBQUARK;

/*********** Undo ***********/
typedef enum {
    GB_UNDO_NONE,   /* no undo */
    GB_UNDO_KILL,   /* no undo and delete all old undos */
    GB_UNDO_UNDO,   /* normal undo -> deleted all redoes */
    GB_UNDO_REDO,   /* moves to UNDO_REDO */
    GB_UNDO_UNDO_REDO   /* internal makes undo redoable */
}   GB_UNDO_TYPE;


struct gb_transaction_save;

#endif /*GBL_INCLUDED*/

enum {GB_FALSE = 0 , GB_TRUE = 1 };

typedef char GB_BOOL;
typedef int GB_COMPRESSION_MASK;

typedef enum gb_key_types {
    GB_NONE     = 0,
    GB_BIT      = 1,
    GB_BYTE     = 2,
    GB_INT      = 3,
    GB_FLOAT    = 4,
    GB_BITS     = 6,
    GB_BYTES    = 8,
    GB_INTS     = 9,
    GB_FLOATS   = 10,
    GB_LINK     = 11,
    GB_STRING   = 12,
    GB_STRING_SHRT  = 13, /* not working and not used anywhere */
    GB_DB       = 15,
    GB_TYPE_MAX = 16
} GB_TYPES;

enum gb_search_enum {
    GB_FIND = 0,
    GB_CREATE_CONTAINER = GB_DB /* create other types: use GB_TYPES */
};

#define GB_TYPE_2_CHAR "-bcif-B-CIFlSS-%"

enum gb_search_types {
    this_level = 1,
    down_level = 2,
    down_2_level = 4,
    search_next = 8 /* search after item : this_level,down_level*/
};
/********************* public end ******************/

/********************* client/server ******************/
struct gbcmc_comm {
    int socket;
    char    *unix_name;
    char    *error;
};


/********************* database ******************/
#define SIZOFINTERN 10

struct gb_extern_data {

    GB_REL_STRING rel_data; /* Typ: (char *) */
    long    memsize;
    long    size;
};

struct GB_INTern_strings {
    char    data[SIZOFINTERN];
    unsigned char memsize;
    unsigned char size;
};

struct GB_INTern {
    char    data[SIZOFINTERN];
};

union gb_data_base_type_union {
    long    i;
    struct GB_INTern_strings istr;
    struct GB_INTern    in;
    struct gb_extern_data   ex;
};

struct gb_callback;
struct gb_db_extended {
    long    creation_date;
    long    update_date;
    struct gb_callback      *callback;
    struct gb_transaction_save  *old;
};

struct gb_data_base_type {
    long    server_id;
    GB_REL_CONTAINER rel_father;    /* Typ: (struct gb_data_base_type2 *) */
    struct gb_db_extended   *ext;
    long    index;
    struct gb_flag_types    flags;
    struct gb_flag_types2   flags2;
    union gb_data_base_type_union info;
    int cache_index;    /* should be a member of gb_db_extended */
};

/*********** Alignment ***********/

typedef enum
{
    GB_AT_UNKNOWN,
    GB_AT_RNA,      /* Nucleotide sequence (U) */
    GB_AT_DNA,      /* Nucleotide sequence (T) */
    GB_AT_AA,       /* AminoAcid */

} GB_alignment_type;

/*********** Sort ***********/


#ifdef __cplusplus
extern "C" {
#endif

    typedef long (*GB_MERGE_SORT)(void *, void *, char *cd);
    /*#define GB_MERGE_SORT long (*)(void *, void *, char *cd )*/

#ifdef __cplusplus
}
#endif

extern long GB_NOVICE;

struct GBL_command_table;

typedef struct GBDATA_SET_STRUCT {
    long nitems;
    long malloced_items;
    GBDATA **items;
} GBDATA_SET;

#ifdef __cplusplus
extern "C" {
#endif

    typedef GBDATA* (*GB_Link_Follower)(GBDATA *GB_root,GBDATA *GB_elem,const char *link);
    typedef long (*gbs_hash_sort_func_type)(const char *key0,long val0,const char *key1,long val1);
    typedef long (*gb_compare_two_items_type)(void *p0,void *p1,char *cd);
    typedef long (*gb_hash_loop_type)(const char *key, long val);
    typedef long (*gb_hash_loop_type2)(const char *key, long val, void *parameter);
    typedef void (*gb_warning_func_type)(const char *msg);
    typedef void (*gb_information_func_type)(const char *msg);
    typedef int (*gb_status_func_type)(double val);
    typedef int (*gb_status_func2_type)(const char *val);
    typedef void (*gb_error_handler_type)(const char *msg);

#ifdef __cplusplus
}
#endif

#if defined(__GNUG__) || defined(__cplusplus)
extern "C" {
#endif

# define P_(s) s

#include <ad_prot.h>
#ifdef GBL_INCLUDED
#include <ad_lpro.h>
#endif /*GBL_INCLUDED*/

#undef P_

#if defined(__GNUG__) || defined(__cplusplus)
}
#endif

// #endif /*GB_INCLUDED*/

#define GB_INLINE

#ifdef __cplusplus

struct gb_data_base_type2;
class GB_transaction {
    GBDATA *ta_main;
    bool    aborted;
public:
    GB_transaction(GBDATA *gb_main);
    ~GB_transaction();

    void abort() { aborted = true; }
};

int GB_info(struct gb_data_base_type2 *gbd);

# ifndef NO_INLINE
#  undef GB_INLINE
#  define GB_INLINE inline
# endif /*NO_INLINE*/
#endif /*__cplusplus*/

#else
#error "arbdb.h included twice"
#endif /*ARBDB_H*/
