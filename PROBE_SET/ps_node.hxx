#ifndef PS_NODE_HXX
#define PS_NODE_HXX

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif
#ifndef PS_DEFS_HXX
#include "ps_defs.hxx"
#endif
#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

using namespace std;

//***********************************************
//* PS_Probe
//***********************************************
typedef struct {
           short int quality;                      // negative quality <=> matches inverse path
  unsigned short int length;
  unsigned short int GC_content;
} PS_Probe;

typedef SmartPtr<PS_Probe> PS_ProbePtr;

inline void PS_printProbe( const PS_Probe *p ) {
  printf("%+i_%u_%u",p->quality,p->length,p->GC_content);
}

inline void PS_printProbe( const PS_ProbePtr p ) {
  printf("%+i_%u_%u",p->quality,p->length,p->GC_content);
}

struct lt_probe
{
  bool operator()(const PS_ProbePtr& p1, const PS_ProbePtr& p2) const
  {
    //printf("\t");PS_printProbe(p1);printf(" < ");PS_printProbe(p2);printf(" ?\n");
    if (abs(p1->quality) == abs(p2->quality)) {
      if (p1->length == p2->length) {
        return (p1->GC_content < p2->GC_content); // equal quality & length => sort by GC-content
      } else {
        return (p1->length < p2->length);         // equal quality => sort by length
      }
    } else {
      return ((p1->quality < p2->quality));       // sort by quality
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
typedef SmartPtr<PS_Node>         PS_NodePtr;
typedef map<SpeciesID,PS_NodePtr> PS_NodeMap;
typedef PS_NodeMap::iterator         PS_NodeMapIterator;
typedef PS_NodeMap::reverse_iterator PS_NodeMapRIterator;
typedef PS_NodeMap::const_iterator         PS_NodeMapConstIterator;
typedef PS_NodeMap::const_reverse_iterator PS_NodeMapConstRIterator;

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
    bool addChild( PS_NodePtr& _child ) {
        //printf("addChild[%d]\n",child->getNum());
        PS_NodeMapIterator it = children.find(_child->getNum());
        if (it == children.end()) {
            children[_child->getNum()] = _child;
            return true;
        } else {
            printf( "child[#%u] already exists\n",_child->getNum() );
            return false;
        }
    }

    PS_NodePtr assertChild( SpeciesID _id ) {
        PS_NodeMapIterator it = children.find( _id );
        if (it == children.end()) {
            PS_NodePtr new_child(new PS_Node(_id));
            children[_id] = new_child;
            return new_child;
        } else {
            return it->second;
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

    size_t countChildren() const { return children.size(); }
    bool   hasChildren()   const { return (!children.empty()); }

    PS_NodeMapIterator       getChildrenBegin()        { return children.begin(); }
    PS_NodeMapRIterator      getChildrenRBegin()       { return children.rbegin(); }
    PS_NodeMapConstIterator  getChildrenBegin()  const { return children.begin(); }
    PS_NodeMapConstRIterator getChildrenRBegin() const { return children.rbegin(); }
    PS_NodeMapIterator       getChildrenEnd()          { return children.end(); }
    PS_NodeMapRIterator      getChildrenREnd()         { return children.rend(); }
    PS_NodeMapConstIterator  getChildrenEnd()    const { return children.end(); }
    PS_NodeMapConstRIterator getChildrenREnd()   const { return children.rend(); }

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

    void addProbes( PS_ProbeSetIterator _begin, PS_ProbeSetIterator _end ) {
        //printf( "check for probes...\n" );
        if (_begin == _end) return;
        //printf( "check for probeset...\n" );
        if (!probes) probes = new PS_ProbeSet;
	for (PS_ProbeSetIterator probe = _begin; probe != _end; ++probe) {
            //printf( "inserting new probe...\n" );
            probes->insert( *probe );
        }
    }

    void addProbesInverted( PS_ProbeSetIterator _begin, PS_ProbeSetIterator _end ) {
        //printf( "check for probes...\n" );
        if (_begin == _end) return;
        //printf( "check for probeset...\n" );
        if (!probes) probes = new PS_ProbeSet;
	for (PS_ProbeSetIterator probe = _begin; probe != _end; ++probe) {
            //printf( "making new probe...\n" );
            PS_ProbePtr new_probe(new PS_Probe);
            new_probe->length     = (*probe)->length;
            new_probe->quality    = -((*probe)->quality);
            new_probe->GC_content = (*probe)->GC_content;
            //printf( "inserting new probe...\n" );
            probes->insert( new_probe );
        }
    }

    size_t countProbes() const { return (probes == 0) ? 0 : probes->size(); }
    bool   hasProbes()   const { return (probes != 0); }
    bool   hasPositiveProbes() const {
        if (!probes) return false;
        for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
            if ((*i)->quality >= 0) return true;
        }
        return false;
    }

    bool   hasInverseProbes() const {
        if (!probes) return false;
        for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
            if ((*i)->quality < 0) return true;
        }
        return false;
    }

    PS_ProbeSetIterator getProbesBegin() {
	ps_assert(probes);
	return probes->begin();
    }
    PS_ProbeSetIterator getProbesEnd() {
	ps_assert(probes);
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
        for (PS_NodeMapIterator i=children.begin(); i!=children.end(); ++i) {
            i->second->print();
        }
	printf( "]" );
    }

    void printOnlyMe() const {
	printf( "N[%d] P[ ", num );
	if (probes) {
	    for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
		PS_printProbe(*i);
		printf(" ");
	    }
	}
	printf( "] C[ " );
        for (PS_NodeMapConstIterator i=children.begin(); i!=children.end(); ++i) {
            printf( "N[%d] P[%d] C[%d] ", i->first, i->second->countProbes(), i->second->countChildren() );
        }
	printf( "]" );
    }

    //
    // *** disk i/o ***
    //
    bool save(      PS_FileBuffer *_fb );
    bool saveASCII( PS_FileBuffer *_fb, char *buffer );
    bool load(      PS_FileBuffer *_fb );
    bool append(    PS_FileBuffer *_fb ); // load from file and append to node
    bool read(      PS_FileBuffer *_fb, PS_Callback *_call_destination ); // parse file and callback after probes read

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
    PS_Node() { ps_assert(0); }
    PS_Node(const PS_Node&); // forbidden
};

#else
#error ps_node.hxx included twice
#endif
