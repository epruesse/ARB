//  ==================================================================== //
//                                                                       //
//    File      : SQ_physical_layout.h                                   //
//    Purpose   : Class used for calculation of the phys. layout of seq. //
//    Time-stamp: <Thu Sep/25/2003 17:58 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)


class SQ_physical_layout {

public:
    SQ_physical_layout();
    void SQ_calc_physical_layout(const char *sequence, int size, GBDATA *gb_quality);
    int SQ_get_number_of_bases() const;


private:
    int count_bases;
    int count_scores;
    int count_dots;
    int percent_scores;
    int percent_dots;
    int percent_bases;
    int GC;
    int GC_proportion;

};


SQ_physical_layout::SQ_physical_layout(){
    count_bases       = 0;
    count_scores      = 0;
    count_dots        = 0;
    percent_scores    = 0;
    percent_dots      = 0;
    percent_bases     = 0;
    GC                = 0;
    GC_proportion     = 0;
}



void SQ_physical_layout::SQ_calc_physical_layout(const char *sequence, int size, GBDATA *gb_quality){
    char c;
    count_bases = size;

    for (int i = 0; i < size; i++) {

	c = sequence[i];
	switch (c) {
	    case '-':	         /*claculate number of dots and spaces*/
		count_bases--;
		count_scores++;
		break;
	    case '.':
		count_bases--;
		count_dots++;
		break;
	    case 'G':	         /*claculate GC layout of sequence*/
		GC++;
		break;
	    case 'C':
		GC++;
		break;
	}

    }

    /*calculate layout in percent*/
    if (GC!=0) {
      GC_proportion = (100 * count_bases) / GC; //this is a hack, as ARB can't save real values
    }
    percent_scores = (100 * count_scores) / size;
    percent_dots   = (100 * count_dots) / size;
    percent_bases  = 100 - (percent_scores + percent_dots);


    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);
    seq_assert(gb_result1);
    GB_write_int(gb_result1, count_bases);

    GBDATA *gb_result2 = GB_search(gb_quality, "percent_of_bases", GB_INT);
    seq_assert(gb_result2);
    GB_write_int(gb_result2, percent_bases);

    GBDATA *gb_result3 = GB_search(gb_quality, "GC_proportion", GB_INT);
    seq_assert(gb_result3);
    GB_write_int(gb_result3, GC_proportion);
}


inline int SQ_physical_layout::SQ_get_number_of_bases() const {
    int i;
    i = count_bases;
    return i;
}
 
