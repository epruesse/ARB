//  ==================================================================== //
//                                                                       //
//    File      : AWT_config_manager.cxx                                 //
//    Purpose   :                                                        //
//    Time-stamp: <Tue Aug/17/2004 14:29 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "awt.hxx"
#include "awt_config_manager.hxx"

#include <map>
#include <string>

using namespace std;



//  --------------------------------
//      class AWT_configuration
//  --------------------------------
class AWT_configuration {
private:
    string id;

    AWT_store_config_to_string  store;
    AWT_load_config_from_string load;
    AW_CL                       client1; // client data
    AW_CL                       client2;

    AW_window  *last_client_aww;
    AW_default  default_file;

public:
    AWT_configuration(AW_window *aww, AW_default default_file_, const char *id_, AWT_store_config_to_string store_,
                      AWT_load_config_from_string load_, AW_CL cl1, AW_CL cl2)
    {
        id              = id_;
        store           = store_;
        load            = load_;
        client1         = cl1;
        client2         = cl2;
        last_client_aww = aww;
        default_file    = default_file_;
    }
    virtual ~AWT_configuration() {}

    bool operator<(const AWT_configuration& other) const { return id<other.id; }
    string get_awar_name(const string& subname) const { return string("general_configs/")+id+'/'+subname; }
    string get_awar_value(const string& subname, const char *default_value = "") const {
        AW_root *aw_root   = last_client_aww->get_root();
        string   awar_name = get_awar_name(subname);
        char    *value     = aw_root->awar_string(awar_name.c_str(), default_value, default_file)->read_string();
        string   result    = value;
        free(value);
        return result;
    }
    void set_awar_value(const string& subname, const string& new_value) const {
        AW_root *aw_root   = last_client_aww->get_root();
        aw_root->awar_string(get_awar_name(subname).c_str(), "")->write_string(new_value.c_str());
    }

    char *Store() const { return store(last_client_aww, client1, client2); }
    void Restore(const string& s) const { return load(last_client_aww, s.c_str(), client1, client2); }
};

//  ---------------------------------------------------------------------------------
//      void remove_from_configs(const string& config, string& existing_configs)
//  ---------------------------------------------------------------------------------
void remove_from_configs(const string& config, string& existing_configs) {
    size_t start = -1U;
    printf("erasing '%s' from '%s'\n", config.c_str(), existing_configs.c_str());

    while (1) {
        start = existing_configs.find(config, start+1);
        if (start == string::npos) break; // not found
        if (start == 0 || existing_configs[start-1] == ';') { // config starts with string
            size_t stop = start+config.length();
            if (stop != existing_configs.length()) {
                if (stop>existing_configs.length()) break; // not found
                if (existing_configs[stop] != ';') continue; // name continues
            }
            existing_configs.erase(start, stop-start+1);
            if (existing_configs[existing_configs.length()-1] == ';') {
                existing_configs.erase(existing_configs.length()-1);
            }
            remove_from_configs(config, existing_configs);
            break;
        }
    }
#if defined(DEBUG)
    printf("result: '%s'\n", existing_configs.c_str());
#endif // DEBUG
}
// ---------------------------------------------------------
//      static char *correct_key_name(const char *name)
// ---------------------------------------------------------
static char *correct_key_name(const char *name) {
    char *corrected = GBS_string_2_key(name);

    if (strcmp(corrected, "__") == 0) {
        free(corrected);
        corrected = strdup("");
    }

    return corrected;
}

//  ------------------------------------------------------------------------------
//      static void AWT_start_config_manager(AW_window *aww, AW_CL cl_config)
//  ------------------------------------------------------------------------------
static void AWT_start_config_manager(AW_window *aww, AW_CL cl_config)
{
    AWT_configuration *config           = (AWT_configuration*)cl_config;
    string             existing_configs = config->get_awar_value("existing");
    config->get_awar_value("current"); // create!
    bool               reopen           = false;
    char              *title            = GBS_global_string_copy("Configurations for '%s'", aww->window_name);

    const char *buttons = "RESTORE,STORE,DELETE,CLOSE,HELP";
    char       *result  = aw_string_selection(title, "Enter a new or select an existing config",
                                              config->get_awar_name("current").c_str(), 0, existing_configs.c_str(),
                                              buttons, correct_key_name);
    int         button  = aw_string_selection_button();

    if (button >= 0 && button <= 2) { // RESTORE, STORE and DELETE
        if (!result || !result[0]) { // did user specify a config-name ?
            aw_message("Please enter or select a config");
        }
        else {
            string awar_name = string("cfg_")+result;
            
            switch (button) {
                case 0: {           // RESTORE
                    config->Restore(config->get_awar_value(awar_name));
                    config->set_awar_value("current", result);
                    break;
                }
                case 1: {               // STORE
                    remove_from_configs(result, existing_configs); // remove existing config

                    if (existing_configs.length()) existing_configs = string(result)+';'+existing_configs;
                    else existing_configs                           = result;

                    char *config_string = config->Store();
                    config->set_awar_value(awar_name, config_string);
                    free(config_string);
                    config->set_awar_value("current", result);
                    config->set_awar_value("existing", existing_configs);
                    reopen = true;
                    break;
                }
                case 2: {               // DELETE
                    remove_from_configs(result, existing_configs); // remove existing config
                    config->set_awar_value("current", "");
                    config->set_awar_value("existing", existing_configs);

                    // config is not really deleted from properties
                    reopen = true;
                    break;
                }
            }
        }
    }
    else {
        if (button == 4) { // HELP
            AW_POPUP_HELP(aww, (AW_CL)"configurations.hlp");
        }
    }

    free(title);
    free(result);

    //     if (reopen) AWT_start_config_manager(aww, cl_config); // crashes!
}

//  -------------------------------------------------------------------------------------------------------------------------------------
//      void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, AWT_store_config_to_string store_cb,
//  -------------------------------------------------------------------------------------------------------------------------------------
void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, AWT_store_config_to_string store_cb,
                               AWT_load_config_from_string load_cb, AW_CL cl1, AW_CL cl2)
{
    AWT_configuration *config = new AWT_configuration(aww, default_file_, id, store_cb, load_cb, cl1, cl2);
    // config will not be freed!!!

    aww->button_length(0); // -> autodetect size by size of graphic 
    aww->callback(AWT_start_config_manager, (AW_CL)config);
    aww->create_button("SAVELOAD_CONFIG", "#disk.bitmap", 0);
}

static GB_ERROR decode_escapes(string& s) {
    string::iterator f = s.begin();
    string::iterator t = s.begin();

    for (; f != s.end(); ++f, ++t) {
        if (*f == '\\') {
            ++f;
            if (f == s.end()) return GBS_global_string("Trailing \\ in '%s'", s.c_str());
            switch (*f) {
                case 'n':
                    *t = '\n';
                    break;
                case 'r':
                    *t = '\r';
                    break;
                case 't':
                    *t = '\t';
                    break;
                default:
                    *t = *f;
                    break;
            }
        }
        else {
            *t = *f;
        }
    }

    s.erase(t, f);

    return 0;
}

static void encode_escapes(string& s, const char *to_escape) {
    string neu;
    neu.reserve(s.length()*2+1);

    for (string::iterator p = s.begin(); p != s.end(); ++p) {
        if (*p == '\\' || strchr(to_escape, *p) != 0) {
            neu = neu+'\\'+*p;
        }
        else if (*p == '\n') { neu = neu+"\\n"; }
        else if (*p == '\r') { neu = neu+"\\r"; }
        else if (*p == '\t') { neu = neu+"\\t"; }
        else { neu = neu+*p; }
    }
    s = neu;
}

typedef map<string, string> config_map;
struct AWT_config_mapping {
    config_map cmap;

    config_map::iterator entry(const string &e) { return cmap.find(e); }

    config_map::iterator begin() { return cmap.begin(); }
    config_map::const_iterator end() const { return cmap.end(); }
    config_map::iterator end() { return cmap.end(); }
};

// ---------------------------------------
//      class AWT_config
// ---------------------------------------

AWT_config::AWT_config(const char *config_char_ptr)
    : mapping(new AWT_config_mapping)
    , parse_error(0)
{
    // parse string in format "key1='value1';key2='value2'"..
    // and put values into a map.
    // assumes that keys are unique

    string      config_string(config_char_ptr);
    config_map& cmap  = mapping->cmap;
    size_t      pos   = 0;

    while (!parse_error) {
        size_t equal = config_string.find('=', pos);
        if (equal == string::npos) break;

        if (config_string[equal+1] != '\'') {
            parse_error = "expected quote \"'\"";
            break;
        }
        size_t start = equal+2;
        size_t end   = config_string.find('\'', start);
        while (end != string::npos) {
            if (config_string[end-1] != '\\') break;
            end = config_string.find('\'', end+1);
        }
        if (end == string::npos) {
            parse_error = "could not find matching quote \"'\"";
            break;
        }

        string config_name = config_string.substr(pos, equal-pos);
        string value       = config_string.substr(start, end-start);

        parse_error = decode_escapes(value);
        if (!parse_error) {
            cmap[config_name] = value;
        }

        pos = end+2;            // skip ';'
    }
}
AWT_config::AWT_config(const AWT_config_mapping *cfgname_2_awar, AW_root *root)
    : mapping(new AWT_config_mapping)
    , parse_error(0)
{
    const config_map& awarmap  = cfgname_2_awar->cmap;
    config_map& valuemap = mapping->cmap;

    for (config_map::const_iterator c = awarmap.begin(); c != awarmap.end(); ++c) {
        const string& key(c->first);
        const string& awar_name(c->second);
        
        char *awar_value = root->awar(awar_name.c_str())->read_as_string();
        valuemap[key]    = awar_value;
        free(awar_value);
    }

    awt_assert(valuemap.size() == awarmap.size());
}

AWT_config::~AWT_config() {
    delete mapping;
}

bool AWT_config::has_entry(const char *entry) const {
    awt_assert(!parse_error);
    return mapping->entry(entry) != mapping->end();
}
const char *AWT_config::get_entry(const char *entry) const {
    awt_assert(!parse_error);
    config_map::iterator found = mapping->entry(entry);
    return (found == mapping->end()) ? 0 : found->second.c_str();
}
void AWT_config::set_entry(const char *entry, const char *value) {
    awt_assert(!parse_error);
    mapping->cmap[entry] = value;
}
void AWT_config::delete_entry(const char *entry) {
    awt_assert(!parse_error);
    mapping->cmap.erase(entry);
}

char *AWT_config::config_string() const {
    awt_assert(!parse_error);
    string result;
    for (config_map::iterator e = mapping->begin(); e != mapping->end(); ++e) {
        const string& config_name(e->first);
        string        value(e->second);

        encode_escapes(value, "\'");
        string entry = config_name+"='"+value+'\'';
        if (result.empty()) {
            result = entry;
        }
        else {
            result = result+';'+entry;
        }
    }
    return strdup(result.c_str());
}
GB_ERROR AWT_config::write_to_awars(const AWT_config_mapping *cfgname_2_awar, AW_root *root) const {
    GB_ERROR        error = 0;
    GB_transaction *ta    = 0;
    awt_assert(!parse_error);
    for (config_map::iterator e = mapping->begin(); !error && e != mapping->end(); ++e) {
        const string& config_name(e->first);
        const string& value(e->second);

        config_map::const_iterator found = cfgname_2_awar->cmap.find(config_name);
        if (found == cfgname_2_awar->end()) {
            error = GBS_global_string("config contains unmapped entry '%s'", config_name.c_str());
        }
        else {
            const string&  awar_name(found->second);
            AW_awar       *awar = root->awar(awar_name.c_str());
            if (!ta) {
                ta = new GB_transaction((GBDATA*)awar->gb_var); // do all awar changes in 1 transaction
            }
            awar->write_as_string(value.c_str());
        }
    }
    if (ta) delete ta; // close transaction
    return error;
}

//  ------------------------------------
//      class AWT_config_definition
//  ------------------------------------

AWT_config_definition::AWT_config_definition(AW_root *aw_root)
    : root(aw_root) , config_mapping(new AWT_config_mapping) {}

AWT_config_definition::AWT_config_definition(AW_root *aw_root, AWT_config_mapping_def *mdef)
    : root(aw_root) , config_mapping(new AWT_config_mapping)
{
    add(mdef);
}

AWT_config_definition::~AWT_config_definition() {
    delete config_mapping;
}

void AWT_config_definition::add(const char *awar_name, const char *config_name) {
    config_mapping->cmap[config_name] = awar_name;
}
void AWT_config_definition::add(const char *awar_name, const char *config_name, int counter) {
    add(awar_name, GBS_global_string("%s%i", config_name, counter));
}
void AWT_config_definition::add(AWT_config_mapping_def *mdef) {
    while (mdef->awar_name && mdef->config_name) {
        add(mdef->awar_name, mdef->config_name);
        mdef++;
    }
}

char *AWT_config_definition::read() const {
    // creates a string from awar values
    
    AWT_config current_state(config_mapping, root);
    return current_state.config_string();
}
void AWT_config_definition::write(const char *config_char_ptr) const {
    // write values from string to awars
    // if the string contains unknown settings, they are silently ignored

    AWT_config wanted_state(config_char_ptr);
    GB_ERROR   error  = wanted_state.parseError();
    if (!error) error = wanted_state.write_to_awars(config_mapping, root);
    if (error) aw_message(GBS_global_string("Error restoring configuration (%s)", error));
}

