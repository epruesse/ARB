//  =============================================================== //
//                                                                  //
//    File      : aw_color_groups.cxx                               //
//    Purpose   :                                                   //
//    Time-stamp: <Thu Jun/07/2001 11:00 MET Coder@ReallySoft.de>   //
//                                                                  //
//    Coded by Ralf Westram (coder@reallysoft.de) in June 2001      //
//    Institute of Microbiology (Technical University Munich)       //
//    http://www.arb-home.de/                                       //
//                                                                  //
//  =============================================================== //

#ifndef AW_COLOR_GROUPS_HXX
#define AW_COLOR_GROUPS_HXX

// color groups define special colors used to colorize
// groups of genes or species (or any other items)
//
// to activate color_group-colors you need the following:
//
//  - call AW_manage_GC with define_color_groups set to true;
//    This defines extra GC's for color_groups (named color_group_xxx)
//    If such GCs are defined the normal properties window (AW_create_gc_window)
//    automatically gets a sub-window call 'Color-groups'
//
//  - add a label in your GC enum (at the end before BLA_GC_MAX)
//    like this:
//
//          BLA_GC_FIRST_COLOR_GROUP,
//          BLA_GC_MAX = BLA_GC_FIRST_COLOR_GROUP + AW_COLOR_GROUPS
//
//


#define AW_COLOR_GROUP_PREFIX     "color_group_"
#define AW_COLOR_GROUP_PREFIX_LEN 12
#define AW_COLOR_GROUP_ENTRY      "ARB_color"
#define AW_COLOR_GROUPS           12

void AW_set_color_group(GBDATA *gbd, long color_group);
long AW_find_color_group(GBDATA *gbd);

#else
#error aw_color_groups.hxx included twice
#endif // AW_COLOR_GROUPS_HXX

