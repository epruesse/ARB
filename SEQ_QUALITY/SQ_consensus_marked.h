class SQ_consensus_marked {

public:
    SQ_consensus_marked();
    //~SQ_consensus_marked();
    void SQ_calc_consensus(char base, int position);
    char SQ_get_consensus(int position);


private:
    char consensus[40000][1];
    char base;
    int position;

};


SQ_consensus_marked::SQ_consensus_marked(){
    position = 0;
    base = 'x';

    //init consensus with 'z' = undefined
    for (int i=0; i<40000; i++){
	consensus[i][0] = 'z';
    }

}


/* SQ_consensus_marked::~SQ_consensus_marked(){ */
/*     free(consensus); */
/* } */


void SQ_consensus_marked::SQ_calc_consensus(char base, int position){
    this->base     = base;
    this->position = position;
    char temp;

    temp = consensus[position][0];
    if (temp == 'z') {
	consensus[position][0] = base;
    }
    else{
	 if (temp != base){
	     consensus[position][0] = 'x';
	 }
    }

}


char SQ_consensus_marked::SQ_get_consensus(int position) {
    this->position = position;
    char c;

    c = consensus[position][0];
    return c;
}
