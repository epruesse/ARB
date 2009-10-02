#define AWAR_ALIGNMENT      "tmp/pars/alignment"
#define AWAR_FILTER_NAME    "tmp/pars/filter/name"
#define AWAR_FILTER_FILTER  "tmp/pars/filter/filter"
#define AWAR_FILTER_ALIGNMENT   "tmp/pars/filter/alignment"
#define AWAR_PARSIMONY      "tmp/pars/parsimony"
#define AWAR_BEST_PARSIMONY "tmp/pars/best_parsimony"
#define AWAR_STACKPOINTER   "tmp/pars/stackpointer"

#define NNI_MODES // uncomment to hide NNI/K.L. mode buttons

//#define AWAR_TREE AWAR_TREE

#define AWAR_PARS_TYPE      "pars/pars_type"

enum PARS_pars_type {
    PARS_WAGNER,
    PARS_TRANSVERSION
};

extern class AP_main {
private:
    class AP_main_stack *stack;
    class AP_main_list  list;
    unsigned long       stack_level;

public:
    // *************** read only
    char    *use;
    unsigned long       user_push_counter;
    // ************** real public
    struct {
        unsigned  int add_marked:1;
        unsigned  int add_selected:1;
        unsigned  int calc_branch_lengths:1;
        unsigned  int calc_bootstrap:1;
        unsigned  int quit:1;
    } commands;

    unsigned long       combineCount;
    AP_main(void);
    ~AP_main(void);
    class AP_tree **tree_root;
    GB_ERROR open(char *db_server);
    // char *test(char *ratename, char *treename);
    void set_tree_root(AP_tree *new_root);

    AP_BOOL buffer_cout;

    void user_push(void);
    void user_pop(void);
    void push(void);
    void pop(void);
    void push_node(AP_tree * node,AP_STACK_MODE);
    void clear(void);               // clears all buffers



} *ap_main;

extern GBDATA *GLOBAL_gb_main;
