#ifndef awt_dtree_hxx_included
#define awt_dtree_hxx_included

#define AWAR_DTREE_BASELINEWIDTH "awt/dtree/baselinewidth"
#define AWAR_DTREE_VERICAL_DIST  "awt/dtree/verticaldist"
#define AWAR_DTREE_AUTO_JUMP     "awt/dtree/autojump"
#define AWAR_DTREE_SHOW_CIRCLE   "awt/dtree/show_circle"
#define AWAR_DTREE_CIRCLE_ZOOM   "awt/dtree/circle_zoom"
#define AWAR_DTREE_GREY_LEVEL    "awt/dtree/greylevel"

#define AWAR_DTREE_REFRESH       AWAR_TREE_REFRESH // touch this awar to refresh the tree display

void awt_create_dtree_awars(AW_root *aw_root,AW_default def);

#define NT_BOX_WIDTH      3.5   /* pixel/2 ! */
#define NT_ROOT_WIDTH     4.5   /* pixel/2 ! */
#define NT_SELECTED_WIDTH 5.5
#define PH_CLICK_SPREAD   0.10

#define AWT_TREE(ntw) ((AWT_graphic_tree *)ntw->tree_disp)


typedef enum {
    AP_TREE_NORMAL, // normal tree display
    AP_TREE_RADIAL, // radial tree display
    AP_TREE_IRS, // like AP_TREE_NORMAL, with folding line
    AP_LIST_NDS,
    AP_LIST_SIMPLE // simple display only showing name (used at startup to avoid NDS error messages)
} AP_tree_sort;

inline bool sort_is_list_style(AP_tree_sort sort) { return sort == AP_LIST_NDS || sort == AP_LIST_SIMPLE; }
inline bool sort_is_tree_style(AP_tree_sort sort) { return !sort_is_list_style(sort); }


class AWT_graphic_tree_group_state;

class AWT_graphic_tree : public AWT_graphic {
protected:

    // variables - tree compatibility

    AP_tree * tree_proto;
    double y_pos;
    double list_tree_ruler_y;
    double irs_tree_ruler_scale_factor;
    double scale;
    AW_pos  grey_level;
    // internal command exec. var.
    double rot_orientation;
    double rot_spread;
    AW_clicked_line rot_cl;
    AW_clicked_text rot_ct;
    AW_clicked_line old_rot_cl;
    AP_tree *rot_at;

    AW_device *disp_device; // device for  rekursiv Funktions
    void scale_text_koordinaten(AW_device *device, int gc, double& x,double& y,double orientation,int flag );

    AW_bitset line_filter,vert_line_filter, text_filter,mark_filter;
    AW_bitset ruler_filter, root_filter;
    int       treemodus;
    bool      nds_show_all;

    // functions to compute displayinformation
    double show_list_tree_rek(AP_tree * at, double x_father,
                              double x_son);
    void show_tree_rek(AP_tree *at, double x_center,
                       double y_center,double tree_sprad,
                       double tree_orientation,
                       double x_root, double y_root, int linewidth);
    void show_nds_list_rek(GBDATA * gb_main, bool use_nds);

    void NT_scalebox(int gc, double x, double y, double width);
    void NT_emptybox(int gc, double x, double y, double width);
    void NT_rotbox(int gc, double x, double y, double width);
    const char *show_ruler(AW_device *device, int gc);
    void rot_show_triangle( AW_device *device);
    void rot_show_line( AW_device *device );

    void show_irs(AP_tree *at,AW_device *device, int height);
    int draw_slot(int x_offset, GB_BOOL draw_at_tips); // return max_x
    int paint_sub_tree(AP_tree *node, int x_offset, int type); // returns y pos

    void          unload();
    char         *species_name;
    int           baselinewidth;
    int           show_circle;
    float         circle_zoom_factor;
public:
    // *********** read only variables !!!
    AW_root      *aw_root;
    AP_tree_sort  tree_sort;
    AP_tree *     tree_root;
    AP_tree *     tree_root_display;
    AP_tree_root *tree_static;
    GBDATA       *gb_main;
    char         *tree_name;

    AP_FLOAT x_cursor,y_cursor;
    // *********** public section
    AWT_graphic_tree(AW_root *aw_root, GBDATA *gb_main);
    virtual ~AWT_graphic_tree(void);

    void init(AP_tree * tree_prot);
    virtual AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    virtual void show(AW_device *device);

    virtual void info(AW_device *device, AW_pos x, AW_pos y,
                      AW_clicked_line *cl, AW_clicked_text *ct);

    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, char key_char, AW_event_type type,
                         AW_pos x, AW_pos y,
                         AW_clicked_line *cl, AW_clicked_text *ct);

    void key_command(AWT_COMMAND_MODE cmd, AW_key_mod key_modifier, char key_char,
                     AW_pos           x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);

    void mark_species_in_tree(AP_tree *at, int mark);
    void mark_species_in_tree_that(AP_tree *at, int mark, int (*condition)(GBDATA*, void*), void *cd);

    void mark_species_in_rest_of_tree(AP_tree *at, int mark);
    void mark_species_in_rest_of_tree_that(AP_tree *at, int mark, int (*condition)(GBDATA*, void*), void *cd);

    bool tree_has_marks(AP_tree *at);
    bool rest_tree_has_marks(AP_tree *at);

    void detect_group_state(AP_tree *at, AWT_graphic_tree_group_state *state, AP_tree *skip_this_son);

    int       group_tree(struct AP_tree *at, int mode, int color_group);
    int       group_rest_tree(AP_tree *at, int mode, int color_group);
    int       resort_tree(int mode, struct AP_tree *at = 0 );
    AW_BOOL   create_group(AP_tree * at);
    void      toggle_group(AP_tree * at);
    void      jump(AP_tree *at, const char *name);
    AP_tree  *search(AP_tree *root, const char *name);
    GB_ERROR  load(GBDATA *gb_main, const char *name,AW_CL link_to_database, AW_CL insert_delete_cbs);
    GB_ERROR  save(GBDATA *gb_main, const char *name,AW_CL cd1, AW_CL cd2);
    int       check_update(GBDATA *gb_main); // reload tree if needed
    void      update(GBDATA *gb_main);
    void      set_tree_type(AP_tree_sort type);

    double get_irs_tree_ruler_scale_factor() const { return irs_tree_ruler_scale_factor; }
};

AWT_graphic *NT_generate_tree( AW_root *root,GBDATA *gb_main );
AW_BOOL      AWT_show_remark_branch(AW_device *device, const char *remark_branch, AW_BOOL is_leaf, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2);


#endif
