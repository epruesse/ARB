//  =============================================================== //
//                                                                  //
//    File      : ps_node.cxx                                       //
//    Purpose   :                                                   //
//    Time-stamp: <Thu Jun/13/2002 12:36 MET Coder@ReallySoft.de>   //
//                                                                  //
//    Coded by Ralf Westram (coder@reallysoft.de) in June 2002      //
//    Institute of Microbiology (Technical University Munich)       //
//    http://www.arb-home.de/                                       //
//                                                                  //
//  =============================================================== //

#include "ps_node.hxx"
#include <unistd.h>

using namespace std;


//
// *** disk output **
//
bool PS_Node::save( PS_FileBuffer* _fb ) {
    unsigned int size;
    //
    // write num
    //
    _fb->put( &num,sizeof(num) );
    //
    // write probes
    //
    size = (probes == 0) ? 0 : probes->size();
    _fb->put( &size, sizeof(size) );
    if (size) {
	for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
	    _fb->put( (PS_Probe *)&(*i), sizeof(PS_Probe) );
	}
    }
    //
    // write children
    //
    size = children.size();
    _fb->put( &size, sizeof(size) );
    for (PS_NodeMapIterator i=children.begin(); i!=children.end(); ++i) {
	i->second->save( _fb );
    }
    //
    // return true to signal success
    //
    return true;
}


//
// *** disk input **
//
bool PS_Node::load( PS_FileBuffer* _fb ) {
    unsigned int size;
    //
    // read num
    //
    _fb->get( &num, sizeof(num) );
    //
    // read probes
    //
    _fb->get( &size, sizeof(size) );
    if (size) {               // does node have probes ?
	probes = new PS_ProbeSet;                                 // make new probeset
	for (unsigned int i=0; i<size; ++i) {
	    PS_ProbePtr new_probe(new PS_Probe);                  // make new probe
	    _fb->get( &(*new_probe), sizeof(PS_Probe) );          // read new probe
	    probes->insert( new_probe );                          // append new probe to probeset
	}
    } else {
	probes = 0;                                               // unset probeset
    }
    //
    // read children
    //
    _fb->get( &size, sizeof(size) );
    for (unsigned int i=0; i<size; ++i) {
	PS_NodePtr new_child(new PS_Node(-1));                    // make new child
	new_child->load( _fb );                                   // read new child
	children[new_child->getNum()] = new_child;                // insert new child to childmap
    }
    // return true to signal success
    return true;
}


//
// *** disk input appending **
//
bool PS_Node::append( PS_FileBuffer* _fb ) {
    unsigned int size;
    //
    // read num if root
    //
    if (num == -1) {
        _fb->get( &num, sizeof(num) );
    }
    //
    // read probes
    //
    _fb->get( &size, sizeof(size) );
    if (size) {               // does node have probes ?
	probes = new PS_ProbeSet;                                 // make new probeset
	for (unsigned int i=0; i<size; ++i) {
	    PS_ProbePtr new_probe(new PS_Probe);                  // make new probe
	    _fb->get( &(*new_probe), sizeof(PS_Probe) );          // read new probe
	    probes->insert( new_probe );                          // append new probe to probeset
	}
    }
    //
    // read children
    //
    _fb->get( &size, sizeof(size) );
    for (unsigned int i=0; i<size; ++i) {
        //
        // read num of child
        //
        SpeciesID childNum;
        _fb->get( &childNum, sizeof(childNum) );
        //
        // test if child already exists
        //
        PS_NodeMapIterator it = children.find( childNum );
        if (it != children.end()) {
            it->second->append( _fb );                           // 'update' child
        } else {
            PS_NodePtr newChild(new PS_Node(childNum));          // make new child
            newChild->append( _fb );                             // read new child
            children[childNum] = newChild;                       // insert new child to childmap
        }
    }
    // return true to signal success
    return true;
}
