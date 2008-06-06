#define MIN_SEQUENCE_LENGTH     20

extern struct NT_global {
    AW_root *awr;
    AWT_graphic_tree *tree;
} * nt;

AWT_graphic *PARS_generate_tree(AW_root *root);

extern AW_CL pars_global_filter;
extern long global_combineCount;
