// =============================================================== //
//                                                                 //
//   File      : AP_buffer.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_buffer.hxx"
#include "ap_tree_nlen.hxx"

#include <iostream>

using namespace std;

// ----------------
//      AP_LIST

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

int AP_LIST::is_element(void * node) {
    if (element(node) == 0) return 0;
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
    }
    else {
        last->next = newelem;
        newelem->prev = last;
        last = newelem;
        newelem->next = 0;
    }
    newelem->node = new_one;
    list_len++;
    return;
}

bool AP_LIST::remove(void * object) {
    AP_list_elem  *elem = element(object);
    if (elem) {
        if (elem->prev) {
            elem->prev->next = elem->next;
        }
        else {
            first = elem->next;
            elem->next->prev = 0;
        }
        if (elem->next) {
            elem->next->prev = elem->prev;
        }
        else {
            last = elem->prev;
            elem->prev->next = 0;
        }
        if (elem == pointer) pointer = 0;
        delete elem;
        list_len --;

        return true;
    }
    return false;
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

void *AP_LIST::pop() {
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

#if defined(PROVIDE_PRINT)
void AP_tree_buffer::print() {
    cout  << "AP_tree_buffer                      " << this;
    cout  << "\nfather " << father;
    cout  << "\nlefts  " << leftson;
    cout  << "\nrights " << rightson << "\n sequence " << sequence << "\n";
}

void AP_main_stack::print() {
    unsigned long i = this->size();
    cout << "AP_main_stack " << this << "  Size " << i << "\n";
    for (AP_main_stack::iterator e = begin(); e != end(); ++e, --i) {
        AP_tree *elem = *e;
        cout << i << " - AP_tree *: " << elem << " \n";
    }
}


void AP_tree_stack::print() {
    unsigned long i = this->size();
    cout << "AP_tree_stack :  Size " << i << "\n";
    for (AP_tree_stack::iterator e = begin(); e != end(); ++e) {
        (*e)->print();
    }
}
#endif
