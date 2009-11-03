// =============================================================== //
//                                                                 //
//   File      : ap_main.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_MAIN_HXX
#define AP_MAIN_HXX

#ifndef AP_BUFFER_HXX
#include "AP_buffer.hxx"
#endif

#define AWAR_ALIGNMENT        "tmp/pars/alignment"
#define AWAR_FILTER_NAME      "tmp/pars/filter/name"
#define AWAR_FILTER_FILTER    "tmp/pars/filter/filter"
#define AWAR_FILTER_ALIGNMENT "tmp/pars/filter/alignment"
#define AWAR_PARSIMONY        "tmp/pars/parsimony"
#define AWAR_BEST_PARSIMONY   "tmp/pars/best_parsimony"
#define AWAR_STACKPOINTER     "tmp/pars/stackpointer"

#define NNI_MODES // uncomment to hide NNI/K.L. mode buttons

//#define AWAR_TREE AWAR_TREE

#define AWAR_PARS_TYPE      "pars/pars_type"

enum PARS_pars_type {
    PARS_WAGNER,
    PARS_TRANSVERSION
};

class AP_tree_nlen;
class AWT_graphic_tree;
class AP_main {
    AP_main_stack    *stack;
    AP_main_list      list;
    unsigned long     stack_level;
    AWT_graphic_tree *agt;                          // provides access to tree!

public:
    // *************** read only
    char          *use;
    unsigned long  user_push_counter;

    // ************** real public
    struct {
        unsigned int add_marked:1;
        unsigned int add_selected:1;
        unsigned int calc_branch_lengths:1;
        unsigned int calc_bootstrap:1;
        unsigned int quit:1;
    } commands;

    unsigned long combineCount;

    AP_main();
    ~AP_main();

    void set_tree_root(AWT_graphic_tree *agt_);
    AWT_graphic_tree *get_tree_root() { return agt; }
    AP_tree_nlen *get_root_node();

    GB_ERROR open(char *db_server);

    AP_BOOL buffer_cout;

    void user_push();
    void user_pop();
    void push();
    void pop();
    void push_node(AP_tree_nlen *node,AP_STACK_MODE);
    void clear();               // clears all buffers
};

extern AP_main *ap_main;
extern GBDATA  *GLOBAL_gb_main;

#else
#error ap_main.hxx included twice
#endif // AP_MAIN_HXX
