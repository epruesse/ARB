//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.cxx                                       //
//    Purpose   : Classes to store global information about sequences    //
//    Time-stamp: <Tue Mar/30/2004 14:31 MET Coder@ReallySoft.de>        //
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

SQ_GroupData::SQ_GroupData() {
    size         = 0;
    avg_bases    = 0;
    nr_sequences = 0;
    gc_prop      = 0;
    initialized  = false;
}


SQ_GroupData::~SQ_GroupData() { }


double SQ_GroupData_RNA::SQ_calc_consensus_deviation(const char *sequence) const {
    double deviation = 0;
    int current[6];


    for (int i = 0; i < size; i++ ){

	for (int k = 0; k < 6; k++) {
	    current[k] = 0;
	}
	//fill up current with decoded iupac values
	switch(toupper(sequence[i])) {
            case 'A':
                current[0] = current[0] + 100;
                break;
            case 'T':
		current[1] = current[1] + 100;
                break;
            case 'C':
		current[2] = current[2] + 100;
                break;
            case 'G':
		current[3] = current[3] + 100;
                break;
            case 'U':
		current[1] = current[1] + 100;
                break;
            case 'R':
		current[0] = current[0] + 50;
		current[3] = current[3] + 50;
                break;
            case 'Y':
		current[2] = current[2] + 50;
		current[1] = current[1] + 50;
                break;
            case 'M':
		current[0] = current[0] + 50;
		current[2] = current[2] + 50;
                break;
            case 'K':
		current[3] = current[3] + 50;
		current[1] = current[1] + 50;
                break;
            case 'W':
		current[0] = current[0] + 50;
		current[1] = current[1] + 50;
                break;
            case 'S':
		current[3] = current[3] + 50;
		current[2] = current[2] + 50;
                break;
            case 'B':
		current[2] = current[2] + 33;
		current[1] = current[1] + 33;
		current[3] = current[3] + 33;
                break;
            case 'D':
		current[0] = current[0] + 33;
		current[1] = current[1] + 33;
		current[3] = current[3] + 33;
                break;
            case 'H':
		current[2] = current[2] + 33;
		current[1] = current[1] + 33;
		current[0] = current[0] + 33;
                break;
            case 'V':
		current[0] = current[0] + 33;
		current[2] = current[2] + 33;
		current[3] = current[3] + 33;
                break;
            case 'N':
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
                seq_assert(0); // unhandled character
                break;

	}//end fill up current

	for (int j = 0; j < 6; j++) {
	    consensus[i].i[j] -= current[j];  //subtract actual value from consensus
	    if ((consensus[i].i[j] <= 0) && (current[j] > 0)) {
		deviation += current[j];
	    }
	    consensus[i].i[j] += current[j];  //restore consensus
	}

    }

    deviation = deviation / size;  //set deviation in relation to sequencelength and percent
    return deviation;
}


double SQ_GroupData_RNA::SQ_calc_consensus_conformity(const char *sequence) const {
    double value = 0;
    int sum = 0;
    int current[6];


    for (int i = 0; i < size; i++ ){

	for (int k = 0; k < 6; k++) {
	    current[k] = 0;
	}
	//fill up current with decoded iupac values
	switch(toupper(sequence[i])) {
            case 'A':
                current[0] = current[0] + 100;
                break;
            case 'T':
		current[1] = current[1] + 100;
                break;
            case 'C':
		current[2] = current[2] + 100;
                break;
            case 'G':
		current[3] = current[3] + 100;
                break;
            case 'U':
		current[1] = current[1] + 100;
                break;
            case 'R':
		current[0] = current[0] + 50;
		current[3] = current[3] + 50;
                break;
            case 'Y':
		current[2] = current[2] + 50;
		current[1] = current[1] + 50;
                break;
            case 'M':
		current[0] = current[0] + 50;
		current[2] = current[2] + 50;
                break;
            case 'K':
		current[3] = current[3] + 50;
		current[1] = current[1] + 50;
                break;
            case 'W':
		current[0] = current[0] + 50;
		current[1] = current[1] + 50;
                break;
            case 'S':
		current[3] = current[3] + 50;
		current[2] = current[2] + 50;
                break;
            case 'B':
		current[2] = current[2] + 33;
		current[1] = current[1] + 33;
		current[3] = current[3] + 33;
                break;
            case 'D':
		current[0] = current[0] + 33;
		current[1] = current[1] + 33;
		current[3] = current[3] + 33;
                break;
            case 'H':
		current[2] = current[2] + 33;
		current[1] = current[1] + 33;
		current[0] = current[0] + 33;
                break;
            case 'V':
		current[0] = current[0] + 33;
		current[2] = current[2] + 33;
		current[3] = current[3] + 33;
                break;
            case 'N':
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
                seq_assert(0); // unhandled character
                break;

	}//end fill up current

	for (int j = 0; j < 6; j++) {
	    consensus[i].i[j] -= current[j];  //subtract actual value from consensus
	    sum += consensus[i].i[j];
	}

	for (int j = 0; j < 6; j++) {
	    if ((consensus[i].i[j] > 0) && (current[j] > 0)) {
		value += consensus[i].i[j] / sum;
	    }
	    consensus[i].i[j] += current[j];  //restore consensus
	}

	sum = 0;

    }

    value = value / size;  //set conformity in relation to sequencelength
    return value;
}


void SQ_GroupData_RNA::SQ_add_sequence(const char *sequence) {

    for (int i=0; i < size; i++) {
#warning has to work with IUPAC codes!
        switch(toupper(sequence[i])) {
            case 'A':
                consensus[i].i[0] = consensus[i].i[0] + 100;
                break;
            case 'T':
		consensus[i].i[1] = consensus[i].i[1] + 100;
                break;
            case 'C':
		consensus[i].i[2] = consensus[i].i[2] + 100;
                break;
            case 'G':
		consensus[i].i[3] = consensus[i].i[3] + 100;
                break;
            case 'U':
		consensus[i].i[1] = consensus[i].i[1] + 100;
                break;
            case 'R':
		consensus[i].i[0] = consensus[i].i[0] + 50;
		consensus[i].i[3] = consensus[i].i[3] + 50;
                break;
            case 'Y':
		consensus[i].i[2] = consensus[i].i[2] + 50;
		consensus[i].i[1] = consensus[i].i[1] + 50;
                break;
            case 'M':
		consensus[i].i[0] = consensus[i].i[0] + 50;
		consensus[i].i[2] = consensus[i].i[2] + 50;
                break;
            case 'K':
		consensus[i].i[3] = consensus[i].i[3] + 50;
		consensus[i].i[1] = consensus[i].i[1] + 50;
                break;
            case 'W':
		consensus[i].i[0] = consensus[i].i[0] + 50;
		consensus[i].i[1] = consensus[i].i[1] + 50;
                break;
            case 'S':
		consensus[i].i[3] = consensus[i].i[3] + 50;
		consensus[i].i[2] = consensus[i].i[2] + 50;
                break;
            case 'B':
		consensus[i].i[2] = consensus[i].i[2] + 33;
		consensus[i].i[1] = consensus[i].i[1] + 33;
		consensus[i].i[3] = consensus[i].i[3] + 33;
                break;
            case 'D':
		consensus[i].i[0] = consensus[i].i[0] + 33;
		consensus[i].i[1] = consensus[i].i[1] + 33;
		consensus[i].i[3] = consensus[i].i[3] + 33;
                break;
            case 'H':
		consensus[i].i[2] = consensus[i].i[2] + 33;
		consensus[i].i[1] = consensus[i].i[1] + 33;
		consensus[i].i[0] = consensus[i].i[0] + 33;
                break;
            case 'V':
		consensus[i].i[0] = consensus[i].i[0] + 33;
		consensus[i].i[2] = consensus[i].i[2] + 33;
		consensus[i].i[3] = consensus[i].i[3] + 33;
                break;
            case 'N':
            case 'X':
		consensus[i].i[2] = consensus[i].i[2] + 25;
		consensus[i].i[1] = consensus[i].i[1] + 25;
		consensus[i].i[0] = consensus[i].i[0] + 25;
		consensus[i].i[3] = consensus[i].i[3] + 25;
                break;
            case '.':
		consensus[i].i[4] = consensus[i].i[4] + 1;
                break;
            case '-':
		consensus[i].i[5] = consensus[i].i[5] + 1;
                break;
            default :
                seq_assert(0); // unhandled character
                break;
        }
    }
}


double SQ_GroupData_PRO::SQ_calc_consensus_deviation(const char *sequence) const {
#warning implementation missing
}

double SQ_GroupData_PRO::SQ_calc_consensus_conformity(const char *sequence) const {
#warning implementation missing
}

void SQ_GroupData_PRO::SQ_add_sequence(const char *sequence) {
#warning implementation missing
}
