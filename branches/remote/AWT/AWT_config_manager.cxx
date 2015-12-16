//  ==================================================================== //
//                                                                       //
//    File      : AWT_config_manager.cxx                                 //
//    Purpose   :                                                        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "awt_config_manager.hxx"
#include "awt.hxx"
#include <arbdb.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>
#include <arb_strarray.h>

#include <map>
#include <string>
#include "awt_sel_boxes.hxx"
#include <arb_defs.h>
#include <arb_str.h>

using namespace std;

// --------------------------
//      AWT_configuration

enum ConfigAwar {
    VISIBLE_COMMENT,
    STORED_COMMENTS,
    EXISTING_CFGS,
    CURRENT_CFG,

    // 'Edit' subwindow
    SELECTED_FIELD,
    FIELD_CONTENT,

    CONFIG_AWARS, // must be last
};

bool is_prefined(const string& cfgname) { return cfgname[0] == '*'; }

class ConfigDefinition : virtual Noncopyable {
    AW_default default_file;
    string     id;

    AW_awar *std_awar[CONFIG_AWARS];

public:
    ConfigDefinition(AW_default default_file_, const char *id_)
        : default_file(default_file_),
          id(id_)
    {
        std_awar[VISIBLE_COMMENT] = get_awar("comment", true);
        std_awar[STORED_COMMENTS] = get_awar("comments");
        std_awar[EXISTING_CFGS]   = get_awar("existing");
        std_awar[CURRENT_CFG]     = get_awar("current");
        std_awar[SELECTED_FIELD]  = get_awar("field",   true);
        std_awar[FIELD_CONTENT]   = get_awar("content", true);
    }

    bool operator<(const ConfigDefinition& other) const { return id<other.id; }

    AW_default get_db() const { return default_file; }
    const char *get_id() const { return id.c_str(); }

    string get_awar_name(const string& subname, bool temp = false) const {
        return string("tmp/general_configs/"+(temp ? 0 : 4))+id+'/'+subname;
    }
    string get_config_dbpath(const string& cfgname) const {
        return get_awar_name(string("cfg_")+cfgname);
    }

    AW_awar *get_awar(const string& subname, bool temp = false) const {
        string   awar_name = get_awar_name(subname, temp);
        return AW_root::SINGLETON->awar_string(awar_name.c_str(), "", default_file);
    }
    AW_awar *get_awar(ConfigAwar a) const { return std_awar[a]; }

    string get_awar_value(ConfigAwar a) const { return get_awar(a)->read_char_pntr(); }
    void set_awar_value(ConfigAwar a, const string& new_value) const { get_awar(a)->write_string(new_value.c_str()); }
};

class AWT_configuration : public ConfigDefinition { // derived from Noncopyable
    StoreConfigCallback   store;
    RestoreConfigCallback load_or_reset;

    const AWT_predefined_config *predefined;

    AW_window         *aw_edit;
    AW_selection_list *field_selection;

    GB_ERROR update_config(const string& cfgname, const AWT_config& config);

public:
    AWT_configuration(AW_default default_file_, const char *id_, const StoreConfigCallback& store_, const RestoreConfigCallback& load_or_reset_, const AWT_predefined_config *predef)
        : ConfigDefinition(default_file_, id_),
          store(store_),
          load_or_reset(load_or_reset_),
          predefined(predef),
          aw_edit(NULL),
          field_selection(NULL)
    {
    }

    string get_config(const string& cfgname) {
        if (is_prefined(cfgname)) {
            const AWT_predefined_config *predef = find_predefined(cfgname);
            return predef ? predef->config : "";
        }
        else {
            GB_transaction  ta(get_db());
            GBDATA         *gb_cfg = GB_search(get_db(), get_config_dbpath(cfgname).c_str(), GB_FIND);
            return gb_cfg ? GB_read_char_pntr(gb_cfg) : "";
        }
    }
    GB_ERROR set_config(const string& cfgname, const string& new_value) {
        GB_ERROR error;
        if (is_prefined(cfgname)) {
            error = "cannot modify predefined config";
        }
        else {
            GB_transaction  ta(get_db());
            GBDATA         *gb_cfg = GB_search(get_db(), get_config_dbpath(cfgname).c_str(), GB_STRING);
            if (!gb_cfg) {
                error = GB_await_error();
            }
            else {
                error = GB_write_string(gb_cfg, new_value.c_str());
                get_awar(CURRENT_CFG)->touch(); // force refresh of config editor
            }
        }
        return error;
    }

    char *Store() const { return store(); }
    GB_ERROR Restore(const string& s) const {
        GB_ERROR error = 0;

        if (s.empty()) error = "empty/nonexistant config";
        else load_or_reset(s.c_str());

        return error;
    }
    void Reset() const {
        load_or_reset(NULL);
    }

    GB_ERROR Save(const char* filename, const string& cfg_name, const string& comment); // AWAR content -> FILE
    GB_ERROR Load(const char* filename, const string& cfg_name, string& found_comment); // FILE -> AWAR content

    bool has_existing(string lookFor) {
        string S(";");
        string existing = S+ get_awar_value(EXISTING_CFGS) +S;
        string wanted   = S+ lookFor +S;

        return existing.find(wanted) != string::npos;
    }

    void erase_deleted_configs();

    void add_predefined_to(ConstStrArray& cfgs) {
        if (predefined) {
            for (int i = 0; predefined[i].name; ++i) {
                awt_assert(predefined[i].name[0] == '*'); // names have to start with '*'
                cfgs.put(predefined[i].name);
            }
        }
    }
    const AWT_predefined_config *find_predefined(const string& cfgname) {
        awt_assert(is_prefined(cfgname));
        for (int i = 0; predefined[i].name; ++i) {
            if (cfgname == predefined[i].name) {
                return &predefined[i];
            }
        }
        return NULL;
    }

    void popup_edit_window(AW_window *aw_config);
    void update_field_selection_list();
    void update_field_content();
    void store_changed_field_content();
    void delete_selected_field();
    void keep_changed_fields();
};

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

#define HEADER    "ARB_CONFIGURATION"
#define HEADERLEN 17

GB_ERROR AWT_configuration::Save(const char* filename, const string& cfg_name, const string& comment) {
    awt_assert(strlen(HEADER) == HEADERLEN);

    printf("Saving config to '%s'..\n", filename);

    FILE     *out   = fopen(filename, "wt");
    GB_ERROR  error = 0;
    if (!out) {
        error = GB_IO_error("saving", filename);
    }
    else {
        if (comment.empty()) {
            fprintf(out, HEADER ":%s\n", get_id()); // =same as old format
        }
        else {
            string encoded_comment(comment);
            encode_escapes(encoded_comment, "");
            fprintf(out, HEADER ":%s;%s\n", get_id(), encoded_comment.c_str());
        }

        string content = get_config(cfg_name);
        fputs(content.c_str(), out);
        fclose(out);
    }
    return error;
}

GB_ERROR AWT_configuration::Load(const char* filename, const string& cfg_name, string& found_comment) {
    GB_ERROR error = 0;

    found_comment = "";
    printf("Loading config from '%s'..\n", filename);

    char *content = GB_read_file(filename);
    if (!content) {
        error = GB_await_error();
    }
    else {
        if (strncmp(content, HEADER ":", HEADERLEN+1) != 0) {
            error = "Unexpected content (" HEADER " missing)";
        }
        else {
            char *id_pos = content+HEADERLEN+1;
            char *nl     = strchr(id_pos, '\n');

            if (!nl) {
                error = "Unexpected content (no ID)";
            }
            else {
                *nl++ = 0;

                char *comment = strchr(id_pos, ';');
                if (comment) *comment++ = 0;

                if (strcmp(id_pos, get_id()) != 0) {
                    error = GBS_global_string("Wrong config type (expected=%s, found=%s)", get_id(), id_pos);
                }
                else {
                    error = set_config(cfg_name, nl);
                    if (comment && !error) {
                        found_comment = comment;
                        error         = decode_escapes(found_comment);
                    }
                }
            }
        }

        if (error) {
            error = GBS_global_string("Error: %s (while reading %s)", error, filename);
        }

        free(content);
    }

    return error;
}

void AWT_configuration::erase_deleted_configs() {
    string   cfg_base = get_awar_name("", false);
    GB_ERROR error    = NULL;
    {
        GB_transaction  ta(get_db());
        GBDATA         *gb_base = GB_search(get_db(), cfg_base.c_str(), GB_FIND);
        if (gb_base) {
            for (GBDATA *gb_cfg = GB_child(gb_base); gb_cfg && !error; ) {
                GBDATA     *gb_next = GB_nextChild(gb_cfg);
                const char *key     = GB_read_key_pntr(gb_cfg);
                if (key && ARB_strBeginsWith(key, "cfg_")) {
                    const char *name = key+4;
                    if (!has_existing(name)) {
                        error = GB_delete(gb_cfg);
                    }
                }
                gb_cfg = gb_next;
            }
        }
    }
    aw_message_if(error);
}

void remove_from_configs(const string& config, string& existing_configs) {
    ConstStrArray cfgs;
    GBT_split_string(cfgs, existing_configs.c_str(), ';');

    ConstStrArray remaining;
    for (int i = 0; cfgs[i]; ++i) {
        if (strcmp(cfgs[i], config.c_str()) != 0) {
            remaining.put(cfgs[i]);
        }
    }

    char *rest       = GBT_join_strings(remaining, ';');
    existing_configs = rest;
    free(rest);
}

#define NO_CONFIG_SELECTED "<no config selected>"

static void current_changed_cb(AW_root*, AWT_configuration *config) {
    AW_awar *awar_current = config->get_awar(CURRENT_CFG);
    AW_awar *awar_comment = config->get_awar(VISIBLE_COMMENT);

    // convert name to key (but allow empty string and strings starting with '*')
    string name;
    {
        const char *entered_name = awar_current->read_char_pntr();
        if (entered_name[0]) {
            bool  isPredefined = is_prefined(entered_name);
            char *asKey        = GBS_string_2_key(entered_name);
            name = isPredefined ? string(1, '*')+asKey : asKey;
            free(asKey);
        }
        else {
            name = "";
        }
    }

    awar_current->write_string(name.c_str());

    // refresh comment field
    if (name[0]) { // cfg not empty
        if (config->has_existing(name)) { // load comment of existing config
            string     storedComments = config->get_awar_value(STORED_COMMENTS);
            AWT_config comments(storedComments.c_str());

            const char *display;
            if (comments.parseError()) {
                display = GBS_global_string("Error reading config comments:\n%s", comments.parseError());
            }
            else {
                const char *saved_comment = comments.get_entry(name.c_str());
                display                   = saved_comment ? saved_comment : "";
            }
            awar_comment->write_string(display);
        }
        else if (is_prefined(name)) {
            const AWT_predefined_config *found = config->find_predefined(name);
            awar_comment->write_string(found ? found->description : NO_CONFIG_SELECTED);
        }
        else { // new config (not stored yet)
            // click <new> and enter name -> clear comment
            // click existing and change name -> reuse existing comment
            if (strcmp(awar_comment->read_char_pntr(), NO_CONFIG_SELECTED) == 0) {
                awar_comment->write_string("");
            }
        }
    }
    else { // no config selected
        awar_comment->write_string(NO_CONFIG_SELECTED);
    }

    // refresh field selection list + content field
    config->update_field_selection_list();
}

inline void save_comments(const AWT_config& comments, AWT_configuration *config) {
    char *comments_string = comments.config_string();
    config->set_awar_value(STORED_COMMENTS, comments_string);
    free(comments_string);
}

static void comment_changed_cb(AW_root*, AWT_configuration *config) {
    string curr_cfg = config->get_awar_value(CURRENT_CFG);
    if (!curr_cfg.empty()) {
        string changed_comment = config->get_awar_value(VISIBLE_COMMENT);
        if (is_prefined(curr_cfg)) {
            const AWT_predefined_config *found = config->find_predefined(curr_cfg);
            if (found && changed_comment != found->description) {
                aw_message("The description of predefined configs is immutable");
                config->get_awar(CURRENT_CFG)->touch(); // reload comment
            }
        }
        else if (config->has_existing(curr_cfg)) {
            AWT_config comments(config->get_awar_value(STORED_COMMENTS).c_str());
            if (comments.parseError()) {
                aw_message(GBS_global_string("Failed to parse config-comments (%s)", comments.parseError()));
            }
            else {
                if (changed_comment.empty()) {
                    comments.delete_entry(curr_cfg.c_str());
                }
                else {
                    comments.set_entry(curr_cfg.c_str(), changed_comment.c_str());
                }
                save_comments(comments, config);
            }
        }
    }
}
static void erase_comment_cb(AW_window*, AW_awar *awar_comment) {
    awar_comment->write_string("");
}

static void restore_cb(AW_window *, AWT_configuration *config) {
    string cfgName = config->get_awar_value(CURRENT_CFG);
    GB_ERROR error;
    if (cfgName.empty()) {
        error = "Please select config to restore";
    }
    else {
        error = config->Restore(config->get_config(cfgName));
    }
    aw_message_if(error);
}
static void store_cb(AW_window *, AWT_configuration *config) {
    string cfgName = config->get_awar_value(CURRENT_CFG);
    if (cfgName.empty()) aw_message("Please select or enter name of config to store");
    else if (is_prefined(cfgName)) aw_message("You can't modify predefined configs");
    else {
        string existing = config->get_awar_value(EXISTING_CFGS);

        AW_awar *awar_comment = config->get_awar(VISIBLE_COMMENT);
        string visibleComment(awar_comment->read_char_pntr());

        remove_from_configs(cfgName, existing); // remove selected from existing configs

        existing = existing.empty() ? cfgName : (string(cfgName)+';'+existing);
        {
            char     *config_string = config->Store();
            GB_ERROR  error         = config->set_config(cfgName, config_string);
            aw_message_if(error);
            free(config_string);
        }
        config->set_awar_value(EXISTING_CFGS, existing);
        awar_comment->rewrite_string(visibleComment.c_str()); // force new config to use last visible comment

        config->get_awar(CURRENT_CFG)->touch(); // force refresh of config editor
    }
}
static void delete_cb(AW_window *, AWT_configuration *config) {
    string cfgName = config->get_awar_value(CURRENT_CFG);
    if (is_prefined(cfgName)) {
        aw_message("You may not delete predefined configs");
    }
    else {
        string existing = config->get_awar_value(EXISTING_CFGS);
        remove_from_configs(cfgName, existing); // remove selected from existing configs
        config->set_awar_value(CURRENT_CFG, "");
        config->set_awar_value(EXISTING_CFGS, existing);

        // erase existing comment:
        AWT_config comments(config->get_awar_value(STORED_COMMENTS).c_str());
        comments.delete_entry(cfgName.c_str());
        save_comments(comments, config);

        config->erase_deleted_configs();
    }
}
static void load_cb(AW_window *, AWT_configuration *config) {
    string   cfgName = config->get_awar_value(CURRENT_CFG);
    GB_ERROR error   = NULL;

    if (cfgName.empty()) error = "Please enter or select target config";
    else if (is_prefined(cfgName)) error = "You may not load over a predefined config";
    else {
        char *loadMask = GBS_global_string_copy("%s_*", config->get_id());
        char *filename = aw_file_selection("Load config from file", "$(ARBCONFIG)", loadMask, ".arbcfg");
        if (filename) {
            string comment;

            error = config->Load(filename, cfgName, comment);
            if (!error) {
                // after successful load restore and store config
                restore_cb(NULL, config);
                store_cb(NULL, config);
                config->set_awar_value(VISIBLE_COMMENT, comment);
            }
            free(filename);
        }
        free(loadMask);
    }
    aw_message_if(error);
}
static void save_cb(AW_window *, AWT_configuration *config) {
    string   cfgName = config->get_awar_value(CURRENT_CFG);
    GB_ERROR error   = NULL;

    if (cfgName.empty()) error = "Please select config to save";
    else {
        char *saveAs = GBS_global_string_copy("%s_%s",
                                              config->get_id(),
                                              cfgName.c_str() + (cfgName[0] == '*')); // skip leading '*'

        char *filename = aw_file_selection("Save config to file", "$(ARBCONFIG)", saveAs, ".arbcfg");
        if (filename && filename[0]) {
            restore_cb(NULL, config);
            string comment = config->get_awar_value(VISIBLE_COMMENT);
            error          = config->Save(filename, cfgName, comment);
            free(filename);
        }
        free(saveAs);
    }
    aw_message_if(error);
}

#if defined(DEBUG)

static string esc(const string& str) {
    // escape C string
    char   *escaped = GBS_string_eval(str.c_str(), "\\n=\\\\n:\\t=\\\\t:\"=\\\\\"", NULL);
    string  result(escaped);
    free(escaped);
    return result;
}

static void dump_cb(AW_window *aww, AWT_configuration *config) {
    // dump code ready to insert into AWT_predefined_config
    string   cfgName = config->get_awar_value(CURRENT_CFG);
    GB_ERROR error   = NULL;

    if (cfgName.empty()) error = "Please select config to dump";
    else {
        string comment = esc(config->get_awar_value(VISIBLE_COMMENT));
        string confStr = esc(config->get_config(cfgName));

        cfgName         = esc(cfgName);
        const char *cfg = cfgName.c_str();

        fprintf(stderr, "    { \"*%s\", \"%s\", \"%s\" },\n",
                cfg[0] == '*' ? cfg+1 : cfg,
                comment.c_str(),
                confStr.c_str());
    }
    aw_message_if(error);
}
#endif


void AWT_configuration::update_field_selection_list() {
    if (field_selection) {
        string  cfgName      = get_awar_value(CURRENT_CFG);
        char   *selected     = get_awar(SELECTED_FIELD)->read_string();
        bool    seenSelected = false;

        field_selection->clear();
        if (!cfgName.empty() && has_existing(cfgName)) {
            string     configString = get_config(cfgName);
            AWT_config stored(configString.c_str());
            ConstStrArray entries;
            stored.get_entries(entries);

            StrArray entries_with_content;
            size_t   maxlen = 0;
            for (size_t e = 0; e<entries.size(); ++e) {
                maxlen = std::max(maxlen, strlen(entries[e]));
                if (strcmp(selected, entries[e]) == 0) seenSelected = true;
            }
            for (size_t e = 0; e<entries.size(); ++e) {
                field_selection->insert(GBS_global_string("%-*s  |  %s",
                                                          int(maxlen), entries[e],
                                                          stored.get_entry(entries[e])),
                                        entries[e]);
            }
        }
        field_selection->insert_default("", "");
        field_selection->update();

        if (!seenSelected) {
            get_awar(SELECTED_FIELD)->write_string("");
        }
        else {
            get_awar(SELECTED_FIELD)->touch();
        }
        free(selected);
    }
}

void AWT_configuration::update_field_content() {
    string cfgName = get_awar_value(CURRENT_CFG);
    string content = "<select a field below>";
    if (!cfgName.empty() && has_existing(cfgName)) {
        string selected = get_awar_value(SELECTED_FIELD);
        if (!selected.empty()) {
            string     configString = get_config(cfgName);
            AWT_config stored(configString.c_str());

            if (stored.has_entry(selected.c_str())) {
                content = stored.get_entry(selected.c_str());
            }
            else {
                content = GBS_global_string("<field '%s' not stored in config>", selected.c_str());
            }
        }
    }
    set_awar_value(FIELD_CONTENT, content.c_str());
}

GB_ERROR AWT_configuration::update_config(const string& cfgname, const AWT_config& config) {
    char     *changedConfigString = config.config_string();
    GB_ERROR  error               = set_config(cfgname, changedConfigString);
    free(changedConfigString);
    return error;
}

void AWT_configuration::store_changed_field_content() {
    string cfgName = get_awar_value(CURRENT_CFG);
    if (!cfgName.empty() && has_existing(cfgName)) {
        string selected = get_awar_value(SELECTED_FIELD);
        if (!selected.empty()) {
            string     configString = get_config(cfgName);
            AWT_config stored(configString.c_str());
            if (stored.has_entry(selected.c_str())) {
                string stored_content  = stored.get_entry(selected.c_str());
                string changed_content = get_awar_value(FIELD_CONTENT);

                if (stored_content != changed_content) {
                    stored.set_entry(selected.c_str(), changed_content.c_str());
                    aw_message_if(update_config(cfgName, stored));
                }
            }
        }
    }
}

void AWT_configuration::delete_selected_field() {
    string cfgName = get_awar_value(CURRENT_CFG);
    if (!cfgName.empty() && has_existing(cfgName)) {
        string selected = get_awar_value(SELECTED_FIELD);
        if (!selected.empty()) {
            string     configString = get_config(cfgName);
            AWT_config stored(configString.c_str());
            if (stored.has_entry(selected.c_str())) {
                stored.delete_entry(selected.c_str());
                aw_message_if(update_config(cfgName, stored));
                field_selection->move_selection(1);
                update_field_selection_list();
            }
        }
    }
}

void AWT_configuration::keep_changed_fields() {
    string cfgName = get_awar_value(CURRENT_CFG);
    if (!cfgName.empty() && has_existing(cfgName)) {
        string     configString = get_config(cfgName);
        AWT_config stored(configString.c_str());

        char       *current_state = Store();
        AWT_config  current(current_state);

        ConstStrArray entries;
        stored.get_entries(entries);
        int           deleted = 0;

        for (size_t e = 0; e<entries.size(); ++e) {
            const char *entry          = entries[e];
            const char *stored_content = stored.get_entry(entry);

            if (current.has_entry(entry)) {
                const char *current_content = current.get_entry(entry);
                if (strcmp(stored_content, current_content) == 0) {
                    stored.delete_entry(entry);
                    deleted++;
                }
            }
            else {
                aw_message(GBS_global_string("Entry '%s' is not (or no longer) supported", entry));
            }
        }

        if (deleted) {
            aw_message_if(update_config(cfgName, stored));
            update_field_selection_list();
        }
        else {
            aw_message("All entries differ from current state");
        }

        free(current_state);
    }
}

static void keep_changed_fields_cb(AW_window*, AWT_configuration *config) { config->keep_changed_fields(); }
static void delete_field_cb(AW_window*, AWT_configuration *config) { config->delete_selected_field(); }
static void selected_field_changed_cb(AW_root*, AWT_configuration *config) { config->update_field_content(); }
static void field_content_changed_cb(AW_root*, AWT_configuration *config) { config->store_changed_field_content(); }

void AWT_configuration::popup_edit_window(AW_window *aw_config) {
    if (!aw_edit) {
        AW_root          *root = aw_config->get_root();
        AW_window_simple *aws  = new AW_window_simple;
        {
            char *wid = GBS_global_string_copy("%s_edit", aw_config->get_window_id());
            aws->init(root, wid, "Edit configuration entries");
            free(wid);
        }
        aws->load_xfig("awt/edit_config.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE");

        aws->at("help");
        aws->callback(makeHelpCallback("prop_configs_edit.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("content");
        aws->create_input_field(get_awar(FIELD_CONTENT)->awar_name);

        aws->at("name");
        aws->create_button(NULL, get_awar(CURRENT_CFG)->awar_name, 0, "+");

        aws->at("entries");
        field_selection = aws->create_selection_list(get_awar(SELECTED_FIELD)->awar_name, true);

        aws->auto_space(0, 3);
        aws->button_length(10);
        aws->at("button");

        int xpos = aws->get_at_xposition();
        int ypos = aws->get_at_yposition();

        aws->callback(makeWindowCallback(delete_field_cb, this));
        aws->create_button("DELETE", "Delete\nselected\nentry", "D");

        aws->at_newline();
        ypos = aws->get_at_yposition();
        aws->at("button");
        aws->at(xpos, ypos);

        aws->callback(makeWindowCallback(keep_changed_fields_cb, this));
        aws->create_button("KEEP_CHANGED", "Keep only\nentries\ndiffering\nfrom\ncurrent\nstate", "K");

        aw_edit = aws;

        // bind callbacks to awars
        get_awar(SELECTED_FIELD)->add_callback(makeRootCallback(selected_field_changed_cb, this));
        get_awar(FIELD_CONTENT)->add_callback(makeRootCallback(field_content_changed_cb, this));

        // fill selection list
        update_field_selection_list();
    }

    aw_edit->activate();
}

static void edit_cb(AW_window *aww, AWT_configuration *config) { config->popup_edit_window(aww); }
static void reset_cb(AW_window *, AWT_configuration *config) { config->Reset(); }

static void get_existing_configs(ConfigDefinition& configDef, ConstStrArray& cfgs) {
    string cfgs_str = configDef.get_awar_value(EXISTING_CFGS);
    GBT_split_string(cfgs, cfgs_str.c_str(), ';');
}

static void refresh_config_sellist_cb(AW_root*, AWT_configuration *config, AW_selection_list *sel) {
    ConstStrArray cfgs;
    get_existing_configs(*config, cfgs);

    config->add_predefined_to(cfgs);
    sel->init_from_array(cfgs, "<new>", "");
}

static AW_window *create_config_manager_window(AW_root *, AWT_configuration *config, AW_window *aww) {
    AW_window_simple *aws = new AW_window_simple;

    char *title = GBS_global_string_copy("Configurations for '%s'", aww->get_window_title());
    char *id    = GBS_global_string_copy("%s_config",               aww->get_window_id());

    aws->init(aww->get_root(), id, title);
    aws->load_xfig("awt/manage_config.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE");

    aws->at("help");
    aws->callback(makeHelpCallback("prop_configs.hlp"));
    aws->create_button("HELP", "HELP");

    // create awars
    AW_awar *awar_existing = config->get_awar(EXISTING_CFGS);
    AW_awar *awar_current  = config->get_awar(CURRENT_CFG);
    AW_awar *awar_comment  = config->get_awar(VISIBLE_COMMENT);

    aws->at("comment");
    aws->create_text_field(awar_comment->awar_name);

    aws->at("clr");
    aws->callback(makeWindowCallback(erase_comment_cb, awar_comment));
    aws->create_autosize_button("erase", "Erase", "E");

    awar_current->add_callback(makeRootCallback(current_changed_cb, config));
    awar_comment->add_callback(makeRootCallback(comment_changed_cb, config));

    AW_selection_list *sel = awt_create_selection_list_with_input_field(aws, awar_current->awar_name, "cfgs", "name");

    awar_existing->add_callback(makeRootCallback(refresh_config_sellist_cb, config, sel));
    awar_existing->touch(); // fills selection list
    awar_current->touch(); // initialized comment textbox

    aws->auto_space(0, 3);
    aws->button_length(10);
    aws->at("button");

    int xpos = aws->get_at_xposition();
    int ypos = aws->get_at_yposition();

    struct but {
        void (*cb)(AW_window*, AWT_configuration*);
        const char *id;
        const char *label;
        const char *mnemonic;
    } butDef[] = {
        { restore_cb, "RESTORE", "Restore",           "R" },
        { store_cb,   "STORE",   "Store",             "S" },
        { delete_cb,  "DELETE",  "Delete",            "D" },
        { load_cb,    "LOAD",    "Load",              "L" },
        { save_cb,    "SAVE",    "Save",              "v" },
        { reset_cb,   "RESET",   "Factory\ndefaults", "F" },
        { edit_cb,    "EDIT",    "Edit",              "E" },
#if defined(DEBUG)
        { dump_cb,    "DUMP",    "dump\npredef",  "U" },
#endif
    };
    const int buttons = ARRAY_ELEMS(butDef);
    for (int b = 0; b<buttons; ++b) {
        const but& B = butDef[b];

        if (b>0) {
            aws->at("button");
            aws->at(xpos, ypos);
        }

        aws->callback(makeWindowCallback(B.cb, config));
        aws->create_button(B.id, B.label, B.mnemonic);

        aws->at_newline();
        ypos = aws->get_at_yposition();
    }

    free(id);
    free(title);

    return aws;
}

static void destroy_AWT_configuration(AWT_configuration *c, AW_window*) { delete c; }

void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, const StoreConfigCallback& store_cb,
                               const RestoreConfigCallback& load_or_reset_cb, const char *macro_id, const AWT_predefined_config *predef)
{
    /*! inserts a config-button into aww
     * @param default_file_ db where configs will be stored
     * @param id unique id (has to be a key)
     * @param store_cb creates a string from current state
     * @param load_or_reset_cb restores state from string or resets factory defaults if string is NULL
     * @param macro_id custom macro id (normally NULL will do)
     * @param predef predefined configs
     */
    AWT_configuration * const config = new AWT_configuration(default_file_, id, store_cb, load_or_reset_cb, predef);

    aww->button_length(0); // -> autodetect size by size of graphic
    aww->callback(makeCreateWindowCallback(create_config_manager_window, destroy_AWT_configuration, config, aww));
    aww->create_button(macro_id ? macro_id : "SAVELOAD_CONFIG", "#conf_save.xpm");
}

static char *store_generated_config_cb(const ConfigSetupCallback *setup_cb) {
    AWT_config_definition cdef;
    (*setup_cb)(cdef);

    return cdef.read();
}
static void load_or_reset_generated_config_cb(const char *stored_string, const ConfigSetupCallback *setup_cb) {
    AWT_config_definition cdef;
    (*setup_cb)(cdef);

    if (stored_string) cdef.write(stored_string);
    else cdef.reset();
}
void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, ConfigSetupCallback setup_cb, const char *macro_id, const AWT_predefined_config *predef) {
    /*! inserts a config-button into aww
     * @param default_file_ db where configs will be stored
     * @param id unique id (has to be a key)
     * @param setup_cb populates an AWT_config_definition (cl is passed to setup_cb)
     * @param macro_id custom macro id (normally NULL will do)
     * @param predef predefined configs
     */

    ConfigSetupCallback * const setup_cb_copy = new ConfigSetupCallback(setup_cb); // not freed (bound to cb)
    AWT_insert_config_manager(aww, default_file_, id,
                              makeStoreConfigCallback(store_generated_config_cb, setup_cb_copy),
                              makeRestoreConfigCallback(load_or_reset_generated_config_cb, setup_cb_copy),
                              macro_id, predef);
}

static void generate_config_from_mapping_cb(AWT_config_definition& cdef, const AWT_config_mapping_def *mapping) {
    cdef.add(mapping);
}

void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, const AWT_config_mapping_def *mapping, const char *macro_id, const AWT_predefined_config *predef) {
    /*! inserts a config-button into aww
     * @param default_file_ db where configs will be stored
     * @param id unique id (has to be a key)
     * @param mapping hardcoded mapping between AWARS and config strings
     * @param macro_id custom macro id (normally NULL will do)
     * @param predef predefined configs
     */
    AWT_insert_config_manager(aww, default_file_, id, makeConfigSetupCallback(generate_config_from_mapping_cb, mapping), macro_id, predef);
}

typedef map<string, string> config_map;
struct AWT_config_mapping {
    config_map cmap;

    config_map::iterator entry(const string &e) { return cmap.find(e); }

    config_map::iterator begin() { return cmap.begin(); }
    config_map::const_iterator end() const { return cmap.end(); }
    config_map::iterator end() { return cmap.end(); }
    config_map::size_type size() const { return cmap.size(); }
};

// -------------------
//      AWT_config

AWT_config::AWT_config(const char *config_char_ptr)
    : mapping(new AWT_config_mapping),
      parse_error(0)
{
    // parse string in format "key1='value1';key2='value2'"..
    // and put values into a map.
    // assumes that keys are unique

    string      configString(config_char_ptr);
    config_map& cmap  = mapping->cmap;
    size_t      pos   = 0;

    while (!parse_error) {
        size_t equal = configString.find('=', pos);
        if (equal == string::npos) break;

        if (configString[equal+1] != '\'') {
            parse_error = "expected quote \"'\"";
            break;
        }
        size_t start = equal+2;
        size_t end   = configString.find('\'', start);
        while (end != string::npos) {
            if (configString[end-1] != '\\') break;
            end = configString.find('\'', end+1);
        }
        if (end == string::npos) {
            parse_error = "could not find matching quote \"'\"";
            break;
        }

        string config_name = configString.substr(pos, equal-pos);
        string value       = configString.substr(start, end-start);

        parse_error = decode_escapes(value);
        if (!parse_error) {
            cmap[config_name] = value;
        }

        pos = end+2;            // skip ';'
    }
}

inline void warn_unknown_awar(const string& awar_name) {
    aw_message(GBS_global_string("Warning: unknown awar referenced\n(%s)", awar_name.c_str()));
}

AWT_config::AWT_config(const AWT_config_mapping *cfgname_2_awar)
    : mapping(new AWT_config_mapping),
      parse_error(0)
{
    const config_map&  awarmap  = cfgname_2_awar->cmap;
    config_map&        valuemap = mapping->cmap;
    AW_root           *aw_root  = AW_root::SINGLETON;

    int skipped = 0;
    for (config_map::const_iterator c = awarmap.begin(); c != awarmap.end(); ++c) {
        const string& key(c->first);
        const string& awar_name(c->second);

        AW_awar *awar = aw_root->awar_no_error(awar_name.c_str());
        if (awar) {
            char *awar_value = awar->read_as_string();
            valuemap[key] = awar_value;
            free(awar_value);
        }
        else {
            valuemap.erase(key);
            warn_unknown_awar(awar_name);
            ++skipped;
        }
    }

    awt_assert((valuemap.size()+skipped) == awarmap.size());
    awt_assert(!parse_error);
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
void AWT_config::write_to_awars(const AWT_config_mapping *cfgname_2_awar, bool warn) const {
    awt_assert(!parse_error);
    AW_root *aw_root  = AW_root::SINGLETON;
    int      unmapped = 0;
    for (config_map::iterator e = mapping->begin(); e != mapping->end(); ++e) {
        const string& config_name(e->first);
        const string& value(e->second);

        config_map::const_iterator found = cfgname_2_awar->cmap.find(config_name);
        if (found == cfgname_2_awar->end()) {
            if (warn) aw_message(GBS_global_string("config contains unknown entry '%s'", config_name.c_str()));
            unmapped++;
        }
        else {
            const string&  awar_name(found->second);
            AW_awar       *awar = aw_root->awar(awar_name.c_str());
            awar->write_as_string(value.c_str());
        }
    }

    if (unmapped && warn) {
        int mapped = mapping->size()-unmapped;
        aw_message(GBS_global_string("Not all config entries were valid:\n"
                                     "(known/restored: %i, unknown/ignored: %i)\n"
                                     "Note: ok for configs shared between multiple windows",
                                     mapped, unmapped));
    }
}

void AWT_config::get_entries(ConstStrArray& to_array) {
    for (config_map::iterator e = mapping->begin(); e != mapping->end(); ++e) {
        const string& key(e->first);
        to_array.put(key.c_str());
    }
}

// ------------------------------
//      AWT_config_definition

AWT_config_definition::AWT_config_definition()
    : config_mapping(new AWT_config_mapping)
{}

AWT_config_definition::AWT_config_definition(AWT_config_mapping_def *mdef)
    : config_mapping(new AWT_config_mapping)
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
void AWT_config_definition::add(const AWT_config_mapping_def *mdef) {
    while (mdef->awar_name && mdef->config_name) {
        add(mdef->awar_name, mdef->config_name);
        mdef++;
    }
}

char *AWT_config_definition::read() const {
    // creates a string from awar values

    AWT_config current_state(config_mapping);
    return current_state.config_string();
}
void AWT_config_definition::write(const char *config_char_ptr) const {
    // write values from string to awars
    // if the string contains unknown settings, they are silently ignored

    awt_assert(config_char_ptr);

    AWT_config wanted_state(config_char_ptr);
    GB_ERROR   error = wanted_state.parseError();
    if (!error) {
        char *old_state = read();
        wanted_state.write_to_awars(config_mapping, true);
        if (strcmp(old_state, config_char_ptr) != 0) { // expect that anything gets changed?
            char *new_state = read();
            if (strcmp(new_state, config_char_ptr) != 0) {
                bool retry      = true;
                int  maxRetries = 10;
                while (retry && maxRetries--) {
                    printf("Note: repeating config restore (did not set all awars correct)\n");
                    wanted_state.write_to_awars(config_mapping, false);
                    char *new_state2 = read();
                    if (strcmp(new_state, new_state2) != 0) { // retrying had some effect -> repeat
                        reassign(new_state, new_state2);
                    }
                    else {
                        retry = false;
                        free(new_state2);
                    }
                }
                if (retry) {
                    error = "Unable to restore everything (might be caused by outdated, now invalid settings)";
                }
            }
            free(new_state);
        }
        free(old_state);
    }
    if (error) aw_message(GBS_global_string("Error restoring configuration (%s)", error));
}

void AWT_config_definition::reset() const {
    // reset all awars (stored in config) to factory defaults
    AW_root *aw_root = AW_root::SINGLETON;
    for (config_map::iterator e = config_mapping->begin(); e != config_mapping->end(); ++e) {
        const string&  awar_name(e->second);
        AW_awar       *awar = aw_root->awar_no_error(awar_name.c_str());
        if (awar) {
            awar->reset_to_default();
        }
        else {
            warn_unknown_awar(awar_name);
        }
    }
}

// --------------------------------------------------------------------------------

void AWT_modify_managed_configs(AW_default default_file, const char *id, ConfigModifyCallback mod_cb, AW_CL cl_user) {
    /*! allows to modify (parts of) all stored configs
     * @param default_file   has to be same as used in AWT_insert_config_manager()
     * @param id             ditto
     * @param mod_cb         called with each key/value pair of each stored config. result == NULL -> delete pair; result != NULL -> change or leave unchanged (result has to be a heapcopy!)
     * @param cl_user        forwarded to mod_cb
     */

    ConfigDefinition configDef(default_file, id);

    ConstStrArray cfgs;
    get_existing_configs(configDef, cfgs);

    for (size_t c = 0; c<cfgs.size(); ++c) {
        GB_transaction  ta(configDef.get_db());
        GBDATA         *gb_cfg = GB_search(configDef.get_db(), configDef.get_config_dbpath(cfgs[c]).c_str(), GB_FIND);
        GB_ERROR        error  = NULL;

        if (gb_cfg) {
            const char *content = GB_read_char_pntr(gb_cfg);

            AWT_config cmap(content);
            error = cmap.parseError();
            if (!error) {
                ConstStrArray entries;
                cmap.get_entries(entries);

                bool update = false;
                for (size_t e = 0; e<entries.size(); ++e) {
                    const char *old_content = cmap.get_entry(entries[e]);
                    char       *new_content = mod_cb(entries[e], old_content, cl_user);

                    if (!new_content) {
                        cmap.delete_entry(entries[e]);
                        update = true;
                    }
                    else if (strcmp(old_content, new_content) != 0) {
                        cmap.set_entry(entries[e], new_content);
                        update = true;
                    }
                    free(new_content);
                }

                if (update) {
                    char *cs = cmap.config_string();
                    error    = GB_write_string(gb_cfg, cs);
                    free(cs);
                }
            }
        }

        if (error) {
            error = GBS_global_string("%s (config='%s')", error, cfgs[c]);
            aw_message(error);
        }
    }
}
