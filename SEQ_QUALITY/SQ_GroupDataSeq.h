//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupDataSeq.h                                         //
//    Purpose   : We will see!                                           //
//    Time-stamp: <Mon Sep/29/2003 19:10 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //
#ifndef SQ_GROUPDATA_H
#include "SQ_GroupData.h"
#endif

#ifndef SQ_GROUPDATASEQ_H
#define SQ_GROUPDATASEQ_H

#ifndef _CPP_CSTDDEF
#include <cstddef>
#endif

struct Int7 { 
  int i[7]; 
  Int7() {
    for (int j=0; j<7; ++j) i[j]=0; 
  }
};

class SQ_GroupDataSeq : public SQ_GroupData {

public:
    SQ_GroupDataSeq();
    ~SQ_GroupDataSeq();
    //SQ_GroupData(const SQ_GroupData& g1, const SQ_GroupData& g2) {}
    void SQ_init_consensus(int size);
    void SQ_add_consensus(int value, int row, int col);
    Int7 *SQ_get_consensus();
    int  SQ_print_on_screen();
    double SQ_test_against_consensus(const char *sequence);

private:
    int    size;
    int    avg_bases;
    Int7   *consensus;
//    int**  consensus;
    bool   initialised;
};

#else
#error SQ_GroupDataSeq.h included twice
#endif
