#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"


GB_ERROR SQ_calc_seq_quality(GBDATA *gbmain, const char *tree_name) {

  GBT_TREE *tree = GBT_read_tree(gbmain, tree_name, sizeof(GBT_TREE));
  if (!tree) /*...*/
  GB_ERROR error = GBT_link_tree(tree,gbmain,GB_FALSE);
  if (error) /*...*/


}
