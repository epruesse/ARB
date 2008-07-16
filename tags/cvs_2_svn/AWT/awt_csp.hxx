#ifndef awt_csp_hxx_included
#define awt_csp_hxx_included


/*
  Create a window, that allows you to get weights from the sais
  1.  create AWT_csp
  2.  build button with callback create_csp_window
  3.  call csp->go( second_filter)
  4.  use csp->weights ....
*/

class AW_root;
class AW_window;

#define AWT_CSP_AWAR_CSP_NAME "/name=/name"
#define AWT_CSP_AWAR_CSP_ALIGNMENT "/name=/alignment"
#define AWT_CSP_AWAR_CSP_SMOOTH "/name=/smooth"
#define AWT_CSP_AWAR_CSP_ENABLE_HELIX "/name=/enable_helix"

#define DIST_MIN_SEQ(seq_anz) (seq_anz / 10)

class AWT_csp {
public:
    GBDATA *gb_main;
    AW_root *awr;

    char *awar_name;
    char *awar_alignment;
    char *alignment_name;
    char *type_path;
    char *awar_smooth;
    char *awar_enable_helix;
    void *sai_sel_box_id;
    /* real public */
    size_t  seq_len;    // real length == 0 -> not valid
    GB_UINT4 *weights;  // helix = 1, non helix == 2
    float   *rates;
    float   *ttratio;
    float   *frequency[256];
    GB_UINT4 *mut_sum;
    GB_UINT4 *freq_sum;
    unsigned char *is_helix;  // == 1 -> helix; == 0 -> loop region
    char    *desc;

    AWT_csp(GBDATA *gb_main, AW_root *awr, const char *awar_template);
    /* awar_template ==     ".../name"
       -> generated        ".../alignment"
       ".../smooth"
       ".../enable_helix"
    */
    ~AWT_csp(void);
    void exit();
    GB_ERROR go(AP_filter *filter = 0);
    void print(void);
};

AW_window *create_csp_window(AW_root *root, AWT_csp *csp);

void create_selection_list_on_csp(AW_window *aww, AWT_csp *csp);





#endif
