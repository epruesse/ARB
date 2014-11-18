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


struct AP_STACK_ELEM {
    struct AP_STACK_ELEM * next;
    void *                 node;
};

class AP_STACK : virtual Noncopyable { // @@@ make typesafe (use template)
    struct AP_STACK_ELEM * first;
    struct AP_STACK_ELEM * pointer;
    unsigned long          stacksize;

public:
    AP_STACK()
        : first(NULL),
          pointer(NULL),
          stacksize(0)
    {}
    virtual ~AP_STACK() {
        if (stacksize>0) {
            GBK_terminate("AP_STACK not empty in dtor");
        }
    }

    void push(void *element) {
        AP_STACK_ELEM *stackelem = new AP_STACK_ELEM;
        stackelem->node = element;
        stackelem->next = first;
        first = stackelem;
        stacksize++;
    }

    void *pop() {
        if (!first) return 0;

        AP_STACK_ELEM *stackelem = first;
        void *         pntr      = first->node;

        first = first->next;
        stacksize --;
        delete stackelem;

        return pntr;
    }
    void clear() {
        while (stacksize > 0) {
            AP_STACK_ELEM *pntr = first;
            first = first->next;
            stacksize --;
            delete pntr;
        }
    }
    void *top() { return first ? first->node : NULL; }
    unsigned long size() { return stacksize; }

    // iterator:
    void  get_init() { pointer = 0; }
    void *get() {
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

class  AP_tree_edge;                                // defined in ap_tree_nlen.hxx
struct AP_tree_buffer;

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
    AP_tree  *father;
    AP_tree  *leftson;
    AP_tree  *rightson;
    AP_tree_root  *root;
    GBDATA        *gb_node;

    int distance;                                   // distance to border (pushed with STRUCTURE!)

    // data from edges:
    AP_tree_edge      *edge[3];
    int                edgeIndex[3];
    AP_tree_edge_data  edgeData[3];

    void print();
};

struct AP_tree_stack : public AP_STACK {
    AP_tree_stack() {}
    virtual ~AP_tree_stack() OVERRIDE {}
    void  push(AP_tree_buffer *value) { AP_STACK::push((void *)value); }
    AP_tree_buffer * pop() { return (AP_tree_buffer *) AP_STACK::pop(); }
    AP_tree_buffer * get() { return (AP_tree_buffer *) AP_STACK::get(); }
    AP_tree_buffer * top() { return (AP_tree_buffer *) AP_STACK::top(); }
    void print();
};

class AP_tree_nlen;

// #define AVOID_MULTI_ROOT_PUSH
// old version did not avoid (i.e. it was possible to push multiple ROOTs)

#if defined(ASSERTION_USED)
#define CHECK_ROOT_POPS
#endif

class AP_main_stack : public AP_STACK {
    unsigned long last_user_push_counter;
#if defined(AVOID_MULTI_ROOT_PUSH)
    bool last_root_pushed;
#endif
#if defined(CHECK_ROOT_POPS)
    AP_tree_nlen *root_at_create; // root at creation time of stack
#endif

public:
    friend class AP_main;
    void push(AP_tree_nlen *value) { AP_STACK::push((void *)value); }
    AP_tree_nlen *pop() { return (AP_tree_nlen*)AP_STACK::pop(); }
    AP_tree_nlen *get() { return (AP_tree_nlen*)AP_STACK::get(); }
    AP_tree_nlen *top() { return (AP_tree_nlen*)AP_STACK::top(); }
    void print();
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
