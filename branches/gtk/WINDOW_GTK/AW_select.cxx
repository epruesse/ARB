// ============================================================= //
//                                                               //
//   File      : AW_select.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_select.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_root.hxx"
#include "aw_awar.hxx"
#ifndef ARBDB_H
#include <arbdb.h>
#endif

#include <gtk-2.0/gtk/gtktreeview.h>
#include <string>
#include <vector>
#include <gtk-2.0/gtk/gtktreestore.h>

__ATTR__NORETURN inline void type_mismatch(const char *triedType, const char *intoWhat) {
    GBK_terminatef("Cannot insert %s into %s which uses a non-%s AWAR", triedType, intoWhat, triedType);
}

__ATTR__NORETURN inline void selection_type_mismatch(const char *triedType) { type_mismatch(triedType, "selection-list"); }
__ATTR__NORETURN inline void option_type_mismatch(const char *triedType) { type_mismatch(triedType, "option-menu"); }
__ATTR__NORETURN inline void toggle_type_mismatch(const char *triedType) { type_mismatch(triedType, "toggle"); }


AW_selection_list::AW_selection_list(const char *variable_namei, int variable_typei,
                                     GtkTreeView *select_list_widgeti) :
        variable_name(nulldup(variable_namei)),
        variable_type((AW_VARIABLE_TYPE)variable_typei),
        select_list_widget(select_list_widgeti),
        list_table(NULL),
        last_of_list_table(NULL),
        default_select(NULL),
        list_model(GTK_LIST_STORE(gtk_tree_view_get_model(select_list_widgeti))),
        next(NULL)
{
    aw_assert(NULL != select_list_widget);
    aw_assert(NULL != list_model);

}

void AW_selection::refresh() {
    GTK_NOT_IMPLEMENTED;
}


void AW_selection_list::clear(bool clear_default) {
    while (list_table) {
        AW_selection_list_entry *nextEntry = list_table->next;
        delete list_table;
        list_table = nextEntry;
    }
    list_table = NULL;
    last_of_list_table = NULL;

    if (clear_default && default_select) {
        delete default_select;
        default_select = NULL;
    }
    
    gtk_tree_store_clear(GTK_TREE_STORE(gtk_tree_view_get_model(select_list_widget)));
    
}

bool AW_selection_list::default_is_selected() const {
    GTK_NOT_IMPLEMENTED;
    return false;
}

void AW_selection_list::delete_element_at(int /*index*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::delete_value(char const */*value*/) {
    GTK_NOT_IMPLEMENTED;
}
const char *AW_selection_list::get_awar_value() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *AW_selection_list::get_content_as_string(long /*number_of_lines*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

const char *AW_selection_list::get_default_display() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

const char * AW_selection_list::get_default_value() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

int AW_selection_list::get_index_of(char const */*searched_value*/) {
    GTK_NOT_IMPLEMENTED;
        return 0;
}

int AW_selection_list::get_index_of_selected() {
    GTK_NOT_IMPLEMENTED;
        return 0;
}

void AW_selection_list::init_from_array(const CharPtrArray& /*entries*/, const char */*defaultEntry*/) {
    GTK_NOT_IMPLEMENTED;

}

void AW_selection_list::insert(const char *displayed, const char *value) {
    
    AW_selection_list_entry* entry = insert_generic(displayed, value, AW_STRING);
    if(NULL != entry)
    {
        appendToListStore(last_of_list_table);
    }
}

void AW_selection_list::appendToListStore(AW_selection_list_entry* entry)
{
    GtkListStore *store;
    GtkTreeIter iter;

    aw_assert(NULL != entry);
    
    //note: gtk crashes if the store is an attribute.
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(select_list_widget)));

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, entry->get_displayed(), -1);    
}

void AW_selection_list::insert_default(const char *displayed, const char *value) {
    if (variable_type != AW_STRING) {
        selection_type_mismatch("string");
        return;
    }
    if (default_select) delete default_select;
    default_select = new AW_selection_list_entry(displayed, value);
}

void AW_selection_list::insert(const char *displayed, int32_t value) {
    AW_selection_list_entry* entry = insert_generic(displayed, value, AW_INT);
    if(NULL != entry)
    {
        appendToListStore(last_of_list_table);
    }
}

void AW_selection_list::insert_default(const char *displayed, int32_t value) {
    if (variable_type != AW_INT) {
        selection_type_mismatch("int");
        return;
    }
    if (default_select) {
        delete default_select;
    }
    default_select = new AW_selection_list_entry(displayed, value);
}

void AW_selection_list::insert(const char *displayed, GBDATA *pointer) {
    AW_selection_list_entry* entry = insert_generic(displayed, value, AW_POINTER);
    if(NULL != entry)
    {
        appendToListStore(last_of_list_table);
    }
}

void AW_selection_list::insert_default(const char *displayed, GBDATA *pointer) {
    if (variable_type != AW_POINTER) {
        selection_type_mismatch("pointer");
        return;
    }
    if (default_select) delete default_select;
    default_select = new AW_selection_list_entry(displayed, pointer);
}


void AW_selection_list::move_content_to(AW_selection_list */*target_list*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::move_selection(int /*offset*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::select_element_at(int /*wanted_index*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::set_awar_value(char const */*new_value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::set_file_suffix(char const */*suffix*/) {
    GTK_NOT_IMPLEMENTED;
}

size_t AW_selection_list::size() {
    AW_selection_list_entry *lt    = list_table;
    size_t                  count = 0;

    while (lt) {
        ++count;
        lt = lt->next;
    }
    return count;
}

void AW_selection_list::sort(bool /*backward*/, bool /*case_sensitive*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::to_array(StrArray& /*array*/, bool /*values*/) {
    GTK_NOT_IMPLEMENTED;
}

GB_HASH *AW_selection_list::to_hash(bool /*case_sens*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_selection_list::update() {      
    //note: This method is deprecated
}

char *AW_selection_list_entry::copy_string_for_display(const char *str) {
    char *out = strdup(str);
    char *p   = out;
    int   ch;

    while ((ch=*(p++)) != 0) {
        if (ch==',')
            p[-1] = ';';
        if (ch=='\n')
            p[-1] = '#';
    }
    return out;
}

void AW_selection_list::refresh() {
    GTK_NOT_IMPLEMENTED;
//    if (!variable_name) return;     // not connected to awar
//
//    AW_root *root  = AW_root::SINGLETON;
//    bool     found = false;
//    int      pos   = 0;
//    AW_awar *awar  = root->awar(variable_name);
//
//    AW_selection_list_entry *lt;
//
//    switch (variable_type) {
//        case AW_STRING: {
//            char *var_value = awar->read_string();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (strcmp(var_value, lt->value.get_string()) == 0) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            free(var_value);
//            break;
//        }
//        case AW_INT: {
//            int var_value = awar->read_int();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (var_value == lt->value.get_int()) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            break;
//        }
//        case AW_FLOAT: {
//            float var_value = awar->read_float();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (var_value == lt->value.get_float()) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            break;
//        }
//        case AW_POINTER: {
//            GBDATA *var_value = awar->read_pointer();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (var_value == lt->value.get_pointer()) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            break;
//        }
//        default:
//            aw_assert(0);
//            GB_warning("Unknown AWAR type");
//            break;
//    }
//
//    if (found || default_select) {
//        pos++;
//        int top;
//        int vis;
//        XtVaGetValues(select_list_widget,
//                      XmNvisibleItemCount, &vis,
//                      XmNtopItemPosition, &top,
//                      NULL);
//        XmListSelectPos(select_list_widget, pos, False);
//
//        if (pos < top) {
//            if (pos > 1) pos --;
//            XmListSetPos(select_list_widget, pos);
//        }
//        if (pos >= top + vis) {
//            XmListSetBottomPos(select_list_widget, pos + 1);
//        }
//    }
//    else {
//        GBK_terminatef("Selection list '%s' has no default selection", variable_name);
//    }
}


//AW_DB_selection
//TODO move to own file
AW_DB_selection::AW_DB_selection(AW_selection_list *sellist_, GBDATA *gbd_)
    : AW_selection(sellist_)
    , gbd(gbd_)
{
    GTK_NOT_IMPLEMENTED;
}

AW_DB_selection::~AW_DB_selection() {
    GTK_NOT_IMPLEMENTED;
}

GBDATA *AW_DB_selection::get_gb_main() {
    return GB_get_root(gbd); 
}

