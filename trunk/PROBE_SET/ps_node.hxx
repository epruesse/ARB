#ifndef PS_NODE_HXX
#define PS_NODE_HXX

#include <smartptr.h>
#include <unistd.h>
#include <set>
#include <map>
#include "../PROBE_GROUP/pg_search.hxx"

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

void PS_printProbe( PS_Probe *p ) {
  printf("%u_%u_%u",p->quality,p->length,p->GC_content);
}

void PS_printProbe( PS_ProbePtr p ) {
  printf("%u_%u_%u",p->quality,p->length,p->GC_content);
}

struct lt_probe
{
  bool operator()(PS_ProbePtr p1, PS_ProbePtr p2) const
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
    if (it != children.end()) {
      children[child->getNum()] = child;
      return true;
    } else {
      //printf( "child[#%u] already exists\n",child->getNum() );
      return false;
    }
  }

  PS_NodePtr getChild( SpeciesID id ) { 
    PS_NodeMapIterator it = children.find(id); 
    if (it!=children.end()) return it->second;
    return 0;
  }

  const PS_NodePtr getChild( SpeciesID id ) const { 
    PS_NodeMapConstIterator it = children.find(id); 
    if (it!=children.end()) return it->second;
    return 0;
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
      //if (!p.second) { printf( "probe " ); PS_printProbe(probe); printf( " already exists\n" ); }
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
  // *** disk output **
  //
  bool save( int fd ) {
    unsigned int size;
    ssize_t written;
    //
    // write num
    //
    written = write( fd, &num, sizeof(num) );
    if (written != sizeof(num)) return false;
    //
    // write probes
    //
    size = (probes == 0) ? 0 : probes->size();
    written = write( fd, &size, sizeof(size) );
    if (written != sizeof(size)) return false;
    if (size) {
      for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
	written = write( fd, (PS_Probe *)&(*i), sizeof(PS_Probe) );
	if (written != sizeof(PS_Probe)) return false;
      }
    }
    //
    // write children
    //
    size = children.size();
    written = write( fd, &size, sizeof(size) );
    if (written != sizeof(size)) return false;
    for (PS_NodeMapIterator i=children.begin(); i!=children.end(); ++i) {
      if (!i->second->save( fd )) return false;
    }
    //
    // return true to signal success
    //
    return true;
  }

  //
  // *** disk input **
  //
  bool load( int fd ) {
    unsigned int size;
    ssize_t readen;
    //
    // read num
    //
    readen = read( fd, &num, sizeof(num) );
    if (readen != sizeof(num)) return false;
    //
    // read probes
    //
    readen = read( fd, &size, sizeof(size) );
    if (readen != sizeof(size)) return false;
    if (size) {                                               // does node have probes ?
      probes = new PS_ProbeSet;                               // make new probeset
      for (unsigned int i=0; i<size; ++i) {
	PS_ProbePtr new_probe(new PS_Probe);                  // make new probe
	readen = read( fd, &(*new_probe), sizeof(PS_Probe) ); // read new probe
	if (readen != sizeof(PS_Probe)) return false;         // check for read error
	probes->insert( new_probe );                          // append new probe to probeset
      }
    } else {
      probes = 0;                                             // unset probeset
    }
    //
    // read children
    //
    readen = read( fd, &size, sizeof(size) );
    if (readen != sizeof(size)) return false;
    for (unsigned int i=0; i<size; ++i) {
      PS_NodePtr new_child(new PS_Node(-1));                  // make new child
      if (!new_child->load( fd )) return false;               // read new child
      children[new_child->getNum()] = new_child;              // insert new child to childmap
    }
    // return true to signal success
    return true;
  }

  //
  // *** constructors ***
  //
  PS_Node( SpeciesID id ) { num = id; probes = 0; } //printf(" constructor PS_Node() num=%d\n",num); }

  //
  // *** destructor ***
  //
  ~PS_Node() {
    //printf("destroying node #%d\n",num);
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
