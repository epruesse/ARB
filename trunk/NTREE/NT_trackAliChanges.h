// =============================================================== //
//                                                                 //
//   File      : NT_trackAliChanges.h                              //
//   Purpose   : Track alignment and sequence changes              //
//   Time-stamp: <Fri Jun/29/2007 17:31 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_TRACKALICHANGES_H
#define NT_TRACKALICHANGES_H

void       NT_create_trackAliChanges_Awars(AW_root *root, AW_default aw_def);
AW_window *NT_create_trackAliChanges_window(AW_root *root);

#else
#error NT_trackAliChanges.h included twice
#endif // NT_TRACKALICHANGES_H
