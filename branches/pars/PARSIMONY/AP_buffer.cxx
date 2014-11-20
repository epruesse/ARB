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

#if defined(PROVIDE_PRINT)
void NodeState::print() {
    cout  << "NodeState                      " << this;
    cout  << "\nfather " << father;
    cout  << "\nlefts  " << leftson;
    cout  << "\nrights " << rightson << "\n sequence " << sequence << "\n";
}

void NodeStack::print() {
    unsigned long i = this->size();
    cout << "NodeStack " << this << "  Size " << i << "\n";
    for (NodeStack::iterator e = begin(); e != end(); ++e, --i) {
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
