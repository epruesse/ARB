// ============================================================ //
//                                                              //
//   File      : awt_input_mask_internal.hxx                    //
//   Purpose   : input mask internal classes                    //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2009   //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef AWT_INPUT_MASK_INTERNAL_HXX
#define AWT_INPUT_MASK_INTERNAL_HXX

#ifndef AWT_INPUT_MASK_HXX
#include <awt_input_mask.hxx>
#endif
#ifndef AWT_HOTKEYS_HXX
#include <awt_hotkeys.hxx>
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef AW_AWAR_HXX
#include <aw_awar.hxx>
#endif
#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef AWT_HXX
#include "awt.hxx"
#endif


#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif


// ---------------------------
//      forward references

class awt_mask_item;
class awt_linked_to_item;
class awt_viewport;

// --------------------------------------
//      class awt_input_mask_id_list

class awt_input_mask_id_list {
private:
    // maps ids to corresponding input_handlers
    std::map<std::string, awt_mask_item*> id;

public:
    awt_input_mask_id_list() {}
    virtual ~awt_input_mask_id_list() {}

    awt_mask_item *lookup(const std::string& name) const {
        std::map<std::string, awt_mask_item*>::const_iterator found = id.find(name);
        return (found == id.end()) ? 0 : found->second;
    }
    GB_ERROR add(const std::string& name, awt_mask_item *item);
    GB_ERROR remove(const std::string& name);
    bool empty() const { return id.empty(); }
};

//  ------------------------------------
//      class awt_input_mask_global
//
// data global to one input mask
class awt_input_mask_global : virtual Noncopyable {
private:
    mutable AW_root *awr;
    mutable GBDATA  *gb_main;
    std::string      mask_name;                     // filename of mask-file
    std::string      internal_mask_name;            // filename of mask-file (prefixed by 0( = local) or 1( = global))
    std::string      mask_id;                       // key generated from mask_name
    bool             local_mask;                    // true if mask was found in "$ARB_PROP/inputMasks"
    awt_item_type    itemtype;                      // what kind of item do we handle ?

    bool test_edit_enabled;                         // true -> the global awar AWAR_INPUT_MASKS_EDIT_ENABLE should be tested before writing to database

    const awt_item_type_selector *sel;

    awt_hotkeys                   hotkeys;
    awt_input_mask_id_list        ids;              // local
    static awt_input_mask_id_list global_ids;

    static std::string generate_id(const std::string& mask_name_);


public:
    awt_input_mask_global(AW_root *awr_, GBDATA *gb_main_, const std::string& mask_name_, awt_item_type itemtype_, bool local, const awt_item_type_selector *sel_, bool test_edit_enabled_)
        : awr(awr_)
        , gb_main(gb_main_)
        , mask_name(mask_name_)
        , internal_mask_name(std::string(1, local ? '0' : '1')+mask_name_)
        , mask_id(generate_id(mask_name_))
        , local_mask(local)
        , itemtype(itemtype_)
        , test_edit_enabled(test_edit_enabled_)
        , sel(sel_)
    {
        awt_assert(mask_name_[0] != '0' && mask_name_[0] != '1');
    }
    virtual ~awt_input_mask_global() {
        awt_assert(ids.empty());
    }

    bool is_local_mask() const { return local_mask; }
    AW_root *get_root() const { return awr; }
    GBDATA *get_gb_main() const { return gb_main; }
    const std::string& get_maskname() const { return mask_name; }
    const std::string& get_internal_maskname() const { return internal_mask_name; }
    std::string get_maskid() const { return mask_id; }
    awt_item_type get_itemtype() const { return itemtype; }
    const awt_item_type_selector *get_selector() const { awt_assert(sel); return sel; }
    const char* hotkey(const std::string& label)  { return hotkeys.hotkey(label); }

    GBDATA *get_selected_item() const { return get_selector()->current(get_root(), get_gb_main()); }

    bool has_local_id(const std::string& name) const { return ids.lookup(name); }
    bool has_global_id(const std::string& name) const { return global_ids.lookup(name); }

    GB_ERROR add_local_id(const std::string& name, awt_mask_item *handler) {
        if (has_global_id(name)) return GBS_global_string("ID '%s' already defined as GLOBAL", name.c_str());
        return ids.add(name, handler);
    }

    GB_ERROR add_global_id(const std::string& name, awt_mask_item *handler) {
        if (has_local_id(name)) return GBS_global_string("ID '%s' already defined as LOCAL", name.c_str());
        return global_ids.add(name, handler);
    }

    GB_ERROR remove_local_id(const std::string& name) { return ids.remove(name); }
    GB_ERROR remove_id(const std::string& name) {
        if (has_local_id(name)) return remove_local_id(name);
        if (has_global_id(name)) return 0; // global ids are only created (never removed)
        return GBS_global_string("ID '%s' not found - can't remove id", name.c_str());
    }

    awt_mask_item *get_identified_item(const std::string& name, GB_ERROR& error) const {
        awt_mask_item *found = 0;
        if (!error) {
            found             = ids.lookup(name);
            if (!found) found = global_ids.lookup(name);
            if (!found) error = GBS_global_string("No item '%s' declared", name.c_str());
        }
        return found;
    }

    void no_item_selected() const;
    bool edit_allowed() const;
};

//  ----------------------------
//      class awt_mask_item
//
class awt_mask_item {
    // works as base class for all elements of a input-mask

    // basic item members
    // awt_input_mask_global *global;    // reference  // @@@ make ref to make class copyable
    awt_input_mask_global& global;
    SmartPtr<std::string>  name;      // name of this item (optional -- caused i.e. by script command 'ID')

public:
    awt_mask_item(awt_input_mask_global& global_); // awar_base has to be unique (in every mask)
    virtual ~awt_mask_item();

    const awt_input_mask_global& mask_global() const { return global; }
    awt_input_mask_global& mask_global() { return global; }

    bool has_name() const { return !name.isNull(); }
    const std::string& get_name() const { awt_assert(has_name()); return *name; }
    GB_ERROR set_name(const std::string& name_, bool is_global);
    GB_ERROR remove_name();

    inline const awt_viewport *to_viewport() const;
    inline awt_viewport *to_viewport();

    inline const awt_linked_to_item *to_linked_item() const;
    inline awt_linked_to_item *to_linked_item();

    bool is_viewport() const { return to_viewport() != 0; }
    bool is_linked_item() const { return to_linked_item() != 0; }

    virtual std::string get_value() const                    = 0; // reads the current value of the item
    virtual GB_ERROR set_value(const std::string& new_value) = 0; // assigns a new value to the item
};

//  --------------------------------
//      class awt_mask_awar_item
//
// holds an awar
class awt_mask_awar_item :  public awt_mask_item {
private:
    // awar related members
    std::string     awarName;        // name of the awar

protected:

    void add_awarItem_callbacks();
    void remove_awarItem_callbacks();

public:
    awt_mask_awar_item(awt_input_mask_global& global_, const std::string& awar_base, const std::string& default_value, bool saved_with_properties);
    ~awt_mask_awar_item() OVERRIDE { remove_awarItem_callbacks(); }

    virtual void awar_changed() = 0; // called when awar changes

    std::string awar_name() const  { return awarName; }
    const AW_awar *awar() const { return mask_global().get_root()->awar(awarName.c_str()); }
    AW_awar *awar() { return mask_global().get_root()->awar(awarName.c_str()); }

    std::string get_value() const OVERRIDE {  // reads the current value of the item
        return const_cast<AW_awar*>(awar())->read_string();
    }
    GB_ERROR set_value(const std::string& new_value) OVERRIDE { // assigns a new value to the item
        awar()->write_string(new_value.c_str());
        return 0; // an overloaded method may return an error
    }
};

// ---------------------------
//      class awt_viewport
//
// awar bound to a widget
class awt_viewport : public awt_mask_awar_item {
private:
    std::string label;               // label of viewport

public:
    awt_viewport(awt_input_mask_global& global_, const std::string& awar_base, const std::string& default_value, bool saved_with_properties, const std::string& label_)
        : awt_mask_awar_item(global_, awar_base, default_value, saved_with_properties)
        , label(label_)
    {}
    awt_viewport(const awt_mask_awar_item& ref_item, const std::string& label_)
        : awt_mask_awar_item(ref_item)
        , label(label_)
    {}
    ~awt_viewport() OVERRIDE {}

    const std::string& get_label() const { return label; }
    virtual void build_widget(AW_window *aws) = 0; // builds the widget at the current position
};

// ---------------------------
//      class awt_variable
//
// awar NOT bound to widget; is saved in properties
class awt_variable : public awt_mask_awar_item {
private:
    bool is_global;

    static std::string generate_baseName(const awt_input_mask_global& global_, const std::string& id, bool is_global_) {
        // the generated name is NOT enumerated, because any reference to a variable should
        // work on the same awar
        return
            is_global_
            ? std::string("global_")+id
            : std::string(GBS_global_string("local_%s_%s", global_.get_maskid().c_str(), id.c_str()));
    }
public:
    awt_variable(awt_input_mask_global& global_, const std::string& id, bool is_global_, const std::string& default_value, GB_ERROR& error);
    ~awt_variable() OVERRIDE;
    void awar_changed() OVERRIDE {
#if defined(DEBUG)
        printf("awt_variable was changed\n");
#endif // DEBUG
    }
};


//  ------------------------
//      class awt_script
//
class awt_script : public awt_mask_item {
private:
    std::string script;

public:
    awt_script(awt_input_mask_global& global_, const std::string& script_)
        : awt_mask_item(global_)
        , script(script_)
    {}
    ~awt_script() OVERRIDE {}

    std::string get_value() const OVERRIDE; // reads the current value of the item
    GB_ERROR set_value(const std::string& /* new_value */) OVERRIDE; // assigns a new value to the item
};

//  ---------------------------------
//      class awt_linked_to_item
//
class awt_linked_to_item : virtual Noncopyable {
private:
    GBDATA                *gb_item; // item this handler is linked to
    // if gb_item == 0 then no callbacks are installed

protected:

    virtual GB_ERROR add_db_callbacks();
    virtual void     remove_db_callbacks();

    void set_item(GBDATA *new_item) {
#if defined(DEBUG)
        printf("gb_item=%p new_item=%p\n", gb_item, new_item);
#endif // DEBUG
        gb_item = new_item;
    }

public:
    awt_linked_to_item() : gb_item(0) {}
    virtual ~awt_linked_to_item() {
        awt_assert(!gb_item); // you forgot to call awt_linked_to_item::unlink from where you destroy 'this'
    }

    GBDATA *item() { return gb_item; }
    virtual GB_ERROR link_to(GBDATA *gb_new_item) = 0; // link to a new item

    GB_ERROR unlink() { return link_to(0); }
    virtual GB_ERROR relink() = 0; // used by callbacks to relink awt_input_handler

    virtual void general_item_change() {} // called if item was changed (somehow)
    virtual void db_changed() = 0;
};


//  ---------------------------------
//      class awt_script_viewport
//
class awt_script_viewport : public awt_viewport, public awt_linked_to_item { // derived from a Noncopyable
private:
    const awt_script *script;
    int               field_width;

    static std::string generate_baseName(const awt_input_mask_global& global_) {
        static int awar_counter = 0;
        return GBS_global_string("%s/scriptview_%i", global_.get_maskid().c_str(), awar_counter++);
    }

public:
    awt_script_viewport(awt_input_mask_global& global_, const awt_script *script_, const std::string& label_, long field_width_);
    ~awt_script_viewport() OVERRIDE;

    GB_ERROR link_to(GBDATA *gb_new_item) OVERRIDE; // link to a new item
    GB_ERROR relink() OVERRIDE { return link_to(mask_global().get_selected_item()); }

    void build_widget(AW_window *aws) OVERRIDE; // builds the widget at the current position
    void awar_changed() OVERRIDE;
    void db_changed() OVERRIDE;
};


//  --------------------------------
//      class awt_input_handler
//
// an awt_input_handler is an awt_viewport bound to a database element
class awt_input_handler : public awt_viewport, public awt_linked_to_item { // derived from a Noncopyable
private:
    GBDATA      *gbd;           // link to database
    std::string  child_path;    // path in database from item to handled child
    GB_TYPES     db_type;       // type of database field
    bool         in_destructor;

    GB_ERROR add_db_callbacks() OVERRIDE;
    void     remove_db_callbacks() OVERRIDE;

    static std::string generate_baseName(const awt_input_mask_global& global_, const std::string& child_path) {
        // the generated name is enumerated to allow different awt_input_handler's to be linked
        // to the same child_path
        static int awar_counter = 0;
        return GBS_global_string("%s/handler_%s_%i", global_.get_maskid().c_str(), child_path.c_str(), awar_counter++);
    }


public:
    awt_input_handler(awt_input_mask_global& global_, const std::string& child_path_, GB_TYPES type_, const std::string& label_);
    ~awt_input_handler() OVERRIDE;

    GB_ERROR link_to(GBDATA *gb_new_item) OVERRIDE; // link to a new item
    GB_ERROR relink() OVERRIDE { return link_to(mask_global().get_selected_item()); }

    GBDATA *data() { return gbd; }

    GB_TYPES type() const { return db_type; }
    void set_type(GB_TYPES typ) { db_type = typ; }

    const std::string& get_child_path() const { return child_path; }
};

typedef SmartPtr<awt_mask_item>      awt_mask_item_ptr;
typedef std::list<awt_mask_item_ptr> awt_mask_item_list;

//  --------------------------------
//      class awt_string_handler
//
// this handler handles string fields
class awt_string_handler : public awt_input_handler {
private:
    std::string  default_value;      // default value for awar if no data present

public:
    awt_string_handler(awt_input_mask_global& global_, const std::string& child_path_, const std::string& default_awar_value_, GB_TYPES default_type, const std::string& label_)
        : awt_input_handler(global_, child_path_, default_type, label_)
        , default_value(default_awar_value_)
    {
    }
    ~awt_string_handler() OVERRIDE {}

    void awar_changed() OVERRIDE;
    void db_changed() OVERRIDE;

    virtual std::string awar2db(const std::string& awar_content) const { return awar_content; }
    virtual std::string db2awar(const std::string& db_content) const { return db_content; }

    void build_widget(AW_window *aws) OVERRIDE = 0; // builds the widget at the current position
};

//  ------------------------------
//      class awt_input_field
//
// awt_input_field holds a text field bound to a database string field
class awt_input_field : public awt_string_handler {
private:
    int    field_width;

public:
    awt_input_field(awt_input_mask_global& global_, const std::string& child_path_, const std::string& label_, int field_width_, const std::string& default_value_, GB_TYPES default_type)
        : awt_string_handler(global_, child_path_, default_value_, default_type, label_)
        , field_width(field_width_)
    {}
    ~awt_input_field() OVERRIDE {}

    void build_widget(AW_window *aws) OVERRIDE;
};

//  -------------------------------
//      class awt_text_viewport
//
class awt_text_viewport : public awt_viewport {
private:
    int    field_width;

public:
    awt_text_viewport(const awt_mask_awar_item *item, const std::string& label_, long field_width_)
        : awt_viewport(*item, label_)
        , field_width(field_width_)
    {}
    ~awt_text_viewport() OVERRIDE {}

    void awar_changed() OVERRIDE {
#if defined(DEBUG)
        printf("awt_text_viewport awar changed!\n");
#endif // DEBUG
    }
    void build_widget(AW_window *aws) OVERRIDE;
};


//  -------------------------------------
//      class awt_numeric_input_field
//
class awt_numeric_input_field : public awt_input_field {
private:
    long min, max;

public:
    awt_numeric_input_field(awt_input_mask_global& global_, const std::string& child_path_, const std::string& label_, int field_width_, long default_value_, long min_, long max_)
        : awt_input_field(global_, child_path_, label_, field_width_, GBS_global_string("%li", default_value_), GB_FLOAT)
        , min(min_)
        , max(max_)
    {}
    ~awt_numeric_input_field() OVERRIDE {}

    std::string awar2db(const std::string& awar_content) const OVERRIDE;
};


//  ---------------------------
//      class awt_check_box
//
class awt_check_box : public awt_string_handler {
private:

public:
    awt_check_box(awt_input_mask_global& global_, const std::string& child_path_, const std::string& label_, bool default_checked)
        : awt_string_handler(global_, child_path_, default_checked ? "yes" : "no", GB_BITS, label_)
    {}
    ~awt_check_box() OVERRIDE {}

    std::string awar2db(const std::string& awar_content) const OVERRIDE;
    std::string db2awar(const std::string& db_content) const OVERRIDE;

    void build_widget(AW_window *aws) OVERRIDE;
};

//  ------------------------------
//      class awt_radio_button
//
class awt_radio_button : public awt_string_handler {
private:
    int            default_position;
    bool           vertical;
    std::vector<std::string> buttons; // the awar contains the names of the buttons
    std::vector<std::string> values; // the database contains the names of the values

public:
    awt_radio_button(awt_input_mask_global& global_, const std::string& child_path_, const std::string& label_, int default_position_, bool vertical_, const std::vector<std::string>& buttons_, const std::vector<std::string>& values_)
        : awt_string_handler(global_, child_path_, buttons_[default_position_], GB_STRING, label_)
        , default_position(default_position_)
        , vertical(vertical_)
        , buttons(buttons_)
        , values(values_)
    {
        awt_assert(buttons.size() == values.size());
    }
    ~awt_radio_button() OVERRIDE {}

    std::string awar2db(const std::string& awar_content) const OVERRIDE;
    std::string db2awar(const std::string& db_content) const OVERRIDE;

    void build_widget(AW_window *aws) OVERRIDE;

    size_t no_of_toggles() const { return buttons.size(); }
    size_t default_toggle() const { return default_position; }
};


//  -----------------------------
//      class awt_input_mask
//
// awt_input_mask holds the description of an input mask.
// an input mask is an i/o-interface to a database entry.

class awt_input_mask : virtual Noncopyable {
private:
    awt_input_mask_global  global;
    awt_mask_item_list     handlers;
    AW_window_simple      *aws;
    bool                   shall_reload_on_reinit;

    void link_to(GBDATA *gbitem);
    
public:
    awt_input_mask(AW_root *awr, GBDATA *gb_main, const std::string& mask_name_, awt_item_type itemtype_, bool local, const awt_item_type_selector *sel_, bool test_edit_enabled)
        : global(awr, gb_main, mask_name_, itemtype_, local, sel_, test_edit_enabled)
        , aws(0)
        , shall_reload_on_reinit(false)
    {}
    // Initialization is done in awt_create_input_mask
    // see also :  AWT_initialize_input_mask

    virtual ~awt_input_mask();

    void show() { aws->activate(); }
    void hide() { aws->hide(); }

    void set_reload_on_reinit(bool dest) { shall_reload_on_reinit = dest; }
    bool reload_on_reinit() const { return shall_reload_on_reinit; }

    AW_window_simple*& get_window() { return aws; }

    const awt_input_mask_global& mask_global() const { return global; }
    awt_input_mask_global& mask_global() { return global; }

    void add_handler(awt_mask_item_ptr handler) { handlers.push_back(handler); }

    void relink() { link_to(global.get_selected_item()); }
    void unlink() { link_to(NULL); }
};

//  ----------------
//      casts :

inline const awt_linked_to_item *awt_mask_item::to_linked_item() const { const awt_linked_to_item *linked = dynamic_cast<const awt_linked_to_item*>(this); return linked; }
inline       awt_linked_to_item *awt_mask_item::to_linked_item()       {       awt_linked_to_item *linked = dynamic_cast<      awt_linked_to_item*>(this); return linked; }

inline const awt_viewport *awt_mask_item::to_viewport() const { const awt_viewport *viewport = dynamic_cast<const awt_viewport*>(this); return viewport; }
inline       awt_viewport *awt_mask_item::to_viewport()       {       awt_viewport *viewport = dynamic_cast<      awt_viewport*>(this); return viewport; }

#else
#error awt_input_mask_internal.hxx included twice
#endif // AWT_INPUT_MASK_INTERNAL_HXX
