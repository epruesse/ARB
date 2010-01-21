// =========================================================== //
//                                                             //
//   File      : awt_nds.hxx                                   //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_NDS_HXX
#define AWT_NDS_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define AWAR_SELECT_ACISRT     "tmp/acisrt/select"
#define AWAR_SELECT_ACISRT_PRE "tmp/acisrt/select_pre"

const char *make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int format, GBT_TREE *species, const char *tree_name);
void        make_node_text_init(GBDATA *gb_main);

# ifndef _NO_AWT_NDS_WINDOW_FUNCTIONS

AW_window *AWT_create_nds_window(AW_root *aw_root, AW_CL cgb_main);
void       create_nds_vars(AW_root *aw_root, AW_default awdef, GBDATA *gb_main);
void       AWT_create_select_srtaci_window(AW_window *aww, AW_CL awar_acisrt, AW_CL awar_short);

# endif



#else
#error awt_nds.hxx included twice
#endif // AWT_NDS_HXX
