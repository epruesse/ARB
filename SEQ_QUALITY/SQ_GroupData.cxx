#include <stdio.h>
#include "SQ_GroupData.h"

using namespace std;


SQ_GroupData::SQ_GroupData(){
    size        = 0;
    avg_bases   = 0;
    initialised = false;
}


SQ_GroupData::~SQ_GroupData(){
}


void SQ_GroupData::SQ_set_avg_bases(int bases){
    avg_bases = bases;
}


int SQ_GroupData::SQ_get_avg_bases() const{
    return avg_bases;
}


bool SQ_GroupData::SQ_is_initialised() {
    return initialised;
}
