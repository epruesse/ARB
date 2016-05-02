// =============================================================== //
//                                                                 //
//   File      : TreeDisplay.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef TREEDISPLAY_HXX
#define TREEDISPLAY_HXX

#ifndef AP_TREE_HXX
#include <AP_Tree.hxx>
#endif
#ifndef AWT_CANVAS_HXX
#include <awt_canvas.hxx>
#endif

#define td_assert(cond) arb_assert(cond)

#define AWAR_DTREE_BASELINEWIDTH   "awt/dtree/baselinewidth"
#define AWAR_DTREE_VERICAL_DIST    "awt/dtree/verticaldist"
#define AWAR_DTREE_AUTO_JUMP       "awt/dtree/autojump"
#define AWAR_DTREE_SHOW_CIRCLE     "awt/dtree/show_circle"
#define AWAR_DTREE_SHOW_BRACKETS   "awt/dtree/show_brackets"
#define AWAR_DTREE_CIRCLE_ZOOM     "awt/dtree/circle_zoom"
#define AWAR_DTREE_CIRCLE_MAX_SIZE "awt/dtree/max_size"
#define AWAR_DTREE_USE_ELLIPSE     "awt/dtree/ellipse"
#define AWAR_DTREE_GREY_LEVEL      "awt/dtree/greylevel"

#define AWAR_DTREE_RADIAL_ZOOM_TEXT "awt/dtree/radial/zoomtext"
#define AWAR_DTREE_RADIAL_XPAD      "awt/dtree/radial/xpadding"

#define AWAR_DTREE_DENDRO_ZOOM_TEXT "awt/dtree/dendro/zoomtext"
#define AWAR_DTREE_DENDRO_XPAD      "awt/dtree/dendro/xpadding"

void awt_create_dtree_awars(AW_root *aw_root, AW_default db);

#define NT_BOX_WIDTH      7 // pixel
#define NT_ROOT_WIDTH     9
#define NT_SELECTED_WIDTH 11

#define AWT_TREE(ntw) DOWNCAST(AWT_graphic_tree*, (ntw)->gfx)


enum AP_tree_display_type {
    AP_TREE_NORMAL, // normal tree display (dendrogram)
    AP_TREE_RADIAL, // radial tree display
    AP_TREE_IRS, // like AP_TREE_NORMAL, with folding line
    AP_LIST_NDS,
    AP_LIST_SIMPLE // simple display only showing name (used at startup to avoid NDS error messages)
};

enum AP_tree_jump_type { // bit-values
    AP_JUMP_UNFOLD_GROUPS     = 1,
    AP_JUMP_CENTER_IF_VISIBLE = 2, // if already visible -> center (normally only done if IRS-mode or selected was invisible)
    AP_JUMP_BE_VERBOOSE       = 4, // tell why nothing happened etc.

    // convenience defs:
    AP_JUMP_AUTO      = 0,
    AP_JUMP_BY_BUTTON = AP_JUMP_UNFOLD_GROUPS|AP_JUMP_CENTER_IF_VISIBLE|AP_JUMP_BE_VERBOOSE,
};

inline bool sort_is_list_style(AP_tree_display_type sort) { return sort == AP_LIST_NDS || sort == AP_LIST_SIMPLE; }
inline bool sort_is_tree_style(AP_tree_display_type sort) { return !sort_is_list_style(sort); }


class AWT_graphic_tree_group_state;

struct AWT_scaled_font_limits {
    double ascent;
    double descent;
    double height;
    double width;

    void init(const AW_font_limits& font_limits, double factor) {
        ascent  = font_limits.ascent*factor;
        descent = font_limits.descent*factor;
        height  = font_limits.height*factor;
        width   = font_limits.width*factor;
    }
};

enum AD_MAP_VIEWER_TYPE {
    ADMVT_NONE = 0,
    ADMVT_INFO,
    ADMVT_WWW,
    ADMVT_SELECT
};

typedef void (*AD_map_viewer_cb)(GBDATA *gbd, AD_MAP_VIEWER_TYPE type);

struct DendroSubtreeLimits {
    double y_branch;                                // ypos of branch to subtree
    double y_top;                                   // top ypos of whole subtree
    double y_bot;                                   // bottom ypos of whole subtree
    double x_right;                                 // rightmost xpos of whole subtree

    void combine(const DendroSubtreeLimits& other) {
        y_top   = std::min(y_top, other.y_top);
        y_bot   = std::max(y_bot, other.y_bot);
        x_right = std::max(x_right, other.x_right);
    }
};

struct AWT_command_data {
    /*! any kind of data which has to be stored between different events (e.g. to support drag&drop)
     * Purpose of this class is to allow to delete such data w/o knowing anything else.
     */
    virtual ~AWT_command_data() {}
};

enum CollapseMode {
    COLLAPSE_ALL      = 0,
    EXPAND_MARKED     = 1, // do not collapse groups containing marked species
    COLLAPSE_TERMINAL = 2, // do not collapse groups with subgroups
    EXPAND_ALL        = 4,
    EXPAND_COLOR      = 8, // do not collapse groups containing species with color == parameter 'color_group'
};

class AWT_graphic_tree : public AWT_graphic, virtual Noncopyable {
    char         *species_name;
    AW::Position  cursor;

    int    baselinewidth;
    int    show_brackets;
    int    show_circle;
    int    use_ellipse;
    float  circle_zoom_factor;
    float  circle_max_size;

    int zombies; // # of zombies during last load()
    int duplicates; // # of duplicates during last load()

    AW_pos paint_irs_sub_tree(AP_tree *node, AW_pos x_offset); // returns y pos

    void unload();

    // variables - tree compatibility

    AP_tree * tree_proto;
    bool link_to_database; // link on load ?

    double list_tree_ruler_y;
    double irs_tree_ruler_scale_factor;

    AWT_scaled_font_limits scaled_font;
    double                 scaled_branch_distance; // vertical distance between branches (may be extra-scaled in options)

    AW_pos grey_level;

    AW_device *disp_device; // device for recursive functions

    const AW_bitset line_filter, vert_line_filter, mark_filter, group_bracket_filter, bs_circle_filter;
    const AW_bitset leaf_text_filter, group_text_filter, remark_text_filter, other_text_filter;
    const AW_bitset ruler_filter, root_filter;

    bool nds_show_all;

    AD_map_viewer_cb  map_viewer_cb;
    AWT_command_data  *cmd_data;

    void scale_text_koordinaten(AW_device *device, int gc, double& x, double& y, double orientation, int flag);

    // functions to compute displayinformation

    void show_dendrogram(AP_tree *at, AW::Position& pen, DendroSubtreeLimits& limits);

    void show_radial_tree(AP_tree *at,
                          double   x_center,
                          double   y_center,
                          double   tree_sprad,
                          double   tree_orientation,
                          double   x_root,
                          double   y_root);

    void show_nds_list(GBDATA * gb_main, bool use_nds);
    void show_irs_tree(AP_tree *at, double height);

    void box(int gc, const AW::Position& pos, int pixel_width, bool filled);
    void filled_box(int gc, const AW::Position& pos, int pixel_width) { box(gc, pos, pixel_width, true); }
    void empty_box(int gc, const AW::Position& pos, int pixel_width) { box(gc, pos, pixel_width, false); }
    void diamond(int gc, const AW::Position& pos, int pixel_width);

    const char *ruler_awar(const char *name);

    void set_line_attributes_for(AP_tree *at) const {
        disp_device->set_line_attributes(at->gr.gc, at->get_linewidth()+baselinewidth, AW_SOLID);
    }

    virtual void read_tree_settings();
    void update_structure() {
        AP_tree *root = get_root_node();
        if (root) root->compute_tree();
    }
    void apply_zoom_settings_for_treetype(AWT_canvas *ntw);

    int draw_branch_line(int gc, const AW::Position& root, const AW::Position& leaf, AW_bitset filter) {
        const AW_click_cd *old = disp_device->get_click_cd();
        td_assert(old && old->get_cd1() && !old->get_cd2()); // cd1 should be the node

        AW_click_cd branch(disp_device, old->get_cd1(), (AW_CL)"branch");
        return disp_device->line(gc, root, leaf, filter);
    }

    bool warn_inappropriate_mode(AWT_COMMAND_MODE mode);

protected:
    void store_command_data(AWT_command_data *new_cmd_data) {
        delete cmd_data;
        cmd_data = new_cmd_data;
    }
    AWT_command_data *get_command_data() { return cmd_data; }

public:

    // *********** read only variables !!!

    AW_root      *aw_root;
    AP_tree_display_type  tree_sort;
    AP_tree      *displayed_root; // root node of shown (sub-)tree; differs from real root if tree is zoomed logically
    AP_tree_root *tree_static;
    GBDATA       *gb_main;

    // *********** public section

    AWT_graphic_tree(AW_root *aw_root, GBDATA *gb_main, AD_map_viewer_cb map_viewer_cb);
    ~AWT_graphic_tree() OVERRIDE;

    AP_tree *get_root_node() { return tree_static ? tree_static->get_root_node() : NULL; }
    bool is_logically_zoomed() { return displayed_root != get_root_node(); }

    void init(RootedTreeNodeFactory *nodeMaker_, AliView *aliview, AP_sequence *seq_prototype, bool link_to_database_, bool insert_delete_cbs);
    AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *ntw) OVERRIDE;

    void show(AW_device *device) OVERRIDE;
    const AW::Position& get_cursor() const { return cursor; }

    void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) OVERRIDE;

private:
    void handle_key(AW_device *device, AWT_graphic_event& event);
public:
    void handle_command(AW_device *device, AWT_graphic_event& event) OVERRIDE;

    void mark_species_in_tree(AP_tree *at, int mark);
    void mark_species_in_tree_that(AP_tree *at, int mark, int (*condition)(GBDATA*, void*), void *cd);

    void mark_species_in_rest_of_tree(AP_tree *at, int mark);
    void mark_species_in_rest_of_tree_that(AP_tree *at, int mark, int (*condition)(GBDATA*, void*), void *cd);

    bool tree_has_marks(AP_tree *at);
    bool rest_tree_has_marks(AP_tree *at);

    void detect_group_state(AP_tree *at, AWT_graphic_tree_group_state *state, AP_tree *skip_this_son);

    bool     group_tree(AP_tree *at, CollapseMode mode, int color_group);
    void     group_rest_tree(AP_tree *at, CollapseMode mode, int color_group);
    void     reorder_tree(TreeOrder mode);
    GB_ERROR create_group(AP_tree * at) __ATTR__USERESULT;
    void     toggle_group(AP_tree * at);
    GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL,  AW_CL) OVERRIDE __ATTR__USERESULT;
    GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) OVERRIDE __ATTR__USERESULT;
    int      check_update(GBDATA *gb_main) OVERRIDE;         // reload tree if needed
    void     update(GBDATA *gb_main) OVERRIDE;
    void     set_tree_type(AP_tree_display_type type, AWT_canvas *ntw);

    double get_irs_tree_ruler_scale_factor() const { return irs_tree_ruler_scale_factor; }
    void show_ruler(AW_device *device, int gc);
    void get_zombies_and_duplicates(int& zomb, int& dups) const { zomb = zombies; dups = duplicates; }

#if defined(UNIT_TESTS) // UT_DIFF
    friend class fake_AWT_graphic_tree;
#endif
};

class ClickedTarget {
    /*! Represents any target corresponding to some (mouse-)position in the tree display.
     *
     * The target is e.g. used as target for keystrokes or mouse clicks.
     *
     * For AP_LIST_NDS, this only represents the species (w/o any tree information).
     * For other tree display modes, this represents a specific tree node.
     *
     * The space outside the tree does represent the whole tree (aka the root-node).
     * (the necessary distance to the tree-structure/-text is defined by AWT_CATCH)
     */

    AP_tree *tree_node;
    GBDATA  *gb_species;
    bool     ruler;
    bool     branch;

    const AW_clicked_element *elem;

    void init() {
        tree_node  = NULL;
        gb_species = NULL;
        ruler      = false;
        branch     = false;
    }

    void identify(AWT_graphic_tree *agt) {
        init();
        if (elem && elem->exists) {
            const char *what = (const char*)elem->cd2();

            if (what) {
                if (strcmp(what, "species") == 0) { // entry in NDS list
                    gb_species = (GBDATA*)elem->cd1();
                    td_assert(gb_species);
                }
                else if (strcmp(what, "ruler") == 0) {
                    ruler = !elem->cd1();
                }
                else if (strcmp(what, "branch") == 0) {
                    branch = true; // indicates that a line really IS the branch (opposed to other branch-related lines like e.g. group-brackets)
                }
                else {
                    td_assert(0); // unknown element type
                }
            }

            if (!(gb_species || ruler)) {
                tree_node = (AP_tree*)elem->cd1();
                td_assert(branch || !what);
            }
        }
        else { // use whole tree if mouse does not point to a subtree
            tree_node = agt ? agt->get_root_node() : NULL;
        }
        td_assert(implicated(branch, tree_node));
    }

public:

    ClickedTarget(AWT_graphic_tree *agt, const AW_clicked_element *clicked) : elem(clicked) {
        // uses root of tree as target, when a position outside of the tree is selected
        // (e.g. used for key-commands)
        identify(agt);
    }
    ClickedTarget(const AW_clicked_element *clicked) : elem(clicked) {
        // accept only normal branches as targets
        identify(NULL);
    }

    const AW_clicked_element *element() const { return elem; }
    AP_tree *node() const { return tree_node; }
    GBDATA *species() const { return gb_species; }

    bool is_text() const { return elem && elem->is_text(); }
    bool is_line() const { return elem && elem->is_line(); }
    bool is_branch() const { return branch; }
    bool is_ruler() const { return ruler; }

    double get_rel_attach() const {
        // return [0..1] according to exact position where element is dropped
        if (is_line() && (is_branch() || ruler)) return elem->get_rel_pos();
        return 0.5; // act like "drop on branch-center"
    }
};

AWT_graphic_tree *NT_generate_tree(AW_root *root, GBDATA *gb_main, AD_map_viewer_cb map_viewer_cb);
bool AWT_show_branch_remark(AW_device *device, const char *remark_branch, bool is_leaf, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri);

#else
#error TreeDisplay.hxx included twice
#endif // TREEDISPLAY_HXX
