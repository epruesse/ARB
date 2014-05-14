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

#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef CB_H
#include <cb.h>
#endif
#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

class AP_filter;
class AW_DB_selection;
class AW_selection;
class AW_selection_list;
struct StrArray;
struct ConstStrArray;
struct CharPtrArray;
struct adfiltercbstruct;

class TypedSelectionList {
    AW_selection_list& selection_list;

    std::string content;     // what is contained in the selection list ? (e.g. "probes")
    std::string filetype_id; // shared by all selection lists with same content type; used as file-extension for save/load 
    std::string unique_id;   // a unique id

public:
    TypedSelectionList(const char *filetype_id_, AW_selection_list *selection_list_, const char *content_, const char *unique_id_)
        : selection_list(*selection_list_),
          content(content_),
          filetype_id(filetype_id_), 
          unique_id(unique_id_)
    {}

    AW_selection_list *get_sellist() const { return &selection_list; }
    const char *whats_contained() const { return content.c_str(); }
    const char *get_unique_id() const { return unique_id.c_str(); }
    const char *get_shared_id() const { return filetype_id.c_str(); }
};

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

    AWT_sai_selection *create_list(AW_window *aws, bool fallback2default) const;
};

// -----------------------------------------
//      various database selection boxes

AW_DB_selection *awt_create_selection_list_on_alignments(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match);
void awt_reconfigure_selection_list_on_alignments(AW_DB_selection *alisel, const char *ali_type_match);

AW_DB_selection *awt_create_selection_list_on_trees(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default);

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, bool popup);
void awt_edit_arbtcpdat_cb(AW_window *aww, GBDATA *gb_main);

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws, const char *varname);
void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws, const char *tablename, const char *varname);
AW_window *AWT_create_tables_admin_window(AW_root *aw_root, GBDATA *gb_main);

AWT_sai_selection *awt_create_selection_list_on_sai(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default, awt_sai_sellist_filter filter_poc = 0, AW_CL filter_cd = 0);
void awt_selection_list_on_sai_update_cb(UNFIXED, AWT_sai_selection *cbsid);
void awt_popup_sai_selection_list(AW_window *aww, const char *awar_name, GBDATA *gb_main);
void awt_create_SAI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc = 0, AW_CL filter_cd = 0);

void  awt_create_selection_list_on_configurations(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default);
char *awt_create_string_on_configurations(GBDATA *gb_main);

// -------------------------------

AW_selection *awt_create_subset_selection_list(AW_window *aww, AW_selection_list *select_subset_from, const char *at_box, const char *at_add, const char *at_sort);

// -------------------------------
//      generic file prompter

AW_window *awt_create_load_box(AW_root                *aw_root,
                               const char             *action,
                               const char             *what,
                               const char             *default_directory,
                               const char             *file_extension,
                               char                  **set_file_name_awar,
                               const WindowCallback&   ok_cb,
                               const WindowCallback&   abort_cb          = makeWindowCallback(AW_POPDOWN),
                               const char             *close_button_text = NULL);

// ------------------------------------------
//      save/load selection list content

typedef GB_ERROR (*ssl_to_storage)(const CharPtrArray& display, const CharPtrArray& value, StrArray& line);
typedef GB_ERROR (*ssl_from_storage)(const CharPtrArray& line, StrArray& display, StrArray& value);

class StorableSelectionList {
    TypedSelectionList tsl;
    ssl_to_storage     list2file;
    ssl_from_storage   file2list;

public:
    StorableSelectionList(const TypedSelectionList& tsl_);
    StorableSelectionList(const TypedSelectionList& tsl_, ssl_to_storage list2file_, ssl_from_storage file2list_)
        : tsl(tsl_),
          list2file(list2file_),
          file2list(file2list_)
    {}

    const TypedSelectionList& get_typedsellist() const { return tsl; }
    const char *get_filter() const { return tsl.get_shared_id(); }

    GB_ERROR save(const char *filename, long number_of_lines = 0) const;
    GB_ERROR load(const char *filemask, bool append) const;
};

AW_window *create_save_box_for_selection_lists(AW_root *aw_root, const StorableSelectionList *storabsellist);
AW_window *create_load_box_for_selection_lists(AW_root *aw_root, const StorableSelectionList *storabsellist);

void create_print_box_for_selection_lists(AW_window *aw_window, const TypedSelectionList *typedsellist);

void awt_clear_selection_list_cb(AW_window *, AW_selection_list *sellist);

#else
#error awt_sel_boxes.hxx included twice
#endif // AWT_SEL_BOXES_HXX

