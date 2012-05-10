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
    char      *displayed;

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

    static char *copy_string_for_display(const char *str);

    template<typename T>
    void set_value(T val) { value = AW_scalar(val); }

    const char *get_displayed() const { return displayed; }
    void set_displayed(const char *displayed_) { freeset(displayed, copy_string_for_display(displayed_)); }
};

class AW_selection_list {
    AW_selection_list_entry *loop_pntr; // @@@ better use some kind of selection-list iterator

public:
    AW_selection_list(const char *variable_namei, int variable_typei, Widget select_list_widgeti);

    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    Widget            select_list_widget;
    bool              value_equal_display; // set true to fix load/save of some selection lists

    AW_selection_list_entry *list_table;
    AW_selection_list_entry *last_of_list_table;
    AW_selection_list_entry *default_select;
    AW_selection_list      *next;

    // ******************** real public ***************
    
    void selectAll();
    void deselectAll();

    size_t size();

    // ---------------------------------------------------
    // the following functions work for string awars only:
    
    const char *get_awar_value(AW_root *aw_root) const;
    void set_awar_value(AW_root *aw_root, const char *new_value);

    const char *get_default_value() const;
    const char *get_default_display() const;

    const char *get_selected_value() const; // may differ from get_awar_value() if default is selected (returns value passed to insert_default_selection) 

    // the following iterator does NOT iterate over default-element:
    const char *first_element();
    const char *next_element();

    bool default_is_selected() const;
};

class AW_selection_list_iterator {
    AW_selection_list_entry *entry;
public:
    AW_selection_list_iterator(AW_selection_list *sellist)
        : entry(sellist->list_table)
    {}

    const char *get_displayed() { return entry ? entry->get_displayed() : NULL; }
    const char *get_value() { return entry ? entry->value.get_string() : NULL; }

    void set_displayed(const char *disp) { aw_assert(entry); entry->set_displayed(disp); }
    void set_value(const char *val) { aw_assert(entry); entry->set_value(val); }

    AW_selection_list_iterator& operator++() {
        entry = entry->next;
        return *this;
    }
};

class AW_selection : virtual Noncopyable {
    AW_window         *win;                         // window containing the selection
    AW_selection_list *sellist;

    virtual void fill() = 0;
    
public:
    AW_selection(AW_window *win_, AW_selection_list *sellist_) : win(win_) , sellist(sellist_) {}
    virtual ~AW_selection() {}

    void refresh();

    AW_window *get_win() { return win; }
    AW_root *get_root() { return win->get_root(); }
    AW_selection_list *get_sellist() { return sellist; }

    void insert_selection(const char *displayed, const char *value) { win->insert_selection(sellist, displayed, value); }
    void insert_default_selection(const char *displayed, const char *value) { win->insert_default_selection(sellist, displayed, value); }
    
    void insert_selection(const char *displayed, int32_t value) { win->insert_selection(sellist, displayed, value); }
    void insert_default_selection(const char *displayed, int32_t value) { win->insert_default_selection(sellist, displayed, value); }

    void get_values(StrArray& intoArray) { get_win()->selection_list_to_array(intoArray, get_sellist(), true); }
    void get_displayed(StrArray& intoArray) { get_win()->selection_list_to_array(intoArray, get_sellist(), false); }
};


class AW_DB_selection : public AW_selection { // derived from a Noncopyable
    GBDATA *gbd;                                    // root container of data displayed in selection list
public:
    AW_DB_selection(AW_window *win_, AW_selection_list *sellist_, GBDATA *gbd_);
    virtual ~AW_DB_selection();
    
    GBDATA *get_gbd() { return gbd; }
    GBDATA *get_gb_main() { return GB_get_root(gbd); }
};
void AW_DB_selection_refresh_cb(GBDATA *, AW_DB_selection *);


#else
#error aw_select.hxx included twice
#endif // AW_SELECT_HXX
