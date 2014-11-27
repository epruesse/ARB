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

struct AP_tree_edge_data
{
    AP_FLOAT parsValue[3];                          // the last three parsimony values (0=lowest 2=highest)
    int      distance;                              // the distance of the last insertion
};

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

struct NodeState { // buffers previous states of AP_tree_nlen
    Level frameNr;          // state of AP_tree_nlen::pushed_to_frame at creation time of NodeState

    // @@@ reorder members and mark what is stored in which mode

    AP_STACK_MODE  mode;
    AP_sequence   *sequence;
    AP_FLOAT       mutation_rate;
    double         leftlen, rightlen;
    AP_tree_nlen  *father;
    AP_tree_nlen  *leftson;
    AP_tree_nlen  *rightson;
    AP_pars_root  *root;
    GBDATA        *gb_node;

    int distance;                                   // distance to border (pushed with STRUCTURE!)

    // data from edges:
    AP_tree_edge      *edge[3];
    int                edgeIndex[3];
    AP_tree_edge_data  edgeData[3];

#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel = 0) const;
#endif
};

struct StateStack : public AP_STACK<NodeState> {
#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel = 0) const;
#endif
};

class AP_tree_nlen;

#define AVOID_MULTI_ROOT_PUSH
// old version did not avoid (i.e. it was possible to push multiple ROOTs)
// fails some tests when undefined

#if defined(ASSERTION_USED)
#define CHECK_ROOT_POPS
#endif

struct StackFrameData { // data local to current stack frame
    Level user_push_counter; // @@@ eliminate (instead maintain in AP_main)
#if defined(AVOID_MULTI_ROOT_PUSH)
    bool root_pushed;
    StackFrameData() : user_push_counter(0), root_pushed(false) {}
#else
    StackFrameData() : user_push_counter(0) {}
#endif
};

class NodeStack : public AP_STACK<AP_tree_nlen> { // derived from Noncopyable
    StackFrameData previous;

public:
    explicit NodeStack(const StackFrameData& data)
        : previous(data)
    {}

    const StackFrameData& get_previous_frame_data() const { return previous; }

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
