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
    AWT_GC_ZOMBIES,

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

enum AWT_RemoveType { // bit flags
    AWT_REMOVE_MARKED        = GBT_REMOVE_MARKED,
    AWT_REMOVE_UNMARKED      = GBT_REMOVE_UNMARKED,
    AWT_REMOVE_ZOMBIES       = GBT_REMOVE_ZOMBIES,
    AWT_REMOVE_NO_SEQUENCE   = 8,
    AWT_REMOVE_BUT_DONT_FREE = 16,

    // please keep AWT_RemoveType in sync with GBT_TreeRemoveType
    // see ../../ARBDB/arbdbt.h@sync_GBT_TreeRemoveType__AWT_RemoveType

    // combined defines:
    AWT_KEEP_MARKED = AWT_REMOVE_UNMARKED|AWT_REMOVE_ZOMBIES,
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

class AP_tree_root : public ARB_seqtree_root { // derived from a Noncopyable
    AP_rootChangedCb  root_changed_cb;
    void             *root_changed_cd;
    AP_nodeDelCb      node_deleted_cb;
    void             *node_deleted_cd;

    GBDATA *gb_species_data;                        // @@@ needed ?

public:
    GBDATA *gb_tree_gone; // if all leafs have been removed by tree operations, remember 'ARB_seqtree_root::gb_tree' here (see change_root)
    char   *gone_tree_name; // set to old tree name when gb_tree_gone gets deleted (used for auto-recreation)

    GBDATA   *gb_table_data;
    long      tree_timer;
    long      species_timer;
    long      table_timer;
    AP_rates *rates;

    AP_tree_root(AliView *aliView, RootedTreeNodeFactory *nodeMaker_, AP_sequence *seq_proto, bool add_delete_callbacks);
    ~AP_tree_root() OVERRIDE;
    DEFINE_TREE_ROOT_ACCESSORS(AP_tree_root, AP_tree);

    // ARB_seqtree_root interface

    virtual void change_root(RootedTree *old, RootedTree *newroot) OVERRIDE;

    virtual GB_ERROR loadFromDB(const char *name) OVERRIDE;
    virtual GB_ERROR saveToDB() OVERRIDE;

    // AP_tree_root interface

    void update_timers();                           // update the timer
    bool is_tree_updated();
    bool is_species_updated();

    void inform_about_delete(AP_tree *old);

    void set_root_changed_callback(AP_rootChangedCb cb, void *cd);
    void set_node_deleted_callback(AP_nodeDelCb cb, void *cd);

    long remove_leafs(AWT_RemoveType awt_remove_type);
};

namespace tree_defaults {
    const float SPREAD    = 1.0;
    const float ANGLE     = 0.0;
    const char  LINEWIDTH = 0;
    const float LENGTH    = DEFAULT_BRANCH_LENGTH;
};

struct AP_tree_members {
public: // @@@ make members private
    unsigned int grouped : 1;   // indicates a folded group
    unsigned int hidden : 1;    // not shown because a father is a folded group
    unsigned int has_marked_children : 1; // at least one child is marked
    unsigned int callback_exists : 1;
    unsigned int gc : 6;        // color

    char left_linewidth; // @@@ it's stupid to store linewidth IN FATHER (also wastes space)
    char right_linewidth;

    unsigned leaf_sum;   // number of leaf children of this node
    unsigned view_sum;   // virtual size of node for display ( isgrouped?sqrt(leaf_sum):leaf_sum )

    float max_tree_depth; // max length of path; for drawing triangles
    float min_tree_depth; // min length of path; for drawing triangle
    float spread;

    float left_angle;     // @@@ it's stupid to store angles IN FATHER (also wastes space)
    float right_angle;

    void reset_child_spread() {
        spread = tree_defaults::SPREAD;
    }
    void reset_both_child_angles() {
        left_angle  = tree_defaults::ANGLE;
        right_angle = tree_defaults::ANGLE;
    }
    void reset_both_child_linewidths() {
        left_linewidth  = tree_defaults::LINEWIDTH;
        right_linewidth = tree_defaults::LINEWIDTH;
    }
    void reset_child_layout() {
        reset_child_spread();
        reset_both_child_angles();
        reset_both_child_linewidths();
    }

    void clear() {
        reset_child_layout();

        grouped             = 0;
        hidden              = 0;
        has_marked_children = 0;
        callback_exists     = 0;
        gc                  = 0;
        leaf_sum            = 0;
        view_sum            = 0;
        max_tree_depth      = 0;
        min_tree_depth      = 0;
    }

    void swap_son_layout() {
        std::swap(left_linewidth, right_linewidth);

        // angles need to change orientation when swapped
        // (they are relative angles, i.e. represent the difference to the default-angle)
        float org_left = left_angle;
        left_angle     = -right_angle;
        right_angle    = -org_left;
    }
};

struct AP_branch_members {
public:
    unsigned int touched : 1;       // nni and kl

    void clear() {
        touched = 0;
    }
};

class AP_tree : public ARB_seqtree {
public: // @@@ fix public members
    AP_tree_members   gr;
    AP_branch_members br;

    unsigned long stack_level; // @@@ maybe can be moved to AP_tree_nlen

    // ------------------
    //      functions
private:
    void load_node_info();    // load linewidth etc from DB

    char& linewidth_ref() {
        AP_tree_members& tm = get_father()->gr;
        return is_leftson() ? tm.left_linewidth : tm.right_linewidth;
    }
    const char& linewidth_ref() const { return const_cast<AP_tree*>(this)->linewidth_ref(); }

    float& angle_ref() {
        AP_tree_members& tm = get_father()->gr;
        return is_leftson() ? tm.left_angle : tm.right_angle;
    }
    const float& angle_ref() const { return const_cast<AP_tree*>(this)->angle_ref(); }

    static inline int force_legal_width(int width) { return width<0 ? 0 : (width>128 ? 128 : width); }

    void buildLeafList_rek(AP_tree **list, long& num);
    void buildNodeList_rek(AP_tree **list, long& num);
    void buildBranchList_rek(AP_tree **list, long& num, bool create_terminal_branches, int deep);

    const AP_tree *flag_branch() const { return get_father()->get_father() ? this : get_father()->get_leftson(); }

    void reset_child_angles();
    void reset_child_linewidths();
    void reset_child_layout();

    void update_subtree_information();

public:
    explicit AP_tree(AP_tree_root *troot)
        : ARB_seqtree(troot),
          stack_level(0)
    {
        gr.clear();
        br.clear();
    }
    ~AP_tree() OVERRIDE;

    DEFINE_TREE_ACCESSORS(AP_tree_root, AP_tree);

    void compute_tree() OVERRIDE;

    unsigned count_leafs() const;
    unsigned get_leaf_count() const OVERRIDE { // assumes compute_tree has been called (since last tree modification)
        return gr.leaf_sum;
    }


    void load_subtree_info(); // recursive load_node_info (no need to call, called by loadFromDB)

    int colorize(GB_HASH *hashptr);  // function for coloring the tree; ak
    void uncolorize() { compute_tree(); }

    virtual void insert(AP_tree *new_brother);
    virtual void initial_insert(AP_tree *new_brother, AP_tree_root *troot);
    virtual void remove();                          // remove this+father (but do not delete)
    virtual void swap_assymetric(AP_TREE_SIDE mode); // 0 = AP_LEFT_son  1=AP_RIGHT_son

    void swap_sons() OVERRIDE {
        rt_assert(!is_leaf); // @@@ if never fails -> remove condition below 
        if (!is_leaf) {
            ARB_seqtree::swap_sons();
            gr.swap_son_layout();
        }
    }

    GB_ERROR cantMoveNextTo(AP_tree *new_brother);  // use this to detect impossible moves
    virtual void moveNextTo(AP_tree *new_brother, AP_FLOAT rel_pos); // move to new brother

    void set_root() OVERRIDE;

    GB_ERROR tree_write_tree_rek(GBDATA *gb_tree);
    GB_ERROR relink() __ATTR__USERESULT; // @@@ used ? if yes -> move to AP_tree_root or ARB_seqtree_root

    virtual AP_UPDATE_FLAGS check_update();

    void update();

    int get_linewidth() const { return is_root_node() ? 0 : linewidth_ref(); }
    void set_linewidth(int width) { if (father) linewidth_ref() = force_legal_width(width); }
    void reset_linewidth() { set_linewidth(tree_defaults::LINEWIDTH); }
    void set_linewidth_recursive(int width);

    float get_angle() const { return is_root_node() ? 0.0 : angle_ref(); }
    void set_angle(float angle) {
        if (father) {
            angle_ref() = angle;
            if (get_father()->is_root_node()) {
                // always set angle of other son at root-node
                // @@@ works wrong if locigal-zoom is active
                get_brother()->angle_ref() = angle;
            }
        }
    }
    void reset_angle() { set_angle(tree_defaults::ANGLE); }

    void buildLeafList(AP_tree **&list, long &num); // returns a list of leafs
    void buildNodeList(AP_tree **&list, long &num); // returns a list of inner nodes (w/o root)
    void buildBranchList(AP_tree **&list, long &num, bool create_terminal_branches, int deep);

    AP_tree **getRandomNodes(int nnodes); // returns a list of random nodes (no leafs)

    void clear_branch_flags();

    void touch_branch() { const_cast<AP_tree*>(flag_branch())->br.touched = 1; }
    int get_branch_flag() const { return flag_branch()->br.touched; }

    GB_ERROR move_group_info(AP_tree *new_group) __ATTR__USERESULT;
    bool is_inside_folded_group() const;

    void mark_duplicates();
    const char *mark_long_branches(double min_rel_diff, double min_abs_diff);
    const char *mark_deep_leafs(int min_depth, double min_rootdist);
    const char *mark_degenerated_branches(double degeneration_factor);
    const char *analyse_distances();

    void justify_branch_lenghs(GBDATA *gb_main);
    void relink_tree(GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash), GB_HASH *organism_hash);

    // reset-functions below affect 'this' and childs:
    void reset_subtree_spreads();
    void reset_subtree_angles();
    void reset_subtree_linewidths();
    void reset_subtree_layout();

    bool hasName(const char *Name) const { return Name && name && Name[0] == name[0] && strcmp(Name, name) == 0; }
};

struct AP_TreeNodeFactory : public RootedTreeNodeFactory {
    virtual RootedTree *makeNode(TreeRoot *root) const {
        return new AP_tree(DOWNCAST(AP_tree_root*, root));
    }
};

#else
#error AP_Tree.hxx included twice
#endif // AP_TREE_HXX
