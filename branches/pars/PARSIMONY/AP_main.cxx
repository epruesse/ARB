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
#include "ap_main.hxx"

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
    currframe.user_push_counter = stack_level + 1;
    push();
}

void AP_main::user_pop() {
    // checks if user_pop possible
    if (currframe.user_push_counter == stack_level) {
        pop();    // changes user_push_counter if user pop
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

    stack = new AP_main_stack(currframe);

#if defined(CHECK_ROOT_POPS)
    stack->root_at_create = DOWNCAST(AP_tree_nlen*, get_tree_root()->get_root_node());
#endif
}

void AP_main::pop() {
    if (!stack) {
        new AP_ERR("AP_main::pop()", "Stack underflow !");
        return;
    }
    {
        AP_tree_nlen *knoten;
        while ((knoten = stack->pop())) {
            if (stack_level != knoten->stack_level) {
                GB_internal_error("AP_main::pop: Error in stack_level");
                cout << "Main UPD - node UPD : " << stack_level << " -- " << knoten->stack_level << " \n";
                return;
            }
            knoten->pop(stack_level);
        }
    }

#if defined(CHECK_ROOT_POPS)
    ap_assert(stack->root_at_create == get_tree_root()->get_root_node()); // root has been restored!
#endif

    delete stack;
    stack_level --;

    stack     = list.pop();
    currframe = stack ? stack->get_previous_frame_data() : StackFrameData();
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
    if (currframe.user_push_counter >= stack_level) {
        if (stack != 0) {
            if (stack->size() > 0) {
                while (stack->size() > 0) {
                    knoten = stack->pop();
                    knoten->clear(stack_level, currframe.user_push_counter);
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
                if (knoten->clear(stack_level, currframe.user_push_counter) != true) {
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

    currframe = stack ? stack->get_previous_frame_data() : StackFrameData();
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

    if (mode == ROOT) {
        // test that it is really root what is pushed
        ap_assert(!node->father);
        ap_assert(node->is_root_node());

#if defined(AVOID_MULTI_ROOT_PUSH)
        if (currframe.root_pushed) {
            // do not push root twice inside same stack_level
            mode = BOTH;
        }
        else {
            currframe.root_pushed = true;
#if defined(CHECK_ROOT_POPS)
            ap_assert(node == stack->root_at_create); // make sure the pushed root is the correct one
#endif
        }
#endif
    }

    if (node->push(mode, stack_level)) stack->push(node);
    if (mode == ROOT) {
        // In AP_main::pop(), root-node has to be restored after everything else has been restored.
        // Move node to bottom of stack now to ensure that.
        ASSERT_RESULT(bool, true, stack->remove(node));
        stack->shift(node);
    }
}

void AP_main::set_tree_root(AWT_graphic_parsimony *agt_) {
    ap_assert(agt == 0 && agt_ != 0);               // do only once
    agt = agt_;
}

const char *AP_main::get_aliname() const {
    return get_tree_root()->get_aliview()->get_aliname();
}

