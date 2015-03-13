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
        // : AWT_graphic_parsimony(parsimony_, GLOBAL_gb_main, NULL),
        : AWT_graphic_parsimony(parsimony_, ap_main->get_gb_main(), NULL),
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
    long               prev_combine_count;

    void common_init(const char *dbname) {
        apMain.open(dbname);

        TEST_EXPECT_NULL(ap_main);
        ap_main = &apMain;

        agt = new fake_agt<SEQTYPE>(parsimony);
        parsimony.set_tree(agt);

        prev_combine_count = AP_sequence::combine_count();
    }

public:
    PARSIMONY_testenv(const char *dbname)
        : parsimony()
    {
        common_init(dbname);
        agt->init(new AliView(ap_main->get_gb_main()));
    }
    PARSIMONY_testenv(const char *dbname, const char *aliName);
    ~PARSIMONY_testenv() {
        if (root_node()) {
            AP_tree_edge::destroy(root_node());
        }

        TEST_EXPECT_EQUAL(ap_main, &apMain);
        ap_main = NULL;

        delete agt;

        ap_assert(prev_combine_count == AP_sequence::combine_count()); // please add tests documenting combines_performed()
    }

    AP_tree_nlen *root_node() { return apMain.get_root_node(); }
    AP_tree_root *tree_root() { return agt->get_tree_root(); }

    GB_ERROR load_tree(const char *tree_name) {
        GBDATA         *gb_main = ap_main->get_gb_main();
        GB_transaction  ta(gb_main);     // @@@ do inside AWT_graphic_tree::load?
        GB_ERROR        error   = agt->load(gb_main, tree_name, 0, 0);
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

    AWT_graphic_parsimony *graphic_tree() { return agt; }

    GBDATA *gbmain() const { return ap_main->get_gb_main(); }

    long combines_performed() {
        long performed     = AP_sequence::combine_count()-prev_combine_count;
        prev_combine_count = AP_sequence::combine_count();
        return performed;
    }
};


#else
#error test_env.h included twice
#endif // TEST_ENV_H

