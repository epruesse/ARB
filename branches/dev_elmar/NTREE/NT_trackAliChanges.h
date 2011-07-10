// =============================================================== //
//                                                                 //
//   File      : NT_trackAliChanges.h                              //
//   Purpose   : Track alignment and sequence changes              //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_TRACKALICHANGES_H
#define NT_TRACKALICHANGES_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

class AW_window;

void       NT_create_trackAliChanges_Awars(AW_root *root, AW_default aw_def);
AW_window *NT_create_trackAliChanges_window(AW_root *root);

#else
#error NT_trackAliChanges.h included twice
#endif // NT_TRACKALICHANGES_H
