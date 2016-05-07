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
#include <gtk/gtk.h>

#include <string>
#include <list>

#include <ad_cb.h>
#include <arb_strbuf.h>
#include <arb_sort.h>
#include <arb_strarray.h>
#include <arb_str.h>

#include "aw_type_checking.hxx"

static void aw_selection_list_awar_changed(AW_root*, AW_selection_list* slist) {
    slist->refresh();
}
static bool aw_selection_list_widget_changed(GtkWidget*, gpointer data) {
    AW_selection_list* slist = (AW_selection_list*)data;
    slist->update_from_widget();
    return true; // correct?
}

static bool aw_selection_list_row_activated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer data) {
    AW_selection_list* slist = (AW_selection_list*)data;
    slist->double_clicked();
    return true; // correct?
}

GtkTreeModel *AW_selection_list::get_model() {
    if (!model) {
#if defined(UNIT_TESTS)
        aw_assert(!RUNNING_TEST());
#endif
        model = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
    }
    return model;
}

AW_selection_list::AW_selection_list(AW_awar *awar_, bool fallback2default)
    : awar(awar_),
      model(NULL),
      widget(NULL),
      change_cb_id(0),
      activate_cb_id(0),
      selected_index(0),
      select_default_on_unknown_awar_value(fallback2default),
      update_cb(NULL),
      cl_update(0),
      list_table(NULL),
      last_of_list_table(NULL),
      default_select(NULL),
      auto_append_to_default(true)
{
    /*! create a selection list
     * @param awar_ Selection list will be bound to this awar.
     * @param fallback2default if true, the awar will reset to the default value whenever its content doesn't match any other entry.
     *
     * Note: use fallback2default == false, if the awar is also bound to another
     *       widget (esp. to input fields) or might somehow change to some custom value.
     */
#if defined(UNIT_TESTS)
    if (RUNNING_TEST() && !awar) {
        return; // accept AW_selection_list w/o awar from unittest
    }
#endif
    aw_assert(NULL != awar);
    awar->add_callback(makeRootCallback(aw_selection_list_awar_changed, this));
}

AW_selection_list::~AW_selection_list() {
    printf("destroying sellist\n");
    if (awar) {
        awar->remove_callback(makeRootCallback(aw_selection_list_awar_changed, this));
    }
    if (change_cb_id) {
        GObject *obj;
        if (GTK_IS_TREE_VIEW(widget)) {
            obj = G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
        } else {
            obj = G_OBJECT(widget);
        }
        g_signal_handler_disconnect(obj, change_cb_id);
    }
    if (activate_cb_id) {
        g_signal_handler_disconnect(G_OBJECT(widget), activate_cb_id);
    }
    clear();
}

void AW_selection_list::bind_widget(GtkWidget *widget_) {
    widget = widget_;
    aw_return_if_fail(widget != NULL);

    GtkCellRenderer *renderer    = gtk_cell_renderer_text_new();
    GtkCellLayout   *cell_layout = NULL;

    if (GTK_IS_TREE_VIEW(widget)) {
        gtk_tree_view_set_model(GTK_TREE_VIEW(widget), get_model());
        cell_layout = GTK_CELL_LAYOUT(gtk_tree_view_column_new());
        gtk_tree_view_append_column(GTK_TREE_VIEW(widget), GTK_TREE_VIEW_COLUMN(cell_layout));
        g_object_set(renderer, "family", "monospace", NULL);
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
        gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
        //connect changed signal
        change_cb_id = g_signal_connect(G_OBJECT(selection), "changed", 
                                        G_CALLBACK(aw_selection_list_widget_changed), 
                                        (gpointer) this);

        // connect double-click signal (row activated)
        activate_cb_id = g_signal_connect(G_OBJECT(widget), "row-activated", 
                                          G_CALLBACK(aw_selection_list_row_activated), 
                                          (gpointer) this);
        
    } else if (GTK_IS_COMBO_BOX(widget)) {
        gtk_combo_box_set_model(GTK_COMBO_BOX(widget), get_model());
        cell_layout = GTK_CELL_LAYOUT(widget);
        //connect changed signal
        change_cb_id = g_signal_connect(G_OBJECT(widget), "changed", 
                                        G_CALLBACK(aw_selection_list_widget_changed),
                                        (gpointer) this);

    }

    aw_assert(cell_layout);
    gtk_cell_layout_pack_start(cell_layout, renderer, true);
    // map column 0 to attribute "text" of the renderer
    gtk_cell_layout_add_attribute(cell_layout, renderer, "text", 0);

}

void AW_selection_list::update_from_widget() {
    aw_return_if_fail(GTK_IS_COMBO_BOX(widget) || GTK_IS_TREE_VIEW(widget));
    
    GtkTreeIter iter;
    //set iter to the currently active item
    if (GTK_IS_COMBO_BOX(widget)) {
        if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) 
            return;
    } 
    else if (GTK_IS_TREE_VIEW(widget)) {
        GtkTreeSelection *tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
        if (!gtk_tree_selection_get_selected(tsel, NULL, &iter)) 
            return;
    }

    GtkTreePath *path = gtk_tree_model_get_path(get_model(), &iter);
    selected_index = gtk_tree_path_get_indices(path)[0];
    gtk_tree_path_free(path);

    if (!get_entry_at(selected_index, true)) {
        if (!default_select) GBK_terminate("no default specified for selection list");
        default_select->value.write_to(awar);
    }
    else {
        get_entry_at(selected_index, true)->value.write_to(awar);
    }
}

void AW_selection_list::double_clicked() {
    awar->dclicked.emit();
}

void AW_selection_list::update() {
    // Warning:
    // update() will not set the connected awar to the default value
    // if it contains a value which is not associated with a list entry!

    gtk_list_store_clear(GTK_LIST_STORE(get_model()));

    bool default_added = false;
    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        if (lt->follows(default_select)) {
            append_to_liststore(default_select);
            default_added = true;
        }
        append_to_liststore(lt);
    }

    if (default_select && !default_added) {
        append_to_liststore(default_select);
    }

    auto_append_to_default = false; // further updates will not insert behind (terminal) default_select
    refresh();

    if (update_cb) update_cb(this, cl_update);
}

void AW_selection_list::set_update_callback(sellist_update_cb ucb, AW_CL cl_user) {
    aw_assert(!update_cb || !ucb); // overwrite allowed only with NULL!

    update_cb = ucb;
    cl_update = cl_user;
}

void AW_selection_list::refresh() {
    aw_return_if_fail(widget != NULL);

    const int NO_ENTRY_MATCHED_AWAR_VALUE = -1;

    int i = 0;
    {   // detect index in gtk-widget matching the awar value
        AW_selection_list_entry *lt;
        for (lt = list_table; lt; lt = lt->next, ++i) {
            if (lt->follows(default_select)) {
                if (default_select->value == awar) {
                    lt = default_select;
                    break;
                }
                ++i;
            }
            if (lt->value == awar) break;
        }
        if (!lt) {
            if (default_select && default_select->value == awar) {
                // if awar matches default and lt == NULL
                // => default_select is bottom element
                // => i has already correct value
            }
            else {
                i = NO_ENTRY_MATCHED_AWAR_VALUE;
            }
        }
    }

    if (i == NO_ENTRY_MATCHED_AWAR_VALUE) {
        if (default_select) { // the awar value does not match any entry, not even the default entry
            if (select_default_on_unknown_awar_value) { // optional fallback to default value
                select_default();
            }
        }
        else {
            GBK_terminatef("Selection list '%s' has no default selection", awar->get_name());
        }
    }
    else {
        aw_assert(i>=0);

        GtkTreeIter iter;
        gtk_tree_model_iter_nth_child(get_model(), &iter, NULL, i);

        if (GTK_IS_COMBO_BOX(widget)) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
        }
        else {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_iter(selection, &iter);
        }
    }
}

void AW_selection::refresh() {
    get_sellist()->clear();
    fill();
    get_sellist()->update();
}

void AW_selection_list::clear() {
    /** Remove all items from the list. Default item is removed as well.*/
    while (list_table) {
        AW_selection_list_entry *nextEntry = list_table->next;
        delete list_table;
        list_table = nextEntry;
    }
    list_table         = NULL;
    last_of_list_table = NULL;

    delete_default();
    auto_append_to_default = true;
}

bool AW_selection_list::default_is_selected() const {
    return default_select->value == awar;
}

const char *AW_selection_list::get_selected_value() const {
    aw_assert(get_awar_type() == GB_STRING);

    if (selected_index == -1) return NULL;

    AW_selection_list_entry *entry = get_entry_at(selected_index, true);
    return entry ? entry->value.get_string() : NULL;
}

AW_selection_list_entry *AW_selection_list::get_entry_at(int index, bool gtk_index) const {
    /*!
     * @param index [0 .. size()-1] if gtk_index == false; otherwise [0 .. size()]
     * @param gtk_index==true -> also return default_select
     * @return NULL if there is no entry at this index
     */

    AW_selection_list_entry *entry = list_table;
    if (gtk_index && default_select) {
        int i = 0;
        for (; entry; ++i) {
            if (entry->follows(default_select)) {
                if (i == index) {
                    entry = default_select;
                    break;
                }
                ++i;
            }
            if (i == index) break;
            entry = entry->next;
        }
        if (!entry && default_select->next == NULL) {
            if (i == index) entry = default_select;
        }
    }
    else {
        while (index && entry) {
            entry = entry->next;
            index--;
        }
    }

    return entry;
}


void AW_selection_list::select_default() {
    if (default_select) {
        default_select->value.write_to(awar);
    }
}

void AW_selection_list::delete_element_at(const int index) {
    aw_return_if_fail(index >= 0);
    
    AW_selection_list_entry *prev = NULL;
    if (index>0) {
        prev = get_entry_at(index-1, false);
        aw_return_if_fail(prev);
    }

    AW_selection_list_entry *toDel = prev ? prev->next : list_table;
    if (toDel) {
        aw_assert(toDel != default_select);

        if (index == selected_index) select_default();
        if (index<selected_index) --selected_index;

        // correct successor of default_select
        if (default_select->next == toDel) {
            default_select->next = toDel->next;
        }

        (prev ? prev->next : list_table) = toDel->next;
        delete toDel;

        if (last_of_list_table == toDel) last_of_list_table = prev;
    }
}

void AW_selection_list::delete_value(const char *value) {
    aw_assert(get_awar_type() == GB_STRING); // only works for GB_STRING awars

#if defined(ASSERTION_USED)
    const char *defval = get_default_value();

    aw_assert(!defval || strcmp(defval, value) != 0); // does NOT work for default-selection
#endif

    int index = get_index_of(value);
    delete_element_at(index);
}

const char *AW_selection_list::get_awar_value() const {
    return awar->read_char_pntr();
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

void AW_selection_list::init_from_array(const CharPtrArray& entries, const char *default_displayed, const char *default_value) {
    // update selection list with contents of NULL-terminated array 'entries'
    //
    // 'default_displayed' and 'default_value' are used as default selection.
    // To position the default selection, add 'default_value' to 'entries' as well.
    //
    // awar value will be changed to 'defaultEntry' if it does not match any other entry
    //
    // Note: This works only with selection lists bound to GB_STRING awars.

    aw_assert(get_awar_type() == GB_STRING);
    aw_assert(default_displayed);
    aw_assert(default_value);

    // use copies (just in case default_* points to a value free'd by clear())
    char *defaultDispCopy  = strdup(default_displayed);
    char *defaultValueCopy = strdup(default_value);

    bool defInserted = false;

    clear();

    for (int i = 0; entries[i]; ++i) {
        if (!defInserted && strcmp(entries[i], defaultValueCopy) == 0) {
            insert_default(defaultDispCopy, defaultValueCopy);
            defInserted = true;
        }
        else {
            insert(entries[i], entries[i]);
        }
    }
    if (!defInserted) insert_default(defaultDispCopy, defaultValueCopy);
    update();

    const char *selected = get_selected_value();
    if (selected) set_awar_value(selected);

    free(defaultValueCopy);
    free(defaultDispCopy);
}

void AW_selection_list::insert(const char *displayed, const char *value) {
    aw_return_if_fail(displayed);
    aw_return_if_fail(value);
    append(make_entry(displayed, value, GB_STRING));
}

void AW_selection_list::append_to_liststore(AW_selection_list_entry* entry) {
    /*!
     * Appends the specified entry to the list store
     */
    aw_return_if_fail(NULL != entry);

    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(get_model()), &iter);
    gtk_list_store_set(GTK_LIST_STORE(get_model()), &iter,
                       0, entry->get_displayed(), 
                       -1);    
}

void AW_selection_list::delete_default() {
    /** Removes the default entry from the list*/
    if (default_select) {
        delete default_select;
        default_select = NULL;
    }
}

void AW_selection_list::replace_default(AW_selection_list_entry *new_default) {
    if (default_select) delete_default();
    aw_assert(!new_default->next);
    default_select = new_default;
    aw_assert(default_select->next == NULL);
}

void AW_selection_list::insert_default(const char *displayed, const char *value) {
    replace_default(make_entry(displayed, value, GB_STRING));
}

void AW_selection_list::insert(const char *displayed, int32_t value) {
    append(make_entry(displayed, value, GB_INT));
}

void AW_selection_list::insert_default(const char *displayed, int32_t value) {
    replace_default(make_entry(displayed, value, GB_INT));
}

void AW_selection_list::insert(const char *displayed, float value) {
    append(make_entry(displayed, value, GB_FLOAT));
}

void AW_selection_list::insert_default(const char *displayed, float value) {
    replace_default(make_entry(displayed, value, GB_FLOAT));
}


void AW_selection_list::insert(const char *displayed, GBDATA *pointer) {
    append(make_entry(displayed, pointer, GB_POINTER));
}

void AW_selection_list::insert_default(const char *displayed, GBDATA *pointer) {
    replace_default(make_entry(displayed, pointer, GB_POINTER));
}


void AW_selection_list::move_content_to(AW_selection_list *target_list) {
    //! move all entries (despite default entry) to another AW_selection_list

    if (default_select) {
        char *defDisp = strdup(default_select->get_displayed());
        char *defVal  = strdup(default_select->value.get_string());

        delete_default();
        move_content_to(target_list);
        insert_default(defDisp, defVal);

        free(defVal);
        free(defDisp);
    }
    else {
        AW_selection_list_entry *entry = list_table;
        while (entry) {
            target_list->insert(entry->get_displayed(), entry->value.get_string());
            entry = entry->next;
        }
        clear();
    }
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
    AW_selection_list_entry *entry = get_entry_at(index, false);
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
    awar->write_string(new_value);
}

void AW_selection_list::set_file_suffix(const char *suffix) {
    AW_root *aw_root = AW_root::SINGLETON;
    char     filter[200];
    sprintf(filter, "tmp/save_box_sel_%li/filter", (long)this);
    aw_root->awar_string(filter, suffix);
    sprintf(filter, "tmp/load_box_sel_%li/filter", (long)this);
    aw_root->awar_string(filter, suffix);
}

size_t AW_selection_list::size() const {
    AW_selection_list_entry *lt    = list_table;
    size_t                  count = 0;

    while (lt) {
        ++count;
        lt = lt->next;
    }
    return count;
}

static int sel_sort_backward(const char *d1, const char *d2) { return strcmp(d2, d1); }
static int sel_isort_backward(const char *d1, const char *d2) { return ARB_stricmp(d2, d1); }
static int gb_compare_function__2__sellist_cmp_fun(const void *t1, const void *t2, void *v_selcmp) {
    sellist_cmp_fun selcmp = (sellist_cmp_fun)v_selcmp;
    return selcmp(static_cast<const AW_selection_list_entry*>(t1)->get_displayed(),
                  static_cast<const AW_selection_list_entry*>(t2)->get_displayed());
}

void AW_selection_list::sortCustom(sellist_cmp_fun cmp) {
    size_t count = size();
    if (count) {
        AW_selection_list_entry **tables = new AW_selection_list_entry *[count];
        count = 0;
        for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
            tables[count++] = lt;
        }

        GB_sort((void**)tables, 0, count, gb_compare_function__2__sellist_cmp_fun, (void*)cmp);

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

void AW_selection_list::sort(bool backward, bool case_sensitive) {
    sellist_cmp_fun cmp;
    if (backward) {
        if (case_sensitive) cmp = sel_sort_backward;
        else cmp                = sel_isort_backward;
    }
    else {
        if (case_sensitive) cmp = strcmp;
        else cmp                = ARB_stricmp;
    }
    sortCustom(cmp);
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

char *AW_selection_list_entry::copy_string_for_display(const char *str) {
    size_t  len     = strlen(str);
    bool    tooLong = len>MAX_DISPLAY_LENGTH;
    char   *out;
    if (tooLong) {
        out = GB_strndup(str, MAX_DISPLAY_LENGTH);
        { // add message about truncation
            char   *truncated = GBS_global_string_copy(" <truncated - original contains %zu byte>", len);
            size_t  tlen      = strlen(truncated);
            aw_assert(MAX_DISPLAY_LENGTH>tlen);
            memcpy(out+MAX_DISPLAY_LENGTH-tlen, truncated, tlen);
        }
        len = MAX_DISPLAY_LENGTH;
    }
    else {
        out = GB_strduplen(str, len);
    }

    for (size_t i = 0; i<len; ++i) {
        switch (out[i]) {
            case ',':   out[i] = ';'; break;
            case '\n':  out[i] = '#'; break;
        }
    }
    return out;
}

AW_selection_list_entry *AW_selection_list::append(AW_selection_list_entry *new_entry) {
    /*!
     * append an entry to list (create it with make_entry)
     */
    if (list_table) {
        last_of_list_table->next = new_entry;
        last_of_list_table = last_of_list_table->next;
        last_of_list_table->next = NULL;
    }
    else {
        last_of_list_table = list_table = new_entry;
    }

    if (auto_append_to_default && default_select && !default_select->next) {
        // remember that 'new_entry' is inserted as successor of default entry
        default_select->next = new_entry;
    }

    return last_of_list_table;
}

template <class T>
AW_selection_list_entry* AW_selection_list::make_entry(const char* displayed, T value, GB_TYPES expectedType) {
    /*!
     * Create a selection list entry from a value
     * @param displayed The value that should be displayed
     * @param value The actual value
     * @param expectedType The type of this variable (only used for type checking)
     * @return A new list entry. NULL in case of error.
     */
    if (awar && awar->get_type() != expectedType) {
        selection_type_mismatch(typeid(T).name()); //note: gcc mangles the name, however this error will only occur during development.
        return NULL; // never reached
    }
    return new AW_selection_list_entry(displayed, value);
}

// -------------------------
//      AW_DB_selection

static void AW_DB_selection_refresh_cb(GBDATA *, AW_DB_selection *selection) {
    selection->refresh();
}

AW_DB_selection::AW_DB_selection(AW_selection_list *sellist_, GBDATA *gbd_)
    : AW_selection(sellist_)
    , gbd(gbd_)
{
    GB_transaction ta(gbd);
    GB_add_callback(gbd, GB_CB_CHANGED, makeDatabaseCallback(AW_DB_selection_refresh_cb, this));
}

AW_DB_selection::~AW_DB_selection() {
    GB_transaction ta(gbd);
    GB_remove_callback(gbd, GB_CB_CHANGED, makeDatabaseCallback(AW_DB_selection_refresh_cb, this));
}

GBDATA *AW_DB_selection::get_gb_main() {
    return GB_get_root(gbd);
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#define TEST_LIST_CONTENT(list,values,expected) do {    \
        StrArray a;                                     \
        (list).to_array(a, values);                     \
        char *str = GBT_join_strings(a, ';');           \
        TEST_EXPECT_EQUAL(str, expected);               \
        free(str);                                      \
    } while(0)

#define TEST_GET_LIST_CONTENT(list,expected) do {       \
        char *str = (list).get_content_as_string(10);   \
        TEST_EXPECT_EQUAL(str, expected);               \
        free(str);                                      \
    } while(0)

void TEST_selection_list_access() {
#if defined(ARB_GTK)
    AW_selection_list list0(NULL, false);
    AW_selection_list list1(NULL, false);
    AW_selection_list list2(NULL, false);
#else // ARB_MOTIF
    AW_selection_list list0("bla", GB_STRING, NULL);
    AW_selection_list list1("bla", GB_STRING, NULL);
    AW_selection_list list2("alb", GB_STRING, NULL);
#endif

    list0.insert_default("First", "1st");
    list0.insert("Second", "2nd");
    list0.insert("Third", "3rd");

    list1.insert("First", "1st");
    list1.insert_default("Second", "2nd");
    list1.insert("Third", "3rd");

    list2.insert("First", "1st");
    list2.insert("Second", "2nd");
    list2.insert("Third", "3rd");
    list2.insert_default("Default", "");

    TEST_EXPECT_EQUAL(list0.size(), 2);
    TEST_EXPECT_EQUAL(list1.size(), 2);
    TEST_EXPECT_EQUAL(list2.size(), 3);

    TEST_EXPECT_EQUAL(list1.get_default_value(), "2nd");
    TEST_EXPECT_EQUAL(list1.get_default_display(), "Second");

    TEST_EXPECT_EQUAL(list0.get_index_of("1st"), -1); // default value is not indexed
    TEST_EXPECT_EQUAL(list0.get_index_of("2nd"), 0);
    TEST_EXPECT_EQUAL(list0.get_index_of("3rd"), 1);  // = second non-default entry

    TEST_EXPECT_EQUAL(list1.get_index_of("1st"), 0);
    TEST_EXPECT_EQUAL(list1.get_index_of("2nd"), -1); // default value is not indexed
    TEST_EXPECT_EQUAL(list1.get_index_of("3rd"), 1);  // = second non-default entry

    TEST_EXPECT_EQUAL(list2.get_index_of("1st"), 0);
    TEST_EXPECT_EQUAL(list2.get_index_of("2nd"), 1);
    TEST_EXPECT_EQUAL(list2.get_index_of("3rd"), 2);

    TEST_EXPECT_EQUAL(list0.peek_displayed(0), "First"); // default
    TEST_EXPECT_EQUAL(list0.peek_displayed(1), "Second");
    TEST_EXPECT_EQUAL(list0.peek_displayed(2), "Third");

    TEST_EXPECT_EQUAL(list1.peek_displayed(0), "First");
    TEST_EXPECT_EQUAL(list1.peek_displayed(1), "Second"); // default
    TEST_EXPECT_EQUAL(list1.peek_displayed(2), "Third");

    TEST_EXPECT_EQUAL(list2.peek_displayed(0), "First");
    TEST_EXPECT_EQUAL(list2.peek_displayed(1), "Second");
    TEST_EXPECT_EQUAL(list2.peek_displayed(2), "Third");
    TEST_EXPECT_EQUAL(list2.peek_displayed(3), "Default"); // default


    TEST_EXPECT_EQUAL(list0.get_value_at(0), "2nd");
    TEST_EXPECT_EQUAL(list0.get_value_at(1), "3rd");
    TEST_EXPECT_NULL(list0.get_value_at(2));

    TEST_EXPECT_EQUAL(list1.get_value_at(0), "1st");
    TEST_EXPECT_EQUAL(list1.get_value_at(1), "3rd");
    TEST_EXPECT_NULL(list1.get_value_at(2));

    TEST_EXPECT_EQUAL(list2.get_value_at(0), "1st");
    TEST_EXPECT_EQUAL(list2.get_value_at(1), "2nd");
    TEST_EXPECT_EQUAL(list2.get_value_at(2), "3rd");
    TEST_EXPECT_NULL(list2.get_value_at(3));

    TEST_LIST_CONTENT(list1, true,  "1st;3rd");
    TEST_LIST_CONTENT(list1, false, "First;Third");
    TEST_GET_LIST_CONTENT(list1,    "First\nThird\n");

    TEST_LIST_CONTENT(list2, true,  "1st;2nd;3rd");
    TEST_LIST_CONTENT(list2, false, "First;Second;Third");
    TEST_GET_LIST_CONTENT(list2,    "First\nSecond\nThird\n");

    {
        AW_selection_list_iterator iter1(&list1);
        AW_selection_list_iterator iter2(&list2);

        TEST_EXPECT(bool(iter1));
        TEST_EXPECT(bool(iter2));

        TEST_EXPECT_EQUAL(iter1.get_displayed(), "First");
        TEST_EXPECT_EQUAL(iter2.get_displayed(), "First");

        TEST_EXPECT_EQUAL(iter1.get_value(), "1st");
        TEST_EXPECT_EQUAL(iter2.get_value(), "1st");

        ++iter1;
        ++iter2;

        TEST_EXPECT(bool(iter1));
        TEST_EXPECT(bool(iter2));

        TEST_EXPECT_EQUAL(iter1.get_displayed(), "Third");
        TEST_EXPECT_EQUAL(iter2.get_displayed(), "Second");

        TEST_EXPECT_EQUAL(iter1.get_value(), "3rd");
        TEST_EXPECT_EQUAL(iter2.get_value(), "2nd");

        ++iter1;
        ++iter2;

        TEST_REJECT(bool(iter1));
        TEST_EXPECT(bool(iter2));
    }

    {
#if defined(ARB_GTK)
        AW_selection_list copy1(NULL, false);
        AW_selection_list copy2(NULL, false);
#else // ARB_MOTIF
        AW_selection_list copy1("c1", GB_STRING, NULL);
        AW_selection_list copy2("c2", GB_STRING, NULL);
#endif

        list1.move_content_to(&copy1);
        list2.move_content_to(&copy2);

        TEST_EXPECT_EQUAL(list1.size(), 0);
        TEST_EXPECT_EQUAL(list2.size(), 0);

        TEST_LIST_CONTENT(copy1, true, "1st;3rd");
        TEST_LIST_CONTENT(copy2, true, "1st;2nd;3rd");
    }
}
TEST_PUBLISH(TEST_selection_list_access);

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

