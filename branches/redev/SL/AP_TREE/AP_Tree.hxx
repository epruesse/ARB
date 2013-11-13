// =============================================================== //
//                                                                 //
//   File      : AP_Tree.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_TREE_HXX
#define AP_TREE_HXX

#define AP_F_LOADED    ((AW_active)1)
#define AP_F_NLOADED   ((AW_active)2)
#define AP_F_SEQUENCES ((AW_active)4)
#define AP_F_MATRIX    ((AW_active)8)
#define AP_F_TREE      ((AW_active)16)
#define AP_F_ALL       ((AW_active)-1)

#define GROUPED_SUM 2   // min. no of species in a group which should be drawn as box

#ifndef ARB_TREE_HXX
#include <ARB_Tree.hxx>
#endif
#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif
#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif

enum {
    AWT_GC_CURSOR=0,
    AWT_GC_BRANCH_REMARK,
    AWT_GC_BOOTSTRAP,
    AWT_GC_BOOTSTRAP_LIMITED,
    AWT_GC_GROUPS,
    AWT_GC_SELECTED,        // == zero mismatches
    AWT_GC_UNDIFF,
    AWT_GC_NSELECTED,       // no hit
    AWT_GC_SOME_MISMATCHES,

    // for multiprobecoloring

    AWT_GC_BLACK,   AWT_GC_YELLOW,
    AWT_GC_RED,     AWT_GC_MAGENTA,
    AWT_GC_GREEN,   AWT_GC_CYAN,
    AWT_GC_BLUE,    AWT_GC_WHITE,

    AWT_GC_FIRST_COLOR_GROUP,
    AWT_GC_MAX = AWT_GC_FIRST_COLOR_GROUP+AW_COLOR_GROUPS
};

enum AP_STACK_MODE {
    NOTHING   = 0,                                                      // nothing to buffer in AP_tree node
    STRUCTURE = 1,                                                      // only structure
    SEQUENCE  = 2,                                                      // only sequence
    BOTH      = 3,                                                      // sequence & treestructure is buffered
    ROOT      = 7                                                       // old root is buffered
};

enum AP_UPDATE_FLAGS {
    AP_UPDATE_OK       = 0,
    AP_UPDATE_RELINKED = -1,
    AP_UPDATE_RELOADED = 1,
    AP_UPDATE_ERROR    = 2
};

enum AP_TREE_SIDE {  // flags zum kennzeichnen von knoten
    AP_LEFT,
    AP_RIGHT,
    AP_FATHER,
    AP_LEFTSON,
    AP_RIGHTSON,
    AP_NONE
};

enum AWT_REMOVE_TYPE { // bit flags
    AWT_REMOVE_MARKED        = GBT_REMOVE_MARKED,
    AWT_REMOVE_NOT_MARKED    = GBT_REMOVE_NOT_MARKED,
    AWT_REMOVE_DELETED       = GBT_REMOVE_DELETED,
    AWT_REMOVE_NO_SEQUENCE   = 8,
    AWT_REMOVE_BUT_DONT_FREE = 16

    // please keep AWT_REMOVE_TYPE in sync with GBT_TREE_REMOVE_TYPE
    // see ../../ARBDB/arbdbt.h@sync_GBT_TREE_REMOVE_TYPE_AWT_REMOVE_TYPE
};

struct AP_rates : virtual Noncopyable {
    AP_FLOAT  *rates;
    long       rate_len;
    AP_filter *filter;
    long       update;

    AP_rates();
    char *init(AP_filter *fil);
    char *init(AP_FLOAT * ra, AP_filter *fil);
    ~AP_rates();
    void print();
};

// ---------------------
//      AP_tree_root

class AP_tree;

typedef void (*AP_rootChangedCb)(void *cd, AP_tree *old, AP_tree *newroot);
typedef void (*AP_nodeDelCb)(void *cd, AP_tree *del);

class AP_tree_root : public ARB_tree_root { // derived from a Noncopyable
    AP_rootChangedCb  root_changed_cb;
    void             *root_changed_cd;
    AP_nodeDelCb      node_deleted_cb;
    void             *node_deleted_cd;

    GBDATA *gb_species_data;                        // @@@ needed ?

public:
    GBDATA   *gb_tree_gone;                         // if all leafs have been removed by tree operations, remember 'ARB_tree_root::gb_tree' here (see change_root)
    GBDATA   *gb_table_data;
    long      tree_timer;
    long      species_timer;
    long      table_timer;
    AP_rates *rates;

    AP_tree_root(AliView *aliView, const AP_tree& tree_proto, AP_sequence *seq_proto, bool add_delete_callbacks);
    virtual ~AP_tree_root() OVERRIDE;
    DEFINE_TREE_ROOT_ACCESSORS(AP_tree_root, AP_tree);

    // ARB_tree_root interface

    virtual void change_root(ARB_tree *old, ARB_tree *newroot) OVERRIDE;

    virtual GB_ERROR loadFromDB(const char *name) OVERRIDE;
    virtual GB_ERROR saveToDB() OVERRIDE;

    // AP_tree_root interface

    void update_timers();                           // update the timer
    bool is_tree_updated();
    bool is_species_updated();

    void inform_about_delete(AP_tree *old);

    void set_root_changed_callback(AP_rootChangedCb cb, void *cd);
    void set_node_deleted_callback(AP_nodeDelCb cb, void *cd);

    long remove_leafs(int awt_remove_type);
    ARB_edge find_innermost_edge();
};

#define DEFAULT_LINEWIDTH 0

struct AP_tree_members {
public: // @@@ make members private
    unsigned int grouped : 1;   // indicates a folded group
    unsigned int hidden : 1;    // not shown because a father is a folded group
    unsigned int has_marked_children : 1; // at least one child is marked
    unsigned int callback_exists : 1;
    unsigned int gc : 6;        // color

    char left_linewidth; // @@@ it's stupid to store linewidth IN FATHER (also wastes space)
    char right_linewidth;

    int leaf_sum;   // number of leaf children of this node
    int view_sum;   // virtual size of node for display ( isgrouped?sqrt(leaf_sum):leaf_sum )

    float tree_depth;     // max length of path; for drawing triangles
    float min_tree_depth; // min length of path; for drawing triangle
    float spread;

    float left_angle;   // @@@ it's stupid to store angles IN FATHER (also wastes space)
    float right_angle;

    void reset_spread() {
        spread = 1.0;
    }
    void reset_rotation() {
        left_angle  = 0;
        right_angle = 0;
    }
    void reset_linewidths() {
        left_linewidth  = DEFAULT_LINEWIDTH;
        right_linewidth = DEFAULT_LINEWIDTH;
    }
    void reset_layout() {
        reset_spread();
        reset_rotation();
        reset_linewidths();
    }

    void clear() {
        reset_layout();

        grouped             = 0;
        hidden              = 0;
        has_marked_children = 0;
        callback_exists     = 0;
        gc                  = 0;
        leaf_sum            = 0;
        view_sum            = 0;
        tree_depth          = 0;
        min_tree_depth      = 0;
    }

    void swap_son_layout();
};

struct AP_branch_members {
public:
    unsigned int touched : 1;       // nni and kl

    void clear() {
        touched = 0;
    }
};


class AP_tree : public ARB_tree {
public: // @@@ fix public member
    AP_tree_members   gr;
    AP_branch_members br;
    unsigned long     stack_level;

    // ------------------
    //      functions
private:
    void load_node_info();                          // load linewidth etc from DB

public:

    explicit AP_tree(AP_tree_root *tree_root);
    virtual ~AP_tree() OVERRIDE; // leave this here to force creation of virtual table
    DEFINE_TREE_ACCESSORS(AP_tree_root, AP_tree);

    // ARB_tree interface
    virtual AP_tree *dup() const OVERRIDE;
    // ARB_tree interface (end)

    int compute_tree(GBDATA *gb_main);

    int arb_tree_set_leafsum_viewsum();             // count all visible leafs -> gr.viewsum + gr.leafsum
    int arb_tree_leafsum2();                        // count all leafs

    void calc_hidden_flag(int father_is_hidden);
    int  calc_color();                        // start a transaction first
    int  calc_color_probes(GB_HASH *hashptr); // new function for coloring the tree; ak

    GBT_LEN arb_tree_min_deep();
    GBT_LEN arb_tree_deep();

    virtual void insert(AP_tree *new_brother);
    virtual void remove();                          // remove this+father (but do not delete)
    virtual void swap_assymetric(AP_TREE_SIDE mode); // 0 = AP_LEFT_son  1=AP_RIGHT_son

    void swap_sons();
    void swap_featured_sons();
    void rotate_subtree(); // flip whole subtree ( = recursive swap_sons())

    GB_ERROR cantMoveNextTo(AP_tree *new_brother);  // use this to detect impossible moves
    virtual void moveNextTo(AP_tree *new_brother, AP_FLOAT rel_pos); // move to new brother

    virtual void set_root();

    void remove_bootstrap();                        // remove bootstrap values from subtree
    void reset_branchlengths();                     // reset branchlengths of subtree to DEFAULT_BRANCH_LENGTH
    void scale_branchlengths(double factor);
    void bootstrap2branchlen();                     // copy bootstraps to branchlengths
    void branchlen2bootstrap();                     // copy branchlengths to bootstraps

    virtual void move_gbt_info(GBT_TREE *tree) OVERRIDE;

    GB_ERROR tree_write_tree_rek(GBDATA *gb_tree);
    GB_ERROR relink() __ATTR__USERESULT; // @@@ used ? if yes -> move to AP_tree_root or ARB_tree_root

    virtual AP_UPDATE_FLAGS check_update();

    void update();

    int get_linewidth() const {
        if (!father) return 0;
        const AP_tree_members& fgr = get_father()->gr;
        return is_leftson(father) ? fgr.left_linewidth : fgr.right_linewidth;
    }
    // cppcheck-suppress functionConst
    void set_linewidth_recursive(int width);
    void set_linewidth(int width) {
        ap_assert(width >= 0 && width < 128);
        if (father) {
            AP_tree_members& fgr = get_father()->gr;
            char& lw = is_leftson(father) ? fgr.left_linewidth : fgr.right_linewidth;
            lw       = width;
        }
    }

    float get_angle() const {
        if (!father) return 0;
        const AP_tree_members& fgr = get_father()->gr;
        return is_leftson(father) ? fgr.left_angle : fgr.right_angle;
    }
    void set_angle(double angle) {
        if (father) {
            AP_tree_members& fgr = get_father()->gr;
            float& a = is_leftson(father) ? fgr.left_angle : fgr.right_angle;
            a        = angle;

            if (father->is_root_node()) {
                // always set angle of other son at root-node
                // @@@ works wrong if locigal-zoom is active
                float& b = is_leftson(father) ? fgr.right_angle : fgr.left_angle;
                b        = angle;
            }
        }
    }

private:
    void buildLeafList_rek(AP_tree **list, long& num);
    void buildNodeList_rek(AP_tree **list, long& num);
    void buildBranchList_rek(AP_tree **list, long& num, bool create_terminal_branches, int deep);

    const AP_tree *flag_branch() const { return get_father()->get_father() ? this : get_father()->get_leftson(); }

public:
    void buildLeafList(AP_tree **&list, long &num); // returns a list of leafs
    void buildNodeList(AP_tree **&list, long &num); // returns a list of inner nodes (w/o root)
    void buildBranchList(AP_tree **&list, long &num, bool create_terminal_branches, int deep);

    AP_tree **getRandomNodes(int nnodes); // returns a list of random nodes (no leafs)

    void replace_self(AP_tree *new_son);
    void set_brother(AP_tree *new_son);

    void clear_branch_flags();

    void touch_branch() { const_cast<AP_tree*>(flag_branch())->br.touched = 1; }
    int get_branch_flag() const { return flag_branch()->br.touched; }

    GB_ERROR move_group_info(AP_tree *new_group) __ATTR__USERESULT;

    void mark_duplicates();
    const char *mark_long_branches(double min_rel_diff, double min_abs_diff);
    const char *mark_deep_leafs(int min_depth, double min_rootdist);
    const char *mark_degenerated_branches(double degeneration_factor);
    const char *analyse_distances();

    void justify_branch_lenghs(GBDATA *gb_main);
    void relink_tree(GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash), GB_HASH *organism_hash);

    void reset_spread();
    void reset_rotation();
    void reset_linewidths();
    void reset_layout();

    bool hasName(const char *Name) const {
        return Name && name && Name[0] == name[0] && strcmp(Name, name) == 0;
    }
};

#else
#error AP_Tree.hxx included twice
#endif // AP_TREE_HXX
