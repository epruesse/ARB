// ============================================================ //
//                                                              //
//   File      : ad_colorset.h                                  //
//   Purpose   : item-colors and colorsets                      //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AD_COLORSET_H
#define AD_COLORSET_H

#ifndef ARBDB_BASE_H
#include "arbdb_base.h"
#endif
#ifndef ARB_STRARRAY_H
#include <arb_strarray.h>
#endif

#define GB_COLORGROUP_ENTRY "ARB_color"

long     GBT_get_color_group(GBDATA *gb_item);
GB_ERROR GBT_set_color_group(GBDATA *gb_item, long color_group);

GBDATA *GBT_colorset_root(GBDATA *gb_main, const char *itemname);
void    GBT_get_colorset_names(ConstStrArray& colorsetNames, GBDATA *gb_colorset_root);
GBDATA *GBT_find_colorset(GBDATA *gb_colorset_root, const char *name);
GBDATA *GBT_find_or_create_colorset(GBDATA *gb_colorset_root, const char *name);

GB_ERROR GBT_load_colorset(GBDATA *gb_colorset, ConstStrArray& colorsetDefs);
GB_ERROR GBT_save_colorset(GBDATA *gb_colorset, CharPtrArray& colorsetDefs);

#else
#error ad_colorset.h included twice
#endif // AD_COLORSET_H
