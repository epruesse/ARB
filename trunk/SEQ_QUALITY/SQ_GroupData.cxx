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
            case 'A': consensus[i].i[0]++; break;
            case 'T': consensus[i].i[1]++; break;
            case 'C': consensus[i].i[2]++; break;
            case 'G': consensus[i].i[3]++; break;
            case 'U': consensus[i].i[4]++; break;
            case '.': consensus[i].i[5]++; break;
            case '-': consensus[i].i[6]++; break;
            default :
                seq_assert(0); // unhandled character
                break;
        }
    }
}

double SQ_GroupData_PRO::SQ_test_against_consensus(const char *sequence) {
#warning implementation missing
}

void SQ_GroupData_PRO::SQ_add_sequence(const char *sequence) {
#warning implementation missing
}
