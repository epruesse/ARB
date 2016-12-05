// ============================================================ //
//                                                              //
//   File      : AP_TreeColors.hxx                              //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AP_TREECOLORS_HXX
#define AP_TREECOLORS_HXX

#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif

enum {
    AWT_GC_CURSOR = 0,
    AWT_GC_BRANCH_REMARK,
    AWT_GC_BOOTSTRAP,
    AWT_GC_BOOTSTRAP_LIMITED,
    AWT_GC_IRS_GROUP_BOX,
    AWT_GC_ALL_MARKED,
    AWT_GC_SOME_MARKED,
    AWT_GC_NONE_MARKED,
    AWT_GC_ONLY_ZOMBIES,

    // for probe coloring

    AWT_GC_BLACK,
    AWT_GC_WHITE,

    AWT_GC_RED,
    AWT_GC_GREEN,
    AWT_GC_BLUE,

    AWT_GC_ORANGE,     // red+yellow // #FFD206
    AWT_GC_AQUAMARIN,  // green+cyan
    AWT_GC_PURPLE,     // blue+magenta

    AWT_GC_YELLOW,     // red+green
    AWT_GC_CYAN,       // green+blue
    AWT_GC_MAGENTA,    // blue+red

    AWT_GC_LAWNGREEN, // green+yellow
    AWT_GC_SKYBLUE,    // blue+cyan
    AWT_GC_PINK,       // red+magenta

    // end probe coloring

    AWT_GC_FIRST_COLOR_GROUP,
    AWT_GC_FIRST_RANGE_COLOR = AWT_GC_FIRST_COLOR_GROUP + AW_COLOR_GROUPS,
    AWT_GC_MAX               = AWT_GC_FIRST_RANGE_COLOR + AW_RANGE_COLORS,
};


#else
#error AP_TreeColors.hxx included twice
#endif // AP_TREECOLORS_HXX
