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

class UserFrame {
    Level frameLevel;
    // @@@ store more information here (e.g. comment, parsimony value, timestamp)
public:
    UserFrame(Level level) : frameLevel(level) {}
    Level get_level() const { return frameLevel; }
};

typedef AP_STACK<UserFrame> UserFrameStack;

class AP_main : virtual Noncopyable {
    NodeStack             *currFrame;
    FrameStack             frames;
    UserFrameStack         user_frames;
    Level                  frameLevel;
    AWT_graphic_parsimony *agt;       // provides access to tree!
    StackFrameData        *frameData; // saved/restored by remember/revert
    GBDATA                *gb_main;

    void push_nodes_changed_since(Level wanted_frameLevel);
    void rollback_to(Level wanted_frameLevel);

public:
    AP_main()
        : currFrame(NULL),
          frameLevel(0),
          agt(NULL),
          frameData(NULL),
          gb_main(NULL)
    {}
    ~AP_main() {
        if (gb_main) GB_close(gb_main);
        ap_assert(!frameData);
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
    size_t get_user_push_counter() const { return user_frames.count_elements(); }
    Level get_frameLevel() const { return frameLevel; }

    GB_ERROR open(const char *db_server);

    bool push_node(AP_tree_nlen *node, AP_STACK_MODE);

    void remember_user_state();
    void revert_user_state();
    // @@@ add accept_user_state (#632)

    void remember();
    void revert();
    void accept();
    void accept_all() { while (currFrame) accept(); }

#if defined(UNIT_TESTS)
    void remember_whole_tree(); // dont use in production
#endif

    void accept_if(bool cond) { if (cond) accept(); else revert(); }
    void revert_if(bool cond) { accept_if(!cond); }

    bool remember_and_rollback_to(Level wanted_frameLevel) {
        if (wanted_frameLevel>=frameLevel) {
            return false;
        }
        remember();
        push_nodes_changed_since(wanted_frameLevel);
        rollback_to(wanted_frameLevel);
        return true;
    }
    bool remember_and_rollback_to_previous() {
        /*! tree is modified into the same (or equiv) state as it would be done by calling pop(),
         * but the current state is pushed onto the stack.
         * Calling pop() afterwards will undo this operation.
         */
        return remember_and_rollback_to(frameLevel-1);
    }

#if defined(PROVIDE_PRINT)
    void print(std::ostream& out);
    void print2file(const char *file_in_ARBHOME);
    void dump2file(const char *name) {
        static int counter = 0;

        if (counter == 0) {
            system("rm $ARBHOME/[0-9][0-9]_*.log");
        }

        char *numbered_name = GBS_global_string_copy("%02i_%s.log", ++counter, name);
        print2file(numbered_name);
        free(numbered_name);
    }

#endif

#if defined(ASSERTION_USED) || defined(UNIT_TESTS)
    Validity revert_will_produce_valid_tree() {
        ASSERT_VALID_TREE(get_root_node());
        ASSERT_RESULT(bool, true, remember_and_rollback_to_previous()); // otherwise stack is empty
        Validity valid(tree_is_valid(get_root_node(), false));
        revert();                                                       // undo remember_and_rollback_to_previous
        ASSERT_VALID_TREE(get_root_node());
        return valid;
    }
    Validity all_available_reverts_will_produce_valid_trees() {
        ASSERT_VALID_TREE(get_root_node());
        Level pops_avail = get_frameLevel();
        Validity valid;
        for (Level rollback = 1; rollback<=pops_avail && valid; ++rollback) {
            Level wanted_level = pops_avail-rollback;
            ASSERT_RESULT(bool, true, remember_and_rollback_to(wanted_level)); // otherwise stack is empty

            // dump2file(GBS_global_string("after_remember_and_rollback_to_%lu", wanted_level));
            valid = Validity(tree_is_valid(get_root_node(), false));

            revert(); // undo remember_and_rollback_to
            // dump2file("after_pop_to_undo__remember_and_rollback_to");

            ASSERT_VALID_TREE(get_root_node());
        }
        return valid;
    }
#endif

    inline AP_tree_nlen *makeNode(AP_pars_root *proot);
    inline AP_tree_edge *makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2);
    inline void destroyNode(AP_tree_nlen *node);
    inline void destroyEdge(AP_tree_edge *edge);
};

#else
#error ap_main_type.hxx included twice
#endif // AP_MAIN_TYPE_HXX

