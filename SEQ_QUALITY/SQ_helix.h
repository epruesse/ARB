//  ==================================================================== //
//                                                                       //
//    File      : SQ_helix.h                                             //
//    Purpose   : Class used for calculation of helix layout             //
//    Time-stamp: <Thu Feb/05/2004 11:40 MET Coder@ReallySoft.de>       //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#include <string>

#undef _USE_AW_WINDOW
#include "BI_helix.hxx"

// #include <arb_assert.h>
// #define seq_assert(bed) arb_assert(bed)


class SQ_helix
{
    public:
        SQ_helix ( int size );
        ~SQ_helix();
        void SQ_calc_helix_layout ( const char *sequence, GBDATA *gb_main, char *alignment_name, GBDATA *gb_quality );
        int  SQ_get_no_helix() const { return count_no_helix; }
        int  SQ_get_weak_helix() const { return count_weak_helix; }
        int  SQ_get_strong_helix() const { return count_strong_helix; }
    private:
        const char * sequence;
        int          count_strong_helix;
        int          count_weak_helix;
        int          count_no_helix;
        int          size;

        static BI_helix    *helix;
        static GBDATA      *helix_gb_main;
        static std::string  helix_ali_name;

        static BI_helix& getHelix ( GBDATA *gb_main, const char *ali_name );
};


// global data
BI_helix    *SQ_helix::helix         = 0;
GBDATA      *SQ_helix::helix_gb_main = 0;
std::string  SQ_helix::helix_ali_name;


// SQ_helix implementation
BI_helix& SQ_helix::getHelix ( GBDATA *gb_main, const char *ali_name )
{
    if ( !helix || gb_main != helix_gb_main || strcmp ( helix_ali_name.c_str(), ali_name ) != 0 )
    {
        delete helix;
        helix = new BI_helix;

        helix->init ( gb_main, ali_name );

        helix_gb_main  = gb_main;
        helix_ali_name = ali_name;
    }
    return *helix;
}


SQ_helix::SQ_helix ( int size_ )
        : sequence ( 0 ),
        count_strong_helix ( 0 ),
        count_weak_helix ( 0 ),
        count_no_helix ( 0 ),
        size ( size_ ) {}

SQ_helix::~SQ_helix() {}


void SQ_helix::SQ_calc_helix_layout ( const char *sequence, GBDATA *gb_main, char *alignment_name, GBDATA *gb_quality )
{
    BI_helix& my_helix = getHelix ( gb_main, alignment_name );

    if ( my_helix.entries==0 )
    {
        count_strong_helix=1;
        count_weak_helix=1;
        count_no_helix=1;
    }
    else
    {
        /*calculate the number of strong, weak and no helixes*/
        for ( int i = 0; i < size; i++ )
        {
            BI_PAIR_TYPE pair_type = my_helix.entries[i].pair_type;
            if ( pair_type == HELIX_PAIR )
            {
                int j = my_helix.entries[i].pair_pos;
                if ( j > i )
                { // ignore right helix positions
                    char left  = sequence[i];
                    char right = sequence[j];
                    int  temp  = my_helix.check_pair ( left, right, pair_type );

                    switch ( temp )
                    {
                        case 2:
                            count_strong_helix++;
                            break;
                        case 1:
                            count_weak_helix++;
                            break;
                        case 0:
                            if ( ! ( ( left == '-' ) && ( right == '-' ) ) ) count_no_helix++;
                            break;
                    }
                }
            }
        }
    }

//     /*if (count_strong_helix != 0)*/ count_strong_helix = count_strong_helix / 2;
//     /*if (count_weak_helix   != 0)*/ count_weak_helix = count_weak_helix / 2;
//     /*if (count_no_helix     != 0)*/ count_no_helix = count_no_helix / 2;

    GBDATA *gb_result1 = GB_search ( gb_quality, "number_of_no_helix", GB_INT );
    seq_assert ( gb_result1 );
    GB_write_int ( gb_result1, count_no_helix );

    GBDATA *gb_result2 = GB_search ( gb_quality, "number_of_weak_helix", GB_INT );
    seq_assert ( gb_result2 );
    GB_write_int ( gb_result2, count_weak_helix );

    GBDATA *gb_result3 = GB_search ( gb_quality, "number_of_strong_helix", GB_INT );
    seq_assert ( gb_result3 );
    GB_write_int ( gb_result3, count_strong_helix );
}
