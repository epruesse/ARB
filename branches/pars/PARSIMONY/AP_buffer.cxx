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
void NodeState::print() const {
    cout  << "NodeState                      " << this;
    cout  << "\nfather " << father;
    cout  << "\nlefts  " << leftson;
    cout  << "\nrights " << rightson << "\n sequence " << sequence << "\n";
}

void NodeStack::print() const {
    unsigned long i = count_elements();
    cout << "NodeStack " << this << "  Size " << i << "\n";
    for (NodeStack::const_iterator e = begin(); e != end(); ++e, --i) {
        const AP_tree *elem = *e;
        cout << i << " - AP_tree *: " << elem << " \n";
    }
}

void StateStack::print() const {
    unsigned long i = count_elements();
    cout << "StateStack :  Size " << i << "\n";
    for (StateStack::const_iterator e = begin(); e != end(); ++e) {
        (*e)->print();
    }
}
#endif
