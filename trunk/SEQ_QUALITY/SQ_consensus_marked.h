//  ==================================================================== //
//                                                                       //
//    File      : SQ_consensus_marked.h                                  //
//    Purpose   : Class used for calculation of consensus sequences      //
//    Time-stamp: <Thu Sep/25/2003 17:58 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


class SQ_consensus_marked {

public:
    SQ_consensus_marked(size_t size);
    ~SQ_consensus_marked();
    void SQ_calc_consensus(char base, int position);
    char SQ_get_consensus(int position);


private:
    char *consensus;
//     char  consensus[40000];
    char  base;
    int   position;

};


SQ_consensus_marked::SQ_consensus_marked(size_t size){
    position  = 0;
    base      = 'x';
    consensus = new char[size];

    //init consensus with 'z' = undefined
    for (size_t i=0; i<size; i++){
	consensus[i] = 'z';
    }

}


SQ_consensus_marked::~SQ_consensus_marked() {
     delete [] consensus;
}


void SQ_consensus_marked::SQ_calc_consensus(char base, int position){
    this->base     = base;
    this->position = position;
    char temp;

    temp = consensus[position];
    if (temp == 'z') {
	consensus[position] = base;
    }
    else{
	 if (temp != base){
	     consensus[position] = 'x';
	 }
    }

}


char SQ_consensus_marked::SQ_get_consensus(int position) {
    this->position = position;
    char c;

    c = consensus[position];
    return c;
}
