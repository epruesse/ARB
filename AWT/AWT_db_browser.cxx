//  ==================================================================== // 
//                                                                       // 
//    File      : AWT_db_browser.cxx                                     // 
//    Purpose   : Simple database viewer                                 // 
//    Time-stamp: <Wed Sep/29/2004 17:57 MET Coder@ReallySoft.de>        // 
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
#include <algorithm>
#include <inline.h>
// #include <ctype.h>

using namespace std;

// used AWARs : 

#define AWAR_DBB_BASE     "/dbbrowser"
#define AWAR_DBB_TMP_BASE "/tmp" AWAR_DBB_BASE

#define AWAR_DBB_DB      AWAR_DBB_BASE "/db"
#define AWAR_DBB_ORDER   AWAR_DBB_BASE "/order"
#define AWAR_DBB_PATH    AWAR_DBB_BASE "/path"
#define AWAR_DBB_HISTORY AWAR_DBB_BASE "/history"

#define AWAR_DBB_BROWSE AWAR_DBB_TMP_BASE "/browse"
#define AWAR_DBB_INFO   AWAR_DBB_TMP_BASE "/info"

#define HISTORY_PSEUDO_PATH "*history*"
#define ENTRY_MAX_LENGTH    1000
#define HISTORY_MAX_LENGTH  20000

enum SortOrder {
    SORT_NONE,
    SORT_NAME,
    SORT_NAME_DB,
    SORT_TYPE,
    SORT_CONTENT,

    SORT_COUNT
};

const char *sort_order_name[SORT_COUNT] = {
    "None",
    "Name",
    "Name (DB)",
    "Type",
    "Content", 
};



// used to sort entries in list
struct list_entry {
    const char *key_name;
    GB_TYPES    type;
    int         childCount;     // -1 if only one child with key_name exists
    GBDATA     *gbd;
    string      content;

    static SortOrder sort_order;

    inline bool less_than_by_name(const list_entry& other) const {
        int cmp = ARB_stricmp(key_name, other.key_name);
        if (cmp != 0) return (cmp<0); // name differs!
        return childCount<other.childCount; // equal names -> compare child count
    }

    inline int cmp_by_container(const list_entry& other) const {
        return int(type != GB_DB) - int(other.type != GB_DB);
    }

    inline bool less_than_by_name_container(const list_entry& other) const {
        int cmp = cmp_by_container(other);
        if (cmp == 0) return less_than_by_name(other);
        return cmp<0;
    }

    bool operator<(const list_entry& other) const {
        bool is_less = false;
        switch (sort_order) {
            case SORT_NONE:
                awt_assert(0);  // not possible
                break;

            case SORT_NAME:
                is_less = less_than_by_name(other);
                break;

            case SORT_NAME_DB: 
                is_less = less_than_by_name_container(other);
                break;

            case SORT_CONTENT: {
                int cmp = ARB_stricmp(content.c_str(), other.content.c_str());

                if (cmp != 0) is_less = cmp<0;
                else is_less          = less_than_by_name_container(other);
                
                break;
            }
            case SORT_TYPE: {
                int cmp = type-other.type;
                
                if (cmp == 0) is_less = less_than_by_name(other);
                else is_less          = cmp<0;
                
                break;
            }
            default :
                awt_assert(0);  // illegal 'sort_order'
                break;
        }
        return is_less;
    }
};

SortOrder list_entry::sort_order = SORT_NONE;

// ---------------------
//      create AWARs
// ---------------------

void AWT_create_db_browser_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_DBB_DB, 0, aw_def); // index to internal order of announced databases
    aw_root->awar_int(AWAR_DBB_ORDER, SORT_NAME_DB, aw_def); // sort order for "browse"-box
    aw_root->awar_string(AWAR_DBB_PATH, "/", aw_def); // path in database
    aw_root->awar_string(AWAR_DBB_BROWSE, "", aw_def); // selection in browser (= child name)
    aw_root->awar_string(AWAR_DBB_INFO, "<select an element>", aw_def); // information about selected item
    aw_root->awar_string(AWAR_DBB_HISTORY, "", aw_def); // '\n'-separated string containing visited nodes
}

// @@@  this may be moved to ARBDB-sources
GBDATA *GB_search_numbered(GBDATA *gbd, const char *str, long /*enum gb_search_enum*/ create) {
    if (str) {
        if (str[0] == '/' && str[1] == 0) { // root
            return GB_get_root(gbd);
        }

        const char *first_bracket = strchr(str, '[');
        if (first_bracket) {
            const char *second_bracket = strchr(first_bracket+1, ']');
            if (second_bracket && (second_bracket[1] == 0 || second_bracket[1] == '/')) {
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
//      browser window callbacks
// ---------------------------------------

static void toggle_tmp_cb(AW_window *aww) {
    AW_awar *awar_path = aww->get_root()->awar(AWAR_DBB_PATH);
    char    *path      = awar_path->read_string();
    bool     done      = false;

    if (ARB_strscmp(path, "/tmp") == 0) {
        if (path[4] == '/') {
            awar_path->write_string(path+4);
            done = true;
        }
        else if (path[4] == 0) {
            awar_path->write_string("/");
            done = true;
        }
    }

    if (!done) {
        awar_path->write_string(GBS_global_string("/tmp%s", path));
    }
    free(path);
}

static void show_history_cb(AW_window *aww) {
    aww->get_root()->awar(AWAR_DBB_PATH)->write_string(HISTORY_PSEUDO_PATH);
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

// --------------------------
//      browser commands:
// --------------------------

#define BROWSE_CMD_PREFIX          "browse_cmd___"
#define BROWSE_CMD_GOTO_VALID_NODE BROWSE_CMD_PREFIX "goto_valid_node"
#define BROWSE_CMD_GO_UP           BROWSE_CMD_PREFIX "go_up"

struct BrowserCommand {
    const char *name;
    GB_ERROR (*function)(AW_window *);
};


static GB_ERROR browse_cmd_go_up(AW_window *aww) {
    go_up_cb(aww);
    return 0;
}
static GB_ERROR browse_cmd_goto_valid_node(AW_window *aww) {
    AW_root *aw_root   = aww->get_root();
    AW_awar *awar_path = aw_root->awar(AWAR_DBB_PATH);
    char    *path      = awar_path->read_string();
    int      len       = strlen(path);
    GBDATA  *gb_main   = get_the_browser()->gb_main();

    {
        GB_transaction t(gb_main);
        while (len>0 && GB_search_numbered(gb_main, path, GB_FIND) == 0) {
            path[--len] = 0;
        }
    }

    awar_path->write_string(len ? path : "/");
    aw_root->awar(AWAR_DBB_BROWSE)->write_string("");
    return 0;
}

static BrowserCommand browser_command_table[] = {
    { BROWSE_CMD_GOTO_VALID_NODE, browse_cmd_goto_valid_node},
    { BROWSE_CMD_GO_UP, browse_cmd_go_up},
    { 0, 0 }
};

static void execute_browser_command(AW_window *aww, const char *command) {
    int idx;
    for (idx = 0; browser_command_table[idx].name; ++idx) {
        if (strcmp(command, browser_command_table[idx].name) == 0) {
            GB_ERROR error = browser_command_table[idx].function(aww);
            if (error) aw_message(error);
            break;
        }
    }

    if (!browser_command_table[idx].name) {
        aw_message(GBS_global_string("Unknown browser command '%s'", command));
    }
}

// ----------------------------
//      the browser window
// ----------------------------

AW_window *AWT_create_db_browser(AW_root *aw_root) {
    return get_the_browser()->get_window(aw_root);
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

inline void insert_history_selection(AW_window *aww, AW_selection_list *sel, const char *entry, int wanted_db) {
    char *colon = strchr(entry, ':');
    if (colon && (atoi(entry) == wanted_db)) {
        aww->insert_selection(sel, colon+1, colon+1);
    }
}

static void update_browser_selection_list(AW_root *aw_root, AW_CL cl_aww, AW_CL cl_id) {
    AW_window         *aww     = (AW_window*)cl_aww;
    AW_selection_list *id      = (AW_selection_list*)cl_id;
    DB_browser        *browser = get_the_browser();
    char              *path    = aw_root->awar(AWAR_DBB_PATH)->read_string();
    bool               is_root;
    GBDATA            *node;

    {
        GBDATA         *gb_main = browser->gb_main();
        GB_transaction  ta(gb_main);

        is_root = (strcmp(path, "/") == 0);
        node    = GB_search_numbered(gb_main, path, GB_FIND);
        set_callback_node(node, aw_root);
    }

    aww->clear_selection_list(id);

    if (node == 0) {
        if (strcmp(path, HISTORY_PSEUDO_PATH) == 0) {
            char *history = aw_root->awar(AWAR_DBB_HISTORY)->read_string();
            aww->insert_selection(id, "Previously visited nodes:", "");
            char *start   = history;
            int   curr_db = aw_root->awar(AWAR_DBB_DB)->read_int();

            for (char *lf = strchr(start, '\n'); lf; start = lf+1, lf = strchr(start, '\n')) {
                lf[0] = 0;
                insert_history_selection(aww, id, start, curr_db);
            }
            insert_history_selection(aww, id, start, curr_db);
            free(history);
        }
        else {
            aww->insert_selection(id, "No such node!", "");
            aww->insert_selection(id, "-> goto valid node", BROWSE_CMD_GOTO_VALID_NODE);
        }
    }
    else {
        map<string, counterPair> child_count;
        GB_transaction           ta(browser->gb_main());

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

        if (!is_root) aww->insert_selection(id, GBS_global_string("%-*s   parent container", maxkeylen, ".."), BROWSE_CMD_GO_UP);

        // collect childs and sort them

        vector<list_entry> sorted_childs;

        for (GBDATA *child = GB_find(node, 0, 0, down_level);
             child;
             child = GB_find(child, 0, 0, this_level|search_next))
        {
            list_entry entry;
            entry.key_name   = GB_read_key_pntr(child);
            entry.childCount = -1;
            entry.type       = GB_read_type(child);
            entry.gbd        = child;

            int occurances = child_count[entry.key_name].occur;
            if (occurances != 1) {
                entry.childCount = child_count[entry.key_name].count;
                child_count[entry.key_name].count++;
            }

            char *content = 0;
            if (entry.type == GB_DB) {
                // the childs listed here are displayed behind the container entry
                const char *known_childs[] = { "@name", "name", "key_name", "alignment_name", "group_name", "key_text", 0 };
                for (int i = 0; known_childs[i]; ++i) {
                    GBDATA *gb_known = GB_find(entry.gbd, known_childs[i], 0, down_level);

                    if (gb_known && GB_read_type(gb_known) != GB_DB &&
                        GB_find(gb_known, known_childs[i], 0, this_level|search_next) == 0) // exactly one child exits
                    {
                        content = GBS_global_string_copy("[%s=%s]", known_childs[i], GB_read_as_string(gb_known));
                        break;
                    }
                }

                if (!content) content = strdup("");
            }
            else {
                content = GB_read_as_string(entry.gbd);
                if (!content) {
                    long size = GB_read_count(entry.gbd);
                    content = GBS_global_string_copy("<%li bytes binary data>", size);
                }
            }

            if (strlen(content)>(ENTRY_MAX_LENGTH+15)) {
                content[ENTRY_MAX_LENGTH] = 0;
                char *shortened_content   = GBS_global_string_copy("%s [rest skipped]", content);
                free(content);
                content                   = shortened_content;
            }

            entry.content = content;
            free(content);

            sorted_childs.push_back(entry);
        }

        list_entry::sort_order = (SortOrder)aw_root->awar(AWAR_DBB_ORDER)->read_int();
        if (list_entry::sort_order != SORT_NONE) {
            sort(sorted_childs.begin(), sorted_childs.end());
        }

        for (vector<list_entry>::iterator ch = sorted_childs.begin(); ch != sorted_childs.end(); ++ch) {
            const list_entry&  entry            = *ch;
            const char        *key_name         = entry.key_name;
            char              *numbered_keyname = 0;

            if (entry.childCount >= 0) {
                numbered_keyname = GBS_global_string_copy("%s[%i]", key_name, entry.childCount);
            }

            key_name = numbered_keyname ? numbered_keyname : key_name;
            const char *displayed = GBS_global_string("%-*s   %-*s   %s", maxkeylen, key_name, maxtypelen, GB_get_type_name(entry.gbd), entry.content.c_str());
            aww->insert_selection(id, displayed, key_name);

            free(numbered_keyname);
        }
    }
    aww->insert_default_selection(id, "", "");
    aww->update_selection_list(id);

    free(path);
}

static void order_changed_cb(AW_root *aw_root) {
    DB_browser *browser = get_the_browser();
    update_browser_selection_list(aw_root, (AW_CL)browser->get_window(aw_root), (AW_CL)browser->get_browser_id());
}

static void child_changed_cb(AW_root *aw_root) {
    static bool avoid_recursion = false;

    if (!avoid_recursion) {
        avoid_recursion = true;

        char *child = aw_root->awar(AWAR_DBB_BROWSE)->read_string();
        if (strncmp(child, BROWSE_CMD_PREFIX, sizeof(BROWSE_CMD_PREFIX)-1) == 0) {
            // a symbolic browser command
            execute_browser_command(get_the_browser()->get_window(aw_root), child);
        }
        else {
            char *path = aw_root->awar(AWAR_DBB_PATH)->read_string();

            char *fullpath;
            if (strcmp(path, "/") == 0) {
                fullpath = GBS_global_string_copy("/%s", child);
            }
            else if (strcmp(path, HISTORY_PSEUDO_PATH) == 0) {
                fullpath = strdup(child);
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
            // info += GBS_global_string("child='%s'\n", child);
            // info += GBS_global_string("path='%s'\n", path);
            info += GBS_global_string("fullpath='%s'\n", fullpath);

            if (gb_selected_node == 0) {
                info += "Node does not exist.\n";
            }
            else {
                info += GBS_global_string("Node exists [address=%p]\n", gb_selected_node);

                if (GB_read_type(gb_selected_node) == GB_DB) {
                    info += "Node type: container\n";

                    aw_root->awar(AWAR_DBB_BROWSE)->write_string("");
                    aw_root->awar(AWAR_DBB_PATH)->write_string(fullpath);
                }
                else {
                    info += GBS_global_string("Node type: data [type=%s]\n", GB_get_type_name(gb_selected_node));
                }

                info += GBS_global_string("Security: read=%i write=%i delete=%i\n",
                                          GB_read_security_read(gb_selected_node),
                                          GB_read_security_write(gb_selected_node),
                                          GB_read_security_delete(gb_selected_node));

                char *callback_info = GB_get_callback_info(gb_selected_node);
                if (callback_info) {
                    info = info+"Callbacks:\n"+callback_info+'\n';
                }
            }

            aw_root->awar(AWAR_DBB_INFO)->write_string(info.c_str());

            free(fullpath);
            free(path);
        }
        free(child);

        avoid_recursion = false;
    }
}

inline char *strmove(char *dest, char *source) {
    return (char*)memmove(dest, source, strlen(source)+1);
}

static void add_to_history(AW_root *aw_root, const char *path) {
    // adds 'path' to history

    if (strcmp(path, HISTORY_PSEUDO_PATH) != 0) {
        int      db           = aw_root->awar(AWAR_DBB_DB)->read_int();
        AW_awar *awar_history = aw_root->awar(AWAR_DBB_HISTORY);
        char    *old_history  = awar_history->read_string();
        char    *entry        = GBS_global_string_copy("%i:%s", db, path);
        int      entry_len    = strlen(entry);

        char *found = strstr(old_history, entry);
        while (found) {
            if (found == old_history || found[-1] == '\n') { // found start of an entry
                if (found[entry_len] == '\n') {
                    strmove(found, found+entry_len+1);
                    found = strstr(old_history, entry);
                }
                else if (found[entry_len] == 0) { // found as last entry
                    if (found == old_history) { // which is also first entry
                        old_history[0] = 0; // empty history
                    }
                    else {
                        strmove(found, found+entry_len+1);
                    }
                    found = strstr(old_history, entry);
                }
                else {
                    found = strstr(found+1, entry);
                }
            }
            else {
                found = strstr(found+1, entry);
            }
        }

        if (old_history[0]) {
            char *new_history = GBS_global_string_copy("%s\n%s", entry, old_history);

            while (strlen(old_history)>HISTORY_MAX_LENGTH) { // shorten history
                char *llf = strrchr(old_history, '\n');
                if (!llf) break;
                llf[0]    = 0;
            }

            awar_history->write_string(new_history);
            free(new_history);
        }
        else {
            awar_history->write_string(entry);
        }

        free(entry);
        free(old_history);
    }
}

static void path_changed_cb(AW_root *aw_root) {
    static bool avoid_recursion = false;
    if (!avoid_recursion) {
        avoid_recursion         = true;
        DB_browser *browser     = get_the_browser();
        char       *goto_child  = 0;

        {
            GBDATA         *gb_main   = browser->gb_main();
            GB_transaction  t(gb_main);
            AW_awar        *awar_path = aw_root->awar(AWAR_DBB_PATH);
            char           *path      = awar_path->read_string();
            GBDATA         *found     = GB_search_numbered(gb_main, path, GB_FIND);

            if (found && GB_read_type(found) != GB_DB) { // exists, but is not a container
                char *lslash = strrchr(path, '/');
                if (lslash) {
                    lslash[0]  = 0;
                    awar_path->write_string(path);
                    goto_child = strdup(lslash+1);
                }
            }

            add_to_history(aw_root, path);
            browser->set_path(path);
            free(path);
        }

        update_browser_selection_list(aw_root, (AW_CL)browser->get_window(aw_root), (AW_CL)browser->get_browser_id());
        aw_root->awar(AWAR_DBB_BROWSE)->write_string(goto_child ? goto_child : "");

        avoid_recursion = false;
    }
}
static void db_changed_cb(AW_root *aw_root) {
    int         selected = aw_root->awar(AWAR_DBB_DB)->read_int();
    DB_browser *browser  = get_the_browser();
    browser->set_selected_db(selected);
    aw_root->awar(AWAR_DBB_PATH)->rewrite_string(browser->get_path());
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
        {
            int idx = 0;
            for (KnownDBiterator i = known_databases.begin(); i != known_databases.end(); ++i, ++idx) {
                const KnownDB& db = *i;
                aws->insert_option(db.get_description().c_str(), "", idx);
            }
        }
        aws->update_option_menu();

        aws->at("order");
        aws->create_option_menu(AWAR_DBB_ORDER);
        for (int idx = 0; idx<SORT_COUNT; ++idx) {
            aws->insert_option(sort_order_name[idx], "", idx);
        }
        aws->update_option_menu();

        aws->at("path");
        aws->create_input_field(AWAR_DBB_PATH, 10);

        aws->auto_space(10, 10);
        aws->button_length(8);

        aws->at("navigation");
        aws->callback(go_up_cb); aws->create_button("go_up", "Up");
        aws->callback(goto_root_cb); aws->create_button("goto_root", "Top");
        aws->callback(show_history_cb); aws->create_button("history", "History");
        aws->callback(toggle_tmp_cb); aws->create_button("toggle_tmp", "/tmp");

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
        aw_root->awar(AWAR_DBB_ORDER)->add_callback(order_changed_cb);

        db_changed_cb(aw_root); // force update
    }
    return aww;
}

