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
#include "ap_main.hxx"

#include <iostream>
#include <fstream>
#include <algorithm>

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
        out << space(indentLevel+1) << '[' << i << "] AP_tree_nlen*=" << node << " remembered_for_frame=" << node->last_remembered_frame() << endl;
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
        << " father=" << get_father()
        << " rff=" << remembered_for_frame
        << " mut=" << mutation_rate
#if 0
        << " dist=" << distance
        << " touched=" << br.touched
#endif
        << " seqcs=" << (hasSequence() ? get_seq()->checksum() : 0);

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

using namespace std;

template <typename SET>
void set_extract_common(SET& set1, SET& set2, SET& common)  {
    ap_assert(!set1.empty() && !set2.empty());

    set_intersection(
        set1.begin(), set1.end(),
        set2.begin(), set2.end(),
        std::inserter<SET>(common, common.begin())
        );

    for (typename SET::iterator e = common.begin(); e != common.end(); ++e) {
        set1.erase(*e);
        set2.erase(*e);
    }
}

void ResourceStack::extract_common(ResourceStack& stack1, ResourceStack& stack2) {
    if (!stack1.nodes.empty() && !stack2.nodes.empty()) {
        set_extract_common(stack1.nodes, stack2.nodes, nodes);
    }
    if (!stack1.edges.empty() && !stack2.edges.empty()) {
        set_extract_common(stack1.edges, stack2.edges, edges);
    }
}

void ResourceStack::destroy_nodes() {
    for (NodeSet::iterator n = nodes.begin(); n != nodes.end(); ++n) {
        AP_tree_nlen *todel = *n;

        // Nodes destroyed from here may link to other nodes, but all these links are outdated.
        // They are just leftovers of calling revert() or accept() -> wipe them
        todel->forget_relatives();
        todel->forget_origin();

        delete todel;
    }
    forget_nodes();
}

void ResourceStack::destroy_edges() {
    for (EdgeSet::iterator e = edges.begin(); e != edges.end(); ++e) {
        AP_tree_edge *todel = *e;

        // Edges destroyed from here may link to nodes, but all these links are outdated.
        // They are just leftovers of calling revert() or accept() -> wipe them
        todel->node[0] = NULL;
        todel->node[1] = NULL;

        delete todel;
    }
    forget_edges();
}

void ResourceStack::forget_nodes() { nodes.clear(); }
void ResourceStack::forget_edges() { edges.clear(); }

void ResourceStack::move_nodes(ResourceStack& target) {
    while (!nodes.empty()) target.put(getNode()); // @@@ optimize
}
void ResourceStack::move_edges(ResourceStack& target) {
    while (!edges.empty()) target.put(getEdge()); // @@@ optimize
}

void StackFrameData::revert_resources(StackFrameData */*previous*/) {
    // if previous==NULL, top StackFrameData is reverted
    created.destroy_nodes();
    destroyed.forget_nodes();

    created.destroy_edges();
    destroyed.forget_edges();
}

void StackFrameData::accept_resources(StackFrameData *previous, ResourceStack *common) {
    // if previous==NULL, top StackFrameData gets destroyed
    ap_assert(correlated(!previous, !common));

    if (previous) {
        if (common->has_nodes()) common->destroy_nodes();
        created.move_nodes(previous->created);
        destroyed.move_nodes(previous->destroyed);
    }
    else {
        created.forget_nodes(); // they are finally accepted as part of the tree
        destroyed.destroy_nodes();
    }

    if (previous) {
        if (common->has_edges()) common->destroy_edges();
        created.move_edges(previous->created);
        destroyed.move_edges(previous->destroyed);
    }
    else {
        created.forget_edges(); // they are finally accepted as part of the tree
        destroyed.destroy_edges();
    }
}

