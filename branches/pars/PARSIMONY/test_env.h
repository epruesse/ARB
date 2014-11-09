// ================================================================= //
//                                                                   //
//   File      : test_env.h                                          //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2014   //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef TEST_ENV_H
#define TEST_ENV_H

void fake_AW_init_color_groups();

template<typename SEQTYPE>
struct fake_agt : public AWT_graphic_parsimony, virtual Noncopyable {
    SEQTYPE *templ;

    fake_agt(ArbParsimony& parsimony_)
        : AWT_graphic_parsimony(parsimony_, GLOBAL_gb_main, NULL),
          templ(NULL)
    {
    }
    ~fake_agt() {
        delete templ;
    }
    void init(AliView *aliview) {
        fake_AW_init_color_groups(); // acts like no species has a color
        delete templ;
        templ = aliview->has_data() ? new SEQTYPE(aliview) : NULL;
        AWT_graphic_tree::init(new AP_TreeNlenNodeFactory, aliview, templ, true, false);
    }
};


template<typename SEQTYPE>
class PARSIMONY_testenv : virtual Noncopyable {
    GB_shell           shell;
    AP_main            apMain;
    fake_agt<SEQTYPE> *agt;
    ArbParsimony       parsimony;

    void common_init(const char *dbname) {
        GLOBAL_gb_main = NULL;
        apMain.open(dbname);

        TEST_EXPECT_NULL(ap_main);
        ap_main = &apMain;

        agt = new fake_agt<SEQTYPE>(parsimony);
        apMain.set_tree_root(agt);
    }

public:
    PARSIMONY_testenv(const char *dbname)
        : parsimony(NULL)
    {
        common_init(dbname);
        agt->init(new AliView(GLOBAL_gb_main));
    }
    PARSIMONY_testenv(const char *dbname, const char *aliName);
    ~PARSIMONY_testenv() {
        if (root_node()) {
            AP_tree_edge::destroy(root_node());
        }

        TEST_EXPECT_EQUAL(ap_main, &apMain);
        ap_main = NULL;

        delete agt;
        GB_close(GLOBAL_gb_main);
        GLOBAL_gb_main = NULL;
    }

    AP_tree_nlen *root_node() { return apMain.get_root_node(); }
    AP_tree_root *tree_root() { return agt->get_tree_root(); }

    GB_ERROR load_tree(const char *tree_name) {
        GB_transaction ta(GLOBAL_gb_main);      // @@@ do inside AWT_graphic_tree::load?
        GB_ERROR       error = agt->load(GLOBAL_gb_main, tree_name, 0, 0);
        if (!error) {
            AP_tree_edge::initialize(rootNode());   // builds edges

            ap_assert(root_node());
            ap_assert(root_node() == rootNode()); // need tree-access via global 'ap_main' (too much code is based on that)
            ap_assert(rootEdge());

            TEST_ASSERT_VALID_TREE(root_node());
        }
        return error;
    }

    void push() { apMain.push(); }
    void pop() { apMain.pop(); }

    unsigned long get_stack_level() { return apMain.get_stack_level(); }
    unsigned long get_user_push_counter() { return apMain.get_user_push_counter(); }

    AWT_graphic_parsimony *graphic_tree() { return agt; }

    GBDATA *gbmain() const { return GLOBAL_gb_main; }
};


#else
#error test_env.h included twice
#endif // TEST_ENV_H

