#include <stdio.h>
#include "SQ_GroupData.h"

using namespace std;

SQ_GroupData::SQ_GroupData() {
    size        = 0;
    avg_bases   = 0;
    initialized = false;
}

SQ_GroupData::~SQ_GroupData() { }

double SQ_GroupData_RNA::SQ_test_against_consensus(const char *sequence) {
    char c;
    bool sema = false;
    double result    = 0;
    double div       = 0;
    int temp         = 1;
    int base_counter = 0;
    int current      = 0;

    for (int i = 0; i < size; i++ ){
	c = sequence[i];
	current = 0;
	div     = 0;
	sema    = false;
        switch(c) {
            case 'A':
                current = consensus[i].i[0];
		sema=true;
		base_counter++;
                break;
            case 'T':
                current = consensus[i].i[1];
		sema=true;
		base_counter++;
                break;
            case 'C':
                current = consensus[i].i[2];
		sema=true;
		base_counter++;
                break;
            case 'G':
                current = consensus[i].i[3];
		sema=true;
		base_counter++;
                break;
            case 'U':
                current = consensus[i].i[4];
		sema=true;
		base_counter++;
                break;
        }
	if (sema==true){
	    temp=0;
	    for (int j = 0; j < 5; j++) {
		temp = temp + consensus[i].i[j];
	    }
	    div = current / temp;
	}
	result = result + div;
	//printf(" %f",result);
    }
    if(base_counter!=0){
	result = result / base_counter;
	//printf(" %i",base_counter);
    }
    else{
	result=0;
    }
    return result;
}

void SQ_GroupData_RNA::SQ_add_sequence(const char *sequence) {

    for (int i=0; i < size; i++) {
#warning has to work with IUPAC codes!
        switch(sequence[i]) {
            case 'A':
                consensus[i].i[0] = consensus[i].i[0] + 100; //ich mach das noch mit operator +=
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
		consensus[i].i[4] = consensus[i].i[4] + 100;
                break;
            case 'R':
		consensus[i].i[0] = consensus[i].i[0] + 50;
		consensus[i].i[3] = consensus[i].i[3] + 50;
                break;
            case 'Y':
		consensus[i].i[2] = consensus[i].i[2] + 33;
		consensus[i].i[1] = consensus[i].i[1] + 33;
		consensus[i].i[4] = consensus[i].i[4] + 33;
                break;
            case 'M':
		consensus[i].i[0] = consensus[i].i[0] + 50;
		consensus[i].i[2] = consensus[i].i[2] + 50;
                break;
            case 'K':
		consensus[i].i[3] = consensus[i].i[3] + 33;
		consensus[i].i[1] = consensus[i].i[1] + 33;
		consensus[i].i[4] = consensus[i].i[4] + 33;
                break;
            case 'W':
		consensus[i].i[0] = consensus[i].i[0] + 33;
		consensus[i].i[1] = consensus[i].i[1] + 33;
		consensus[i].i[4] = consensus[i].i[4] + 33;
                break;
            case 'S':
		consensus[i].i[3] = consensus[i].i[3] + 50;
		consensus[i].i[2] = consensus[i].i[2] + 50;
                break;
            case 'B':
		consensus[i].i[2] = consensus[i].i[2] + 25;
		consensus[i].i[1] = consensus[i].i[1] + 25;
		consensus[i].i[3] = consensus[i].i[3] + 25;
		consensus[i].i[4] = consensus[i].i[4] + 25;
                break;
            case 'D':
		consensus[i].i[0] = consensus[i].i[0] + 25;
		consensus[i].i[1] = consensus[i].i[1] + 25;
		consensus[i].i[3] = consensus[i].i[3] + 25;
		consensus[i].i[4] = consensus[i].i[4] + 25;
                break;
            case 'H':
		consensus[i].i[2] = consensus[i].i[2] + 25;
		consensus[i].i[1] = consensus[i].i[1] + 25;
		consensus[i].i[0] = consensus[i].i[0] + 25;
		consensus[i].i[4] = consensus[i].i[4] + 25;
                break;
            case 'V':
		consensus[i].i[0] = consensus[i].i[0] + 33;
		consensus[i].i[2] = consensus[i].i[2] + 33;
		consensus[i].i[3] = consensus[i].i[3] + 33;
                break;
            case 'N':
		consensus[i].i[2] = consensus[i].i[2] + 20;
		consensus[i].i[1] = consensus[i].i[1] + 20;
		consensus[i].i[0] = consensus[i].i[0] + 20;
		consensus[i].i[3] = consensus[i].i[3] + 20;
		consensus[i].i[4] = consensus[i].i[4] + 20;
                break;
            case '.':
		consensus[i].i[5] = consensus[i].i[5] + 1;
                break;
            case '-':
		consensus[i].i[6] = consensus[i].i[6] + 1;
                break;
        }
    }



//     for (int i=0; i < size; i++) {
// #warning has to work with IUPAC codes!
//         switch(sequence[i]) {
//             case 'A': consensus[i].i[0]++; break;
//             case 'T': consensus[i].i[1]++; break;
//             case 'C': consensus[i].i[2]++; break;
//             case 'G': consensus[i].i[3]++; break;
//             case 'U': consensus[i].i[4]++; break;
//             case '.': consensus[i].i[5]++; break;
//             case '-': consensus[i].i[6]++; break;
//             default :
//                 seq_assert(0); // unhandled character
//                 break;
//         }
//     }
}

double SQ_GroupData_PRO::SQ_test_against_consensus(const char *sequence) {
#warning implementation missing
}

void SQ_GroupData_PRO::SQ_add_sequence(const char *sequence) {
#warning implementation missing
}
