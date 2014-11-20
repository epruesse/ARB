// =============================================================== //
//                                                                 //
//   File      : AP_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ap_tree_nlen.hxx"
#include "ap_main.hxx"
#include <aw_msg.hxx>

using namespace std;

// ----------------
//      AP_main

GB_ERROR AP_main::open(const char *db_server) {
    GB_ERROR error             = 0;
    GLOBAL_gb_main             = GB_open(db_server, "rwt");
    if (!GLOBAL_gb_main) error = GB_await_error();
    return error;
}

void AP_main::user_push() {
    frameData.user_push_counter = stack_level + 1;
    push();
}

void AP_main::user_pop() {
    // checks if user_pop possible
    if (frameData.user_push_counter == stack_level) {
        pop();    // changes user_push_counter if user pop
    }
    else {
        aw_message("No user-pop possible");
    }
}

void AP_main::push() {
    // if count > 1 the nodes are buffered more than once
    // WARNING:: node only has to be buffered once in the stack
    //
    //
    stack_level ++;
    if (currFrame) list.push(currFrame);

    currFrame = new NodeStack(frameData);

#if defined(CHECK_ROOT_POPS)
    currFrame->root_at_create = DOWNCAST(AP_tree_nlen*, get_tree_root()->get_root_node());
#endif
}

void AP_main::pop() {
    if (!currFrame) GBK_terminate("AP_main::pop on empty stack");

    {
        AP_tree_nlen *node;
        while ((node = currFrame->pop())) {
            if (stack_level != node->stack_level) {
                GB_internal_error("AP_main::pop: Error in stack_level");
                cout << "Main UPD - node UPD : " << stack_level << " -- " << node->stack_level << " \n";
                return;
            }
            node->pop(stack_level);
        }
    }

#if defined(CHECK_ROOT_POPS)
    ap_assert(currFrame->root_at_create == get_tree_root()->get_root_node()); // root has been restored!
#endif

    delete currFrame;
    stack_level --;

    currFrame     = list.pop();
    frameData = currFrame ? currFrame->get_previous_frame_data() : StackFrameData();
}

void AP_main::clear() {
    // removes count elements from the list
    // because the current tree is used
    //
    // if stack_counter greater than last user_push then
    // moves all not previous buffered nodes in the
    // previous stack

    if (!currFrame) GBK_terminate("AP_main::clear on empty stack");

    AP_tree_nlen *node;

    if (frameData.user_push_counter >= stack_level) {
        if (currFrame->size() > 0) {
            while (currFrame->size() > 0) {
                node = currFrame->pop();
                node->clear(stack_level, frameData.user_push_counter);
            }
        }
        delete currFrame;
        currFrame = list.pop();
    }
    else {
        NodeStack *next_frame = list.pop();
        while ((node = currFrame->pop())) {
            if (node->clear(stack_level, frameData.user_push_counter) != true) {
                // node is not cleared because buffered in previous node stack
                // node is instead copied in previous level
                if (next_frame) next_frame->push(node);
            }
        }
        delete currFrame;
        currFrame = next_frame;
    }
    stack_level --;

    frameData = currFrame ? currFrame->get_previous_frame_data() : StackFrameData();
}

void AP_main::push_node(AP_tree_nlen *node, AP_STACK_MODE mode) {
    //
    //  stores node
    //
    if (!currFrame) {
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
        if (frameData.root_pushed) {
            // do not push root twice inside same stack_level
            mode = BOTH;
        }
        else {
            frameData.root_pushed = true;
#if defined(CHECK_ROOT_POPS)
            ap_assert(node == currFrame->root_at_create); // make sure the pushed root is the correct one
#endif
        }
#endif
    }

    if (node->push(mode, stack_level)) currFrame->push(node);
    if (mode == ROOT) {
        // In AP_main::pop(), root-node has to be restored after everything else has been restored.
        // Move node to bottom of stack now to ensure that.
        ASSERT_RESULT(bool, true, currFrame->remove(node));
        currFrame->shift(node);
    }
}

void AP_main::set_tree_root(AWT_graphic_parsimony *agt_) {
    ap_assert(agt == 0 && agt_ != 0);               // do only once
    agt = agt_;
}

const char *AP_main::get_aliname() const {
    return get_tree_root()->get_aliview()->get_aliname();
}

