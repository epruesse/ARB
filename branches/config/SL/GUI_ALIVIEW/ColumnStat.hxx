// =============================================================== //
//                                                                 //
//   File      : ColumnStat.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef COLUMNSTAT_HXX
#define COLUMNSTAT_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

/* Create a window, that allows you to select a column statistic and 
 * get the weights from the selected SAI
 *
 * 1.  create ColumnStat
 * 2.  build button with callback COLSTAT_create_selection_window
 * 3.  call ColumnStat::go(second_filter)
 * 4.  use ColumnStat::get_weights()
 */

class AW_root;
class AW_window;
class AP_filter;
class AW_awar;

#define DIST_MIN_SEQ(seq_anz) (seq_anz / 10)

class ColumnStat : virtual Noncopyable {
    GBDATA  *gb_main;
    AW_root *awr;

    char *awar_name;
    char *awar_alignment;
    char *awar_smooth;
    char *awar_enable_helix;

    char *alignment_name;
    char *type_path;

    class AW_DB_selection *saisel;

    // all members below are valid after calling calculate() only!
    //
    // @@@ they should be stored in a separate class and referenced by pointer here
    // (e.g. rename this class into ColumnStatSelector and name the new class ColumnStat)
    
    size_t    seq_len;                              // real length == 0 -> not valid
    GB_UINT4 *weights;                              // helix        = 1, non helix == 2
    float    *rates;
    float    *ttratio;
    bool     *is_helix;
    GB_UINT4 *mut_sum;
    GB_UINT4 *freq_sum;
    char     *desc;
    float    *frequency[256];

public:
    ColumnStat(GBDATA *gb_main, AW_root *awr, const char *awar_template, AW_awar *awar_used_alignment);
    ~ColumnStat();

    __ATTR__USERESULT GB_ERROR calculate(AP_filter *filter);
    void     forget();

    GBDATA *get_gb_main() const { return gb_main; }

    bool has_valid_alignment() const { return !strchr(alignment_name, '?'); }

    const char *get_awar_smooth() const { return awar_smooth; }
    const char *get_awar_enable_helix() const { return awar_enable_helix; }
    const char *get_type_path() const { arb_assert(has_valid_alignment()); return type_path; }

    bool has_rates() const { return rates != NULL; }
    void weight_by_inverseRates() const;

    size_t get_length() const { return seq_len; }

    const GB_UINT4 *get_weights() const { return weights; }
    const float *get_rates() const { return rates; }
    const float *get_ttratio() const { return ttratio; }
    const bool *get_is_helix() const { return is_helix; }
    const float *get_frequencies(unsigned char c) const { return frequency[c]; }

    void create_sai_selection_list(AW_window *aww);
    void refresh_sai_selection_list();

    void print();
};

AW_window *COLSTAT_create_selection_window(AW_root *root, ColumnStat *column_stat);
void       COLSTAT_create_selection_list(AW_window *aww, ColumnStat *column_stat);

#else
#error ColumnStat.hxx included twice
#endif // COLUMNSTAT_HXX
