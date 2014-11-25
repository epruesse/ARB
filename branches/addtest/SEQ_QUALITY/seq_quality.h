//  ==================================================================== //
//                                                                       //
//    File      : SQ_quality.h                                           //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef SEQ_QUALITY_H
#define SEQ_QUALITY_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

void SQ_create_awars(AW_root * awr, AW_default aw_def);
AW_window *SQ_create_seq_quality_window(AW_root *aw_root, GBDATA *gb_main);

#else
#error seq_quality.h included twice
#endif                          // SEQ_QUALITY_H
