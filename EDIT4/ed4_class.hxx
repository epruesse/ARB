#ifndef ed4_class_hxx_included
#define ed4_class_hxx_included

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef AW_FONT_GROUP_HXX
#include <aw_font_group.hxx>
#endif

#define e4_assert(bed) arb_assert(bed)

#ifdef DEBUG
# define IMPLEMENT_DUMP // comment out this line to skip compilation of the dump() methods
#endif

#include "ed4_defs.hxx"
#include "ed4_search.hxx"
#include "string.h"

#define ed4_beep() do {fputc(char(7), stdout); fflush(stdout); } while(0)

//****************************************
// needed prototypes, definitions below
//****************************************

class ED4_Edit_String;
class ED4_area_manager;
class ED4_base;
class ED4_bases_table;
class ED4_bracket_terminal;
class ED4_char_table;
class ED4_columnStat_terminal;
class ED4_consensus_sequence_terminal;
class ED4_cursor;
class ED4_device_manager;
class ED4_folding_line;
class ED4_group_manager;
class ED4_line_terminal;
class ED4_list;
class ED4_main_manager;
class ED4_manager;
class ED4_members;
class ED4_multi_name_manager;
class ED4_multi_sequence_manager;
class ED4_multi_species_manager;
class ED4_name_manager;
class ED4_pure_text_terminal;
class ED4_remap;
class ED4_root;
class ED4_root_group_manager;
class ED4_sequence_info_terminal;
class ED4_sequence_manager;
class ED4_sequence_terminal;
class ED4_spacer_terminal;
class ED4_species_manager;
class ED4_species_name_terminal;
class ED4_species_pointer;
class ED4_terminal;
class ED4_text_terminal;
class ED4_tree_terminal;
class ED4_window;

class AP_tree;
class AWT_reference;
class AWT_seq_colors;
class BI_ecoli_ref;
class AW_helix;
class ST_ML;
class ed_key;

// --------------------------------------------------------------------------------
//     class EDB_root_bact
// --------------------------------------------------------------------------------
class EDB_root_bact {

public:
    char        *make_string();             //just for debuggig
    char        *make_top_bot_string();         //just for debugging

    void calc_no_of_all(char *string_to_scan,   // group gets the number of groups in string_to_scan,
                        long *group,        // species gets the number of species in string_to_scan
                        long *species);

    ED4_returncode fill_species(ED4_multi_species_manager *multi_species_manager,
                                ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                ED4_sequence_terminal *ref_sequence_terminal,
                                char *string,
                                int *index,
                                ED4_index *y,
                                ED4_index actual_local_position,
                                ED4_index *length_of_terminals,
                                int group_depth);

    ED4_returncode fill_data(ED4_multi_species_manager *multi_species_manager,
                             ED4_sequence_info_terminal *ref_sequence_info_terminal,
                             ED4_sequence_terminal *ref_sequence_terminal,
                             char *string,
                             ED4_index *y,
                             ED4_index actual_local_position,
                             ED4_index *length_of_terminals,
                             int group_depth,
                             ED4_datamode datamode);        // flag only needed for loading a new configuration

    ED4_returncode search_sequence_data_rek(ED4_multi_sequence_manager *multi_sequence_manager,
                                            ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                            ED4_sequence_terminal *ref_sequence_terminal,
                                            GBDATA *gb_datamode,
                                            int count_too,
                                            ED4_index *seq_coords,
                                            ED4_index *max_seq_terminal_length,
                                            ED4_alignment alignment_flag);

    ED4_index scan_string(ED4_multi_species_manager *parent,
                          ED4_sequence_info_terminal *ref_sequence_info_terminal,
                          ED4_sequence_terminal *ref_sequence_terminal,
                          char *string,
                          int *index,
                          ED4_index *y);

    ED4_returncode create_group_header(ED4_multi_species_manager *parent,
                                       ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                       ED4_sequence_terminal *ref_sequence_terminal,
                                       ED4_multi_species_manager **multi_species_manager,
                                       ED4_bracket_terminal **bracket_terminal,
                                       ED4_index *y,
                                       char *groupname,
                                       int group_depth,
                                       bool is_folded,
                                       ED4_index local_count_position);

    char *generate_config_string(char *confname);


    EDB_root_bact();
    ~EDB_root_bact();
};



// --------------------------------------------------------------------------------
//     struct ED4_object_specification
// --------------------------------------------------------------------------------
struct ED4_object_specification
{
    ED4_properties static_prop;
    ED4_level      level;
    ED4_level      allowed_children;
    ED4_level      handled_level;
    ED4_level      restriction_level;
    float          justification; //Justification of Object, which is controlled by a manager

#if defined(IMPLEMENT_DUMP)
    void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

};                              //Manager coordinates Layout


// --------------------------------------------------------------------------------
//     class ED4_folding_line
// --------------------------------------------------------------------------------
class ED4_folding_line
//properties of an object, i.e. concerning rectangles on screen showing sequences
{
    ED4_folding_line(const ED4_folding_line&); // copy-constructor not allowed
public:
    AW_pos            world_pos[2];
    AW_pos            window_pos[2];
    AW_pos            length;
    AW_pos            dimension;
    ED4_base         *link;
    ED4_folding_line *next;

    ED4_folding_line();
    ~ED4_folding_line();
};

// --------------------------------------------------------------------------------
//     struct ED4_scroll_links
// --------------------------------------------------------------------------------
struct ED4_scroll_links
{
    ED4_base *link_for_hor_slider;
    ED4_base *link_for_ver_slider;

};

// --------------------------------------------------------------------------------
//     struct ED4_scrolled_rectangle
// --------------------------------------------------------------------------------
struct ED4_scrolled_rectangle
{
    ED4_folding_line *scroll_top, *scroll_bottom, *scroll_left, *scroll_right;
    ED4_base         *x_link, *y_link, *width_link, *height_link;
    AW_pos            world_x, world_y, width, height;

    void clear() {
        scroll_top    = 0;
        scroll_bottom = 0;
        scroll_left   = 0;
        scroll_right  = 0;
        x_link        = 0;
        y_link        = 0;
        width_link    = 0;
        height_link   = 0;
        world_x       = 0;
        world_y       = 0;
        width         = 0;
        height        = 0;
    }
};

// --------------------------------------------------------------------------------
//     class ED4_list_elem
// --------------------------------------------------------------------------------
class ED4_list_elem
{
    void          *my_elem;
    ED4_list_elem *my_next;
public:
    ED4_list_elem(void *element) { my_elem = element; my_next = 0; }
    ~ED4_list_elem() {}

    ED4_list_elem *next() const { return my_next; }
    void *elem() const { return my_elem; }

    void set_next(ED4_list_elem *the_next) { my_next = the_next; }
};


// --------------------------------------------------------------------------------
//     class ED4_list
// --------------------------------------------------------------------------------
class ED4_list
// class which implements a general purpose linked list of void*
{
    ED4_list_elem *my_first;
    ED4_list_elem *my_last;
    ED4_index      my_no_of_entries;

    ED4_list(const ED4_list&);  // copy-constructor not allowed

public:

    ED4_list_elem *first() const { return my_first; }
    ED4_list_elem *last() const { return my_last; }
    ED4_index no_of_entries() const { return my_no_of_entries; }

    ED4_returncode  append_elem( void *elem );
    ED4_returncode  delete_elem( void *elem );
    ED4_returncode  append_elem_backwards( void *elem );
    short is_elem( void *elem );

    ED4_list();
    ~ED4_list();
};

class ED4_window;

// --------------------------------------------------------------------------------
//     class ED4_base_position
// --------------------------------------------------------------------------------
class ED4_base_position
{
    ED4_base *calced4base;
    int      *seq_pos;
    int       count;

    void calc4base(ED4_base *base);

    ED4_base_position(const ED4_base_position&); // copy-constructor not allowed

public:

    ED4_base_position()     { calced4base = 0; seq_pos = 0; count = 0; }
    ~ED4_base_position()   { delete [] seq_pos; }

    int get_base_position(ED4_base *base, int sequence_position);
    int get_sequence_position(ED4_base *base, int base_position);

};

class ED4_CursorShape;


typedef enum
    {
        ED4_RIGHT_ORIENTED_CURSOR,
        ED4_RIGHT_ORIENTED_CURSOR_THIN,
        ED4_TRADITIONAL_CURSOR,
        ED4_TRADITIONAL_CURSOR_BOTTOM,
        ED4_TRADITIONAL_CURSOR_CONNECTED,
        ED4_FUCKING_BIG_CURSOR,

        ED4_CURSOR_TYPES

    } ED4_CursorType;

extern int ED4_update_global_cursor_awars_allowed;

// --------------------------------------------------------------------------------
//     class ED4_cursor
// --------------------------------------------------------------------------------

class ED4_cursor
{
    ED4_index          cursor_abs_x; // absolute (to terminal) x-position of cursor (absolute world coordinate of edit window)
    int                screen_position; // number of displayed characters leading the cursor
    int                last_seq_position; // last sequence position, cursor was set to (or -1)
    ED4_base_position  base_position; // # of bases left of cursor
    ED4_CursorType     ctype;
    ED4_CursorShape   *cursor_shape;

    ED4_returncode  draw_cursor( AW_pos x, AW_pos y, /*ED4_gc gc,*/ ED4_base *target_terminal );
    ED4_returncode  delete_cursor( AW_pos del_mark , ED4_base *target_terminal);
    //    ED4_returncode    draw_cursor_pos_box(/*ED4_gc gc,*/ ED4_base *terminal);
    ED4_returncode  get_upper_lower_cursor_pos( ED4_manager *starting_point, ED4_base **terminal, ED4_cursor_move cursor_move, AW_pos actual_y, bool (*terminal_is_appropriate)(ED4_base *terminal));
    void        updateAwars();

    ED4_cursor(const ED4_cursor&); // copy-constructor not allowed

    void        jump_cursor(AW_window *aww, int new_cursor_screen_pos, bool center_cursor, bool fix_left_border);

public:

    bool      allowed_to_draw;  //needed for cursor handling
    ED4_base *owner_of_cursor;

    bool is_partly_visible() const;

    void changeType(ED4_CursorType typ);
    ED4_CursorType  getType() const { return ctype; }

    void redraw() { changeType(getType()); }

    ED4_returncode HideCursor(); // deletes cursor and does refresh
    ED4_returncode move_cursor( AW_event *event );
    static void calc_cursor_position(AW_pos x, AW_pos *corrected_x, ED4_index *scr_pos);
    ED4_returncode  show_clicked_cursor( AW_event *event , ED4_base *target_terminal);
    ED4_returncode  ShowCursor(ED4_index offset_x, ED4_cursor_move move, int move_pos = 1);

    int get_sequence_pos() const;
    int get_sequence_pos_set() const { return last_seq_position==-1 ? get_sequence_pos() : last_seq_position; }
    int get_screen_pos() const { return screen_position; }

    void set_to_sequence_position(int seq_pos);
    ED4_returncode set_to_terminal(AW_window *aww, ED4_terminal *terminal, int seq_pos);

    long get_abs_x() const   { return cursor_abs_x; }
    void set_abs_x();

    int base2sequence_position(int pos) { return base_position.get_sequence_position(owner_of_cursor, pos); }

    void jump_left_right_cursor(AW_window *aww, int new_cursor_screen_pos);
    void jump_left_right_cursor_to_seq_pos(AW_window *aww, int new_cursor_seq_pos);

    void jump_centered_cursor(AW_window *aww, int new_cursor_screen_pos);
    void jump_centered_cursor_to_seq_pos(AW_window *aww, int new_cursor_seq_pos);

    void init();
    //    ED4_returncode    calc_cursor_pos_box(/*ED4_gc gc,*/ ED4_base *target_terminal);                      //calcs and draws

    ED4_window *window() const;

    ED4_cursor();
    ~ED4_cursor();
};

// --------------------------------------------------------------------------------
//     class ED4_window
// --------------------------------------------------------------------------------
class ED4_window
{
    ED4_window(const ED4_window&); // copy-constructor not allowed
public:
    AW_window              *aww; //Points to Window
    ED4_window             *next;
    AW_pos                  slider_pos_horizontal;
    AW_pos                  slider_pos_vertical;
    ED4_folding_line       *horizontal_fl;
    ED4_folding_line       *vertical_fl;
    ED4_scrolled_rectangle  scrolled_rect;
    int                     id; //unique id in window-list
    ED4_coords              coords;
    static int              no_of_windows;

    char awar_path_for_cursor[50]; // position in current sequence
    char awar_path_for_Ecoli[50]; // position relative to ecoli
    char awar_path_for_basePos[50]; // base position in current sequence (# of bases left to cursor)
    char awar_path_for_IUPAC[50]; // IUPAC decoder content for current position
    char awar_path_for_helixNr[50]; // # of helix (or 0) for current position

    bool       is_hidden;
    ED4_cursor cursor;

    //ED4_window controlling functions
    static ED4_window *insert_window(AW_window *new_aww); //append to window list

    void        delete_window( ED4_window *window); //delete from window list
    void        reset_all_for_new_config(); //reset structures for loading new config
    ED4_window *get_matching_ed4w( AW_window *aww );

    //functions concerned the scrolled area
    ED4_returncode      update_scrolled_rectangle( void );
    ED4_returncode      set_scrollbar_indents( void );
    ED4_returncode      scroll_rectangle( int dx, int dy );
    ED4_returncode      set_scrolled_rectangle( AW_pos    world_x, AW_pos world_y, AW_pos width, AW_pos height,
                                                ED4_base *x_link, ED4_base *y_link, ED4_base *width_link, ED4_base *height_link );

    void update_window_coords();

    // functions concerned with folding lines
    ED4_folding_line    *insert_folding_line( AW_pos world_x, AW_pos world_y, AW_pos length, AW_pos dimension, ED4_base *link, ED4_properties prop );
    ED4_returncode  delete_folding_line( ED4_folding_line *fl, ED4_properties prop );

    //    void redraw();

    ED4_window(AW_window *window);
    ~ED4_window();
};

// --------------------------------------------------------------------------------
//     class ED4_members
// --------------------------------------------------------------------------------
class ED4_members
//contains children related functions from members of a manager
{
    ED4_members(const ED4_members&);    // copy-constructor not allowed

    ED4_manager  *my_owner;     //who is controlling this object
    ED4_base    **memberList;
    ED4_index     no_of_members; //How much members are in the list
    ED4_index     size_of_list;

public:

    ED4_manager* owner() const { return my_owner; }
    ED4_base* member(ED4_index i) const { e4_assert(i>=0 && i<size_of_list); return memberList[i]; }
    ED4_index members() const { return no_of_members; }
    //    ED4_index size() const { return size_of_list; }

    ED4_returncode  insert_member   (ED4_base *new_member); // only used to move members with mouse
    ED4_returncode  append_member   (ED4_base *new_member);

    // an array is chosen instead of a linked list, because destructorhandling is more comfortable in various destructors (manager-destructors)

    ED4_returncode  delete_member       (ED4_base *member );
    ED4_index search_member       (ED4_extension *location, ED4_properties prop); // search member
    ED4_returncode  shift_list      (ED4_index start_index, int length);
    // list has to be shifted because member_list is an array and not a linked list

    ED4_returncode  search_target_species   ( ED4_extension *location, ED4_properties prop, ED4_base **found_member, ED4_level return_level );

    ED4_returncode  move_member     (ED4_index old_pos, ED4_index new_pos);

#if defined(IMPLEMENT_DUMP)
    void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

#if defined(ASSERTION_USED)
    int members_ok() const;
#endif // ASSERTION_USED

    ED4_members(ED4_manager *the_owner);
    ~ED4_members();
};

#ifdef DEBUG
// # define TEST_BASES_TABLE
#endif

#define SHORT_TABLE_ELEM_SIZE 1
#define SHORT_TABLE_MAX_VALUE 0xff
#define LONG_TABLE_ELEM_SIZE  4

// --------------------------------------------------------------------------------
//     class ED4_bases_table
// --------------------------------------------------------------------------------
class ED4_bases_table
{
    int table_entry_size;       // how many bytes are used for each element of 'no_of_bases' (1 or 4 bytes)
    union {
        unsigned char *shortTable;
        int           *longTable;
    } no_of_bases;      // counts bases for each sequence position
    int no_of_entries;      // length of bases table

    int legal(int offset) const { return offset>=0 && offset<no_of_entries; }

    void set_elem_long(int offset, int value) {
#ifdef TEST_BASES_TABLE
        e4_assert(legal(offset));
        e4_assert(table_entry_size==LONG_TABLE_ELEM_SIZE);
#endif
        no_of_bases.longTable[offset] = value;
    }

    void set_elem_short(int offset, int value) {
#ifdef TEST_BASES_TABLE
        e4_assert(legal(offset));
        e4_assert(table_entry_size==SHORT_TABLE_ELEM_SIZE);
        e4_assert(value>=0 && value<=SHORT_TABLE_MAX_VALUE);
#endif
        no_of_bases.shortTable[offset] = value;
    }

    int get_elem_long(int offset) const {
#ifdef TEST_BASES_TABLE
        e4_assert(legal(offset));
        e4_assert(table_entry_size==LONG_TABLE_ELEM_SIZE);
#endif
        return no_of_bases.longTable[offset];
    }

    int get_elem_short(int offset) const {
#ifdef TEST_BASES_TABLE
        e4_assert(legal(offset));
        e4_assert(table_entry_size==SHORT_TABLE_ELEM_SIZE);
#endif
        return no_of_bases.shortTable[offset];
    }

    ED4_bases_table(const ED4_bases_table&);    // copy-constructor not allowed

public:

    ED4_bases_table(int maxseqlength);
    ~ED4_bases_table();

    void init(int length);
    int size() const { return no_of_entries; }

    int get_table_entry_size() const { return table_entry_size; }
    void expand_table_entry_size();
    int bigger_table_entry_size_needed(int new_no_of_sequences) { return table_entry_size==SHORT_TABLE_ELEM_SIZE ? (new_no_of_sequences>SHORT_TABLE_MAX_VALUE) : 0; }

    int operator[](int offset) const {return table_entry_size==SHORT_TABLE_ELEM_SIZE ? get_elem_short(offset) : get_elem_long(offset);}

    void inc_short(int offset)  { set_elem_short(offset, get_elem_short(offset)+1); }
    void dec_short(int offset)  { set_elem_short(offset, get_elem_short(offset)-1); }
    void inc_long(int offset)   { set_elem_long(offset, get_elem_long(offset)+1); }
    void dec_long(int offset)   { set_elem_long(offset, get_elem_long(offset)-1); }

    int firstDifference(const ED4_bases_table& other, int start, int end, int *firstDifferentPos) const;
    int lastDifference(const ED4_bases_table& other, int start, int end, int *lastDifferentPos) const;

    void add(const ED4_bases_table& other, int start, int end);
    void sub(const ED4_bases_table& other, int start, int end);
    void sub_and_add(const ED4_bases_table& Sub, const ED4_bases_table& Add, int start, int end);

    void change_table_length(int new_length, int default_entry);


#ifdef ASSERTION_USED
    int empty() const;
#endif // ASSERTION_USED
};

typedef ED4_bases_table *ED4_bases_table_ptr;

#if defined(DEBUG)
# define TEST_CHAR_TABLE_INTEGRITY // uncomment to remove tests for ED4_char_table
#endif // DEBUG

// --------------------------------------------------------------------------------
//     class ED4_char_table
// --------------------------------------------------------------------------------
class ED4_char_table
{
    ED4_bases_table_ptr *bases_table;
    int                  sequences; // # of sequences added to the table
    int                  ignore; // this table will be ignored when calculating tables higher in hierarchy
    // (used to suppress SAI in root_group_man tables)

    // #ifdef TEST_CHAR_TABLE_INTEGRITY
    //     static bool tables_are_valid;
    // #endif

    static bool           initialized;
    static unsigned char  char_to_index_tab[MAXCHARTABLE];
    static unsigned char *upper_index_chars;
    static unsigned char *lower_index_chars;
    static int            used_bases_tables; // size of 'bases_table'

    inline void         set_char_to_index(unsigned char c, int index);
    inline void         set_string_to_index(const char *s, int index);

    ED4_char_table(const ED4_char_table&); // copy-constructor not allowed

    void add(const ED4_char_table& other, int start, int end);
    void sub(const ED4_char_table& other, int start, int end);

    void expand_tables();
    int get_table_entry_size() const {
        return linear_table(0).get_table_entry_size();
    }
    void prepare_to_add_elements(int new_sequences) {
        e4_assert(used_bases_tables);
        if (linear_table(0).bigger_table_entry_size_needed(sequences+new_sequences)) {
            expand_tables();
        }
    }

public:

#if defined(TEST_CHAR_TABLE_INTEGRITY) || defined(ASSERTION_USED)
    bool ok() const;
    bool empty() const;
#endif

#if defined(TEST_CHAR_TABLE_INTEGRITY)
    void test() const; // test if table is valid (dumps core if invalid)
#else
    void test() const { }
#endif

    ED4_char_table(int maxseqlength=0);
    ~ED4_char_table();

    void ignore_me() { ignore = 1; }
    int is_ignored() const { return ignore; }

    void init(int maxseqlength);
    int size() const { return bases_table[0]->size(); }
    int added_sequences() const { return sequences; }

    void bases_and_gaps_at(int column, int *bases, int *gaps) const;

    unsigned char index_to_upperChar(int index) const;
    unsigned char index_to_lowerChar(int index) const;

    // linear access to all tables
    ED4_bases_table&        linear_table(int c)         { e4_assert(c<used_bases_tables); return *bases_table[c]; }
    const ED4_bases_table&  linear_table(int c) const   { e4_assert(c<used_bases_tables); return *bases_table[c]; }

    // access via character
    ED4_bases_table&        table(int c)        { e4_assert(c>0 && c<MAXCHARTABLE); return linear_table(char_to_index_tab[c]); }
    const ED4_bases_table&  table(int c) const  { e4_assert(c>0 && c<MAXCHARTABLE); return linear_table(char_to_index_tab[c]); }

    int changed_range(const ED4_char_table& other, int *start, int *end) const;
    static int changed_range(const char *string1, const char *string2, int min_len, int *start, int *end);

    void add(const ED4_char_table& other);
    void sub(const ED4_char_table& other);
    void sub_and_add(const ED4_char_table& Sub, const ED4_char_table& Add);
    void sub_and_add(const ED4_char_table& Sub, const ED4_char_table& Add, int start, int end);

    void add(const char *string, int len);
    void sub(const char *string, int len);
    void sub_and_add(const char *old_string, const char *new_string, int start, int end);

    char *build_consensus_string(int left_idx=0, int right_index=-1, char *fill_id=0) const;

    void change_table_length(int new_length);
};

// --------------------------------------------------------------------------------
//     class ED4_species_pointer
// --------------------------------------------------------------------------------
class ED4_species_pointer // @@@ shall be renamed into ED4_gbdata_pointer to reflect general usage
{
    GBDATA *species_pointer;    // points to database

    void add_callback(int *clientdata);
    void remove_callback(int *clientdata);

public:

    ED4_species_pointer();
    ~ED4_species_pointer();

    GBDATA *get_species_pointer() const { return species_pointer; }
    void set_species_pointer(GBDATA *gbd, int *clientdata);
    void notify_deleted() {
        species_pointer=0;
    }
};

// --------------------------------------------------------------------------------
//     class ED4_base
// --------------------------------------------------------------------------------
class ED4_base                          //functions and variables which are available in all objects
{
    ED4_species_pointer my_species_pointer;
    ED4_base(const ED4_base&);  // copy-constructor not allowed

    // cache world coordinates:

    static int actualTimestamp;
    AW_pos     lastXpos;
    AW_pos     lastYpos;
    int        timestamp;

public:

    ED4_manager              *parent; //Points to parent
    ED4_object_specification *spec; //contains information about Objectproperties
    ED4_properties            dynamic_prop; //contains info about what i am, what i can do, what i should do
    char                     *id; //globally unique name in hierarchy
    ED4_index                 index; //defines the order of child objects
    ED4_base                 *width_link; //concerning the hierarchy
    ED4_base                 *height_link; //concerning the hierarchy
    ED4_extension             extension; //contains relativ info about graphical properties
    ED4_list                  linked_objects; //linked list of objects which are depending from this object
    ED4_update_info           update_info; //info about things to be done for the object, i.e. refresh; flag structure
    struct {
        unsigned int hidden:1;  //flag whether object is hidden or not
        unsigned int is_consensus:1; //indicates whether object is consensus(manager)
        unsigned int is_SAI:1;  //indicates whether object is extendend
    } flag;

    void draw_bb(int color);

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const = 0;
    void dump_base(size_t indent) const;
#endif // IMPLEMENT_DUMP

    // function for species_pointer

    GBDATA *get_species_pointer() const { return my_species_pointer.get_species_pointer(); }
    void set_species_pointer(GBDATA *gbd) { my_species_pointer.set_species_pointer(gbd, (int*)(this)); }
    int has_callback() const { return get_species_pointer()!=0; }

    // callbacks

    virtual void changed_by_database();
    virtual void deleted_from_database();

    virtual bool remove_deleted_childs();

    // functions concerned with graphic output
    virtual int adjust_clipping_rectangle(void); //sets scrolling area in AW_MIDDLE_AREA
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0) = 0;
    virtual ED4_returncode  Resize()                                  = 0;
    virtual ED4_returncode  clear_background(int color=0);
    virtual short calc_bounding_box(void)                             = 0;

    //    virtual ED4_returncode      show_scrolled(ED4_properties scroll_prop, int only_text = 0 ) = 0;
    ED4_returncode clear_whole_background( void ); // clear AW_MIDDLE_AREA
    bool is_visible(AW_pos x, AW_pos y, ED4_direction direction);
    bool is_visible(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2, ED4_direction direction);

    //functions concerned with links in the hierarchy
    virtual ED4_returncode  set_links(ED4_base *width_link, ED4_base *height_link);
    virtual ED4_returncode  link_changed(ED4_base *link);

    //functions concerned with special initialisation
    virtual void set_properties  (ED4_properties prop);

    //functions concerned with coordinate transformation

    void update_world_coords_cache();
    void calc_rel_coords( AW_pos *x, AW_pos *y );

    void calc_world_coords(AW_pos *x, AW_pos *y) {
        bool cache_up_to_date = timestamp == actualTimestamp;
        if (!cache_up_to_date) {
            update_world_coords_cache();
        }
        *x = lastXpos;
        *y = lastYpos;
    }

    static void touch_world_cache() {
        actualTimestamp++;
    }

    //functions which refer to the object as a child, i.e. travelling down the hierarchy
    virtual ED4_returncode  set_refresh (int clear=1)=0;
    virtual ED4_returncode  resize_requested_by_child(void)=0;
    virtual ED4_returncode  resize_requested_by_parent(void)=0;
    //    virtual ED4_returncode    refresh_requested_by_parent(void)=0;

    virtual ED4_returncode  delete_requested_by_parent(void)=0;
    virtual ED4_returncode  delete_requested_by_child(void);
    virtual ED4_returncode  delete_requested_childs(void)=0;

    virtual ED4_returncode  calc_size_requested_by_parent( void )=0;
    //    virtual ED4_returncode      set_scroll_refresh( AW_pos world_x, AW_pos world_y, AW_pos width, AW_pos height, ED4_properties scroll_prop )=0;  // (***)
    virtual ED4_returncode  move_requested_by_parent( ED4_move_info *mi )=0;
    virtual ED4_returncode  event_sent_by_parent(AW_event *event, AW_window *aww);
    virtual ED4_returncode  move_requested_by_child(ED4_move_info *moveinfo)=0;

    virtual ED4_returncode  handle_move(ED4_move_info *moveinfo)=0;
    virtual ED4_returncode  route_down_hierarchy(void **arg1, void **arg2, ED4_returncode (*function) (void **, void **, ED4_base *));
    int             calc_group_depth();

    //general purpose functions
    virtual ED4_base        *search_ID(const char *id)=0;
    virtual void        check_all();
    virtual short           in_border( AW_pos abs_x, AW_pos abs_y, ED4_movemode mode );
    virtual ED4_returncode      set_width();
    ED4_base            *get_parent(ED4_level lev) const;

    ED4_AREA_LEVEL      get_area_level(ED4_multi_species_manager **multi_species_manager=0); // returns area we belong to and the next multi species manager of the area

    int             is_child_of(ED4_manager *parent);
    ED4_group_manager       *is_in_folded_group() const;
    virtual char        *resolve_pointer_to_string_copy(int *str_len = 0) const;
    virtual const char      *resolve_pointer_to_char_pntr(int *str_len = 0) const;
    virtual ED4_ERROR       *write_sequence(const char *seq, int seq_len);
    char            *get_name_of_species();                 // go from terminal to name of species

    // functions which refer to the selected object(s), i.e. across the hierarchy
    virtual ED4_base        *get_competent_child( AW_pos x, AW_pos y, ED4_properties relevant_prop)=0;
    virtual ED4_base        *get_competent_clicked_child( AW_pos x, AW_pos y, ED4_properties relevant_prop)=0;
    virtual ED4_base        *search_spec_child_rek( ED4_level level );  //recursive search for level

    ED4_terminal        *get_next_terminal();
    ED4_terminal        *get_prev_terminal();

    ED4_returncode      generate_configuration_string(char **generated_string);

    virtual ED4_returncode  remove_callbacks();
    ED4_terminal        *get_consensus_relevant_terminal();

    ED4_base(GB_CSTR id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent);
    virtual ~ED4_base();

    // use the following functions to test which derived class we have

    int is_terminal()               const { return !this || spec->static_prop & ED4_P_IS_TERMINAL; }

    int is_text_terminal()          const { return !this || spec->level & (ED4_L_SPECIES_NAME|ED4_L_SEQUENCE_INFO|ED4_L_SEQUENCE_STRING|ED4_L_PURE_TEXT|ED4_L_COL_STAT); }

    int is_species_name_terminal()  const { return !this || spec->level & ED4_L_SPECIES_NAME; }
    int is_sequence_info_terminal() const { return !this || spec->level & ED4_L_SEQUENCE_INFO; }
    int is_sequence_terminal()      const { return !this || spec->level & ED4_L_SEQUENCE_STRING; }
    int is_pure_text_terminal()     const { return !this || spec->level & ED4_L_PURE_TEXT; }
    int is_columnStat_terminal()    const { return !this || spec->level & ED4_L_COL_STAT; }

    int is_bracket_terminal()       const { return !this || spec->level & ED4_L_BRACKET; }
    int is_spacer_terminal()        const { return !this || spec->level & ED4_L_SPACER; }
    int is_line_terminal()          const { return !this || spec->level & ED4_L_LINE; }

    int is_manager()                const { return !this || spec->static_prop & ED4_P_IS_MANAGER; }

    int is_sequence_manager()       const { return !this || spec->level & ED4_L_SEQUENCE; }
    int is_multi_name_manager()     const { return !this || spec->level & ED4_L_MULTI_NAME; }
    int is_name_manager()           const { return !this || spec->level & ED4_L_NAME_MANAGER; }
    int is_multi_species_manager()  const { return !this || spec->level & ED4_L_MULTI_SPECIES; }
    int is_multi_sequence_manager() const { return !this || spec->level & ED4_L_MULTI_SEQUENCE; }
    int is_device_manager()         const { return !this || spec->level & ED4_L_DEVICE; }
    int is_group_manager()          const { return !this || spec->level & ED4_L_GROUP; }
    int is_species_manager()        const { return !this || spec->level & ED4_L_SPECIES; }
    int is_area_manager()           const { return !this || spec->level & ED4_L_AREA; }

    // use the following functions to cast ED4_base to derived classes:

    ED4_base                    *to_base() const                    { e4_assert(is_terminal() || is_manager());         return (ED4_base *)this; }

    ED4_terminal                *to_terminal() const                { e4_assert(is_terminal());         return (ED4_terminal *)this; }

    ED4_text_terminal           *to_text_terminal() const           { e4_assert(is_text_terminal());    return (ED4_text_terminal *)this; }

    ED4_species_name_terminal   *to_species_name_terminal() const   { e4_assert(is_species_name_terminal()); return (ED4_species_name_terminal *)this; }
    ED4_sequence_info_terminal  *to_sequence_info_terminal() const  { e4_assert(is_sequence_info_terminal()); return (ED4_sequence_info_terminal*)this; }
    ED4_sequence_terminal       *to_sequence_terminal() const       { e4_assert(is_sequence_terminal());    return (ED4_sequence_terminal*)this; }
    ED4_pure_text_terminal      *to_pure_text_terminal() const      { e4_assert(is_pure_text_terminal());   return (ED4_pure_text_terminal*)this; }
    ED4_columnStat_terminal     *to_columnStat_terminal() const     { e4_assert(is_columnStat_terminal());  return (ED4_columnStat_terminal*)this; }

    ED4_bracket_terminal        *to_bracket_terminal() const        { e4_assert(is_bracket_terminal());     return (ED4_bracket_terminal*)this; }
    ED4_spacer_terminal         *to_spacer_terminal() const         { e4_assert(is_spacer_terminal());  return (ED4_spacer_terminal*)this; }
    ED4_line_terminal           *to_line_terminal() const           { e4_assert(is_line_terminal());    return (ED4_line_terminal*)this; }

    ED4_manager                 *to_manager() const                 { e4_assert(is_manager());      return (ED4_manager *)this; }

    ED4_sequence_manager        *to_sequence_manager() const        { e4_assert(is_sequence_manager());     return (ED4_sequence_manager*)this; }
    ED4_multi_name_manager      *to_multi_name_manager() const      { e4_assert(is_multi_name_manager());   return (ED4_multi_name_manager *)this; }
    ED4_name_manager            *to_name_manager() const            { e4_assert(is_name_manager());     return (ED4_name_manager *)this; }
    ED4_multi_species_manager   *to_multi_species_manager() const   { e4_assert(is_multi_species_manager()); return (ED4_multi_species_manager *)this; }
    ED4_multi_sequence_manager  *to_multi_sequence_manager() const  { e4_assert(is_multi_sequence_manager()); return (ED4_multi_sequence_manager *)this; }
    ED4_device_manager          *to_device_manager() const          { e4_assert(is_device_manager());   return (ED4_device_manager *)this; }
    ED4_group_manager           *to_group_manager() const           { e4_assert(is_group_manager());    return (ED4_group_manager *)this; }
    ED4_species_manager         *to_species_manager() const         { e4_assert(is_species_manager());  return (ED4_species_manager *)this; }
    ED4_area_manager            *to_area_manager() const            { e4_assert(is_area_manager());     return (ED4_area_manager *)this; }
};

// --------------------------------------------------------------------------------
//     class ED4_manager : public ED4_base
// --------------------------------------------------------------------------------
class ED4_manager : public ED4_base
{
    ED4_manager(const ED4_manager&); // copy-constructor not allowed

public:
    ED4_members *children;
    bool         is_group;

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

    int refresh_flag_ok();

    virtual void changed_by_database();
    virtual void deleted_from_database();

    virtual bool  remove_deleted_childs();

    //functions concerned with graphics
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0);
    virtual ED4_returncode  Resize();
    virtual short       calc_bounding_box(void);
    virtual ED4_returncode  distribute_children();
    //    virtual ED4_returncode    show_scrolled(ED4_properties scroll_prop, int only_text = 0 );

    //top-down functions, means travelling down the hierarchy
    virtual ED4_returncode  event_sent_by_parent(AW_event *event, AW_window *aww);
    virtual ED4_returncode  set_refresh(int clear=1);
    ED4_returncode      clear_refresh(void);
    virtual ED4_returncode      resize_requested_by_parent( void );
    //    virtual ED4_returncode    refresh_requested_by_parent(void);

    virtual ED4_returncode  delete_requested_by_parent(void);
    virtual ED4_returncode  delete_requested_childs(void);

    virtual ED4_returncode  calc_size_requested_by_parent( void );
    virtual ED4_returncode  move_requested_by_parent( ED4_move_info *mi );
    void                        create_consensus(ED4_group_manager *upper_group_manager);
    virtual ED4_returncode      route_down_hierarchy(void **arg1, void **arg2, ED4_returncode (*function) (void **, void **, ED4_base *));

    virtual ED4_base*       find_first_that(ED4_level level, int (*condition)(ED4_base *to_test, AW_CL arg), AW_CL arg);
    virtual ED4_base*       find_first_that(ED4_level level, int (*condition)(ED4_base *to_test));

    // bottom-up functions
    virtual ED4_returncode  move_requested_by_child(ED4_move_info *moveinfo);
    virtual ED4_returncode  resize_requested_by_child(void);
    virtual ED4_returncode  refresh_requested_by_child(void);
    //    virtual ED4_returncode    set_scroll_refresh( AW_pos world_x, AW_pos world_y, AW_pos width, AW_pos height, ED4_properties scroll_prop );       // (***)
    ED4_base            *get_defined_level(ED4_level lev) const;

    // functions refering the consensus

    ED4_returncode      create_group( ED4_group_manager **group_manager, GB_CSTR group_name);

    virtual ED4_returncode  update_consensus(ED4_manager *old_parent, ED4_manager *new_parent, ED4_base *sequence, int start_pos = 0, int end_pos = -1);
    virtual ED4_returncode  rebuild_consensi( ED4_base *start_species, ED4_update_flag update_flag);

    virtual ED4_returncode  check_in_bases(ED4_base *added_base, int start_pos=0, int end_pos=-1);
    virtual ED4_returncode  check_out_bases(ED4_base *subbed_base, int start_pos=0, int end_pos=-1);
    virtual ED4_returncode  check_bases(const ED4_base *old_base, const ED4_base *new_base, int start_pos=0, int end_pos=-1);
    virtual ED4_returncode  check_bases(const char *old_seq, int old_len, const char *new_seq, int new_len, int start_pos=0, int end_pos=-1);
    virtual ED4_returncode  check_bases(const char *old_seq, int old_len, const ED4_base *new_base, int start_pos=0, int end_pos=-1);
    virtual ED4_returncode  check_bases(const ED4_char_table *old_table, const ED4_char_table *new_table, int start_pos=0, int end_pos=-1);

    virtual ED4_returncode  check_bases_and_rebuild_consensi(const char *old_seq, int old_len, ED4_base *species, ED4_update_flag update_flag, int start_pos=0, int end_pos=-1);

    void            generate_id_for_groups();

    // handle moves across the hierarchy
    virtual ED4_returncode  handle_move(ED4_move_info *moveinfo);
    virtual ED4_base        *get_competent_child( AW_pos x, AW_pos y, ED4_properties relevant_prop);
    virtual ED4_base        *get_competent_clicked_child( AW_pos x, AW_pos y, ED4_properties relevant_prop);
    virtual ED4_base        *search_spec_child_rek( ED4_level level );  //recursive search for level

    //general purpose functions
    virtual ED4_base        *search_ID(const char *id);
    virtual ED4_returncode  remove_callbacks();

    ED4_terminal *get_first_terminal(int start_index=0) const;
    ED4_terminal *get_last_terminal(int start_index=-1) const;

    //general folding functions
    virtual ED4_returncode  unfold_group(char *bracketID_to_unfold);
    virtual ED4_returncode  fold_group(char *bracketID_to_fold);
    virtual ED4_returncode      make_children_visible(void);
    virtual ED4_returncode  hide_children(void);

    ED4_manager(const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0);
    virtual ~ED4_manager();
};


// --------------------------------------------------------------------------------
//     class ED4_terminal : public ED4_base
// --------------------------------------------------------------------------------
class ED4_terminal : public ED4_base
{
    ED4_terminal(const ED4_terminal&); // copy-constructor not allowed
public:
    struct {
        unsigned int        selected:1;     //Flag for 'Object selected'
        unsigned int        dragged:1;      //Flag for 'Object dragged'
        unsigned int        deleted:1;
    } flag;
    //    unsigned int      has_callback:1;
    ED4_selection_entry *selection_info; //Info about i.e. Position
    long                 actual_timestamp;

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

    // callbacks

    virtual void changed_by_database(void);
    virtual void deleted_from_database(void);

    virtual bool  remove_deleted_childs();

    //functions concerning graphic output
    virtual ED4_returncode Show(int refresh_all=0, int is_cleared=0) = 0;
    virtual ED4_returncode Resize();
    virtual ED4_returncode draw( int only_text = 0 )                 = 0;

    virtual int   adjust_clipping_rectangle(void);
    virtual short calc_bounding_box(void);
    virtual ED4_returncode  calc_size_requested_by_parent( void );

    //    virtual ED4_returncode    show_selected( void );
    virtual ED4_returncode      draw_drag_box( AW_pos x, AW_pos y, GB_CSTR text = NULL, int cursor_y=-1 );
    //    virtual ED4_returncode    show_scrolled(ED4_properties scroll_prop, int only_text = 0 );
    //    virtual ED4_returncode    adjust_scroll_clipping_rectangle( void );

    //functions which concern the object as a child
    virtual ED4_returncode  set_refresh(int clear=1);
    virtual ED4_returncode  resize_requested_by_child(void);
    virtual ED4_returncode      resize_requested_by_parent(void);
    //    virtual ED4_returncode    refresh_requested_by_parent( void );

    virtual ED4_returncode  delete_requested_by_parent(void);
    virtual ED4_returncode  delete_requested_childs(void);

    //    virtual ED4_returncode    set_scroll_refresh( AW_pos world_x, AW_pos world_y, AW_pos width, AW_pos height, ED4_properties scroll_prop );      // (***)
    virtual ED4_returncode  move_requested_by_parent( ED4_move_info *mi );
    virtual ED4_returncode  event_sent_by_parent(AW_event *event, AW_window *aww);
    virtual ED4_base        *get_competent_child( AW_pos x, AW_pos y, ED4_properties relevant_prop);
    virtual ED4_base        *get_competent_clicked_child( AW_pos x, AW_pos y, ED4_properties relevant_prop);
    virtual ED4_returncode  move_requested_by_child(ED4_move_info *moveinfo);
    virtual ED4_returncode  handle_move(ED4_move_info *moveinfo);
    virtual ED4_returncode      kill_object();

    //general purpose functions
    virtual ED4_base  *search_ID(const char *id);
    virtual char *resolve_pointer_to_string_copy(int *str_len = 0) const; // concerning terminal and database
    virtual const char      *resolve_pointer_to_char_pntr(int *str_len = 0) const; // concerning terminal and database
    virtual ED4_ERROR *write_sequence(const char *seq, int seq_len);
    virtual ED4_returncode  remove_callbacks();

    inline int          setCursorTo(ED4_cursor *cursor, int pos, int unfoldGroups);

    ED4_terminal(GB_CSTR id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent);
    virtual ~ED4_terminal();
};

typedef enum {
    ED4_SM_MOVE,
    ED4_SM_KILL,
    ED4_SM_MARK
} ED4_species_mode;

// --------------------------------------------------------------------------------
//     class ED4_reference_terminals
// --------------------------------------------------------------------------------
class ED4_reference_terminals
{
    ED4_sequence_info_terminal *ref_sequence_info;
    ED4_sequence_terminal      *ref_sequence;
    ED4_sequence_info_terminal *ref_column_stat_info;
    ED4_columnStat_terminal    *ref_column_stat;

    void null() { ref_sequence_info = 0; ref_sequence = 0; ref_column_stat = 0; ref_column_stat_info = 0; }
public:
    void clear();
    void init(ED4_sequence_info_terminal *, ED4_sequence_terminal *, ED4_sequence_info_terminal *, ED4_columnStat_terminal *);

    ED4_sequence_info_terminal *get_ref_sequence_info()     { return ref_sequence_info; }
    ED4_sequence_terminal *get_ref_sequence()           { return ref_sequence; }
    ED4_sequence_info_terminal *get_ref_column_stat_info()  { return ref_column_stat_info; }
    ED4_columnStat_terminal *get_ref_column_stat()      { return ref_column_stat; }

    ED4_reference_terminals()  { null(); }
    ~ED4_reference_terminals() { clear(); }
};

// --------------------------------------------------------------------------------
//     class ED4_root
// --------------------------------------------------------------------------------
class ED4_root
{
    ED4_root(const ED4_root&);  // copy-constructor not allowed
public:
    AW_root                 *aw_root; // Points to 'AW-Window-Controller'
    AW_default               db; // Default Properties database
    const char              *db_name; // name of Default Properties database
    ED4_window              *first_window; // Points to List of Main Windows of ED4
    ED4_main_manager        *main_manager; // Points to Main manager of ED4
    ED4_area_manager        *middle_area_man; // Points to middle area
    ED4_area_manager        *top_area_man;
    ED4_root_group_manager  *root_group_man;
    EDB_root_bact           *database; // Points to Object which controls Data
    ED4_list                 selected_objects;
    ED4_scroll_links         scroll_links;
    bool                     folding_action; // flag tells whether action was folding action or not
    //    ED4_cursor            has_cursor[MAXWINDOWS]; // class for cursor handling
    ED4_reference_terminals  ref_terminals;
    ED4_species_mode         species_mode;
    ED4_scroll_picture       scroll_picture;
    BI_ecoli_ref            *ecoli_ref;
    char                    *alignment_name;
    GB_alignment_type        alignment_type;
    AWT_reference           *reference;
    AWT_seq_colors          *sequence_colors;
    ST_ML                   *st_ml;
    AW_helix                *helix;
    int                      helix_spacing;
    long                     helix_add_spacing;
    long                     terminal_add_spacing;
    char                    *protstruct; // protein structure summary
    long                     protstruct_len; // protein structure summary
    ed_key                  *edk;
    ED4_Edit_String         *edit_string;
    int                      column_stat_activated;
    int                      column_stat_initialized;
    int                      visualizeSAI;
    int                      visualizeSAI_allSpecies;

    // temporary Variables changing during runtime
    ED4_window    *temp_ed4w;
    AW_window     *temp_aww;
    AW_device     *temp_device;
    int            temp_gc;
    AW_font_group  font_group;

    //Initializing functions
    ED4_returncode  create_hierarchy(char *area_string_middle, char *area_string_top); // flag only needed for loading a new configuration
    ED4_returncode  init_alignment();
    void recalc_font_group();

    AW_window       *create_new_window(void);
    ED4_returncode  generate_window( AW_device **device, ED4_window **new_window);
    void        copy_window_struct( ED4_window *source , ED4_window *destination );

    //functions concerned with global refresh and resize
    ED4_returncode      resize_all( void );

private:
    ED4_returncode      refresh_window_simple(int redraw);
public:
    ED4_returncode      refresh_window(int redraw);
    ED4_returncode  refresh_all_windows(int redraw);

    //    ED4_returncode      show_all( void );
    //    ED4_returncode    resize_all_windows( void );

    void announce_deletion(ED4_base *object); // before deleting an object, announce here

    // functions concerned with list of selected objects
    ED4_returncode  add_to_selected(  ED4_terminal *object );
    ED4_returncode  remove_from_selected(  ED4_terminal *object);
    short               is_primary_selection(  ED4_terminal *object);
    ED4_returncode      deselect_all( void );

    //functions concerning coordinate transformation
    ED4_returncode  world_to_win_coords(AW_window *aww, AW_pos *x, AW_pos *y);
    ED4_returncode      win_to_world_coords( AW_window *aww, AW_pos *x, AW_pos *y );
    ED4_returncode  get_area_rectangle(AW_rectangle *rect, AW_pos x, AW_pos y);

    ED4_root();
    ~ED4_root();
};


//***************************************
//* Manager specifications   beginning  *
//***************************************
// All manager class only differ in their static properties. This kind of construction was chosen for using a minimum of RAM

// --------------------------------------------------------------------------------
//     class ED4_main_manager : public ED4_manager
  // --------------------------------------------------------------------------------
class ED4_main_manager : public ED4_manager // first in hierarchy
{
    // these terminals are redrawn after refresh (with increase clipping area)
    // to revert text from middle area drawn into top area: 
    ED4_terminal *top_middle_line;
    ED4_terminal *top_middle_spacer;

    ED4_main_manager(const ED4_main_manager&); // copy-constructor not allowed
public:
    ED4_main_manager( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0 );
    ~ED4_main_manager();

    void set_top_middle_spacer_terminal(ED4_terminal *top_middle_spacer_) { top_middle_spacer = top_middle_spacer_; }
    void set_top_middle_line_terminal(ED4_terminal *top_middle_line_) { top_middle_line = top_middle_line_; }
    
    ED4_terminal *get_top_middle_spacer_terminal() const { return top_middle_spacer; }
    ED4_terminal *get_top_middle_line_terminal() const { return top_middle_line; }

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
    virtual ED4_returncode Show(int refresh_all=0, int is_cleared=0);
    virtual ED4_returncode resize_requested_by_parent(void);
};

// --------------------------------------------------------------------------------
//     class ED4_device_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_device_manager : public ED4_manager
{
    ED4_device_manager(const ED4_device_manager&); // copy-constructor not allowed
public:
    ED4_device_manager  ( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_device_manager ();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_area_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_area_manager : public ED4_manager
{
    ED4_area_manager(const ED4_area_manager&); // copy-constructor not allowed
public:
    ED4_area_manager    ( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_area_manager   ();
#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_multi_species_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_multi_species_manager : public ED4_manager
{
    int species;    // # of species (-1 == unknown)
    int selected_species; // # of selected species (-1 == unknown)

    void    set_species_counters(int no_of_species, int no_of_selected);
#ifdef DEBUG
    void    count_species(int *speciesPtr, int *selectedPtr) const;
#endif
    void    update_species_counters();

    ED4_multi_species_manager(const ED4_multi_species_manager&); // copy-constructor not allowed

public:
    ED4_multi_species_manager( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_multi_species_manager();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

    int           count_visible_children(); // is called by a multi_species_manager
    int           count_all_children_and_set_group_id(); // counts all children of a parent
    ED4_terminal *get_consensus_terminal(); // returns the consensus-terminal or 0
    ED4_species_manager *get_consensus_manager() const; // returns the consensus-manager or 0

    // functions concerned with selection
    int         get_no_of_selected_species();
    int         get_no_of_species();

    void        invalidate_species_counters();
    int         has_valid_counters() const      { return species!=-1 && selected_species!=-1; }

    void        select_all_species();
    void        deselect_all_species();
    void        invert_selection_of_all_species();
    void        select_marked_species(int select);
    void        mark_selected_species(int mark);
};

// --------------------------------------------------------------------------------
//     class ED4_group_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_group_manager : public ED4_manager
{
    ED4_group_manager(const ED4_group_manager&); // copy-constructor not allowed

protected:

    ED4_char_table my_table; // table concerning Consensusfunction

public:

    ED4_group_manager( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    virtual ~ED4_group_manager();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

    ED4_char_table&         table() { return my_table; }
    const ED4_char_table&   table() const { return my_table; }

    ED4_bases_table&        table(unsigned char c) {return table().table(c);}
    const ED4_bases_table&  table(unsigned char c) const {return table().table(c);}

    void reinit_char_table();
};

typedef enum
    {
        ED4_RM_NONE,            // no remapping (normal mode)
        ED4_RM_SHOW_ABOVE,              // Show all positions, where xxx% of all edited sequences have any base
        ED4_RM_MAX_ALIGN,           // ------------------------- any edited sequence has a base
        ED4_RM_MAX_EDIT,            // like ED4_RM_MAX_ALIGN, but bases are pushed OVER remapped gaps (not INTO)
        ED4_RM_DYNAMIC_GAPS,        // gaps shown as spaces (more gaps => more spaces)

        ED4_RM_MODES

    } ED4_remap_mode;

// --------------------------------------------------------------------------------
//     class ED4_remap
// --------------------------------------------------------------------------------
class ED4_remap {

    ED4_remap_mode mode;
    int show_above_percent;     // used only for ED4_RM_SHOW_ABOVE

    size_t sequence_table_len;      // allocated size of sequence_to_screen_tab
    size_t screen_table_len;        // allocated size of screen_to_sequence_tab

    size_t sequence_len;        // size of recently compiled part of sequence_to_screen_tab
    size_t screen_len;          // size of recently compiled part of screen_to_sequence_tab

    int *screen_to_sequence_tab;
    int *sequence_to_screen_tab;    // <0 means position is not mapped (last displayed sequence position)

    int changed;            // remap-table changed at last compile
    int update_needed;          // remapping should be recompiled

    long clip(long pos, long min_pos, long max_pos) const { return pos<min_pos ? min_pos : (pos<=max_pos ? pos : max_pos); }
    inline void set_sequence_to_screen(int pos, int newVal);

    ED4_remap(const ED4_remap&); // copy-constructor not allowed

public:

    ED4_remap();
    ~ED4_remap();

    int screen_to_sequence(int screen_pos) const;
    int sequence_to_screen(int sequence_pos) const;
    int clipped_sequence_to_screen(int sequence_pos) const;
    int sequence_to_screen_clipped(int sequence_pos) const;
    size_t get_max_screen_pos() const { return screen_len-1; }

    ED4_remap_mode get_mode() const { return mode; }
    void set_mode(ED4_remap_mode Mode, int above_percent) {
        if (Mode<0 || Mode>=ED4_RM_MODES) {
            Mode = ED4_RM_NONE;
        }
        if (mode!=Mode) {
            mode = Mode;
            update_needed = 1;
        }

        if (show_above_percent!=above_percent) {
            above_percent = show_above_percent;
            if (mode==ED4_RM_SHOW_ABOVE) {
                update_needed = 1;
            }
        }
    }

    void mark_compile_needed();     // recompile if mode != none
    void mark_compile_needed_force();   // always recompile
    int compile_needed() const { return update_needed; }

    GB_ERROR compile(ED4_root_group_manager *gm);
    int was_changed() const { return changed; }     // mapping changed by last compile ?

    int is_visible(int position) const { return sequence_to_screen(position)>=0; }

    void clip_screen_range(long *left_screen_pos, long *right_screen_pos) const {
        *right_screen_pos = clip(*right_screen_pos, 0, screen_len-1);
        *left_screen_pos = clip(*left_screen_pos, 0, screen_len-1);
    }
};

// --------------------------------------------------------------------------------
//     class ED4_root_group_manager : public ED4_group_manager
// --------------------------------------------------------------------------------
class ED4_root_group_manager : public ED4_group_manager
{
    ED4_remap my_remap;

    ED4_root_group_manager(const ED4_root_group_manager&); // copy-constructor not allowed

public:

    ED4_root_group_manager( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent  );
    ~ED4_root_group_manager();

    int update_remap(); // TRUE if mapping has changed

    const ED4_remap *remap() const { return &my_remap; }
    ED4_remap *remap() { return &my_remap; }

    virtual ED4_returncode Show(int refresh_all=0, int is_cleared=0);
    virtual ED4_returncode resize_requested_by_parent( void );
};

// --------------------------------------------------------------------------------
//     class ED4_species_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_species_manager : public ED4_manager
{
    ED4_species_manager(const ED4_species_manager&); // copy-constructor not allowed
public:
    ED4_species_manager ( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_species_manager   ();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

    int setCursorTo(ED4_cursor *cursor, int position, int unfold_groups);
};

// --------------------------------------------------------------------------------
//     class ED4_multi_sequence_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_multi_sequence_manager : public ED4_manager
{
    ED4_multi_sequence_manager(const ED4_multi_sequence_manager&); // copy-constructor not allowed
public:
    ED4_multi_sequence_manager( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_multi_sequence_manager();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_sequence_manager : public ED4_manager
// --------------------------------------------------------------------------------
class ED4_sequence_manager : public ED4_manager
{
    ED4_sequence_manager(const ED4_sequence_manager&); // copy-constructor not allowed
public:
    ED4_sequence_manager    ( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_sequence_manager   ();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_multi_name_manager : public ED4_manager // contains info concerning the species; it's linked into speciesmanager
// --------------------------------------------------------------------------------
class ED4_multi_name_manager : public ED4_manager // contains info concerning the species; it's linked into speciesmanager
{
    ED4_multi_name_manager(const ED4_multi_name_manager&); // copy-constructor not allowed
public:
    ED4_multi_name_manager  ( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    virtual ~ED4_multi_name_manager ();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};


// --------------------------------------------------------------------------------
//     class ED4_name_manager : public ED4_manager // contains speciesname and other info concerning the species; it's linked into speciesmanager
// --------------------------------------------------------------------------------
class ED4_name_manager : public ED4_manager // contains speciesname and other info concerning the species; it's linked into speciesmanager
{
    ED4_name_manager(const ED4_name_manager&); // copy-constructor not allowed
public:
    ED4_name_manager( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent, bool temp_is_group = 0  );
    ~ED4_name_manager();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

//***************************************
//* Manager specifications  end *
//***************************************



//***************************************
//* Terminal specifications beginning   *
//***************************************

// --------------------------------------------------------------------------------
//     class ED4_tree_terminal : public ED4_terminal
// --------------------------------------------------------------------------------
class ED4_tree_terminal : public ED4_terminal
{
    ED4_tree_terminal(const ED4_tree_terminal&); // copy-constructor not allowed
public:
    virtual ED4_returncode  draw(int only_text=0);
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0);

    ED4_tree_terminal( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    ~ED4_tree_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_bracket_terminal : public ED4_terminal
// --------------------------------------------------------------------------------
class ED4_bracket_terminal : public ED4_terminal
{
    ED4_bracket_terminal(const ED4_bracket_terminal&); // copy-constructor not allowed
public:
    virtual ED4_returncode  draw( int only_text = 0 );
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0);

    ED4_bracket_terminal( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    ~ED4_bracket_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_text_terminal : public ED4_terminal
// --------------------------------------------------------------------------------
class ED4_text_terminal : public ED4_terminal
{
    ED4_text_terminal(const ED4_text_terminal&); // copy-constructor not allowed
public:
    //functions concerning graphic output
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0);
    virtual ED4_returncode      draw(int only_text=0);

    virtual int get_length() const = 0;
    virtual void deleted_from_database();

    ED4_text_terminal(GB_CSTR id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent);
    virtual ~ED4_text_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

/* Beispielklasse :

class Beispiel : public ED4_terminal
{
public:
.
.
.

Beispiel(const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent); // Construktorparameter an parents weitergeben
~Beispiel();
};

*/

// --------------------------------------------------------------------------------
//     class ED4_sequence_terminal : public ED4_text_terminal
// --------------------------------------------------------------------------------
class ED4_sequence_terminal : public ED4_text_terminal
{
    ED4_SearchResults searchResults;

    virtual ED4_returncode  draw( int only_text = 0 );
    ED4_sequence_terminal(const ED4_sequence_terminal&); // copy-constructor not allowed

public:

    char    *species_name;
    AP_tree *st_ml_node;

    ED4_sequence_terminal( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    virtual ~ED4_sequence_terminal();

    virtual void deleted_from_database();

    ED4_SearchResults& results() { return searchResults; }
    ED4_species_name_terminal *corresponding_species_name_terminal() const {
        return get_parent(ED4_L_SPECIES)->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
    }
    ED4_columnStat_terminal *corresponding_columnStat_terminal() const {
        ED4_base *col_term = get_parent(ED4_L_MULTI_SEQUENCE)->search_spec_child_rek(ED4_L_COL_STAT);
        return col_term ? col_term->to_columnStat_terminal() : 0;
    }

    int get_length() const { int len; resolve_pointer_to_char_pntr(&len); return len; }

    void calc_intervall_displayed_in_rectangle(AW_rectangle *area_rect, long *left_index, long *right_index );
    void calc_update_intervall(long *left_index, long *right_index );

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_columnStat_terminal : public ED4_text_terminal
// --------------------------------------------------------------------------------
class ED4_columnStat_terminal : public ED4_text_terminal
{
    char *likelihood[4];        // likelihood-array for each base (ACGU) [length of array = alignment_length]
    int   latest_update;

    static double threshold;

    int update_likelihood();

    ED4_columnStat_terminal(const ED4_columnStat_terminal&); // copy-constructor not allowed
public:
    //functions concerning graphic output
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0);
    virtual ED4_returncode      draw(int only_text=0);

    static int threshold_is_set();
    static void set_threshold(double aThreshold);
    static double get_threshold() { return threshold; }

    ED4_sequence_terminal *corresponding_sequence_terminal() const { return get_parent(ED4_L_MULTI_SEQUENCE)->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal(); }

    int get_length() const { return corresponding_sequence_terminal()->to_text_terminal()->get_length(); }

    GB_CSTR build_probe_match_string(int start_pos, int end_pos) const;

    ED4_columnStat_terminal(GB_CSTR id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_columnStat_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_species_name_terminal : public ED4_text_terminal
// --------------------------------------------------------------------------------
class ED4_species_name_terminal : public ED4_text_terminal
{
    ED4_species_name_terminal(const ED4_species_name_terminal&); // copy-constructor not allowed
public:
    //functions concerning graphic output

    ED4_species_name_terminal( GB_CSTR id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    ~ED4_species_name_terminal();

    GB_CSTR get_displayed_text() const;
    int get_length() const { return strlen(get_displayed_text()); }

    ED4_sequence_terminal *corresponding_sequence_terminal() const {
        ED4_base *seq_term = get_parent(ED4_L_SPECIES)->search_spec_child_rek(ED4_L_SEQUENCE_STRING);
        return seq_term ? seq_term->to_sequence_terminal() : 0;
    }

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_sequence_info_terminal : public ED4_text_terminal
// --------------------------------------------------------------------------------
class ED4_sequence_info_terminal : public ED4_text_terminal
{
    // now handled via ED4_species_pointer // GBDATA *gbdata;
    ED4_sequence_info_terminal(const ED4_sequence_info_terminal&); // copy-constructor not allowed
public:
    ED4_sequence_info_terminal( const char *id, /*GBDATA *gbd,*/ AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    virtual ~ED4_sequence_info_terminal();

    ED4_species_name_terminal *corresponding_species_name_terminal() const {
        return get_parent(ED4_L_SPECIES)->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
    }
    
    virtual ED4_returncode draw( int only_text = 0);

    // GBDATA *data() { return gbdata; }
    // const GBDATA *data() const { return gbdata; }
    GBDATA *data() { return get_species_pointer(); }
    const GBDATA *data() const { return get_species_pointer(); }

    int get_length() const { return 1+strlen(id); }

    virtual bool remove_deleted_childs();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_pure_text_terminal : public ED4_text_terminal
// --------------------------------------------------------------------------------
class ED4_pure_text_terminal : public ED4_text_terminal
{
    //    virtual ED4_returncode draw(int only_text=0);
    ED4_pure_text_terminal(const ED4_pure_text_terminal&);  // copy-constructor not allowed
public:
    ED4_pure_text_terminal(const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_pure_text_terminal();

    int get_length() const { int len; resolve_pointer_to_char_pntr(&len); return len; }

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_consensus_sequence_terminal : public ED4_sequence_terminal
// --------------------------------------------------------------------------------
class ED4_consensus_sequence_terminal : public ED4_sequence_terminal
{
    virtual ED4_returncode  draw( int only_text = 0 );
    ED4_consensus_sequence_terminal(const ED4_consensus_sequence_terminal&); // copy-constructor not allowed
public:
    ED4_consensus_sequence_terminal( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    ~ED4_consensus_sequence_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_spacer_terminal : public ED4_terminal
// --------------------------------------------------------------------------------
class ED4_spacer_terminal : public ED4_terminal
{
    ED4_spacer_terminal(const ED4_spacer_terminal&); // copy-constructor not allowed
public:
    virtual ED4_returncode      Show(int refresh_all=0, int is_cleared=0);
    //    ED4_returncode      show_scrolled(ED4_properties scroll_prop, int only_text = 0 );
    virtual ED4_returncode      draw(int only_text = 0);

    ED4_spacer_terminal( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    ~ED4_spacer_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

// --------------------------------------------------------------------------------
//     class ED4_line_terminal : public ED4_terminal
// --------------------------------------------------------------------------------
class ED4_line_terminal : public ED4_terminal
{
    ED4_line_terminal(const ED4_line_terminal&); // copy-constructor not allowed
public:
    virtual ED4_returncode      Show(int refresh_all=0, int is_cleared=0);
    virtual ED4_returncode  draw(int only_text = 0);
    //    virtual ED4_returncode set_scroll_refresh(AW_pos world_x, AW_pos world_y, AW_pos width, AW_pos height, ED4_properties scroll_prop ); // (***)

    ED4_line_terminal( const char *id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *parent );
    ~ED4_line_terminal();

#if defined(IMPLEMENT_DUMP)
    virtual void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP
};

//***************************************
//* Terminal specifications     end *
//***************************************

//**************************************************
//* inline expansion which need complete classdefs *
//**************************************************

inline int ED4_terminal::setCursorTo(ED4_cursor *cursor, int pos, int unfoldGroups)
{
    ED4_species_manager *sm = get_parent(ED4_L_SPECIES)->to_species_manager();
    return sm->setCursorTo(cursor, pos, unfoldGroups);
}

//***************************************
//* Prototype functions without a class *
//***************************************

extern      ST_ML *st_ml;

void        ED4_expose_cb           (AW_window *aww,AW_CL cd1, AW_CL cd2);

void        ED4_calc_terminal_extentions();

void        ED4_input_cb            (AW_window *aww,AW_CL cd1, AW_CL cd2);
void        ED4_resize_cb           (AW_window *aww,AW_CL cd1, AW_CL cd2);

void        ED4_gc_is_modified      (AW_window *aww,AW_CL cd1, AW_CL cd2);
void        ED4_quit            (AW_window *aww,AW_CL cd1, AW_CL cd2);

void        ED4_motion_cb           (AW_window *aww,AW_CL cd1, AW_CL cd2);
void        ED4_vertical_change_cb      (AW_window *aww,AW_CL cd1, AW_CL cd2);
void        ED4_horizontal_change_cb    (AW_window *aww,AW_CL cd1, AW_CL cd2);
void        ED4_scrollbar_change_cb     (AW_window *aww, AW_CL cd1, AW_CL cd2);

void        ED4_no_dangerous_modes      (void);
void        group_species_cb        (AW_window *aww, AW_CL cd1, AW_CL cd2);
AW_window   *ED4_create_group_species_by_field_window( AW_root *aw_root);

void        ED4_timer           (AW_root   *ar , AW_CL cd1, AW_CL cd2 );
void        ED4_timer_refresh       ();

ED4_returncode  update_terminal_extension   (void **arg1, void **arg2, ED4_base *this_object );

void        ED4_load_new_config         (char *string);
void        ED4_start_editor_on_configuration   (AW_window *aww);
AW_window   *ED4_start_editor_on_old_configuration  (AW_root *awr);
void        ED4_restart_editor          (AW_window *aww, AW_CL,AW_CL);
void        ED4_save_configuration          (AW_window *aww,AW_CL close_flag);
AW_window   *ED4_save_configuration_as_open_window  (AW_root *awr);

extern void ED4_create_all_awars        (AW_root *root, const char *config_name);
void        ED4_set_iupac           (AW_window *aww, char *awar_name, bool callback_flag);
void        ED4_set_helixnr         (AW_window *aww, char *awar_name, bool callback_flag);
void        ed4_changesecurity      (AW_root *root, AW_CL cd1);
void        ed4_change_edit_mode        (AW_root *root, AW_CL cd1);

ED4_returncode  call_edit           (void **error, void **work_info, ED4_base *object ); // function for editing sequences
ED4_returncode  rebuild_consensus       (void **arg1, void **arg2, ED4_base *object);

void        ED4_quit_editor         (AW_window *aww, AW_CL cd1, AW_CL cd2 );                //Be Careful: Is this the last window?
void        ED4_load_data           (AW_window *aww, AW_CL cd1, AW_CL cd2 );
void        ED4_save_data           (AW_window *aww, AW_CL cd1, AW_CL cd2 );
void        ED4_refresh_window      (AW_window *aww, AW_CL cd1, AW_CL cd2 );

void        ED4_store_curpos        (AW_window *aww, AW_CL cd1, AW_CL cd2 );
void        ED4_restore_curpos      (AW_window *aww, AW_CL cd1, AW_CL cd2 );
void        ED4_clear_stored_curpos     (AW_window *aww, AW_CL cd1, AW_CL cd2 );
void        ED4_helix_jump_opposite     (AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/);
void        ED4_jump_to_cursor_position (AW_window *aww, char *awar_name, bool callback_flag);
void        ED4_remote_set_cursor_cb    (AW_root *awr, AW_CL, AW_CL);
void        ED4_change_cursor       (AW_window */*aww*/, AW_CL /*cd1*/, AW_CL /*cd2*/);
void        ED4_set_reference_species   (AW_window *aww, AW_CL cd1, AW_CL cd2 );

void        ED4_show_detailed_column_stats  (AW_window *aww, AW_CL, AW_CL);
void        ED4_set_col_stat_threshold  (AW_window *aww, AW_CL, AW_CL);

void        ED4_new_editor_window       (AW_window *aww, AW_CL cd1, AW_CL cd2 );

AW_window   *ED4_create_consensus_definition_window (AW_root *root);
void        ED4_create_consensus_awars      (AW_root *aw_root);
void        ED4_consensus_definition_changed    (AW_root*, AW_CL,AW_CL);
void        ED4_consensus_display_changed       (AW_root*, AW_CL,AW_CL);

AW_window   *ED4_create_level_1_options_window  (AW_root *root);
void        ED4_compression_toggle_changed_cb   (AW_root *root, AW_CL cd1, AW_CL cd2);

AW_window   *ED4_create_new_seq_window      (AW_root *root, AW_CL cl_creation_mode);

void ED4_jump_to_current_species     (AW_window *, AW_CL);
void ED4_get_and_jump_to_actual      (AW_window *, AW_CL);
void ED4_get_and_jump_to_actual_from_menu    (AW_window *aw, AW_CL cl, AW_CL);
void ED4_get_and_jump_to_species     (GB_CSTR species_name);
void ED4_get_marked_from_menu        (AW_window *, AW_CL, AW_CL);
void ED4_selected_species_changed_cb     (AW_root *aw_root);
void ED4_selected_SAI_changed_cb     (AW_root *aw_root);

extern int  ED4_elements_in_species_container; // # of elements in species container
void        ED4_undo_redo               (AW_window*, AW_CL undo_type);

ED4_species_name_terminal *ED4_find_species_name_terminal(const char *species_name);
ED4_multi_species_manager *ED4_new_species_multi_species_manager(void); // returns manager into which new species should be inserted

void ED4_activate_col_stat(AW_window *aww,AW_CL, AW_CL);

void ED4_compression_changed_cb(AW_root *awr);

// functions passed to external c-functions (i.e. as callbacks) have to be declared as 'extern "C"'

extern "C" {
    void    ED4_species_container_changed_cb(GBDATA *gbd, int *cl, GB_CB_TYPE gbtype);
    void    ED4_sequence_changed_cb(GBDATA *gb_seq, int *cl, GB_CB_TYPE gbtype);
    void    ED4_alignment_length_changed(GBDATA *gb_alignment_len, int *dummy, GB_CB_TYPE gbtype);
}

#endif

struct AWTC_faligner_cd;
void ED4_init_faligner_data(AWTC_faligner_cd *faligner_data);




