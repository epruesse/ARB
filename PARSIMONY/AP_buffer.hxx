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

#ifndef AP_TREE_HXX
#include <AP_Tree.hxx>
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
    unsigned long          max_stacksize;

public:
    AP_STACK();
    virtual ~AP_STACK();

    void           push(void * element);
    void          *pop();
    void           clear();
    void           get_init();
    void          *get();
    void          *get_first();
    unsigned long  size();
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

    void print();
};

struct AP_tree_stack : public AP_STACK {
    AP_tree_stack() {}
    virtual ~AP_tree_stack() OVERRIDE {}
    void  push(AP_tree_buffer *value) { AP_STACK::push((void *)value); }
    AP_tree_buffer * pop() { return (AP_tree_buffer *) AP_STACK::pop(); }
    AP_tree_buffer * get() { return (AP_tree_buffer *) AP_STACK::get(); }
    AP_tree_buffer * get_first() { return (AP_tree_buffer *) AP_STACK::get_first(); }
    void print();
};

class AP_tree_nlen;

class AP_main_stack : public AP_STACK {
protected:
    unsigned long last_user_buffer;
public:
    friend class AP_main;
    void push(AP_tree_nlen *value) { AP_STACK::push((void *)value); }
    AP_tree_nlen *pop() { return (AP_tree_nlen*)AP_STACK::pop(); }
    AP_tree_nlen *get() { return (AP_tree_nlen*)AP_STACK::get(); }
    AP_tree_nlen *get_first() { return (AP_tree_nlen*)AP_STACK::get_first(); }
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
