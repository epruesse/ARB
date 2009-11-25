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

// typedef unsigned char uchar;
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
}; // AW_gc

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
    // see ../ARBDB/arbdbt.h@sync_GBT_TREE_REMOVE_TYPE_AWT_REMOVE_TYPE
};

class AP_rates {
public:
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

// ---------------------------------------------
//      downcasts for AP_tree and AP_tree_root

#ifndef DOWNCAST_H
#include <downcast.h>
#endif

#define AP_TREE_CAST(arb_tree)                 DOWNCAST(AP_tree*, arb_tree)
#define AP_TREE_CONST_CAST(arb_tree)           DOWNCAST(const AP_tree*, arb_tree)
#define AP_TREE_ROOT_CAST(arb_tree_root)       DOWNCAST(AP_tree_root*, arb_tree_root)
#define AP_TREE_ROOT_CONST_CAST(arb_tree_root) DOWNCAST(const AP_tree_root*, arb_tree_root)

// ---------------------
//      AP_tree_root

class AP_tree;

typedef void (*AP_rootChangedCb)(void *cd, AP_tree *old, AP_tree *newroot);
typedef void (*AP_nodeDelCb)(void *cd, AP_tree *del);

class AP_tree_root : public ARB_tree_root {
    AP_rootChangedCb  root_changed_cb;
    void             *root_changed_cd;
    AP_nodeDelCb      node_deleted_cb;
    void             *node_deleted_cd;

    GBDATA *gb_species_data;                        // @@@ needed ?
    
public:
    GBDATA   *gb_tree_gone;                         // if all leaves have been removed by tree operations, remember 'ARB_tree_root::gb_tree' here (see change_root)
    GBDATA   *gb_table_data;
    long      tree_timer;
    long      species_timer;
    long      table_timer;
    AP_rates *rates;

    AP_tree_root(AliView *aliView, const AP_tree& tree_proto, AP_sequence *seq_proto, bool add_delete_callbacks);
    virtual ~AP_tree_root();

    // ARB_tree interface
    AP_tree *get_root_node() { return AP_TREE_CAST(ARB_tree_root::get_root_node()); }
    virtual void change_root(AP_tree *old, AP_tree *newroot);
    
    virtual GB_ERROR loadFromDB(const char *name);
    virtual GB_ERROR saveToDB();

    // AP_tree_root interface

    void update_timers();                           // update the timer
    bool is_tree_updated();
    bool is_species_updated();

    void inform_about_delete(AP_tree *old);

    void set_root_changed_callback(AP_rootChangedCb cb, void *cd);
    void set_node_deleted_callback(AP_nodeDelCb cb, void *cd);

    void remove_leafs(int awt_remove_type); 
    ARB_edge find_innermost_edge();
};


struct AP_tree_members {
public:
    // elements from struct a_tree_node

    // struct arb_flags
    unsigned int grouped:1;     // indicates a folded group
    unsigned int hidden:1;      // not shown because a father is a folded group
    unsigned int has_marked_children:1; // at least one child is marked
    unsigned int callback_exists:1;
    unsigned int gc:6;          // color

    char    left_linewidth;
    char    right_linewidth;
    // struct arb_data
    int leave_sum;  // number of leaf children of this node
    int view_sum;   // virtual size of node for display ( isgrouped?sqrt(leave_sum):leave_sum

    float   tree_depth; // max length of path; for drawing triangles */
    float   min_tree_depth; /* min length of path; for drawing triangle */
    float   spread;

    float   left_angle;
    float   right_angle;

    void clear() {
        grouped = 0;
        hidden = 0;
        has_marked_children = 0;
        callback_exists = 0;
        gc = 0;
        left_linewidth = 0;
        right_linewidth = 0;
        leave_sum = 0;
        view_sum = 0;
        tree_depth = 0;
        min_tree_depth = 0;
        spread = 0;
        left_angle = 0;
        right_angle = 0;
    }
};

struct AP_branch_members {
public:
    unsigned int kl_marked:1;       // kernighan lin marked
    unsigned int touched:1;         // nni and kl

    void clear() {
        kl_marked = 0;
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
    virtual ~AP_tree(); // leave this here to force creation of virtual table

    // ARB_tree interface
    virtual AP_tree *dup() const;
    // ARB_tree interface (end)

    AP_tree *get_father() { return AP_TREE_CAST(father); }
    const AP_tree *get_father() const { return AP_TREE_CONST_CAST(father); }
    AP_tree *get_leftson() { return AP_TREE_CAST(leftson); }
    const AP_tree *get_leftson() const { return AP_TREE_CONST_CAST(leftson); }
    AP_tree *get_rightson() { return AP_TREE_CAST(rightson); }
    const AP_tree *get_rightson() const { return AP_TREE_CONST_CAST(rightson); }
    AP_tree *get_brother() { return AP_TREE_CAST(ARB_tree::get_brother()); }
    const AP_tree *get_brother() const { return AP_TREE_CONST_CAST(ARB_tree::get_brother()); }

    AP_tree_root *get_tree_root() const {
        ARB_tree_root *base = ARB_tree::get_tree_root();
        return base ? AP_TREE_ROOT_CAST(base) : NULL;
    }
    AP_tree *get_root() { return AP_TREE_CAST(ARB_tree::get_root()); }

    int compute_tree(GBDATA *gb_main);

    int arb_tree_set_leafsum_viewsum();             // count all visible leafs -> gr.viewsum + gr.leafsum
    int arb_tree_leafsum2();                        // count all leafs

    void calc_hidden_flag(int father_is_hidden);
    virtual int calc_color();                       // start a transaction first

    virtual int calc_color_probes(GB_HASH *hashptr); //new function for coloring the tree; ak

    GBT_LEN arb_tree_min_deep();
    GBT_LEN arb_tree_deep();

    virtual void insert(AP_tree *new_brother);
    virtual void remove();                          // remove this+father (but do not delete)
    virtual void swap_assymetric(AP_TREE_SIDE mode); // 0 = AP_LEFT_son  1=AP_RIGHT_son
    void         swap_sons();                       // exchange sons

    GB_ERROR     cantMoveTo(AP_tree *new_brother);  // use this to detect impossible moves
    virtual void moveTo(AP_tree *new_brother,AP_FLOAT rel_pos); // move to new brother

    virtual void set_root();

    void remove_bootstrap();                        // remove bootstrap values from subtree
    void reset_branchlengths();                     // reset branchlengths of subtree to 0.1
    void scale_branchlengths(double factor);
    void bootstrap2branchlen();                     // copy bootstraps to branchlengths
    void branchlen2bootstrap();                     // copy branchlengths to bootstraps

    virtual void move_gbt_info(GBT_TREE *tree);  

    GB_ERROR tree_write_tree_rek(GBDATA *gb_tree);
    GB_ERROR relink() __ATTR__USERESULT; // @@@ used ? if yes -> move to AP_tree_root or ARB_tree_root  

    virtual AP_UPDATE_FLAGS check_update();

    virtual void update();

private:
    void buildLeafList_rek(AP_tree **list, long& num);
    void buildNodeList_rek(AP_tree **list, long& num);
    void buildBranchList_rek(AP_tree **list, long& num, bool create_terminal_branches, int deep);

public:
    void buildLeafList(AP_tree **&list, long &num); // returns a list of leafs
    void buildNodeList(AP_tree **&list, long &num); // returns a list of inner nodes (w/o root)
    void buildBranchList(AP_tree **&list, long &num, bool create_terminal_branches, int deep);

    AP_tree **getRandomNodes(int nnodes); // returns a list of random nodes (no leafs)

    void replace_self(AP_tree *new_son);
    void set_brother(AP_tree *new_son);

    virtual void clear_branch_flags();

    void touch_branch() {
        AP_tree *flagBranch    = father->father ? this : AP_TREE_CAST(father->leftson);
        flagBranch->br.touched = 1;
    }
    int get_branch_flag() const {
        const AP_tree *flagBranch = father->father ? this : AP_TREE_CONST_CAST(father->leftson);
        return flagBranch->br.touched;
    }

    GB_ERROR move_group_info(AP_tree *new_group) __ATTR__USERESULT;

    void mark_duplicates(GBDATA *gb_main);
    void mark_long_branches(GBDATA *gb_main, double min_rel_diff, double min_abs_diff);
    void mark_deep_branches(GBDATA *gb_main,int rel_depth);
    void mark_degenerated_branches(GBDATA *gb_main,double degeneration_factor);

    void justify_branch_lenghs(GBDATA *gb_main);
    void relink_tree(GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash), GB_HASH *organism_hash);

    void reset_spread();
    void reset_rotation();
    void reset_line_width();
    
};

#else
#error AP_Tree.hxx included twice
#endif // AP_TREE_HXX
