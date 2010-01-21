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

#ifndef ARBDBT_H
#include <arbdbt.h>
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
    
    AW_selection_list *id;

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

/***********************    FILTERS     ************************/

adfiltercbstruct *awt_create_select_filter(AW_root *aw_root, GBDATA *gb_main, const char *def_name);
/* Create a data structure for filters (needed for awt_create_select_filter_win */
/* Create a filter selection box, this box needs 3 AWARS:
   1. "$def_name"
   2. "$def_name:/name=/filter"
   3. "$def_name:/name=/alignment"
   and some internal awars
*/
void awt_set_awar_to_valid_filter_good_for_tree_methods(GBDATA *gb_main, AW_root *awr, const char *awar_name);

AW_window *awt_create_select_filter_win(AW_root *aw_root, AW_CL cd_adfiltercbstruct);
/* not just the box, but the whole window */

AP_filter *awt_get_filter(AW_root *aw_root, adfiltercbstruct *acbs);

char *AWT_get_combined_filter_name(AW_root *aw_root, GB_CSTR prefix);

#else
#error awt_filter.hxx included twice
#endif // AWT_FILTER_HXX
