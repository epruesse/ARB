#include <stdio.h>
#include "SQ_GroupData.h"

using namespace std;



SQ_GroupData::SQ_GroupData(){
    size        = 0;
    avg_bases   = 0;
    initialised = false;
}


SQ_GroupData::~SQ_GroupData(){
    delete [] consensus;
}


void SQ_GroupData::SQ_set_avg_bases(int bases){
    avg_bases = bases;
}


int SQ_GroupData::SQ_get_avg_bases() const{
    return avg_bases;
}


void SQ_GroupData::SQ_init_consensus(int size){
    this->size = size;

    // two dimensional array
    consensus = new Int7[size];

    initialised=true;
}


void SQ_GroupData::SQ_add_consensus_column(int col, const Int7& val) {
    consensus[col] += val;
}

void SQ_GroupData::SQ_add_consensus(int value, int row, int col) {

    consensus[col].i[row] = consensus[col].i[row] + value;
//    consensus[row][col] = consensus[row][col] + value;

}


bool SQ_GroupData::SQ_is_initialised() {

    return initialised;

}


int SQ_GroupData::SQ_print_on_screen() {
    for (int i=0; i < size; i++ ){
	for (int j = 0; j<7; j++) {
	    printf("%i ",consensus[i].i[j]);
	}
    }
    return (0);
}

double SQ_GroupData::SQ_test_against_consensus(const char *sequence) {
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
