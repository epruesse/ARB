//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.h                                         //
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
#define SQ_GROUPDATA_H

#ifndef _CPP_CSTDDEF
#include <cstddef>
#endif

class SQ_GroupData {

public:
    SQ_GroupData();
    //SQ_GroupData(const SQ_GroupData& g1, const SQ_GroupData& g2) {}
    void SQ_init_consensus(int size);
    void SQ_add_consensus(int value, int row, int col);
    int **SQ_get_consensus();
    int SQ_print_on_screen();
    bool SQ_is_initialised();

private: // !
    int    size;
    int    **consensus;
    bool   initialised;
};

#else
#error SQ_GroupData.h included twice
#endif
