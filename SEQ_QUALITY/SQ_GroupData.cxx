//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.cxx                                       //
//    Purpose   : Classes to store global information about sequences    //
//    Time-stamp: <Wed Sep/22/2004 19:02 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include <cstdio>
#include <cctype>
#include "SQ_GroupData.h"

using namespace std;

SQ_GroupData::SQ_GroupData()
{
    size         = 0;
    avg_bases    = 0;
    nr_sequences = 0;
    gc_prop      = 0;
    initialized  = false;
}


SQ_GroupData::~SQ_GroupData() { }


double SQ_GroupData_RNA::SQ_calc_consensus_deviation ( const char *sequence ) const
{
    double deviation = 0;
    // int current[6];

    for ( int i = 0; i < size; i++ )
    {
        int current[6] = { 0, 0, 0, 0, 0, 0};

        //fill up current with decoded iupac values
        switch ( sequence[i] )
        {
            case 'a':
            case 'A':
                current[0] = current[0] + 100;
                break;
            case 't':
            case 'T':
                current[1] = current[1] + 100;
                break;
            case 'c':
            case 'C':
                current[2] = current[2] + 100;
                break;
            case 'g':
            case 'G':
                current[3] = current[3] + 100;
                break;
            case 'u':
            case 'U':
                current[1] = current[1] + 100;
                break;
            case 'r':
            case 'R':
                current[0] = current[0] + 50;
                current[3] = current[3] + 50;
                break;
            case 'y':
            case 'Y':
                current[2] = current[2] + 50;
                current[1] = current[1] + 50;
                break;
            case 'm':
            case 'M':
                current[0] = current[0] + 50;
                current[2] = current[2] + 50;
                break;
            case 'k':
            case 'K':
                current[3] = current[3] + 50;
                current[1] = current[1] + 50;
                break;
            case 'w':
            case 'W':
                current[0] = current[0] + 50;
                current[1] = current[1] + 50;
                break;
            case 's':
            case 'S':
                current[3] = current[3] + 50;
                current[2] = current[2] + 50;
                break;
            case 'b':
            case 'B':
                current[2] = current[2] + 33;
                current[1] = current[1] + 33;
                current[3] = current[3] + 33;
                break;
            case 'd':
            case 'D':
                current[0] = current[0] + 33;
                current[1] = current[1] + 33;
                current[3] = current[3] + 33;
                break;
            case 'h':
            case 'H':
                current[2] = current[2] + 33;
                current[1] = current[1] + 33;
                current[0] = current[0] + 33;
                break;
            case 'v':
            case 'V':
                current[0] = current[0] + 33;
                current[2] = current[2] + 33;
                current[3] = current[3] + 33;
                break;
            case 'n':
            case 'N':
            case 'x':
            case 'X':
                current[2] = current[2] + 25;
                current[1] = current[1] + 25;
                current[0] = current[0] + 25;
                current[3] = current[3] + 25;
                break;
            case '.':
                current[4] = current[4] + 1;
                break;
            case '-':
                current[5] = current[5] + 1;
                break;
            default :
                seq_assert ( 0 ); // unhandled character
                break;

        }//end fill up current

        for ( int j = 0; j < 6; j++ )
        {
            int currentj  = current[j];
            if ( ( currentj > 0 ) && ( ( consensus[i].i[j] - currentj ) <= 0 ) )
            {
                deviation += currentj;
            }
        }
    }
    deviation = deviation / size;  //set deviation in relation to sequencelength and percent
    return deviation;
}


double SQ_GroupData_RNA::SQ_calc_consensus_conformity ( const char *sequence ) const
{
    double value = 0;
    // int current[6];

    for ( int i = 0; i < size; i++ )
    {
        int current[6] = { 0, 0, 0, 0, 0, 0};

        //fill up current with decoded iupac values
        switch ( sequence[i] )
        {
            case 'a':
            case 'A':
                current[0] = current[0] + 100;
                break;
            case 't':
            case 'T':
                current[1] = current[1] + 100;
                break;
            case 'c':
            case 'C':
                current[2] = current[2] + 100;
                break;
            case 'g':
            case 'G':
                current[3] = current[3] + 100;
                break;
            case 'u':
            case 'U':
                current[1] = current[1] + 100;
                break;
            case 'r':
            case 'R':
                current[0] = current[0] + 50;
                current[3] = current[3] + 50;
                break;
            case 'y':
            case 'Y':
                current[2] = current[2] + 50;
                current[1] = current[1] + 50;
                break;
            case 'm':
            case 'M':
                current[0] = current[0] + 50;
                current[2] = current[2] + 50;
                break;
            case 'k':
            case 'K':
                current[3] = current[3] + 50;
                current[1] = current[1] + 50;
                break;
            case 'w':
            case 'W':
                current[0] = current[0] + 50;
                current[1] = current[1] + 50;
                break;
            case 's':
            case 'S':
                current[3] = current[3] + 50;
                current[2] = current[2] + 50;
                break;
            case 'b':
            case 'B':
                current[2] = current[2] + 33;
                current[1] = current[1] + 33;
                current[3] = current[3] + 33;
                break;
            case 'd':
            case 'D':
                current[0] = current[0] + 33;
                current[1] = current[1] + 33;
                current[3] = current[3] + 33;
                break;
            case 'h':
            case 'H':
                current[2] = current[2] + 33;
                current[1] = current[1] + 33;
                current[0] = current[0] + 33;
                break;
            case 'v':
            case 'V':
                current[0] = current[0] + 33;
                current[2] = current[2] + 33;
                current[3] = current[3] + 33;
                break;
            case 'n':
            case 'N':
            case 'x':
            case 'X':
                current[2] = current[2] + 25;
                current[1] = current[1] + 25;
                current[0] = current[0] + 25;
                current[3] = current[3] + 25;
                break;
            case '.':
                current[4] = current[4] + 1;
                break;
            case '-':
                current[5] = current[5] + 1;
                break;
            default :
                seq_assert ( 0 ); // unhandled character
                break;

        }//end fill up current

        int* cs = consensus[i].i;
        int sum = cs[0]+cs[1]+cs[2]+cs[3]+cs[4]+cs[5]; // some work for the compiler optimization...

        for ( int j = 0; j < 6; j++ )
        {
            int currentj  = current[j];
            if ( ( currentj > 0 ) && ( ( cs[j] - currentj ) > 0 ) )
            {
                value += ( cs[j] - currentj ) / sum;
            }
        }
    }

    value = value / size;  //set conformity in relation to sequencelength
    return value;
}


void SQ_GroupData_RNA::SQ_add_sequence ( const char *sequence )
{

    for ( int i=0; i < size; i++ )
    {
        int* cs = consensus[i].i;

        switch ( sequence[i] )
        {
            case 'a':
            case 'A':
                cs[0] = cs[0] + 100;
                break;
            case 't':
            case 'T':
                cs[1] = cs[1] + 100;
                break;
            case 'c':
            case 'C':
                cs[2] = cs[2] + 100;
                break;
            case 'g':
            case 'G':
                cs[3] = cs[3] + 100;
                break;
            case 'u':
            case 'U':
                cs[1] = cs[1] + 100;
                break;
            case 'r':
            case 'R':
                cs[0] = cs[0] + 50;
                cs[3] = cs[3] + 50;
                break;
            case 'y':
            case 'Y':
                cs[2] = cs[2] + 50;
                cs[1] = cs[1] + 50;
                break;
            case 'm':
            case 'M':
                cs[0] = cs[0] + 50;
                cs[2] = cs[2] + 50;
                break;
            case 'k':
            case 'K':
                cs[3] = cs[3] + 50;
                cs[1] = cs[1] + 50;
                break;
            case 'w':
            case 'W':
                cs[0] = cs[0] + 50;
                cs[1] = cs[1] + 50;
                break;
            case 's':
            case 'S':
                cs[3] = cs[3] + 50;
                cs[2] = cs[2] + 50;
                break;
            case 'b':
            case 'B':
                cs[2] = cs[2] + 33;
                cs[1] = cs[1] + 33;
                cs[3] = cs[3] + 33;
                break;
            case 'd':
            case 'D':
                cs[0] = cs[0] + 33;
                cs[1] = cs[1] + 33;
                cs[3] = cs[3] + 33;
                break;
            case 'h':
            case 'H':
                cs[2] = cs[2] + 33;
                cs[1] = cs[1] + 33;
                cs[0] = cs[0] + 33;
                break;
            case 'v':
            case 'V':
                cs[0] = cs[0] + 33;
                cs[2] = cs[2] + 33;
                cs[3] = cs[3] + 33;
                break;
            case 'n':
            case 'N':
            case 'x':
            case 'X':
                cs[2] = cs[2] + 25;
                cs[1] = cs[1] + 25;
                cs[0] = cs[0] + 25;
                cs[3] = cs[3] + 25;
                break;
            case '.':
                cs[4] = cs[4] + 1;
                break;
            case '-':
                cs[5] = cs[5] + 1;
                break;
            default :
                fprintf ( stderr, "Illegal character '%c'", sequence[i] );
                seq_assert ( 0 ); // unhandled character
                break;
        }
    }
}


// double SQ_GroupData_PRO::SQ_calc_consensus_deviation ( const char *sequence ) const
double SQ_GroupData_PRO::SQ_calc_consensus_deviation ( const char * ) const
{
#warning implementation missing
    return 0; // dummy return value
}

// double SQ_GroupData_PRO::SQ_calc_consensus_conformity ( const char *sequence ) const
double SQ_GroupData_PRO::SQ_calc_consensus_conformity ( const char * ) const
{
#warning implementation missing
    return 0; // dummy return value
}

// void SQ_GroupData_PRO::SQ_add_sequence ( const char *sequence )
void SQ_GroupData_PRO::SQ_add_sequence ( const char * )
{
#warning implementation missing
}
