//  ==================================================================== //
//                                                                       //
//    File      : SQ_helix.h                                             //
//    Purpose   : Class used for calculation of helix layout             //
//    Time-stamp: <Tue Feb/03/2004 14:34 MET  Coder@ReallySoft.de>       //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
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
    SQ_helix(int size);
    ~SQ_helix();
    void SQ_calc_helix_layout(const char *sequence, GBDATA *gb_main, char *alignment_name, GBDATA *gb_quality);
    int  SQ_get_no_helix() const;
    int  SQ_get_weak_helix() const;
    int  SQ_get_strong_helix() const;


private:
    const char * sequence;
    int count_strong_helix;
    int count_weak_helix;
    int count_no_helix;
    int size;

};



SQ_helix::SQ_helix(int size){
    this->size = size;
    count_strong_helix = 0;
    count_weak_helix   = 0;
    count_no_helix     = 0;
}


SQ_helix::~SQ_helix() {
}


void SQ_helix::SQ_calc_helix_layout(const char *sequence, GBDATA *gb_main, char *alignment_name, GBDATA *gb_quality){

    int  j     = 0;
    int  temp  = 0;
    char left;
    char right;

    BI_PAIR_TYPE pair_type = HELIX_PAIR;
    BI_helix my_helix;
    my_helix.init(gb_main, alignment_name);
    

    if(my_helix.entries==0){
	count_strong_helix=1;
	count_weak_helix=1;
	count_no_helix=1;
    }

    else{
    /*claculate the number of strong, weak and no helixes*/
	for (int i = 0; i < size; i++) {
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
    }

    if (count_strong_helix != 0) count_strong_helix = count_strong_helix / 2;
    if (count_weak_helix   != 0) count_weak_helix = count_weak_helix / 2;
    if (count_no_helix     != 0) count_no_helix = count_no_helix / 2;

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


inline int SQ_helix::SQ_get_strong_helix() const {
    int i;
    i = count_strong_helix;
    return i;
}


inline int SQ_helix::SQ_get_weak_helix() const {
    int i;
    i = count_weak_helix;
    return i;
}


inline int SQ_helix::SQ_get_no_helix() const {
    int i;
    i = count_no_helix;
    return i;
}
