// =============================================================== //
//                                                                 //
//   File      : AP_buffer.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_BUFFER_HXX
#define AP_BUFFER_HXX

#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif
#ifndef ARB_FORWARD_LIST_H
#include <arb_forward_list.h>
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif

/* AP_STACK        Stack container
 *
 * -- buffers
 *
 * NodeState  holds (partial) state of AP_tree_nlen
 *
 * -- used stacks
 *
 * StateStack      stack containing NodeState* (each AP_tree_nlen contains one)
 * NodeStack       stack containing AP_tree_nlen* (NodeStack of current frame is member of AP_main)
 * FrameStack      stack containing NodeStacks (member of AP_main, stores previous NodeStack frames)
 */

template <typename ELEM>
struct AP_STACK : public arb_forward_list<ELEM*> {
    typedef arb_forward_list<ELEM*>       BASE;
    typedef typename BASE::iterator       iterator;
    typedef typename BASE::const_iterator const_iterator;

    void push(ELEM *element) {
        //! add 'element' to top of stack
        BASE::push_front(element);
    }
    void shift(ELEM *element) {
        //! add 'element' to bottom of stack
#if defined(Cxx11)
        // uses forward_list
        if (BASE::empty()) {
            push(element);
        }
        else {
            iterator i = BASE::begin();
            iterator n = i;
            while (true) {
                if (++n == BASE::end()) {
                    BASE::insert_after(i, element);
                    break;
                }
                i = n;
            }
        }
#else
        // uses list
        BASE::push_back(element);
#endif
    }
    ELEM *pop() {
        if (BASE::empty()) return NULL;

        ELEM *result = top();
        BASE::pop_front();
        return result;
    }

    ELEM *top() {
        ap_assert(!BASE::empty());
        return BASE::front();
    }
    const ELEM *top() const {
        ap_assert(!BASE::empty());
        return BASE::front();
    }

    size_t count_elements() const {
#if defined(Cxx11)
        size_t s = 0;
        for (const_iterator i = BASE::begin(); i != BASE::end(); ++i) ++s;
        return s;
#else // !defined(Cxx11)
        return BASE::size();
#endif
    }
};

// ----------------------------------------------------------------
//      special buffer-structures for AP_tree and AP_tree_edge

#if defined(DEBUG)
#define PROVIDE_PRINT
#endif

#if defined(PROVIDE_PRINT)
#include <ostream>
#endif


class AP_tree_edge; // defined in ap_tree_nlen.hxx

enum AP_STACK_MODE {
    NOTHING   = 0, // nothing to buffer in AP_tree node
    STRUCTURE = 1, // only structure
    SEQUENCE  = 2, // only sequence
    BOTH      = 3, // sequence & treestructure is buffered
    ROOT      = 7  // old root is buffered (includes BOTH)
};

class AP_tree_nlen;
class AP_pars_root;

typedef unsigned long Level;

struct NodeState : virtual Noncopyable { // buffers previous states of AP_tree_nlen
    Level         frameNr;  // state of AP_tree_nlen::remembered_for_frame at creation time of NodeState
    AP_STACK_MODE mode;     // what has been stored?

    // only defined if mode & SEQUENCE:
    AP_sequence  *sequence; // if set -> NodeState is owner!
    AP_FLOAT      mutation_rate;

    // only defined if mode & STRUCTURE:
    double        leftlen, rightlen;
    AP_tree_nlen *father;
    AP_tree_nlen *leftson;
    AP_tree_nlen *rightson;
    AP_pars_root *root;
    GBDATA       *gb_node;
    AP_tree_edge *edge[3];
    int           edgeIndex[3];

    // defined/restored if mode & (SEQUENCE|STRUCTURE): 
    bool had_marked;

    void move_info_to(NodeState& target, AP_STACK_MODE what);

#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel = 0) const;
#endif

    explicit NodeState(Level frame_nr) : frameNr(frame_nr), mode(NOTHING) {}
    ~NodeState() { if (mode & SEQUENCE) delete sequence; }
};

struct StateStack : public AP_STACK<NodeState> {
#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel = 0) const;
#endif
};

class AP_tree_nlen;
class AP_tree_edge;

#if defined(ASSERTION_USED)
#define CHECK_ROOT_POPS
#endif

typedef std::set<AP_tree_nlen*> NodeSet;
typedef std::set<AP_tree_edge*> EdgeSet;

class ResourceStack {
    // stores nodes and edges for resource management
    NodeSet nodes;
    EdgeSet edges;
public:
    ResourceStack() {}
    ~ResourceStack() {
        ap_assert(nodes.empty());
        ap_assert(edges.empty());
    }

    void extract_common(ResourceStack& stack1, ResourceStack& stack2);

    void destroy_nodes();
    void destroy_edges();
    void forget_nodes();
    void forget_edges();
    void move_nodes(ResourceStack& target);
    void move_edges(ResourceStack& target);

    AP_tree_nlen *put(AP_tree_nlen *node) { nodes.insert(node); return node; }
    AP_tree_edge *put(AP_tree_edge *edge) { edges.insert(edge); return edge; }

    AP_tree_nlen *getNode() { ap_assert(!nodes.empty()); AP_tree_nlen *result = *nodes.begin(); nodes.erase(nodes.begin()); return result; }
    AP_tree_edge *getEdge() { ap_assert(!edges.empty()); AP_tree_edge *result = *edges.begin(); edges.erase(edges.begin()); return result; }

    bool has_nodes() const { return !nodes.empty(); }
    bool has_edges() const { return !edges.empty(); }

    bool has_node(AP_tree_nlen *node) const { return nodes.find(node) != nodes.end(); }
};

class StackFrameData : virtual Noncopyable {
    // data local to current stack frame
    // as well exists for stack frame = 0 (i.e. when nothing has been remember()ed yet)

    ResourceStack created;   // nodes and edges created in the current stack frame
    ResourceStack destroyed; // same for destroyed

public:
    bool  root_pushed; // @@@ move into NodeStack
    StackFrameData() : root_pushed(false) {}

    void revert_resources(StackFrameData *previous);
    void accept_resources(StackFrameData *previous, ResourceStack *common);

    void extract_common_to(ResourceStack& common) { common.extract_common(created, destroyed); }

    inline AP_tree_nlen *makeNode(AP_pars_root *proot);
    inline AP_tree_edge *makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2);
    inline void destroyNode(AP_tree_nlen *node);
    inline void destroyEdge(AP_tree_edge *edge);
};

#if defined(ASSERTION_USED)
#define CHECK_STACK_RESOURCE_HANDLING
#endif

class NodeStack : public AP_STACK<AP_tree_nlen> { // derived from Noncopyable
    StackFrameData *previous;
#if defined(CHECK_STACK_RESOURCE_HANDLING)
    enum ResHandled { NO, ACCEPTED, REVERTED } resources_handled;
#endif

public:
    explicit NodeStack(StackFrameData*& data)
        : previous(data)
#if defined(CHECK_STACK_RESOURCE_HANDLING)
        , resources_handled(NO)
#endif
    {
        data = NULL; // take ownership
    }
    ~NodeStack() {
        ap_assert(!previous); // forgot to use take_previous_frame_data()
#if defined(CHECK_STACK_RESOURCE_HANDLING)
        ap_assert(resources_handled != NO);
#endif
    }

    StackFrameData *take_previous_frame_data() {
        StackFrameData *release = previous;
        previous = NULL;     // release ownership
        return release;
    }

    void revert_resources(StackFrameData *current) {
#if defined(CHECK_STACK_RESOURCE_HANDLING)
        ap_assert(resources_handled == NO);
        resources_handled = REVERTED;
#endif
        current->revert_resources(previous);
    }
    void accept_resources(StackFrameData *current, ResourceStack *common) {
#if defined(CHECK_STACK_RESOURCE_HANDLING)
        ap_assert(resources_handled == NO);
        resources_handled = ACCEPTED;
#endif
        current->accept_resources(previous, common);
    }

#if defined(CHECK_ROOT_POPS)
    AP_tree_nlen *root_at_create; // root at creation time of stack
#endif
#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel, Level frameNr) const;
#endif
};

struct FrameStack : public AP_STACK<NodeStack> {
#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel = 0) const;
#endif
};

#else
#error AP_buffer.hxx included twice
#endif // AP_BUFFER_HXX
