//  ==================================================================== //
//                                                                       //
//    File      : SQ_consensus.h                                         //
//    Purpose   : Class used for calculation of consensus sequences      //
//    Time-stamp: <Fri Nov/14/2003 15:25 MET Coder@ReallySoft.de>        //
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
//     typedef vector<int> PosCounter;
    typedef Int7 PosCounter;

public:
    SQ_consensus(int size_);
    void SQ_calc_consensus(const char *sequence);
    const Int7& SQ_get_consensus(int col) const {
        return v2[col];
    }

private:
    int size;
    vector<PosCounter>  v2;

};


// @@@ ab hier gehoert alles nach .cxx


SQ_consensus::SQ_consensus(int size_)
    : size(size_)
    , v2(size_)
{
}


void SQ_consensus::SQ_calc_consensus(const char *sequence){
    char temp;

    for (int i=0; i < size; i++) {
        temp = sequence[i];
        switch(temp) {
            case 'A':
                v2[i].i[0] = v2[i].i[0] + 100;
                break;
            case 'T':
		v2[i].i[1] = v2[i].i[1] + 100;
                break;
            case 'C':
		v2[i].i[2] = v2[i].i[2] + 100;
                break;
            case 'G':
		v2[i].i[3] = v2[i].i[3] + 100;
                break;
            case 'U':
		v2[i].i[4] = v2[i].i[4] + 100;
                break;
            case 'R':
		v2[i].i[0] = v2[i].i[0] + 50;
		v2[i].i[3] = v2[i].i[3] + 50;
                break;
            case 'Y':
		v2[i].i[2] = v2[i].i[2] + 33;
		v2[i].i[1] = v2[i].i[1] + 33;
		v2[i].i[4] = v2[i].i[4] + 33;
                break;
            case 'M':
		v2[i].i[0] = v2[i].i[0] + 50;
		v2[i].i[2] = v2[i].i[2] + 50;
                break;
            case 'K':
		v2[i].i[3] = v2[i].i[3] + 33;
		v2[i].i[1] = v2[i].i[1] + 33;
		v2[i].i[4] = v2[i].i[4] + 33;
                break;
            case 'W':
		v2[i].i[0] = v2[i].i[0] + 33;
		v2[i].i[1] = v2[i].i[1] + 33;
		v2[i].i[4] = v2[i].i[4] + 33;
                break;
            case 'S':
		v2[i].i[3] = v2[i].i[3] + 50;
		v2[i].i[2] = v2[i].i[2] + 50;
                break;
            case 'B':
		v2[i].i[2] = v2[i].i[2] + 25;
		v2[i].i[1] = v2[i].i[1] + 25;
		v2[i].i[3] = v2[i].i[3] + 25;
		v2[i].i[4] = v2[i].i[4] + 25;
                break;
            case 'D':
		v2[i].i[0] = v2[i].i[0] + 25;
		v2[i].i[1] = v2[i].i[1] + 25;
		v2[i].i[3] = v2[i].i[3] + 25;
		v2[i].i[4] = v2[i].i[4] + 25;
                break;
            case 'H':
		v2[i].i[2] = v2[i].i[2] + 25;
		v2[i].i[1] = v2[i].i[1] + 25;
		v2[i].i[0] = v2[i].i[0] + 25;
		v2[i].i[4] = v2[i].i[4] + 25;
                break;
            case 'V':
		v2[i].i[0] = v2[i].i[0] + 33;
		v2[i].i[2] = v2[i].i[2] + 33;
		v2[i].i[3] = v2[i].i[3] + 33;
                break;
            case 'N':
		v2[i].i[2] = v2[i].i[2] + 20;
		v2[i].i[1] = v2[i].i[1] + 20;
		v2[i].i[0] = v2[i].i[0] + 20;
		v2[i].i[3] = v2[i].i[3] + 20;
		v2[i].i[4] = v2[i].i[4] + 20;
                break;
            case '.':
		v2[i].i[5] = v2[i].i[5] + 1;
                break;
            case '-':
		v2[i].i[6] = v2[i].i[6] + 1;
                break;
        }
    }

}

// int* SQ_consensus::SQ_get_consensus(int row, int col) {
//     int* pa;

//     pa = &v2[row].i[col];
//     return pa;
// }
