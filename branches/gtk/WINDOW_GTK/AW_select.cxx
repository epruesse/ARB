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
#include <gtk/gtktreeselection.h>

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#ifndef ARB_STRBUF_H
#include <arb_strbuf.h>
#endif

#ifndef ARB_SORT_H
#include <arb_sort.h>
#endif

#ifndef ARB_STRARRAY_H
#include <arb_strarray.h>
#endif

#ifndef ARB_STR_H
#include <arb_str.h>
#endif

__ATTR__NORETURN inline void type_mismatch(const char *triedType, const char *intoWhat) {
    GBK_terminatef("Cannot insert %s into %s which uses a non-%s AWAR", triedType, intoWhat, triedType);
}

__ATTR__NORETURN inline void selection_type_mismatch(const char *triedType) { type_mismatch(triedType, "selection-list"); }
__ATTR__NORETURN inline void option_type_mismatch(const char *triedType) { type_mismatch(triedType, "option-menu"); }
__ATTR__NORETURN inline void toggle_type_mismatch(const char *triedType) { type_mismatch(triedType, "toggle"); }

static int AW_sort_AW_select_table_struct(const void *t1, const void *t2, void *) {
    return strcmp(static_cast<const AW_selection_list_entry*>(t1)->get_displayed(),
                  static_cast<const AW_selection_list_entry*>(t2)->get_displayed());
}
static int AW_sort_AW_select_table_struct_backward(const void *t1, const void *t2, void *) {
    return strcmp(static_cast<const AW_selection_list_entry*>(t2)->get_displayed(),
                  static_cast<const AW_selection_list_entry*>(t1)->get_displayed());
}
static int AW_isort_AW_select_table_struct(const void *t1, const void *t2, void *) {
    return ARB_stricmp(static_cast<const AW_selection_list_entry*>(t1)->get_displayed(),
                       static_cast<const AW_selection_list_entry*>(t2)->get_displayed());
}
static int AW_isort_AW_select_table_struct_backward(const void *t1, const void *t2, void *) {
    return ARB_stricmp(static_cast<const AW_selection_list_entry*>(t2)->get_displayed(),
                       static_cast<const AW_selection_list_entry*>(t1)->get_displayed());
}



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


int AW_selection_list::get_selected_index() const {
    //note: copy&paste from http://ubuntuforums.org/showthread.php?t=1208655
    GtkTreeSelection *tsel = gtk_tree_view_get_selection (select_list_widget);
    GtkTreeIter iter;
    GtkTreeModel *tm;
    GtkTreePath *path;
    int *i;
    if (gtk_tree_selection_get_selected (tsel , &tm , &iter))
        {
        path = gtk_tree_model_get_path (tm , &iter);
        i = gtk_tree_path_get_indices(path);
        return i[0];
        }
    return -1;
}

const char *AW_selection_list::get_selected_value() const { // @@@ refactor
    FIXME("untested code");
    AW_selection_list_entry *lt;
    AW_selection_list_entry *found = 0;
    GtkTreeIter              iter;
    GtkTreeModel             *model = GTK_TREE_MODEL(gtk_tree_view_get_model(select_list_widget));
    GtkTreeSelection         *selection = GTK_TREE_SELECTION(gtk_tree_view_get_selection(select_list_widget));

    //note: this loop only works if the iter walks the items in order
    if(gtk_tree_model_get_iter_first(model, &iter)) {//if the model is not empty
        for( lt = list_table; lt; lt=lt->next, gtk_tree_model_iter_next(model, &iter)) {
            lt->is_selected = gtk_tree_selection_iter_is_selected(selection, &iter);
            
            if(lt->is_selected && !found) found = lt;
        }
    }
    if (found) printf("selected : %s\n", found->value.get_string());
    return found ? found->value.get_string() : NULL;
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


void AW_selection_list::delete_value(const char *value) {
    int index = get_index_of(value);
    delete_element_at(index);
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

const char *AW_selection_list::get_default_value() const {
    return default_select ? default_select->value.get_string() : NULL;
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

void AW_selection_list::init_from_array(const CharPtrArray& entries, const char *defaultEntry) {
    // update selection list with contents of NULL-terminated array 'entries'
    // 'defaultEntry' is used as default selection
    // awar value will be changed to 'defaultEntry' if it does not match any other entry
    // Note: This works only with selection lists bound to AW_STRING awars.

    aw_assert(defaultEntry);
    char *defaultEntryCopy = strdup(defaultEntry); // use a copy (just in case defaultEntry point to a value free'd by clear_selection_list())
    bool  defInserted      = false;

    clear();
    for (int i = 0; entries[i]; ++i) {
        if (!defInserted && strcmp(entries[i], defaultEntryCopy) == 0) {
            insert_default(defaultEntryCopy, defaultEntryCopy);
            defInserted = true;
        }
        else {
            insert(entries[i], entries[i]);
        }
    }
    if (!defInserted) insert_default(defaultEntryCopy, defaultEntryCopy);
    update();

    const char *selected = get_selected_value();
    if (selected) set_awar_value(selected);

    free(defaultEntryCopy);
}

void AW_selection_list::insert(const char *displayed, const char *value) {
    
    AW_selection_list_entry* entry = insert_generic(displayed, value, AW_STRING);
    aw_assert(NULL != entry);
}

void AW_selection_list::append_to_liststore(AW_selection_list_entry* entry)
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


void AW_selection_list::move_content_to(AW_selection_list *target_list) {
    // @@@ instead of COPYING, it could simply move the entries (may cause problems with iterator)

    AW_selection_list_entry *entry = list_table;
    while (entry) {
        aw_assert(default_select != entry); // should not be possible
        
        if (!target_list->list_table) {
            target_list->last_of_list_table = target_list->list_table = new AW_selection_list_entry(entry->get_displayed(), entry->value.get_string());
        }
        else {
            target_list->last_of_list_table->next = new AW_selection_list_entry(entry->get_displayed(), entry->value.get_string());
            target_list->last_of_list_table = target_list->last_of_list_table->next;
            target_list->last_of_list_table->next = NULL;
        }

        entry = entry->next;
    }

    clear(false);
}

void AW_selection_list::move_selection(int offset) {
    /*! move selection 'offset' position
     *  offset == 1  -> select next element
     *  offset == -1 -> select previous element
     */
    
    int index = get_index_of_selected();
    select_element_at(index+offset);
}

const char *AW_selection_list::get_value_at(int index) {
    // get value of the entry at position 'index' [0..n-1] of the 'selection_list'
    // returns NULL if index is out of bounds
    AW_selection_list_entry *entry = get_entry_at(index);
    return entry ? entry->value.get_string() : NULL;
}

void AW_selection_list::select_element_at(int wanted_index) {
    const char *wanted_value = get_value_at(wanted_index);

    if (!wanted_value) {
        wanted_value = get_default_value();
        if (!wanted_value) wanted_value = "";
    }

    set_awar_value(wanted_value);
}

void AW_selection_list::set_awar_value(const char *new_value) {
    AW_root::SINGLETON->awar(variable_name)->write_string(new_value);
}

void AW_selection_list::set_file_suffix(const char *suffix) {
    AW_root *aw_root = AW_root::SINGLETON;
    char     filter[200];
    sprintf(filter, "tmp/save_box_sel_%li/filter", (long)this);
    aw_root->awar_string(filter, suffix);
    sprintf(filter, "tmp/load_box_sel_%li/filter", (long)this);
    aw_root->awar_string(filter, suffix);
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

void AW_selection_list::sort(bool backward, bool case_sensitive) {
    size_t count = size();
    if (count) {
        AW_selection_list_entry **tables = new AW_selection_list_entry *[count];
        count = 0;
        for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
            tables[count++] = lt;
        }

        gb_compare_function comparator;
        if (backward) {
            if (case_sensitive) comparator = AW_sort_AW_select_table_struct_backward;
            else comparator                = AW_isort_AW_select_table_struct_backward;
        }
        else {
            if (case_sensitive) comparator = AW_sort_AW_select_table_struct;
            else comparator                = AW_isort_AW_select_table_struct;
        }

        GB_sort((void**)tables, 0, count, comparator, 0);

        size_t i;
        for (i=0; i<count-1; i++) {
            tables[i]->next = tables[i+1];
        }
        tables[i]->next = 0;
        list_table = tables[0];
        last_of_list_table = tables[i];

        delete [] tables;
    }
}

void AW_selection_list::to_array(StrArray& array, bool values) {
    /*! read contents of selection list into an array.
     * @param values true->read values, false->read displayed strings
     * Use GBT_free_names() to free the result.
     *
     * Note: if 'values' is true, this function only works for string selection lists!
     */

    array.reserve(size());
    
    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        array.put(strdup(values ? lt->value.get_string() : lt->get_displayed()));
    }
    aw_assert(array.size() == size());
}

GB_HASH *AW_selection_list::to_hash(bool case_sens) {
    // creates a hash (key = value of selection list, value = display string from selection list)
    // (Warning: changing the selection list will render the hash invalid!)

    GB_HASH *hash = GBS_create_hash(size(), case_sens ? GB_MIND_CASE : GB_IGNORE_CASE);

    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        GBS_write_hash(hash, lt->value.get_string(), (long)lt->get_displayed());
    }

    return hash;
}

void AW_selection_list::update() {      
    // Warning:
    // update() will not set the connected awar to the default value
    // if it contains a value which is not associated with a list entry!

    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(select_list_widget)));
    
    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        append_to_liststore(lt);
    }

    if (default_select) {
        append_to_liststore(default_select);
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
//select the item that matches the awars value
   if (!variable_name) return;     // not connected to awar

   
    AW_root *root  = AW_root::SINGLETON;
    bool     found = false;
    AW_awar *awar  = root->awar(variable_name);
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(select_list_widget));
    AW_selection_list_entry *lt;
    gtk_tree_model_get_iter_first(model, &iter);
    
    switch (variable_type) {
        case AW_STRING: {
            char *var_value = awar->read_string();
            for (lt = list_table; lt; lt = lt->next) {
                if (strcmp(var_value, lt->value.get_string()) == 0) {
                    found = true;
                    break;
                }
                gtk_tree_model_iter_next(model, &iter);
            }
            free(var_value);
            break;
        }
        case AW_INT: {
            int var_value = awar->read_int();
            for (lt = list_table; lt; lt = lt->next) {
                if (var_value == lt->value.get_int()) {
                    found = true;
                    break;
                }
                gtk_tree_model_iter_next(model, &iter);
            }
            break;
        }
        case AW_FLOAT: {
            float var_value = awar->read_float();
            for (lt = list_table; lt; lt = lt->next) {
                if (var_value == lt->value.get_float()) {
                    found = true;
                    break;
                }
                gtk_tree_model_iter_next(model, &iter);
            }
            break;
        }
        case AW_POINTER: {
            GBDATA *var_value = awar->read_pointer();
            for (lt = list_table; lt; lt = lt->next) {
                if (var_value == lt->value.get_pointer()) {
                    found = true;
                    break;
                }
                gtk_tree_model_iter_next(model, &iter);
            }
            break;
        }
        default:
            aw_assert(0);
            GB_warning("Unknown AWAR type");
            break;
    }

    if (found || default_select) {
//        int top;
//        int vis;
//        XtVaGetValues(select_list_widget,
//                      XmNvisibleItemCount, &vis,
//                      XmNtopItemPosition, &top,
//                      NULL);
         gtk_tree_selection_select_iter(GTK_TREE_SELECTION(gtk_tree_view_get_selection(select_list_widget)), &iter);

        FIXME("position adjustment not implemented."); //NO why XmListSetPos is called here....
//        if (pos < top) {
//            if (pos > 1) pos --;
//            XmListSetPos(select_list_widget, pos);
//        }
//        if (pos >= top + vis) {
//            XmListSetBottomPos(select_list_widget, pos + 1);
//        }
    }
    else {
        GBK_terminatef("Selection list '%s' has no default selection", variable_name);
    }
}

static void AW_DB_selection_refresh_cb(GBDATA *, int *cl_selection, GB_CB_TYPE) {
    AW_DB_selection *selection = (AW_DB_selection*)cl_selection;;
    selection->refresh();
}

//AW_DB_selection
//TODO move to own file
AW_DB_selection::AW_DB_selection(AW_selection_list *sellist_, GBDATA *gbd_)
    : AW_selection(sellist_)
    , gbd(gbd_)
{
    GB_transaction ta(gbd);
    GB_add_callback(gbd, GB_CB_CHANGED, AW_DB_selection_refresh_cb, (int*)this);
}

AW_DB_selection::~AW_DB_selection() {
    GB_transaction ta(gbd);
    GB_remove_callback(gbd, GB_CB_CHANGED, AW_DB_selection_refresh_cb, (int*)this);
}

GBDATA *AW_DB_selection::get_gb_main() {
    return GB_get_root(gbd); 
}

