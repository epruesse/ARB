// =============================================================== //
//                                                                 //
//   File      : AP_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ap_main.hxx"
#include <aw_msg.hxx>

using namespace std;

// ----------------
//      AP_main

GB_ERROR AP_main::open(const char *db_server) {
    GB_ERROR error      = 0;
    gb_main             = GB_open(db_server, "rwt");
    if (!gb_main) error = GB_await_error();
    return error;
}

void AP_main::user_push() {
    frameData->user_push_counter = frameLevel + 1;
    remember();
}

void AP_main::user_pop() {
    // checks if user_pop possible
    if (frameData->user_push_counter == frameLevel) {
        revert();    // changes user_push_counter if user pop
    }
    else {
        aw_message("No user-pop possible");
    }
}

void AP_main::remember() {
    /*! remember current tree state
     * @see revert() and accept()
     */

    frameLevel ++;
    if (currFrame) frames.push(currFrame);

    {
        Level curr_user_push_counter = frameData->user_push_counter;

        currFrame = new NodeStack(frameData);
        ap_assert(!frameData);
        frameData = new StackFrameData(curr_user_push_counter);
    }

#if defined(CHECK_ROOT_POPS)
    currFrame->root_at_create = DOWNCAST(AP_tree_nlen*, get_tree_root()->get_root_node());
#endif
}

void AP_main::revert() {
    /*! revert tree to last remembered state
     * @see remember() and accept()
     */
    if (!currFrame) GBK_terminate("AP_main::pop on empty stack");

    bool rootPopped = false;
    {
        AP_tree_nlen *node;
        while ((node = currFrame->pop())) {
            if (frameLevel != node->get_pushed_to_frame()) {
                cerr << "Main frame level=" << frameLevel << " node frame level=" << node->get_pushed_to_frame() << endl;
                GBK_terminate("AP_main::pop: main/node frame-level inconsistency");
            }
            node->pop(frameLevel, rootPopped);
        }
    }

    ap_assert(rootPopped == frameData->root_pushed);
#if defined(CHECK_ROOT_POPS)
    ap_assert(currFrame->root_at_create == get_tree_root()->get_root_node()); // root has been restored!
#endif

    delete frameData;
    frameData = currFrame->take_previous_frame_data();

    delete currFrame;
    currFrame = frames.pop();
    frameLevel --;
}

void AP_main::accept() {
    /*! accept changes performed on tree (since last remember())
     * @see revert()
     */

    // @@@ outdated
    // removes count(?) elements from the list because the current tree is used
    //
    // if stack_counter greater than last user_push then
    // moves all not previous buffered nodes in the
    // previous stack

    if (!currFrame) GBK_terminate("AP_main::clear on empty stack");

    AP_tree_nlen *node;

    // @@@ ensure test coverage -> DRY cases below (they are nearly the same)

    NodeStack *prev_frame = frames.pop();
    if (frameData->user_push_counter >= frameLevel) {
        while (!currFrame->empty()) {
            UNCOVERED();
            node = currFrame->pop();
            node->clear(frameLevel, frameData->user_push_counter);
        }
    }
    else {
        while ((node = currFrame->pop())) {
            // UNCOVERED();
            if (node->clear(frameLevel, frameData->user_push_counter) != true) {
                // node was not cleared (because also buffered in previous node stack).
                // @@@ has to be done independent of user_push_counter
                // if revert() gets called for previous stack, it is necessary to revert
                // the current change as well -> move into previous frame
                // UNCOVERED();
                if (prev_frame) {
                    // UNCOVERED();
                    prev_frame->push(node); // @@@ frames are pushed in reverted order (seems to be wrong)
                }
            }
        }
    }

    delete frameData;
    frameData = currFrame->take_previous_frame_data();

    delete currFrame;
    currFrame = prev_frame;
    frameLevel --;
}

void AP_main::push_node(AP_tree_nlen *node, AP_STACK_MODE mode) {
    //
    //  stores node
    //
    if (!currFrame) {
        if (mode & SEQUENCE)    node->unhash_sequence();
        return;
    }

    if (frameLevel < node->get_pushed_to_frame()) {
        cerr << "Main frame level=" << frameLevel << " node frame level=" << node->get_pushed_to_frame() << endl;
        GBK_terminate("AP_main::push_node: main/node frame-level inconsistency");
    }

    if (mode == ROOT) {
        // test that it is really root what is pushed
        ap_assert(!node->father);
        ap_assert(node->is_root_node());

        if (frameData->root_pushed) {
            // do not push root twice in same frame
            mode = BOTH;
        }
        else {
            frameData->root_pushed = true;
#if defined(CHECK_ROOT_POPS)
            ap_assert(node == currFrame->root_at_create); // make sure the pushed root is the correct one
#endif
        }
    }

    if (node->push(mode, frameLevel)) currFrame->push(node);
    if (mode == ROOT) {
        // In AP_main::pop(), root-node has to be restored after everything else has been restored.
        // Move node to bottom of stack now to ensure that.
        currFrame->remove(node);
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

