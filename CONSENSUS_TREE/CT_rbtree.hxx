typedef struct {
    GBT_LEN len;
    GBT_TREE *node;
    int percent;
} RB_INFO;


void rb_init(char **names);

GBT_TREE *rb_gettree(NT_NODE *tree);
