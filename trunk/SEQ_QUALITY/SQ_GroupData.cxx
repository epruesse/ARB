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


SQ_GroupData_RNA::SQ_GroupData_RNA()
{
    SQ_init_iupacmatrix();
}


int **SQ_GroupData_RNA::m_iupacmatrix = NULL;
int *SQ_GroupData_RNA::m_iupacsum =   NULL;

// TODO: Counter, wann m_iupacmatrix im Destruktor freigegeben werden kann!?

void SQ_GroupData_RNA::SQ_init_iupacmatrix()
{
    if ( m_iupacmatrix )
    {
        return;
    }

    m_iupacmatrix = ( int** ) malloc ( 256*sizeof ( int* ) );

    for ( int i= 0; i < 256; ++i )
    {
        m_iupacmatrix[i] = ( int* ) malloc ( 6*sizeof ( int ) );
        memset ( m_iupacmatrix[i], 0, 6*sizeof ( int ) );
    }

    m_iupacsum = ( int* ) malloc ( 256*sizeof ( int ) );
    memset ( m_iupacsum, 0, 256*sizeof ( int ) );

    // populate matrix
    //     m_iupacmatrix[0] <==> 'A'
    //     m_iupacmatrix[1] <==> 'T','U'
    //     m_iupacmatrix[2] <==> 'C'
    //     m_iupacmatrix[3] <==> 'G'
    //     m_iupacmatrix[4] <==> '.'
    //     m_iupacmatrix[5] <==> '-'

    //     case 'a':
    //     case 'A':
    m_iupacmatrix['a'][0] = 100;
    m_iupacmatrix['A'][0] = 100;

    m_iupacsum['a'] = 100;
    m_iupacsum['A'] = 100;

    //     case 't':
    //     case 'T':
    m_iupacmatrix['t'][1] = 100;
    m_iupacmatrix['T'][1] = 100;

    m_iupacsum['t'] = 100;
    m_iupacsum['T'] = 100;

    //     case 'c':
    //     case 'C':
    m_iupacmatrix['c'][2] = 100;
    m_iupacmatrix['C'][2] = 100;

    m_iupacsum['c'] = 100;
    m_iupacsum['C'] = 100;

    //     case 'g':
    //     case 'G':
    m_iupacmatrix['g'][3] = 100;
    m_iupacmatrix['G'][3] = 100;

    m_iupacsum['g'] = 100;
    m_iupacsum['G'] = 100;

    //     case 'u':
    //     case 'U':
    m_iupacmatrix['u'][1] = 100;
    m_iupacmatrix['U'][1] = 100;

    m_iupacsum['u'] = 100;
    m_iupacsum['U'] = 100;

    //     case 'r':
    //     case 'R':
    m_iupacmatrix['r'][0] = 50;
    m_iupacmatrix['R'][0] = 50;
    m_iupacmatrix['r'][1] = 50;
    m_iupacmatrix['R'][1] = 50;

    m_iupacsum['r'] = 100;
    m_iupacsum['R'] = 100;

    //     case 'y':
    //     case 'Y':
    m_iupacmatrix['y'][2] = 50;
    m_iupacmatrix['Y'][2] = 50;
    m_iupacmatrix['y'][3] = 50;
    m_iupacmatrix['Y'][3] = 50;

    m_iupacsum['y'] = 100;
    m_iupacsum['Y'] = 100;

    //     case 'm':
    //     case 'M':
    m_iupacmatrix['m'][0] = 50;
    m_iupacmatrix['m'][0] = 50;
    m_iupacmatrix['M'][3] = 50;
    m_iupacmatrix['M'][3] = 50;

    m_iupacsum['m'] = 100;
    m_iupacsum['M'] = 100;

    //     case 'k':
    //     case 'K':
    m_iupacmatrix['k'][1] = 50;
    m_iupacmatrix['k'][1] = 50;
    m_iupacmatrix['K'][2] = 50;
    m_iupacmatrix['K'][2] = 50;

    m_iupacsum['k'] = 100;
    m_iupacsum['K'] = 100;

    //     case 'w':
    //     case 'W':
    m_iupacmatrix['w'][0] = 50;
    m_iupacmatrix['w'][0] = 50;
    m_iupacmatrix['W'][2] = 50;
    m_iupacmatrix['W'][2] = 50;

    m_iupacsum['w'] = 100;
    m_iupacsum['W'] = 100;

    //     case 's':
    //     case 'S':
    m_iupacmatrix['s'][1] = 50;
    m_iupacmatrix['s'][1] = 50;
    m_iupacmatrix['S'][3] = 50;
    m_iupacmatrix['S'][3] = 50;

    m_iupacsum['s'] = 100;
    m_iupacsum['S'] = 100;

    //     case 'b':
    //     case 'B':
    m_iupacmatrix['b'][1] = 33;
    m_iupacmatrix['b'][2] = 33;
    m_iupacmatrix['b'][3] = 33;
    m_iupacmatrix['B'][1] = 33;
    m_iupacmatrix['B'][2] = 33;
    m_iupacmatrix['B'][3] = 33;

    m_iupacsum['b'] = 99;
    m_iupacsum['B'] = 99;

    //     case 'd':
    //     case 'D':
    m_iupacmatrix['d'][0] = 33;
    m_iupacmatrix['d'][1] = 33;
    m_iupacmatrix['d'][2] = 33;
    m_iupacmatrix['D'][0] = 33;
    m_iupacmatrix['D'][1] = 33;
    m_iupacmatrix['D'][2] = 33;

    m_iupacsum['d'] = 99;
    m_iupacsum['D'] = 99;

    //     case 'h':
    //     case 'H':
    m_iupacmatrix['h'][0] = 33;
    m_iupacmatrix['h'][2] = 33;
    m_iupacmatrix['h'][3] = 33;
    m_iupacmatrix['H'][0] = 33;
    m_iupacmatrix['H'][2] = 33;
    m_iupacmatrix['H'][3] = 33;

    m_iupacsum['h'] = 99;
    m_iupacsum['H'] = 99;

    //     case 'v':
    //     case 'V':
    m_iupacmatrix['v'][0] = 33;
    m_iupacmatrix['v'][1] = 33;
    m_iupacmatrix['v'][3] = 33;
    m_iupacmatrix['V'][0] = 33;
    m_iupacmatrix['V'][1] = 33;
    m_iupacmatrix['v'][3] = 33;

    m_iupacsum['v'] = 99;
    m_iupacsum['V'] = 99;

    //     case 'n':
    //     case 'N':
    //     case 'x':
    //     case 'X':
    m_iupacmatrix['n'][0] = 25;
    m_iupacmatrix['n'][1] = 25;
    m_iupacmatrix['n'][2] = 25;
    m_iupacmatrix['n'][3] = 25;
    m_iupacmatrix['N'][0] = 25;
    m_iupacmatrix['N'][1] = 25;
    m_iupacmatrix['N'][2] = 25;
    m_iupacmatrix['N'][3] = 25;

    m_iupacsum['n'] = 100;
    m_iupacsum['N'] = 100;

    m_iupacmatrix['x'][0] = 25;
    m_iupacmatrix['x'][1] = 25;
    m_iupacmatrix['x'][2] = 25;
    m_iupacmatrix['x'][3] = 25;
    m_iupacmatrix['X'][0] = 25;
    m_iupacmatrix['X'][1] = 25;
    m_iupacmatrix['X'][2] = 25;
    m_iupacmatrix['X'][3] = 25;

    m_iupacsum['x'] = 100;
    m_iupacsum['X'] = 100;

    //     case '.':
    m_iupacmatrix['.'][4] = 1;
    m_iupacsum['.'] = 1;

    //     case '-':
    m_iupacmatrix['-'][5] = 1;
    m_iupacsum['-'] = 1;
}


double SQ_GroupData_RNA::SQ_calc_consensus_deviation ( const char *sequence ) const
{
    double deviation = 0;

    for ( int i = 0; i < size; i++ )
    {
        char s = sequence[i];

        for ( int j = 0; j < 6; j++ )
        {
            int c  = m_iupacmatrix[s][j];
            if ( ( c > 0 ) && ( consensus[i].i[j] <= c ) )
            {
                deviation += c;
            }
        }
    }
    deviation = deviation / size;  //set deviation in relation to sequencelength and percent
    return deviation;
}


double SQ_GroupData_RNA::SQ_calc_consensus_conformity ( const char *sequence ) const
{
    double value = 0;

    for ( int i = 0; i < size; i++ )
    {
        int* cs = consensus[i].i;
        char s = sequence[i];

        for ( int j = 0; j < 6; j++ )
        {
            int c  = m_iupacmatrix[s][j];

            if ( ( c > 0 ) && ( cs[j] > c ) )
            {
                value += ( double ) ( cs[j] - c ) / ( double ) m_iupacsum[s];
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
        char s = sequence[i];

        for ( int j=0; j < 6; ++j )
        {
            cs[j] = m_iupacmatrix[s][j];
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
