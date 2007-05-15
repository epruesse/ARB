//  ==================================================================== //
//                                                                       //
//    File      : SQ_ambiguities.h                                       //
//    Purpose   : Class used for evaluation of iupac ambiguities         //
//    Time-stamp: <Wed Feb/04/2004 14:34 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#include "awt_iupac.hxx"
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)


class SQ_ambiguities {
public:
    SQ_ambiguities();
    void SQ_count_ambiguities ( const char *iupac, int length, GBDATA *gb_quality );
    int SQ_get_nr_ambiguities() const;
    int SQ_get_percent_ambiguities() const;
    int SQ_get_iupac_value() const;

private:
    int number;
    int percent;
    int iupac_value;
};


inline SQ_ambiguities::SQ_ambiguities() {
    number      = 0;
    percent     = 0;
    iupac_value = 0;
}


void SQ_ambiguities::SQ_count_ambiguities ( const char *iupac, int length, GBDATA *gb_quality ) {
    char c;

    for ( int i = 0; i <length; i++ ) {
        c = iupac[i];
        switch ( c ) {
        case 'R':
            number++;
            iupac_value = iupac_value+2;
            break;
        case 'Y':
            number++;
            iupac_value = iupac_value+3;
            break;
        case 'M':
            number++;
            iupac_value = iupac_value+2;
            break;
        case 'K':
            number++;
            iupac_value = iupac_value+3;
            break;
        case 'W':
            number++;
            iupac_value = iupac_value+3;
            break;
        case 'S':
            number++;
            iupac_value = iupac_value+2;
            break;
        case 'B':
            number++;
            iupac_value = iupac_value+4;
            break;
        case 'D':
            number++;
            iupac_value = iupac_value+4;
            break;
        case 'H':
            number++;
            iupac_value = iupac_value+4;
            break;
        case 'V':
            number++;
            iupac_value = iupac_value+3;
            break;
        case 'N':
            number++;
            iupac_value = iupac_value+5;
            break;
        }
    }
    percent = ( 100 * number ) / length;

    GBDATA *gb_result1 = GB_search ( gb_quality, "iupac_value", GB_INT );
    seq_assert ( gb_result1 );
    GB_write_int ( gb_result1, iupac_value );
}


inline int SQ_ambiguities::SQ_get_nr_ambiguities() const {
    return number;
}


inline int SQ_ambiguities::SQ_get_percent_ambiguities() const {
    return percent;
}


inline int SQ_ambiguities::SQ_get_iupac_value() const {
    return iupac_value;
}
