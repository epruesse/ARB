//  ==================================================================== //
//                                                                       //
//    File      : SQ_helix.h                                             //
//    Purpose   : Class used for calculation of helix layout             //
//    Time-stamp: <Thu Sep/25/2003 17:58 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#undef _USE_AW_WINDOW
#include "BI_helix.hxx"
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)


class SQ_helix {

public:
    SQ_helix(size_t size);
    ~SQ_helix();
    void SQ_calc_helix_layout(const char *sequence, GBDATA *gb_main, char *alignment_name, GBDATA *gb_quality);
    int  SQ_get_no_helix();
    int  SQ_get_weak_helix();
    int  SQ_get_strong_helix();


private:
    const char *sequence;
    int count_strong_helix;
    int count_weak_helix;
    int count_no_helix;
    size_t size;

}; 



SQ_helix::SQ_helix(size_t size){
    this->size = size;
    count_strong_helix      = 0;
    count_weak_helix = 0;
    count_no_helix   = 0;
    sequence = new char[size];
}


SQ_helix::~SQ_helix() {
     delete [] sequence;
}


void SQ_helix::SQ_calc_helix_layout(const char *sequence, GBDATA *gb_main, char *alignment_name, GBDATA *gb_quality){
    this->sequence = sequence;
    int j                 = 0;
    int temp              = 0;
    char left;
    char right;

    BI_PAIR_TYPE pair_type = HELIX_PAIR;
    BI_helix my_helix;
    my_helix.init(gb_main, alignment_name);


    /*claculate the number of strong, weak and no helixes*/
    for (size_t i = 0; i < size; i++) {
	pair_type = my_helix.entries[i].pair_type;
	if (pair_type == HELIX_PAIR) {
	    left = sequence[i];
	    j = my_helix.entries[i].pair_pos;
	    right = sequence[j];
	    temp = my_helix.check_pair(left, right, pair_type);

	    switch(temp){
		case 2:
		    count_strong_helix++;
		    break;
		case 1:
		    count_weak_helix++;
		    break;
		case 0:
		    count_no_helix++;
		    break;
	    }
	}
    }

    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
    seq_assert(gb_result1);
    GB_write_int(gb_result1, count_no_helix);

    GBDATA *gb_result2 = GB_search(gb_quality, "number_of_weak_helix", GB_INT);
    seq_assert(gb_result2);
    GB_write_int(gb_result2, count_weak_helix);

    GBDATA *gb_result3 = GB_search(gb_quality, "number_of_strong_helix", GB_INT);
    seq_assert(gb_result3);
    GB_write_int(gb_result3, count_strong_helix);

}


int SQ_helix::SQ_get_strong_helix() {
    int i;
    i = count_strong_helix;
    return i;
}


int SQ_helix::SQ_get_weak_helix() {
    int i;
    i = count_weak_helix;
    return i;
}


int SQ_helix::SQ_get_no_helix() {
    int i;
    i = count_no_helix;
    return i;
}
