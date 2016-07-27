// =============================================================== //
//                                                                 //
//   File      : ap_tree_nlen.hxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in Summer 1995    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_TREE_NLEN_HXX
#define AP_TREE_NLEN_HXX

#ifndef _GLIBCXX_IOSTREAM
#include <iostream>
#endif
#ifndef _GLIBCXX_CLIMITS
#include <climits>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AP_TREE_HXX
#include <AP_Tree.hxx>
#endif
#ifndef AP_BUFFER_HXX
#include "AP_buffer.hxx"
#endif
#ifndef AP_MAIN_TYPE_HXX
#include "ap_main_type.hxx"
#endif
#ifndef PARS_AWARS_H
#include "pars_awars.h"
#endif

class AP_tree_nlen;

enum KL_RECURSION_TYPE { // =path reduction
    // bit-values!
    AP_NO_REDUCTION = 0,
    AP_DYNAMIK      = 1,
    AP_STATIC       = 2,
};

class QuadraticThreshold {
    double a, b, c;
public:
    QuadraticThreshold() {}
    QuadraticThreshold(KL_DYNAMIC_THRESHOLD_TYPE type, double startx, double maxy, double maxx, double maxDepth, AP_FLOAT pars_start);

    double calculate(double x) const {
        // y = ax^2 + bx + c
        return
            x * x * a +
            x * b +
            c;
    }

    void change_parsimony_start(double offset)  {
        c += offset;
    }
};

struct KL_params {
    int                max_rec_depth;
    KL_RECURSION_TYPE  rec_type;                 // recursion type
    int                rec_width[CUSTOM_DEPTHS]; // customized recursion width (static path reduction)
    QuadraticThreshold thresFunctor;             // functor for dynamic path reduction (start parsvalue -> worst allowed parsvalue @ depth)
    int                inc_rec_depth;            // increment max. recursion depth (if better topology found)
    bool               stopAtFoldedGroups;       // if true = > do not swap across folded groups
};

enum AP_BL_MODE {
    APBL_NONE                = 0,
    AP_BL_NNI_ONLY           = 1, // try te find a better tree only
    AP_BL_BL_ONLY            = 2, // try to calculate the branch lengths
    AP_BL_BOOTSTRAP_LIMIT    = 4, // calculate upper bootstrap limits
    AP_BL_BOOTSTRAP_ESTIMATE = 12 // calculate estimate of bootstrap (includes AP_BL_BOOTSTRAP_LIMIT)
};

enum AP_TREE_SIDE {
    AP_LEFT,
    AP_RIGHT,
};

class  AP_tree_edge;
class  AP_main;

class AP_pars_root : public AP_tree_root {
    // @@@ add responsibility for node/edge resources
    bool has_been_saved;
public:
    AP_pars_root(AliView *aliView, AP_sequence *seq_proto, bool add_delete_callbacks)
        : AP_tree_root(aliView, seq_proto, add_delete_callbacks),
          has_been_saved(false)
    {}
    inline TreeNode *makeNode() const OVERRIDE;
    inline void destroyNode(TreeNode *node) const OVERRIDE;
    GB_ERROR saveToDB() OVERRIDE;

    bool was_saved() const { return has_been_saved; }
};

typedef uint8_t EdgeIndex;

class AP_tree_nlen : public AP_tree { // derived from a Noncopyable // @@@ rename -> AP_pars_tree? (later!)
    // tree that is independent of branch lengths and root

    // definitions for AP_tree_edge:
    AP_tree_edge *edge[3];      // the edges to the father and the sons
    EdgeIndex     index[3];     // index to node[] in AP_tree_edge

    AP_FLOAT mutation_rate;

    Level      remembered_for_frame; // last frame this nodes' state has been remembered, 0 = none
    StateStack states;               // containes previous states of 'this'

    void forget_relatives() { AP_tree::forget_relatives(); }

    void restore_structure(const NodeState& state);
    void restore_sequence(NodeState& state);
    void restore_sequence_nondestructive(const NodeState& state);
    void restore_root(const NodeState& state);

    void buildNodeList_rec(AP_tree_nlen **list, long& num);
    void buildNodeList(AP_tree_nlen **&list, long &num); // returns a list of inner nodes (w/o root)
    void buildBranchList_rec(AP_tree_nlen **list, long& num, bool create_terminal_branches, int deep);

protected:
    ~AP_tree_nlen() OVERRIDE { ap_assert(states.empty()); }
    friend void AP_main::destroyNode(AP_tree_nlen *node); // allowed to call dtor
    friend void ResourceStack::destroy_nodes();
    friend void ResourceStack::destroy_edges();
    friend void StackFrameData::destroyNode(AP_tree_nlen *node);
public:
    explicit AP_tree_nlen(AP_pars_root *troot)
        : AP_tree(troot),
          mutation_rate(0),
          remembered_for_frame(0)
    {
        edge[0]  = edge[1]  = edge[2]  = NULL;
        index[0] = index[1] = index[2] = 0;
    }

    DEFINE_TREE_ACCESSORS(AP_pars_root, AP_tree_nlen);

    void     unhash_sequence();
    AP_FLOAT costs(char *mutPerSite = NULL);        // cost of a tree (number of changes ..)

    bool rememberState(AP_STACK_MODE, Level frame_level);
    void revertToPreviousState(Level curr_frameLevel, bool& rootPopped);
    void restore(NodeState& state);
    void restore_nondestructive(const NodeState& state);
    bool acceptCurrentState(Level frame_level);

    Level last_remembered_frame() const { return remembered_for_frame; }
    bool  may_be_recollected() const { return !states.empty(); }
    const StateStack& get_states() const { return states; }

    virtual AP_UPDATE_FLAGS check_update() OVERRIDE; // disable  load !!!!

    bool recalc_marked_from_sons() {
        // return true if changed
        if (is_leaf) return false;
        unsigned marked_childs = get_leftson()->gr.mark_sum + get_rightson()->gr.mark_sum;
        if (marked_childs == gr.mark_sum) return false;
        gr.mark_sum = marked_childs;
        return true;
    }

    void recalc_marked_from_sons_and_forward_upwards() {
        if (recalc_marked_from_sons() && father) {
            get_father()->recalc_marked_from_sons_and_forward_upwards();
        }
    }

    void copy(AP_tree_nlen *tree);

    // tree reconstruction methods:
    void insert(AP_tree_nlen *new_brother);
    void initial_insert(AP_tree_nlen *new_brother, AP_pars_root *troot);

    AP_tree_nlen *REMOVE() OVERRIDE;
    void swap_sons() OVERRIDE;
    void swap_assymetric(AP_TREE_SIDE mode);
    void moveNextTo(AP_tree_nlen *new_brother, AP_FLOAT rel_pos); // if unsure, use cantMoveNextTo to test if possible
    void set_root() OVERRIDE;

    // overload virtual methods from AP_tree:
    void insert(AP_tree *new_brother) OVERRIDE { insert(DOWNCAST(AP_tree_nlen*, new_brother)); }
    void initial_insert(AP_tree *new_brother, AP_tree_root *troot) OVERRIDE { initial_insert(DOWNCAST(AP_tree_nlen*, new_brother), DOWNCAST(AP_pars_root*, troot)); }
    void moveNextTo(AP_tree *node, AP_FLOAT rel_pos) OVERRIDE { moveNextTo(DOWNCAST(AP_tree_nlen*, node), rel_pos); }

    // tree optimization methods:
    void parsimony_rec(char *mutPerSite = NULL);

    AP_FLOAT nn_interchange_rec(EdgeSpec whichEdges, AP_BL_MODE mode);
    AP_FLOAT nn_interchange(AP_FLOAT parsimony, AP_BL_MODE mode);

    void buildBranchList(AP_tree_nlen **&list, long &num, bool create_terminal_branches, int deep);

    // misc stuff:

    void setBranchlen(double leftLen, double rightLen) { leftlen = leftLen; rightlen = rightLen; }

    const char* fullname() const;
    const char* sortByName();

    // AP_tree_edge access functions:

    int indexOf(const AP_tree_edge *e) const { int i; for (i=0; i<3; i++) if (edge[i]==e) return i; return -1; }

    AP_tree_edge* edgeTo(const AP_tree_nlen *brother) const;
    AP_tree_edge* nextEdge(const AP_tree_edge *afterThatEdge=NULL) const;
    int unusedEdgeIndex() const; // [0..2], -1 if none

    // more complex edge functions:

    void linkAllEdges(AP_tree_edge *edge1, AP_tree_edge *edge2, AP_tree_edge *edge3);
    void unlinkAllEdges(AP_tree_edge **edgePtr1, AP_tree_edge **edgePtr2, AP_tree_edge **edgePtr3);

    char *getSequenceCopy();

#if defined(PROVIDE_TREE_STRUCTURE_TESTS)
    Validity has_valid_edges() const;
    Validity sequence_state_valid() const;
    Validity is_valid() const;
#endif // PROVIDE_TREE_STRUCTURE_TESTS

#if defined(PROVIDE_PRINT)
    void print(std::ostream& out, int indentLevel, const char *label) const;
#endif

#if defined(UNIT_TESTS)
    void remember_subtree(AP_main *main) { // dont use in production
        if (is_leaf) {
            main->push_node(this, STRUCTURE);
        }
        else {
            main->push_node(this, BOTH);
            get_leftson()->remember_subtree(main);
            get_rightson()->remember_subtree(main);
        }
    }
#endif


    friend      class AP_tree_edge;
    friend      std::ostream& operator<<(std::ostream&, const AP_tree_nlen*);
};

#if defined(ASSERTION_USED) || defined(UNIT_TESTS)
bool allBranchlengthsAreDefined(AP_tree_nlen *tree);
#endif

// ---------------------
//      AP_tree_edge

class MutationsPerSite;

class AP_tree_edge : virtual Noncopyable {
    // the following members are stored/restored by
    // AP_tree_nlen::push/pop:

    AP_tree_nlen *node[2];      // the two nodes of this edge
    EdgeIndex     index[2];     // index to edge[] in AP_tree_nlen

    // and these arent pushed:

    AP_tree_edge *next_in_chain;        // do not use (use EdgeChain!)
    int           used;                 // test-counter
    long          age;                  // age of the edge
    bool          kl_visited;

    static long timeStamp;              // static counter for edge-age

    size_t buildChainInternal(EdgeSpec whichEdges, bool depthFirst, const AP_tree_nlen *skip, AP_tree_edge**& prevNextPtr);

    bool is_linked() const { return node[0]; }

    void set_inner_branch_length_and_calc_adj_leaf_lengths(AP_FLOAT bcosts);

    // my friends:
    friend class AP_tree_nlen;
    friend class EdgeChain;

    friend std::ostream& operator<<(std::ostream&, const AP_tree_edge*);
    friend AP_tree_edge *StackFrameData::makeEdge(AP_tree_nlen *n1, AP_tree_nlen *n2); // allowed to relink edge
    friend void          AP_main::destroyEdge(AP_tree_edge *edge);                     // allowed to delete edge
    friend void          ResourceStack::destroy_edges();                               // allowed to delete edge
#if defined(UNIT_TESTS) // UT_DIFF
    friend void TEST_basic_tree_modifications();
#endif

protected:

    ~AP_tree_edge();

    // relink & unlink (they are also used by constructor & destructor)

    void relink(AP_tree_nlen *node1, AP_tree_nlen *node2);
    AP_tree_edge *unlink();

public:

    AP_tree_edge(AP_tree_nlen *node1, AP_tree_nlen *node2);

    static void initialize(AP_tree_nlen *root);
    static void destroy(AP_tree_nlen *root);

    // access methods:

    int isConnectedTo(const AP_tree_nlen *n) const              { return node[0]==n || node[1]==n; }
    int indexOf(const AP_tree_nlen *n) const                    { ap_assert(isConnectedTo(n)); return node[1] == n; }
    AP_tree_nlen* otherNode(const AP_tree_nlen *n) const        { return node[1-indexOf(n)]; }
    AP_tree_nlen* sonNode() const                               { return node[0]->get_father() == node[1] ? node[0] : node[1]; }
    AP_tree_nlen* notSonNode() const                            { return otherNode(sonNode()); }

    long Age() const { return age; }

    // queries

    bool is_root_edge() const { return node[0]->father != node[1] && node[1]->father != node[0]; }
    bool is_leaf_edge() const { return node[0]->is_leaf || node[1]->is_leaf; }
    bool next_to_folded_group() const { return node[0]->gr.grouped || node[1]->gr.grouped; }
    bool has_marked() const { // true if subtree contains marked species
        return is_root_edge()
            ? node[0]->gr.mark_sum+node[1]->gr.mark_sum
            : sonNode()->gr.mark_sum;
    }

    // encapsulated AP_tree_nlen methods:

    void set_root() { return sonNode()->set_root(); }

    // tree optimization:

    AP_FLOAT nni_rec(EdgeSpec whichEdges, AP_BL_MODE mode, AP_tree_nlen *skipNode, bool includeStartEdge);
    bool kl_rec(const KL_params& KL, const int rec_depth, AP_FLOAT pars_best);

    AP_FLOAT calc_branchlengths() { return nni_rec(ANY_EDGE, AP_BL_BL_ONLY, NULL, true); }

    AP_FLOAT nni_mutPerSite(AP_FLOAT pars_one, AP_BL_MODE mode, MutationsPerSite *mps);

    void mixTree(int repeat, int percent, EdgeSpec whichEdges);

    void set_visited(bool vis) { kl_visited = vis; }
};

std::ostream& operator<<(std::ostream&, const AP_tree_edge*);

class EdgeChain : virtual Noncopyable {
    AP_tree_edge *start;  // start edge
    AP_tree_edge *curr;   // current edge (for iteration)
    size_t        len;    // chain size
    static bool   exists; // singleton flag
public:
    EdgeChain(AP_tree_edge *start_, EdgeSpec whichEdges, bool depthFirst, const AP_tree_nlen *skip = NULL, bool includeStart = true);
    ~EdgeChain() { exists = false; }

    size_t size() const  {
        return len;
    }
    operator bool() const {
        return curr;
    }
    AP_tree_edge *operator*() {
        return curr;
    }
    const EdgeChain& operator++() {
        ap_assert(curr);
        curr = curr->next_in_chain;
        return *this;
    }
    void restart() {
        curr = start;
    }
};

#else
#error ap_tree_nlen.hxx included twice
#endif // AP_TREE_NLEN_HXX
