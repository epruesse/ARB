//  =============================================================== //
//                                                                  //
//    File      : aw_color_groups.cxx                               //
//    Purpose   :                                                   //
//                                                                  //
//    Coded by Ralf Westram (coder@reallysoft.de) in June 2001      //
//    Institute of Microbiology (Technical University Munich)       //
//    http://www.arb-home.de/                                       //
//                                                                  //
//  =============================================================== //

#ifndef AW_COLOR_GROUPS_HXX
#define AW_COLOR_GROUPS_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

// ----------------------
//      color groups
//
// color groups define special colors used to colorize
// groups of genes or species (or any other items)
//
// to activate color_group-colors you need the following:
//
//  - call AW_init_color_group_defaults with current program name
//
//  - call AW_manage_GC with define_color_groups set to true
//    This defines extra GC's for color_groups (named color_group_xxx)
//    If such GCs are defined the normal properties window (AW_create_gc_window)
//    automatically gets a sub-window called 'Edit color-groups'
//
//  - add a label in your GC enum (at the end before BLA_GC_MAX)
//    like this:
//
//          BLA_GC_FIRST_COLOR_GROUP,
//          BLA_GC_MAX = BLA_GC_FIRST_COLOR_GROUP + AW_COLOR_GROUPS
//
//  - use
//       GBT_set_color_group() to set and
//       GBT_get_color_group() to read
//    the color of an item.
//
//  - use AW_color_groups_active() to detect whether color groups shall be shown at all.

#define AW_COLOR_GROUP_PREFIX     "color_group_"
#define AW_COLOR_GROUP_PREFIX_LEN 12
#define AW_COLOR_GROUP_NAME_LEN   (AW_COLOR_GROUP_PREFIX_LEN+2)
#define AW_COLOR_GROUPS           12

void  AW_init_color_group_defaults(const char *for_program);
bool  AW_color_groups_active();
const char *AW_get_color_groups_active_awarname();
char *AW_get_color_group_name(AW_root *awr, int color_group);

// ----------------------
//      color ranges
//
// color ranges provide blended colors (e.g. red .. green or black .. white)

#define AW_RANGE_COLORS 4096 // amount of gcs used for color-ranges (4096 = 64*64 = 16*16*16)
// #define AW_RANGE_COLORS 16384
// #define AW_RANGE_COLORS 65536
// #define AW_RANGE_COLORS 262144

// #define AW_RANGE_COLORS 1048576 // test 1 million gcs (uses several Gb of memory; in arb_ntree and XOrg)

#else
#error aw_color_groups.hxx included twice
#endif // AW_COLOR_GROUPS_HXX

