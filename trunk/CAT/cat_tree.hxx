#ifndef cat_tree_hxx_included
#define cat_tree_hxx_included

// #ifndef GB_INCLUDED
typedef struct gb_data_base_type GBDATA;
// #endif

enum {				// COLOR SCHEME (orable)
	T2J_COLOR_UNKNOWN = 0,
	T2J_COLOR_YES = 1,
	T2J_COLOR_NO = 2,
	T2J_COLOR_BOTH = 3
};


enum CAT_FIELDS {
	CAT_FIELD_NAME,		// internal id
	CAT_FIELD_FULL_NAME,	// fullname
	CAT_FIELD_GROUP_NAME,	// name of internal nodes
	CAT_FIELD_ACC,		// accession number
	CAT_FIELD_STRAIN,	// first strain word
	CAT_FIELD_MAX
};

#define CAT_INIT_STRING_FILE_SIZE 10000

typedef enum {
	CAT_TRUE = 1,
	CAT_FALSE = 0
}	CAT_BOOL;

typedef long CAT_node_id;

typedef enum {
	CAT_NUMBERING_LEAFS_ONLY,
	CAT_NUMBERING_LEVELS,
	CAT_NUMBERING_MAX
} CAT_NUMBERING;

class CAT_node {
	public:
	CAT_node_id		father;		// == 0 if root
	CAT_node_id		leftson;	// == 0 if leaf
	CAT_node_id		rightson;
	long		nleafs;			// count all the leafs
	long		numbering[CAT_NUMBERING_MAX];	// all leafes are numbered from left to right
	long		deep;
	unsigned char	color_in;		// imported from ARB
	unsigned char 	is_grouped_in;		// imported from ARB !!! 0 expanded  1 grouped
	unsigned char	color;	
	unsigned char 	is_grouped;		// 0 expanded  1 grouped
	unsigned char	branch_length_byte;
        float 		branch_length_float;
	long		field_offsets[CAT_FIELD_MAX];	// offset in 
				// CAT_tree->data
	char		*user_data;
};

struct CAT_tree {

	public:
	/********* READ ONLY *************/
	char	data[4];			// array of strings, holds
						// all the words
	int nnodes;
	CAT_node nodes[1];			// array of nodes (note 1 is
						// replaced by nnodes !!!
						// data is organized as nlr
};

CAT_tree *new_CAT_tree(int size);


inline CAT_tree *load_CAT_tree(const char *path){
	return (CAT_tree *)GB_map_file(path,1);
}

typedef struct gbt_tree_struct  GBT_TREE;


const char *create_and_save_CAT_tree(GBDATA *gb_main, const char *tree_name, const char *path);
const char *create_and_save_CAT_tree(GBT_TREE *tree, const char *path);




#endif
