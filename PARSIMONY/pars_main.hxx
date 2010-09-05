#define MIN_SEQUENCE_LENGTH     20

extern struct NT_global {
    AW_root *awr;
    AWT_graphic_tree *tree;
} *GLOBAL_NT;

AWT_graphic_tree *PARS_generate_tree(AW_root *root);

