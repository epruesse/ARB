// ================================================================ //
//                                                                  //
//   File      : aw_select.hxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#pragma once

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef AW_SCALAR_HXX
#include "aw_scalar.hxx"
#endif
#ifndef AW_AWAR_HXX
#include "aw_awar.hxx"
#endif

#include "aw_gtk_forward_declarations.hxx"
#include <typeinfo>
class CharPtrArray;
class StrArray;


// cppcheck-suppress noConstructor
class AW_selection_list_entry : virtual Noncopyable {
#if defined(ARB_GTK)
    static const size_t MAX_DISPLAY_LENGTH = 599000; // 599000 -> no wrap-around happens in gtk
#else // ARB_MOTIF
    static const size_t MAX_DISPLAY_LENGTH = 8192; // 8192 -> no wrap-around happens in motif
#endif
    // 100000 -> works in motif (no crash, but ugly because line wraps around - overwriting itself; also happens in gtk, e.g. for len=600000)
    // setting it to 750000 crashes with "X Error BadLength" in motif (when a string with that length is displayed)

    char *displayed;

    static char *copy_string_for_display(const char *str);

public:
    // @@@ make members private
    AW_scalar  value;
    bool is_selected;                                // internal use only
    AW_selection_list_entry *next;

    template<typename T>
    AW_selection_list_entry(const char *display, T val)
        : displayed(copy_string_for_display(display)),
          value(val),
          is_selected(false),
          next(NULL)
    {}
    ~AW_selection_list_entry() { free(displayed); }

    template<typename T>
    void set_value(T val) { value = AW_scalar(val); }

    const char *get_displayed() const { return displayed; } // may differ from string passed to set_displayed() if the string was longer than MAX_DISPLAY_LENGTH
    void set_displayed(const char *displayed_) { freeset(displayed, copy_string_for_display(displayed_)); }

    bool follows(const AW_selection_list_entry *other) const {
        return other ? other->next == this : false;
    }
};

typedef int (*sellist_cmp_fun)(const char *disp1, const char *disp2);
typedef void (*sellist_update_cb)(AW_selection_list *, AW_CL);

class AW_selection_list : virtual Noncopyable {
    AW_awar           *awar;
    GtkTreeModel      *model;
    GtkWidget         *widget;
    unsigned long      change_cb_id;
    unsigned long      activate_cb_id;
    int                selected_index;
    bool               select_default_on_unknown_awar_value; /**< If true the default value is selected if no entry that matches the awars value can be found*/
    sellist_update_cb  update_cb;
    AW_CL              cl_update;


    GtkTreeModel *get_model(); // lazy eval

    AW_selection_list_entry *get_entry_at(int index, bool gtk_index) const;
    
    void append_to_liststore(AW_selection_list_entry* entry);

    template <class T>
    AW_selection_list_entry* make_entry(const char* displayed, T value, GB_TYPES expectedType);

    AW_selection_list_entry *append(AW_selection_list_entry *new_entry);
    void replace_default(AW_selection_list_entry *new_default);

    // list of non-default entries
    AW_selection_list_entry *list_table;
    AW_selection_list_entry *last_of_list_table;

    // default entry
    // Note: default_select->next may point to an entry stored in 'list_table',
    // in which case the default-entry will be displayed before that entry.
    // When default_select->next is NULL, the default entry will be the last entry.
    AW_selection_list_entry *default_select;
    bool auto_append_to_default; // true => inserting normal elements after default respects order
                                 // (is true till next update: initially and after clear())

    friend class AW_selection_list_iterator;

public:
    void update_from_widget();  // called from internal callback
    void double_clicked();

    AW_selection_list(AW_awar *awar_, bool fallback2default);
    ~AW_selection_list();

    void bind_widget(GtkWidget*);


    // ******************** real public ***************

    const char *get_awar_name() const { return awar->get_name(); }
#if defined(ASSERTION_USED)
    GB_TYPES get_awar_type() const { return awar->get_type(); }
#endif
#if defined(UNIT_TESTS)
    const char *peek_displayed(int index) const {
        return get_entry_at(index, true)->get_displayed();
    }
#endif

    size_t size() const;

    void insert(const char *displayed, const char *value);
    void insert_default(const char *displayed, const char *value);
    void insert(const char *displayed, int32_t value);
    void insert_default(const char *displayed, int32_t value);
    void insert(const char *displayed, float value);
    void insert_default(const char *displayed, float value);
    void insert(const char *displayed, GBDATA *pointer);
    void insert_default(const char *displayed, GBDATA *pointer);

    void init_from_array(const CharPtrArray& entries, const char *default_displayed, const char *default_value);

    void update();
    void refresh();

    void set_update_callback(sellist_update_cb ucb, AW_CL cl_user);

    void sort(bool backward, bool case_sensitive); // uses displayed value!
    void sortCustom(sellist_cmp_fun cmp);          // uses displayed value!

    // ---------------------------------------------------
    // the following functions work for string awars only:

    const char *get_awar_value() const;
    void set_awar_value(const char *new_value);

    const char *get_default_value() const;
    const char *get_default_display() const;

    void select_default();

    const char *get_selected_value() const; // may differ from get_awar_value() if default is selected (returns value passed to insert_default)

    int get_index_of(const char *searched_value);
    int get_index_of_selected();

    const char *get_value_at(int index);

    void select_element_at(int wanted_index);
    void move_selection(int offset);

    bool default_is_selected() const;

    void delete_element_at(int index);
    void delete_value(const char *value);
    void delete_default();
    void clear();

    void move_content_to(AW_selection_list *target_list);

    void to_array(StrArray& array, bool values);
    GB_HASH *to_hash(bool case_sens);
    char *get_content_as_string(long number_of_lines); // displayed content (e.g. for printing)

    // save/load:
    void set_file_suffix(const char *suffix);
    GB_ERROR load(const char *filename);
    GB_ERROR save(const char *filename, long number_of_lines);
};

class AW_selection_list_iterator {
    AW_selection_list_entry *entry;
public:
    AW_selection_list_iterator(AW_selection_list *sellist)
        : entry(sellist->list_table)
    {}
    AW_selection_list_iterator(AW_selection_list *sellist, int index)
        : entry(sellist->list_table)
    {
        aw_assert(index>=0);
        forward(index);
    }

    operator bool() const { return entry; }

    const char *get_displayed() { return entry ? entry->get_displayed() : NULL; }
    const char *get_value() { return entry ? entry->value.get_string() : NULL; }

    void set_displayed(const char *disp) { aw_assert(entry); entry->set_displayed(disp); }
    void set_value(const char *val) { aw_assert(entry); entry->set_value(val); }

    void forward(size_t offset) {
        while (offset--) ++(*this);
    }
    AW_selection_list_iterator& operator++() {
        if (entry) entry = entry->next;
        return *this;
    }
};

class AW_selection : virtual Noncopyable {
    //! a AW_selection_list which knows how to fill itself
    // (clients can't modify)

    AW_selection_list *sellist;

    virtual void fill() = 0;

protected: 
    void insert(const char *displayed, const char *value) { sellist->insert(displayed, value); }
    void insert_default(const char *displayed, const char *value) { sellist->insert_default(displayed, value); }

    void insert(const char *displayed, int32_t value) { sellist->insert(displayed, value); }
    void insert_default(const char *displayed, int32_t value) { sellist->insert_default(displayed, value); }

public:
    AW_selection(AW_selection_list *sellist_) : sellist(sellist_) {}
    virtual ~AW_selection() {}

    void refresh();
    AW_selection_list *get_sellist() { return sellist; }
    
    void get_values(StrArray& intoArray) { get_sellist()->to_array(intoArray, true); }
    void get_displayed(StrArray& intoArray) { get_sellist()->to_array(intoArray, false); }
};


class AW_DB_selection : public AW_selection { // derived from a Noncopyable
    GBDATA *gbd;                                    // root container of data displayed in selection list
public:
    AW_DB_selection(AW_selection_list *sellist_, GBDATA *gbd_);
    virtual ~AW_DB_selection() OVERRIDE;
    
    GBDATA *get_gbd() { return gbd; }
    GBDATA *get_gb_main();
};


__ATTR__NORETURN inline void selection_type_mismatch(const char *triedType);
