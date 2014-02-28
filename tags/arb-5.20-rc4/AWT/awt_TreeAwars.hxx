// =============================================================== //
//                                                                 //
//   File      : awt_TreeAwars.hxx                                 //
//   Purpose   : tree awar registry                                //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2014   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AWT_TREEAWARS_HXX
#define AWT_TREEAWARS_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif

void AWT_initTreeAwarRegistry(GBDATA *gbmain);

void AWT_registerTreeAwarCallback(AW_awar *awar, const TreeAwarCallback& tacb, bool triggerIfTreeDataChanges);
void AWT_registerTreeAwarSimple(AW_awar *awar);

// the following functions should only be used by TreeAdmin!
void AWT_announce_tree_renamed(const char *oldname, const char *newname);
void AWT_announce_tree_deleted(const char *name);

#else
#error awt_TreeAwars.hxx included twice
#endif // AWT_TREEAWARS_HXX
