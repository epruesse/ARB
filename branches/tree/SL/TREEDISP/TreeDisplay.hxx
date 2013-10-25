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

void awt_create_dtree_awars(AW_root *aw_root, AW_default def);

#define NT_BOX_WIDTH      7 // pixel
#define NT_ROOT_WIDTH     9
#define NT_SELECTED_WIDTH 11

#define PH_CLICK_SPREAD   0.10

#define AWT_TREE(ntw) DOWNCAST(AWT_graphic_tree*, (ntw)->gfx)


enum AP_tree_display_type {
    AP_TREE_NORMAL, // normal tree display (dendrogram)
    AP_TREE_RADIAL, // radial tree display
    AP_TREE_IRS, // like AP_TREE_NORMAL, with folding line
    AP_LIST_NDS,
    AP_LIST_SIMPLE // simple display only showing name (used at startup to avoid NDS error messages)
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

    AW_pos          grey_level;
    double          rot_orientation;
    double          rot_spread;
    AW_clicked_text rot_ct;

    AW_device *disp_device; // device for recursive functions

    AW_bitset line_filter, vert_line_filter, text_filter, mark_filter, group_bracket_filter;
    AW_bitset ruler_filter, root_filter;
    int       treemode;
    bool      nds_show_all;

    AD_map_viewer_cb map_viewer_cb;

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

    const char *show_ruler(AW_device *device, int gc);
    void        rot_show_triangle(AW_device *device);

    void set_line_attributes_for(AP_tree *at) const {
        disp_device->set_line_attributes(at->gr.gc, at->get_linewidth()+baselinewidth, AW_SOLID);
    }

    virtual void read_tree_settings();
    void apply_zoom_settings_for_treetype(AWT_canvas *ntw);
    
protected:

    AW_clicked_line  old_rot_cl;
    AW_clicked_line  rot_cl;
    AP_tree         *rot_at;

    void rot_show_line(AW_device *device);

public:

    // *********** read only variables !!!

    AW_root      *aw_root;
    AP_tree_display_type  tree_sort;
    AP_tree      *tree_root_display;                // @@@ what is this used for ?
    AP_tree_root *tree_static;
    GBDATA       *gb_main;

    // *********** public section

    AWT_graphic_tree(AW_root *aw_root, GBDATA *gb_main, AD_map_viewer_cb map_viewer_cb);
    virtual ~AWT_graphic_tree() OVERRIDE;

    AP_tree *get_root_node() { return tree_static ? tree_static->get_root_node() : NULL; }

    void init(const AP_tree& tree_prototype, AliView *aliview, AP_sequence *seq_prototype, bool link_to_database_, bool insert_delete_cbs);
    AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *ntw) OVERRIDE;

    virtual void show(AW_device *device) OVERRIDE;
    const AW::Position& get_cursor() const { return cursor; }

    virtual void info(AW_device *device, AW_pos x, AW_pos y,
                      AW_clicked_line *cl, AW_clicked_text *ct) OVERRIDE;

    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char, AW_event_type type,
                         AW_pos x, AW_pos y,
                         AW_clicked_line *cl, AW_clicked_text *ct) OVERRIDE;

    void key_command(AWT_COMMAND_MODE cmd, AW_key_mod key_modifier, char key_char,
                     AW_pos           x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);

    void mark_species_in_tree(AP_tree *at, int mark);
    void mark_species_in_tree_that(AP_tree *at, int mark, int (*condition)(GBDATA*, void*), void *cd);

    void mark_species_in_rest_of_tree(AP_tree *at, int mark);
    void mark_species_in_rest_of_tree_that(AP_tree *at, int mark, int (*condition)(GBDATA*, void*), void *cd);

    bool tree_has_marks(AP_tree *at);
    bool rest_tree_has_marks(AP_tree *at);

    void detect_group_state(AP_tree *at, AWT_graphic_tree_group_state *state, AP_tree *skip_this_son);

    int      group_tree(AP_tree *at, int mode, int color_group);
    void     group_rest_tree(AP_tree *at, int mode, int color_group);
    void     reorder_tree(TreeOrder mode);
    GB_ERROR create_group(AP_tree * at) __ATTR__USERESULT;
    void     toggle_group(AP_tree * at);
    void     jump(AP_tree *at, const char *name);
    AP_tree *search(AP_tree *root, const char *name);
    GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL,  AW_CL) OVERRIDE __ATTR__USERESULT;
    GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) OVERRIDE __ATTR__USERESULT;
    int      check_update(GBDATA *gb_main) OVERRIDE;         // reload tree if needed
    void     update(GBDATA *gb_main) OVERRIDE;
    void     set_tree_type(AP_tree_display_type type, AWT_canvas *ntw);

    double get_irs_tree_ruler_scale_factor() const { return irs_tree_ruler_scale_factor; }
    void get_zombies_and_duplicates(int& zomb, int& dups) const { zomb = zombies; dups = duplicates; }

#if defined(UNIT_TESTS)
    friend class fake_AWT_graphic_tree;
#endif
};

AWT_graphic_tree *NT_generate_tree(AW_root *root, GBDATA *gb_main, AD_map_viewer_cb map_viewer_cb);
bool AWT_show_branch_remark(AW_device *device, const char *remark_branch, bool is_leaf, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri);

#else
#error TreeDisplay.hxx included twice
#endif // TREEDISPLAY_HXX
