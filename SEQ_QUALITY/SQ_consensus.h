//  ==================================================================== //
//                                                                       //
//    File      : SQ_consensus.h                                         //
//    Purpose   : Class used for calculation of consensus sequences      //
//    Time-stamp: <Thu Oct/02/2003 19:12 MET Coder@ReallySoft.de>        //
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
    typedef vector<int> PosCounter;

public:
    SQ_consensus(int size_);
//     void SQ_init_consensus(int size);
    void SQ_calc_consensus(const char *sequence);
    int* SQ_get_consensus(int row, int col);

private:
    int size;
    vector<PosCounter>  v2;

};


// @@@ ab hier gehoert alles nach .cxx


SQ_consensus::SQ_consensus(int size_)
    : size(size_)
    , v2(size_, PosCounter(7))
{
}


// void SQ_consensus::SQ_init_consensus(int size){
//     this->size = size;
//     this->v2   = v2; // ist aequivalent zu 'v2 = v2;' oder 'this->v2 = this->v2' oder 'v2 = this->v2'
//     vector<int> v(6);
//     vector<vector<int> >  v2(size,v); // Vorsicht! verdeckt die Membervariable 'v2'
// }


void SQ_consensus::SQ_calc_consensus(const char *sequence){
    char temp;

    for (int i=0; i < size; i++) {
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
            case 'U':
                v2[i][4]++;
                break;
            case '.':
                v2[i][5]++;
                break;
            case '-':
                v2[i][6]++;
                break;
        }
    }

}

int* SQ_consensus::SQ_get_consensus(int row, int col) {
    int* pa;

    pa = &v2[row][col];
    return pa;
}
