//  ==================================================================== //
//                                                                       //
//    File      : SQ_physical_layout.h                                   //
//    Purpose   : Class used for calculation of the phys. layout of seq. //
//    Time-stamp: <Tue Feb/03/2004 14:48 MET  Coder@ReallySoft.de>       //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
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
    void SQ_calc_physical_layout(const char *sequence, int size,
                                 GBDATA * gb_quality_ali);
    int SQ_get_number_of_bases() const;
    double SQ_get_gc_proportion() const;
  private:
    int roundme(double value);
    double temp;
    double count_bases;
    double count_scores;
    double count_dots;
    double GC;
    double GC_proportion;
    int percent_bases;
    int count_bases2;
};


SQ_physical_layout::SQ_physical_layout()
{
    temp = 0;
    count_bases = 0;
    count_scores = 0;
    count_dots = 0;
    percent_bases = 0;
    GC = 0;
    GC_proportion = 0;
    count_bases2 = 0;
}


int SQ_physical_layout::roundme(double value)
{
    int x;
    value += 0.5;
    x = (int) floor(value);
    return x;
}


void SQ_physical_layout::SQ_calc_physical_layout(const char *sequence,
                                                 int size,
                                                 GBDATA * gb_quality_ali)
{
    count_bases = size;

    for (int i = 0; i < size; i++) {
        switch (sequence[i]) {
        case '-':              /*calculate number of dots and spaces */
            count_bases--;
            count_scores++;
            break;
        case '.':
            count_bases--;
            count_dots++;
            break;
        case 'G':              /*calculate GC layout of sequence */
        case 'g':
            GC++;
            break;
        case 'C':
        case 'c':
            GC++;
            break;
        }
    }

    /*calculate layout in percent */
    if (GC != 0) {
        GC_proportion = GC / count_bases;
    }
    temp = 100 - (100 * ((count_scores + count_dots) / size));
    percent_bases = roundme(temp);
    count_bases2 = roundme(count_bases);

    GBDATA *gb_result1 =
        GB_search(gb_quality_ali, "number_of_bases", GB_INT);
    seq_assert(gb_result1);
    GB_write_int(gb_result1, count_bases2);

    GBDATA *gb_result2 =
        GB_search(gb_quality_ali, "percent_of_bases", GB_INT);
    seq_assert(gb_result2);
    GB_write_int(gb_result2, percent_bases);

    GBDATA *gb_result3 =
        GB_search(gb_quality_ali, "GC_proportion", GB_FLOAT);
    seq_assert(gb_result3);
    GB_write_float(gb_result3, GC_proportion);
}


inline int SQ_physical_layout::SQ_get_number_of_bases() const
{
    int i;
    i = count_bases2;
    return i;
}


inline double SQ_physical_layout::SQ_get_gc_proportion() const
{
    double i;
    i = GC_proportion;
    return i;
}
