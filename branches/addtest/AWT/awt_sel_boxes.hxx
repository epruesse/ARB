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

typedef char *(*awt_sai_sellist_filter)(GBDATA *, AW_CL); // returns NULL for unwanted SAI; heap-allocated selection-list display-string for wanted

// -----------------------------------------
//      various database selection boxes

void awt_create_ALI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match);
AW_DB_selection *awt_create_ALI_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match);
void awt_reconfigure_ALI_selection_list(AW_DB_selection *alisel, const char *ali_type_match);

AW_DB_selection *awt_create_TREE_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default);

void awt_create_PTSERVER_selection_button(AW_window *aws, const char *varname);
void awt_create_PTSERVER_selection_list(AW_window *aws, const char *varname);

void awt_create_SAI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc = 0, AW_CL filter_cd = 0);
AW_DB_selection *awt_create_SAI_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default, awt_sai_sellist_filter filter_poc = 0, AW_CL filter_cd = 0);
void awt_popup_SAI_selection_list(AW_window *aww, const char *awar_name, GBDATA *gb_main);

AW_DB_selection *awt_create_CONFIG_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default);

// ---------------------------
//      related functions

void awt_edit_arbtcpdat_cb(AW_window *aww, GBDATA *gb_main);
char *awt_create_CONFIG_string(GBDATA *gb_main);

// --------------------------
//      subset selection

typedef       void (*SubsetChangedCb)(AW_selection*, bool interactive_change, AW_CL cl_user);
AW_selection *awt_create_subset_selection_list(AW_window *aww, AW_selection_list *select_subset_from, const char *at_box, const char *at_add, const char *at_sort, bool autocorrect_subselection = true, SubsetChangedCb subChanged_cb = NULL, AW_CL cl_user = 0);
void          awt_set_subset_selection_content(AW_selection *subset_sel_, const CharPtrArray& values);

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

AW_selection_list *awt_create_selection_list_with_input_field(AW_window *aww, const char *awar_name, const char *at_box, const char *at_field);

#else
#error awt_sel_boxes.hxx included twice
#endif // AWT_SEL_BOXES_HXX

