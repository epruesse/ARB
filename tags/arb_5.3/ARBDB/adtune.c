#include <adtune.h>

const int GBCM_BUFFER               = 8192;        /* The communication buffer size */
const int GB_REMOTE_HASH_SIZE       = 0x40000;     /* The initial hash size in any client to find the database entry in the server */
const int GBM_MAX_UNINDEXED_ENTRIES = 64;          /* The maximum number fields with the same key which are not put together in one memory segment */

const int GB_TOTAL_CACHE_SIZE   = 25000000;         /* Initial cache size in bytes */
const int GB_MAX_CACHED_ENTRIES = 8192;             /* maximum number of cached items (Maximum 32000) */

const int GB_MAX_QUICK_SAVE_INDEX = 99;       /* Maximum extension-index of quick saves (Maximum 99) */
const int GB_MAX_QUICK_SAVES      = 10;       /* maximum number of quick saves */

const int GB_MAX_LOCAL_SEARCH = 256;           /* Maximum number of children before doing a search in the database server */

const int GBTUM_SHORT_STRING_SIZE = 128;       /* the maximum strlen which is stored in short string format */
const int GB_HUFFMAN_MIN_SIZE     = 128;       /* min length, before huffmann code is used */
const int GB_RUNLENGTH_MIN_SIZE   = 64;        /* min length, before runlength code is used */

const int GB_MAX_REDO_CNT  = 10;                   /* maximum number of redos */
const int GB_MAX_UNDO_CNT  = 100;                  /* maximum number of undos */
const int GB_MAX_UNDO_SIZE = 1000000;              /* total bytes used for undo*/
