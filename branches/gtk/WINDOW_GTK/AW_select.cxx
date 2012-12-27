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

#include <gtk/gtktreeview.h>
#include <string>
#include <list>
#include <gtk/gtktreestore.h>
#include <gtk/gtkliststore.h>


#ifndef ARBDB_H
#include <arbdb.h>
#endif

#ifndef ARB_STRBUF_H
#include <arb_strbuf.h>
#endif

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
        next(NULL)
{
    aw_assert(NULL != select_list_widget);

}

void AW_selection::refresh() {
    sellist->clear();
    fill();
    sellist->update();
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
    
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(select_list_widget)));
    
}

bool AW_selection_list::default_is_selected() const {
    return strcmp(get_selected_value(), get_default_value()) == 0;
}


const char *AW_selection_list::get_selected_value() const { // @@@ refactor
    GTK_NOT_IMPLEMENTED;
    return "NOT IMPL";
//    int                      i;
//    AW_selection_list_entry *lt;
//    AW_selection_list_entry *found = 0;
//    
//    for (i=1, lt = list_table; lt; i++, lt = lt->next) {
//        lt->is_selected = XmListPosSelected(select_list_widget, i);
//        if (lt->is_selected && !found) found = lt;
//    }
//
//    if (default_select) {
//        default_select->is_selected = XmListPosSelected(select_list_widget, i);
//        if (default_select->is_selected && !found) found = default_select;
//    }
//    return found ? found->value.get_string() : NULL;
}

AW_selection_list_entry *AW_selection_list::get_entry_at(int index) {
    AW_selection_list_entry *entry = list_table;
    while (index && entry) {
        entry = entry->next;
        index--;
    }
    return entry;
}


void AW_selection_list::select_default() {
    set_awar_value(get_default_value()); 
}

void AW_selection_list::delete_element_at(const int index) {
    if (index<0) return;
    
    AW_selection_list_entry *prev = NULL;
    if (index>0) {
        prev = get_entry_at(index-1);
        if (!prev) return; // invalid index
    }
    
    int selected_index = get_index_of_selected();
    if (index == selected_index) select_default();

    AW_selection_list_entry *toDel = prev ? prev->next : list_table;
    aw_assert(toDel != default_select);

    (prev ? prev->next : list_table) = toDel->next;
    delete toDel;

    if (last_of_list_table == toDel) last_of_list_table = prev;
    
}


void AW_selection_list::delete_value(char const */*value*/) {
    GTK_NOT_IMPLEMENTED;
}

const char *AW_selection_list::get_awar_value() const {
    return AW_root::SINGLETON->awar(variable_name)->read_char_pntr();
}

char *AW_selection_list::get_content_as_string(long number_of_lines) {
    // number_of_lines == 0 -> print all

    AW_selection_list_entry *lt;
    GBS_strstruct *fd = GBS_stropen(10000);

    for (lt = list_table; lt; lt = lt->next) {
        number_of_lines--;
        GBS_strcat(fd, lt->get_displayed());
        GBS_chrcat(fd, '\n');
        if (!number_of_lines) break;
    }
    return GBS_strclose(fd);
}

const char *AW_selection_list::get_default_display() const {
        return default_select ? default_select->get_displayed() : NULL;
}

const char * AW_selection_list::get_default_value() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

int AW_selection_list::get_index_of(const char *searched_value) {
    /*! get index of an entry in the selection list
     * @return 0..n-1 index of matching element (or -1)
     */
    int element_index = 0;
    for (AW_selection_list_iterator entry(this); entry; ++entry) {
        if (strcmp(entry.get_value(), searched_value) == 0) return element_index;
        ++element_index;
    }
    return -1;
}

int AW_selection_list::get_index_of_selected() {
    // returns index of element (or index of default)
    const char *awar_value = get_awar_value();
    return get_index_of(awar_value);
}

void AW_selection_list::init_from_array(const CharPtrArray& /*entries*/, const char */*defaultEntry*/) {
    GTK_NOT_IMPLEMENTED;

}

void AW_selection_list::insert(const char *displayed, const char *value) {
    
    AW_selection_list_entry* entry = insert_generic(displayed, value, AW_STRING);
    aw_assert(NULL != entry);
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
    aw_assert(NULL != entry);
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
    AW_selection_list_entry* entry = insert_generic(displayed, pointer, AW_POINTER);
    aw_assert(NULL != entry);
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
    // Warning:
    // update() will not set the connected awar to the default value
    // if it contains a value which is not associated with a list entry!

    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(select_list_widget)));
    
    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        appendToListStore(lt);
    }

    if (default_select) {
        appendToListStore(default_select);
    }

    refresh();
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

