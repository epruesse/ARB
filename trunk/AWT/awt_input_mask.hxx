//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.h                                       //
//    Purpose   : General input masks                                    //
//    Time-stamp: <Wed Aug/15/2001 22:52 MET Coder@ReallySoft.de>        //
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
// awt_item_type_selector holds all data needed to attach a mask to a database
// element (i.e. a species, a gene or an experiment)

class awt_item_type_selector {
private:
public:
    awt_item_type_selector() {}
    virtual ~awt_item_type_selector() {}

    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const = 0; // add callbacks to awars (i.e. to AWAR_SPECIES_NAME)
    virtual GBDATA *current(AW_root *root) const                                                    = 0; // give the current item
    virtual const char *getKeyPath() const                                                          = 0; // give the keypath for items
};

//  ------------------------------------
//      class awt_input_mask_global
//  ------------------------------------
// data global to one input mask
class awt_input_mask_global {
private:
    AW_root *awr;
    GBDATA  *gb_main;
    string   awar_root;

    const awt_item_type_selector& sel;

public:
    awt_input_mask_global(AW_root *awr_, GBDATA *gb_main_, const string& awar_root_, const awt_item_type_selector& sel_)
        : awr(awr_)
        , gb_main(gb_main_)
        , awar_root(awar_root_)
        , sel(sel_)
    {}
    virtual ~awt_input_mask_global() {}

    AW_root *get_root() { return awr; }
    GBDATA *get_gb_main() { return gb_main; }
    const string& get_awar_root() const { return awar_root; }
    const awt_item_type_selector& get_selector() const { return sel; }
};


//  --------------------------------
//      class awt_input_handler
//  --------------------------------
// a awt_input_handler is the base class for all types of input-handlers
class awt_input_handler {
private:
    awt_input_mask_global *global;
    GBDATA                *gb_item; // item this handler is linked to
    // if gb_item == 0 then no callbacks are installed

    GBDATA   *gbd;              // link to database
    string    label;            // label of this input handler
    string    child_path;       // path in database from item to handled child
    GB_TYPES  db_type;             // type of database field

    GB_ERROR add_db_callbacks();
    GB_ERROR remove_db_callbacks();

    GB_ERROR add_awar_callbacks();
    GB_ERROR remove_awar_callbacks();

public:
    GB_ERROR link_to(GBDATA *gb_new_item); // link to a new item
    GB_ERROR unlink() { return link_to(0); }

    awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_);
    virtual ~awt_input_handler() { unlink(); }

    GBDATA *data() { return gbd; }
    GBDATA *item() { return gb_item; }
    GB_TYPES type() const { return db_type; }
    void set_type(GB_TYPES typ) { db_type = typ; }

    awt_input_mask_global *get_global() { return global; }

    const string& get_child_path() const { return child_path; }
    string awar_name() { return global->get_awar_root()+"/"+child_path; }
    AW_awar *awar() { return global->get_root()->awar(awar_name().c_str()); }

    // callbacks are handled via the following virtual functions :
    virtual void awar_changed() = 0;
    virtual void db_changed()   = 0;

    virtual void build_widget(AW_window *aws) = 0; // builds the widget at the current position
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
    awt_string_handler(awt_input_mask_global *global_, const string& child_path_, const string& default_awar_value_, GB_TYPES default_type)
        : awt_input_handler(global_, child_path_, default_type)
        , default_value(default_awar_value_)
    {
        get_global()->get_root()->awar_string(awar_name().c_str(), default_value.c_str()); // generate a string AWAR
    }
    virtual ~awt_string_handler() {}

    virtual void awar_changed();
    virtual void db_changed();

    virtual string awar2db(const string& awar_content) { return awar_content; }
    virtual string db2awar(const string& db_content) { return db_content; }

    virtual void build_widget(AW_window *aws) = 0; // builds the widget at the current position
};

//  ------------------------------
//      class awt_input_field
//  ------------------------------
// awt_input_field holds a text field bound to a database string field
class awt_input_field : public awt_string_handler {
private:
    int    field_width;
    string label;

public:
    awt_input_field(awt_input_mask_global *global_, const string& child_path_, const string& label_, int field_width_)
        : awt_string_handler(global_, child_path_, "", GB_STRING)
        , field_width(field_width_)
        , label(label_)
    {
    }
    virtual ~awt_input_field() {}

    void build_widget(AW_window *aws);
};

//  --------------------------------------------------------
//      class awt_check_box : public awt_string_handler
//  --------------------------------------------------------
class awt_check_box : public awt_string_handler {
private:
    string label;

public:
    awt_check_box(awt_input_mask_global *global_, const string& child_path_, const string& label_, bool default_checked)
        : awt_string_handler(global_, child_path_, default_checked ? "yes" : "no", GB_BITS)
        , label(label_)
    {}
    virtual ~awt_check_box() {}

    virtual string awar2db(const string& awar_content);
    virtual string db2awar(const string& db_content);

    void build_widget(AW_window *aws);
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

public:
    awt_input_mask(AW_root *awr, GBDATA *gb_main, const string& mask_name_, const string& awar_root, const awt_item_type_selector& sel_)
        : mask_name(mask_name_)
        , global(awr, gb_main, awar_root, sel_)
        , aws(0)
    {}
    // Initialisation is done in awt_create_input_mask
    // see also :  AWT_initialize_input_mask

    virtual ~awt_input_mask() {}

    AW_window_simple*& get_window() { return aws; }

    const awt_input_mask_global *get_global() const { return &global; }
    awt_input_mask_global *get_global() { return &global; }
    const string& get_maskname() const { return mask_name; }

    void add_handler(awt_input_handler_ptr handler) { handlers.push_back(handler); }

    void relink();
};

#else
#error AWT_input_mask.h included twice
#endif // AWT_INPUT_MASK_H

