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

/* AP_STACK        Stack container
 *
 * -- buffers
 *
 * AP_tree_buffer  holds (partial) state of AP_tree_nlen
 *
 * -- used stacks
 *
 * AP_tree_stack   stack containing AP_tree_buffer* (each AP_tree_nlen contains one)
 * AP_main_stack   stack containing AP_tree_nlen* (current stackframe is member of AP_main)
 * AP_main_list    stack containing AP_main_stack (member of AP_main, stores previous stackframes)
 */

#if defined(DEBUG)
#define PROVIDE_PRINT
#endif

template <typename ELEM>
struct AP_STACK_ELEM {
    AP_STACK_ELEM *next;
    ELEM          *node;
};

template <typename ELEM>
class AP_STACK : virtual Noncopyable {
    typedef AP_STACK_ELEM<ELEM> StackElem;

    StackElem     *first;
    unsigned long  stacksize;

public:
    AP_STACK()
        : first(NULL),
          stacksize(0)
    {}
    virtual ~AP_STACK() {
        if (stacksize>0) {
            GBK_terminate("AP_STACK not empty in dtor");
        }
    }

    class iterator {
        StackElem *current;
    public:
        iterator(AP_STACK *stack) : current(stack ? stack->first : NULL) {}

        ELEM *operator*() const { return current->node; }
        iterator& operator++() { current = current->next; return *this; }
        bool operator == (const iterator& other) const { return current == other.current; }
        bool operator != (const iterator& other) const { return current != other.current; }
    };

    iterator begin() { return iterator(this); }
    iterator end() { return iterator(NULL); }

    void push(ELEM *element) {
        //! add 'element' to top of stack
        StackElem *stackelem = new StackElem;
        stackelem->node = element;
        stackelem->next = first;
        first = stackelem;
        stacksize++;
    }
    void shift(ELEM *element) {
        //! add 'element' to bottom of stack
        if (stacksize) {
            StackElem *bottomelem               = first;
            while (bottomelem->next) bottomelem = bottomelem->next;

            StackElem *newelem = new StackElem;
            newelem->node      = element;
            newelem->next      = NULL;

            bottomelem->next = newelem;
        }
        else {
            push(element);
        }
    }

    ELEM *pop() {
        if (!first) return 0;

        StackElem *stackelem = first;
        ELEM      *pntr      = first->node;

        first = first->next;
        stacksize --;
        delete stackelem;

        return pntr;
    }
    bool remove(ELEM *element) {
        //! remove 'element' from stack
        if (!first) return false;

        StackElem** nextPtr = &first;
        while (*nextPtr) {
            StackElem *next = *nextPtr;
            if (next->node == element) {
                *nextPtr = next->next;
                delete next;
                return true;
            }
            nextPtr = &(next->next);
        }
        return false;
    }

    void clear() {
        while (stacksize > 0) {
            StackElem *pntr = first;
            first = first->next;
            stacksize --;
            delete pntr;
        }
    }
    ELEM *top() { return first ? first->node : NULL; }
    unsigned long size() { return stacksize; }
};

// ----------------------------------------------------------------
//      special buffer-structures for AP_tree and AP_tree_edge

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

class AP_tree;
class AP_tree_root;

struct AP_tree_buffer {
    unsigned long  controll;                        // used for internal buffer check
    unsigned int   count;                           // counts how often the entry is buffered
    AP_STACK_MODE  mode;
    AP_sequence   *sequence;
    AP_FLOAT       mutation_rate;
    double         leftlen, rightlen;
    AP_tree       *father;
    AP_tree       *leftson;
    AP_tree       *rightson;
    AP_tree_root  *root;
    GBDATA        *gb_node;

    int distance;                                   // distance to border (pushed with STRUCTURE!)

    // data from edges:
    AP_tree_edge      *edge[3];
    int                edgeIndex[3];
    AP_tree_edge_data  edgeData[3];

#if defined(PROVIDE_PRINT)
    void print();
#endif
};

struct AP_tree_stack : public AP_STACK<AP_tree_buffer> {
#if defined(PROVIDE_PRINT)
    void print();
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
    unsigned long user_push_counter;
#if defined(AVOID_MULTI_ROOT_PUSH)
    bool          root_pushed;
    StackFrameData() : user_push_counter(0), root_pushed(false) {}
#else
    StackFrameData() : user_push_counter(0) {}
#endif
};

class AP_main_stack : public AP_STACK<AP_tree_nlen> { // derived from Noncopyable
    StackFrameData previous;

public:
    explicit AP_main_stack(const StackFrameData& data)
        : previous(data)
    {}

    const StackFrameData& get_previous_frame_data() const { return previous; }

#if defined(CHECK_ROOT_POPS)
    AP_tree_nlen *root_at_create; // root at creation time of stack
#endif
#if defined(PROVIDE_PRINT)
    void print();
#endif
};

typedef AP_STACK<AP_main_stack> AP_main_list;

#else
#error AP_buffer.hxx included twice
#endif // AP_BUFFER_HXX
