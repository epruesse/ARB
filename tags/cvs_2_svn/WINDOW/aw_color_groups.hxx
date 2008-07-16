//  =============================================================== //
//                                                                  //
//    File      : aw_color_groups.cxx                               //
//    Purpose   :                                                   //
//    Time-stamp: <Tue Dec/21/2004 16:56 MET Coder@ReallySoft.de>   //
//                                                                  //
//    Coded by Ralf Westram (coder@reallysoft.de) in June 2001      //
//    Institute of Microbiology (Technical University Munich)       //
//    http://www.arb-home.de/                                       //
//                                                                  //
//  =============================================================== //

#ifndef AW_COLOR_GROUPS_HXX
#define AW_COLOR_GROUPS_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif


// color groups define special colors used to colorize
// groups of genes or species (or any other items)
//
// to activate color_group-colors you need the following:
//
//  - call AW_init_color_group_defaults with current program name
//
//  - call AW_manage_GC with define_color_groups set to true;
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
//  - use AW_set_color_group to set and AW_find_color_group to read the
//    color of a species/gene/experiment/organism/etc.

#define AWAR_COLOR_GROUPS_PREFIX "color_groups"
#define AWAR_COLOR_GROUPS_USE    AWAR_COLOR_GROUPS_PREFIX "/use" // int : whether to use the colors in display or not

#define AW_COLOR_GROUP_PREFIX     "color_group_"
#define AW_COLOR_GROUP_PREFIX_LEN 12
#define AW_COLOR_GROUP_NAME_LEN   (AW_COLOR_GROUP_PREFIX_LEN+2)

#define AW_COLOR_GROUP_ENTRY "ARB_color"
#define AW_COLOR_GROUPS      12

GB_ERROR  AW_set_color_group(GBDATA *gbd, long color_group);
long      AW_find_color_group(GBDATA *gbd, AW_BOOL ignore_usage_flag = false);
char     *AW_get_color_group_name(AW_root *awr, int color_group);

void AW_init_color_group_defaults(const char *for_program);

#else
#error aw_color_groups.hxx included twice
#endif // AW_COLOR_GROUPS_HXX

