//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.h                                       //
//    Purpose   : General input masks                                    //
//    Time-stamp: <Fri Aug/17/2001 21:44 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_INPUT_MASK_H
#define AWT_INPUT_MASK_H

#ifndef __STRING__
#include <string>
#endif
#ifndef __LIST__
#include <list>
#endif
#ifndef __VECTOR__
#include <vector>
#endif

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

typedef enum {
    AWT_IT_UNKNOWN,
    AWT_IT_SPECIES,
    AWT_IT_ORGANISM,
    AWT_IT_GENE,
    AWT_IT_EXPERIMENT,

    AWT_IT_TYPES
} awt_item_type;

extern const char *awt_itemtype_names[]; // names of itemtypes

//  -------------------------------------
//      class awt_item_type_selector
//  -------------------------------------
// awt_item_type_selector is a interface for specific item-types
//
// derive from awt_item_type_selector to get the functionality for
// other item types.
//
// (implemented for Species in nt_item_type_species_selector (see NTREE/NT_extern.cxx) )
//

class awt_item_type_selector {
private:
public:
    awt_item_type_selector() {}
    virtual ~awt_item_type_selector() {}

    // add callbacks to awars (i.e. to AWAR_SPECIES_NAME)
    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const = 0;

    // give the current item
    virtual GBDATA *current(AW_root *root) const = 0;

    // give the keypath for items
    virtual const char *getKeyPath() const = 0;

    // give the name of an awar containing the name of the current item
    virtual const char *get_self_awar() const =  0;
};

//  ------------------------------------
//      class awt_input_mask_global
//  ------------------------------------
// data global to one input mask
class awt_input_mask_global {
private:
    AW_root       *awr;
    GBDATA        *gb_main;
    string         awar_root;
    awt_item_type  itemtype;    // what kind of item do we handle ?

    const awt_item_type_selector *sel;

public:
    awt_input_mask_global(AW_root *awr_, GBDATA *gb_main_, const string& awar_root_, awt_item_type itemtype_, const awt_item_type_selector *sel_)
        : awr(awr_)
        , gb_main(gb_main_)
        , awar_root(awar_root_)
        , itemtype(itemtype_)
        , sel(sel_)
    {}
    virtual ~awt_input_mask_global() {}

    AW_root *get_root() { return awr; }
    GBDATA *get_gb_main() { return gb_main; }
    const string& get_awar_root() const { return awar_root; }
    awt_item_type get_itemtype() const { return itemtype; }
    const awt_item_type_selector *get_selector() const { aw_assert(sel); return sel; }
};


//  --------------------------------
//      class awt_input_handler
//  --------------------------------
// a awt_input_handler is the base class for all types of input-handlers
class awt_input_handler {
private:
    static int awar_counter; // used to create unique awars for each handler

    awt_input_mask_global *global;
    GBDATA                *gb_item; // item this handler is linked to
    // if gb_item == 0 then no callbacks are installed

    GBDATA   *gbd;              // link to database
    string    label;            // label of this input handler
    string    child_path;       // path in database from item to handled child
    int       id;               // my personal id
    GB_TYPES  db_type;          // type of database field
    bool      in_destructor;

    GB_ERROR add_db_callbacks();
    GB_ERROR remove_db_callbacks();

    GB_ERROR add_awar_callbacks();
    GB_ERROR remove_awar_callbacks();

public:
    awt_input_mask_global *mask_global() { return global; }

    GBDATA *item() { return gb_item; }
    GB_ERROR link_to(GBDATA *gb_new_item); // link to a new item
    GB_ERROR unlink() { return link_to(0); }
    GB_ERROR relink() { return link_to(mask_global()->get_selector()->current(mask_global()->get_root())); }

    awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_);
    virtual ~awt_input_handler() { in_destructor = true; unlink(); }

    GBDATA *data() { return gbd; }
    const string& get_label() const { return label; }

    GB_TYPES type() const { return db_type; }
    void set_type(GB_TYPES typ) { db_type = typ; }

    const string& get_child_path() const { return child_path; }

    // callbacks are handled via the following virtual functions :
    virtual void awar_changed() = 0;
    virtual void db_changed()   = 0;

    virtual void build_widget(AW_window *aws) = 0; // builds the widget at the current position

    string awar_name() {
        return GBS_global_string("%s/%s_%i", global->get_awar_root().c_str(), child_path.c_str(), id);
    }
    AW_awar *awar() { return global->get_root()->awar(awar_name().c_str()); }
};

typedef SmartPtr<awt_input_handler> awt_input_handler_ptr;
typedef list<awt_input_handler_ptr> awt_input_handler_list;

//  ------------------------------------------------------------
//      class awt_string_handler : public awt_input_handler
//  ------------------------------------------------------------
// this handler handles string fields
class awt_string_handler : public awt_input_handler {
private:
    string  default_value;      // default value for awar if no data present

public:
    awt_string_handler(awt_input_mask_global *global_, const string& child_path_, const string& default_awar_value_, GB_TYPES default_type, const string& label_)
        : awt_input_handler(global_, child_path_, default_type, label_)
        , default_value(default_awar_value_)
    {
        mask_global()->get_root()->awar_string(awar_name().c_str(), default_value.c_str()); // generate a string AWAR
    }
    virtual ~awt_string_handler() {}

    virtual void awar_changed();
    virtual void db_changed();

    virtual string awar2db(const string& awar_content) const { return awar_content; }
    virtual string db2awar(const string& db_content) const { return db_content; }

    virtual void build_widget(AW_window *aws) = 0; // builds the widget at the current position
};

//  ------------------------------
//      class awt_input_field
//  ------------------------------
// awt_input_field holds a text field bound to a database string field
class awt_input_field : public awt_string_handler {
private:
    int    field_width;

public:
    awt_input_field(awt_input_mask_global *global_, const string& child_path_, const string& label_, int field_width_, const string& default_value, GB_TYPES default_type)
        : awt_string_handler(global_, child_path_, default_value, default_type, label_)
        , field_width(field_width_)
    { }
    virtual ~awt_input_field() {}

    virtual void build_widget(AW_window *aws);
};

//  ---------------------------------------------------------------
//      class awt_numeric_input_field : public awt_input_field
//  ---------------------------------------------------------------
class awt_numeric_input_field : public awt_input_field {
private:
    long min, max;

public:
    awt_numeric_input_field(awt_input_mask_global *global_, const string& child_path_, const string& label_, int field_width_, long default_value, long min_, long max_)
        : awt_input_field(global_, child_path_, label_, field_width_, GBS_global_string("%li", default_value), GB_FLOAT)
        , min(min_)
        , max(max_)
    {}
    virtual ~awt_numeric_input_field() {}

    virtual string awar2db(const string& awar_content) const;
};


//  --------------------------------------------------------
//      class awt_check_box : public awt_string_handler
//  --------------------------------------------------------
class awt_check_box : public awt_string_handler {
private:

public:
    awt_check_box(awt_input_mask_global *global_, const string& child_path_, const string& label_, bool default_checked)
        : awt_string_handler(global_, child_path_, default_checked ? "yes" : "no", GB_BITS, label_)
    {}
    virtual ~awt_check_box() {}

    virtual string awar2db(const string& awar_content) const;
    virtual string db2awar(const string& db_content) const;

    virtual void build_widget(AW_window *aws);
};

//  -----------------------------------------------------------
//      class awt_radio_button : public awt_string_handler
//  -----------------------------------------------------------
class awt_radio_button : public awt_string_handler {
private:
    int            default_position;
    bool           vertical;
    vector<string> buttons; // the awar contains the names of the buttons
    vector<string> values; // the database contains the names of the values

public:
    awt_radio_button(awt_input_mask_global *global_, const string& child_path_, const string& label_, int default_position_, bool vertical_, const vector<string>& buttons_, const vector<string>& values_)
        : awt_string_handler(global_, child_path_, buttons_[default_position_], GB_STRING, label_)
        , default_position(default_position_)
        , vertical(vertical_)
        , buttons(buttons_)
        , values(values_)
    {
        aw_assert(buttons.size() == values.size());
    }
    virtual ~awt_radio_button() {}

    virtual string awar2db(const string& awar_content) const;
    virtual string db2awar(const string& db_content) const;

    virtual void build_widget(AW_window *aws);
};


//  -----------------------------
//      class awt_input_mask
//  -----------------------------
// awt_input_mask holds the description of an input mask.
// an input mask is an i/o-interface to a database entry.

class awt_input_mask {
private:
    string                  mask_name;
    awt_input_mask_global   global;
    awt_input_handler_list  handlers;
    AW_window_simple       *aws;
    bool                    shall_reload_on_reinit;

public:
    awt_input_mask(AW_root *awr, GBDATA *gb_main, const string& mask_name_, const string& awar_root, awt_item_type itemtype_, const awt_item_type_selector *sel_)
        : mask_name(mask_name_)
        , global(awr, gb_main, awar_root, itemtype_, sel_)
        , aws(0)
        , shall_reload_on_reinit(false)
    {}
    // Initialisation is done in awt_create_input_mask
    // see also :  AWT_initialize_input_mask

    virtual ~awt_input_mask() { }

    void show() { aws->show(); }
    void hide() { aws->hide(); }

    void set_reload_on_reinit(bool dest) { shall_reload_on_reinit = dest; }
    bool reload_on_reinit() { return shall_reload_on_reinit; }

    AW_window_simple*& get_window() { return aws; }

    const awt_input_mask_global *mask_global() const { return &global; }
    awt_input_mask_global *mask_global() { return &global; }
    const string& get_maskname() const { return mask_name; }

    void add_handler(awt_input_handler_ptr handler) { handlers.push_back(handler); }

    void relink();
};

awt_item_type AWT_getItemType(const string& itemtype_name);

#else
#error AWT_input_mask.h included twice
#endif // AWT_INPUT_MASK_H

