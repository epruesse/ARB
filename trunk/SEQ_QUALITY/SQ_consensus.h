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

#include <vector>

class SQ_consensus {

public:
    SQ_consensus();
    ~SQ_consensus();
    void SQ_init_consensus(int size);
    void SQ_calc_consensus(const char *sequence);
    int* SQ_get_consensus();
    void SQ_add_consensus(int **consensus_add);

private:
    int size;
    vector<vector<int> >  v2;

};


SQ_consensus::SQ_consensus() {
    size = 0;
}


SQ_consensus::~SQ_consensus() {
}


void SQ_consensus::SQ_init_consensus(int size){
    this->size = size;
    this->v2 = v2;
    vector<int> v(6);
    vector<vector<int> >  v2(size,v);
}


void SQ_consensus::SQ_calc_consensus(const char *sequence){
    char temp;

    for (int i; i < size; i++) {
	temp = sequence[i];

	switch(temp) {
	    case 'A':
		v2[i][0]++;
		break;
	    case 'T':
		v2[i][1]++;
		break;
	    case 'C':
		v2[i][2]++;
		break;
	    case 'G':
		v2[i][3]++;
		break;
	    case '.':
		v2[i][4]++;
		break;
	    case '-':
		v2[i][5]++;
		break;
	}
    }
}


int* SQ_consensus::SQ_get_consensus() {
    int* pa;

    pa = &v2[0][0];
    return pa;
}


void SQ_consensus::SQ_add_consensus(int **consensus_add) {

    for (int i = 0; i < size; i++){
	for (int j = 0; i < 6; i++){
	   v2[i][j] = v2[i][j] + consensus_add[i][j];
	}
    }
}
