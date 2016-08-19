#ifndef ED4_CLASS_HXX
#define ED4_CLASS_HXX

#ifndef AW_FONT_GROUP_HXX
#include <aw_font_group.hxx>
#endif
#ifndef POS_RANGE_H
#include <pos_range.h>
#endif

#define e4_assert(bed) arb_assert(bed)

#define concat(a,b) a##b

#ifdef DEBUG
# define IMPLEMENT_DUMP         // comment out this line to skip compilation of the dump() methods
#endif

#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef ED4_DEFS_HXX
#include "ed4_defs.hxx"
#endif
#ifndef ED4_SEARCH_HXX
#include "ed4_search.hxx"
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef BI_BASEPOS_HXX
#include <BI_basepos.hxx>
#endif
#ifndef DOWNCAST_H
#include <downcast.h>
#endif
#ifndef CHARTABLE_H
#include "chartable.h"
#endif

#if defined(IMPLEMENT_DUMP) // ------------------------------

#if 0
#define WARN(msg) void dummy_to_produce_a_warning(int msg) {}
#else
#define WARN(msg)
#endif

#define EMPTY_FUNCTION(name)         void name() const {}
#define EMPTY_FUNCTION_VERBOSE(name) void name(int je_mappelle_##name) const {}

#define DERIVABLEFROM(base) concat(prohibited_leafclass_derivation,base)

#define COMMON_FOR_BASES()                      \
    virtual void dump(size_t indent) const = 0; \

#define COMMON_FOR_DERIVABLE(self)               \
    void dump_base(size_t indent) const;         \
    EMPTY_FUNCTION(DERIVABLEFROM(self));         \
        
#define COMMON_FOR_DERIVED(mybase)                                              \
    void dump_my_base(size_t indent) const { mybase::dump_base(indent); }       \
    virtual const char *baseclassname() const { return #mybase; }               \
    
#define COMMON_FOR_INSTANCIABLE(mybase)                 \
    void dump(size_t indent) const OVERRIDE;            \

#define DECLARE_DUMP_FOR_ROOTCLASS(self)        \
    COMMON_FOR_BASES();                         \
    COMMON_FOR_DERIVABLE(self);                 \

#define DECLARE_DUMP_FOR_BASECLASS(self,mybase) \
    COMMON_FOR_BASES();                         \
    COMMON_FOR_DERIVABLE(self);                 \
    COMMON_FOR_DERIVED(mybase);                 \

#define DECLARE_DUMP_FOR_MIDCLASS(self,mybase)  \
    WARN(midclass_is_unwanted);                 \
    COMMON_FOR_DERIVABLE(self);                 \
    COMMON_FOR_DERIVED(mybase);                 \
    COMMON_FOR_INSTANCIABLE(mybase);            \
    
#define DECLARE_DUMP_FOR_LEAFCLASS(mybase)              \
    virtual void leaf() { DERIVABLEFROM(mybase)(); }    \
    COMMON_FOR_DERIVED(mybase);                         \
    COMMON_FOR_INSTANCIABLE(mybase);                    \

#else
#define DECLARE_DUMP_FOR_ROOTCLASS(self)
#define DECLARE_DUMP_FOR_BASECLASS(self,mybase)
#define DECLARE_DUMP_FOR_MIDCLASS(self,mybase)
#define DECLARE_DUMP_FOR_LEAFCLASS(mybase)

#endif // IMPLEMENT_DUMP ------------------------------



// #define LIMIT_TOP_AREA_SPACE // if defined, top area is size-limited
#ifdef LIMIT_TOP_AREA_SPACE
#define MAX_TOP_AREA_SIZE 10    // size limit for top-area
#endif

#define ed4_beep() do { fputc(char(7), stdout); fflush(stdout); } while (0)

enum PositionType {
    ED4_POS_SEQUENCE, 
    ED4_POS_ECOLI, 
    ED4_POS_BASE, 
};
    
// ****************************************
// needed prototypes, definitions below
// ****************************************

class ED4_Edit_String;
class ED4_area_manager;
class ED4_abstract_group_manager;
class ED4_base;
class ED4_bracket_terminal;
class BaseFrequencies;
class ED4_columnStat_terminal;
class ED4_consensus_sequence_terminal;
class ED4_cursor;
class ED4_device_manager;
class ED4_folding_line;
class ED4_group_manager;
class ED4_line_terminal;
class ED4_main_manager;
class ED4_manager;
class ED4_multi_name_manager;
class ED4_multi_sequence_manager;
class ED4_multi_species_manager;
class ED4_name_manager;
class ED4_pure_text_terminal;
class ED4_reference_terminals;
class ED4_remap;
class ED4_root;
class ED4_root_group_manager;
class ED4_sequence_info_terminal;
class ED4_sequence_manager;
class ED4_sequence_terminal;
class ED4_orf_terminal;
class ED4_spacer_terminal;
class ED4_species_manager;
class ED4_species_name_terminal;
class ED4_species_pointer;
class ED4_terminal;
class ED4_text_terminal;
class ED4_tree_terminal;
class ED4_window;

class AP_tree;
class ED4_reference;
class ED4_seq_colors;
class BI_ecoli_ref;
class AW_helix;
class arb_progress;
class ST_ML;
class ed_key;
class ConsensusBuildParams;

template <class T> class ED4_list;      // derived from Noncopyable
template <class T> class ED4_list_elem; // derived from Noncopyable

typedef ED4_list<ED4_base>      ED4_base_list;
typedef ED4_list_elem<ED4_base> ED4_base_list_elem;

typedef ED4_list<ED4_selection_entry>      ED4_selected_list;
typedef ED4_list_elem<ED4_selection_entry> ED4_selected_elem;


struct EDB_root_bact {
    char *make_string();
    char *make_top_bot_string();

    void calc_no_of_all(const char *string_to_scan,       // group gets the number of groups in string_to_scan,
                        long *group,                // species gets the number of species in string_to_scan
                        long *species);

    ED4_returncode fill_species(ED4_multi_species_manager *multi_species_manager,
                                ED4_reference_terminals&   refterms,
                                const char                *str,
                                int                       *index,
                                int                        group_depth,
                                arb_progress              *progress);

    ED4_returncode fill_data(ED4_multi_species_manager *multi_species_manager,
                             ED4_reference_terminals&   refterms,
                             char                      *str,
                             int                        group_depth,
                             ED4_datamode               datamode);      // flag only needed for loading a new configuration

    ED4_returncode search_sequence_data_rek(ED4_multi_sequence_manager *multi_sequence_manager,
                                            ED4_reference_terminals&    refterms,
                                            GBDATA                     *gb_datamode,
                                            int                         count_too,
                                            ED4_index                  *max_seq_terminal_length,
                                            ED4_alignment               alignment_flag,
                                            bool                        isSAI);

    void scan_string(ED4_multi_species_manager *parent,
                     ED4_reference_terminals&   refterms,
                     const char                *str,
                     int                       *index,
                     arb_progress&              progress);

    void save_current_config(char *confname);

    EDB_root_bact() {}
};

#define SPECIFIED_OBJECT_TYPES 21

class ED4_objspec : public Noncopyable {
    static bool object_specs_initialized;
    static bool descendants_uptodate;

    mutable ED4_level used_children;         // in any object of this type
    mutable ED4_level possible_descendants;  // below any object of this type (depends on used_children)
    mutable ED4_level allowed_descendants;   // below any object of this type (depends on allowed_children)

    void calc_descendants() const;

public:
    ED4_properties static_prop;
    ED4_level      level;
    ED4_level      allowed_children;
    ED4_level      handled_level;
    ED4_level      restriction_level;

    ED4_objspec(ED4_properties static_prop_, ED4_level level_, ED4_level allowed_children_, ED4_level handled_level_, ED4_level restriction_level_);

#if defined(IMPLEMENT_DUMP)
    void dump(size_t indent) const;
#endif // IMPLEMENT_DUMP

    static void init_object_specs();

    bool is_manager() const { return static_prop & PROP_IS_MANAGER; }
    bool is_terminal() const { return static_prop & PROP_IS_TERMINAL; }

    bool allowed_to_contain(ED4_level child_level) const {
        // e4_assert(object_specs_initialized); // @@@ cant check here.. but where
        e4_assert(is_manager()); // terminals can't contain anything - your test is senseless
        return allowed_children & child_level;
    }

    void announce_added(ED4_level child_level) const {
        e4_assert(allowed_to_contain(child_level));
        used_children          = ED4_level(used_children|child_level);
        descendants_uptodate = false;
    }

    static void recalc_descendants();

    ED4_level get_possible_descendants() const { 
        e4_assert(is_manager());
        if (!descendants_uptodate) recalc_descendants();
        return possible_descendants;
    }
    ED4_level get_allowed_descendants() const { // (allowed = possible + those allowed to add, but not added anywhere)
        e4_assert(is_manager());
        if (!descendants_uptodate) recalc_descendants();
        return allowed_descendants;
    }
};

class ED4_folding_line : virtual Noncopyable {
    AW_pos            dimension; // amount of pixel folded away
    AW_pos            pos;       // window position of folding line (x or y, only owner knows which coordinate is folded)
    ED4_folding_line *next;

    ED4_folding_line *insert(ED4_folding_line *fl) {
        e4_assert(!fl->next);
        if (pos <= fl->pos) { // insert behind
            next = next ? next->insert(fl) : fl;
            return this;
        }

        fl->next = this;
        return fl;
    }

public:

    ED4_folding_line(AW_pos world, AW_pos dim) : dimension(dim), next(0) { set_pos(world-dimension); }

    ~ED4_folding_line() { delete next; }

    void insertAs(ED4_folding_line*& ptr) {
        ED4_folding_line *other = ptr;
        e4_assert(knownNonNull(this));
        ptr = other ? other->insert(this) : this;
    }

    ED4_folding_line *delete_member(ED4_folding_line *fl) {
        ED4_folding_line *result = this;
        if (this == fl) {
            result = next;
            next   = NULL;
            delete this;
        }
        return result;
    }

    AW_pos get_dimension() const { return dimension; }
    const ED4_folding_line *get_next() const { return next; }

    void warn_illegal_dimension();

    void set_dimension(AW_pos dim) { dimension = dim; warn_illegal_dimension(); }
    void add_to_dimension(AW_pos offset) { dimension += offset; warn_illegal_dimension(); }

    void set_pos(AW_pos p) { pos = p; }
    AW_pos get_pos() const { return pos; }

    AW_pos win2world(AW_pos win) const {
        if (win<pos) return win;
        return (next ? next->win2world(win) : win)+dimension;
    }
    AW_pos world2win(AW_pos world) const {
        if (world<pos) return world;
        world -= dimension;
        if (!next) return world;
        return next->world2win(world);
    }
};

struct ED4_scroll_links {
    ED4_base *link_for_hor_slider;
    ED4_base *link_for_ver_slider;

    ED4_scroll_links() : link_for_hor_slider(0), link_for_ver_slider(0) {}
};

class ED4_foldable : virtual Noncopyable {
    ED4_folding_line *horizontal_fl;
    ED4_folding_line *vertical_fl;
protected:
    void reset() {
        delete horizontal_fl;
        delete vertical_fl;
        horizontal_fl = NULL;
        vertical_fl   = NULL;
    }
    bool is_reset() const { return !horizontal_fl && !vertical_fl; }
public:

    ED4_foldable() : horizontal_fl(NULL), vertical_fl(NULL) {}
    ~ED4_foldable() { reset(); }

    const ED4_folding_line *get_horizontal_folding() { return horizontal_fl; }
    const ED4_folding_line *get_vertical_folding() { return vertical_fl; }

    void world_to_win_coords(AW_pos *xPtr, AW_pos *yPtr) const { // @@@ old style
        // Calculates transformation from world to window coordinates in a given window.
        // world-coordinates inside folded range result in window coordinates lower than folding line position.
        e4_assert(!is_reset());
        *xPtr = vertical_fl->world2win(*xPtr);
        *yPtr = horizontal_fl->world2win(*yPtr);
    }
    void win_to_world_coords(AW_pos *xPtr, AW_pos *yPtr) const { // @@@ old style
        // calculates transformation from window to world coordinates in a given window
        e4_assert(!is_reset());
        *xPtr = vertical_fl->win2world(*xPtr);
        *yPtr = horizontal_fl->win2world(*yPtr);
    }

    AW::Position world_to_win_coords(const AW::Position& pos) const {
        e4_assert(!is_reset());
        return AW::Position(vertical_fl->world2win(pos.xpos()),
                            horizontal_fl->world2win(pos.ypos()));
    }
    AW::Position win_to_world_coords(const AW::Position& pos) const {
        e4_assert(!is_reset());
        return AW::Position(vertical_fl->win2world(pos.xpos()),
                            horizontal_fl->win2world(pos.ypos()));
    }

    ED4_folding_line *insert_folding_line(AW_pos pos, AW_pos dimension, ED4_properties prop);
    void              delete_folding_line(ED4_folding_line *fl, ED4_properties prop);
};


class ED4_scrolled_rectangle : virtual Noncopyable {
    ED4_folding_line *scroll_bottom;
    ED4_folding_line *scroll_right;
    ED4_folding_line *scroll_top;
    ED4_folding_line *scroll_left;

    ED4_base *x_link;
    ED4_base *y_link;
    ED4_base *width_link;
    ED4_base *height_link;

    AW::Rectangle world;

    bool folding_dimensions_calculated; // flag to ensure calc_bottomRight_folding_dimensions is called before get_window_rect
    
    void init_links() {
        x_link      = 0;
        y_link      = 0;
        width_link  = 0;
        height_link = 0;
    }

    void init_folding_lines() {
        scroll_top    = 0;
        scroll_bottom = 0;
        scroll_left   = 0;
        scroll_right  = 0;

        folding_dimensions_calculated = false;
    }
    void init_pos_size() { world = AW::Rectangle(AW::Origin, AW::ZeroVector); }

    void init() {
        init_folding_lines();
        init_pos_size();
        init_links();
    }

    void update_folding_line_positions() {
        scroll_top->set_pos(world.top());
        scroll_left->set_pos(world.left());
    }

public:

    ED4_scrolled_rectangle() { init(); }

    AW_pos bottom() const { return world.bottom(); }
    AW_pos right() const { return world.right(); }

    void reset(ED4_foldable& owner) {
        destroy_folding_lines(owner);
        init_pos_size();
    }

    AW_pos top_dim() const { return scroll_top->get_dimension(); }
    AW_pos left_dim() const { return scroll_left->get_dimension(); }

    bool exists() const { return scroll_top && scroll_bottom && scroll_left && scroll_right; }
    bool is_linked() const {
        if (x_link) {
            e4_assert(y_link);
            e4_assert(width_link);
            e4_assert(height_link);
            return true;
        }
        e4_assert(!y_link);
        e4_assert(!width_link);
        e4_assert(!height_link);
        return false;
    }
    void link(ED4_base *x, ED4_base *y, ED4_base *w, ED4_base *h) {
        e4_assert(x && y && w && h);

        x_link      = x;
        y_link      = y;
        width_link  = w;
        height_link = h;
    }

    void replace_x_width_link_to(ED4_base *old_link, ED4_base *new_link) {
        if (x_link == old_link)     x_link     = new_link;
        if (width_link == old_link) width_link = new_link;
    }

    void add_to_top_left_dimension(int dx, int dy) {
        scroll_left->add_to_dimension(dx);
        scroll_top->add_to_dimension(dy);
    }

    void scroll(int dx, int dy) {
        scroll_left->add_to_dimension(-dx);
        scroll_top->add_to_dimension(-dy);
        scroll_right->add_to_dimension(dx);
        scroll_bottom->add_to_dimension(dy);
    }

    AW::Rectangle get_window_rect() const {
        e4_assert(folding_dimensions_calculated);
        return AW::Rectangle(scroll_left->get_pos(), scroll_top->get_pos(),
                             scroll_right->get_pos(), scroll_bottom->get_pos());
    }

    AW::Rectangle get_world_rect() const;

    void set_rect(const AW::Rectangle& rect) { world = rect; }
    void set_rect_and_update_folding_line_positions(const AW::Rectangle& rect) {
        set_rect(rect);
        update_folding_line_positions();
    }

    void calc_bottomRight_folding_dimensions(int area_width, int area_height) {
        area_width  -= SLIDER_OFFSET;
        area_height -= SLIDER_OFFSET;

        AW_pos dim;
        if (bottom() > area_height) {   // our world doesn't fit vertically in our window
            dim = bottom()-area_height; // calc dimension of both horizontal folding lines
            scroll_top->set_dimension(std::min(dim, scroll_top->get_dimension()));
            scroll_bottom->set_dimension(std::max(0, int(dim - scroll_top->get_dimension())));
        }
        else {
            dim = 0;
            scroll_bottom->set_dimension(0);
            scroll_top->set_dimension(0);
        }

        e4_assert(dim == (scroll_top->get_dimension()+scroll_bottom->get_dimension()));
        scroll_bottom->set_pos(world.bottom()-dim+SLIDER_OFFSET);

        if (right()>area_width) {     // our world doesn't fit horizontally in our window
            dim = right()-area_width; // calc dimension of both vertical folding lines
            scroll_left->set_dimension(std::min(dim, scroll_left->get_dimension()));
            scroll_right->set_dimension(std::max(0, int(dim - scroll_left->get_dimension())));
        }
        else {
            dim = 0;
            scroll_right->set_dimension(0);
            scroll_left->set_dimension(0);
        }

        e4_assert(dim == (scroll_left->get_dimension()+scroll_right->get_dimension()));
        scroll_right->set_pos(world.right()-dim+SLIDER_OFFSET);

        folding_dimensions_calculated = true;
    }

    void create_folding_lines(ED4_foldable& owner, const AW::Rectangle& rect, int area_width, int area_height) {
        scroll_top  = owner.insert_folding_line(rect.top(), 0, PROP_HORIZONTAL);
        scroll_left = owner.insert_folding_line(rect.left(), 0, PROP_VERTICAL);

        AW_pos dim = 0;
        if (rect.bottom() > area_height) dim = rect.bottom() - area_height;
        scroll_bottom = owner.insert_folding_line(rect.bottom(), dim, PROP_HORIZONTAL);

        dim = 0;
        if (rect.right() > area_width) dim = rect.right() - area_width;
        scroll_right = owner.insert_folding_line(rect.right(), dim, PROP_VERTICAL);
    }

    void destroy_folding_lines(ED4_foldable& owner) {
        if (scroll_top)    owner.delete_folding_line(scroll_top,    PROP_HORIZONTAL);
        if (scroll_bottom) owner.delete_folding_line(scroll_bottom, PROP_HORIZONTAL);
        if (scroll_left)   owner.delete_folding_line(scroll_left,   PROP_VERTICAL);
        if (scroll_right)  owner.delete_folding_line(scroll_right,  PROP_VERTICAL);

        init_folding_lines();
    }
};

class ED4_base_position : private BasePosition { // derived from a Noncopyable
    const ED4_terminal *calced4term; // if calced4term!=NULL => callback is bound to its species manager
    bool needUpdate;

    void calc4term(const ED4_terminal *term);
    void set_term(const ED4_terminal *term) {
        if (calced4term != term || needUpdate) {
            calc4term(term);
        }
    }
    void remove_changed_cb();

public:

    ED4_base_position()
        : calced4term(NULL),
          needUpdate(true)
    {}

    ~ED4_base_position() {
        remove_changed_cb();
    }

    void invalidate() {
        needUpdate = true;
    }

    void announce_deletion(const ED4_terminal *term) {
        if (term == calced4term) {
            invalidate();
            remove_changed_cb();
        }
        e4_assert(calced4term != term);
    }
    void prepare_shutdown() {
        if (calced4term) announce_deletion(calced4term);
    }

    int get_base_position(const ED4_terminal *base, int sequence_position);
    int get_sequence_position(const ED4_terminal *base, int base_position);

    int get_base_count(const ED4_terminal *term) { set_term(term); return base_count(); }
    int get_abs_len(const ED4_terminal *term) { set_term(term); return abs_count(); }
};

class ED4_CursorShape;


enum ED4_CursorType {
    ED4_RIGHT_ORIENTED_CURSOR,
    ED4_RIGHT_ORIENTED_CURSOR_THIN,
    ED4_TRADITIONAL_CURSOR,
    ED4_TRADITIONAL_CURSOR_BOTTOM,
    ED4_TRADITIONAL_CURSOR_CONNECTED,
    ED4_FUCKING_BIG_CURSOR,

    ED4_CURSOR_TYPES

};

extern bool ED4_update_global_cursor_awars_allowed; // update selected species/SAI/cursor position

struct ED4_TerminalPredicate {
    virtual ~ED4_TerminalPredicate() {}
    virtual bool fulfilled_by(const ED4_terminal *) const = 0;
};

class ED4_WinContextFree { // denies usage of the following functions in classes derived from this
    AW_device   *current_device();
    ED4_window  *current_ed4w();
    AW_window   *current_aww();
    ED4_cursor&  current_cursor();
public:
    void avoid_warning() {}
};

class ED4_cursor : virtual Noncopyable, virtual ED4_WinContextFree {
    ED4_window                *win;
    ED4_index                  cursor_abs_x;    // absolute (to terminal) x-position of cursor (absolute world coordinate of edit window)
    int                        screen_position; // number of displayed characters leading the cursor
    mutable ED4_base_position  base_position;   // # of bases left of cursor
    ED4_CursorType             ctype;
    ED4_CursorShape           *cursor_shape;

    ED4_returncode  draw_cursor(AW_pos x, AW_pos y);
    ED4_returncode  delete_cursor(AW_pos del_mark,  ED4_base *target_terminal);

    void updateAwars(bool new_term_selected);

public:

    bool          allowed_to_draw; // needed for cursor handling
    ED4_terminal *owner_of_cursor;

    bool is_partly_visible() const;
    bool is_completely_visible() const;

    bool is_hidden_inside_group() const;

    void changeType(ED4_CursorType typ);
    ED4_CursorType  getType() const { return ctype; }

    void redraw() { changeType(getType()); }

    ED4_returncode HideCursor(); // deletes cursor and does refresh
    ED4_returncode move_cursor(AW_event *event);
    ED4_returncode show_clicked_cursor(AW_pos click_xpos, ED4_terminal *target_terminal);
    ED4_returncode show_cursor_at(ED4_terminal *target_terminal, ED4_index what_pos);
    ED4_returncode ShowCursor(ED4_index offset_x, ED4_cursor_move move, int move_pos = 1);

    int get_sequence_pos() const;
    int get_screen_pos() const { return screen_position; }

    long get_abs_x() const   { return cursor_abs_x; }
    void set_abs_x();

    int base2sequence_position(int base_pos) const { return base_position.get_sequence_position(owner_of_cursor, base_pos); }
    int sequence2base_position(int seq_pos) const { return base_position.get_base_position(owner_of_cursor, seq_pos); }

    int get_base_position() const { return sequence2base_position(get_sequence_pos()); }

    void prepare_shutdown() { base_position.prepare_shutdown(); }

    void jump_screen_pos(int screen_pos, ED4_CursorJumpType jump_type);
    void jump_sequence_pos(int sequence_pos, ED4_CursorJumpType jump_type);
    void jump_base_pos(int base_pos, ED4_CursorJumpType jump_type);

    int get_screen_relative_pos() const;
    void set_screen_relative_pos(int scroll_to_relpos);

    void set_to_terminal(ED4_terminal *terminal, int seq_pos, ED4_CursorJumpType jump_type);

    inline bool in_species_seq_terminal() const;
    inline bool in_consensus_terminal() const;
    inline bool in_SAI_terminal() const;
    
    void announce_deletion(ED4_terminal *object) {
        base_position.announce_deletion(object);
        if (object == owner_of_cursor) owner_of_cursor = NULL; // no need to delete the cursor (deletion triggers full redraw)
    }

    void init();

    ED4_window *window() const { return win; }

    ED4_cursor(ED4_window *win);
    ~ED4_cursor();
};

class ED4_window : public ED4_foldable, virtual ED4_WinContextFree { // derived from Noncopyable
    void set_scrollbar_indents();

public:
    AW_window_menu_modes   *aww;   // Points to Window
    ED4_window             *next;
    int                     slider_pos_horizontal;
    int                     slider_pos_vertical;
    ED4_scrolled_rectangle  scrolled_rect;
    int                     id;    // unique id in window-list
    ED4_coords              coords;

    static int no_of_windows;

    char awar_path_for_cursor[50];                  // position in current sequence, range = [1;len]
    char awar_path_for_Ecoli[50];                   // position relative to ecoli
    char awar_path_for_basePos[50];                 // base position in current sequence (# of bases left to cursor)
    char awar_path_for_IUPAC[50];                   // IUPAC decoder content for current position
    char awar_path_for_helixNr[50];                 // # of helix (or 0) for current position

    bool       is_hidden;
    ED4_cursor cursor;

    // ED4_window controlling functions
    static ED4_window *insert_window(AW_window_menu_modes *new_aww); // append to window list

    void        delete_window(ED4_window *window);  // delete from window list
    void        reset_all_for_new_config(); // reset structures for loading new config
    ED4_window *get_matching_ed4w(AW_window *aww);

    void announce_deletion(ED4_terminal *object) { cursor.announce_deletion(object); }
    
    // functions concerned the scrolled area
    void update_scrolled_rectangle();
    ED4_returncode scroll_rectangle(int dx, int dy);
    ED4_returncode set_scrolled_rectangle(ED4_base *x_link, ED4_base *y_link, ED4_base *width_link, ED4_base *height_link);

    bool scrollbars_and_scrolledRect_inSync() const {
        // Scrolling in EDIT4 window uses redundant data
        // - dimension of folding lines
        // - slider positions in AW_window and ED4_window
        // This function checks whether they are in sync.
        
        bool inSync                    = 
            (scrolled_rect.top_dim()  == aww->slider_pos_vertical) &&
            (scrolled_rect.left_dim() == aww->slider_pos_horizontal);

#if defined(DEBUG)
        if (!inSync) {
            fputs("scrollbars not in sync with scrolled_rect:\n", stderr);
#if defined(ARB_GTK)
#define POSTYPE "%zu"
#else // ARB_MOTIF
#define POSTYPE "%i"
#endif
            fprintf(stderr, "    aww->slider_pos_vertical  =" POSTYPE " scrolled_rect->top_dim() =%f\n", aww->slider_pos_vertical,   scrolled_rect.top_dim());
            fprintf(stderr, "    aww->slider_pos_horizontal=" POSTYPE " scrolled_rect->left_dim()=%f\n", aww->slider_pos_horizontal, scrolled_rect.left_dim());
        }
#endif

        return inSync;
    }

    void check_valid_scrollbar_values() { e4_assert(scrollbars_and_scrolledRect_inSync()); }

    bool shows_xpos(int x) const { return x >= coords.window_left_clip_point && x <= coords.window_right_clip_point; }
    bool partly_shows(int x1, int y1, int x2, int y2) const;
    bool completely_shows(int x1, int y1, int x2, int y2) const;
    
    void update_window_coords();

    AW_device *get_device() const { return aww->get_device(AW_MIDDLE_AREA); }

    ED4_window(AW_window_menu_modes *window);
    ~ED4_window();
};

class ED4_container : virtual Noncopyable {
    // contains children of a manager

    ED4_base  **memberList;
    ED4_index   no_of_members;   // How much members are in the list
    ED4_index   size_of_list;    // allocated size

    void correct_insert_position(ED4_index& index);
    void resize(ED4_index needed_size);

    void shift_list(ED4_index start_index, int length);

protected:

    bool valid_index(ED4_index idx) const { return idx >= 0 && idx<size_of_list; }
    bool existing_index(ED4_index idx) const { return idx >= 0 && idx<no_of_members; }

public:

    ED4_manager *owner() { return DOWNCAST(ED4_manager*, this); }
    const ED4_manager *owner() const { return const_cast<ED4_container*>(this)->owner(); }

    ED4_base* member(ED4_index i) const { e4_assert(i>=0 && i<size_of_list); return memberList[i]; }

    ED4_index members() const { return no_of_members; }
    bool empty() const { return !members(); }
    void clear();

    void insert_member(ED4_base *new_member); // only used to move members with mouse
    void append_member(ED4_base *new_member);

    // an array is chosen instead of a linked list, because destructorhandling is more comfortable in various destructors (manager-destructors)

    ED4_returncode remove_member(ED4_base *member);
    ED4_index search_member(ED4_extension *location, ED4_properties prop);       // search member

    ED4_returncode search_target_species(ED4_extension *location, ED4_properties prop, ED4_base **found_member, ED4_level return_level);

    ED4_returncode move_member(ED4_index old_pos, ED4_index new_pos);

#if defined(IMPLEMENT_DUMP)
    void dump_container(size_t indent) const;
#endif // IMPLEMENT_DUMP

#if defined(ASSERTION_USED)
    bool members_ok() const;
#endif // ASSERTION_USED

    ED4_container();
    virtual ~ED4_container();
};

// ----------------------------
//      ED4_species_pointer

class ED4_species_pointer : virtual Noncopyable {
    // @@@ shall be renamed into ED4_gbdata_pointer to reflect general usage

    GBDATA *species_pointer;    // points to database

    void addCallback(ED4_base *base);
    void removeCallback(ED4_base *base);

public:

    ED4_species_pointer();
    ~ED4_species_pointer();

    GBDATA *Get() const { return species_pointer; }
    void Set(GBDATA *gbd, ED4_base *base);
    void notify_deleted() {
        species_pointer=0;
    }
};

// ------------------------
//      callback types

template <class BASE, class BASECB>
class ED4_cb_list {
    std::list<BASECB> callbacks;
public:
#if defined(ASSERTION_USED)
    ED4_cb_list() {}
    ~ED4_cb_list() {
        e4_assert(empty()); // calling ED4_root::remove_all_callbacks() did not remove all callbacks!
    }
#endif
    void add_cb(BASECB cb) { callbacks.push_back(cb); }
    void remove_cb(BASECB cb) { callbacks.remove(cb); }

    void call(BASE *b) {
        for (typename std::list<BASECB>::iterator cb = callbacks.begin(); cb != callbacks.end();) {
            typename std::list<BASECB>::iterator curr = cb;
            ++cb;
            (*curr)(b); // Note: may be removed while called
        }
    }
    void clear() { callbacks.clear(); }
    bool empty() const { return callbacks.empty(); }
};

// declare callback types used in ED4_cb_list:
DECLARE_CBTYPE_FVV_AND_BUILDERS(ED4_managerCallback,         void, ED4_manager*);         // generates makeED4_managerCallback
DECLARE_CBTYPE_FVV_AND_BUILDERS(ED4_species_managerCallback, void, ED4_species_manager*); // generates makeED4_species_managerCallback

// -----------------
//      ED4_base

class ED4_base;

DECLARE_CBTYPE_FVV_AND_BUILDERS(ED4_route_cb, ARB_ERROR, ED4_base*); // generates makeED4_route_cb

enum ED4_species_type {
    ED4_SP_NONE, 
    ED4_SP_SPECIES, 
    ED4_SP_SAI, 
    ED4_SP_CONSENSUS,
};

class ED4_base : virtual Noncopyable {
    // base object

    ED4_species_pointer my_species_pointer;

    // cache world coordinates:

    static int           currTimestamp;
    mutable AW::Position lastPos;
    mutable int          timestamp;

    ED4_base_list *linked_objects;                  // linked list of objects which are depending from this object

    virtual bool set_dynamic_size() { // @@@ make abstract?
        // default behavior == "keep current size"
        return false; // = "nothing changed"

        // overload to implement dynamic sizing
    }

public:
    const ED4_objspec& spec;           // contains information about Objectproperties

    ED4_manager *parent;                            // Points to parent


    ED4_properties   dynamic_prop;                  // contains info about what i am, what i can do, what i should do
    char            *id;                            // globally unique name in hierarchy
    ED4_index        index;                         // defines the order of child objects
    ED4_base        *width_link;                    // concerning the hierarchy
    ED4_base        *height_link;                   // concerning the hierarchy
    ED4_extension    extension;                     // contains relative info about graphical properties
    ED4_update_info  update_info;                   // info about things to be done for the object, i.e. refresh; flag structure
    struct {
        unsigned int hidden : 1;                    // flag whether object is hidden or not
    } flag;

    void draw_bb(int color);

    DECLARE_DUMP_FOR_ROOTCLASS(ED4_base);

    // function for species_pointer

    GBDATA *get_species_pointer() const { return my_species_pointer.Get(); }
    void set_species_pointer(GBDATA *gbd) { my_species_pointer.Set(gbd, this); }
    int has_callback() const { return get_species_pointer()!=0; }

    // callbacks

    virtual void changed_by_database();
    virtual void deleted_from_database();

    // functions concerned with graphic output
    int adjust_clipping_rectangle();
    virtual ED4_returncode  Show(int refresh_all=0, int is_cleared=0) = 0;

    virtual bool calc_bounding_box() = 0;
    void resize_dynamic() { if (set_dynamic_size()) { request_resize(); } }

    ED4_returncode  clear_background(int color=0);

    void set_links(ED4_base *width_ref, ED4_base *height_ref);
    void set_both_links(ED4_base *ref) { set_links(ref, ref); }

    // functions concerned with special initialization
    void set_property(ED4_properties prop) { dynamic_prop = (ED4_properties) (dynamic_prop | prop); }
    void clr_property(ED4_properties prop) { dynamic_prop = (ED4_properties) (dynamic_prop & ~prop); }
    bool has_property(ED4_properties prop) const { return dynamic_prop & prop; }
    
    // functions concerned with coordinate transformation

    void calc_rel_coords(AW_pos *x, AW_pos *y);

    void calc_world_coords(AW_pos *x, AW_pos *y) const {
        update_world_coords_cache();
        *x = lastPos.xpos();
        *y = lastPos.ypos();
    }
    const AW::Position& calc_world_coords() const {
        update_world_coords_cache();
        return lastPos;
    }

    void update_world_coords_cache() const {
        bool cache_up_to_date = timestamp == currTimestamp;
        if (!cache_up_to_date) {
            if (parent) {
                ED4_base *pab = (ED4_base*)parent;
                lastPos = pab->calc_world_coords();
            }
            else {
                lastPos = AW::Origin;
            }
            lastPos.move(extension.get_parent_offset());
            timestamp = currTimestamp;
        }
    }

    static void touch_world_cache() {
        currTimestamp++;
    }

    AW::Rectangle get_win_area(ED4_window *ed4w) const {
        AW::Position pos = ed4w->world_to_win_coords(calc_world_coords());
        return AW::Rectangle(pos, extension.get_size()-AW::Vector(1, 1));
    }

    // functions which refer to the object as a child, i.e. travelling down the hierarchy
    virtual void request_refresh(int clear=1) = 0;

    inline void request_resize();
    void request_resize_of_linked();
    void resize_requested_by_link(ED4_base *link);
    virtual void resize_requested_children() = 0;

    virtual void delete_requested_children() = 0;
    virtual void Delete()                    = 0;

    inline void set_update();
    virtual void update_requested_children() = 0;

    virtual ED4_returncode  move_requested_by_parent(ED4_move_info *mi)      = 0;
    virtual ED4_returncode  event_sent_by_parent(AW_event *event, AW_window *aww);
    virtual ED4_returncode  move_requested_by_child(ED4_move_info *moveinfo) = 0;


    virtual ED4_returncode  handle_move(ED4_move_info *moveinfo) = 0;

    virtual ARB_ERROR route_down_hierarchy(const ED4_route_cb& cb);

    int calc_group_depth();

    // general purpose functions
    virtual ED4_base *search_ID(const char *id) = 0;

    void  check_all();
    short in_border(AW_pos abs_x, AW_pos abs_y, ED4_movemode mode);
    ED4_returncode set_width();


    ED4_AREA_LEVEL get_area_level(ED4_multi_species_manager **multi_species_manager=0) const; // returns area we belong to and the next multi species manager of the area

    ED4_base *get_parent(ED4_level lev) const;
    void unlink_from_parent();
    bool has_parent(ED4_manager *Parent);
    bool is_child_of(ED4_manager *Parent) { return has_parent(Parent); }

    virtual char       *resolve_pointer_to_string_copy(int *str_len = 0) const;
    virtual const char *resolve_pointer_to_char_pntr(int *str_len = 0) const;

    ED4_group_manager  *is_in_folded_group() const;
    virtual bool is_hidden() const = 0;

    char *get_name_of_species();                      // go from terminal to name of species

    // functions which refer to the selected object(s), i.e. across the hierarchy
    virtual ED4_base        *get_competent_child(AW_pos x, AW_pos y, ED4_properties relevant_prop)=0;
    virtual ED4_base        *get_competent_clicked_child(AW_pos x, AW_pos y, ED4_properties relevant_prop)=0;
    virtual ED4_base        *search_spec_child_rek(ED4_level level);    // recursive search for level

    ED4_terminal        *get_next_terminal();
    ED4_terminal        *get_prev_terminal();

    void generate_configuration_string(GBS_strstruct& buffer);

    virtual ED4_returncode  remove_callbacks();
    
    const ED4_terminal *get_consensus_relevant_terminal() const;

    ED4_base(const ED4_objspec& spec_, GB_CSTR id, AW_pos width, AW_pos height, ED4_manager *parent);
    virtual ~ED4_base();

    // use the following functions to test which derived class we have

    int is_terminal()               const { e4_assert(knownNonNull(this)); return spec.static_prop & PROP_IS_TERMINAL; }

    int is_text_terminal()          const { e4_assert(knownNonNull(this)); return spec.level & (LEV_SPECIES_NAME|LEV_SEQUENCE_INFO|LEV_SEQUENCE_STRING|LEV_PURE_TEXT|LEV_COL_STAT); }

    int is_species_name_terminal()  const { e4_assert(knownNonNull(this)); return spec.level & LEV_SPECIES_NAME; }

    int is_sequence_info_terminal() const { e4_assert(knownNonNull(this)); return spec.level & LEV_SEQUENCE_INFO; }
    int is_sequence_terminal()      const { e4_assert(knownNonNull(this)); return spec.level & LEV_SEQUENCE_STRING; }
    int is_orf_terminal()           const { e4_assert(knownNonNull(this)); return spec.level & LEV_ORF; }

    int is_pure_text_terminal()     const { e4_assert(knownNonNull(this)); return spec.level & LEV_PURE_TEXT; }
    int is_columnStat_terminal()    const { e4_assert(knownNonNull(this)); return spec.level & LEV_COL_STAT; }

    int is_bracket_terminal()       const { e4_assert(knownNonNull(this)); return spec.level & LEV_BRACKET; }
    int is_spacer_terminal()        const { e4_assert(knownNonNull(this)); return spec.level & LEV_SPACER; }
    int is_line_terminal()          const { e4_assert(knownNonNull(this)); return spec.level & LEV_LINE; }

    int is_manager()                const { e4_assert(knownNonNull(this)); return spec.static_prop & PROP_IS_MANAGER; }

    int is_sequence_manager()       const { e4_assert(knownNonNull(this)); return spec.level & LEV_SEQUENCE; }
    int is_multi_name_manager()     const { e4_assert(knownNonNull(this)); return spec.level & LEV_MULTI_NAME; }
    int is_name_manager()           const { e4_assert(knownNonNull(this)); return spec.level & LEV_NAME_MANAGER; }
    int is_multi_species_manager()  const { e4_assert(knownNonNull(this)); return spec.level & LEV_MULTI_SPECIES; }
    int is_multi_sequence_manager() const { e4_assert(knownNonNull(this)); return spec.level & LEV_MULTI_SEQUENCE; }
    int is_device_manager()         const { e4_assert(knownNonNull(this)); return spec.level & LEV_DEVICE; }

    int is_group_manager()          const { e4_assert(knownNonNull(this)); return spec.level & LEV_GROUP; }
    int is_root_group_manager()     const { e4_assert(knownNonNull(this)); return spec.level & LEV_ROOTGROUP; }
    int is_abstract_group_manager() const { e4_assert(knownNonNull(this)); return spec.level & (LEV_GROUP|LEV_ROOTGROUP); }
    
    int is_species_manager()        const { e4_assert(knownNonNull(this)); return spec.level & LEV_SPECIES; }
    int is_area_manager()           const { e4_assert(knownNonNull(this)); return spec.level & LEV_AREA; }

    // use the following functions to cast ED4_base to derived classes:

#define E4B_DECL_CASTOP_helper(Class,toName)            \
    inline const Class *toName() const;                 \
    inline Class *toName();
    
#define E4B_AVOID_CAST__helper(Class,toName,isName)     \
    const Class *toName() const;                        \
    Class *toName();                                    \
    int isName() const;

#define E4B_IMPL_CASTOP_helper(Class,toName,isName)                             \
    const Class *ED4_base::toName() const {                                     \
        e4_assert(isName());                                                    \
        return DOWNCAST(const Class*, this);                                    \
    }                                                                           \
    Class *ED4_base::toName() {                                                 \
        return const_cast<Class*>(const_cast<const ED4_base*>(this)->toName()); \
    }

#define E4B_DECL_CASTOP(name)          E4B_DECL_CASTOP_helper(concat(ED4_,name), concat(to_,name))
#define E4B_AVOID_UNNEEDED_CASTS(name) E4B_AVOID_CAST__helper(concat(ED4_,name), concat(to_,name), concat(is_,name))
#define E4B_IMPL_CASTOP(name)          E4B_IMPL_CASTOP_helper(concat(ED4_,name), concat(to_,name), concat(is_,name))

    E4B_DECL_CASTOP(area_manager);                // to_area_manager
    E4B_DECL_CASTOP(abstract_group_manager);      // to_abstract_group_manager
    E4B_DECL_CASTOP(bracket_terminal);            // to_bracket_terminal
    E4B_DECL_CASTOP(columnStat_terminal);         // to_columnStat_terminal
    E4B_DECL_CASTOP(consensus_sequence_terminal); // to_consensus_sequence_terminal
    E4B_DECL_CASTOP(device_manager);              // to_device_manager
    E4B_DECL_CASTOP(group_manager);               // to_group_manager
    E4B_DECL_CASTOP(line_terminal);               // to_line_terminal
    E4B_DECL_CASTOP(manager);                     // to_manager
    E4B_DECL_CASTOP(multi_name_manager);          // to_multi_name_manager
    E4B_DECL_CASTOP(multi_sequence_manager);      // to_multi_sequence_manager
    E4B_DECL_CASTOP(multi_species_manager);       // to_multi_species_manager
    E4B_DECL_CASTOP(name_manager);                // to_name_manager
    E4B_DECL_CASTOP(orf_terminal);                // to_orf_terminal
    E4B_DECL_CASTOP(pure_text_terminal);          // to_pure_text_terminal
    E4B_DECL_CASTOP(root_group_manager);          // to_root_group_manager
    E4B_DECL_CASTOP(sequence_info_terminal);      // to_sequence_info_terminal
    E4B_DECL_CASTOP(sequence_manager);            // to_sequence_manager
    E4B_DECL_CASTOP(sequence_terminal);           // to_sequence_terminal
    E4B_DECL_CASTOP(spacer_terminal);             // to_spacer_terminal
    E4B_DECL_CASTOP(species_manager);             // to_species_manager
    E4B_DECL_CASTOP(species_name_terminal);       // to_species_name_terminal
    E4B_DECL_CASTOP(terminal);                    // to_terminal
    E4B_DECL_CASTOP(text_terminal);               // to_text_terminal

    // simple access to containing managers
    inline ED4_species_manager *containing_species_manager() const;

    // discriminate between different sequence managers:

    inline bool is_consensus_manager() const;
    inline bool is_SAI_manager() const;
    inline bool is_species_seq_manager() const;

    inline ED4_species_type get_species_type() const; // works for all items (recursively) contained in ED4_species_manager

    inline bool inside_consensus_manager() const;
    inline bool inside_SAI_manager() const;
    inline bool inside_species_seq_manager() const;

    inline bool is_consensus_terminal() const;
    inline bool is_SAI_terminal() const;
    inline bool is_species_seq_terminal() const;

    inline bool is_consensus_sequence_terminal() const { // alias for CASTOP
        return is_consensus_terminal();
    }
};

DECLARE_CBTYPE_FVV_AND_BUILDERS(ED4_basePredicate, bool, ED4_base*); // generates makeED4_basePredicate

class ED4_manager : public ED4_base, public ED4_container { // derived from a Noncopyable
    ED4_cb_list<ED4_manager, ED4_managerCallback> delete_cbs;
public:

    E4B_AVOID_UNNEEDED_CASTS(manager);
    DECLARE_DUMP_FOR_BASECLASS(ED4_manager, ED4_base);

    void add_delete_callback(const ED4_managerCallback& cb) { delete_cbs.add_cb(cb); }
    void remove_delete_callback(const ED4_managerCallback& cb) { delete_cbs.remove_cb(cb); }

    int refresh_flag_ok();

    void changed_by_database() OVERRIDE;
    void deleted_from_database() OVERRIDE;

    // functions concerned with graphics
    ED4_returncode  Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    bool calc_bounding_box() OVERRIDE;

    ED4_returncode distribute_children();

    // top-down functions, means travelling down the hierarchy
    ED4_returncode event_sent_by_parent(AW_event *event, AW_window *aww) OVERRIDE;

    void request_refresh(int clear=1) OVERRIDE;
    ED4_returncode clear_refresh();

    void resize_requested_children() OVERRIDE;

    void update_requested_children() OVERRIDE;

    void delete_requested_children() OVERRIDE;
    void Delete() OVERRIDE;

    ED4_returncode  move_requested_by_parent(ED4_move_info *mi) OVERRIDE;

    void create_consensus(ED4_abstract_group_manager *upper_group_manager, arb_progress *progress);

    ARB_ERROR route_down_hierarchy(const ED4_route_cb& cb) OVERRIDE;

    ED4_base *find_first_that(ED4_level level, const ED4_basePredicate& fulfills_predicate);

     // bottom-up functions
    ED4_returncode  move_requested_by_child(ED4_move_info *moveinfo) OVERRIDE;
    inline void resize_requested_by_child();
    ED4_returncode  refresh_requested_by_child();
    void delete_requested_by_child();
    void update_requested_by_child();
    
    ED4_base *get_defined_level(ED4_level lev) const;

    // functions referring the consensus

    void update_consensus(ED4_manager *old_parent, ED4_manager *new_parent, ED4_base *sequence);
    ED4_returncode rebuild_consensi(ED4_base *start_species, ED4_update_flag update_flag);

    ED4_returncode  check_in_bases(ED4_base *added_base);
    ED4_returncode  check_out_bases(ED4_base *subbed_base);

    ED4_returncode  update_bases(const ED4_base *old_base, const ED4_base *new_base, PosRange range = PosRange::whole());
    ED4_returncode  update_bases(const char *old_seq, int old_len, const char *new_seq, int new_len, PosRange range = PosRange::whole());
    ED4_returncode  update_bases(const char *old_seq, int old_len, const ED4_base *new_base, PosRange range = PosRange::whole());
    ED4_returncode  update_bases(const BaseFrequencies *old_table, const BaseFrequencies *new_table, PosRange range = PosRange::whole());

    ED4_returncode  update_bases_and_rebuild_consensi(const char *old_seq, int old_len, ED4_base *species, ED4_update_flag update_flag, PosRange range = PosRange::whole());

    // handle moves across the hierarchy
    ED4_returncode  handle_move(ED4_move_info *moveinfo) OVERRIDE;

    ED4_base *get_competent_child(AW_pos x, AW_pos y, ED4_properties relevant_prop) OVERRIDE;
    ED4_base *get_competent_clicked_child(AW_pos x, AW_pos y, ED4_properties relevant_prop) OVERRIDE;
    ED4_base *search_spec_child_rek(ED4_level level) OVERRIDE;           // recursive search for level

    // general purpose functions
    ED4_base        *search_ID(const char *id) OVERRIDE;
    ED4_returncode  remove_callbacks() OVERRIDE;

    ED4_terminal *get_first_terminal(int start_index=0) const;
    ED4_terminal *get_last_terminal(int start_index=-1) const;

    void hide_children();
    void unhide_children();

    bool is_hidden() const OVERRIDE {
        if (flag.hidden) return true;
        if (!parent) return false;
        return parent->is_hidden();
    }

    ED4_manager(const ED4_objspec& spec_, const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_manager() OVERRIDE;
};

struct ED4_terminal : public ED4_base { // derived from a Noncopyable
    E4B_AVOID_UNNEEDED_CASTS(terminal);

    struct { unsigned int deleted : 1; } tflag; // @@@ go bool

    long curr_timestamp;

    DECLARE_DUMP_FOR_BASECLASS(ED4_terminal,ED4_base);

    // callbacks

    void changed_by_database() OVERRIDE;
    void deleted_from_database() OVERRIDE;

    // functions concerning graphic output
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE = 0;
    virtual ED4_returncode draw() = 0;

    bool calc_bounding_box() OVERRIDE;

    ED4_returncode draw_drag_box(AW_pos x, AW_pos y, GB_CSTR text = NULL, int cursor_y=-1);

    // functions which concern the object as a child
    void request_refresh(int clear=1) OVERRIDE;

    void resize_requested_children() OVERRIDE;

    void update_requested_children() OVERRIDE;
    void delete_requested_children() OVERRIDE;
    void Delete() OVERRIDE;

    ED4_returncode  move_requested_by_parent(ED4_move_info *mi) OVERRIDE;
    ED4_returncode  event_sent_by_parent(AW_event *event, AW_window *aww) OVERRIDE;
    ED4_base *get_competent_child(AW_pos x, AW_pos y, ED4_properties relevant_prop) OVERRIDE;
    ED4_base *get_competent_clicked_child(AW_pos x, AW_pos y, ED4_properties relevant_prop) OVERRIDE;
    ED4_returncode  move_requested_by_child(ED4_move_info *moveinfo) OVERRIDE;
    ED4_returncode  handle_move(ED4_move_info *moveinfo) OVERRIDE;

    ED4_returncode kill_object();

    // general purpose functions
    ED4_base *search_ID(const char *id) OVERRIDE;
    char          *resolve_pointer_to_string_copy(int *str_len = 0) const OVERRIDE;
    const char    *resolve_pointer_to_char_pntr(int *str_len = 0) const OVERRIDE;
    ED4_returncode remove_callbacks() OVERRIDE;

    GB_ERROR write_sequence(const char *seq, int seq_len);

    void scroll_into_view(ED4_window *ed4w);
    inline bool setCursorTo(ED4_cursor *cursor, int seq_pos, bool unfoldGroups, ED4_CursorJumpType jump_type);

    bool is_hidden() const OVERRIDE { return parent && parent->is_hidden(); }

    ED4_terminal(const ED4_objspec& spec_, GB_CSTR id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_terminal() OVERRIDE;
};

enum ED4_species_mode {
    ED4_SM_MOVE,
    ED4_SM_KILL,
    ED4_SM_MARK,
    ED4_SM_INFO // same functionality as AWT_MODE_INFO
};

class ED4_reference_terminals : virtual Noncopyable {
    ED4_sequence_info_terminal *ref_sequence_info;
    ED4_sequence_terminal      *ref_sequence;
    ED4_sequence_info_terminal *ref_column_stat_info;
    ED4_columnStat_terminal    *ref_column_stat;

    void null() { ref_sequence_info = 0; ref_sequence = 0; ref_column_stat = 0; ref_column_stat_info = 0; }
public:
    void clear();
    void init(ED4_sequence_info_terminal *, ED4_sequence_terminal *, ED4_sequence_info_terminal *, ED4_columnStat_terminal *);

    ED4_sequence_terminal      *sequence()         { return ref_sequence; }
    ED4_sequence_info_terminal *sequence_info()    { return ref_sequence_info; }
    ED4_columnStat_terminal    *column_stat()      { return ref_column_stat; }
    ED4_sequence_info_terminal *column_stat_info() { return ref_column_stat_info; }

    ED4_reference_terminals()  { null(); }
    ~ED4_reference_terminals() { clear(); }
};

class ED4_WinContext {
    ED4_window *ed4w;
    AW_device  *device;

    bool is_set() const { return ed4w; }
    void init(ED4_window *ew) {
        e4_assert(ew);
        ed4w   = ew;
        device = ed4w->get_device();
    }

    void warn_missing_context() const;
    void expect_context() const { if (!is_set()) warn_missing_context(); }

protected:
    ED4_WinContext() : ed4w(0), device(0) {}
    static ED4_WinContext current_context;
    
public:
    inline ED4_WinContext(AW_window *aww_);
    ED4_WinContext(ED4_window *ed4w_) { init(ed4w_); }

    AW_device *get_device() const { expect_context(); return device; }
    ED4_window *get_ed4w() const { expect_context(); return ed4w; }

    static const ED4_WinContext& get_current_context() { return current_context; }
    static bool have_context() { return current_context.is_set(); }
};

// accessors for current context (see also ED4_WinContextFree)
inline AW_device *current_device() { return ED4_WinContext::get_current_context().get_device(); }
inline ED4_window *current_ed4w() { return ED4_WinContext::get_current_context().get_ed4w(); }
inline AW_window *current_aww() { return current_ed4w()->aww; }
inline ED4_cursor& current_cursor() { return current_ed4w()->cursor; }

enum LoadableSaiState {
    LSAI_UNUSED,
    LSAI_UPTODATE,
    LSAI_OUTDATED,
};

class ED4_root : virtual Noncopyable {
    void ED4_ROOT() const { e4_assert(0); } // avoid ED4_root-members use global ED4_ROOT

    void refresh_window_simple(bool redraw);
    void handle_update_requests(bool& redraw);

    ED4_window           *most_recently_used_window;
    ConsensusBuildParams *cons_param;

public:
    char       *db_name;                            // name of Default Properties database (complete path)
    AW_root    *aw_root;                            // Points to 'AW-Window-Controller'
    AW_default  props_db;                           // Default Properties database

    ED4_window              *first_window;          // Points to List of Main Windows of ED4
    ED4_main_manager        *main_manager;          // Points to Main manager of ED4
    ED4_area_manager        *middle_area_man;       // Points to middle area
    ED4_area_manager        *top_area_man;
    ED4_root_group_manager  *root_group_man;
    EDB_root_bact           *database;              // Points to Object which controls Data
    ED4_selected_list       *selected_objects;
    ED4_scroll_links         scroll_links;
    bool                     folding_action;        // flag tells whether action was folding action or not
    ED4_reference_terminals  ref_terminals;
    ED4_species_mode         species_mode;
    ED4_scroll_picture       scroll_picture;
    BI_ecoli_ref            *ecoli_ref;
    char                    *alignment_name;
    GB_alignment_type        alignment_type;
    ED4_reference           *reference;
    ED4_seq_colors          *sequence_colors;
    AW_gc_manager           *gc_manager;
    ST_ML                   *st_ml;
    AW_helix                *helix;
    int                      helix_spacing;
    long                     helix_add_spacing;
    long                     terminal_add_spacing;
    char                    *protstruct;            // protein structure summary
    long                     protstruct_len;        // protein structure summary
    ed_key                  *edk;
    ED4_Edit_String         *edit_string;

    bool column_stat_activated;
    bool column_stat_initialized;
    bool visualizeSAI;
    bool visualizeSAI_allSpecies;

    const ConsensusBuildParams& get_consensus_params();
    void reset_consensus_params();

    LoadableSaiState loadable_SAIs; // maintain proper refresh of list of loadable SAIs
    void loadable_SAIs_may_have_changed() { if (loadable_SAIs == LSAI_UPTODATE) loadable_SAIs = LSAI_OUTDATED; }

    int temp_gc;
    AW_font_group font_group;

    void announce_useraction_in(AW_window *aww);
    ED4_window *get_most_recently_used_window() const {
        e4_assert(most_recently_used_window);
        return most_recently_used_window;
    }

    inline ED4_device_manager *get_device_manager();

    // Initializing functions
    ED4_returncode  create_hierarchy(const char *area_string_middle, const char *area_string_top);
    ARB_ERROR init_alignment();
    void recalc_font_group();

    AW_window *create_new_window();
    ED4_returncode generate_window(AW_device **device, ED4_window **new_window);
    void copy_window_struct(ED4_window *source,   ED4_window *destination);

    // functions concerned with global refresh and resize
    void resize_all_requesting_childs();

    void special_window_refresh(bool handle_updates);
    ED4_returncode refresh_all_windows(bool redraw);

    void request_refresh_for_all_terminals();
    void request_refresh_for_specific_terminals(ED4_level lev);
    void request_refresh_for_consensus_terminals();
    void request_refresh_for_sequence_terminals();

    inline void announce_deletion(ED4_base *object); // before deleting an object, use this to announce

     // functions concerned with list of selected objects
    ED4_returncode add_to_selected(ED4_species_name_terminal *object);
    void remove_from_selected(ED4_species_name_terminal *object);
    ED4_returncode deselect_all();

    ED4_returncode get_area_rectangle(AW_screen_area *rect, AW_pos x, AW_pos y);

    ED4_index pixel2pos(AW_pos click_x);

    void remove_all_callbacks();
    
    ED4_root(int *argc, char*** argv);
    ~ED4_root();
};

ED4_WinContext::ED4_WinContext(AW_window *aww_) { init(ED4_ROOT->first_window->get_matching_ed4w(aww_)); }

struct ED4_LocalWinContext : private ED4_WinContext {
    ED4_LocalWinContext(AW_window *aww) : ED4_WinContext(current_context) { current_context = ED4_WinContext(aww); }
    ED4_LocalWinContext(ED4_window *ew) : ED4_WinContext(current_context) { current_context = ED4_WinContext(ew); }
    ~ED4_LocalWinContext() { current_context = *this; }
};

class ED4_MostRecentWinContext : virtual Noncopyable { 
    ED4_LocalWinContext *most_recent;
public:
    ED4_MostRecentWinContext() : most_recent(0) {
        if (!ED4_WinContext::have_context()) {
            most_recent = new ED4_LocalWinContext(ED4_ROOT->get_most_recently_used_window());
        }
    }
    ~ED4_MostRecentWinContext() {
        delete most_recent;
    }
};

inline void ED4_root::announce_deletion(ED4_base *object) {
    if (object->is_terminal()) {
        ED4_terminal *term = object->to_terminal();
        for (ED4_window *win = first_window; win; win = win->next) {
            ED4_LocalWinContext uses(win);
            win->announce_deletion(term);
        }
    }
}

// ------------------------
//      manager classes
//
// All manager classes only differ in their static properties.
// This kind of construction was chosen for using a minimum of RAM

class ED4_main_manager : public ED4_manager { // derived from a Noncopyable
    // first in hierarchy

    E4B_AVOID_UNNEEDED_CASTS(main_manager);

    // these terminals are redrawn after refresh (with increase clipping area)
    // to revert text from middle area drawn into top area:
    ED4_terminal *top_middle_line;
    ED4_terminal *top_middle_spacer;

public:
    ED4_main_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    void set_top_middle_spacer_terminal(ED4_terminal *top_middle_spacer_) { top_middle_spacer = top_middle_spacer_; }
    void set_top_middle_line_terminal(ED4_terminal *top_middle_line_) { top_middle_line = top_middle_line_; }

    ED4_terminal *get_top_middle_spacer_terminal() const { return top_middle_spacer; }
    ED4_terminal *get_top_middle_line_terminal() const { return top_middle_line; }

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);

    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    void resize_requested_children() OVERRIDE;
    
    void clear_whole_background();
};

struct ED4_device_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(device_manager);
    ED4_device_manager  (const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);
};

struct ED4_area_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(area_manager);
    ED4_area_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);

    ED4_multi_species_manager *get_multi_species_manager() const {
        return get_defined_level(LEV_MULTI_SPECIES)->to_multi_species_manager();
    }
};

class ED4_multi_species_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(multi_species_manager);

    int species;          // # of species (-1 == unknown)
    int selected_species; // # of selected species (-1 == unknown)

    void    set_species_counters(int no_of_species, int no_of_selected);
#ifdef DEBUG
    void    count_species(int *speciesPtr, int *selectedPtr) const;
#endif
    void    update_species_counters();

public:
    ED4_multi_species_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);

    void update_requested_children() OVERRIDE;
    void delete_requested_children() OVERRIDE;

    int count_visible_children();           // is called by a multi_species_manager

    ED4_species_manager       *get_consensus_manager() const;       // returns the consensus-manager or NULL
    ED4_species_name_terminal *get_consensus_name_terminal() const; // returns the consensus-name-terminal or NULL

    // functions concerned with selection
    int get_no_of_selected_species();
    int get_no_of_species();

    void update_group_id();

    void invalidate_species_counters();
    int  has_valid_counters() const { return species != -1 && selected_species!=-1; }
    bool all_are_selected() const { e4_assert(has_valid_counters()); return species == selected_species; }

    void select_all(bool only_species);
    void deselect_all_species_and_SAI();
    void invert_selection_of_all_species();
    void marked_species_select(bool select);
    void selected_species_mark(bool mark);

    void toggle_selected_species();
};

class ED4_abstract_group_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(abstract_group_manager);
protected:
    BaseFrequencies my_table; // table concerning Consensusfunction

public:
    ED4_abstract_group_manager(const ED4_objspec& spec_, const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    DECLARE_DUMP_FOR_BASECLASS(ED4_abstract_group_manager, ED4_manager);

    BaseFrequencies&       table()       { return my_table; }
    const BaseFrequencies& table() const { return my_table; }

    ED4_multi_species_manager *get_multi_species_manager() const {
        return get_defined_level(LEV_MULTI_SPECIES)->to_multi_species_manager();
    }

    char *build_consensus_string(PosRange range) const { return table().build_consensus_string(range, ED4_ROOT->get_consensus_params()); }
    char *build_consensus_string(int *cons_length = NULL) const {
        if (cons_length) *cons_length = table().size();
        return table().build_consensus_string(ED4_ROOT->get_consensus_params());
    }
};

struct ED4_group_manager : public ED4_abstract_group_manager {
    E4B_AVOID_UNNEEDED_CASTS(group_manager);
    ED4_group_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_abstract_group_manager);
    void reinit_char_table();

    void fold();
    void unfold();
    void toggle_folding();
};

enum ED4_remap_mode {
    ED4_RM_NONE,                                    // no remapping (normal mode)
    ED4_RM_SHOW_ABOVE,                              // Show all positions, where xxx% of all edited sequences have any base
    ED4_RM_MAX_ALIGN,                               // ------------------------- any edited sequence has a base
    ED4_RM_MAX_EDIT,                                // like ED4_RM_MAX_ALIGN, but bases are pushed OVER remapped gaps (not INTO)
    ED4_RM_DYNAMIC_GAPS,                            // gaps shown as spaces (more gaps => more spaces)

    ED4_RM_MODES

};

class ED4_remap : virtual Noncopyable {

    ED4_remap_mode mode;
    int show_above_percent;     // used only for ED4_RM_SHOW_ABOVE

    size_t sequence_table_len;      // allocated size of sequence_to_screen_tab
    size_t screen_table_len;        // allocated size of screen_to_sequence_tab

    size_t sequence_len;        // size of recently compiled part of sequence_to_screen_tab
    size_t screen_len;          // size of recently compiled part of screen_to_sequence_tab

    int *screen_to_sequence_tab;
    int *sequence_to_screen_tab;    // <0 means position is not mapped (last displayed sequence position); 0 may be mapped or not; see is_shown() below

    int changed;            // remap-table changed at last compile
    int update_needed;          // remapping should be recompiled

    inline void set_sequence_to_screen(int pos, int newVal);

public:

    ED4_remap();
    ~ED4_remap();

    int screen_to_sequence(int screen_pos) const;

    int sequence_to_screen_PLAIN(int sequence_pos) const { 
        e4_assert(sequence_pos>=0 && size_t(sequence_pos)<=sequence_len);
        return sequence_to_screen_tab[sequence_pos];
    }
    int shown_sequence_to_screen(int sequence_pos) const {
        // as well works for sequence_pos == MAXSEQUENCECHARACTERLENGTH
        int screen_pos = sequence_to_screen_PLAIN(sequence_pos);
        e4_assert(screen_pos >= 0); // sequence_pos expected to be visible (i.e. not folded away)
        return screen_pos;
    }

    int clipped_sequence_to_screen_PLAIN(int sequence_pos) const; 
    int sequence_to_screen(int sequence_pos) const;

    PosRange sequence_to_screen(PosRange range) const {
        e4_assert(!range.is_empty());
        return PosRange(sequence_to_screen(range.start()), sequence_to_screen(range.end()));
    }
    PosRange screen_to_sequence(PosRange range) const {
        e4_assert(!range.is_empty());
        if (range.is_unlimited()) return PosRange::from(screen_to_sequence(range.start()));
        return PosRange(screen_to_sequence(range.start()), screen_to_sequence(range.end()));
    }

    size_t get_max_screen_pos() const { return screen_len-1; }

    void adjacent_screen_positions(int seq_pos, int& screen_pos_left, int& screen_pos_right);

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
            show_above_percent = above_percent;
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

    int is_shown(int seq_pos) const {
        int scr_pos = sequence_to_screen_PLAIN(seq_pos);
        if (scr_pos<0) return false;
        if (scr_pos>0) return true;

        int seq_pos2    = screen_to_sequence_tab[0];
        return seq_pos == seq_pos2;
    }

    ExplicitRange clip_screen_range(PosRange screen_range) const { return ExplicitRange(screen_range, screen_len-1); }
};

class ED4_root_group_manager : public ED4_abstract_group_manager {
    E4B_AVOID_UNNEEDED_CASTS(root_group_manager);
    ED4_remap my_remap;
public:
    ED4_root_group_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    bool update_remap(); // 'true' if mapping has changed

    const ED4_remap *remap() const { return &my_remap; }
    ED4_remap *remap() { return &my_remap; }

    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    void resize_requested_children() OVERRIDE;

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_abstract_group_manager);
};

class ED4_species_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(species_manager);

    ED4_cb_list<ED4_species_manager, ED4_species_managerCallback> changed_cbs; // called when sequence changes

    ED4_species_type type;
    bool selected;

public:
    ED4_species_manager(ED4_species_type type_, const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_species_manager() OVERRIDE;

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);

    ED4_species_type get_type() const { return type; }

    bool is_selected() const { return selected; }
    void set_selected(bool select) {
        // e4_assert(type != ED4_SP_CONSENSUS); // it's not allowed to select a consensus // @@@ happens atm when moving a group
        selected = select;
    }

    bool setCursorTo(ED4_cursor *cursor, int seq_pos, bool unfold_groups, ED4_CursorJumpType jump_type);

    void add_sequence_changed_cb(const ED4_species_managerCallback& cb) { changed_cbs.add_cb(cb); }
    void remove_sequence_changed_cb(const ED4_species_managerCallback& cb) { changed_cbs.remove_cb(cb); }
    void do_callbacks() { changed_cbs.call(this); }

    ED4_species_name_terminal *get_name_terminal() const { return member(0)->to_species_name_terminal(); }
};

inline ED4_species_manager *ED4_base::containing_species_manager() const {
    ED4_base *sman = get_parent(LEV_SPECIES);
    return sman ? sman->to_species_manager() : NULL;
}

inline bool ED4_base::is_consensus_manager()    const { return is_species_manager() && to_species_manager()->get_type() == ED4_SP_CONSENSUS; }
inline bool ED4_base::is_SAI_manager()          const { return is_species_manager() && to_species_manager()->get_type() == ED4_SP_SAI; }
inline bool ED4_base::is_species_seq_manager()  const { return is_species_manager() && to_species_manager()->get_type() == ED4_SP_SPECIES; }

inline ED4_species_type ED4_base::get_species_type() const {
    ED4_species_manager *sman = containing_species_manager();
    return sman ? sman->get_type() : ED4_SP_NONE;
}

inline bool ED4_base::inside_consensus_manager()   const { return get_species_type() == ED4_SP_CONSENSUS; }
inline bool ED4_base::inside_SAI_manager()         const { return get_species_type() == ED4_SP_SAI; }
inline bool ED4_base::inside_species_seq_manager() const { return get_species_type() == ED4_SP_SPECIES; }

inline bool ED4_base::is_consensus_terminal()   const { return is_sequence_terminal() && inside_consensus_manager(); }
inline bool ED4_base::is_SAI_terminal()         const { return is_sequence_terminal() && inside_SAI_manager(); }
inline bool ED4_base::is_species_seq_terminal() const { return is_sequence_terminal() && inside_species_seq_manager(); }

inline bool ED4_cursor::in_species_seq_terminal() const { return owner_of_cursor && owner_of_cursor->is_species_seq_terminal(); }
inline bool ED4_cursor::in_consensus_terminal()   const { return owner_of_cursor && owner_of_cursor->is_consensus_terminal(); }
inline bool ED4_cursor::in_SAI_terminal()         const { return owner_of_cursor && owner_of_cursor->is_SAI_terminal(); }


struct ED4_multi_sequence_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(multi_sequence_manager);
    ED4_multi_sequence_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);
};

struct ED4_sequence_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(sequence_manager);
    ED4_sequence_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);
};

struct ED4_multi_name_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(multi_name_manager);
    // member of ED4_species_manager (contains ED4_name_manager for name and info)
    ED4_multi_name_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);
};


struct ED4_name_manager : public ED4_manager {
    E4B_AVOID_UNNEEDED_CASTS(name_manager);
    // member of ED4_multi_name_manager (contains speciesname or other info concerning the species)
    ED4_name_manager(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    DECLARE_DUMP_FOR_LEAFCLASS(ED4_manager);
};


// -------------------------
//      terminal classes


struct ED4_tree_terminal : public ED4_terminal {
    E4B_AVOID_UNNEEDED_CASTS(tree_terminal);
    
    ED4_returncode draw() OVERRIDE;
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;

    ED4_tree_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_terminal);
};

struct ED4_bracket_terminal : public ED4_terminal {
    E4B_AVOID_UNNEEDED_CASTS(bracket_terminal);

    ED4_returncode draw() OVERRIDE;
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;

    void toggle_folding();

    ED4_bracket_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_terminal);
};

struct ED4_text_terminal : public ED4_terminal {
    E4B_AVOID_UNNEEDED_CASTS(text_terminal);
    
    // functions concerning graphic output
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    ED4_returncode draw() OVERRIDE;

    virtual int get_length() const = 0;
    void deleted_from_database() OVERRIDE;

    ED4_text_terminal(const ED4_objspec& spec_, GB_CSTR id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_text_terminal() OVERRIDE {}

    DECLARE_DUMP_FOR_BASECLASS(ED4_text_terminal, ED4_terminal);
};

class ED4_abstract_sequence_terminal : public ED4_text_terminal { // derived from a Noncopyable

    PosRange pixel2index(PosRange pixel_range);

    E4B_AVOID_UNNEEDED_CASTS(abstract_sequence_terminal);
public:
    char *species_name; // @@@ wrong place (may be member of ED4_sequence_manager)


    ED4_abstract_sequence_terminal(const ED4_objspec& spec_, const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_abstract_sequence_terminal() OVERRIDE;

    virtual GB_alignment_type GetAliType() = 0;
    int get_length() const OVERRIDE { int len; resolve_pointer_to_char_pntr(&len); return len; }

    ED4_species_name_terminal *corresponding_species_name_terminal() const {
        return get_parent(LEV_SPECIES)->search_spec_child_rek(LEV_SPECIES_NAME)->to_species_name_terminal();
    }
    PosRange calc_interval_displayed_in_rectangle(AW_screen_area *area_rect);
    PosRange calc_update_interval();

    DECLARE_DUMP_FOR_BASECLASS(ED4_abstract_sequence_terminal, ED4_text_terminal);
};

class ED4_orf_terminal : public ED4_abstract_sequence_terminal { // derived from a Noncopyable
    // NOTE: ED4_orf_terminal is a separate terminal class used to display Open Reading Frames (ORFs)
    //       for the corresponding gene (DNA) sequence. It is used in ProteinViewer Module and should not be
    //       used for drawing aminoacid sequence alone as in protein alignment. Aminoacid sequences are
    //       handled by the standard "ED4_sequence_terminal" class.

    char *aaSequence;
    size_t aaSeqLen;
    char *aaColor;
    int   aaStartPos;
    int   aaStrandType;

    ED4_returncode draw() OVERRIDE;
    E4B_AVOID_UNNEEDED_CASTS(orf_terminal);
public:
    ED4_orf_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_orf_terminal() OVERRIDE;

    GB_alignment_type GetAliType() OVERRIDE;

    void SET_aaSeqFlags (int startPos, int strandType) { aaStartPos = startPos; aaStrandType = strandType; }
    void SET_aaSequence(const char *aaSeq) { freedup(aaSequence, aaSeq); aaSeqLen = strlen(aaSequence); }
    void SET_aaColor(const char *aaSeq) { freedup(aaColor, aaSeq); }

    int GET_aaStartPos () { return aaStartPos; }
    int GET_aaStrandType () { return aaStrandType; }

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_abstract_sequence_terminal);
};

class ED4_sequence_terminal : public ED4_abstract_sequence_terminal { // derived from a Noncopyable
    mutable ED4_SearchResults searchResults;
    bool shall_display_secstruct_info; // helix or protstruct

    ED4_returncode draw() OVERRIDE;

    E4B_AVOID_UNNEEDED_CASTS(sequence_terminal);
    
public:

    AP_tree *st_ml_node;

    ED4_sequence_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent, bool shall_display_secstruct_info_);

    GB_alignment_type GetAliType() OVERRIDE;

    void deleted_from_database() OVERRIDE;
    int get_length() const OVERRIDE { return ED4_abstract_sequence_terminal::get_length(); }

    virtual char *get_sequence_copy(int *str_len = 0) const { return resolve_pointer_to_string_copy(str_len); }

    void set_secstruct_display(bool show) { shall_display_secstruct_info = show; }

    ED4_SearchResults& results() const { return searchResults; }

    ED4_columnStat_terminal *corresponding_columnStat_terminal() const {
        ED4_base *col_term = get_parent(LEV_MULTI_SEQUENCE)->search_spec_child_rek(LEV_COL_STAT);
        return col_term ? col_term->to_columnStat_terminal() : 0;
    }

    DECLARE_DUMP_FOR_MIDCLASS(ED4_sequence_terminal,ED4_abstract_sequence_terminal);
};

class ED4_columnStat_terminal : public ED4_text_terminal { // derived from a Noncopyable
    char *likelihood[4];        // likelihood-array for each base (ACGU) [length of array = alignment_length]
    int   latest_update;

    static double threshold;

    int update_likelihood();

    E4B_AVOID_UNNEEDED_CASTS(columnStat_terminal);

public:
    // functions concerning graphic output
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    ED4_returncode draw() OVERRIDE;
    int get_length() const OVERRIDE { return corresponding_sequence_terminal()->get_length(); }

    static int threshold_is_set();
    static void set_threshold(double aThreshold);
    static double get_threshold() { return threshold; }

    ED4_sequence_terminal *corresponding_sequence_terminal() const { return get_parent(LEV_MULTI_SEQUENCE)->search_spec_child_rek(LEV_SEQUENCE_STRING)->to_sequence_terminal(); }

    GB_CSTR build_probe_match_string(PosRange range) const;

    ED4_columnStat_terminal(GB_CSTR id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_columnStat_terminal() OVERRIDE;

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_text_terminal);
};

class ED4_species_name_terminal : public ED4_text_terminal { // derived from a Noncopyable
    E4B_AVOID_UNNEEDED_CASTS(species_name_terminal);
    bool set_dynamic_size() OVERRIDE;

public:
    ED4_species_name_terminal(GB_CSTR id, AW_pos width, AW_pos height, ED4_manager *parent);
    ~ED4_species_name_terminal() OVERRIDE { delete selection_info; }


    ED4_selection_entry *selection_info;            // Info about i.e. Position
    bool dragged;

    GB_CSTR get_displayed_text() const;
    int get_length() const OVERRIDE { return strlen(get_displayed_text()); }

    ED4_sequence_terminal *corresponding_sequence_terminal() const {
        ED4_base *seq_term = get_parent(LEV_SPECIES)->search_spec_child_rek(LEV_SEQUENCE_STRING);
        return seq_term ? seq_term->to_sequence_terminal() : 0;
    }

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_text_terminal);
};

class ED4_sequence_info_terminal : public ED4_text_terminal {
    E4B_AVOID_UNNEEDED_CASTS(sequence_info_terminal);
    bool set_dynamic_size() OVERRIDE;

public:
    ED4_sequence_info_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    ED4_species_name_terminal *corresponding_species_name_terminal() const {
        return get_parent(LEV_SPECIES)->search_spec_child_rek(LEV_SPECIES_NAME)->to_species_name_terminal();
    }

    ED4_returncode draw() OVERRIDE;

    GBDATA *data() { return get_species_pointer(); } // DB-entry ("ali_xxx/data")
    const GBDATA *data() const { return get_species_pointer(); }

    int get_length() const OVERRIDE { return 1+strlen(id); }

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_text_terminal);
};

struct ED4_pure_text_terminal : public ED4_text_terminal {
    E4B_AVOID_UNNEEDED_CASTS(pure_text_terminal);
    
    ED4_pure_text_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    int get_length() const OVERRIDE { int len; resolve_pointer_to_char_pntr(&len); return len; }

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_text_terminal);
};

class ED4_consensus_sequence_terminal : public ED4_sequence_terminal { // derived from a Noncopyable
    E4B_AVOID_UNNEEDED_CASTS(consensus_sequence_terminal);

    ED4_returncode draw() OVERRIDE;
    const ED4_abstract_group_manager *get_group_manager() const  { return get_parent(LEV_GROUP)->to_group_manager(); }
    const BaseFrequencies& get_char_table() const { return get_group_manager()->table(); }

public:
    char *temp_cons_seq; // used for editing consensus (normally NULL)

    ED4_consensus_sequence_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);
#if defined(ASSERTION_USED)
    virtual ~ED4_consensus_sequence_terminal() { e4_assert(!temp_cons_seq); }
#endif

    int get_length() const OVERRIDE;
    char *get_sequence_copy(int *str_len = 0) const OVERRIDE;

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_sequence_terminal);
};

class ED4_spacer_terminal : public ED4_terminal {
    E4B_AVOID_UNNEEDED_CASTS(spacer_terminal);
    bool shallDraw; // true -> spacer is really drawn (otherwise it's only a placeholder)

    bool set_dynamic_size() OVERRIDE;

public:
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    ED4_returncode draw() OVERRIDE;

    ED4_spacer_terminal(const char *id, bool shallDraw_, AW_pos width, AW_pos height, ED4_manager *parent);

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_terminal);
};

class ED4_line_terminal : public ED4_terminal {
    E4B_AVOID_UNNEEDED_CASTS(line_terminal);
    bool set_dynamic_size() OVERRIDE;

public:
    ED4_returncode Show(int refresh_all=0, int is_cleared=0) OVERRIDE;
    ED4_returncode draw() OVERRIDE;

    ED4_line_terminal(const char *id, AW_pos width, AW_pos height, ED4_manager *parent);

    DECLARE_DUMP_FOR_LEAFCLASS(ED4_terminal);
};


// ----------------------------------------------
//      inlines which need complete classdefs

inline void ED4_base::set_update() { // @@@ rename into request_update
    if (!update_info.update_requested) {
        update_info.update_requested = 1;
        if (parent) parent->update_requested_by_child();
    }
}

inline void ED4_base::request_resize() {
    update_info.set_resize(1);
    if (parent) parent->resize_requested_by_child();
}

void ED4_manager::resize_requested_by_child() { 
    if (!update_info.resize) request_resize();
}

inline bool ED4_terminal::setCursorTo(ED4_cursor *cursor, int seq_pos, bool unfoldGroups, ED4_CursorJumpType jump_type) {
    ED4_species_manager *sm = get_parent(LEV_SPECIES)->to_species_manager();
    return sm->setCursorTo(cursor, seq_pos, unfoldGroups, jump_type);
}

E4B_IMPL_CASTOP(area_manager);                // to_area_manager
E4B_IMPL_CASTOP(abstract_group_manager);      // to_abstract_group_manager
E4B_IMPL_CASTOP(bracket_terminal);            // to_bracket_terminal
E4B_IMPL_CASTOP(columnStat_terminal);         // to_columnStat_terminal
E4B_IMPL_CASTOP(consensus_sequence_terminal); // to_consensus_sequence_terminal
E4B_IMPL_CASTOP(device_manager);              // to_device_manager
E4B_IMPL_CASTOP(group_manager);               // to_group_manager
E4B_IMPL_CASTOP(line_terminal);               // to_line_terminal
E4B_IMPL_CASTOP(manager);                     // to_manager
E4B_IMPL_CASTOP(multi_name_manager);          // to_multi_name_manager
E4B_IMPL_CASTOP(multi_sequence_manager);      // to_multi_sequence_manager
E4B_IMPL_CASTOP(multi_species_manager);       // to_multi_species_manager
E4B_IMPL_CASTOP(name_manager);                // to_name_manager
E4B_IMPL_CASTOP(orf_terminal);                // to_orf_terminal
E4B_IMPL_CASTOP(pure_text_terminal);          // to_pure_text_terminal
E4B_IMPL_CASTOP(root_group_manager);          // to_root_group_manager
E4B_IMPL_CASTOP(sequence_info_terminal);      // to_sequence_info_terminal
E4B_IMPL_CASTOP(sequence_manager);            // to_sequence_manager
E4B_IMPL_CASTOP(sequence_terminal);           // to_sequence_terminal
E4B_IMPL_CASTOP(spacer_terminal);             // to_spacer_terminal
E4B_IMPL_CASTOP(species_manager);             // to_species_manager
E4B_IMPL_CASTOP(species_name_terminal);       // to_species_name_terminal
E4B_IMPL_CASTOP(terminal);                    // to_terminal
E4B_IMPL_CASTOP(text_terminal);               // to_text_terminal

inline ED4_device_manager *ED4_root::get_device_manager() {
    return main_manager->search_spec_child_rek(LEV_DEVICE)->to_device_manager();
}


inline ED4_species_name_terminal *ED4_multi_species_manager::get_consensus_name_terminal() const { 
    ED4_species_manager *consensus_man = get_consensus_manager();
    return consensus_man ? consensus_man->get_name_terminal() : NULL;
}

// ----------------------------------------

enum SpeciesCreationMode {
    CREATE_NEW_SPECIES,
    CREATE_FROM_CONSENSUS, // create new species from group consensus (of surrounding group)
    COPY_SPECIES,          // copy current species
};

#if defined(ASSERTION_USED)
CONSTEXPR_RETURN inline bool valid(SpeciesCreationMode m) { return m>=CREATE_NEW_SPECIES && m<=COPY_SPECIES; }
#endif

extern ST_ML *st_ml;
struct AlignDataAccess;

// -------------------------------------------------
//      factory functions (hierarchy builders):

ED4_group_manager *ED4_build_group_manager_start(ED4_manager                 *group_parent,
                                                 GB_CSTR                      group_name,
                                                 int                          group_depth,
                                                 bool                         is_folded,
                                                 ED4_reference_terminals&     refterms,
                                                 ED4_multi_species_manager*&  multi_species_manager);

void ED4_build_group_manager_end(ED4_multi_species_manager *multi_species_manager);

// -------------------------------------
//      hierarchy access functions:

ED4_species_name_terminal *ED4_find_species_name_terminal(const char *species_name);
ED4_species_name_terminal *ED4_find_SAI_name_terminal(const char *sai_name);
ED4_species_name_terminal *ED4_find_species_or_SAI_name_terminal(const char *species_name);

ED4_multi_species_manager *ED4_find_MoreSequences_manager();

// --------------------------
//      other functions:

void ED4_new_editor_window(AW_window *aww);
void ED4_with_all_edit_windows(void (*cb)(ED4_window *));

void ED4_resize_all_extensions();
void ED4_calc_terminal_extentions();

void ED4_input_cb(AW_window *aww);
void ED4_remote_event(AW_event *faked_event);
void ED4_motion_cb(AW_window *aww);
void ED4_vertical_change_cb(AW_window *aww);
void ED4_horizontal_change_cb(AW_window *aww);
void ED4_scrollbar_change_cb(AW_window *aww);
void ED4_trigger_instant_refresh();
void ED4_request_relayout();
void ED4_request_full_refresh();
void ED4_request_full_instant_refresh();

void ED4_store_curpos(AW_window *aww);
void ED4_restore_curpos(AW_window *aww);
void ED4_clear_stored_curpos();
void ED4_helix_jump_opposite(AW_window *aww);
void ED4_jump_to_cursor_position(AW_window *aww, const char *awar_name, PositionType posType);
void ED4_remote_set_cursor_cb(AW_root *awr);
void ED4_change_cursor(AW_window *aww);

void ED4_set_iupac(AW_window *aww, const char *awar_name);
void ED4_set_helixnr(AW_window *aww, const char *awar_name);
void ed4_changesecurity(AW_root *root);
void ed4_change_edit_mode(AW_root *root);

void ED4_jump_to_current_species(AW_window *);
void ED4_get_and_jump_to_current(AW_window *);
void ED4_get_and_jump_to_species(GB_CSTR species_name);

void ED4_get_marked_from_menu(AW_window *);
void ED4_selected_species_changed_cb(AW_root *aw_root);

AW_window *ED4_create_loadSAI_window(AW_root *awr);
void       ED4_get_and_jump_to_selected_SAI(AW_window *aww);
void       ED4_selected_SAI_changed_cb(AW_root *aw_root);

void ED4_init_notFoundMessage();
void ED4_finish_and_show_notFoundMessage();

const AlignDataAccess *ED4_get_aligner_data_access();

void ED4_popup_gc_window(AW_window *awp, AW_gc_manager *gcman);
void ED4_no_dangerous_modes();

void       group_species_cb(AW_window *aww, bool use_fields);
AW_window *ED4_create_group_species_by_field_window(AW_root *aw_root);

AW_window *ED4_create_loadConfiguration_window(AW_root *awr);
void       ED4_reloadConfiguration(AW_window *aww);
void       ED4_saveConfiguration(AW_window *aww, bool hide_aww);
AW_window *ED4_create_saveConfigurationAs_window(AW_root *awr);

ARB_ERROR  rebuild_consensus(ED4_base *object);

void       ED4_create_consensus_awars(AW_root *aw_root);
AW_window *ED4_create_consensus_definition_window(AW_root *root);
void       ED4_consensus_definition_changed(AW_root*);
void       ED4_consensus_display_changed(AW_root *root);

AW_window *ED4_create_editor_options_window(AW_root *root);
void       ED4_compression_toggle_changed_cb(AW_root *root, bool hideChanged);
void       ED4_compression_changed_cb(AW_root *awr);
void       ED4_alignment_length_changed(GBDATA *gb_alignment_len, GB_CB_TYPE gbtype);

AW_window *ED4_create_new_seq_window(AW_root *root, SpeciesCreationMode creation_mode);

void ED4_quit_editor(AW_window *aww);
void ED4_exit() __ATTR__NORETURN;


#else
#error ed4_class included twice
#endif
