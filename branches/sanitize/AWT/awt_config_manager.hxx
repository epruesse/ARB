//  ==================================================================== //
//                                                                       //
//    File      : awt_config_manager.hxx                                 //
//    Purpose   : general interface to store/restore                     //
//                a set of related awars                                 //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_CONFIG_MANAGER_HXX
#define AWT_CONFIG_MANAGER_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

struct AWT_config_mapping;

struct AWT_config_mapping_def {
    const char *awar_name;
    const char *config_name;
};

// -------------------
//      AWT_config

class AWT_config {
    // stores one specific configuration (key->value pairs)
    //
    // this class allows to modify the config_string before calling AWT_config_definition::write().
    // This is e.g. necessary if some config-entries change and you want to support
    // automatic conversion from old format to new format.

    AWT_config_mapping *mapping;
    GB_ERROR           parse_error; // set by AWT_config(const char *)

    AWT_config(const AWT_config&);
    AWT_config& operator = (const AWT_config&);
public:
    AWT_config(const char *config_string);
    AWT_config(const AWT_config_mapping *cfgname_2_awar); // internal use (reads current awar values)
    ~AWT_config();

    GB_ERROR parseError() const { return parse_error; }

    // props + modifiers
    bool has_entry(const char *entry) const; // returns true if mapping contains 'entry'
    const char *get_entry(const char *entry) const; // returns value of 'entry'
    void set_entry(const char *entry, const char *value); // sets a (new) entry to value
    void delete_entry(const char *entry); // deletes an existing 'entry'

    // result
    char *config_string() const; // return current state as config string
    GB_ERROR write_to_awars(const AWT_config_mapping *cfgname_2_awar) const; // internal use (write config into awars)
};

// ------------------------------
//      AWT_config_definition

class AWT_config_definition {
private:
    AWT_config_mapping *config_mapping; // defines config-name -> awar-name relation

    AWT_config_definition(const AWT_config_definition&);
    AWT_config_definition& operator = (const AWT_config_definition&);
public:
    AWT_config_definition();
    AWT_config_definition(AWT_config_mapping_def *mapping_definition); // simple definition
    ~AWT_config_definition();

    void add(const char *awar_name, const char *config_name);
    void add(const char *awar_name, const char *config_name, int counter);
    void add(AWT_config_mapping_def *mapping_definition);

    char *read() const;         // awars -> config string (heap copy)
    void write(const char *config_string) const; // config string -> awars (use to restore a saved configuration)
};

// ----------------------------------------
//      callbacks from config manager :

typedef char *(*AWT_store_config_to_string)(AW_CL cl1, AW_CL cl2);
typedef void (*AWT_load_config_from_string)(const char *stored_string, AW_CL cl1, AW_CL cl2);

// ----------------------------------
// the config manager itself
// adds button at cursor position when called (from a window generator function)

void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, AWT_store_config_to_string store, AWT_load_config_from_string load, AW_CL cl1, AW_CL cl2, const char *macro_id = NULL);

#else
#error awt_config_manager.hxx included twice
#endif // AWT_CONFIG_MANAGER_HXX

