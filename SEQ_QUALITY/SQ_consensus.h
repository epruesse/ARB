//  ==================================================================== //
//                                                                       //
//    File      : SQ_consensus.h                                         //
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


class SQ_consensus {

public:
    SQ_consensus(size_t size);
    ~SQ_consensus();
    void SQ_calc_consensus(const char *sequence);
    //char SQ_get_consensus(int position);


private:
    int **consensus;
    size_t size;

};


SQ_consensus::SQ_consensus(size_t size){
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


SQ_consensus::~SQ_consensus() {
     delete [] consensus;
}


void SQ_consensus::SQ_calc_consensus(const char *sequence){
    char temp;

    for (size_t i; i < size; i++) {
	temp = sequence[i];
	switch(temp) {
	    case 'A':
		consensus[i][0] = consensus[i][0]++;
		break;
	    case 'T':
		consensus[i][0] = consensus[i][1]++;
		break;
	    case 'C':
		consensus[i][0] = consensus[i][2]++;
		break;
	    case 'G':
		consensus[i][0] = consensus[i][3]++;
		break;
	    case '.':
		consensus[i][0] = consensus[i][4]++;
		break;
	    case '-':
		consensus[i][0] = consensus[i][5]++;
		break;
	}
    }

}


/* char SQ_consensus::SQ_get_consensus(int position) { */
/*     this->position = position; */
/*     char c; */

/*     c = consensus[position]; */
/*     return c; */
/* } */
