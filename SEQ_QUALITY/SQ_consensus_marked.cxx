#include "SQ_consensus_marked.hxx"

SQ_consensus_marked::SQ_consensus_marked(){
    position = 0;
    base = 'x';
}

void SQ_consensus_marked::SQ_clac_consensus(char& base, int& position){
    this->base = base;
    this->position = position;

    consensus[position][0] = base;
}
