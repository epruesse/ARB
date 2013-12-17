// =============================================================== //
//                                                                 //
//   File      : AP_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_error.hxx"
#include "ap_tree_nlen.hxx"

using namespace std;

// ---------------
//      AP_ERR

int AP_ERR::mode = 0;

AP_ERR::~AP_ERR()
{
    delete text;
}


AP_ERR::AP_ERR (const char *pntr)
// setzt den Fehlertext und zeigt ihn an
{
    text = pntr;
    if (mode == 0) {
        cout << "\n*** WARNING *** \n" << text << "\n";
        cout.flush();
    }
}

AP_ERR::AP_ERR (const char *pntr, const char *pntr2)
{
    text = pntr2;
    if (mode == 0) {
        cout << "\n***** WARNING  in " << pntr << "\n" << text << "\n";
        cout.flush();
    }
}

AP_ERR::AP_ERR (const char *pntr, const char *pntr2, const int core)
{
    text = pntr2;
    cout << "\n*** FATAL ERROR *** " << core << " in " << pntr << "\n" << text << "\n";
    cout.flush();
    GBK_terminate("AP_ERR[1]");
}

AP_ERR::AP_ERR (const char *pntr, const int core)
// setzt den Fehlertext
// bricht ab
{
    text = pntr;
    cout << "\n*** FATAL ERROR *** " << core << "\n" << text << "\n";
    cout.flush();
    GBK_terminate("AP_ERR[2]");
}

const char *AP_ERR::show()
{
    return text;
}

void AP_ERR::set_mode(int i) {
    mode = i;
}

// ----------------
//      AP_main

GB_ERROR AP_main::open(const char *db_server) {
    GB_ERROR error             = 0;
    GLOBAL_gb_main             = GB_open(db_server, "rwt");
    if (!GLOBAL_gb_main) error = GB_await_error();
    return error;
}

void AP_main::user_push() {
    this->user_push_counter = stack_level + 1;
    this->push();
}

void AP_main::user_pop() {
    // checks if user_pop possible
    if (user_push_counter == stack_level) {
        this->pop();    // changes user_push_counter if user pop
    }
    else {
        new AP_ERR("AP_main::user_pop()", "No user pop possible");
    }
    return;
}

void AP_main::push() {
    // if count > 1 the nodes are buffered more than once
    // WARNING:: node only has to be buffered once in the stack
    //
    //
    stack_level ++;
    if (stack) list.push(stack);
    stack = new AP_main_stack;
    stack->last_user_buffer = this->user_push_counter;
}

void AP_main::pop() {
    AP_tree_nlen *knoten;
    if (!stack) {
        new AP_ERR("AP_main::pop()", "Stack underflow !");
        return;
    }
    while ((knoten = stack->pop())) {
        if (stack_level != knoten->stack_level) {
            GB_internal_error("AP_main::pop: Error in stack_level");
            cout << "Main UPD - node UPD : " << stack_level << " -- " << knoten->stack_level << " \n";
            return;
        }
        knoten->pop(stack_level);
    }
    delete stack;
    stack_level --;
    stack = list.pop();
    user_push_counter = stack ? stack->last_user_buffer : 0;
}

void AP_main::clear() {
    // removes count elements from the list
    // because the current tree is used
    //
    // if stack_counter greater than last user_push then
    // moves all not previous buffered nodes in the
    // previous stack

    AP_tree_nlen  *knoten;
    AP_main_stack *new_stack;

    if (!stack) {
        new AP_ERR("AP_main::clear", "Stack underflow !");
        return;
    }
    if (user_push_counter >= stack_level) {
        if (stack != 0) {
            if (stack->size() > 0) {
                while (stack->size() > 0) {
                    knoten = stack->pop();
                    knoten->clear(stack_level, user_push_counter);
                }
            }
            delete stack;
            stack = list.pop();
        }
    }
    else {
        if (stack) {
            new_stack = list.pop();
            while ((knoten = stack->pop())) {
                if (knoten->clear(stack_level, user_push_counter) != true) {
                    // node is not cleared because buffered in previous node stack
                    // node is instead copied in previous level
                    if (new_stack) new_stack->push(knoten);
                }
            }
            delete stack;
            stack = new_stack;
        }
        else {
            new AP_ERR("AP_main::clear");
        }
    }
    stack_level --;
    if (stack) user_push_counter = stack->last_user_buffer;
    else user_push_counter = 0;

}

void AP_main::push_node(AP_tree_nlen *node, AP_STACK_MODE mode) {
    //
    //  stores node
    //
    if (!stack) {
        if (mode & SEQUENCE)    node->unhash_sequence();
        return;
    }

    if (stack_level < node->stack_level) {
        GB_warning("AP_main::push_node: stack_level < node->stack_level");
        return;
    }

    if (node->push(mode, stack_level))  stack->push(node);
}

void AP_main::set_tree_root(AWT_graphic_tree *agt_) {
    ap_assert(agt == 0 && agt_ != 0);               // do only once
    agt = agt_;
}

const char *AP_main::get_aliname() const {
    return agt->tree_static->get_aliview()->get_aliname();
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arb_diff.h>
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

// @@@ Tests wanted:
// - NNI (difficult - needs sequences)

void fake_AW_init_color_groups();

struct fake_agt : public AWT_graphic_tree {
    fake_agt() : AWT_graphic_tree(NULL, GLOBAL_gb_main, NULL) {}
    void init(AliView *aliview) {
        fake_AW_init_color_groups(); // acts like no species has a color
        AWT_graphic_tree::init(new AP_TreeNlenNodeFactory, aliview, NULL, true, false);
    }
};

struct PARSIMONY_testenv {
    GB_shell  shell;
    AP_main   apMain;
    fake_agt *agt;

    PARSIMONY_testenv(const char  *dbname) {
        GLOBAL_gb_main = NULL;
        apMain.open(dbname);

        agt = new fake_agt;
        apMain.set_tree_root(agt);
        agt->init(new AliView(GLOBAL_gb_main));
    }
    ~PARSIMONY_testenv() {
        delete agt;
        GB_close(GLOBAL_gb_main);
        GLOBAL_gb_main = NULL;
    }

    GB_ERROR load_tree(const char *tree_name) {
        GB_transaction ta(GLOBAL_gb_main); // @@@ do inside AWT_graphic_tree::load?
        return agt->load(GLOBAL_gb_main, tree_name, 0, 0);
    }
    AP_tree_nlen *tree_root() { return apMain.get_root_node(); }

    void push() { apMain.push(); }
    void pop() { apMain.pop(); }
};

static AP_tree_nlen *findNode(AP_tree_nlen *node, const char *name) {
    if (node->name && strcmp(node->name, name) == 0) return node;
    if (node->is_leaf) return NULL;

    AP_tree_nlen *found = findNode(node->get_leftson(), name);
    if (!found) found   = findNode(node->get_rightson(), name);
    return found;
}

void TEST_tree_modifications() {
    PARSIMONY_testenv env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    {
        AP_tree_nlen *root = env.tree_root();
        TEST_REJECT_NULL(root);

        AP_tree_edge::initialize(root);   // builds edges

        TEST_ASSERT_VALID_TREE(root);

        root->compute_tree();

        // first check initial state:
        {
            AP_tree_members& root_info = root->gr;

            TEST_EXPECT_EQUAL(root_info.grouped,             0);
            TEST_EXPECT_EQUAL(root_info.hidden,              0);
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 1);
            TEST_EXPECT_EQUAL(root_info.leaf_sum,            15);

            TEST_EXPECT_SIMILAR(root_info.max_tree_depth, 1.624975, 0.000001);
            TEST_EXPECT_SIMILAR(root_info.min_tree_depth, 0.341681, 0.000001);

            GB_transaction ta(GLOBAL_gb_main);
            GBT_mark_all(GLOBAL_gb_main, 0); // unmark all species
            root->compute_tree();
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 0);
        }


#define B1_TOP "(((((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274,CloInnoc:0.371):0.057,CloBifer:0.388):0.124"
#define B1_BOT "(CloBifer:0.388,(CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.057):0.124"
#define B2_TOP "(((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010,CloPaste:0.179):0.131"
#define B2_BOT "(CloPaste:0.179,(CloCarni:0.120,(CloButy2:0.009,CloButyr:0.000):0.564):0.010):0.131"


#define B3_LEFT_TOP_SONS "(((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053,CelBiazo:0.059)"
#define B3_TOP_SONS      B3_LEFT_TOP_SONS ":0.207,CytAquat:0.711"
#define B3_TOP_SONS_CCR  "((CorAquat:0.187,CorGluta:0.522):0.053,CelBiazo:0.059):0.207,CytAquat:0.711" // CCR = CurCitre removed
#define B3_TOP           "(" B3_TOP_SONS "):0.081"
#define B3_BOT           "(CytAquat:0.711,(CelBiazo:0.059,(CorGluta:0.522,(CorAquat:0.084,CurCitre:0.058):0.103):0.053):0.207):0.081"


        const char *top_topo    = "((" B1_TOP "," B2_TOP "):0.081," B3_TOP ");";
        const char *edge_topo   = "((" B1_TOP "," B2_BOT "):0.081," B3_BOT ");";
        const char *bottom_topo = "(" B3_BOT ",(" B2_BOT "," B1_BOT "):0.081);";

        LocallyModify<AP_main*> setGlobal(ap_main, &env.apMain);
        env.push(); // 1st stack level (=top_topo)

        TEST_ASSERT_VALID_TREE(root);

        TEST_EXPECT_NEWICK_LEN_EQUAL(root, top_topo);
        // test reorder_tree:
        root->reorder_tree(BIG_BRANCHES_TO_EDGE);   TEST_EXPECT_NEWICK_LEN_EQUAL(root, edge_topo); env.push();   // 2nd stack level (=edge_topo)
        root->reorder_tree(BIG_BRANCHES_TO_BOTTOM); TEST_EXPECT_NEWICK_LEN_EQUAL(root, bottom_topo); env.push(); // 3rd stack level (=bottom_topo)
        root->reorder_tree(BIG_BRANCHES_TO_TOP);    TEST_EXPECT_NEWICK_LEN_EQUAL(root, top_topo);

        TEST_ASSERT_VALID_TREE(root);

        // test set root:
        AP_tree_nlen *CloTyrob = findNode(root, "CloTyrob");
        TEST_REJECT_NULL(CloTyrob);

        ARB_edge rootEdge(root->get_leftson(), root->get_rightson());
        CloTyrob->set_root();

        TEST_ASSERT_VALID_TREE(root);

        const char *rootAtCloTyrob_topo =
            "(CloTyrob:0.004,"
            "(((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,"
            "((((" B3_TOP_SONS "):0.162," B2_TOP "):0.124,CloBifer:0.388):0.057,CloInnoc:0.371):0.274):0.004);";

        TEST_EXPECT_NEWICK_LEN_EQUAL(root, rootAtCloTyrob_topo);
        env.push(); // 4th stack level (=rootAtCloTyrob_topo)

        TEST_ASSERT_VALID_TREE(root);

        AP_tree_nlen *CelBiazoFather = findNode(root, "CelBiazo")->get_father();
        TEST_REJECT_NULL(CelBiazoFather);
        CelBiazoFather->set_root();

        const char *rootAtCelBiazoFather_topo = "(" B3_LEFT_TOP_SONS ":0.104,((" B1_TOP "," B2_TOP "):0.162,CytAquat:0.711):0.104);";
        TEST_EXPECT_NEWICK_LEN_EQUAL(root, rootAtCelBiazoFather_topo);

        TEST_ASSERT_VALID_TREE(root);

        ARB_edge oldRootEdge(rootEdge.source(), rootEdge.dest());
        DOWNCAST(AP_tree_nlen*,oldRootEdge.son())->set_root();

        const char *rootSetBack_topo = top_topo;
        TEST_EXPECT_NEWICK_LEN_EQUAL(root, rootSetBack_topo);
        env.push(); // 5th stack level (=rootSetBack_topo)

        TEST_ASSERT_VALID_TREE(root);

        // test remove:
        AP_tree_nlen *CurCitre = findNode(root, "CurCitre");
        TEST_REJECT_NULL(CurCitre);
        TEST_REJECT_NULL(CurCitre->get_father());

        CurCitre->remove();
        const char *CurCitre_removed_topo = "((" B1_TOP "," B2_TOP "):0.081,(" B3_TOP_SONS_CCR "):0.081);";
        // ------------------------------------------------------------------- ^^^ = B3_TOP_SONS minus CurCitre
        TEST_EXPECT_NEWICK_LEN_EQUAL(root, CurCitre_removed_topo);

        TEST_ASSERT_VALID_TREE(root);
        TEST_ASSERT_VALID_TREE(CurCitre);

        TEST_EXPECT_EQUAL(root->gr.leaf_sum, 15); // out of date
        root->compute_tree();
        TEST_EXPECT_EQUAL(root->gr.leaf_sum, 14);

        env.push(); // 6th stack level (=CurCitre_removed_topo)

        TEST_ASSERT_VALID_TREE(root);

        // test insert:
        AP_tree_nlen *CloCarni = findNode(root, "CloCarni");
        TEST_REJECT_NULL(CloCarni);
        CurCitre->insert(CloCarni); // this creates two extra edges (not destroyed by destroy() below) and one extra node

        const char *CurCitre_inserted_topo = "((" B1_TOP ",(((CloButy2:0.009,CloButyr:0.000):0.564,(CurCitre:0.060,CloCarni:0.060):0.060):0.010,CloPaste:0.179):0.131):0.081,(" B3_TOP_SONS_CCR "):0.081);";
        TEST_EXPECT_NEWICK_LEN_EQUAL(root, CurCitre_inserted_topo);

        AP_tree_nlen *node_del_manually  = CurCitre->get_father();
        AP_tree_edge *edge1_del_manually = CurCitre->edgeTo(node_del_manually);
        AP_tree_edge *edge2_del_manually = CurCitre->get_brother()->edgeTo(node_del_manually);

        TEST_ASSERT_VALID_TREE(root);

        // now check pops:
        env.pop(); TEST_EXPECT_NEWICK_LEN_EQUAL(root, CurCitre_removed_topo);
        env.pop(); TEST_EXPECT_NEWICK_LEN_EQUAL(root, rootSetBack_topo);
        env.pop(); TEST_EXPECT_NEWICK_LEN_EQUAL(root, rootAtCloTyrob_topo);
        env.pop(); TEST_EXPECT_NEWICK_LEN_EQUAL(root, bottom_topo);
        env.pop(); TEST_EXPECT_NEWICK_LEN_EQUAL(root, edge_topo);
        env.pop(); TEST_EXPECT_NEWICK_LEN_EQUAL(root, top_topo);

        TEST_ASSERT_VALID_TREE(root);

        AP_tree_edge::destroy(root);

        // delete memory allocated by insert() above and lost due to pop()s
        delete edge1_del_manually;
        delete edge2_del_manually;

        node_del_manually->forget_origin();
        node_del_manually->father   = NULL;
        node_del_manually->leftson  = NULL;
        node_del_manually->rightson = NULL;
        delete node_del_manually;
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
