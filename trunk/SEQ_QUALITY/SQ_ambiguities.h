//  ==================================================================== //
//                                                                       //
//    File      : SQ_ambiguities.h                                       //
//    Purpose   : Class used for evaluation of iupac ambiguities         //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#define seq_assert(bed) arb_assert(bed)

class SQ_ambiguities {
    int number;
    int percent;
    int iupac_value;

public:
    SQ_ambiguities()
        : number(0)
        , percent(0)
        , iupac_value(0)
    {}

    void SQ_count_ambiguities(const char *iupac, int length, GBDATA * gb_quality);
    
    int SQ_get_nr_ambiguities() const      { return number; }
    int SQ_get_percent_ambiguities() const { return percent; }
    int SQ_get_iupac_value() const         { return iupac_value; }
};

