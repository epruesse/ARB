#include <stdio.h>
#include "SQ_GroupData.h"

using namespace std;


SQ_GroupData::SQ_GroupData(){
    size = 0;
}

void SQ_GroupData::SQ_init_consensus(size_t size){
    this->size = size;

    // two dimensional array
    consensus = new int *[size];
    for ( size_t i=0; i < size; i++ ){
	consensus[i] = new int [6];
    }
    for ( size_t i=0; i < size; i++ ){
	for (int j = 0; i<6; i++) {
	    consensus[i][j] = 0;
	}
    }
}


void SQ_GroupData::SQ_add_consensus(int **consensus_add) {

    for ( size_t i=0; i < size; i++ ){
	for (int j = 0; i<6; i++) {
	    consensus[i][j] = consensus[i][j] + consensus_add[i][j];
	}
    }
}

int **SQ_GroupData::SQ_get_consensus() {
    return (consensus);
}


void SQ_GroupData::SQ_print_on_screen() {

    for ( size_t i=0; i < size; i++ ){

	printf("%i",consensus[i][0]);
	
    }
}
