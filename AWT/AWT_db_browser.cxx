//  ==================================================================== // 
//                                                                       // 
//    File      : AWT_db_browser.cxx                                     // 
//    Purpose   : Simple database viewer                                 // 
//    Time-stamp: <Wed May/12/2004 21:27 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2004              // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

#include "awt.hxx"

#include <string>
#include <vector>
#include <map>
#include <ctype.h>

using namespace std;

// used AWARs : 

#define AWAR_DBB_BASE     "/dbbrowser"
#define AWAR_DBB_TMP_BASE "/tmp" AWAR_DBB_BASE

#define AWAR_DBB_DB     AWAR_DBB_BASE "/db"
#define AWAR_DBB_PATH   AWAR_DBB_BASE "/path"
#define AWAR_DBB_BROWSE AWAR_DBB_TMP_BASE "/browse"
#define AWAR_DBB_INFO   AWAR_DBB_TMP_BASE "/info"

// browser commands:

#define BROWSE_CMD                 "browse_cmd___"
#define BROWSE_CMD_GOTO_LEGAL_NODE BROWSE_CMD "goto_legal_node"
#define BROWSE_CMD_GO_UP           BROWSE_CMD "go_up"

// --------------------- 
//      create AWARs     
// --------------------- 

void AWT_create_db_browser_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_DBB_DB, 0, aw_def); // index to internal order of announced databases
    aw_root->awar_string(AWAR_DBB_PATH, "/", aw_def); // path in database
    aw_root->awar_string(AWAR_DBB_BROWSE, "", aw_def); // selection in browser (= child name)
    aw_root->awar_string(AWAR_DBB_INFO, "<select an element>", aw_def); // information about selected item
}

// @@@  this may go to ARBDB 
GBDATA *GB_search_numbered(GBDATA *gbd, const char *str, long /*enum gb_search_enum*/ create) {
    if (str) {
        const char *first_bracket = strchr(str, '[');
        if (first_bracket) {
            const char *second_bracket = strchr(first_bracket+1, ']');
            if (second_bracket) {
                int count = atoi(first_bracket+1);
                if (count >= 0 && isdigit(first_bracket[1])) {
                    // we are sure we have sth with number in brackets (e.g. "/species_data/species[42]/name")
                    const char *previous_slash = first_bracket-1;
                    while (previous_slash >= str && previous_slash[0] != '/') previous_slash--; //
                    if (previous_slash<str) previous_slash = 0;

                    GBDATA *gb_parent = 0;
                    {
                        if (previous_slash) { // found a slash
                            int   parent_path_len = previous_slash-str;
                            char *parent_path     = (char*)malloc(parent_path_len+1); memcpy(parent_path, str, parent_path_len); parent_path[parent_path_len] = 0;

                            // parent path may not contain brackets -> search normal
                            if (parent_path[0] == 0) { // that means : root-item is numbered (e.g. '/species_data[7]/...')
                                gb_parent = GB_get_root(gbd);
                            }
                            else {
                                gb_parent = GB_search(gbd, parent_path, GB_FIND);
                            }

                            if (!gb_parent) fprintf(stderr, "Warning: parent '%s' not found\n", parent_path);
                            free(parent_path);
                        }
                        else {
                            gb_parent = gbd; 
                        }
                    }

                    if (gb_parent) { 
                        GBDATA *gb_son = 0;
                        {
                            int   key_name_len = first_bracket-previous_slash-1;
                            char *key_name     = (char*)malloc(key_name_len+1); memcpy(key_name, previous_slash+1, key_name_len); key_name[key_name_len] = 0;
                            int   c            = 0;

                            gb_son = GB_find(gb_parent, key_name, 0, down_level);
                            while (c<count && gb_son) {
                                gb_son = GB_find(gb_son, key_name, 0, this_level|search_next);
                                if (gb_son) ++c;
                            }

                            if (!gb_son) fprintf(stderr, "Warning: did not find %i. son '%s'\n", count, key_name);
                            free(key_name);
                        }

                        if (gb_son) {
                            const char *rest = 0;
                            if (second_bracket[1] == '/') { // continue search ?
                                if (second_bracket[2]) {
                                    rest = second_bracket+2;
                                }
                            }

                            return rest
                                ? GB_search_numbered(gb_son, rest, create) // recursive search
                                : gb_son; // numbering occurred at end of search path
                        }
                    }
                    else {
                        fprintf(stderr, "Warning: don't know where to start numbered search in '%s'\n", str);
                    }

                    return 0; // not found
                }
                else {
                    fprintf(stderr, "Warning: Illegal content in search path - expected digits at '%s'\n", first_bracket+1);
                }
            }
            else {
                fprintf(stderr, "Warning: Unbalanced or illegal [] in search path (%s)\n", str);
            }
        }
        // no brackets -> normal search
    }
    return GB_search(gbd, str, create); // do normal search
}

// -----------------------
//      class KnownDB
// -----------------------
class KnownDB {
    GBDATA *gb_main;
    string  description;
    string  current_path;

public:
    KnownDB(GBDATA *gb_main_, const char *description_)
        : gb_main(gb_main_)
        , description(description_)
        , current_path("/")
    { }

    const GBDATA *get_db() const { return gb_main; }
    const string& get_description() const { return description; }

    const string& get_path() const { return current_path; }
    void set_path(const string& path) { current_path = path; }
    void set_path(const char* path) { current_path = path; }
};

// --------------------------
//      class DB_browser
// --------------------------
class DB_browser {
    typedef vector<KnownDB>::iterator KnownDBiterator;

    vector<KnownDB> known_databases;
    size_t          current_db; // index of current db (in known_databases)

    AW_window         *aww;     // browser window
    AW_selection_list *browse_id; // the browse subwindow

    DB_browser(const DB_browser& other); // copying not allowed
    DB_browser& operator = (const DB_browser& other); // assignment not allowed
public:
    DB_browser() : current_db(0), aww(0) {}

    void add_db(GBDATA *gb_main, const char *description) {
        awt_assert(!aww);       // now it's too late to announce!
        known_databases.push_back(KnownDB(gb_main, description));
    }

    AW_window *get_window(AW_root *aw_root);
    AW_selection_list *get_browser_id() { return browse_id; }

    size_t get_selected_db() const { return current_db; }
    void set_selected_db(size_t idx) {
        awt_assert(idx < known_databases.size());
        current_db = idx;
    }

    const char *get_path() const { return known_databases[current_db].get_path().c_str(); }
    void set_path(const char *new_path) { known_databases[current_db].set_path(new_path); }

    GBDATA *gb_main() const { return const_cast<GBDATA*>(known_databases[current_db].get_db()); }
};


// -----------------------------
//      DB_browser singleton
// -----------------------------
static DB_browser *get_the_browser() {
    static DB_browser *the_browser = 0;
    if (!the_browser) the_browser = new DB_browser;
    return the_browser;
}

// --------------------------
//      announce databases
// --------------------------

void AWT_announce_db_to_browser(GBDATA *gb_main, const char *description) {
    get_the_browser()->add_db(gb_main, description);
}

// ---------------------------------------
//      the browser window + callbacks
// ---------------------------------------

AW_window *AWT_create_db_browser(AW_root *aw_root) {
    return get_the_browser()->get_window(aw_root);
}

static void goto_root_cb(AW_window *aww) {
    AW_awar *awar_path = aww->get_root()->awar(AWAR_DBB_PATH);
    awar_path->write_string("/");
}
static void go_up_cb(AW_window *aww) {
    AW_awar *awar_path = aww->get_root()->awar(AWAR_DBB_PATH);
    char    *path      = awar_path->read_string();
    char    *lslash    = strrchr(path, '/');

    if (lslash) {
        lslash[0] = 0;
        if (!path[0]) strcpy(path, "/");
        awar_path->write_string(path);
    }
}
static void goto_next_cb(AW_window *aww) {
#warning implement me
}

static void path_changed_cb(AW_root *aw_root); // prototype

static void browsed_node_changed_cb(GBDATA *, int *cl_aw_root, GB_CB_TYPE) {
    path_changed_cb((AW_root*)cl_aw_root);
}

static void set_callback_node(GBDATA *node, AW_root *aw_root) {
    static GBDATA *active_node = 0;

    if (active_node) GB_remove_callback(active_node, GB_CB_CHANGED, browsed_node_changed_cb, (int*)aw_root);
    if (node) GB_add_callback(node, GB_CB_CHANGED, browsed_node_changed_cb, (int*)aw_root);
    active_node = node;
}

struct counterPair {
    int occur, count;
    counterPair() : occur(0), count(0) {}
};

static void update_browser_selection_list(AW_root *aw_root, AW_CL cl_aww, AW_CL cl_id) {
    AW_window         *aww     = (AW_window*)cl_aww;
    AW_selection_list *id      = (AW_selection_list*)cl_id;
    DB_browser        *browser = get_the_browser();
    char              *path    = aw_root->awar(AWAR_DBB_PATH)->read_string();
    GBDATA            *gb_main = browser->gb_main();
    GB_transaction     dummy(gb_main);
    GBDATA            *node    = (strcmp(path, "/") == 0) ? GB_get_root(gb_main) : GB_search_numbered(gb_main, path, GB_FIND);

    set_callback_node(node, aw_root);

    aww->clear_selection_list(id);

    if (node == 0) {
        aww->insert_default_selection(id, "No such node!", "");
        aww->insert_selection(id, "-> goto legal node", BROWSE_CMD_GOTO_LEGAL_NODE);
    }
    else {
        map<string, counterPair> child_count;

        for (GBDATA *child = GB_find(node, 0, 0, down_level);
             child;
             child = GB_find(child, 0, 0, this_level|search_next))
        {
            const char *key_name   = GB_read_key_pntr(child);
            child_count[key_name].occur++;
        }

        int maxkeylen = 0;
        int maxtypelen = 0;
        for (map<string, counterPair>::iterator i = child_count.begin(); i != child_count.end(); ++i) {
            {
                int keylen   = i->first.length();
                int maxcount = i->second.occur;

                if (maxcount != 1) {
                    keylen += 2;    // brackets
                    while (maxcount) { maxcount /= 10; keylen++; } // increase keylen for each digit
                }

                if (keylen>maxkeylen) maxkeylen = keylen;
            }
            {
                GBDATA     *child     = GB_find(node, i->first.c_str(), 0, down_level); // find first child
                const char *type_name = GB_get_type_name(child);
                int         typelen   = strlen(type_name);

                if (typelen>maxtypelen) maxtypelen = typelen;
            }
        }


        for (GBDATA *child = GB_find(node, 0, 0, down_level);
             child;
             child = GB_find(child, 0, 0, this_level|search_next))
        {
            const char *key_name         = GB_read_key_pntr(child);
            int         occurances       = child_count[key_name].occur;
            char       *numbered_keyname = 0;

            if (occurances != 1) {
                numbered_keyname = GBS_global_string_copy("%s[%i]", key_name, child_count[key_name].count);
                child_count[key_name].count++;
            }

            char *content = 0;
            if (GB_read_type(child) == GB_DB) {
                const char *known_childs[] = { "name", 0 };
                for (int i = 0; known_childs[i]; ++i) {
                    GBDATA *gb_known = GB_find(child, known_childs[i], 0, down_level);

                    if (gb_known && GB_read_type(gb_known) != GB_DB &&
                        GB_find(gb_known, known_childs[i], 0, this_level|search_next) == 0) // exactly one child exits
                    {
                        content = GBS_global_string_copy("[%s=%s]", known_childs[i], GB_read_as_string(gb_known));
                        break;
                    }
                }

                if (!content) content = strdup("[]");
            }
            else {
                content = GB_read_as_string(child);
            }

            key_name = numbered_keyname ? numbered_keyname : key_name;

            const char *displayed = GBS_global_string("%-*s   %-*s   %s",
                                                      maxkeylen, key_name,
                                                      maxtypelen, GB_get_type_name(child),
                                                      content);

            aww->insert_selection(id, displayed, key_name);

            free(content);
            free(numbered_keyname);
        }
        aww->insert_default_selection(id, "", "");
    }

    aww->update_selection_list(id);

    free(path);
}
static void child_changed_cb(AW_root *aw_root) {
    static bool avoid_recursion = false;

    if (!avoid_recursion) {
        avoid_recursion = true;

        char *path  = aw_root->awar(AWAR_DBB_PATH)->read_string();
        char *child = aw_root->awar(AWAR_DBB_BROWSE)->read_string();

        char *fullpath;
        if (strcmp(path, "/") == 0) {
            fullpath = GBS_global_string_copy("/%s", child);
        }
        else if (child[0] == 0) {
            fullpath = strdup(path);
        }
        else {
            fullpath = GBS_global_string_copy("%s/%s", path, child);
        }

        DB_browser     *browser          = get_the_browser();
        GBDATA         *gb_main          = browser->gb_main();
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_selected_node = GB_search_numbered(gb_main, fullpath, GB_FIND);

        string info;
        info += GBS_global_string("child='%s'\n", child);
        info += GBS_global_string("path='%s'\n", path);
        info += GBS_global_string("fullpath='%s'\n", fullpath);

        if (gb_selected_node == 0) {
            info += "Node does not exist.\n";
        }
        else {
            info += "Node has been found.\n";

            if (GB_read_type(gb_selected_node) == GB_DB) {
                info += "Node is a container.\n";

                aw_root->awar(AWAR_DBB_BROWSE)->write_string("");
                aw_root->awar(AWAR_DBB_PATH)->write_string(fullpath);
            }
        }

        aw_root->awar(AWAR_DBB_INFO)->write_string(info.c_str());

        free(fullpath);
        free(child);
        free(path);

        avoid_recursion = false;
    }
}
static void path_changed_cb(AW_root *aw_root) {
    DB_browser *browser = get_the_browser();
    {
        char *path = aw_root->awar(AWAR_DBB_PATH)->read_string();
        browser->set_path(path);
        free(path);
    }
    update_browser_selection_list(aw_root, (AW_CL)browser->get_window(aw_root), (AW_CL)browser->get_browser_id());
    child_changed_cb(aw_root);
}
static void db_changed_cb(AW_root *aw_root) {
    int selected = aw_root->awar(AWAR_DBB_DB)->read_int();
    get_the_browser()->set_selected_db(selected);
    path_changed_cb(aw_root);
}

AW_window *DB_browser::get_window(AW_root *aw_root) {
    awt_assert(!known_databases.empty()); // have no db to browse

    if (!aww) {
        // select current db+path
        {
            int wanted_db = aw_root->awar(AWAR_DBB_DB)->read_int();
            int known     = known_databases.size();
            if (wanted_db >= known) {
                wanted_db = 0;
                aw_root->awar(AWAR_DBB_DB)->write_int(wanted_db);
                aw_root->awar(AWAR_DBB_PATH)->write_string("/"); // reset
            }
            current_db = wanted_db;

            char *wanted_path = aw_root->awar(AWAR_DBB_PATH)->read_string();
            known_databases[wanted_db].set_path(wanted_path);
            free(wanted_path);
        }

        AW_window_simple *aws = new AW_window_simple;
        aww                   = aws;
        aws->init(aw_root, "DB_BROWSER", "ARB database browser");
        aws->load_xfig("dbbrowser.fig");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE","C");

        aws->callback( AW_POPUP_HELP, (AW_CL)"db_browser.hlp");
        aws->at("help");
        aws->create_button("HELP","HELP","H");

        aws->at("db");
        aws->create_option_menu(AWAR_DBB_DB);
        int idx = 0;
        for (KnownDBiterator i = known_databases.begin(); i != known_databases.end(); ++i, ++idx) {
            const KnownDB& db = *i;
            aws->insert_option(db.get_description().c_str(), "", idx);
        }
        aws->update_option_menu();

        aws->at("path");
        aws->create_input_field(AWAR_DBB_PATH, 10);

        aws->auto_space(10, 10);
        aws->button_length(8);

        aws->at("navigation");
        aws->callback(goto_next_cb); aws->create_button("goto_next", "Next");
        aws->callback(go_up_cb); aws->create_button("go_up", "Up");
        aws->callback(goto_root_cb); aws->create_button("goto_root", "Top");

        aws->at("browse");
        browse_id = aws->create_selection_list(AWAR_DBB_BROWSE);
        update_browser_selection_list(aw_root, (AW_CL)aws, (AW_CL)browse_id);

        aws->at("info");
        aws->create_text_field(AWAR_DBB_INFO, 40, 40);
        // update_info_selection_list(aw_root, (AW_CL)aws, (AW_CL)info_id);

        // add callbacks
        aw_root->awar(AWAR_DBB_BROWSE)->add_callback(child_changed_cb);
        aw_root->awar(AWAR_DBB_PATH)->add_callback(path_changed_cb);
        aw_root->awar(AWAR_DBB_DB)->add_callback(db_changed_cb);

        db_changed_cb(aw_root); // force update
    }
    return aww;
}

