// =============================================================== //
//                                                                 //
//   File      : ed4_extern.hxx                                    //
//   Purpose   : external interface (e.g. for secedit)             //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ED4_EXTERN_HXX
#define ED4_EXTERN_HXX

#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif

// define GCs
// (used by plugins to sync colors)

#define ED4_G_FIRST_FONT ED4_G_STANDARD // ED4_G_SEQUENCES
#define ED4_G_LAST_FONT  ED4_G_HELIX

typedef enum
{
    ED4_G_STANDARD,

    ED4_G_SEQUENCES,
    ED4_G_HELIX,

    ED4_G_COLOR_2,
    ED4_G_COLOR_3,
    ED4_G_COLOR_4,
    ED4_G_COLOR_5,
    ED4_G_COLOR_6,
    ED4_G_COLOR_7,
    ED4_G_COLOR_8,
    ED4_G_COLOR_9,

    ED4_G_CBACK_0,  // Ranges for column statistics
    ED4_G_CBACK_1,
    ED4_G_CBACK_2,
    ED4_G_CBACK_3,
    ED4_G_CBACK_4,
    ED4_G_CBACK_5,
    ED4_G_CBACK_6,
    ED4_G_CBACK_7,
    ED4_G_CBACK_8,
    ED4_G_CBACK_9,

    ED4_G_SBACK_0,  // Background for search
    ED4_G_SBACK_1,
    ED4_G_SBACK_2,
    ED4_G_SBACK_3,
    ED4_G_SBACK_4,
    ED4_G_SBACK_5,
    ED4_G_SBACK_6,
    ED4_G_SBACK_7,
    ED4_G_SBACK_8,
    ED4_G_MBACK,    // Mismatches

    ED4_G_CURSOR,               // Color of cursor
    ED4_G_MARKED,               // Background for marked species
    ED4_G_SELECTED,             // Background for selected species

    ED4_G_FIRST_COLOR_GROUP,   // Background colors for colored species
    ED4_G_LAST_COLOR_GROUP = ED4_G_FIRST_COLOR_GROUP+AW_COLOR_GROUPS-1,

    ED4_G_DRAG = ED4_G_LAST_COLOR_GROUP+1               // must be last
} ED4_gc;


#define ED4_AWAR_SEARCH_RESULT_CHANGED "tmp/search/result_changed" // triggered when search result changes
#define AWAR_EDIT_DIRECTION            "tmp/edit4/edit_direction"

#else
#error ed4_extern.hxx included twice
#endif // ED4_EXTERN_HXX
