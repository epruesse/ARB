//  ==================================================================== //
//                                                                       //
//    File      : AWT_config_manager.cxx                                 //
//    Purpose   :                                                        //
//    Time-stamp: <Thu Jun/03/2004 16:51 MET Coder@ReallySoft.de>        //
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
    printf("result: '%s'\n", existing_configs.c_str());
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


//  --------------------------------------------------
//      inline GB_ERROR decode_escapes(string& s)
//  --------------------------------------------------
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

//  ---------------------------------------------------------------------------------------
//      inline void encode_escapes(string& s, const char *to_escape, char escape_char)
//  ---------------------------------------------------------------------------------------
inline void encode_escapes(string& s, const char *to_escape, char escape_char) {
    string neu;
    neu.reserve(s.length()*2+1);

    for (string::iterator p = s.begin(); p != s.end(); ++p) {
        if (*p == escape_char || strchr(to_escape, *p) != 0) {
            neu = neu+escape_char+*p;
        }
        else if (*p == '\n') { neu = neu+escape_char+'n'; }
        else if (*p == '\r') { neu = neu+escape_char+'r'; }
        else if (*p == '\t') { neu = neu+escape_char+'t'; }
        else { neu = neu+*p; }
    }
    s = neu;
}

//  ------------------------------------
//      class AWT_config_definition
//  ------------------------------------
class AWT_config_definition {
private:
    AW_root                     *root;
    typedef map<string, string>  AWT_configMapping;
    AWT_configMapping            config_mapping;

public:
    AWT_config_definition(AW_root *aw_root) : root(aw_root) {}
    virtual ~AWT_config_definition() {}

    void add(const string& awar_name, const string& config_name) { config_mapping[config_name] = awar_name; }

    string read() const {       // creates a string from awar values
        string result;
        for (AWT_configMapping::const_iterator cm = config_mapping.begin(); cm != config_mapping.end(); ++cm) {
            const string& config_name = cm->first;
            const string& awar_name   = cm->second;
            string        escaped_content;
            {
                char *content   = root->awar(awar_name.c_str())->read_as_string();
                escaped_content = content;
                free(content);
                encode_escapes(escaped_content, "'", '\\');
            }

            result = result + (result.length() ? ";" : "") + config_name+"='"+escaped_content+'\'';
        }
        return result;
    }

    void write(const string& s) const { // write values from string to awars
        size_t pos = 0;
        bool   err = false;

        while (1) {
            err = false;

            size_t equal = s.find('=', pos);
            if (equal == string::npos) break;

            err = true;

            if (s[equal+1] != '\'') break;
            size_t start = equal+2;
            size_t end   = s.find('\'', start);
            while (end != string::npos) {
                if (s[end-1] != '\\') break;
                end = s.find('\'', end+1);
            }
            if (end == string::npos) break;

            string                            config_name = s.substr(pos, equal-pos);
            AWT_configMapping::const_iterator found       = config_mapping.find(config_name);

            if (found != config_mapping.end()) {
                string   value     = s.substr(start, end-start);
                GB_ERROR err       = decode_escapes(value);
                string   awar_name = found->second;

                if (!err) root->awar(awar_name.c_str())->write_as_string(value.c_str());
            }
            pos = end+2;        // skip ';'
        }

        if (err) aw_message("Error in configuration (delete or save again)");
    }
};


static AWT_config_definition *config_def = 0;

//  ----------------------------------------------------------
//      void AWT_reset_configDefinition(AW_root *aw_root)
//  ----------------------------------------------------------
void AWT_reset_configDefinition(AW_root *aw_root) {
    delete config_def;
    config_def = new AWT_config_definition(aw_root);
}

//  ---------------------------------------------------------------------------------------------------
//      void AWT_add_configDefinition(const char *awar_name, const char *config_name, int counter)
//  ---------------------------------------------------------------------------------------------------
void AWT_add_configDefinition(const char *awar_name, const char *config_name, int counter) {
    awt_assert(config_def);
    if (counter == -1) {
        config_def->add(awar_name, config_name);
    }
    else {
        config_def->add(awar_name, GBS_global_string("%s%i", config_name, counter));
    }
}

//  -------------------------------------------
//      char *AWT_store_configDefinition()
//  -------------------------------------------
char *AWT_store_configDefinition() {
    awt_assert(config_def);
    return GB_strdup(config_def->read().c_str());
}
//  ---------------------------------------------------------
//      void AWT_restore_configDefinition(const char *s)
//  ---------------------------------------------------------
void AWT_restore_configDefinition(const char *s) {
    awt_assert(config_def);
    config_def->write(s);
}
