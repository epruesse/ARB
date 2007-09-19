#ifndef ED4_defs
#define ED4_defs

#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif

//needed prototype classes
class ED4_root;
class ED4_database;

#ifndef HAVE_BOOL
typedef char bool;
#endif

typedef const char  ED4_ERROR;
typedef int     ED4_COORDINATE;

typedef enum ad_edit_modus {
    AD_ALIGN,   // add & remove of . possible (default)
    AD_NOWRITE, // no edits allowed
    AD_NOWRITE_IF_COMPRESSED,
    AD_REPLACE, // all edits (overwrite)
    AD_INSERT   // all edits (insert)
} ED4_EDITMODI;


//global variables
extern GBDATA       *gb_main;
extern ED4_root     *ED4_ROOT;
extern ED4_database *main_db;

extern int TERMINALHEIGHT;      // this variable replaces the define
extern int INFO_TERM_TEXT_YOFFSET;
extern int SEQ_TERM_TEXT_YOFFSET;

extern int           MAXSEQUENCECHARACTERLENGTH; // greatest # of characters in a sequence string terminal
extern int           MAXSPECIESWIDTH;
extern int           MAXINFOWIDTH;
extern int           MARGIN;    // sets margin for cursor moves in characters
extern long          ED4_counter;
extern long          all_found; // nr of species which haven't been found
extern long          species_read; // nr of species read; important during loading
extern void         *not_found_message;
extern long          max_seq_terminal_length; // global maximum of sequence terminal length
extern ED4_EDITMODI  awar_edit_modus;
extern long          awar_edit_direction;
extern bool          move_cursor; //only needed for editing in consensus
extern bool          DRAW;
extern bool          last_window_reached; //only needed for refreshing all windows

extern double           status_add_count;                   //only needed for loading configuration
extern double           status_total_count;
extern bool         loading;
//extern long           last_used_timestamp;                    // time of last timed callback

// globally used defines and flags

#define INFINITE    -1
#define SLIDER_OFFSET   5
//#define HALFCURSORWIDTH   3

// these are recommended for accessing ED4_extension:
#define X_POS       0
#define Y_POS       1
#define WIDTH       0
#define HEIGHT      1

// size of some display elements:
#define BRACKETWIDTH    10
#define SPACERHEIGHT    5
#define SPACERNOCONSENSUSHEIGHT 15 // height of spacer at top of group (used if consensus is hidden and group is folded)
#define SEQUENCEINFOSIZE 50
#define TREETERMINALSIZE 100

#define COLUMN_STAT_ROW_HEIGHT(font_height)     (2.2*(font_height)) // each row contains 2 sub-rows (plus some xtra space)
#define COLUMN_STAT_ROWS            4

#define MAX_POSSIBLE_SEQ_LENGTH     100000000

#define MAXCHARTABLE    256                     // Maximum of Consensustable
//#define MAXBASESTABLE 7
#define MAXWINDOWS  5
#define MINSPECFORSTATWIN   200

#define AWAR_EDIT_MODE                  "tmp/edit4/edit_mode"
#define AWAR_INSERT_MODE                "tmp/edit4/insert_mode"
#define AWAR_EDIT_SECURITY_LEVEL        "tmp/edit4/security_level"
#define AWAR_EDIT_SECURITY_LEVEL_ALIGN  "tmp/edit4/security_level_align"
#define AWAR_EDIT_SECURITY_LEVEL_CHANGE "tmp/edit4/security_level_change"
#define AWAR_EDIT_CONFIGURATION         "tmp/edit4/configuration"

#define AWAR_EDIT_SEQ_POSITION   "tmp/edit4/jump_to_position%d" // used to create ED4_window.awar_path_for_cursor[]
#define AWAR_EDIT_ECOLI_POSITION "tmp/edit4/cursor_ref_ecoli%d" // used to create ED4_window.awar_path_for_Ecoli[]
#define AWAR_EDIT_BASE_POSITION  "tmp/edit4/cursor_to_basepos%d" // used to create ED4_window.awar_path_for_basePos[]
#define AWAR_EDIT_IUPAC          "tmp/edit4/iupac%d" // used to create ED4_window.awar_path_for_IUPAC[]
#define AWAR_EDIT_HELIXNR        "tmp/edit4/helixnr%d" // used to create ED4_window.awar_path_for_helixNr[]

#define AWAR_FIELD_CHOSEN "tmp/edit4/field_chosen" // used for field selection box

#define AWAR_EDIT_TITLE_MODE       "edit4/title_mode"
#define AWAR_EDIT_HELIX_SPACING    "edit4/helix_add_spacing"
#define AWAR_EDIT_TERMINAL_SPACING "edit4/terminal_add_spacing"

#define CHARACTEROFFSET 5       // spacer-width left of text-terminal
#define CONSENSUS       "Consensusfunktion"

#define ED4_index   long

inline int max(int x, int y)    { return x>y ? x : y; }
inline int min(int x, int y)    { return x<y ? x : y; }
inline int ABS(int x)       { return x<0 ? -x : x; }

#define ED4_SCROLL_OVERLAP 20   // 15 Pixels overlap


typedef enum
{
    ED4_L_NO_LEVEL      = 0x0,
    ED4_L_ROOT          = 0x1,
    ED4_L_DEVICE        = 0x2,
    ED4_L_AREA          = 0x4,
    ED4_L_MULTI_SPECIES     = 0x8,
    ED4_L_SPECIES       = 0x10,
    ED4_L_MULTI_SEQUENCE    = 0x20,
    ED4_L_SEQUENCE      = 0x40,
    ED4_L_TREE          = 0x80,
    ED4_L_SPECIES_NAME      = 0x100,
    ED4_L_SEQUENCE_INFO     = 0x200,                //evtl. aendern fuer Name-Manager und group-manager
    ED4_L_SEQUENCE_STRING   = 0x400,
    ED4_L_AA_SEQUENCE_STRING   = 0x600, //ykadi
    ED4_L_SPACER        = 0x800,
    ED4_L_LINE          = 0x1000,
    ED4_L_MULTI_NAME        = 0x2000,
    ED4_L_NAME_MANAGER      = 0x4000,
    ED4_L_GROUP         = 0x8000,
    ED4_L_BRACKET       = 0x10000,
    ED4_L_PURE_TEXT     = 0x20000,
    ED4_L_COL_STAT      = 0x40000
}   ED4_level;



typedef enum
{
    ED4_D_SPECIES,
    ED4_D_EXTENDED
}   ED4_datamode;


typedef enum
{
    ED4_M_NO_MOVE   = 0,
    ED4_M_HORIZONTAL    = 1,
    ED4_M_VERTICAL  = 2,
    ED4_M_FREE      = 4
}   ED4_movemode;

typedef enum
{
    ED4_R_OK        = 0,
    ED4_R_WARNING   = 1,
    ED4_R_IMPOSSIBLE    = 2,
    ED4_R_ERROR     = 4,
    ED4_R_BREAK     = 8,
    ED4_R_DESASTER  = 16,
//    ED4_R_REMOVE  = 32,
//    ED4_R_LEAVE       = 64,
//    ED4_R_ADD     = 128,
//    ED4_R_REMOVE_ALL  = 256,
    ED4_R_ALL       = 0x7fffffff
}   ED4_returncode;

typedef enum
{
    ED4_A_TOP_AREA,
    ED4_A_MIDDLE_AREA,
    ED4_A_BOTTOM_AREA,
    ED4_A_ERROR
}   ED4_AREA_LEVEL;

typedef enum
{
        ED4_B_LEFT_BUTTON   = 1,
        ED4_B_MIDDLE_BUTTON     = 2,
        ED4_B_RIGHT_BUTTON  = 3
}          ED4_mouse_buttons;

typedef enum
{
    ED4_P_NO_PROP       = 0,
    ED4_P_IS_MANAGER    = 1,
    ED4_P_IS_TERMINAL   = 2,
    ED4_P_HORIZONTAL    = 4,
    ED4_P_VERTICAL      = 8,
    ED4_P_TMP       = 16,
    ED4_P_SELECTABLE    = 32,
    ED4_P_DRAGABLE      = 64,
    ED4_P_MOVABLE       = 128,
    ED4_P_IS_HANDLE     = 256,
    ED4_P_CURSOR_ALLOWED    = 512,
//  ED4_P_SCROLL_HORIZONTAL = 1024,
//  ED4_P_SCROLL_VERTICAL   = 2048,
    ED4_P_IS_FOLDED     = 4096,                 // Flag whether group is folded or not
    ED4_P_CONSENSUS_RELEVANT= 8192, // contains information relevant for consensus
    ED4_P_ALIGNMENT_DATA= 16384, // contains aligned data (also SAIs)
    ED4_P_ALL       = 0x7fffffff
}   ED4_properties;

typedef enum
{
    ED4_C_UP,
    ED4_C_DOWN,
    ED4_C_LEFT,
    ED4_C_RIGHT,
    ED4_C_NONE
}   ED4_cursor_move;

typedef enum
{
    ED4_U_UP    = 0,
    ED4_U_UP_DOWN
}   ED4_update_flag;

typedef enum
{
    ED4_B_BORDER,
    ED4_B_INDENT,
    ED4_B_BOTTOM_AREA
}   ED4_border_flag;

typedef enum
{
    ED4_K_ADD,
    ED4_K_SUB
}   ED4_consensus;

typedef enum
{
    ED4_A_DEFAULT,
    ED4_A_CONTAINER
}   ED4_alignment;

typedef enum
{
    ED4_AA_FRWD_1,
    ED4_AA_FRWD_2,
    ED4_AA_FRWD_3,
    ED4_AA_RVRS_1,
    ED4_AA_RVRS_2,
    ED4_AA_RVRS_3,
    ED4_AA_DB_FIELD
}  ED4_aa_seq_flag;

typedef enum
{
    ED4_D_VERTICAL,
    ED4_D_HORIZONTAL,
    ED4_D_ALL_DIRECTION,
    ED4_D_VERTICAL_ALL
}   ED4_direction;


class ED4_base;
class ED4_terminal;

struct ED4_move_info
{
    ED4_base         *object;            // object to be moved
    AW_pos      end_x, end_y;      // position to move to
    ED4_movemode    mode;              // move mode
    ED4_level           preferred_parent;  // level to move to
};

struct ED4_selection_entry
{
    AW_pos               drag_off_x, drag_off_y;
    AW_pos               drag_old_x, drag_old_y;
    AW_pos               actual_width, actual_height;
    int          old_event_y;
    ED4_terminal     *object;
};



struct ED4_extension // contains info about graphical properties
{
    AW_pos  position[2];        // upper left corner (pos_x, pos_y) in relation to parent
    // !!! WARNING: This is a hack!!! Every time you change 'position' you HAVE TO call ED4_base::touch_world_cache() !!!

    AW_pos    size[2];          // width and height
    ED4_index y_folded;         // remember old position of consensus when folding group

#if defined(IMPLEMENT_DUMP)
    void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

struct ED4_scroll_picture
{
    bool    scroll;
    long    old_x;
    long    old_y;
};


struct ED4_work_info
{
    AW_event    event;
    GBDATA  *gb_data;
    char    *string;        // pointer to consensus; only if editing the consensus
    long    char_position;      // screen position after cursor

    int     direction;      // contains direction of editing (-1 left, +1 right )
    ED4_EDITMODI mode;

    bool    is_sequence;        // ==1 -> special handling for sequences
    bool    cannot_handle;      // if TRUE than cannot edit
    bool    center_cursor;      // if true => cursor gets centered horizontally
    bool    refresh_needed;

    long    out_seq_position;   // sequence position (after editing)

    char    *out_string;        // nur falls new malloc
    char    *error;

    int     repeat_count;       // only for keystrokes: contains # of times key should be repeated

    ED4_terminal *working_terminal; // this contains the terminal 
};


struct ED4_update_info // if you add new elements, please ensure to initialize them in ED4_base::ED4_base()
{
    unsigned int        resize:1;
    unsigned int        refresh:1;
    unsigned int    clear_at_refresh:1;
    unsigned int        linked_to_folding_line:1;
    unsigned int        linked_to_scrolled_rectangle:1;
    unsigned int        refresh_horizontal_scrolling:1;
    unsigned int        refresh_vertical_scrolling:1;
    unsigned int    delete_requested:1;

    void set_clear_at_refresh(int value) {
        clear_at_refresh = value;
    }
    void set_refresh(int value) {
        refresh = value;
    }
    void set_resize(int value) {
        resize = value;
    }

#if defined(IMPLEMENT_DUMP)
    void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

struct ED4_coords
{
    long top_area_x,
    top_area_y,
    top_area_height,

    middle_area_x,
    middle_area_y,

    window_width,               //of whole window (top and middle area and ... )
    window_height,              //of whole window (top and middle area and ... )

    window_upper_clip_point,        //absolute coordinate of upper visible clipping point in middle area
    window_lower_clip_point,        //absolute coordinate of lower visible clipping point in middle area
    window_left_clip_point,         //absolute coordinate of left  visible clipping point in top and middle area
    window_right_clip_point;        //absolute coordinate of right visible clipping point in top and middle area

    void clear() {
    top_area_x = 0;
    top_area_y = 0;
    top_area_height = 0;

    middle_area_x = 0;
    middle_area_y = 0;

    window_width = 0;
    window_height = 0;

    window_upper_clip_point = 0;
    window_lower_clip_point = 0;
    window_left_clip_point = 0;
    window_right_clip_point = 0;
    }
};

#endif
