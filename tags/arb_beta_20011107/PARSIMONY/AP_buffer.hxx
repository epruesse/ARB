
#ifndef _AP_BUUFER_INC
#define _AP_BUUFER_INC




/*******************

AP_STACK	dynamischer Stack fuer void *
AP_LIST		allgemeine doppelt verketteten Liste

-- Pufferstrukturen 

AP_tree_buffer  Struktur die im AP_tree gepuffert wird

-- spezielle stacks ( um casts zu vermeiden ) 

AP_tree_stack   Stack fuer AP_tree_buffer *
AP_main_stack	Stack fuer AP_tree *
AP_main_list	Liste fuer AP_main_buffer

********************/


struct AP_STACK_ELEM {
	struct AP_STACK_ELEM * next;
	void * node;
	};

class AP_STACK {
	struct AP_STACK_ELEM * 	first;
	struct AP_STACK_ELEM * 	pointer;		
	unsigned long 		stacksize;
	unsigned long 		max_stacksize;

	public:
	AP_STACK(); 
	~AP_STACK();
	void 		push(void * element);
	void * 		pop();
	void 		clear(); 
	void 		get_init()	;
	void *		get(); 	
	void *		get_first();			
	unsigned long	size();
	};
	
/*********************************

AP_LIST

Listenklasse

********************************/

struct AP_list_elem {
	AP_list_elem *	next;
	AP_list_elem *	prev;
	void *  	node;
	};

class AP_LIST {
	unsigned int list_len;
	unsigned int akt;
	AP_list_elem *first,*last,*pointer;
	AP_list_elem *element(void * elem);
	public:
	AP_LIST() ;
	~AP_LIST();
	int 		len(); 	 
	int 		is_element(void * node );
	int 		eof();
	void		 insert(void * new_one);
	void 		append(void * new_one);
	void 		remove(void * object);
	void 		push(void *elem);
	void * 		pop(); 
	void 		clear();			
};


/************************************

	Spezielle Puffer Strukturen
	fuer AP_NTREE

************************************/
class AP_tree_edge; 		// defined in ap_tree_nlen.hxx
struct AP_tree_buffer; 

struct AP_tree_edge_data
{
    AP_FLOAT parsValue[3];	// the last three parsimony values (0=lowest 2=highest)
    int	distance;		// the distance of the last insertion
}; 

		
struct AP_tree_buffer {	
	unsigned long controll;		// used for internal buffer check
	unsigned int count;		// counts how often the entry is buffered
	AP_STACK_MODE mode;
	class AP_sequence * sequence;
	AP_FLOAT mutation_rate;
	double leftlen,rightlen;
	class AP_tree *father;
	class AP_tree *leftson;
	class AP_tree *rightson;
	GBDATA *gb_node;

	int distance; 	// distance to border (pushed with STRUCTURE!)
    // stuff from edges:
    
	class AP_tree_edge 		*edge[3]; 
	int 				edgeIndex[3]; 
	struct AP_tree_edge_data 	edgeData[3]; 
    	
	void print();
	};

class AP_tree_stack : public AP_STACK {	
	public:
	AP_tree_stack() {
		;
		}
	void  push(struct AP_tree_buffer *value) {
		AP_STACK::push((void *)value);
		}
	AP_tree_buffer * pop() {
		return ( AP_tree_buffer *) AP_STACK::pop();
		}
	AP_tree_buffer * get() {
		return ( AP_tree_buffer *) AP_STACK::get();
		}
	AP_tree_buffer * get_first() {
		return ( AP_tree_buffer *) AP_STACK::get_first();
		}
	void print();
	};



class AP_main_stack : public AP_STACK {
	protected:
	unsigned long last_user_buffer;
	public:
	friend class AP_main;	
	void push(AP_tree *value) {
		AP_STACK::push((void *)value);
		}
	AP_tree * pop() {
		return (AP_tree *) AP_STACK::pop();
		}
	AP_tree * get() {
		return (AP_tree *) AP_STACK::get();
		}
	AP_tree * get_first() {
		return (AP_tree *) AP_STACK::get_first();
		}
	void print();		
	};


class AP_main_list : public AP_LIST {
	public:
	AP_main_stack * pop() {
		return 	(AP_main_stack *)AP_LIST::pop();
		}
	void push(AP_main_stack * stack) {
		AP_LIST::push( (void *) stack);
		}
	void insert(AP_main_stack * stack) {
		AP_LIST::insert((void *)stack);
		}
	void append(AP_main_stack * stack) {
		AP_LIST::append((void *)stack);
		}
	};

#endif
