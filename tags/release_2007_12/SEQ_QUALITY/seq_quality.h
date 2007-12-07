//  ==================================================================== //
//                                                                       //
//    File      : SQ_quality.h                                           //
//    Time-stamp: <Thu Feb/05/2004 11:40 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#ifndef SEQ_QUALITY_H
#define SEQ_QUALITY_H


void SQ_create_awars(AW_root * awr, AW_default aw_def);
AW_window *SQ_create_seq_quality_window(AW_root * aw_root, AW_CL);


#else
#error seq_quality.h included twice
#endif                          // SEQ_QUALITY_H
