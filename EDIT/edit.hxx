#define AWAR_CURSER_POS_REF_ECOLI "tmp/edit/curser_ref_ecoli"
#define AWAR_SPECIES_DEST "tmp/edit/species_name_dest"
#define AWAR_LINE_SPACING "edit/line_spacing"
#define AWAR_CENTER_SPACING "edit/center_spacing"
#define AWAR_HELIX_AT_SAIS "edit/helix_at_extendeds"
#define AWAR_EDIT_MODE "tmp/edit/edit_mode"
#define AWAR_EDIT_DIRECTION "tmp/edit/edit_direction"
#define AWAR_EDIT_MULTI_SEQ "tmp/edit/edit_multi"

#define AED_LINE_SPACING ((int)aed_root.line_spacing)
#define AED_CENTER_SPACING ((int)aed_root.center_spacing)
#define AED_TOP_LINE 2

const long AED_MAX_SPECIES=100;


enum {
    AED_GC_NAME,
    AED_GC_SELECTED,
    AED_GC_SEQUENCE,
    AED_GC_HELIX,

    AED_GC_0,                   // @@@
    AED_GC_1,
    AED_GC_2,
    AED_GC_3,
    AED_GC_4,
    AED_GC_5,
    AED_GC_6,
    AED_GC_7,
    AED_GC_8,
    AED_GC_9,

    AED_GC_NAME_DRAG,
    AED_GC_SELECTED_DRAG,
    AED_GC_SEQUENCE_DRAG,
    AED_GC_HELIX_DRAG
};


typedef enum {
    AED_F_ALLWAYS           = 0,
    AED_F_NAME              = 1,
    AED_F_NAME_SELECTED     = 2,
    AED_F_SEQUENCE          = 4,
    AED_F_SEQUENCE_SELECTED = 8,
    AED_F_INFO              = 16,
    AED_F_INFO_SEPARATOR    = 32,
    AED_F_WINFO             = 64,
    AED_F_CURSOR            = 128,
    AED_F_HELIX             = 256,
    AED_F_TEXT_1            = 512,
    AED_F_TEXT_2            = 1024,
    AED_F_TEXT_3            = 2048,
    AED_F_FRAME             = 4096,
    AED_F_ALL               = -1
} AED_filters;


extern class AED_root {
public:
    AD_MAIN *ad_main;
    AW_root *aw_root;

    AED_root(void);

    AW_default db;
    long       center_spacing;
    long       line_spacing;
    long       helix_at_extendeds;
} aed_root;


typedef enum   {
    AED_ALIGN   = 0,
    AED_INSERT  = 1,
    AED_REPLACE = 2
} AED_modi;

class AED_dlist;
class AED_area_entry {
public:

    AD_SPECIES   *ad_species;
    AD_SAI       *ad_extended;
    AD_CONT      *ad_container;
    ADT_SEQUENCE *adt_sequence;
    AD_STAT      *ad_stat;

    bool            is_selected;
    AW_pos          in_line;
    AW_pos          absolut_x;
    AW_pos          absolut_y;
    AED_area_entry *previous;
    AED_area_entry *next;
    AED_dlist      *in_area;

    AED_area_entry() { is_selected = false; previous = NULL; next = NULL; }
    ~AED_area_entry() {};
};

class AED_window;
class AED_dummy { class AED_window *dummy; };

class AED_dlist {
    GB_HASH *hash;
    GBDATA  *gb_main;
    char    *hash_awar;
    int      hash_level;

public:
    int             size;
    AED_area_entry *first;
    AED_area_entry *last;
    AED_area_entry *current;

    AED_dlist();
    ~AED_dlist() {}

    void create_hash(GBDATA *gb_main,const char *awar_suffix);
    void insert_hash(const char *name);
    void remove_hash(const char *name);
    long read_hash(const char *name);
    void optimize_hash(void);

    void remove_entry(AED_area_entry *to_be_removed);
    void append(AED_area_entry *to_be_inserted);
    void insert_after_entry(AED_area_entry *add_after_this, AED_area_entry *to_be_inserted);
    void insert_before_entry(AED_area_entry *add_before_this, AED_area_entry *to_be_inserted);
};

class AED_window;
class AED_dlist_left_side;

class AED_left_side {
public:
    char *text;
    char  text_for_dragging[100];

    void        (*make_text)( class AED_window *aedw, AED_area_entry *area_entry, char *text );

    bool                 is_selected;
    AW_pos               absolut_x;
    AW_pos               absolut_y;
    AED_left_side       *previous;
    AED_left_side       *next;
    AED_dlist_left_side *in_side;

    AED_left_side( void(*f)(class AED_window *aedw,AED_area_entry *area_entry, char *text), const char *string );
    ~AED_left_side();
};


class AED_dlist_left_side {
public:
    int            size;
    AED_left_side *first;
    AED_left_side *last;
    AED_left_side *current;

    AED_dlist_left_side() { size = 0; first = NULL; last = NULL; current = NULL; }
    ~AED_dlist_left_side() {}
    
    void remove_entry( AED_left_side *to_be_removed );
    void append( AED_left_side *to_be_inserted );
    void insert_after_entry( AED_left_side *add_after_this, AED_left_side *to_be_inserted );
    void insert_before_entry( AED_left_side *add_before_this, AED_left_side *to_be_inserted );
};



typedef struct {
    bool clear;
    bool calc_size;
    bool visible_control;
    int  top_indent;
    int  bottom_indent;
    int  left_indent;
    int  slider_pos_horizontal;
    int  slider_pos_vertical;
    int  picture_l;
    int  picture_t;

} AED_area_display_struct;


class AED_window {
public:

    bool            config_window_created;
    AW_window_menu *config_window;

    AED_dlist_left_side *show_dlist_left_side;
    AED_dlist_left_side *hide_dlist_left_side;
    AED_left_side       *selected_entry_of_dlist_left_side;
    bool                 one_entry_dlist_left_side_is_selected;

    AED_root  *root;
    AW_window *aww;
    ADT_ALI   *alignment;

    bool global_focus_use;

    AED_dlist      *area_top;
    AED_dlist      *area_middle;
    AED_dlist      *area_bottom;
    AED_area_entry *selected_area_entry;
    bool            one_area_entry_is_selected;
    bool            selected_area_entry_is_visible;

    int  selected_info_area_text;
    int  info_area_height;
    bool edit_info_area;

    AW_world       size_information;
    int            last_slider_position;
    bool           quickdraw;
    int            quickdraw_left_indent;
    int            quickdraw_right_indent;
    AW_cursor_type cursor_type;
    int            cursor;
    bool           cursor_is_managed;
    bool           drag;
    int            drag_x, drag_y;
    AW_pos         drag_x_correcting, drag_y_correcting;

    long edit_modus;
    long edit_direction;
    int  owntimestamp;

    // modes ....
    AED_window( void );
    ~AED_window( void );

    int  load_data( void );
    void show_data( AW_device *device, AW_window *awmm, bool visibility_control );
    void show_top_data( AW_device *device, AW_window *awmm, AED_area_display_struct& display_struct );
    void show_single_top_data(AW_device *device, AW_window *awmm, AED_area_entry *area_entry, AED_area_display_struct& display_struct, AW_pos *y );
    void show_middle_data( AW_device *device, AW_window *awmm, AED_area_display_struct& display_struct );
    void show_single_middle_data(AW_device *device, AW_window *awmm, AED_area_entry *area_entry, AED_area_display_struct& display_struct, AW_pos *y );
    void show_bottom_data( AW_device *device, AW_window *awmm, AED_area_display_struct& display_struct );
    void show_single_bottom_data(AW_device *device, AW_window *awmm, AED_area_entry *area_entry, AED_area_display_struct& display_struct, AW_pos *y );
    void hide_cursor( AW_device *device, AW_window *awmm );
    void show_cursor( AW_device *device, AW_window *awmm );
    bool manage_cursor( AW_device *device, AW_window *awmm, bool use_last_slider_position );
    void show_single_area_entry( AW_device *device, AW_window *awmm, AED_area_entry *area_entry );
    void calculate_size(AW_window *awmm);
    void expose(AW_window *awmm);

    void init( AED_root *rootin );
    void select_area_entry( AED_area_entry *area_entry, AW_pos cursor_position );
    void deselect_area_entry( void );
    void make_left_text( char *string, AED_area_entry *area_entry );
};

void aed_create_window(AED_root *aedr);
void aed_initialize_device(AW_device *device);
void drag_box(AW_device *device, int gc, AW_pos x, AW_pos y, AW_pos width, AW_pos height, char *str);
void aed_expose(AW_window *aw, AW_CL cd1, AW_CL cd2);
void aed_clear_expose(AW_window *aw, AW_CL cd1, AW_CL cd2);
void aed_resize(AW_window *aw, AW_CL cd1, AW_CL cd2);


AW_window *create_naligner_window( AW_root *root, AW_CL cd2 );
void       create_naligner_variables(AW_root *root,AW_default db1);

void create_tool_variables(AW_root *root,AW_default db1);
//void create_submission_variables(AW_root *root,AW_default db1);

AW_window *create_tool_search( AW_root *root, AED_window *aedwindow );
AW_window *create_tool_replace( AW_root *root, AED_window *aedwindow );
AW_window *create_tool_complement( AW_root *root, AED_window *aedwindow );
AW_window *create_tool_speaker( AW_root *root, AED_window *aedwindow );
AW_window *create_tool_consensus( AW_root *root, AED_window *aedwindow );
//AW_window *create_submission_window( AW_root *root );
void set_cursor_to( AED_window *aedw, long cursor,class AED_area_entry *aed );
