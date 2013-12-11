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

// @@@ tests wanted:
// - set root to different edge + set root back
// - remove node
// - insert node
// - NNI
// - tree stack

struct fake_agt : public AWT_graphic_tree {
    fake_agt() : AWT_graphic_tree(NULL, GLOBAL_gb_main, NULL) {}
    void init(const AP_tree_nlen& proto, AliView *aliview) {
        AWT_graphic_tree::init(proto, aliview, NULL, false, false);
    }
};

struct PARSIMONY_testenv {
    GB_shell shell;
    AP_main  apMain;
    fake_agt agt;
    AP_tree_nlen proto;

    PARSIMONY_testenv(const char  *dbname)
        : proto(NULL)
    {
        GLOBAL_gb_main = NULL;
        apMain.open(dbname);
        apMain.set_tree_root(&agt);
        agt.init(proto, new AliView(GLOBAL_gb_main));
    }
    ~PARSIMONY_testenv() {
        GB_close(GLOBAL_gb_main);
        GLOBAL_gb_main = NULL;
    }

    GB_ERROR load_tree(const char *tree_name) { return agt.load(GLOBAL_gb_main, tree_name, 0, 0); }
    AP_tree_nlen *tree_root() { return apMain.get_root_node(); }
};

void TEST_tree_modifications() {
    PARSIMONY_testenv env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    {
        AP_tree_nlen *root = env.tree_root();
        TEST_REJECT_NULL(root);

        root->compute_tree(GLOBAL_gb_main);

        // first check initial state:
        {
            AP_tree_members& root_info = root->gr;

            TEST_EXPECT_EQUAL(root_info.grouped,             0);
            TEST_EXPECT_EQUAL(root_info.hidden,              0);
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 1);
            TEST_EXPECT_EQUAL(root_info.leaf_sum,            15);

            TEST_EXPECT_SIMILAR(root_info.tree_depth,     1.624975, 0.000001);
            TEST_EXPECT_SIMILAR(root_info.min_tree_depth, 0.341681, 0.000001);
        }

        const char *initial_topology = "(((((((CloTyro3,CloTyro4),CloTyro2),CloTyrob),CloInnoc),CloBifer),(((CloButy2,CloButyr),CloCarni),CloPaste)),((((CorAquat,CurCitre),CorGluta),CelBiazo),CytAquat));";
        TEST_EXPECT_NEWICK_EQUAL(root->get_gbt_tree(), initial_topology);

        root->reorder_tree(BIG_BRANCHES_TO_BOTTOM);
        TEST_EXPECT_NEWICK_EQUAL(root->get_gbt_tree(), "((CytAquat,(CelBiazo,(CorGluta,(CorAquat,CurCitre)))),((CloPaste,(CloCarni,(CloButy2,CloButyr))),(CloBifer,(CloInnoc,(CloTyrob,(CloTyro2,(CloTyro3,CloTyro4)))))));");

        root->reorder_tree(BIG_BRANCHES_TO_CENTER);
        TEST_EXPECT_NEWICK_EQUAL(root->get_gbt_tree(), "(((((((CloTyro3,CloTyro4),CloTyro2),CloTyrob),CloInnoc),CloBifer),(CloPaste,(CloCarni,(CloButy2,CloButyr)))),(CytAquat,(CelBiazo,(CorGluta,(CorAquat,CurCitre)))));");

        root->reorder_tree(BIG_BRANCHES_TO_TOP);
        TEST_EXPECT_NEWICK_EQUAL(root->get_gbt_tree(), initial_topology);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
