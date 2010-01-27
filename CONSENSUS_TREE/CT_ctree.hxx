#ifndef CT_ctree_hxx_included
#define CT_ctree_hxx_included

void ctree_init(int node_count, char **names);
void insert_ctree(GBT_TREE *tree, int weight);
GBT_TREE *get_ctree();




#endif
