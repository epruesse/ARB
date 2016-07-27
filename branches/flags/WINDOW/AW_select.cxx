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
#include "aw_root.hxx"
#include "aw_awar.hxx"

#include <arb_strarray.h>
#include <arb_strbuf.h>
#include <arb_str.h>
#include <arb_sort.h>
#include <ad_cb.h>

#include <Xm/List.h>

__ATTR__NORETURN inline void selection_type_mismatch(const char *triedType) { type_mismatch(triedType, "selection-list"); }

// --------------------------
//      AW_selection_list


AW_selection_list::AW_selection_list(const char *variable_name_, int variable_type_, Widget select_list_widget_)
    : variable_name(nulldup(variable_name_)),
      variable_type(AW_VARIABLE_TYPE(variable_type_)),
      update_cb(NULL),
      cl_update(0),
      select_list_widget(select_list_widget_),
      list_table(NULL),
      last_of_list_table(NULL),
      default_select(NULL),
      next(NULL)
{}

AW_selection_list::~AW_selection_list() {
    clear();
    free(variable_name);
}

inline XmString XmStringCreateSimple_wrapper(const char *text) {
    return XmStringCreateSimple((char*)text);
}

void AW_selection_list::update() {
    // Warning:
    // update() will not set the connected awar to the default value
    // if it contains a value which is not associated with a list entry!

    size_t count = size();
    if (default_select) count++;

    XmString *strtab = new XmString[count];

    count = 0;
    for (AW_selection_list_entry *lt = list_table; lt; lt = lt->next) {
        const char *s2 = lt->get_displayed();
        if (!s2[0]) s2 = "  ";
        strtab[count]  = XmStringCreateSimple_wrapper(s2);
        count++;
    }

    if (default_select) {
        const char *s2 = default_select->get_displayed();
        if (!strlen(s2)) s2 = "  ";
        strtab[count] = XmStringCreateSimple_wrapper(s2);
        count++;
    }
    if (!count) {
        strtab[count] = XmStringCreateSimple_wrapper("   ");
        count ++;
    }

    XtVaSetValues(select_list_widget, XmNitemCount, count, XmNitems, strtab, NULL);

    refresh();

    for (size_t i=0; i<count; i++) XmStringFree(strtab[i]);
    delete [] strtab;

    if (update_cb) update_cb(this, cl_update);
}

void AW_selection_list::set_update_callback(sellist_update_cb ucb, AW_CL cl_user) {
    aw_assert(!update_cb || !ucb); // overwrite allowed only with NULL!

    update_cb = ucb;
    cl_update = cl_user;
}

void AW_selection_list::refresh() {
    if (!variable_name) return;     // not connected to awar

    AW_root *root  = AW_root::SINGLETON;
    bool     found = false;
    int      pos   = 0;
    AW_awar *awar  = root->awar(variable_name);

    AW_selection_list_entry *lt;

    switch (variable_type) {
        case AW_STRING: {
            char *var_value = awar->read_string();
            for (lt = list_table; lt; lt = lt->next) {
                if (strcmp(var_value, lt->value.get_string()) == 0) {
                    found = true;
                    break;
                }
                pos++;
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
                pos++;
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
                pos++;
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
                pos++;
            }
            break;
        }
        default:
            aw_assert(0);
            GB_warning("Unknown AWAR type");
            break;
    }

    if (found || default_select) {
        pos++;
        int top;
        int vis;
        XtVaGetValues(select_list_widget,
                      XmNvisibleItemCount, &vis,
                      XmNtopItemPosition, &top,
                      NULL);
        XmListSelectPos(select_list_widget, pos, False);

        if (pos < top) {
            if (pos > 1) pos --;
            XmListSetPos(select_list_widget, pos);
        }
        if (pos >= top + vis) {
            XmListSetBottomPos(select_list_widget, pos + 1);
        }
    }
    else {
        GBK_terminatef("Selection list '%s' has no default selection", variable_name);
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
}

bool AW_selection_list::default_is_selected() const {
    const char *sel = get_selected_value();
    if (!sel) {
        // (case should be impossible in gtk port)
        return true; // handle "nothing" like default
    }

    const char *def = get_default_value();
    return def && (strcmp(sel, def) == 0);
}

const char *AW_selection_list::get_selected_value() const {
    int                      i;
    AW_selection_list_entry *lt;
    AW_selection_list_entry *found = 0;

    aw_assert(select_list_widget);
    
    for (i=1, lt = list_table; lt; i++, lt = lt->next) {
        lt->is_selected = XmListPosSelected(select_list_widget, i);
        if (lt->is_selected && !found) found = lt;
    }

    if (default_select) {
        default_select->is_selected = XmListPosSelected(select_list_widget, i);
        if (default_select->is_selected && !found) found = default_select;
    }
    return found ? found->value.get_string() : NULL;
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
    AW_awar *awar = AW_root::SINGLETON->awar(variable_name);
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
    // Note: This works only with selection lists bound to AW_STRING awars.

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
    if (variable_type != AW_STRING) {
        selection_type_mismatch("string");
        return;
    }

    if (list_table) {
        last_of_list_table->next = new AW_selection_list_entry(displayed, value);
        last_of_list_table = last_of_list_table->next;
        last_of_list_table->next = NULL;
    }
    else {
        last_of_list_table = list_table = new AW_selection_list_entry(displayed, value);
    }
}

void AW_selection_list::delete_default() {
    /** Removes the default entry from the list*/
    if (default_select) {
        delete default_select;
        default_select = NULL;
    }
}

void AW_selection_list::insert_default(const char *displayed, const char *value) {
    if (variable_type != AW_STRING) {
        selection_type_mismatch("string");
        return;
    }
    if (default_select) delete_default();
    default_select = new AW_selection_list_entry(displayed, value);
}

void AW_selection_list::insert(const char *displayed, int32_t value) {
    if (variable_type != AW_INT) {
        selection_type_mismatch("int");
        return;
    }
    if (list_table) {
        last_of_list_table->next = new AW_selection_list_entry(displayed, value);
        last_of_list_table = last_of_list_table->next;
        last_of_list_table->next = NULL;
    }
    else {
        last_of_list_table = list_table = new AW_selection_list_entry(displayed, value);
    }
}

void AW_selection_list::insert_default(const char *displayed, int32_t value) {
    if (variable_type != AW_INT) {
        selection_type_mismatch("int");
        return;
    }
    if (default_select) delete_default();
    default_select = new AW_selection_list_entry(displayed, value);
}


void AW_selection_list::insert(const char *displayed, GBDATA *pointer) {
    if (variable_type != AW_POINTER) {
        selection_type_mismatch("pointer");
        return;
    }
    if (list_table) {
        last_of_list_table->next = new AW_selection_list_entry(displayed, pointer);
        last_of_list_table = last_of_list_table->next;
        last_of_list_table->next = NULL;
    }
    else {
        last_of_list_table = list_table = new AW_selection_list_entry(displayed, pointer);
    }
}

void AW_selection_list::insert_default(const char *displayed, GBDATA *pointer) {
    if (variable_type != AW_POINTER) {
        selection_type_mismatch("pointer");
        return;
    }
    if (default_select) delete_default();
    default_select = new AW_selection_list_entry(displayed, pointer);
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
    AW_awar *awar = AW_root::SINGLETON->awar(variable_name);
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
    // WARNING: function behaves different in motif and gtk:
    // gtk also sorts default-element, motif always places default-element @ bottom
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
    // WARNING: function behaves different in motif and gtk:
    // gtk also sorts default-element, motif always places default-element @ bottom
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

