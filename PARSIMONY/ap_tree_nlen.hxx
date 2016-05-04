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
#ifndef AP_MAIN_HXX
#include <ap_main.hxx>
#endif

class AP_tree_nlen;

struct AP_CO_LIST {     // liste fuer crossover
    GBDATA *leaf0;
    GBDATA *leaf1;
    int     node0;
    int     node1;

    class AP_tree_nlen *pntr;
};

enum AP_KL_FLAG {  // flags fuer die Rekursionsart bei Kernighan Lin
    AP_DYNAMIK       = 1,
    AP_STATIC        = 2,
    AP_BETTER        = 4,
    // Funktionstyp der Schwellwertfunktion
    AP_QUADRAT_START = 5,
    AP_QUADRAT_MAX   = 6
};

enum AP_BL_MODE {
    APBL_NONE                = 0,
    AP_BL_NNI_ONLY           = 1, // try te find a better tree only
    AP_BL_BL_ONLY            = 2, // try to calculate the branch lengths
    AP_BL_NNI_BL             = 3, // better tree & branch lengths
    AP_BL_BOOTSTRAP_LIMIT    = 4, // calculate upper bootstrap limits
    AP_BL_BOOTSTRAP_ESTIMATE = 12 // calculate estimate of bootstrap (includes AP_BL_BOOTSTRAP_LIMIT)
};

class AP_tree_edge;

class AP_tree_nlen : public AP_tree { // derived from a Noncopyable
    // tree that is independent of branch lengths and root

    AP_TREE_SIDE kernighan;     // Flag zum markieren
    int          distance;      // distance to tree border (0=leaf, INT_MAX=UNKNOWN)

    // definitions for AP_tree_edge:

    AP_tree_edge *edge[3];      // the edges to the father and the sons
    int           index[3];     // index to node[] in AP_tree_edge

    AP_FLOAT mutation_rate;



    void createListRekUp(AP_CO_LIST *list, int *cn);
    void createListRekSide(AP_CO_LIST *list, int *cn);

public:
    explicit AP_tree_nlen(AP_tree_root *troot)
        : AP_tree(troot),
          kernighan(AP_NONE),
          distance(INT_MAX),
          mutation_rate(0)
    {
        edge[0]  = edge[1]  = edge[2]  = NULL;
        index[0] = index[1] = index[2] = 0;
    }
    ~AP_tree_nlen() OVERRIDE {}

    DEFINE_TREE_ACCESSORS(AP_tree_root, AP_tree_nlen);

    void     unhash_sequence();
    AP_FLOAT costs(char *mutPerSite = NULL);        // cost of a tree (number of changes ..)

    bool push(AP_STACK_MODE, unsigned long); // push state of costs
    void pop(unsigned long);    // pop old tree costs
    bool clear(unsigned long stack_update, unsigned long user_push_counter);

    virtual AP_UPDATE_FLAGS check_update() OVERRIDE; // disable  load !!!!

    void copy(AP_tree_nlen *tree);
    int  Distance();

    // tree reconstruction methods:
    void insert(AP_tree_nlen *new_brother);
    void initial_insert(AP_tree_nlen *new_brother, AP_tree_root *troot);

    void remove() OVERRIDE;
    void swap_sons() OVERRIDE;
    void swap_assymetric(AP_TREE_SIDE mode) OVERRIDE;
    void moveNextTo(AP_tree_nlen *new_brother, AP_FLOAT rel_pos); // if unsure, use cantMoveNextTo to test if possible
    void set_root() OVERRIDE;

    // overload virtual methods from AP_tree:
    void insert(AP_tree *new_brother) OVERRIDE { insert(DOWNCAST(AP_tree_nlen*, new_brother)); }
    void initial_insert(AP_tree *new_brother, AP_tree_root *troot) OVERRIDE { initial_insert(DOWNCAST(AP_tree_nlen*, new_brother), troot); }
    void moveNextTo(AP_tree *node, AP_FLOAT rel_pos) OVERRIDE { moveNextTo(DOWNCAST(AP_tree_nlen*, node), rel_pos); }

    // tree optimization methods:
    void parsimony_rek(char *mutPerSite = NULL);

    AP_FLOAT nn_interchange_rek(int         deep,   // -1 means: do whole subtree
                                AP_BL_MODE  mode,
                                bool        skip_hidden);

    AP_FLOAT nn_interchange(AP_FLOAT parsimony, AP_BL_MODE mode);

    void kernighan_rek(int         rek_deep,
                       int        *rek_breite,
                       int         rek_breite_anz,
                       const int   rek_deep_max,
                       double    (*function)(double, double *, int),
                       double     *param_liste,
                       int         param_anz,
                       AP_FLOAT    pars_best,
                       AP_FLOAT    pars_start,
                       AP_FLOAT    pars_prev,
                       AP_KL_FLAG  searchflag,
                       bool       *abort_flag);

    // for crossover creates a list of 3 times the nodes with all
    // ancestors in it
    AP_CO_LIST * createList(int *size);

    class AP_tree_stack stack;  // tree stack

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
    void assert_edges_valid() const;
    void assert_valid() const;
#endif // PROVIDE_TREE_STRUCTURE_TESTS


    friend      class AP_tree_edge;
    friend      std::ostream& operator<<(std::ostream&, const AP_tree_nlen&);
};

struct AP_TreeNlenNodeFactory : public RootedTreeNodeFactory {
    RootedTree *makeNode(TreeRoot *root) const OVERRIDE {
        return new AP_tree_nlen(DOWNCAST(AP_tree_root*, root));
    }
};

// ---------------------
//      AP_tree_edge

class MutationsPerSite;

class AP_tree_edge : virtual Noncopyable {
    // the following members are stored/restored by
    // AP_tree_nlen::push/pop:

    AP_tree_nlen      *node[2]; // the two nodes of this edge
    int                index[2]; // index to edge[] in AP_tree_nlen
    AP_tree_edge_data  data;    // data stored in edge (defined in AP_buffer.hxx)

    // and these arent pushed:

    AP_tree_edge *next;                 // temporary next pointer used by some methods
    int           used;                 // test-counter
    long          age;                  // age of the edge

    static long timeStamp;              // static counter for edge-age
    // recursive methods:
    //
    //  deep:   determines how deep we go into the tree
    //          -1 means we go through the whole tree
    //           0 means we only act on the actual edge

    AP_tree_edge *buildChain(int                 deep,
                             bool                skip_hidden = false,
                             int                 distance    = 0,
                             const AP_tree_nlen *skip        = NULL,
                             AP_tree_edge       *comesFrom   = NULL);

    long sizeofChain();
    void calcDistance();
    void tailDistance(AP_tree_nlen*);

    int distanceOK() const { int diff = node[0]->distance-node[1]->distance; return diff>=-1 && diff<=1; }

    // my friends:

    friend class AP_tree_nlen;
    friend std::ostream& operator<<(std::ostream&, const AP_tree_edge&);
#if defined(UNIT_TESTS) // UT_DIFF
    friend void TEST_tree_modifications();
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
    AP_tree_edge* Next() const                                  { return next; }
    long Age() const                                            { return age; }

    // encapsulated AP_tree_nlen methods:

    void set_root()                                             { return sonNode()->set_root(); }

    // tree optimization:

    AP_FLOAT nni_rek(int deep, bool skip_hidden, AP_BL_MODE mode, AP_tree_nlen *skipNode);

    AP_FLOAT calc_branchlengths() { return nni_rek(-1, false, AP_BL_BL_ONLY, NULL); }

    AP_FLOAT nni_mutPerSite(AP_FLOAT pars_one, AP_BL_MODE mode, MutationsPerSite *mps);
    AP_FLOAT nni(AP_FLOAT pars_one, AP_BL_MODE mode) { return nni_mutPerSite(pars_one, mode, NULL); }

    // test methods:

    void mixTree(int cnt);
    int test() const;
    int dumpChain() const;
    void testChain(int deep);

    int Distance() const { ap_assert(distanceOK()); return (node[0]->distance+node[1]->distance) >> 1; }
    int distanceToBorder(int maxsearch=INT_MAX, AP_tree_nlen *skipNode=NULL) const; // obsolete

    static int dumpNNI;             // should NNI dump its values?
    static int distInsertBorder; // distance from insert pos to tree border
    static int basesChanged;    // no of bases which were changed
    // in fathers sequence because of insertion

    void countSpecies(int deep=-1, const AP_tree_nlen* skip=NULL);

    static int speciesInTree;                       // no of species (leafs) in tree (updated by countSpecies)
    static int nodesInTree;                         // no of nodes in tree - including leafs, but w/o rootnode (updated by countSpecies)

    static int edgesInTree() { return nodesInTree-1; } // (updated by countSpecies)
};

std::ostream& operator<<(std::ostream&, const AP_tree_edge&);


inline AP_tree_nlen *rootNode() {
    return ap_main->get_root_node();
}

inline AP_tree_edge *rootEdge() {
    return rootNode()->get_leftson()->edgeTo(rootNode()->get_rightson());
}

#else
#error ap_tree_nlen.hxx included twice
#endif // AP_TREE_NLEN_HXX
