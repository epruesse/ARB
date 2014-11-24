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
#include "ap_main.hxx"

#include <iostream>
#include <fstream>

using namespace std;

#if defined(PROVIDE_PRINT)

inline string space(int count) {
    return string(count, ' ');
}

void NodeState::print(ostream& out, int indentLevel) const {
    out  << space(indentLevel)
         << "NodeState=" << this
         << " frameNr=" << frameNr << " mode=" << mode;
    if (mode & STRUCTURE) {
        out << " father=" << father << " lson=" << leftson << " rson=" << rightson;
        out << " edges={";
        for (int e = 0; e<3; ++e) {
            out << " e[" << e << "]=" << edge[e] << "[" << edgeIndex[e] << "]";
        }
        out << " }";
    }
    if (mode & SEQUENCE) {
        out << " sequence=" << sequence;
    }
    out << endl;
}

void NodeStack::print(ostream& out, int indentLevel, Level frameNr) const {
    size_t i = count_elements();
    out << space(indentLevel) << "NodeStack=" << this << "  size " << i << " frameNr=" << frameNr << endl;
    for (NodeStack::const_iterator e = begin(); e != end(); ++e, --i) {
        const AP_tree_nlen *node = *e;
        out << space(indentLevel+1) << '[' << i << "] AP_tree_nlen*=" << node << " pushed_to_frame=" << node->get_pushed_to_frame() << endl;
        node->get_states().print(out, indentLevel+2);
    }
}

void StateStack::print(ostream& out, int indentLevel) const {
    size_t i = count_elements();
    out << space(indentLevel) << "StateStack=" << this << " size " << i << endl;
    for (StateStack::const_iterator e = begin(); e != end(); ++e) {
        const NodeState& state = **e;
        state.print(out, indentLevel+1);
    }
}

void FrameStack::print(ostream& out, int indentLevel) const {
    size_t i = count_elements();
    out << space(indentLevel) << "FrameStack=" << this << " size " << i << endl;

    Level frameNr = i;
    for (FrameStack::const_iterator e = begin(); e != end(); ++e, --frameNr) {
        const NodeStack& nodeStack = **e;
        nodeStack.print(out, indentLevel+1, frameNr);
    }
}


void AP_tree_nlen::print(std::ostream& out, int indentLevel, const char *label) const {
    out << space(indentLevel)
        << label << "=" << this
        << " father=" << get_father();
    for (int e = 0; e<3; ++e) {
        out << " edge[" << e << "]=" << edge[e];
        if (edge[e]) {
            const AP_tree_edge& E = *edge[e];
            if (E.isConnectedTo(this)) {
                out << "->" << E.otherNode(this);
            }
            else {
                out << "(not connected to 'this'!";
                AP_tree_nlen *son = E.sonNode();
                if (son) {
                    AP_tree_nlen *fath = E.otherNode(son);
                    out << " son=" << son << " father=" << fath;
                }
                else {
                    out << "no son node";
                }

                out << ')';
            }
        }
    }

    if (is_leaf) {
        out << " name=" << name << endl;
    }
    else {
        out << endl;
        get_leftson()->print(out, indentLevel+1, "left");
        get_rightson()->print(out, indentLevel+1, "right");
    }
}


void AP_main::print(ostream& out) {
    out << "AP_main tree:" << endl;
    get_root_node()->print(out, 1, "root");

    out << "AP_main frames:" << endl;
    if (currFrame) {
        out << " currFrame:" << endl;
        currFrame->print(out, 2, frames.count_elements()+1);
    }
    else {
        out << " no currFrame" << endl;
    }
    frames.print(out, 1);
}

void AP_main::print2file(const char *file_in_ARBHOME) {
    const char    *full = GB_path_in_ARBHOME(file_in_ARBHOME);
    std::ofstream  out(full, std::ofstream::out);
    print(out);
    out.close();
}

#endif

