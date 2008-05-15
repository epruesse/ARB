
typedef enum
{
    ED4_BT_NOBLOCK,		// nothing is marked
    ED4_BT_LINEBLOCK,		// all species with EDIT4-marks are marked
    ED4_BT_COLUMNBLOCK,		// range is marked
    ED4_BT_MODIFIED_COLUMNBLOCK // was columnblock, but single species were removed/added
    
} ED4_blocktype;

typedef enum {
    ED4_BO_UPPER_CASE,
    ED4_BO_LOWER_CASE,
    ED4_BO_REVERSE,
    ED4_BO_COMPLEMENT,
    ED4_BO_REVERSE_COMPLEMENT,
    ED4_BO_UNALIGN,
    ED4_BO_UNALIGN_RIGHT,
    ED4_BO_SHIFT_LEFT,
    ED4_BO_SHIFT_RIGHT
    
} ED4_blockoperation_type;

ED4_blocktype ED4_getBlocktype();
void ED4_setBlocktype(ED4_blocktype bt);
void ED4_toggle_block_type();
void ED4_correctBlocktypeAfterSelection();
void ED4_setColumnblockCorner(AW_event *event, ED4_sequence_terminal *seq_term);
int ED4_get_selected_range(ED4_terminal *term, int *first_column, int *last_column); 


typedef char *(*ED4_blockoperation)(const char *sequence_data, int len, int repeat, int *new_len, GB_ERROR *error);
void ED4_with_whole_block(ED4_blockoperation block_operation, int repeat);
void ED4_perform_block_operation(ED4_blockoperation_type type);

AW_window *ED4_create_replace_window(AW_root *root); 
