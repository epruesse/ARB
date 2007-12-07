#ifndef awt_tree_hxx_included
#define awt_tree_hxx_included

#define AP_F_LOADED ((AW_active)1)
#define AP_F_NLOADED ((AW_active)2)
#define AP_F_SEQUENCES ((AW_active)4)
#define AP_F_MATRIX ((AW_active)8)
#define AP_F_TREE ((AW_active)16)
#define AP_F_ALL ((AW_active)-1)

#define GROUPED_SUM 2   // min. no of species in a group which should be drawn as box

class AW_window;
class AW_root;

#include <awt_pro_a_nucs.hxx>
#include <aw_color_groups.hxx>

typedef unsigned char uchar;
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
};                              // AW_gc

typedef enum {
    NOTHING=0,  // nothing to buffer in AP_tree node
    STRUCTURE=1,    // only structure
    SEQUENCE=2, // only sequence
    BOTH=3,     // sequence & treestructure is buffered
    ROOT=7  // old root is buffered
} AP_STACK_MODE;

typedef enum {
    AP_FALSE,
    AP_TRUE
} AP_BOOL;

typedef enum {
    AP_UPDATE_OK = 0,
    AP_UPDATE_RELINKED = -1,
    AP_UPDATE_RELOADED = 1,
    AP_UPDATE_ERROR = 2
} AP_UPDATE_FLAGS;

typedef enum {  // flags zum kennzeichnen von knoten
    AP_LEFT,
    AP_RIGHT,
    AP_FATHER,
    AP_LEFTSON,
    AP_RIGHTSON,
    AP_NONE
} AP_TREE_SIDE;

enum {          // Flags can be ored !!!
    AWT_REMOVE_MARKED = 1,
    AWT_REMOVE_NOT_MARKED = 2,
    AWT_REMOVE_DELETED = 4,
    AWT_REMOVE_NO_SEQUENCE = 8,
    AWT_REMOVE_BUT_DONT_FREE = 16
};  //  AWT_REMOVE_TYPE;

typedef double AP_FLOAT;

enum AWT_FILTER_SIMPLIFY {
    AWT_FILTER_SIMPLIFY_NONE,
    AWT_FILTER_SIMPLIFY_DNA,
    AWT_FILTER_SIMPLIFY_PROTEIN
};

class AP_filter {
public:
    char    *filter_mask;   // 0 1
    long    filter_len;
    long    real_len;   // how many 1
    long    update;
    uchar   simplify[256];
    int *filterpos_2_seqpos;
    int *bootstrap; // if set then sizeof(bootstrap) == real_len; bootstrap[i] points to random original positions [0..filter_len]

    GB_ERROR init(const char *filter,const char *zerobases, long size);
    GB_ERROR init(long size);
    void    calc_filter_2_seq();
    void    enable_bootstrap(int random_seed);
    void    enable_simplify(AWT_FILTER_SIMPLIFY type);
    char    *to_string();   // convert to 0/1 string
    AP_filter(void);
    ~AP_filter(void);
};



class AP_weights {
protected:
    friend class AP_sequence;
    friend class AP_sequence_parsimony;
    friend class AP_sequence_protein;
    friend class AP_sequence_protein_old;
    GB_UINT4 *weights;
public:
    long weight_len;
    AP_filter *filter;
    long    update;
    GB_BOOL dummy_weights; // if true all weights are == 1
    AP_weights(void);
    char *init(AP_filter *fil); // init weights
    char *init(GB_UINT4 *w, AP_filter *fil);
    ~AP_weights(void);
};

class AP_rates {
public:
    AP_FLOAT *rates;
    long    rate_len;
    AP_filter *filter;
    long    update;

    AP_rates(void);
    char *init(AP_filter *fil);
    char *init(AP_FLOAT * ra, AP_filter *fil);
    ~AP_rates(void);
    void print(void);
};

class AP_smatrix {      // Symmetrical Matrix (upper triangular matrix)
public:
    AP_FLOAT **m;       // m[i][j]  i<= j !!!!
    long    size;
    AP_smatrix(long si);
    ~AP_smatrix(void);
    void    set(long i, long j, AP_FLOAT val) { if (i>j) m[i][j] = val; else m[j][i] = val; };
    AP_FLOAT get(long i, long j) { if (i>j) return m[i][j]; else return m[j][i]; };
};

class AP_matrix {       // Matrix
public:
    AP_FLOAT **m;
    char    **x_description; // optional discription, strdupped
    char    **y_description;
    long    size;
    AP_matrix(long si);
    ~AP_matrix(void);
    void create_awars(AW_root *awr,const char *awar_prefix);
    void read_awars(AW_root *awr,const char *awar_prefix);
    void normize();     // set average non diag element to 1.0 (only for descripted elements
    void create_input_fields(AW_window *aww,const char *awar_prefix);
    void set_description(const char *xstring,const char *ystring);
    void    set(int i, int j, AP_FLOAT val) { m[i][j] = val;};
    AP_FLOAT get(int i, int j) { return m[i][j];};
};

class AP_tree_root;

class AP_sequence {
protected:
    AP_FLOAT cashed_real_len;

public:
    AP_tree_root *root;
    static char *mutation_per_site; // if != 0 then mutations are set by combine
    static char *static_mutation_per_site[3];   // if != 0 then mutations are set by combine

    AP_BOOL is_set_flag;
    long    sequence_len;
    long    update;
    AP_FLOAT    costs;

    AP_sequence(AP_tree_root *rooti);
    virtual ~AP_sequence(void);

    virtual AP_sequence *dup(void) = 0;             // used to get the real new element
    virtual void set_gb(GBDATA *gb_sequence ); // by default calls set((char *))
    virtual void set(   char *sequence )  = 0;
    /* seq = acgtututu   */
    virtual AP_FLOAT combine(const AP_sequence* lefts, const AP_sequence *rights) = 0;
    virtual void partial_match(const AP_sequence* part, long *overlap, long *penalty) const = 0;
    virtual AP_FLOAT real_len(void);
};


class AP_tree;

class AP_tree_root {
public:
    GBDATA  *gb_main;
    GBDATA  *gb_tree;
    GBDATA *gb_species_data;
    GBDATA *gb_table_data;
    long    tree_timer;
    long    species_timer;
    long    table_timer;
    char    *tree_name;
    AP_tree *tree_template;
    AP_sequence     *sequence_template;

    AP_filter *filter;
    AP_weights *weights;
    AP_rates  *rates;
    AP_smatrix *matrix;

    AP_tree_root(GBDATA *gb_main, AP_tree *tree_proto,const char *name);
    void update_timers(void);       // update the timer
    GB_BOOL is_tree_updated(void);
    GB_BOOL is_species_updated(void);


    char        *(*root_changed)(void *cd,  AP_tree *old,  AP_tree *newroot);
    void        *root_changed_cd;
    char        *(*node_deleted)(void *cd,  AP_tree *old);
    void        *node_deleted_cd;

    AP_tree *tree;
    char        *inform_about_changed_root( AP_tree *old,  AP_tree *newroot);

    char        *inform_about_delete( AP_tree *old);

    ~AP_tree_root();
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



class AP_tree {
public:
    virtual     ~AP_tree(void); // leave this here to force creation of virtual table
private:
    void    _load_sequences_rek(char *use,GB_BOOL set_by_gbdata, long max, long *counter) ; // uses seq->filter
protected:

public:
    GBT_TREE_ELEMENTS( AP_tree);
    AP_tree_members gr;
    AP_branch_members   br;

    AP_FLOAT    mutation_rate;
    unsigned long stack_level;
    AP_tree_root    *tree_root;
    AP_sequence     *sequence;

    AP_tree(void);

    GBT_TREE *get_gbt_tree() { return (GBT_TREE*)this; }

    int     compute_tree(GBDATA *gb_main);

    int arb_tree_set_leafsum_viewsum(); // count all visible leafs -> gr.viewsum + gr.leafsum
    int arb_tree_leafsum2();// count all leafs

    void calc_hidden_flag(int father_is_hidden);
    virtual int calc_color();// start a transaction first

    virtual int calc_color_probes(GB_HASH *hashptr);        //new function for coloring the tree; ak

    GBT_LEN arb_tree_min_deep();
    GBT_LEN arb_tree_deep();

    void    load_sequences_rek(char *use,GB_BOOL set_by_gbdata,GB_BOOL show_status) ;   // uses seq->filter
    virtual void    parsimony_rek();
    AP_tree(AP_tree_root    *tree_root);
    virtual     AP_tree *dup(void);

    virtual GB_ERROR insert(AP_tree *new_brother);
    virtual GB_ERROR remove(void); // no delete of father
    virtual GB_ERROR swap_assymetric(AP_TREE_SIDE modus); // 0 = AP_LEFT_son  1=AP_RIGHT_son
    void             swap_sons(void); // exchange sons
    virtual GB_ERROR move(AP_tree *new_brother,AP_FLOAT rel_pos); // move to new brother
    virtual GB_ERROR set_root(void);
    virtual void     delete_tree(void);
    virtual GB_ERROR remove_leafs(GBDATA *gb_main,int awt_remove_type);
    
    void remove_bootstrap(GBDATA *); // remove bootstrap values from subtree
    void reset_branchlengths(GBDATA *); // reset branchlengths of subtree to 0.1
    void scale_branchlengths(GBDATA *gb_main, double factor); 
    void bootstrap2branchlen(GBDATA *); // copy bootstraps to branchlengths
    void branchlen2bootstrap(GBDATA *); // copy branchlengths to bootstraps

    virtual void test_tree(void) const;

    virtual AP_FLOAT costs(void);       /* cost of a tree (number of changes ..)*/

    virtual AP_BOOL     push(AP_STACK_MODE, unsigned long);
    virtual void        pop(unsigned long);
    virtual AP_BOOL     clear(  unsigned long stack_update, unsigned long user_push_counter);
    virtual void        unhash_sequence(void);

    void move_gbt_2_ap(GBT_TREE *tree, GB_BOOL insert_remove_cb);   // moves all node/leaf information from struct GBT_TREE to AP_tree
    void load_node_info();  // load linewidth etc

    GB_ERROR load(AP_tree_root *tree_static, int link_to_database,
                  GB_BOOL insert_delete_cbs, GB_BOOL show_status,
                  int *zombies, int *duplicates);

    virtual GB_ERROR save(char *tree_name);
    GB_ERROR relink( );

    virtual AP_UPDATE_FLAGS check_update();

    virtual void    update();


    GB_ERROR buildLeafList(AP_tree **&list, long &num); // returns a list of leafs
    GB_ERROR buildNodeList(AP_tree **&list, long &num); // returns a list of leafs
    GB_ERROR buildBranchList(AP_tree **&list, long &num,    AP_BOOL create_terminal_branches,int deep);
    // returns a pairs o leafs/father,
    //          node/father

    AP_tree **getRandomNodes(int nnodes);
    // returns a list of random nodes (no leafs)

    AP_BOOL is_son(AP_tree *father);
    AP_tree *brother(void) const;
    void set_fatherson(AP_tree *new_son);
    void set_fathernotson(AP_tree *new_son);

    virtual void clear_branch_flags(void);
    void touch_branch(void) { if (!father->father) father->leftson->br.touched =1 ; else this->br.touched=1; };
    int get_branch_flag(void) { return (!father->father) ? father->leftson->br.touched : this->br.touched; } ;

    GBT_LEN get_branchlength() const {
        if (father) {
            if (father->rightson == this) return father->rightlen;
            return father->leftlen;
        }
        return 0;
    }
    void set_branchlength(GBT_LEN newlen) {
        if (father) {
            if (father->rightson == this)
                father->rightlen = newlen;
            else
                father->leftlen  = newlen;
        }
    }

    GB_ERROR move_group_info(AP_tree *new_group);
    void     mark_duplicates(GBDATA *gb_main);
    void     mark_long_branches(GBDATA *gb_main,double diff);
    void     justify_branch_lenghs(GBDATA *gb_main);
    void     relink_tree(GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash), GB_HASH *organism_hash);
};

long AP_timer(void);


#endif
