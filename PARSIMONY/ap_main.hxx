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
#ifndef TREEDISPLAY_HXX
#include <TreeDisplay.hxx>
#endif

#define AWAR_ALIGNMENT        "tmp/pars/alignment"
#define AWAR_FILTER_NAME      "tmp/pars/filter/name"
#define AWAR_FILTER_FILTER    "tmp/pars/filter/filter"
#define AWAR_FILTER_ALIGNMENT "tmp/pars/filter/alignment"
#define AWAR_PARSIMONY        "tmp/pars/parsimony"
#define AWAR_BEST_PARSIMONY   "tmp/pars/best_parsimony"
#define AWAR_STACKPOINTER     "tmp/pars/stackpointer"

#define AWAR_PARS_TYPE      "pars/pars_type"

enum PARS_pars_type {
    PARS_WAGNER,
    PARS_TRANSVERSION
};

struct PARS_commands {
    bool add_marked;
    bool add_selected;
    bool calc_branch_lengths;
    bool calc_bootstrap;
    bool quit;

    PARS_commands()
        : add_marked(false)
        , add_selected(false)
        , calc_branch_lengths(false)
        , calc_bootstrap(false)
        , quit(false)
    {
    }
};

class AP_tree_nlen;
class AWT_graphic_tree;

class AP_main : virtual Noncopyable {
    AP_main_stack    *stack;
    AP_main_list      list;
    unsigned long     stack_level;
    AWT_graphic_tree *agt;                          // provides access to tree!
    unsigned long     user_push_counter;

public:
    AP_main()
        : stack(NULL),
          stack_level(0),
          agt(NULL),
          user_push_counter(0)
    {}
    ~AP_main() {
        delete stack;
    }

    void set_tree_root(AWT_graphic_tree *agt_);
    AWT_graphic_tree *get_tree_root() { return agt; }

    DEFINE_DOWNCAST_ACCESSORS(AP_tree_nlen, get_root_node, agt->get_root_node());

    const char *get_aliname() const;
    unsigned long get_user_push_counter() const { return user_push_counter; }

    GB_ERROR open(const char *db_server);

    void user_push();
    void user_pop();
    void push();
    void pop();
    void push_node(AP_tree_nlen *node, AP_STACK_MODE);
    void clear();               // clears all buffers
};

extern AP_main *ap_main;
extern GBDATA  *GLOBAL_gb_main;

#else
#error ap_main.hxx included twice
#endif // AP_MAIN_HXX
