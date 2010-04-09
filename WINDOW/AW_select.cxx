// ================================================================ //
//                                                                  //
//   File      : AW_select.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "aw_select.hxx"
#include "aw_window_Xm.hxx"

#include <Xm/List.h>

// --------------------------
//      AW_selection_list

void AW_selection_list::selectAll() {
    int i;
    AW_select_table_struct *lt;
    for (i=0, lt = list_table; lt; i++, lt = lt->next) {
        XmListSelectPos(select_list_widget, i, False);
    }
    if (default_select) {
        XmListSelectPos(select_list_widget, i, False);
    }
}

void AW_selection_list::deselectAll() {
    XmListDeselectAllItems(select_list_widget);
}


const char *AW_selection_list::first_element() {
    AW_select_table_struct *lt;
    for (lt = list_table; lt; lt = lt->next) {
        lt->is_selected = 1;
    }
    loop_pntr = list_table;
    if (!loop_pntr) return 0;
    return loop_pntr->char_value;
}

const char *AW_selection_list::next_element() {
    if (!loop_pntr) return 0;
    loop_pntr = loop_pntr->next;
    if (!loop_pntr) return 0;
    while (loop_pntr && !loop_pntr->is_selected) loop_pntr=loop_pntr->next;
    if (!loop_pntr) return 0;
    return loop_pntr->char_value;
}

const char *AW_selection_list::first_selected() {
    int i;
    AW_select_table_struct *lt;
    loop_pntr = 0;
    for (i=1, lt = list_table; lt; i++, lt = lt->next) {
        lt->is_selected = XmListPosSelected(select_list_widget, i);
        if (lt->is_selected && !loop_pntr) loop_pntr = lt;
    }
    if (default_select) {
        default_select->is_selected = XmListPosSelected(select_list_widget, i);
        if (default_select->is_selected && !loop_pntr) loop_pntr = default_select;
    }
    return loop_pntr ? loop_pntr->char_value : NULL;
}

const char *AW_selection_list::get_awar_value(AW_root *aw_root) const {
    return aw_root->awar(variable_name)->read_char_pntr();
}

void AW_selection_list::set_awar_value(AW_root *aw_root, const char *new_value) {
    aw_root->awar(variable_name)->write_string(new_value);
}

const char *AW_selection_list::get_default_value() const {
    return default_select ? default_select->char_value : NULL;
}

size_t AW_selection_list::size() {
    AW_select_table_struct *lt    = list_table;
    size_t                  count = 0;

    while (lt) {
        ++count;
        lt = lt->next;
    }
    return count;
}

// ---------------------
//      AW_selection

void AW_selection::refresh() {
    get_win()->clear_selection_list(get_sellist());
    fill();
    get_win()->update_selection_list(get_sellist());
}


// -------------------------
//      AW_DB_selection

void AW_DB_selection_refresh_cb(GBDATA *, int *cl_selection, GB_CB_TYPE) {
    AW_DB_selection *selection = (AW_DB_selection*)cl_selection;;
    selection->refresh();
}

AW_DB_selection::AW_DB_selection(AW_window *win_, AW_selection_list *sellist_, GBDATA *gbd_)
    : AW_selection(win_, sellist_)
    , gbd(gbd_)
{
    GB_transaction ta(gbd);
    GB_add_callback(gbd, GB_CB_CHANGED, AW_DB_selection_refresh_cb, (int*)this);
}

AW_DB_selection::~AW_DB_selection() {
    GB_transaction ta(gbd);
    GB_remove_callback(gbd, GB_CB_CHANGED, AW_DB_selection_refresh_cb, (int*)this);
}

