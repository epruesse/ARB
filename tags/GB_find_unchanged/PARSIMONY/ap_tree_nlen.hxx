#include <iostream>
#include <limits.h>

#define ap_assert(x) arb_assert(x)

class       AP_tree_nlen;
extern long global_combineCount;

struct AP_CO_LIST {	// liste fuer crossover
	GBDATA *leaf0;
	GBDATA *leaf1;
	int     node0;
	int     node1;

	class AP_tree_nlen *pntr;
};

typedef enum {	// flags fuer die Rekursionsart bei Kernighan Lin
    AP_DYNAMIK       = 1,
    AP_STATIC        = 2,
    AP_BETTER        = 4,
    // Funktionstyp der Schwellwertfunktion
    AP_QUADRAT_START = 5,
    AP_QUADRAT_MAX   = 6
} AP_KL_FLAG;

typedef enum {
	APBL_NONE                = 0,
	AP_BL_NNI_ONLY           = 1, // try te find a better tree only
	AP_BL_BL_ONLY            = 2, // try to calculate the branch lengths
	AP_BL_NNI_BL             = 3, // better tree & branch lengths
	AP_BL_BOOTSTRAP_LIMIT    = 4, // calculate upper bootstrap limits
	AP_BL_BOOTSTRAP_ESTIMATE = 12 // calculate estimate of bootstrap
} AP_BL_MODE;

class AP_tree_edge;

class AP_tree_nlen : public AP_tree {		/* tree that is independent of branch lengths and root */
	AP_tree	*dup(void);

	AP_BOOL	clear(unsigned long stack_update,unsigned long user_push_counter);
	void 	unhash_sequence(void);
	void	createListRekUp(AP_CO_LIST *list,int *cn);
	void	createListRekSide(AP_CO_LIST *list,int *cn);


	AP_TREE_SIDE kernighan;	    // Flag zum markieren
    int			 distance; 		// distance to tree border (0=leaf, INT_MAX=UNKNOWN)

    // definitions for AP_tree_edge:

    AP_tree_edge *edge[3];	    // the edges to the father and the sons
    int			  index[3]; 	// index to node[] in AP_tree_edge

protected:

public:
	AP_FLOAT	            costs(void); /* cost of a tree (number of changes ..)*/
	AP_BOOL		            push(AP_STACK_MODE, unsigned long); /* push state of costs */
	void		            pop(unsigned long); /* pop old tree costs */
    virtual AP_UPDATE_FLAGS check_update(); // disable  load !!!!


	AP_tree_nlen(AP_tree_root *tree_root);
	virtual ~AP_tree_nlen() {}

	void copy(AP_tree_nlen *tree);
    int	 Distance();

    // tree reconstruction methods:
	GB_ERROR insert(AP_tree *new_brother);
	GB_ERROR remove(void);
	GB_ERROR swap_assymetric(AP_TREE_SIDE modus);
	GB_ERROR move(AP_tree *new_brother,AP_FLOAT rel_pos);
	GB_ERROR set_root();

    // tree optimization methods:
	void parsimony_rek();

    AP_FLOAT nn_interchange_rek(AP_BOOL     openclosestatus,
                                int        &abort,
                                int         deep, // -1 means: do whole subtree
                                AP_BL_MODE  mode        = AP_BL_NNI_ONLY,
                                GB_BOOL	    skip_hidden = GB_FALSE);

	AP_FLOAT nn_interchange(AP_FLOAT parsimony,AP_BL_MODE mode);

	void 		kernighan_rek(int         rek_deep,
                              int        *rek_breite,
                              int         rek_breite_anz,
                              const int   rek_deep_max,
                              double (*funktion)(double,double *,int),
                              double     *param_liste,
                              int         param_anz,
                              AP_FLOAT    pars_best,
                              AP_FLOAT    pars_start,
                              AP_FLOAT    pars_prev,
                              AP_KL_FLAG  searchflag,
                              AP_BOOL    *abbruch_flag);

	// for crossover creates a list of 3 times the nodes with all
	// ancestors in it
	AP_CO_LIST * createList(int *size);

	class AP_tree_stack	stack;			/* tree stack */

    // misc stuff:

    void setBranchlen(double leftLen,double rightLen) { leftlen = leftLen; rightlen = rightLen; }

    int		test() const;
    const char*	fullname() const;

    const char*	sortByName();

    // casted access to neighbours:

    AP_tree_nlen *Father() const { return (AP_tree_nlen*)father; }
    AP_tree_nlen *Brother() const { return (AP_tree_nlen*)brother(); }
    AP_tree_nlen *Leftson() const { return (AP_tree_nlen*)leftson; }
    AP_tree_nlen *Rightson() const { return (AP_tree_nlen*)rightson; }

    // AP_tree_edge access functions:

    int	indexOf(const AP_tree_edge *e) const { int i; for (i=0; i<3; i++) if (edge[i]==e) return i;return -1; }

    AP_tree_edge* edgeTo(const AP_tree_nlen *brother) const;
    AP_tree_edge* nextEdge(const AP_tree_edge *thisEdge=NULL) const;
    int	unusedEdge() const;

    // more complex edge functions:

    void linkAllEdges(AP_tree_edge *edge1, AP_tree_edge *edge2, AP_tree_edge *edge3);
    void unlinkAllEdges(AP_tree_edge **edgePtr1, AP_tree_edge **edgePtr2, AP_tree_edge **edgePtr3);

    //void 	destroyAllEdges();

    // test stuff:

    char *getSequence();

    friend 	class AP_tree_edge;
    friend 	std::ostream& operator<<(std::ostream&,const AP_tree_nlen&);
};


/************ AP_tree_edge  ************/
class AP_tree_edge
{
    // the following members are stored/restored by
    // AP_tree_nlen::push/pop:

    AP_tree_nlen 	  *node[2];	// the two nodes of this edge
    int			       index[2]; // index to edge[] in AP_tree_nlen
    AP_tree_edge_data  data; 	// data stored in edge (defined in AP_buffer.hxx)

    // and these arent pushed:

    AP_tree_edge *next; 		// temporary next pointer used by some methods
    int           used; 		// test-counter
    long          age; 			// age of the edge

    static long timeStamp;		// static counter for edge-age
    // recursive methods:
    //
    //  deep:	determines how deep we go into the tree
    //		-1 means we go through the whole tree
    //		 0 means we only act on the actual edge

    //    int clearValues(int deep,AP_tree_nlen *skip=NULL);	// clears used and data.distance
    AP_tree_edge *buildChain(int deep,
                             GB_BOOL skip_hidden = GB_FALSE,
                             int distance=0,
                             const AP_tree_nlen *skip=NULL,
                             AP_tree_edge *comesFrom=NULL);
    long sizeofChain(void);
    void calcDistance();
    void tailDistance(AP_tree_nlen*);
    int distanceOK() const { int diff = node[0]->distance-node[1]->distance; return diff>=-1 && diff<=1; }

    // my friends:

    friend class AP_tree_nlen;
    friend std::ostream& operator<<(std::ostream&,const AP_tree_edge&);

protected:

    ~AP_tree_edge();

    // relink & unlink (they are also used by constructor & destructor)

    void relink(AP_tree_nlen *node1, AP_tree_nlen *node2);
    AP_tree_edge *unlink();

public:

    AP_tree_edge(AP_tree_nlen *node1, AP_tree_nlen *node2);

    static void initialize(AP_tree_nlen *root);

    // access methods:

    int isConnectedTo(const AP_tree_nlen *n) const 		{ return node[0]==n || node[1]==n; }
    int	indexOf(const AP_tree_nlen *n) const 			{ ap_assert(isConnectedTo(n)); return node[1] == n; }
    AP_tree_nlen* otherNode(const AP_tree_nlen *n) const 	{ return node[1-indexOf(n)]; }
    AP_tree_nlen* sonNode() const				{ return node[0]->Father() == node[1] ? node[0] : node[1]; }
    AP_tree_edge* Next() const 					{ return next; }
    long Age() const						{ return age; }

    // encapsulated AP_tree_nlen methods:

    GB_ERROR set_root()						{ return sonNode()->set_root(); }

    // tree optimization:

    AP_FLOAT nni_rek(AP_BOOL       openclosestatus,
                     int&          Abort,
                     int           deep,
                     GB_BOOL       skip_hidden = GB_FALSE,
                     AP_BL_MODE    mode        = AP_BL_NNI_ONLY,
                     AP_tree_nlen *skipNode    = NULL);

    AP_FLOAT nni(AP_FLOAT pars_one, AP_BL_MODE mode);

    // test methods:

    void mixTree(int cnt);
    int test() const;
    int dumpChain() const;
    void testChain(int deep);

    int Distance() const { ap_assert(distanceOK()); return (node[0]->distance+node[1]->distance) >> 1; }
    int distanceToBorder(int maxsearch=INT_MAX,AP_tree_nlen *skip=NULL) const;	// obsolete

    static int dumpNNI;		    // should NNI dump its values?
    static int distInsertBorder; // distance from insert pos to tree border
    static int basesChanged; 	// no of bases which were changed
    // in fathers sequence because of insertion

    void countSpecies(int deep=-1,const AP_tree_nlen* skip=NULL);

    static int speciesInTree;	// no of species (leafs) in tree
    static int nodesInTree; 	// no of nodes in tree
};

std::ostream& operator<<(std::ostream&, const AP_tree_edge&);

/******** two easy-access-functions for the root *********/

inline AP_tree_nlen *rootNode() {
    return ((AP_tree_nlen*)*ap_main->tree_root);
}

inline AP_tree_edge *rootEdge() {
    return rootNode()->Leftson()->edgeTo(rootNode()->Rightson());
}

/**************************************************/
