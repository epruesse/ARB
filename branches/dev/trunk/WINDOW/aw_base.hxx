// ================================================================= //
//                                                                   //
//   File      : aw_base.hxx                                         //
//   Purpose   : forward declare some gui types                      //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AW_BASE_HXX
#define AW_BASE_HXX

class AW_root;
class AW_window;

typedef long AW_CL; // client data (casted from pointer or value)

class           GBDATA;
typedef GBDATA *AW_default;

typedef double AW_pos;

struct AW_world { AW_pos t, b, l, r; };
struct AW_rectangle { int t, b, l, r; };

typedef int AW_font;

typedef long      AW_bitset;
typedef AW_bitset AW_active;     // bits to activate/inactivate buttons
typedef float     AW_grey_level; // <0 don't fill  0.0 white 1.0 black

typedef struct _WidgetRec *Widget;

typedef const char *AWAR;

typedef enum {
    AW_NONE    = 0,
    AW_BIT     = 1,
    AW_BYTE    = 2,
    AW_INT     = 3,
    AW_FLOAT   = 4,
    AW_POINTER = 5,
    AW_BITS    = 6,
    // 7 is unused
    AW_BYTES   = 8,
    AW_INTS    = 9,
    AW_FLOATS  = 10,
    AW_STRING  = 12,
    // 13 is reserved (GB_STRING_SHRT)
    // 14 is unused
    AW_DB      = 15,

    // keep AW_VARIABLE_TYPE consistent with GB_TYPES
    // see ../ARBDB/arbdb.h@sync_GB_TYPES_AW_VARIABLE_TYPE

    AW_TYPE_MAX = 16
} AW_VARIABLE_TYPE;

typedef enum {
    AW_INFO_AREA,
    AW_MIDDLE_AREA,
    AW_BOTTOM_AREA,
    AW_MAX_AREA
} AW_area;

typedef enum {
    AW_WINDOW_BG,
    AW_WINDOW_FG,
    AW_WINDOW_C1,
    AW_WINDOW_C2,
    AW_WINDOW_C3,
    AW_WINDOW_DRAG,
    AW_DATA_BG,
    AW_COLOR_MAX
} AW_color;


AW_default get_AW_ROOT_DEFAULT();
#define AW_ROOT_DEFAULT get_AW_ROOT_DEFAULT()

#else
#error aw_base.hxx included twice
#endif // AW_BASE_HXX
