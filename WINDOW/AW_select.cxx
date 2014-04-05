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

static bool aw_selection_list_widget_changed(GtkWidget *widget, gpointer data) {
    AW_selection_list* slist = (AW_selection_list*) data;
    slist->update_from_widget();
    return true; // correct?
}

static bool aw_selection_list_row_activated(GtkTreeView       *tree_view,
                                            GtkTreePath       *path,
                                            GtkTreeViewColumn *column,
                                            gpointer           data) {
    AW_selection_list* slist = (AW_selection_list*) data;
    slist->double_clicked();
    return true; // correct?
}

AW_selection_list::AW_selection_list(AW_awar* awar_) 
    : awar(awar_),
      model(GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING))),
      widget(NULL),
      change_cb_id(0),
      activate_cb_id(0),
      selected_index(0),
      list_table(NULL),
      last_of_list_table(NULL),
      default_select(NULL),
      select_default_on_unknown_awar_value(true)
{
    aw_assert(NULL != awar);
    awar->add_callback(makeRootCallback(aw_selection_list_awar_changed, this));
}

AW_selection_list::~AW_selection_list() {
    printf("destroying sellist\n");
    awar->remove_callback(makeRootCallback(aw_selection_list_awar_changed, this));
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
}

void AW_selection_list::select_default_on_awar_mismatch(bool value) {
    select_default_on_unknown_awar_value = value;
}

void AW_selection_list::bind_widget(GtkWidget *widget_) {
    widget = widget_;
    aw_return_if_fail(widget != NULL);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkCellLayout *cell_layout;
    if (GTK_IS_TREE_VIEW(widget)) {
        gtk_tree_view_set_model(GTK_TREE_VIEW(widget), model);
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
        gtk_combo_box_set_model(GTK_COMBO_BOX(widget), model);
        cell_layout = GTK_CELL_LAYOUT(widget);
        //connect changed signal
        change_cb_id = g_signal_connect(G_OBJECT(widget), "changed", 
                                      G_CALLBACK(aw_selection_list_widget_changed), 
                                      (gpointer) this);
        
    }
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

    GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
    selected_index = gtk_tree_path_get_indices(path)[0];
    gtk_tree_path_free(path);

    if (!get_entry_at(selected_index)) {
        if (!default_select) GBK_terminate("no default specified for selection list");
        default_select->value.write_to(awar);
    }
    else {
        get_entry_at(selected_index)->value.write_to(awar);
    }
}

void AW_selection_list::double_clicked() {
    awar->dclicked.emit();
}

void AW_selection_list::update() {
    // Warning:
    // update() will not set the connected awar to the default value
    // if it contains a value which is not associated with a list entry!

    gtk_list_store_clear(GTK_LIST_STORE(model));
    
    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        append_to_liststore(lt);
    }

    refresh();
}

void AW_selection_list::refresh() {
    aw_return_if_fail(widget != NULL);
    // select the item that matches the awars value

    AW_selection_list_entry *lt;

    int i = 0;
    for (lt = list_table; lt; lt = lt->next, i++) {
        if (lt->value == awar) break;
    }
    
    if (lt) {
        GtkTreeIter iter;
        gtk_tree_model_iter_nth_child(model, &iter, NULL, i);

        if (GTK_IS_COMBO_BOX(widget)) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
        } else {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_iter(selection, &iter);
        }
    }
    else if(NULL != default_select) {//the awar value does not match any entry, not even the default entry, we set the default entry anyway
        if(select_default_on_unknown_awar_value) {
            select_default();
        }
    }
    else {
        GBK_terminatef("Selection list '%s' has no default selection", awar->get_name());
    }
}

void AW_selection::refresh() {
    sellist->clear();
    fill();
    sellist->update();
}

void AW_selection_list::clear() {
    while (list_table) {
        AW_selection_list_entry *nextEntry = list_table->next;
        delete list_table;
        list_table = nextEntry;
    }
    list_table = NULL;
    last_of_list_table = NULL;
    default_select = NULL;

    gtk_list_store_clear(GTK_LIST_STORE(model));
}

bool AW_selection_list::default_is_selected() const {
    const char *defVal = get_default_value();
    return defVal && ARB_strNULLcmp(get_selected_value(), defVal) == 0;
}

const char *AW_selection_list::get_selected_value() const {
    if (selected_index == -1) return NULL;
    AW_selection_list_entry *entry = get_entry_at(selected_index);
    
    if(entry){
        return entry->get_displayed();
    }
    return NULL;
}

AW_selection_list_entry *AW_selection_list::get_entry_at(int index) const {
    AW_selection_list_entry *entry = list_table;
    while (index && entry) {
        entry = entry->next;
        index--;
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
        prev = get_entry_at(index-1);
        aw_return_if_fail(prev);
    }
    
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

void AW_selection_list::init_from_array(const CharPtrArray& entries, const char *defaultEntry) {
    // update selection list with contents of NULL-terminated array 'entries'
    // 'defaultEntry' is used as default selection
    // awar value will be changed to 'defaultEntry' if it does not match any other entry
    // Note: This works only with selection lists bound to GB_STRING awars.

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
    aw_return_if_fail(displayed);
    aw_return_if_fail(value);
    AW_selection_list_entry* entry = insert_generic(displayed, value, GB_STRING);
    aw_assert(NULL != entry);
}

void AW_selection_list::append_to_liststore(AW_selection_list_entry* entry)
{
    aw_return_if_fail(NULL != entry);

    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
                       0, entry->get_displayed(), 
                       -1);    
}

void AW_selection_list::delete_default() {
    if(default_select) {
        delete_value(get_default_value());
        default_select = NULL;
    }
}

void AW_selection_list::insert_default(const char *displayed, const char *value) {
    if (default_select) delete_default();
    default_select = insert_generic(displayed, value, GB_STRING);
}

void AW_selection_list::insert(const char *displayed, int32_t value) {
    AW_selection_list_entry* entry = insert_generic(displayed, value, GB_INT);
    aw_assert(NULL != entry);
}

void AW_selection_list::insert_default(const char *displayed, int32_t value) {
    if (default_select) delete_default();
    default_select = insert_generic(displayed, value, GB_INT);
}

void AW_selection_list::insert(const char *displayed, double value) {
    AW_selection_list_entry* entry = insert_generic(displayed, value, GB_FLOAT);
    aw_assert(NULL != entry);
}

void AW_selection_list::insert_default(const char *displayed, double value) {
    if (default_select) delete_default();
    default_select = insert_generic(displayed, value, GB_FLOAT);
}


void AW_selection_list::insert(const char *displayed, GBDATA *pointer) {
    AW_selection_list_entry* entry = insert_generic(displayed, pointer, GB_POINTER);
    aw_assert(NULL != entry);
}

void AW_selection_list::insert_default(const char *displayed, GBDATA *pointer) {
    if (default_select) delete_default();
    default_select = insert_generic(displayed, pointer, GB_POINTER);
}


void AW_selection_list::move_content_to(AW_selection_list *target_list) {
    // @@@ instead of COPYING, it could simply move the entries (may cause problems with iterator)

    AW_selection_list_entry *entry = list_table;
    while (entry) {
        if(entry != default_select) {//default selection should not be coied over
        
            if (!target_list->list_table) {
                target_list->last_of_list_table = target_list->list_table = new AW_selection_list_entry(entry->get_displayed(), entry->value.get_string());
            }
            else {
                target_list->last_of_list_table->next = new AW_selection_list_entry(entry->get_displayed(), entry->value.get_string());
                target_list->last_of_list_table = target_list->last_of_list_table->next;
                target_list->last_of_list_table->next = NULL;
            }
        }
        entry = entry->next;
    }

    clear();
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

size_t AW_selection_list::size() {
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

static void AW_DB_selection_refresh_cb(GBDATA *, AW_DB_selection *selection) {
    selection->refresh();
}

//AW_DB_selection
//TODO move to own file
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


template <class T>
AW_selection_list_entry* AW_selection_list::insert_generic(const char* displayed, T value, GB_TYPES expectedType) {
    if (awar->get_type() != expectedType) {
    selection_type_mismatch(typeid(T).name()); //note: gcc mangles the name, however this error will only occur during development.
    return NULL;
    }
    if (list_table) {
        last_of_list_table->next = new AW_selection_list_entry(displayed, value);
        last_of_list_table = last_of_list_table->next;
        last_of_list_table->next = NULL;
    }
    else {
        last_of_list_table = list_table = new AW_selection_list_entry(displayed, value);
    }

    return last_of_list_table;
}
