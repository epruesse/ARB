// ==================================================================== //
//                                                                      //
//   File      : awt_sel_boxes.hxx                                      //
//   Purpose   :                                                        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef AWT_SEL_BOXES_HXX
#define AWT_SEL_BOXES_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

class AP_filter;
struct adfiltercbstruct;

typedef char *(*awt_sai_sellist_filter)(GBDATA *, AW_CL);
class AWT_sai_selection;

class SAI_selection_list_spec : virtual Noncopyable {
    char   *awar_name;
    GBDATA *gb_main;

    awt_sai_sellist_filter filter_poc;
    AW_CL                  filter_cd;

public:
    SAI_selection_list_spec(const char *awar_name_, GBDATA *gb_main_)
        : awar_name(strdup(awar_name_)),
          gb_main(gb_main_),
          filter_poc(NULL),
          filter_cd(0)
    {}
    ~SAI_selection_list_spec() { free(awar_name); }

    void define_filter(awt_sai_sellist_filter filter_poc_, AW_CL filter_cd_) {
        // Warning: do not use different filters for same awar! (wont work as expected)
        filter_poc = filter_poc_;
        filter_cd  = filter_cd_;
    }

    const char *get_awar_name() const { return awar_name; }

    AWT_sai_selection *create_list(AW_window *aws) const;
};

// -----------------------------------------
//      various database selection boxes

class AW_DB_selection;

AW_DB_selection *awt_create_selection_list_on_alignments(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match);
void awt_reconfigure_selection_list_on_alignments(AW_DB_selection *alisel, const char *ali_type_match);

AW_DB_selection *awt_create_selection_list_on_trees(GBDATA *gb_main, AW_window *aws, const char *varname);

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, bool popup);
void awt_edit_arbtcpdat_cb(AW_window *aww, AW_CL cl_gb_main);

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws, const char *varname);
void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws, const char *tablename, const char *varname);
AW_window *AWT_create_tables_admin_window(AW_root *aw_root, GBDATA *gb_main);

AWT_sai_selection *awt_create_selection_list_on_sai(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc = 0, AW_CL filter_cd = 0);
void awt_selection_list_on_sai_update_cb(GBDATA *dummy, AWT_sai_selection *cbsid);
void awt_popup_sai_selection_list(AW_root *aw_root, AW_CL cl_awar_name, AW_CL cl_gb_main);
void awt_popup_sai_selection_list(AW_window *aww, AW_CL cl_awar_name, AW_CL cl_gb_main);
void awt_popup_filtered_sai_selection_list(AW_root *aw_root, AW_CL cl_sai_sellist_spec);
void awt_popup_filtered_sai_selection_list(AW_window *aww, AW_CL cl_sai_sellist_spec);
void awt_create_SAI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc = 0, AW_CL filter_cd = 0);

void  awt_create_selection_list_on_configurations(GBDATA *gb_main, AW_window *aws, const char *varname);
char *awt_create_string_on_configurations(GBDATA *gb_main);

AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*), AW_window* (*create_popup)(AW_root *, AW_default));

// -------------------------------

class AW_selection;
class AW_selection_list;
class ConstStrArray;

AW_selection *awt_create_subset_selection_list(AW_window *aww, AW_selection_list *select_subset_from, const char *at_box, const char *at_add, const char *at_sort);

// -------------------------------

AW_window *create_save_box_for_selection_lists(AW_root *aw_root, AW_CL selid);
AW_window *create_load_box_for_selection_lists(AW_root *aw_root, AW_CL selid);
void create_print_box_for_selection_lists(AW_window *aw_window, AW_CL selid);

#else
#error awt_sel_boxes.hxx included twice
#endif // AWT_SEL_BOXES_HXX

