#ifndef PS_NODE_HXX
#define PS_NODE_HXX

#include <smartptr.h>
#include <set>
#include <map>
#include "../PROBE_GROUP/pg_search.hxx"

#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

using namespace std;

//***********************************************
//* PS_Probe
//***********************************************
typedef struct {
  unsigned short int quality;
  unsigned short int length;
  unsigned short int GC_content;
} PS_Probe;

typedef SmartPtr<PS_Probe> PS_ProbePtr;

inline void PS_printProbe( const PS_Probe *p ) {
  printf("%u_%u_%u",p->quality,p->length,p->GC_content);
}

inline void PS_printProbe( const PS_ProbePtr p ) {
  printf("%u_%u_%u",p->quality,p->length,p->GC_content);
}

struct lt_probe
{
  bool operator()(const PS_ProbePtr& p1, const PS_ProbePtr& p2) const
  {
    //printf("\t");PS_printProbe(p1);printf(" < ");PS_printProbe(p2);printf(" ?\n");
    if (p1->quality == p2->quality) {
      if (p1->length == p2->length) {
        return (p1->GC_content < p2->GC_content); // equal quality & length => sort by GC-content
      } else {
        return (p1->length < p2->length);         // equal quality => sort by length
      }
    } else {
      return (p1->quality < p2->quality);         // sort by quality
    }
  }
};



//***********************************************
//* PS_ProbeSet
//***********************************************
typedef set<PS_ProbePtr,lt_probe>   PS_ProbeSet;
typedef PS_ProbeSet*                PS_ProbeSetPtr;
typedef PS_ProbeSet::const_iterator PS_ProbeSetIterator;


//***********************************************
//* PS_Node
//***********************************************
class PS_Node;
typedef SmartPtr<PS_Node>          PS_NodePtr;
typedef map<SpeciesID,PS_NodePtr>  PS_NodeMap;
typedef PS_NodeMap::iterator       PS_NodeMapIterator;
typedef PS_NodeMap::const_iterator PS_NodeMapConstIterator;

class PS_Node {
private:

    SpeciesID      num;
    PS_NodeMap     children;
    PS_ProbeSetPtr probes;

public:

    //
    // *** num ***
    //
    void      setNum( SpeciesID id ) { num = id;   }
    SpeciesID getNum() const         { return num; }

    //
    // *** children ***
    //
    bool addChild( PS_NodePtr& child ) {
        //printf("addChild[%d]\n",child->getNum());
        PS_NodeMapIterator it = children.find(child->getNum());
        if (it == children.end()) {
            children[child->getNum()] = child;
            return true;
        } else {
            printf( "child[#%u] already exists\n",child->getNum() );
            return false;
        }
    }

    pair<bool,PS_NodePtr> getChild( SpeciesID id ) {
	PS_NodeMapIterator it = children.find(id);
	return pair<bool,PS_NodePtr>(it!=children.end(),it->second);
    }

    pair<bool,const PS_NodePtr> getChild( SpeciesID id ) const {
	PS_NodeMapConstIterator it = children.find(id);
	return pair<bool,const PS_NodePtr>(it!=children.end(),it->second);
    }

    bool   hasChildren()   const { return (!children.empty()); }
    size_t countChildren() const { return children.size(); }

    PS_NodeMapIterator      getChildrenBegin()       { return children.begin(); }
    PS_NodeMapConstIterator getChildrenBegin() const { return children.begin(); }
    PS_NodeMapIterator      getChildrenEnd()         { return children.end(); }
    PS_NodeMapConstIterator getChildrenEnd()   const { return children.end(); }

    //
    // *** probes ***
    //
    bool addProbe( const PS_ProbePtr& probe ) {
	//printf("addProbe(");PS_printProbe(probe);printf(")\n");
	if (!probes) {
	    //printf( "creating new ProbeSet at node #%u (%p)\n",num,this );
	    probes = new PS_ProbeSet;
	    probes->insert(probe);
	    return true;
	} else {
	    pair<PS_ProbeSetIterator,bool> p = probes->insert(probe);
// 	    if (!p.second) {
// 		printf( "probe " ); PS_printProbe(probe);
// 		printf( " already exists\n" );
// 	    }

	    return p.second;
	}
    }

    bool   hasProbes()   const { return (probes != 0); }
    size_t countProbes() const { return probes->size(); }

    PS_ProbeSetIterator getProbesBegin() {
	pg_assert(probes);
	return probes->begin();
    }
    PS_ProbeSetIterator getProbesEnd() {
	pg_assert(probes);
	return probes->end();
    }

    //
    // *** output **
    //
    void print() {
	printf( "\nN[%d] P[ ", num );
	if (probes) {
	    for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
		PS_printProbe(*i);
		printf(" ");
	    }
	}
	printf( "] C[" );
	for (PS_NodeMap::iterator i=children.begin(); i!=children.end(); ++i) {
	    i->second->print();
	}
	printf( "]" );
    }

    //
    // *** disk i/o ***
    //
    bool save( PS_FileBuffer* _fb );
    bool saveASCII( PS_FileBuffer* _fb, char *buffer );
    bool load( PS_FileBuffer* _fb );
    bool append( PS_FileBuffer* _fb ); // load from file and append to node

    //
    // *** constructors ***
    //
    PS_Node( SpeciesID id ) { num = id; probes = 0; } //printf(" constructor PS_Node() num=%d (%d)\n",num,int(this)); }

    //
    // *** destructor ***
    //
    ~PS_Node() {
	//printf("destroying node #%d (%d)\n",num,int(this));
	if (probes) delete probes;
	children.clear();
    }

private:
    PS_Node() { pg_assert(0); }
    PS_Node(const PS_Node&); // forbidden
};

#else
#error ps_node.hxx included twice
#endif
