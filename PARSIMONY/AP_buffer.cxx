#include <cstdio>
#include <memory.h>
#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>

#include "AP_buffer.hxx"
#include "AP_error.hxx"

using namespace std;

#define MAX_SPECIES 250000L
#define MAX_DSTACKSIZE (MAX_SPECIES*2)   // defaultwert fuer dynamischen stack

AP_STACK::AP_STACK() {
    first = 0;
    pointer = 0;
    stacksize = 0;
    max_stacksize = MAX_DSTACKSIZE;
}

AP_STACK::~AP_STACK() {
    if (stacksize > 0) {
        new AP_ERR("~AP_STACK()","Stack is not empty !",0);
    }
}

void AP_STACK::push(void * element) {
    AP_STACK_ELEM * stackelem = new AP_STACK_ELEM ;
    if (stacksize > max_stacksize) {
        new AP_ERR("AP_STACK:push()","Stack owerflow!",0);
    }
    stackelem->node = element;
    stackelem->next = first;
    first = stackelem;
    stacksize ++;
}

void * AP_STACK::pop() {
    void * pntr;
    AP_STACK_ELEM * stackelem;
    if (!first) return 0;
    stackelem = first;
    pntr = first->node;
    first = first->next;
    stacksize --;
    delete stackelem;
    return pntr;
}

void AP_STACK::clear() {
    AP_STACK_ELEM * pntr;
    while (stacksize > 0) {
        pntr = first;
        first = first->next;
        stacksize --;
        delete pntr;
    }
    return;
}

void AP_STACK::get_init() {
    pointer = 0;
}
void  * AP_STACK::get_first() {
    if (first != 0) {
        return first->node;
    } else {
        return 0;
    }
}

void  * AP_STACK::get() {
    if (0 == pointer ) {
        pointer = first;
    } else {
        if ( pointer->next == 0) {
            new AP_ERR("AP_STACK: get()"," more get() than element in stack");
            pointer = 0;
            return 0;
        } else {
            pointer = pointer->next;
        }
    }
    return pointer->node;
}

unsigned long AP_STACK::size() {
    return stacksize;
}



/******************************************

AP_LIST

****************************************/

AP_LIST::AP_LIST() {
    list_len = 0;
    first =  0;
    last = 0;
    akt = 0;
}

AP_LIST::~AP_LIST() {
}


AP_list_elem * AP_LIST::element(void * elem) {
    AP_list_elem *pntr = first;
    while (pntr != 0) {
        if (pntr->node == elem)
            return pntr;
        pntr = pntr->next;
    }
    return pntr;
}

int AP_LIST::len() {
    return list_len;
}

int AP_LIST::is_element(void * node ) {
    if (element(node) == 0) return 0 ;
    return 1;
}

int AP_LIST::eof() {
    if (akt == list_len) return 1;
    return 0;
}

void AP_LIST::insert(void * new_one) {
    AP_list_elem * newelem = new AP_list_elem;
    if (first == 0) {
        first = newelem;
        last = newelem;
        newelem->next = 0;
        newelem->prev = 0;
        pointer = first;
    }
    else {
        first->prev = newelem;
        newelem->prev = 0;
        newelem->next = first;
        first = newelem;
    }
    newelem->node = new_one;
    list_len++;
    return;
}

void AP_LIST::append(void * new_one) {
    AP_list_elem * newelem = new AP_list_elem;
    if (last == 0) {
        first = newelem;
        last = newelem;
        newelem->prev = 0;
        newelem->next = 0;
        pointer = first;
    } else {
        last->next = newelem;
        newelem->prev = last;
        last = newelem;
        newelem->next = 0;
    }
    newelem->node = new_one;
    list_len++;
    return;
}

void AP_LIST::remove(void * object) {
    AP_list_elem  *elem = element(object);
    if (elem) {
        if (elem->prev) {
            elem->prev->next = elem->next;
        } else {
            first = elem->next;
            elem->next->prev = 0;
        }
        if (elem->next) {
            elem->next->prev = elem->prev;
        } else {
            last = elem->prev;
            elem->prev->next = 0;
        }
        if (elem == pointer) pointer = 0;
        delete elem;
        list_len --;
        return;
    }
    new AP_ERR("AP_LIST::remove(void * object)","no buffer element !\n");
    return;
}

void AP_LIST::push(void *elem) {
    AP_list_elem * newelem = new AP_list_elem;
    if (first == 0) {
        first = newelem;
        last = newelem;
        newelem->next = 0;
        newelem->prev = 0;
        pointer = first;
    }
    else {
        first->prev = newelem;
        newelem->prev = 0;
        newelem->next = first;
        first = newelem;
    }
    newelem->node = elem;
    list_len++;
    return;
}

void *	AP_LIST::pop() {
    AP_list_elem * pntr = first;
    if (!first) return 0;
    void * node = first->node;
    list_len --;
    if (0 == list_len) {
        first = last = 0;
        delete pntr;
        return node;
    }
    else {
        first = first->next;
        first->prev = 0;
    }
    delete pntr;
    return node;
}


void AP_LIST::clear() {
    AP_list_elem*  npntr;
    AP_list_elem* pntr = first;
    while (pntr != 0) {
        npntr = pntr->next;
        delete pntr;
        pntr = npntr;
    }
    first = last = 0;
    akt = 0;
    list_len = 0;
}


/******************
print Funktionen fuer spezial Stacks

*******************/

void AP_tree_buffer::print() {
    cout  << "AP_tree_buffer                      " << this ;
    cout  << "\nfather " << father;
    cout  << "\nlefts  " << leftson;
    cout  << "\nrights " << rightson << "\n sequence " << sequence << "\n";
}

void AP_main_stack::print() {
    class AP_tree *elem;
    unsigned long i = this->size();
    cout << "AP_main_stack " << this << "  Size " << i << "\n";
    get_init();
    for (; i > 0; i--) {
        elem = (AP_tree *)get();
        cout << i << " - AP_tree *: " << elem <<" \n";
    }
    return;
}


void AP_tree_stack::print() {
    struct AP_tree_buffer *elem;
    unsigned long i = this->size();
    cout << "AP_tree_stack :  Size " << i << "\n";
    get_init();
    for (; i > 0; i--) {
        elem = (AP_tree_buffer *)get();
        elem->print();
    }
    return;
}

