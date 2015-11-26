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
#include <map>

using namespace std;

// ----------------
//      AP_main

GB_ERROR AP_main::open(const char *db_server) {
    GB_ERROR error      = 0;
    gb_main             = GB_open(db_server, "rwt");
    if (!gb_main) error = GB_await_error();
    return error;
}

void AP_main::remember_user_state() {
    user_frames.push(new UserFrame(frameLevel));
    remember();
}

void AP_main::revert_user_state() {
    if (user_frames.empty()) {
        aw_message("No user-pop possible");
    }
    else {
        UserFrame *wanted = user_frames.pop();
        if (frameLevel<=wanted->get_level()) {
            aw_message("Wanted user_frame not available");
        }
        else {
            while (frameLevel>wanted->get_level()) {
                revert();
            }
        }
        delete wanted;
    }
}

void AP_main::remember() {
    /*! remember current tree state
     * @see revert() and accept()
     */

    frameLevel ++;
    if (currFrame) frames.push(currFrame);
    currFrame = new NodeStack(frameData);
    ap_assert(!frameData);
    frameData = new StackFrameData;

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
            if (frameLevel != node->last_remembered_frame()) {
                cerr << "Main frame level=" << frameLevel << " node frame level=" << node->last_remembered_frame() << endl;
                GBK_terminate("AP_main::pop: main/node frame-level inconsistency");
            }
            node->revertToPreviousState(frameLevel, rootPopped);
        }
    }

    ap_assert(rootPopped == frameData->root_pushed);
#if defined(CHECK_ROOT_POPS)
    ap_assert(currFrame->root_at_create == get_tree_root()->get_root_node()); // root has been restored!
#endif

    currFrame->revert_resources(frameData);
    delete frameData;
    frameData = currFrame->take_previous_frame_data();

    delete currFrame;
    currFrame = frames.pop();
    frameLevel --;
}

struct HasLevelSmallerThan {
    Level level;
    HasLevelSmallerThan(Level level_) : level(level_) {}
    bool operator()(const NodeState *state) const { return state->frameNr < level; }
};

void AP_main::push_nodes_changed_since(Level wanted_frameLevel) {
    // does not consider currFrame (because we push_nodes to currFrame)
    Level frame_level = frameLevel-1;
    ap_assert(frame_level == frames.count_elements());

    typedef map<AP_tree_nlen*,int> NodeCounter;
    NodeCounter pops2skip; // specifies how many pops to skip for each node

    AP_tree_nlen *oldRoot = get_root_node();

    push_node(oldRoot, ROOT);

    for (FrameStack::const_iterator f = frames.begin(); f != frames.end(); ++f, --frame_level) {
        if (frame_level == wanted_frameLevel) break;
        ap_assert(frame_level>wanted_frameLevel);

        const NodeStack     *frame = *f;
        HasLevelSmallerThan  hasWantedLevel(frame_level);

        for (NodeStack::const_iterator n = frame->begin(); n != frame->end(); ++n) {
            AP_tree_nlen *node = *n;
            if (node == oldRoot) continue;

            NodeCounter::iterator      skipCounter = pops2skip.find(node);
            int                        skip        = skipCounter == pops2skip.end() ? 0 : skipCounter->second;
            const StateStack&          states      = node->get_states();
            StateStack::const_iterator toPush      = states.begin();

            if (skip>0) advance(toPush, skip);

            ap_assert(toPush != states.end()); // otherwise node should not be listed in 'frame'
            ap_assert(hasWantedLevel(*toPush));

            const NodeState *prevState = *toPush;
            ap_assert(prevState->frameNr < frame_level);

            AP_STACK_MODE mode = prevState->mode;
            if (mode == ROOT || node->is_root_node()) {
                mode = BOTH;
            }
#if defined(ASSERTION_USED)
            Level lold = states.count_elements();
#endif
            int pushed = push_node(node, mode);

            ap_assert(states.count_elements() == lold+pushed);

            pops2skip[node] = skip+1+pushed;
        }
    }
}

void AP_main::rollback_to(Level wanted_frameLevel) {
    // does not consider currFrame
    // (assumes tree state is identical to state before last push())
    Level frame_level = frameLevel-1;
    ap_assert(frame_level == frames.count_elements());

    typedef map<AP_tree_nlen*,int> NodeCounter;
    NodeCounter pops2skip; // specifies how many pops to skip for each node

    // skip pushes done by push_nodes_changed_since()
    for (NodeStack::const_iterator n = currFrame->begin(); n != currFrame->end(); ++n) {
        AP_tree_nlen *node = *n;
        pops2skip[node]    = 1;
    }

    for (FrameStack::const_iterator f = frames.begin(); f != frames.end(); ++f, --frame_level) {
        if (frame_level == wanted_frameLevel) break;
        ap_assert(frame_level>wanted_frameLevel);

        const NodeStack *frame = *f;
#if defined(ASSERTION_USED)
        HasLevelSmallerThan  hasWantedLevel(frame_level);
#endif

        for (NodeStack::const_iterator n = frame->begin(); n != frame->end(); ++n) {
            AP_tree_nlen               *node        = *n;
            NodeCounter::iterator       skipCounter = pops2skip.find(node);
            int                         skip        = skipCounter == pops2skip.end() ? 0 : skipCounter->second;
            const StateStack&           states      = node->get_states();
            StateStack::const_iterator  toRestore   = states.begin();

            if (skip>0) advance(toRestore, skip);

            ap_assert(toRestore != states.end()); // otherwise node should not be listed in 'frame'
            ap_assert(hasWantedLevel(*toRestore));

            const NodeState *prevState = *toRestore;
            ap_assert(prevState->frameNr < frame_level);
            node->restore_nondestructive(*prevState);

            pops2skip[node] = skip+1;
        }

#if defined(CHECK_ROOT_POPS)
        ap_assert(frame->root_at_create == get_tree_root()->get_root_node()); // root has been restored!
#endif
    }
}

void AP_main::accept() {
    /*! accept changes performed on tree (since last remember())
     * @see revert()
     */

    if (!currFrame) GBK_terminate("AP_main::accept on empty stack");
    ap_assert(frameData);

    AP_tree_nlen *node;

    NodeStack *prev_frame = frames.pop();

    // detect nodes (and edges) created AND destroyed inside currently accepted stack frame
    ResourceStack *common = NULL;
    if (prev_frame) {
        common = new ResourceStack;
        frameData->extract_common_to(*common);
    }

    while ((node = currFrame->pop())) {
        if (node->acceptCurrentState(frameLevel) != true) {
            // stored node state was not discarded (because it was not stored for the previous stack frame).
            // if revert() gets called for previous stack frane, it is necessary to revert
            // the current change as well -> move into previous frame
            if (prev_frame) {
                if (common->has_node(node)) {
                    // do NOT push nodes which get destroyed by accept_resources() below!
                    ASSERT_RESULT(bool, true, node->acceptCurrentState(1)); // force state drop
                }
                else {
                    prev_frame->push(node); // @@@ frames are pushed in reverted order (seems to be wrong)
                }
            }
        }
    }

    currFrame->accept_resources(frameData, common);
    delete frameData;
    frameData = currFrame->take_previous_frame_data();

    delete currFrame;
    currFrame = prev_frame;
    frameLevel --;

    delete common;
}

bool AP_main::push_node(AP_tree_nlen *node, AP_STACK_MODE mode) {
    /*! stores node in currFrame (if exists)
     * if (mode&SEQUENCE) => sequence gets invalidated
     * @return true, if a new entry has been created. false, if existing entry was extended or no stack exists.
     */

    bool pushed = false;
    if (!currFrame) {
        if (mode & SEQUENCE) node->unhash_sequence();
    }
    else {
        if (frameLevel < node->last_remembered_frame()) {
            cerr << "Main frame level=" << frameLevel << " node frame level=" << node->last_remembered_frame() << endl;
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

        if (node->rememberState(mode, frameLevel)) {
            currFrame->push(node);
            pushed = true;
        }
        if (mode == ROOT) {
            // In AP_main::pop(), root-node has to be restored after everything else has been restored.
            // Move node to bottom of stack now to ensure that.
            currFrame->remove(node);
            currFrame->shift(node);
        }
    }
    return pushed;
}

void AP_main::set_tree_root(AWT_graphic_parsimony *agt_) {
    ap_assert(agt == 0 && agt_ != 0);               // do only once
    agt = agt_;
}

const char *AP_main::get_aliname() const {
    return get_tree_root()->get_aliview()->get_aliname();
}

#if defined(UNIT_TESTS)
void AP_main::remember_whole_tree() {
    remember();
    AP_tree_nlen *root = get_root_node();
    push_node(root, ROOT);
    ap_assert(!root->is_leaf);
    root->get_leftson()->remember_subtree(this);
    root->get_rightson()->remember_subtree(this);
}
#endif
