#ifndef arbdbt_h_included
#define arbdbt_h_included

#define GBT_SPECIES_INDEX_SIZE 10000
#define GBT_SAI_INDEX_SIZE 1000

typedef float GBT_LEN;

#define GBT_TREE_ELEMENTS(type)                     \
	GB_BOOL			is_leaf;                        \
	GB_BOOL			tree_is_one_piece_of_memory;    \
	type			*father,*leftson,*rightson;     \
	GBT_LEN			leftlen,rightlen;               \
	GBDATA			*gb_node;                       \
	char			*name;                          \
	char			*remark_branch 

#define CLEAR_GBT_TREE_ELEMENTS(tree_obj_ptr)           \
(tree_obj_ptr)->is_leaf = GB_FALSE;                     \
(tree_obj_ptr)->tree_is_one_piece_of_memory = GB_FALSE; \
(tree_obj_ptr)->father = 0;                             \
(tree_obj_ptr)->leftson = 0;                            \
(tree_obj_ptr)->rightson = 0;                           \
(tree_obj_ptr)->leftlen = 0;                            \
(tree_obj_ptr)->rightlen = 0;                           \
(tree_obj_ptr)->gb_node = 0;                            \
(tree_obj_ptr)->name = 0;                               \
(tree_obj_ptr)->remark_branch = 0

#define AWAR_SPECIES_DATA       "species_data"
#define AWAR_SPECIES	        "species"

#define AWAR_SAI_DATA	        "extended_data"
#define AWAR_SAI	            "extended"

#define AWAR_CONFIG_DATA	    "configuration_data"
#define AWAR_CONFIG		        "configuration"

#define AWAR_TREE_DATA		    "tree_data"

#ifdef FAKE_VIRTUAL_TABLE_POINTER
typedef FAKE_VIRTUAL_TABLE_POINTER virtualTable;
#endif

typedef struct gbt_tree_struct {
#ifdef FAKE_VIRTUAL_TABLE_POINTER    
    virtualTable *dummy_virtual; /* simulate pointer to virtual-table used in AP_tree */ 
#endif    
    GBT_TREE_ELEMENTS(struct gbt_tree_struct);
} GBT_TREE;

typedef struct gb_seq_compr_tree {
#ifdef FAKE_VIRTUAL_TABLE_POINTER    
    virtualTable *dummy_virtual; /* simulate pointer to virtual-table used in AP_tree */ 
#endif    
    GBT_TREE_ELEMENTS(struct gb_seq_compr_tree);
    int	index;			/* either master/sequence index */
} GB_CTREE;				/* @@@ OLI */

#define	GBT_TREE_AWAR_SRT       " =:\n=:*=tree_*1:tree_tree_*=tree_*1"
#define	GBT_ALI_AWAR_SRT        " =:\n=:*=ali_*1:ali_ali_*=ali_*1"

#define P_(s) s

#if defined(__GNUG__) || defined(__cplusplus) 
extern "C" {
# include <ad_t_prot.h>    
# ifdef GBL_INCLUDED
#  include <ad_t_lpro.h>
# endif
} 
#else 
# include <ad_t_prot.h>
# ifdef GBL_INCLUDED
#  include <ad_t_lpro.h>
# endif /* GBL_INCLUDED */
#endif /* defined(__GNUG__) || defined(__cplusplus) */

#undef P_ 
#endif /*arbdbt_h_included*/