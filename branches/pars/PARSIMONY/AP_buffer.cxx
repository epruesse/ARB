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


inline string space(int count) {
    return string(count, ' ');
}

#if defined(PROVIDE_PRINT)
void NodeState::print(int indentLevel) const {
    cout  << space(indentLevel) << "NodeState=" << this << endl
          << space(indentLevel+1) << "father=" << father << " lson=" << leftson << " rson=" << rightson << endl
          << space(indentLevel+1) << "sequence=" << sequence << endl
          << space(indentLevel+1) << "frameNr=" << frameNr << " mode=" << mode << endl;
}

void NodeStack::print(int indentLevel) const {
    unsigned long i = count_elements();
    cout << space(indentLevel) << "NodeStack=" << this << "  size " << i << endl;
    for (NodeStack::const_iterator e = begin(); e != end(); ++e, --i) {
        const AP_tree_nlen *node = *e;
        cout << space(indentLevel+1) << '[' << i << "] AP_tree_nlen*=" << node << endl;
        node->get_states().print(indentLevel+2);
    }
}

void StateStack::print(int indentLevel) const {
    unsigned long i = count_elements();
    cout << space(indentLevel) << "StateStack=" << this << " size " << i << endl;
    for (StateStack::const_iterator e = begin(); e != end(); ++e) {
        (*e)->print(indentLevel+1);
    }
}

void FrameStack::print(int indentLevel) const {
    unsigned long i = count_elements();
    cout << space(indentLevel) << "FrameStack=" << this << " size " << i << endl;
    for (FrameStack::const_iterator e = begin(); e != end(); ++e) {
        (*e)->print(indentLevel+1);
    }
}

#endif

