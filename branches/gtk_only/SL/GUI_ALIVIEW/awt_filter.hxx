// =========================================================== //
//                                                             //
//   File      : awt_filter.hxx                                //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_FILTER_HXX
#define AWT_FILTER_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

class AW_window;
class AW_root;
class AW_selection_list;
class AP_filter;

typedef long AW_CL;

struct adfiltercbstruct {
    AW_window *aw_filt;
    AW_root   *awr;
    GBDATA    *gb_main;

    AW_selection_list *filterlist;

    char *def_name;
    char *def_2name;
    char *def_2filter;
    char *def_2alignment;
    char *def_subname;
    char *def_alignment;
    char *def_simplify;
    char *def_source;
    char *def_dest;
    char *def_cancel;
    char *def_filter;
    char *def_min;
    char *def_max;
    char *def_len;
    char  may_be_an_error;

};

adfiltercbstruct *awt_create_select_filter(AW_root *aw_root, GBDATA *gb_main, const char *def_name);
void awt_set_awar_to_valid_filter_good_for_tree_methods(GBDATA *gb_main, AW_root *awr, const char *awar_name);

AW_window *awt_create_select_filter_win(AW_root *aw_root, adfiltercbstruct *acbs);

AP_filter *awt_get_filter(adfiltercbstruct *acbs);
void       awt_destroy_filter(AP_filter *filter);

GB_ERROR awt_invalid_filter(AP_filter *filter);

char *AWT_get_combined_filter_name(AW_root *aw_root, GB_CSTR prefix);

#else
#error awt_filter.hxx included twice
#endif // AWT_FILTER_HXX
