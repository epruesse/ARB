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


/* AP_STACK        dynamischer Stack fuer void *
 * AP_LIST         allgemeine doppelt verketteten Liste
 *
 * -- Pufferstrukturen
 *
 * AP_tree_buffer  Struktur die im AP_tree gepuffert wird
 *
 * -- spezielle stacks ( um casts zu vermeiden )
 *
 * AP_tree_stack   Stack fuer AP_tree_buffer *
 * AP_main_stack   Stack fuer AP_tree *
 * AP_main_list    Liste fuer AP_main_buffer
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

#if defined(PROVIDE_PRINT)
    StackElem *pointer;
#endif

public:
    AP_STACK()
        : first(NULL),
          stacksize(0)
#if defined(PROVIDE_PRINT)
        , pointer(NULL)
#endif
    {}
    virtual ~AP_STACK() {
        if (stacksize>0) {
            GBK_terminate("AP_STACK not empty in dtor");
        }
    }

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

#if defined(PROVIDE_PRINT)
    // iterator:
    void  get_init() { pointer = 0; }
    ELEM *get() {
        if (0 == pointer) {
            pointer = first;
        }
        else {
            if (pointer->next == 0) {
                return 0;
            }
            else {
                pointer = pointer->next;
            }
        }
        return pointer->node;
    }
#endif
};

// ----------------
//      AP_LIST

struct AP_list_elem {
    AP_list_elem * next;
    AP_list_elem * prev;
    void *         node;
};

class AP_LIST : virtual Noncopyable {
    unsigned int  list_len;
    unsigned int  akt;
    AP_list_elem *first, *last, *pointer;

    AP_list_elem *element(void * elem);
public:
    AP_LIST()
        : list_len(0),
          akt(0),
          first(NULL), 
          last(NULL), 
          pointer(NULL) 
    {}

    virtual ~AP_LIST() {}

    int   len();
    int   is_element(void * node);
    int   eof();
    void  insert(void * new_one);
    void  append(void * new_one);
    void  remove(void * object);
    void  push(void *elem);
    void *pop();
    void  clear();
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


struct AP_main_list : public AP_LIST {
    AP_main_stack * pop() { return  (AP_main_stack *)AP_LIST::pop(); }
    void push(AP_main_stack * stack) { AP_LIST::push((void *) stack); }
    void insert(AP_main_stack * stack) { AP_LIST::insert((void *)stack); }
    void append(AP_main_stack * stack) { AP_LIST::append((void *)stack); }
};

#else
#error AP_buffer.hxx included twice
#endif // AP_BUFFER_HXX
