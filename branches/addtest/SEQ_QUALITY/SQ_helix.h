//  ==================================================================== //
//                                                                       //
//    File      : SQ_helix.h                                             //
//    Purpose   : Class used for calculation of helix layout             //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif

#ifndef BI_HELIX_HXX
#include <BI_helix.hxx>
#endif
#ifndef AP_FILTER_HXX
#include <AP_filter.hxx>
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif


class SQ_helix : virtual Noncopyable {
    const char *sequence;
    int         count_strong_helix;
    int         count_weak_helix;
    int         count_no_helix;
    int         size;

    static SmartPtr<BI_helix>  helix;
    static GBDATA             *helix_gb_main;
    static std::string         helix_ali_name;
    static std::map<int, int>  filterMap;
    static bool                has_filterMap;

    static BI_helix & getHelix(GBDATA * gb_main, const char *ali_name);

public:
    SQ_helix(int size);
    ~SQ_helix();
    void SQ_calc_helix_layout(const char *sequence, GBDATA * gb_main,
                              char       *alignment_name, GBDATA * gb_quality, AP_filter * filter);
    int SQ_get_no_helix() const {
        return count_no_helix;
    }
    int SQ_get_weak_helix() const {
        return count_weak_helix;
    }
    int SQ_get_strong_helix() const {
        return count_strong_helix;
    }
};

// global data
SmartPtr<BI_helix>  SQ_helix::helix;
GBDATA             *SQ_helix::helix_gb_main = 0;
std::string         SQ_helix::helix_ali_name;
std::map<int, int>  SQ_helix::filterMap;
bool                SQ_helix::has_filterMap = false;

// SQ_helix implementation
BI_helix & SQ_helix::getHelix(GBDATA * gb_main, const char *ali_name) {
    if (helix.isNull() ||
        gb_main != helix_gb_main ||
        strcmp(helix_ali_name.c_str(), ali_name) != 0)
    {
        helix = new BI_helix;
        helix->init(gb_main, ali_name);

        helix_gb_main  = gb_main;
        helix_ali_name = ali_name;
    }
    return *helix;
}

SQ_helix::SQ_helix(int size_)
    : sequence(0),
      count_strong_helix(0),
      count_weak_helix(0),
      count_no_helix(0),
      size(size_)
{
}

SQ_helix::~SQ_helix() {
}

void SQ_helix::SQ_calc_helix_layout(const char *seq, GBDATA * gb_main,
                                    char *alignment_name, GBDATA * gb_quality, AP_filter * filter) {
    getHelix(gb_main, alignment_name);

    size_t        filterLen          = filter->get_filtered_length();
    const size_t *filterpos_2_seqpos = filter->get_filterpos_2_seqpos();

    // one call should be enough here (alignment does not change during the whole evaluation)
    if (!has_filterMap) {
        filterMap.clear();

        for (size_t filter_pos = 0; filter_pos < filterLen; filter_pos++) {
            filterMap[filterpos_2_seqpos[filter_pos]] = filter_pos;
        }

        has_filterMap = true;
    }

    if (!helix->has_entries()) {
        count_strong_helix = 1;
        count_weak_helix = 1;
        count_no_helix = 1;
    }
    else {
        // calculate the number of strong, weak and no helixes
        std::map<int, int>::iterator it;

        for (size_t filter_pos = 0; filter_pos < filterLen; filter_pos++) {
            int seq_pos = filterpos_2_seqpos[filter_pos];

            BI_PAIR_TYPE pair_type = helix->pairtype(seq_pos);
            if (pair_type == HELIX_PAIR) {
                int v_seq_pos = helix->opposite_position(seq_pos);

                if (v_seq_pos > seq_pos) { // ignore right helix positions
                    it = filterMap.find(v_seq_pos);

                    if (it != filterMap.end()) {
                        char left = seq[filter_pos];
                        char right = seq[it->second];
                        int check = helix->check_pair(left, right, pair_type);

                        switch (check) {
                        case 2:
                            count_strong_helix++;
                            break;
                        case 1:
                            count_weak_helix++;
                            break;
                        case 0:
                            if (!((left == '-') && (right == '-')))
                                count_no_helix++;
                            break;
                        }
                    }
                }
            }
        }
    }

    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
    seq_assert(gb_result1);
    GB_write_int(gb_result1, count_no_helix);

    GBDATA *gb_result2 = GB_search(gb_quality, "number_of_weak_helix", GB_INT);
    seq_assert(gb_result2);
    GB_write_int(gb_result2, count_weak_helix);

    GBDATA *gb_result3 =
            GB_search(gb_quality, "number_of_strong_helix", GB_INT);
    seq_assert(gb_result3);
    GB_write_int(gb_result3, count_strong_helix);
}
