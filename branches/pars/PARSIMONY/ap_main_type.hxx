// ================================================================ //
//                                                                  //
//   File      : ap_main_type.hxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef AP_MAIN_TYPE_HXX
#define AP_MAIN_TYPE_HXX

#ifndef AP_BUFFER_HXX
#include "AP_buffer.hxx"
#endif
#ifndef PARS_DTREE_HXX
#include "pars_dtree.hxx"
#endif

class AP_main : virtual Noncopyable {
    NodeStack             *currFrame;
    FrameStack             frames;
    Level                  frameLevel;
    AWT_graphic_parsimony *agt;       // provides access to tree!
    StackFrameData        *frameData; // saved/restored by push/pop
    GBDATA                *gb_main;

public:
    AP_main()
        : currFrame(NULL),
          frameLevel(0),
          agt(NULL),
          frameData(new StackFrameData(0)),
          gb_main(NULL)
    {}
    ~AP_main() {
        if (gb_main) GB_close(gb_main);
        ap_assert(frameData);
        delete frameData;
        delete currFrame;
    }

    void set_tree_root(AWT_graphic_parsimony *agt_);
    AWT_graphic_parsimony *get_graphic_tree() { return agt; }
    AP_pars_root *get_tree_root() const { return agt->get_tree_root(); }

    DEFINE_READ_ACCESSORS(AP_tree_nlen*, get_root_node, agt->get_root_node());

    GBDATA *get_gb_main() const {
        ap_assert(gb_main); // you need to call open() before you can use get_gb_main()
        return gb_main;
    }
    const char *get_aliname() const;
    Level get_user_push_counter() const { return frameData->user_push_counter; }
    Level get_frameLevel() const { return frameLevel; }

    GB_ERROR open(const char *db_server);

    void push_node(AP_tree_nlen *node, AP_STACK_MODE);

    void user_push(); // @@@ -> user_remember
    void user_pop();  // @@@ -> user_revert
    // @@@ add user_accept

    void remember();
    void revert();
    void accept();

    void accept_if(bool cond) { if (cond) accept(); else revert(); }
    void revert_if(bool cond) { accept_if(!cond); }

#if defined(PROVIDE_PRINT)
    void print(std::ostream& out);
    void print2file(const char *file_in_ARBHOME);
#endif

    inline AP_tree_nlen *makeNode(AP_pars_root *proot);
    inline AP_tree_edge *makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2);
    inline void destroyNode(AP_tree_nlen *node);
    inline void destroyEdge(AP_tree_edge *edge);
};

#else
#error ap_main_type.hxx included twice
#endif // AP_MAIN_TYPE_HXX

