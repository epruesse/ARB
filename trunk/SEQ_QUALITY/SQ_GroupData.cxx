#include <stdio.h>
#include "SQ_GroupData.h"

using namespace std;


SQ_GroupData::SQ_GroupData(){
    size = 0;
    initialised=false;
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
