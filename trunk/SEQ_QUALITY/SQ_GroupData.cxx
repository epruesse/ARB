#include <stdio.h>
#include "SQ_GroupData.h"

using namespace std;


SQ_GroupData::SQ_GroupData(){
    size = 0;
    initialised=false;
}


SQ_GroupData::~SQ_GroupData(){
    delete [] consensus;
}


void SQ_GroupData::SQ_init_consensus(int size){
    this->size = size;

    // two dimensional array
    consensus = new int *[size];
    for (int i=0; i < size; i++ ){
	consensus[i] = new int [7];
    }
    for (int i=0; i < size; i++ ){
	for (int j=0; j<7; j++) {
	    consensus[i][j] = 0;
	}
    }
    initialised=true;
}


void SQ_GroupData::SQ_add_consensus(int value, int row, int col) {

    consensus[row][col] = consensus[row][col] + value;

}

int **SQ_GroupData::SQ_get_consensus() {

    return consensus;

}


bool SQ_GroupData::SQ_is_initialised() {

    return initialised;

}


int SQ_GroupData::SQ_print_on_screen() {
    for (int i=0; i < size; i++ ){
	for (int j = 0; j<7; j++) {
	    printf("%i",consensus[i][j]);
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
        switch(c) {
            case 'A':
                current = consensus[i][0];
		sema=true;
		base_counter++;
                break;
            case 'T':
                current = consensus[i][1];
		sema=true;
		base_counter++;
                break;
            case 'C':
                current = consensus[i][2];
		sema=true;
		base_counter++;
                break;
            case 'G':
                current = consensus[i][3];
		sema=true;
		base_counter++;
                break;
            case 'U':
                current = consensus[i][4];
		sema=true;
		base_counter++;
                break;
        }
	if (sema==true){
	    temp=0;
	    for (int j = 0; j < 5; j++) {
		temp = temp + consensus[i][j];
	    }
	    div = current / temp;
	    current = 0;
	    sema = false;
	}

	result = result + div;
	//printf(" %f",result);
	div = 0;
    }

    result = result / base_counter;
    //printf(" %i",base_counter);
    return result;
}
