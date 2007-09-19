//  ==================================================================== //
//                                                                       //
//    File      : SQ_helix.h                                             //
//    Purpose   : Class used for calculation of helix layout             //
//    Time-stamp: <Mon Sep/17/2007 12:28 MET Coder@ReallySoft.de>       //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef _CPP_STRING
#include <string>
#endif
#ifndef _CPP_MAP
#include <map>
#endif

#ifndef BI_HELIX_HXX
#include <BI_helix.hxx>
#endif

class SQ_helix {
  public:
    SQ_helix(int size);
    ~SQ_helix();
    void SQ_calc_helix_layout(const char *sequence, GBDATA * gb_main,
                              char *alignment_name, GBDATA * gb_quality,
                              AP_filter * filter);
    int SQ_get_no_helix() const {
        return count_no_helix;
    }
    int SQ_get_weak_helix() const {
        return count_weak_helix;
    }
    int SQ_get_strong_helix() const {
        return count_strong_helix;
    }
  private:
    const char *sequence;
    int count_strong_helix;
    int count_weak_helix;
    int count_no_helix;
    int size;

    static BI_helix *helix;
    static GBDATA *helix_gb_main;
    static std::string helix_ali_name;
    static std::map < int, int >filterMap;
    static bool has_filterMap;

    static BI_helix & getHelix(GBDATA * gb_main, const char *ali_name);
};


// global data
BI_helix *SQ_helix::helix = 0;
GBDATA *SQ_helix::helix_gb_main = 0;
std::string SQ_helix::helix_ali_name;
std::map < int, int >SQ_helix::filterMap;
bool SQ_helix::has_filterMap = false;


// SQ_helix implementation
BI_helix & SQ_helix::getHelix(GBDATA * gb_main, const char *ali_name)
{
    if (!helix || gb_main != helix_gb_main
        || strcmp(helix_ali_name.c_str(), ali_name) != 0) {
        delete helix;
        helix = new BI_helix;

        helix->init(gb_main, ali_name);

        helix_gb_main = gb_main;
        helix_ali_name = ali_name;
    }
    return *helix;
}


SQ_helix::SQ_helix(int size_)
:  sequence(0),
count_strong_helix(0), count_weak_helix(0), count_no_helix(0), size(size_)
{
}

SQ_helix::~SQ_helix()
{
}


void SQ_helix::SQ_calc_helix_layout(const char *sequence, GBDATA * gb_main,
                                    char *alignment_name,
                                    GBDATA * gb_quality,
                                    AP_filter * filter)
{
    BI_helix& helix = getHelix(gb_main, alignment_name);

    // one call should be enough here (alignment does not change during the whole evaluation)
    if (!has_filterMap) {
        filterMap.clear();

        for (int filter_pos = 0; filter_pos < filter->real_len; filter_pos++) {
            filterMap[filter->filterpos_2_seqpos[filter_pos]] = filter_pos;
        }

        has_filterMap = true;
    }

    if (!helix.has_entries()) {
        count_strong_helix = 1;
        count_weak_helix = 1;
        count_no_helix = 1;
    } else {
        // calculate the number of strong, weak and no helixes
        for (int filter_pos = 0; filter_pos < filter->real_len;
             filter_pos++) {
            int seq_pos = filter->filterpos_2_seqpos[filter_pos];

            BI_PAIR_TYPE pair_type = helix.pairtype(seq_pos);
            if (pair_type == HELIX_PAIR) {
                int v_seq_pos = helix.opposite_position(seq_pos);
                if (v_seq_pos > seq_pos) {      // ignore right helix positions
                    int v_filter_pos = filterMap[v_seq_pos];

#warning v_filter_pos==0 indicates a 'not found' entry but it also can be a legal position
                    seq_assert(v_filter_pos);

                    if (v_filter_pos > 0) {
                        char left  = sequence[filter_pos];
                        char right = sequence[v_filter_pos];
                        int  check = helix.check_pair(left, right, pair_type);

                        switch (check) {
                            case 2:
                                count_strong_helix++;
                                break;
                            case 1:
                                count_weak_helix++;
                                break;
                            case 0:
                                if (!((left == '-') && (right == '-'))) count_no_helix++;
                                break;
                        }
                    }
                }
            }
        }
    }

//     /*if (count_strong_helix != 0)*/ count_strong_helix = count_strong_helix / 2;
//     /*if (count_weak_helix   != 0)*/ count_weak_helix = count_weak_helix / 2;
//     /*if (count_no_helix     != 0)*/ count_no_helix = count_no_helix / 2;

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
