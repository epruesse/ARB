#include "arbdb.h"
#include "arbdbt.h"


GB_ERROR SQ_calc_seq_quality(GBDATA *gbmain, const char *tree_name) {

    GBT_TREE *tree = GBT_read_tree(gbmain,tree_name, sizeof(GBT_TREE) );
    if (!tree) return GB_get_error();
    GB_ERROR error = GBT_link_tree(tree,gbmain,GB_FALSE);
    if (error) return error;


}
