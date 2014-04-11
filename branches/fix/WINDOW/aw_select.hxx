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

#ifndef AW_SELECT_HXX
#define AW_SELECT_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef AW_SCALAR_HXX
#include "aw_scalar.hxx"
#endif

// cppcheck-suppress noConstructor
class AW_selection_list_entry : virtual Noncopyable {
    static const size_t MAX_DISPLAY_LENGTH = 8192;
    // 8192 -> no wrap-around in motif (gtk may handle different value)
    // 100000 -> works in motif (no crash, but ugly because line wraps around - overwriting itself; also happens in gtk)
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
};

typedef int (*sellist_cmp_fun)(const char *disp1, const char *disp2);

class AW_selection_list {
    AW_selection_list_entry *get_entry_at(int index);
    
public:
    AW_selection_list(const char *variable_namei, int variable_typei, Widget select_list_widgeti);

    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            select_list_widget;

    AW_selection_list_entry *list_table;
    AW_selection_list_entry *last_of_list_table;
    AW_selection_list_entry *default_select;
    AW_selection_list      *next;

    // ******************** real public ***************
    
    void selectAll();
    void deselectAll();

    size_t size();

    void insert(const char *displayed, const char *value);
    void insert_default(const char *displayed, const char *value);
    void insert(const char *displayed, int32_t value);
    void insert_default(const char *displayed, int32_t value);
    void insert(const char *displayed, GBDATA *pointer);
    void insert_default(const char *displayed, GBDATA *pointer);

    void init_from_array(const CharPtrArray& entries, const char *defaultEntry);
    
    void update();
    void refresh();

    void sort(bool backward, bool case_sensitive); // uses displayed value!
    void sortCustom(sellist_cmp_fun cmp);          // uses displayed value!

    // ---------------------------------------------------
    // the following functions work for string awars only:

    const char *get_awar_value() const;
    void set_awar_value(const char *new_value);

    const char *get_default_value() const;
    const char *get_default_display() const;

    void select_default() { set_awar_value(get_default_value()); }

    const char *get_selected_value() const; // may differ from get_awar_value() if default is selected (returns value passed to insert_default_selection)

    int get_index_of(const char *searched_value);
    int get_index_of_displayed(const char *displayed);
    int get_index_of_selected();

    const char *get_value_at(int index);

    void select_element_at(int wanted_index);
    void move_selection(int offset);

    bool default_is_selected() const;

    void delete_element_at(int index);
    void delete_value(const char *value);
    void clear(bool clear_default = true); 

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
    virtual ~AW_DB_selection();
    
    GBDATA *get_gbd() { return gbd; }
    GBDATA *get_gb_main() { return GB_get_root(gbd); }
};


#else
#error aw_select.hxx included twice
#endif // AW_SELECT_HXX
