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
#ifndef PARS_DTREE_HXX
#include "pars_dtree.hxx"
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

class AP_main : virtual Noncopyable {
    NodeStack             *currFrame;
    FrameStack             frames;
    unsigned long          frameLevel;
    AWT_graphic_parsimony *agt;       // provides access to tree!
    StackFrameData         frameData; // saved/restored by push/pop

public:
    AP_main()
        : currFrame(NULL),
          frameLevel(0),
          agt(NULL)
    {}
    ~AP_main() {
        delete currFrame;
    }

    void set_tree_root(AWT_graphic_parsimony *agt_);
    AWT_graphic_parsimony *get_graphic_tree() { return agt; }
    AP_tree_root *get_tree_root() const { return agt->get_tree_root(); }

    DEFINE_DOWNCAST_ACCESSORS(AP_tree_nlen, get_root_node, agt->get_root_node());

    const char *get_aliname() const;
    unsigned long get_user_push_counter() const { return frameData.user_push_counter; }
    unsigned long get_frameLevel() const { return frameLevel; }

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

inline AP_tree_nlen *rootNode() {
    return ap_main->get_root_node();
}

inline AP_tree_edge *rootEdge() {
    return rootNode()->get_leftson()->edgeTo(rootNode()->get_rightson());
}

#else
#error ap_main.hxx included twice
#endif // AP_MAIN_HXX
