#ifdef arbdb_h_included
#error Please do not include "arbdb.h" yourself when also including "adlocal.h"
#endif


#ifndef GBL_INCLUDED
#define GBL_INCLUDED

/* ================ Test memory @@@===================== */
#if defined(DEBUG)
#define MEMORY_TEST 1
#else
#define MEMORY_TEST 0
#endif

#ifdef __cplusplus
inline void ad_use(int, ...) {}
#else
void ad_use(int dummy, ...);
#endif

#define ADUSE(variable) ad_use(1, variable)

#if (MEMORY_TEST==1)

#define gbb_put_memblk(block,size)
#define gbb_get_memblk(size)            (char*)(GB_calloc(1,size))
#define gbm_get_mem(size,index)         (char*)(GB_calloc(1,size))
#define gbm_free_mem(block,size,index)  do { free(block); ADUSE(index); } while(0)
#define fread(block,size,nelem,stream) (memset(block,0,size*nelem), (fread)(block,size,nelem,stream))

#endif

/* ================ Assert's ======================== */

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define ad_assert(bed) arb_assert(bed)

/* ================================================== */

#define GBTUM_MAGIC_NUMBER        0x17488400
#define GBTUM_MAGIC_NUMBER_FILTER 0xffffff00

#define GBTUM_MAGIC_REVERSED 0x00844817

#define GB_MAX_PROJECTS          256
#define GBTUM_COMPRESS_TREE_SIZE 32
#define GB_MAX_USERS             4
#define GB_MAX_KEYS              0x1000000 /* 24 bit see flags also */

#define CROSS_BUFFER_DIFF 20
#define SIZOFINTERN       10

#define GB_SYSTEM_FOLDER   "__SYSTEM__"
#define GB_SYSTEM_KEY_DATA "@key_data"

#define GB_MAX_MAPPED_FILES 10


/********** RELATIVE ADRESSING *************/

#if (MEMORY_TEST==1)
typedef void                            *GB_REL_ADD;
typedef char                            *GB_REL_STRING;
typedef struct gb_data_base_type        *GB_REL_GBDATA;
typedef struct gb_data_base_type2       *GB_REL_CONTAINER;
typedef struct gb_header_list_struct    *GB_REL_HLS;
typedef struct gb_if_entries            *GB_REL_IFES;
typedef struct gb_index_files_struct    *GB_REL_IFS;
typedef struct gb_if_entries            **GB_REL_PIFES;
#else
typedef long    GB_REL_ADD;     /* relative adress */
typedef long    GB_REL_STRING;      /* relative adress */
typedef long    GB_REL_GBDATA;      /* relative adress */
typedef long    GB_REL_CONTAINER;       /* relative adress */
typedef long    GB_REL_HLS;     /* relative adress */
typedef long    GB_REL_IFES;        /* relative adress */
typedef long    GB_REL_IFS;     /* relative adress */
typedef long    GB_REL_PIFES;       /* relative adress */
#endif

typedef short   GB_MAIN_IDX;        /* random-index */
typedef struct gbs_hash_struct GB_HASH;

#define GB_MAIN_ARRAY_SIZE 4096

/********************************************/

enum {
    GBM_CB_INDEX     = -1,
    GBM_HASH_INDEX   = -2,
    GBM_HEADER_INDEX = -3,
    GBM_UNDO         = -4,
    GBM_DICT_INDEX   = -5,
    GBM_USER_INDEX   = -6
};

typedef long gb_bool;
struct gb_map_header;

#define _GB_UNDO_TYPE_DEFINED
typedef enum {          /* Warning: Same typedef in arbdb.h */
    GB_UNDO_NONE,
    GB_UNDO_KILL,   /* no undo and delete all old undos */
    GB_UNDO_UNDO,
    GB_UNDO_REDO,
    GB_UNDO_UNDO_REDO
} GB_UNDO_TYPE;

typedef enum gb_changed_types {
    gb_not_changed       = 0,
    gb_son_changed       = 2,
    gb_changed           = 4,
    gb_created           = 5,
    gb_deleted           = 6,
    gb_deleted_in_master = 7
} GB_CHANGED;

enum gb_open_types {
    gb_open_all             = 0,
    gb_open_read_only_all   = 16,
    gb_open_read_only_small = 17
};

typedef enum gb_call_back_type {
    GB_CB_DELETE      = 1,
    GB_CB_CHANGED     = 2,
    GB_CB_SON_CREATED = 4,
    GB_CB_ALL         = 7
} GB_CB_TYPE;

/* #define GBDATA struct gb_data_base_type */
typedef struct gb_data_base_type GBDATA;

/*#define GB_CB void (*func)(GBDATA *,int *clientdata, GB_CB_TYPE gbtype)*/

#ifdef __cplusplus
extern "C" {
#endif

    typedef void (*GB_CB)(GBDATA *,int *clientdata, GB_CB_TYPE gbtype);

#ifdef __cplusplus
}
#endif


typedef int GBQUARK;

/********************* compress ******************/
enum GB_COMPRESSION_TYPES {
    GB_COMPRESSION_NONE       = 0,
    GB_COMPRESSION_RUNLENGTH  = 1,
    GB_COMPRESSION_HUFFMANN   = 2,
    GB_COMPRESSION_DICTIONARY = 4,
    GB_COMPRESSION_SEQUENCE   = 8,
    GB_COMPRESSION_SORTBYTES  = 16,
    GB_COMPRESSION_BITS       = 32,
    GB_COMPRESSION_LAST       = 128
};
typedef int GB_NINT;        /* Network byte order int */

typedef struct {
    int            words;
    int            textlen;
    unsigned char *text;
    GB_NINT       *offsets;     /* in network byte order */
    GB_NINT       *resort;      /* in network byte order */

} GB_DICTIONARY;

extern int gb_convert_type_2_compression_flags[];
extern int gb_convert_type_2_sizeof[];
extern int gb_convert_type_2_appendix_size[];
extern int gb_verbose_mode;

struct gb_compress_tree {
    char    leave;
    struct gb_compress_tree *son[2];
};

enum gb_compress_list_commands {
    gb_cs_ok   = 0,
    gb_cs_sub  = 1,
    gb_cs_id   = 2,
    gb_cs_end  = 3,
    gb_cd_node = 4
};

struct gb_compress_list {
    enum gb_compress_list_commands command;

    int  value;
    int  bitcnt;
    int  bits;
    int  mask;
    long count;

    struct gb_compress_list *son[2];
};

/********************* main ******************/

struct gb_user_struct {
    char *username;
    int   userid;
    int   userbit;
    int   nusers;               /* number of clients of this user */
};

struct gb_project_struct {
    char *projectname;
    int   projectid;

    struct gb_export_project {
        struct gb_export_project *next;
        char                     *username;
    }   *export_;

    long export_2_users;        /* bits , one bit for each logged in user */
};

struct gb_data_base_type;
struct gb_key_struct {
    char *key;

    long nref;
    long next_free_key;
    long nref_last_saved;

    struct gb_data_base_type *gb_key; /* for fast access and dynamic loading */
    struct gb_data_base_type *gb_master_ali; /* Pointer to the master container */
    int                       gb_key_disabled; /* There will never be a gb_key */
    int                       compression_mask; /* maximum compression for this type */
    GB_DICTIONARY            *dictionary; /* optional dictionary */

};

struct gb_quick_save_struct {
    char *quick_save_disabled;  /* GB_BOOL if set, than text decsribes reason*/
    int   last_index;
};

struct gb_cache_entry_struct {
    struct gb_data_base_type *gbd;

    long  prev;
    long  next;
    char *data;
    long  clock;
    int   sizeof_data;
};

struct gb_cache_struct {
    struct gb_cache_entry_struct *entries;

    long firstfree_entry;
    long newest_entry;
    long oldest_entry;
    long sum_data_size;
    long max_data_size;
    long max_entries;
};



/** root structure for each database */
struct gbcmc_comm;
struct g_b_undo_mgr_struct;
struct gb_callback_list;
struct gb_data_base_type2;

typedef struct gb_main_type {
    int transaction;
    int aborted_transaction;
    int local_mode;
    int client_transaction_socket;

    struct gbcmc_comm         *c_link;
    void                      *server_data;
    struct gb_data_base_type2 *dummy_father;
    struct gb_data_base_type2 *data;
    struct gb_data_base_type  *gb_key_data;
    char                      *path;
    enum gb_open_types         opentype;
    char                      *disabled_path;
    int                        allow_corrupt_file_recovery;

    struct gb_quick_save_struct qs;
    struct gb_cache_struct      cache;
    int                         compression_mask;

    int                   keycnt; /* first non used key */
    long                  sizeofkeys; /* malloc size */
    long                  first_free_key; /* index of first gap */
    struct gb_key_struct *keys;
    GB_HASH              *key_2_index_hash;
    long                  key_clock; /* trans. nr. of last change */

    char         *keys_new[256];
    unsigned int  last_updated;
    long          last_saved_time;
    long          last_saved_transaction;
    long          last_main_saved_transaction;

    GB_UNDO_TYPE        requested_undo_type;
    GB_UNDO_TYPE        undo_type;
    struct g_b_undo_mgr_struct *undo;

    char         *dates[256];
    unsigned int  security_level;
    int           old_security_level;
    int           pushed_security_level;
    long          clock;
    long          remote_hash;

    GB_HASH *command_hash;
    GB_HASH *resolve_link_hash;
    GB_HASH *table_hash;

    struct gb_callback_list *cbl;
    struct gb_callback_list *cbl_last;

    struct gb_callback_list *cbld;
    struct gb_callback_list *cbld_last;

    struct gb_user_struct    *users[GB_MAX_USERS]; /* user 0 is server */
    struct gb_project_struct *projects[GB_MAX_PROJECTS]; /* projects */
    struct gb_user_struct    *this_user;
    struct gb_project_struct *this_project;

} GB_MAIN_TYPE;

extern GB_MAIN_TYPE *gb_main_array[];


typedef enum trans_type {   ARB_COMMIT, ARB_ABORT, ARB_TRANS } ARB_TRANS_TYPE;


/** global data structure that is valid for all databases*/
struct gb_local_data {
    char *buffer2;
    long  bufsize2;
    char *buffer;
    long  bufsize;
    char *write_buffer;
    char *write_ptr;
    long  write_bufsize;
    long  write_free;
    int   iamclient;
    int   search_system_folder;

    struct gb_compress_tree *bituncompress;
    struct gb_compress_list *bitcompress;

    long bc_size;
    long gb_compress_keys_count;
    long gb_compress_keys_level;

    struct gb_main_type *gb_compress_keys_main;
    ARB_TRANS_TYPE       running_client_transaction;
    struct {
        struct gb_data_base_type *gb_main;
    }   gbl;
};

extern struct gb_local_data *gb_local;
extern const unsigned long crctab[];

struct gb_header_flags {
    unsigned int flags:GB_MAX_USERS; /* public */
    unsigned int key_quark:24;  /* == 0 -> invalid */
    unsigned int changed:3;
    unsigned int ever_changed:1; /* is this element ever changed */
} ;

struct gb_header_list_struct {  /* public fast flags */
    struct gb_header_flags flags;

    GB_REL_GBDATA rel_hl_gbd;   /* Typ: (struct gb_data_base_type *) */
    /* pointer to data
       if 0 & !key_index -> free data
       if 0 & key_index -> data only in server */
};

struct gb_data_list {

    GB_REL_HLS rel_header;  /* Typ: (struct gb_header_list_struct *) */

    int headermemsize;
    int size;       /* number of valid items */
    int nheader;    /* size + deleted items */
};
struct gb_flag_types {      /* public flags, abort possible */
    unsigned int type:4;
    unsigned int security_delete:3;
    unsigned int security_write:3;
    unsigned int security_read:3;
    unsigned int compressed_data:1;
    unsigned int unused:1;      /* last bit saved */
    unsigned int user_flags:8;
    unsigned int temporary:1;   /* ==1 -> dont save entry */
    unsigned int saved_flags:8;
};
struct gb_flag_types2 {     /* private flags, abortable */
    /* uncritic section undoable */
    unsigned int last_updated:8;
    unsigned int usr_ref:7;     /* for user access  */
    /* critic section, do not update any below */
    unsigned int folded_container:1;
    unsigned int update_in_server:1; /* already informed */
    unsigned int extern_data:1; /* data ref. by pntr*/
    unsigned int header_changed:1; /* used by container*/
    unsigned int gbm_index:8;   /* memory section*/
    unsigned int tisa_index:1;  /* this should be indexed */
    unsigned int is_indexed:1;  /* this db. field is indexed*/
};

struct gb_flag_types3 {     /* user and project flags (public)
                               not abortable !!! */
    unsigned int project:8;
    unsigned int unused:24;
};

struct gb_if_entries
{
    GB_REL_IFES   rel_ie_next;  /* Typ: (struct gb_if_entries *)    */
    GB_REL_GBDATA rel_ie_gbd;   /* Typ: (struct gb_data_base_type *)    */
};

/** hash index to speed up GB_find(x,x,x,down_2_level) ***/

struct gb_index_files_struct
{
    GB_REL_IFS  rel_if_next;    /* Typ: (struct gb_index_files_struct *) */

    GBQUARK key;
    long    hash_table_size;
    long    nr_of_elements;

    GB_REL_PIFES  rel_entries;  /* Typ: (struct gb_if_entries **) */
};

struct gb_db_extended;

typedef struct gb_data_base_type2 { /* public area */
    long                   server_id;
    GB_REL_CONTAINER       rel_father; /* Typ: (struct gb_data_base_type2 *) */
    struct gb_db_extended *ext;
    long                   index;
    struct gb_flag_types   flags;
    struct gb_flag_types2  flags2; /* privat area */
    struct gb_flag_types3  flags3;
    struct gb_data_list    d;

    long index_of_touched_one_son;  /* index of modified son
                                       in case of a single mod. son
                                       -1 more than one (or one with ind = 0)
                                       0    no son
                                       >0   index */
    long header_update_date;

    GB_MAIN_IDX main_idx;       /* Typ: (GB_MAIN_TYPE *) */
    GB_REL_IFS  rel_ifs;        /* Typ: (struct gb_index_files_struct *) */

} GBCONTAINER;


struct gb_extern_data2 {
    char *data;                 /* Typ: (char *) */
    long  memsize;
    long  size;
};

struct GB_INTern_strings2 {
    char          data[SIZOFINTERN];
    unsigned char memsize;
    unsigned char size;
};

struct GB_INTern2 {
    char data[SIZOFINTERN];
};

union gb_data_base_type_union2 {
    struct GB_INTern_strings2 istr;
    struct GB_INTern2         in;
    struct gb_extern_data2    ex;
};

struct gb_transaction_save {
    struct gb_flag_types           flags;
    struct gb_flag_types2          flags2;
    union gb_data_base_type_union2 info;
    short                          refcount; /* number of references to this object */
};

struct gb_callback
{
    struct gb_callback     *next;
    GB_CB                   func;
    enum gb_call_back_type  type;
    int                    *clientdata;
    int                     priority;
};

struct gb_callback_list {
    struct gb_callback_list    *next;
    GB_CB                       func;
    struct gb_transaction_save *old;
    GB_CB_TYPE                  type;
    struct gb_data_base_type   *gbd;
    int                        *clientdata;
};

/*#undef GBDATA*/

enum gb_undo_commands {
    _GBCMC_UNDOCOM_REQUEST_NOUNDO_KILL,
    _GBCMC_UNDOCOM_REQUEST_NOUNDO,
    _GBCMC_UNDOCOM_REQUEST_UNDO,
    _GBCMC_UNDOCOM_INFO_UNDO,
    _GBCMC_UNDOCOM_INFO_REDO,
    _GBCMC_UNDOCOM_UNDO,
    _GBCMC_UNDOCOM_REDO,

    _GBCMC_UNDOCOM_SET_MEM = 10000      /* Minimum */
};

enum gb_scan_quicks_types {
    GB_SCAN_NO_QUICK,
    GB_SCAN_NEW_QUICK,
    GB_SCAN_OLD_QUICK
};

struct gb_scandir {
    int                       highest_quick_index;
    int                       newest_quick_index;
    unsigned long             date_of_quick_file;
    enum gb_scan_quicks_types type; /* xxx.arb.quick? or xxx.a?? */
};

#define GBCM_SERVER_OK_WAIT 3
#define GBCM_SERVER_ABORTED 2
#define GBCM_SERVER_FAULT   1
#define GBCM_SERVER_OK      0

#ifndef arbdb_h_included
# include "arbdb.h"
#endif

#include "adtune.h"
#include "adlmacros.h"

/* command interpreter */

#define GBL_MAX_ARGUMENTS 50

typedef struct gbl_struct {
    char *str;
} GBL;

typedef struct gbl_command_arguments_struct {
    GBDATA     *gb_ref;         /* a database entry on which the command is applied (may be species, gene, experiment, group and maybe more) */
    const char *default_tree_name; /* if we have a default tree, its name is specified here (0 otherwise) */
    const char *command;        /* the name of the current command */

    int cinput; const GBL *vinput; /* input streams */
    int cparam; const GBL *vparam; /* parameter "streams" */
    int *coutput; GBL **voutput; /* the output streams */

} GBL_command_arguments;

#ifdef __cplusplus
extern "C" {
#endif

    typedef GB_ERROR (*GBL_COMMAND)(GBL_command_arguments *args);
    /* typedef GB_ERROR (*GBL_COMMAND)(GBDATA *gb_ref, char *com, GBL_client_data *cd,
       int argcinput, GBL *argvinput, int argcparam,GBL *argvparam, int *argcout, GBL **argvout); */

#ifdef __cplusplus
}
#endif

struct GBL_command_table {
    const char *command_identifier;
    GBL_COMMAND function;
};

#endif
