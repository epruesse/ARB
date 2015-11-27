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

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef CB_H
#include <cb.h>
#endif

struct AWT_config_mapping;

struct AWT_config_mapping_def {
    const char *awar_name;
    const char *config_name;
};

struct AWT_predefined_config {
    const char *name;        // of predefined config (has to begin with '*')
    const char *description;
    const char *config;      // config string (as generated by AWT_config::config_string)
};

// -------------------
//      AWT_config

class AWT_config : virtual Noncopyable {
    // stores one specific configuration (key->value pairs)
    //
    // this class allows to modify the config_string before calling AWT_config_definition::write().
    // This is e.g. necessary if some config-entries change and you want to support
    // automatic conversion from old format to new format.

    AWT_config_mapping *mapping;
    GB_ERROR           parse_error; // set by AWT_config(const char *)

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
    void write_to_awars(const AWT_config_mapping *cfgname_2_awar, bool warn) const; // internal use (write config into awars)

    void get_entries(class ConstStrArray& to_array);
};

// ------------------------------
//      AWT_config_definition

class AWT_config_definition : virtual Noncopyable {
    AWT_config_mapping *config_mapping; // defines config-name -> awar-name relation

public:
    AWT_config_definition();
    AWT_config_definition(AWT_config_mapping_def *mapping_definition); // simple definition
    ~AWT_config_definition();

    void add(const char *awar_name, const char *config_name);
    void add(const char *awar_name, const char *config_name, int counter);
    void add(const AWT_config_mapping_def *mapping_definition);

    char *read() const;                          // awars -> config string (heap copy)
    void write(const char *config_string) const; // config string -> awars (use to restore a saved configuration)
    void reset() const;                          // reset awars to defaults
};

// ----------------------------------------
//      callbacks from config manager :

DECLARE_CBTYPE_FVV_AND_BUILDERS(ConfigSetupCallback,   void, AWT_config_definition&); // defines makeConfigSetupCallback
DECLARE_CBTYPE_VV_AND_BUILDERS (StoreConfigCallback,   char*);                        // defines makeStoreConfigCallback
DECLARE_CBTYPE_FVV_AND_BUILDERS(RestoreConfigCallback, void, const char *);           // defines makeRestoreConfigCallback

// ----------------------------------
// the config manager itself
// adds button at cursor position when called (from a window generator function)

void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, const StoreConfigCallback& store, const RestoreConfigCallback& load_or_reset, const char *macro_id = NULL, const AWT_predefined_config *predef = NULL);
void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, ConfigSetupCallback setup_cb, const char *macro_id = NULL, const AWT_predefined_config *predef = NULL);
void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, const AWT_config_mapping_def *mapping, const char *macro_id = NULL, const AWT_predefined_config *predef = NULL);

// -------------------------------
//      modify stored configs

typedef char *(*ConfigModifyCallback)(const char *key, const char *value, AW_CL cl_user);
void AWT_modify_managed_configs(AW_default default_file_, const char *id, ConfigModifyCallback mod_cb, AW_CL cl_user);

#else
#error awt_config_manager.hxx included twice
#endif // AWT_CONFIG_MANAGER_HXX

