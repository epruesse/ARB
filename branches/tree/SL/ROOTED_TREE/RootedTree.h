// ================================================================ //
//                                                                  //
//   File      : RootedTree.h                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ROOTEDTREE_H
#define ROOTEDTREE_H

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

class TreeRoot {
};

class RootedTree : public GBT_TREE {
};

#else
#error RootedTree.h included twice
#endif // ROOTEDTREE_H
