#define GB_RUNLENGTH_SIZE 6

	
extern const int GBCM_BUFFER;					/* The communication buffer size */
extern const int GB_REMOTE_HASH_SIZE;				/* The initial hash size in any client to find the database entry in the server */
extern const int GBM_MAX_UNINDEXED_ENTRIES;			/* The maximum number fields with the same key which are not put together in one memory segment */

extern const int GB_TOTAL_CACHE_SIZE		;		/* Initial cache size in bytes */
extern const int GB_MAX_CACHED_ENTRIES 		;		/* maximum number of cached items (Maximum 32000) */

extern const int GB_MAX_QUICK_SAVE_INDEX 	;		/* Maximum extension-index of quick saves (Maximum 99) */
extern const int GB_MAX_QUICK_SAVES		;		/* maximum number of quick saves */
	
extern const int GB_MAX_LOCAL_SEARCH		;		/* Maximum number of childs before doing a earch in the database server */

extern const int GBTUM_SHORT_STRING_SIZE	;		/* the maximum strlen which is stored in short string format */
extern const int GB_HUFFMAN_MIN_SIZE		;		/* min length, before huffmann code is used */
extern const int GB_RUNLENGTH_MIN_SIZE		;		/* min length, before runlength code is used */

extern const int GB_MAX_REDO_CNT		;		/* maximum number of redos */
extern const int GB_MAX_UNDO_CNT		;		/* maximum number of undos */
extern const int GB_MAX_UNDO_SIZE		;		/* total bytes used for undo*/

