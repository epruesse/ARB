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
    SQ_consensus();
    ~SQ_consensus();
    void SQ_init_consensus(int size);
    void SQ_calc_consensus(const char *sequence);
    int** SQ_get_consensus();
    void SQ_add_consensus(int **consensus_add);

private:
    int **consensus;
    int size;

};


SQ_consensus::SQ_consensus() {
     size = 0;
}


SQ_consensus::~SQ_consensus() {
     delete [] consensus;
}


void SQ_consensus::SQ_init_consensus(int size){
    this->size = size;

    // two dimensional array
    consensus = new int *[size];
    for ( int i=0; i < size; i++ ){
	consensus[i] = new int [6];
    }
    for ( int i = 0; i < size; i++ ){
	for (int j = 0; i < 6; i++) {
	    consensus[i][j] = 0;
	}
    }
}


void SQ_consensus::SQ_calc_consensus(const char *sequence){
    char temp;

    for (int i; i < size; i++) {
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


int** SQ_consensus::SQ_get_consensus() {

    for(int i = 0; i < 10; i++) {
	for(int j = 0; j < 6; j++) {
	    printf("\n %i ",consensus[i][j]);
	}
    }
    return consensus;
}


void SQ_consensus::SQ_add_consensus(int **consensus_add) {

    for (int i = 0; i < size; i++){
	for (int j = 0; i < 6; i++){
	    consensus[i][j] = consensus[i][j] + consensus_add[i][j];
	}
    }
}


/* int* SQ_consensus::tester() { */
/*     int *i; */
/*     i = &consensus[0][0]; */
/*     return i; */
/* } */
