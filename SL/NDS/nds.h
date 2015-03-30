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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

struct GBT_TREE;

#define AWAR_SELECT_ACISRT     "tmp/acisrt/select"
#define AWAR_SELECT_ACISRT_PRE "tmp/acisrt/select_pre"

enum NDS_Type {
    NDS_OUTPUT_LEAFTEXT        = 0,   // compress info (no tabbing, separate single fields by comma, completely skip empty fields)
    NDS_OUTPUT_SPACE_PADDED    = 1,   // format info (using spaces)
    NDS_OUTPUT_TAB_SEPARATED   = 2,   // format info (using 1 tab per column - for easy import into star-calc, excel, etc. ).
                                      // (also used by AWT_graphic_tree::show_nds_list for non-tree-display of species in ARB_NTREE)
    NDS_OUTPUT_COMMA_SEPARATED = 3,   // like NDS_OUTPUT_TAB_SEPARATED, but using commas
};

void        make_node_text_init(GBDATA *gb_main);
const char *make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, NDS_Type format, GBT_TREE *species, const char *tree_name);

char *NDS_mask_nonprintable_chars(char *inStr);

AW_window *AWT_create_nds_window(AW_root *aw_root, GBDATA *gb_main);
void       create_nds_vars(AW_root *aw_root, AW_default awdef, GBDATA *gb_main);
void       AWT_popup_select_srtaci_window(AW_window *aww, const char *acisrt_awarname);

#else
#error awt_nds.hxx included twice
#endif // AWT_NDS_HXX
