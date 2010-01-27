// =========================================================== //
//                                                             //
//   File      : awt_csp.hxx                                   //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_CSP_HXX
#define AWT_CSP_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif


/*
  Create a window, that allows you to get weights from the sais
  1.  create AWT_csp
  2.  build button with callback create_csp_window
  3.  call csp->go( second_filter)
  4.  use csp->weights ....
*/

class AW_root;
class AW_window;
class AP_filter;

#define AWT_CSP_AWAR_CSP_NAME         "/name=/name"
#define AWT_CSP_AWAR_CSP_ALIGNMENT    "/name=/alignment"
#define AWT_CSP_AWAR_CSP_SMOOTH       "/name=/smooth"
#define AWT_CSP_AWAR_CSP_ENABLE_HELIX "/name=/enable_helix"

#define DIST_MIN_SEQ(seq_anz) (seq_anz / 10)

class AWT_csp {
    GBDATA  *gb_main;
    AW_root *awr;

    char *awar_name;
    char *awar_alignment;
    char *awar_smooth;
    char *awar_enable_helix;

    char *alignment_name;
    char *type_path;
    void *sai_sel_box_id;

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
    AWT_csp(GBDATA *gb_main, AW_root *awr, const char *awar_template);
    ~AWT_csp();

    GB_ERROR go(AP_filter *filter);
    void     exit();

    GBDATA *get_gb_main() const { return gb_main; }

    const char *get_awar_smooth() const { return awar_smooth; }
    const char *get_awar_enable_helix() const { return awar_enable_helix; }
    const char *get_type_path() const { return type_path; }

    bool has_rates() const { return rates != NULL; }
    void weight_by_inverseRates() const;

    size_t get_length() const { return seq_len; }

    const GB_UINT4 *get_weights() const { return weights; }
    const float *get_rates() const { return rates; }
    const float *get_ttratio() const { return ttratio; }
    const bool *get_is_helix() const { return is_helix; }
    const float *get_frequencies(unsigned char c) const { return frequency[c]; }

    void create_sai_selection(AW_window *aww);
    void refresh_sai_selection();

    void print();
};

AW_window *create_csp_window(AW_root *root, AWT_csp *csp);
void       create_selection_list_on_csp(AW_window *aww, AWT_csp *csp);


#else
#error awt_csp.hxx included twice
#endif // AWT_CSP_HXX
