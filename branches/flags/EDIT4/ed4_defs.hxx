#ifndef ED4_DEFS_HXX
#define ED4_DEFS_HXX

#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif
#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#if defined(DEBUG)
// #define TRACE_REFRESH
// #define TRACE_JUMPS
#endif

#define e4_assert(bed) arb_assert(bed)

class  ED4_root;
class  ED4_database;
struct GBS_strstruct;

typedef int ED4_COORDINATE;

enum ED4_EDITMODE {
    AD_ALIGN,   // add & remove of . possible (default)
    AD_NOWRITE, // no edits allowed
    AD_NOWRITE_IF_COMPRESSED,
    AD_REPLACE, // all edits (overwrite)
    AD_INSERT   // all edits (insert)
};


// global variables
extern GBDATA   *GLOBAL_gb_main;
extern ED4_root *ED4_ROOT;

extern int TERMINALHEIGHT;      // this variable replaces the define
extern int INFO_TERM_TEXT_YOFFSET;
extern int SEQ_TERM_TEXT_YOFFSET;

extern int  MAXSEQUENCECHARACTERLENGTH;             // greatest # of characters in a sequence string terminal
extern int  MAXSPECIESWIDTH;
extern int  MAXINFOWIDTH;
extern long ED4_counter;

// use ED4_init_notFoundMessage and ED4_finish_and_show_notFoundMessage to
// modify the following elements
#define MAX_SHOWN_MISSING_SPECIES 200U              // limit no of missing species/data printed into not_found_message
extern size_t         not_found_counter;            // nr of species which haven't been found
extern GBS_strstruct *not_found_message;            // error message containing (some) missing/unloadable species

extern long         max_seq_terminal_length;        // global maximum of sequence terminal length
extern ED4_EDITMODE awar_edit_mode;
extern long         awar_edit_rightward;            // 0 = leftward, 1 = rightward
extern bool         move_cursor;                    // only needed for editing in consensus
extern bool         DRAW;

// globally used defines and flags

#define INFINITE -1

#if defined(ARB_MOTIF)
#define SLIDER_OFFSET 5
#else // ARB_GTK
#define SLIDER_OFFSET 0
#endif

// these are recommended for accessing ED4_extension:
#define X_POS       0
#define Y_POS       1
#define WIDTH       0
#define HEIGHT      1

// size of some display elements:
#define BRACKETWIDTH    11
#define SPACERHEIGHT    5
#define SPACERNOCONSENSUSHEIGHT 15 // height of spacer at top of group (used if consensus is hidden and group is folded)
#define SEQUENCEINFOSIZE 50
#define TREETERMINALSIZE 100

#define COLUMN_STAT_ROW_HEIGHT(font_height)     (2.2*(font_height)) // each row contains 2 sub-rows (plus some xtra space)
#define COLUMN_STAT_ROWS            4

#define MAX_POSSIBLE_SEQ_LENGTH     100000000

#define MAXWINDOWS 5

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

#define NAME_BUFFERSIZE 100 // size of buffer used to build terminal/manager IDs

typedef long ED4_index;

#define ED4_SCROLL_OVERLAP 20   // 15 Pixels overlap


enum ED4_level { // has to contain bit values
    ED4_L_INVALID         = -1U,
    ED4_L_NO_LEVEL        = 0,
    ED4_L_ROOT            = 0x1,
    ED4_L_DEVICE          = 0x2,
    ED4_L_AREA            = 0x4,
    ED4_L_MULTI_SPECIES   = 0x8,
    ED4_L_SPECIES         = 0x10,
    ED4_L_MULTI_SEQUENCE  = 0x20,
    ED4_L_SEQUENCE        = 0x40,
    ED4_L_TREE            = 0x80,
    ED4_L_SPECIES_NAME    = 0x100,
    ED4_L_SEQUENCE_INFO   = 0x200,
    ED4_L_SEQUENCE_STRING = 0x400,
    ED4_L_ORF             = 0x800,
    ED4_L_SPACER          = 0x1000,
    ED4_L_LINE            = 0x2000,
    ED4_L_MULTI_NAME      = 0x4000,
    ED4_L_NAME_MANAGER    = 0x8000,
    ED4_L_GROUP           = 0x10000,
    ED4_L_ROOTGROUP       = 0x20000,
    ED4_L_BRACKET         = 0x40000,
    ED4_L_PURE_TEXT       = 0x80000,
    ED4_L_COL_STAT        = 0x100000,
};

enum ED4_datamode {
    ED4_D_SPECIES,
    ED4_D_EXTENDED
};


enum ED4_movemode {
    ED4_M_NO_MOVE    = 0,
    ED4_M_HORIZONTAL = 1,
    ED4_M_VERTICAL   = 2,
    ED4_M_FREE       = 4
};

enum ED4_returncode {
    ED4_R_OK         = 0,
    ED4_R_WARNING    = 1,
    ED4_R_IMPOSSIBLE = 2,
    ED4_R_ERROR      = 4,
    ED4_R_BREAK      = 8,
    ED4_R_ALL        = 0x7fffffff
};

enum ED4_AREA_LEVEL {
    ED4_A_TOP_AREA,
    ED4_A_MIDDLE_AREA,
    ED4_A_BOTTOM_AREA,
    ED4_A_ERROR
};

enum ED4_properties {
    ED4_P_NO_PROP            = 0,
    ED4_P_IS_MANAGER         = 1,
    ED4_P_IS_TERMINAL        = 2,
    ED4_P_HORIZONTAL         = 4,
    ED4_P_VERTICAL           = 8,
    ED4_P_TMP                = 16,
    ED4_P_SELECTABLE         = 32,
    ED4_P_DRAGABLE           = 64,
    ED4_P_MOVABLE            = 128,
    ED4_P_IS_HANDLE          = 256,
    ED4_P_CURSOR_ALLOWED     = 512,
    //  ED4_P_               = 1024,
    //  ED4_P_               = 2048,
    ED4_P_IS_FOLDED          = 4096,            // Flag whether group is folded or not
    ED4_P_CONSENSUS_RELEVANT = 8192,            // contains information relevant for consensus
    ED4_P_ALIGNMENT_DATA     = 16384,           // contains aligned data (also SAIs)
    ED4_P_ALL                = 0x7fffffff
};

enum ED4_cursor_move {
    ED4_C_UP,
    ED4_C_DOWN,
    ED4_C_LEFT,
    ED4_C_RIGHT,
    ED4_C_NONE
};

enum ED4_update_flag {
    ED4_U_UP = 0,
    ED4_U_UP_DOWN
};

enum ED4_border_flag {
    ED4_B_BORDER,
    ED4_B_INDENT,
    ED4_B_BOTTOM_AREA
};

enum ED4_consensus {
    ED4_K_ADD,
    ED4_K_SUB
};

enum ED4_alignment {
    ED4_A_DEFAULT,
    ED4_A_CONTAINER
};

enum ED4_aa_seq_flag {
    ED4_AA_FRWD_1,
    ED4_AA_FRWD_2,
    ED4_AA_FRWD_3,
    ED4_AA_RVRS_1,
    ED4_AA_RVRS_2,
    ED4_AA_RVRS_3,
    ED4_AA_DB_FIELD
};

enum ED4_direction {
    ED4_D_VERTICAL,
    ED4_D_HORIZONTAL,
    ED4_D_ALL_DIRECTION,
};


class ED4_base;
class ED4_terminal;
class ED4_species_name_terminal;

struct ED4_move_info
{
    ED4_base     *object;                // object to be moved
    AW_pos        end_x, end_y;          // position to move to
    ED4_movemode  mode;                  // move mode
    ED4_level     preferred_parent;      // level to move to
};

struct ED4_selection_entry {
    AW_pos drag_off_x, drag_off_y;
    AW_pos drag_old_x, drag_old_y;
    AW_pos actual_width, actual_height;
    int    old_event_y;

    ED4_species_name_terminal *object;
};



struct ED4_extension // contains info about graphical properties
{
    AW_pos  position[2];        // upper left corner (pos_x, pos_y) in relation to parent
    // !!! WARNING: This is a hack!!! Every time you change 'position' you HAVE TO call ED4_base::touch_world_cache() !!!

    AW_pos    size[2];          // width and height
    ED4_index y_folded;         // remember old position of consensus when folding group

    AW::Vector get_size() const { return AW::Vector(size[WIDTH], size[HEIGHT]); }
    AW::Vector get_parent_offset() const { return AW::Vector(position[X_POS], position[Y_POS]); }

    bool set_size_does_change(int idx, AW_pos value) {
        // returns true if changed
        e4_assert(idx == WIDTH || idx == HEIGHT);
        if (size[idx] != value) {
            size[idx] = value;
            return true;
        }
        return false;
    }

#if defined(IMPLEMENT_DUMP)
    void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

struct ED4_scroll_picture {
    bool scroll;
    long old_x;
    long old_y;

    ED4_scroll_picture() : scroll(false), old_x(0), old_y(0) {}
};

enum ED4_CursorJumpType {
    ED4_JUMP_CENTERED,          // centers the cursor
    ED4_JUMP_KEEP_VISIBLE,      // keeps the cursor visible (keeps a fixed margin)
    ED4_JUMP_KEEP_POSITION,     // paint cursor at previous position
};

struct ED4_work_info
{
    AW_event  event;
    GBDATA   *gb_data;
    char     *string;           // pointer to consensus; only if editing the consensus
    long      char_position;    // screen position after cursor

    bool rightward; // contains direction of editing (0 = leftward, 1 = rightward)

    ED4_EDITMODE mode;

    bool    is_sequence;        // ==1 -> special handling for sequences
    bool    cannot_handle;      // if TRUE then cannot edit

    ED4_CursorJumpType cursor_jump;
    bool    refresh_needed;

    long    out_seq_position;   // sequence position (after editing)

    char     *out_string;                           // nur falls new malloc

    int     repeat_count;       // only for keystrokes: contains # of times key should be repeated

    ED4_terminal *working_terminal; // this contains the terminal
};


struct ED4_update_info // if you add new elements, please ensure to initialize them in ED4_base::ED4_base()
{
    unsigned int resize : 1;
    unsigned int refresh : 1;
    unsigned int clear_at_refresh : 1;
    unsigned int linked_to_folding_line : 1;
    unsigned int linked_to_scrolled_rectangle : 1;
    unsigned int refresh_horizontal_scrolling : 1;
    unsigned int refresh_vertical_scrolling : 1;
    unsigned int delete_requested : 1;
    unsigned int update_requested : 1;

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

        window_width,               // of whole window (top and middle area and ... )
        window_height,              // of whole window (top and middle area and ... )

        window_upper_clip_point,        // absolute coordinate of upper visible clipping point in middle area
        window_lower_clip_point,        // absolute coordinate of lower visible clipping point in middle area
        window_left_clip_point,         // absolute coordinate of left  visible clipping point in top and middle area
        window_right_clip_point;        // absolute coordinate of right visible clipping point in top and middle area

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

#else
#error ed4_defs.hxx included twice
#endif
