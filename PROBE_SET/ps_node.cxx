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

using namespace std;

  bool PS_Node::load( int fd ) {
      unsigned int size;
      ssize_t      readen;
      //
      // read num
      //
      readen = read( fd, &num, sizeof(num) );

      if (readen != sizeof(num)) return false;
      //
      // read probes
      //
      readen         = read( fd, &size, sizeof(size) );
      if (readen != sizeof(size)) return false;
      if (size) {               // does node have probes ?
          probes     = new PS_ProbeSet; // make new probeset
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



