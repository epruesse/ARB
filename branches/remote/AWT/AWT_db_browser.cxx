//  ==================================================================== //
//                                                                       //
//    File      : AWT_db_browser.cxx                                     //
//    Purpose   : Simple database viewer                                 //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2004              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include "awt.hxx"
#include "awt_hexdump.hxx"

#include <aw_window.hxx>
#include <aw_msg.hxx>
#include <aw_awar.hxx>
#include <aw_select.hxx>
#include <aw_advice.hxx>

#include <arbdbt.h>

#include <arb_str.h>
#include <arb_strarray.h>

#include <arb_misc.h>
#include <arb_diff.h>
#include <arb_file.h>
#include <arb_sleep.h>
#include <ad_cb.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

// do includes above (otherwise depends depend on DEBUG)
#if defined(DEBUG)

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

#define AWAR_DBB_RECSIZE AWAR_DBB_BASE "/recsize"

#define AWAR_DUMP_HEX   AWAR_DBB_BASE "/hex"
#define AWAR_DUMP_ASCII AWAR_DBB_BASE "/ascii"
#define AWAR_DUMP_SPACE AWAR_DBB_BASE "/space"
#define AWAR_DUMP_WIDTH AWAR_DBB_BASE "/width"
#define AWAR_DUMP_BLOCK AWAR_DBB_BASE "/block"

#define HISTORY_PSEUDO_PATH "*history*"
#define ENTRY_MAX_LENGTH    1000
#define HISTORY_MAX_LENGTH  20000

inline bool is_dbbrowser_pseudo_path(const char *path) {
    return
        path           &&
        path[0] == '*' &&
        strcmp(path, HISTORY_PSEUDO_PATH) == 0;
}

enum SortOrder {
    SORT_NONE,
    SORT_NAME,
    SORT_CONTAINER,
    SORT_OCCUR,
    SORT_TYPE,
    SORT_CONTENT,

    SORT_COUNT
};

static const char *sort_order_name[SORT_COUNT] = {
    "Unsorted",
    "Name",
    "Name (Container)",
    "Name (Occur)",
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

    inline int cmp_by_container(const list_entry& other) const { return int(type != GB_DB) - int(other.type != GB_DB); }
    inline int cmp_by_childcount(const list_entry& other) const { return childCount - other.childCount; }

    inline bool less_than_by_container_name(const list_entry& other) const {
        int cmp = cmp_by_container(other);
        if (cmp == 0) return less_than_by_name(other);
        return cmp<0;
    }
    inline bool less_than_by_childcount_name(const list_entry& other) const {
        int cmp = cmp_by_childcount(other);
        if (cmp == 0) return less_than_by_name(other);
        return cmp<0;
    }

    bool operator<(const list_entry& other) const {
        bool is_less = false;
        switch (sort_order) {
            case SORT_COUNT: break;
            case SORT_NONE:
                awt_assert(0);  // not possible
                break;

            case SORT_NAME:
                is_less = less_than_by_name(other);
                break;

            case SORT_CONTAINER:
                is_less = less_than_by_container_name(other);
                break;

            case SORT_OCCUR:
                is_less = less_than_by_childcount_name(other);
                break;

            case SORT_CONTENT: {
                int cmp = ARB_stricmp(content.c_str(), other.content.c_str());

                if (cmp != 0) is_less = cmp<0;
                else is_less          = less_than_by_container_name(other);

                break;
            }
            case SORT_TYPE: {
                int cmp = type-other.type;

                if (cmp == 0) is_less = less_than_by_name(other);
                else is_less          = cmp<0;

                break;
            }
        }
        return is_less;
    }
};

SortOrder list_entry::sort_order = SORT_NONE;

// ---------------------
//      create AWARs

static MemDump make_userdefined_MemDump(AW_root *awr) {
    bool   hex      = awr->awar(AWAR_DUMP_HEX)->read_int();
    bool   ascii    = awr->awar(AWAR_DUMP_ASCII)->read_int();
    bool   space    = awr->awar(AWAR_DUMP_SPACE)->read_int();
    size_t width    = awr->awar(AWAR_DUMP_WIDTH)->read_int();
    size_t separate = awr->awar(AWAR_DUMP_BLOCK)->read_int();
    
    bool offset = (hex||ascii) && width;

    return MemDump(offset, hex, ascii, width, separate, space);
}

static void nodedisplay_changed_cb(AW_root *aw_root) {
    aw_root->awar(AWAR_DBB_BROWSE)->touch();
}

void AWT_create_db_browser_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int   (AWAR_DBB_DB,      0,                     aw_def); // index to internal order of announced databases
    aw_root->awar_int   (AWAR_DBB_ORDER,   SORT_CONTAINER,        aw_def); // sort order for "browse"-box
    aw_root->awar_string(AWAR_DBB_PATH,    "/",                   aw_def); // path in database
    aw_root->awar_string(AWAR_DBB_BROWSE,  "",                    aw_def); // selection in browser (= child name)
    aw_root->awar_string(AWAR_DBB_INFO,    "<select an element>", aw_def); // information about selected item
    aw_root->awar_string(AWAR_DBB_HISTORY, "",                    aw_def); // '\n'-separated string containing visited nodes

    aw_root->awar_int(AWAR_DBB_RECSIZE, 0,  aw_def)->add_callback(nodedisplay_changed_cb); // collect size recursive?

    // hex-dump-options
    aw_root->awar_int(AWAR_DUMP_HEX,   1,  aw_def)->add_callback(nodedisplay_changed_cb); // show hex ?
    aw_root->awar_int(AWAR_DUMP_ASCII, 1,  aw_def)->add_callback(nodedisplay_changed_cb); // show ascii ?
    aw_root->awar_int(AWAR_DUMP_SPACE, 1,  aw_def)->add_callback(nodedisplay_changed_cb); // space bytes
    aw_root->awar_int(AWAR_DUMP_WIDTH, 16, aw_def)->add_callback(nodedisplay_changed_cb); // bytes/line
    aw_root->awar_int(AWAR_DUMP_BLOCK, 8,  aw_def)->add_callback(nodedisplay_changed_cb); // separate each bytes
}

static GBDATA *GB_search_numbered(GBDATA *gbd, const char *str, GB_TYPES create) { // @@@  this may be moved to ARBDB-sources
    awt_assert(!GB_have_error());
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
                            char *parent_path = GB_strpartdup(str, previous_slash-1);

                            // we are sure parent path does not contain brackets -> search normal
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
                            const char *name_start = previous_slash ? previous_slash+1 : str;
                            char       *key_name   = GB_strpartdup(name_start, first_bracket-1);
                            int         c          = 0;

                            gb_son = GB_entry(gb_parent, key_name);
                            while (c<count && gb_son) {
                                gb_son = GB_nextEntry(gb_son);
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
    awt_assert(!GB_have_error());
    return GB_search(gbd, str, create); // do normal search
}

// -----------------------
//      class KnownDB

class KnownDB {
    GBDATA& gb_main; 
    string  description;
    string  current_path;

public:
    KnownDB(GBDATA *gb_main_, const char *description_)
        : gb_main(*gb_main_)
        , description(description_)
        , current_path("/")
    {}
    KnownDB(const KnownDB& other)
        : gb_main(other.gb_main),
          description(other.description),
          current_path(other.current_path)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(KnownDB);

    const GBDATA *get_db() const { return &gb_main; }
    const string& get_description() const { return description; }

    const string& get_path() const { return current_path; }
    void set_path(const string& path) { current_path = path; }
    void set_path(const char* path) { current_path = path; }
};

class hasDB {
    GBDATA *db;
public:
    hasDB(GBDATA *gbm) : db(gbm) {}
    bool operator()(const KnownDB& kdb) { return kdb.get_db() == db; }
};

// --------------------------
//      class DB_browser

class DB_browser;
static DB_browser *get_the_browser(bool autocreate);

class DB_browser {
    typedef vector<KnownDB>::iterator KnownDBiterator;

    vector<KnownDB> known_databases;
    size_t          current_db; // index of current db (in known_databases)

    AW_window             *aww;                     // browser window
    AW_option_menu_struct *oms;                     // the DB selector
    AW_selection_list     *browse_list;             // the browse subwindow

    static SmartPtr<DB_browser> the_browser;
    friend DB_browser *get_the_browser(bool autocreate);

    void update_DB_selector();

    DB_browser(const DB_browser& other);            // copying not allowed
    DB_browser& operator = (const DB_browser& other); // assignment not allowed
public:
    DB_browser() : current_db(0), aww(0), oms(0) {}

    void add_db(GBDATA *gb_main, const char *description) {
        known_databases.push_back(KnownDB(gb_main, description));
        if (aww) update_DB_selector();
    }

    void del_db(GBDATA *gb_main) {
        KnownDBiterator known = find_if(known_databases.begin(), known_databases.end(), hasDB(gb_main));
        if (known != known_databases.end()) known_databases.erase(known);
#if defined(DEBUG)
        else awt_assert(0); // no need to delete unknown databases
#endif // DEBUG
    }

    AW_window *get_window(AW_root *aw_root);
    AW_selection_list *get_browser_list() { return browse_list; }

    bool legal_selected_db() const { return current_db < known_databases.size(); }

    size_t get_selected_db() const { awt_assert(legal_selected_db()); return current_db; }
    void set_selected_db(size_t idx) { awt_assert(idx < known_databases.size()); current_db = idx; }

    const char *get_path() const { return known_databases[get_selected_db()].get_path().c_str(); }
    void set_path(const char *new_path) { known_databases[get_selected_db()].set_path(new_path); }

    GBDATA *get_db() const {
        return legal_selected_db() ? const_cast<GBDATA*>(known_databases[get_selected_db()].get_db()) : NULL;
    }
};


// -----------------------------
//      DB_browser singleton

SmartPtr<DB_browser> DB_browser::the_browser;

static DB_browser *get_the_browser(bool autocreate = true) {
    if (DB_browser::the_browser.isNull() && autocreate) {
        DB_browser::the_browser = new DB_browser;
    }
    return &*DB_browser::the_browser;
}

// --------------------------
//      announce databases

void AWT_announce_db_to_browser(GBDATA *gb_main, const char *description) {
    get_the_browser()->add_db(gb_main, description);
}

static void AWT_announce_properties_to_browser(GBDATA *gb_defaults, const char *defaults_name) {
    AWT_announce_db_to_browser(gb_defaults, GBS_global_string("Properties (%s)", defaults_name));
}

void AWT_browser_forget_db(GBDATA *gb_main) {
    if (gb_main) {
        DB_browser *browser = get_the_browser(false);
        awt_assert(browser);
        if (browser) browser->del_db(gb_main);
    }
}

// ---------------------------------------
//      browser window callbacks

static void toggle_tmp_cb(AW_window *aww) {
    AW_awar *awar_path = aww->get_root()->awar(AWAR_DBB_PATH);
    char    *path      = awar_path->read_string();
    bool     done      = false;

    if (ARB_strBeginsWith(path, "/tmp")) {
        if (path[4] == '/') {
            awar_path->write_string(path+4);
            done = true;
        }
        else if (path[4] == 0) {
            awar_path->write_string("/");
            done = true;
        }
    }

    if (!done && !is_dbbrowser_pseudo_path(path)) {
        char *path_in_tmp = GBS_global_string_copy("/tmp%s", path);

        char *lslash = strrchr(path_in_tmp, '/');
        if (lslash && !lslash[1]) { // ends with '/'
            lslash[0] = 0; // cut-off trailing '/'
        }
        awar_path->write_string(path_in_tmp);
        free(path_in_tmp);
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
    GBDATA  *gb_main   = get_the_browser()->get_db();

    {
        GB_transaction t(gb_main);
        while (len>0 && GB_search_numbered(gb_main, path, GB_FIND) == 0) {
            GB_clear_error();
            path[--len] = 0;
        }
    }

    awar_path->write_string(len ? path : "/");
    aw_root->awar(AWAR_DBB_BROWSE)->write_string("");
    return 0;
}

static BrowserCommand browser_command_table[] = {
    { BROWSE_CMD_GOTO_VALID_NODE, browse_cmd_goto_valid_node },
    { BROWSE_CMD_GO_UP, browse_cmd_go_up },
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

static AW_window *create_db_browser(AW_root *aw_root) {
    return get_the_browser()->get_window(aw_root);
}

struct counterPair {
    int occur, count;
    counterPair() : occur(0), count(0) {}
};

inline void insert_history_selection(AW_selection_list *sel, const char *entry, int wanted_db) {
    const char *colon = strchr(entry, ':');
    if (colon && (atoi(entry) == wanted_db)) {
        sel->insert(colon+1, colon+1);
    }
}


static char *get_container_description(GBDATA *gbd) {
    char *content = NULL;
    const char *known_children[] = { "@name", "name", "key_name", "alignment_name", "group_name", "key_text", 0 };
    for (int i = 0; known_children[i]; ++i) {
        GBDATA *gb_known = GB_entry(gbd, known_children[i]);
        if (gb_known && GB_read_type(gb_known) != GB_DB && GB_nextEntry(gb_known) == 0) { // exactly one child exits
            char *asStr = GB_read_as_string(gb_known);
            content     = GBS_global_string_copy("[%s=%s]", known_children[i], asStr);
            free(asStr);
            break;
        }
    }

    return content;
}

static char *get_dbentry_content(GBDATA *gbd, GB_TYPES type, bool shorten_repeats, const MemDump& dump) {
    awt_assert(type != GB_DB);

    char *content = NULL;
    if (!dump.wrapped()) content = GB_read_as_string(gbd); // @@@
    if (!content) { // use dumper
        long        size;
        if (type == GB_POINTER) {
            size = sizeof(GBDATA*);
        }
        else {
            size = GB_read_count(gbd);
        }
        const int plen = 30;
        GBS_strstruct buf(dump.mem_needed_for_dump(size)+plen);

        if (!dump.wrapped()) buf.nprintf(plen, "<%li bytes>: ", size);
        dump.dump_to(buf, GB_read_pntr(gbd), size);

        content         = buf.release();
        shorten_repeats = false;
    }
    size_t len = shorten_repeats ? GBS_shorten_repeated_data(content) : strlen(content);
    if (!dump.wrapped() && len>(ENTRY_MAX_LENGTH+15)) {
        content[ENTRY_MAX_LENGTH] = 0;
        freeset(content, GBS_global_string_copy("%s [rest skipped]", content));
    }
    return content;
}

static void update_browser_selection_list(AW_root *aw_root, AW_selection_list *id) {
    awt_assert(!GB_have_error());
    DB_browser *browser = get_the_browser();
    char       *path    = aw_root->awar(AWAR_DBB_PATH)->read_string();
    bool        is_root;
    GBDATA     *node    = NULL;

    {
        GBDATA *gb_main = browser->get_db();
        if (gb_main) {
            GB_transaction ta(gb_main);
            is_root = (strcmp(path, "/") == 0);
            node    = GB_search_numbered(gb_main, path, GB_FIND);
        }
    }

    id->clear();

    if (node == 0) {
        if (strcmp(path, HISTORY_PSEUDO_PATH) == 0) {
            GB_clear_error(); // ignore error about invalid key
            char *history = aw_root->awar(AWAR_DBB_HISTORY)->read_string();
            id->insert("Previously visited nodes:", "");
            char *start   = history;
            int   curr_db = aw_root->awar(AWAR_DBB_DB)->read_int();

            for (char *lf = strchr(start, '\n'); lf; start = lf+1, lf = strchr(start, '\n')) {
                lf[0] = 0;
                insert_history_selection(id, start, curr_db);
            }
            insert_history_selection(id, start, curr_db);
            free(history);
        }
        else {
            if (GB_have_error()) id->insert(GBS_global_string("Error: %s", GB_await_error()), "");
            id->insert("No such node!", "");
            id->insert("-> goto valid node", BROWSE_CMD_GOTO_VALID_NODE);
        }
    }
    else {
        map<string, counterPair> child_count;
        GB_transaction           ta(browser->get_db());

        for (GBDATA *child = GB_child(node); child; child = GB_nextChild(child)) {
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
                GBDATA     *child     = GB_entry(node, i->first.c_str()); // find first child
                const char *type_name = GB_get_type_name(child);
                int         typelen   = strlen(type_name);

                if (typelen>maxtypelen) maxtypelen = typelen;
            }
        }

        if (!is_root) {
            id->insert(GBS_global_string("%-*s   parent container", maxkeylen, ".."), BROWSE_CMD_GO_UP);
            id->insert(GBS_global_string("%-*s   container", maxkeylen, "."), "");
        }
        else {
            id->insert(GBS_global_string("%-*s   root container", maxkeylen, "/"), "");
        }
        
        // collect children and sort them

        vector<list_entry> sorted_children;

        MemDump simpleDump(false, true, false);
        for (GBDATA *child = GB_child(node); child; child = GB_nextChild(child)) {
            list_entry entry;
            entry.key_name   = GB_read_key_pntr(child);
            entry.childCount = -1;
            entry.type       = GB_read_type(child);
            entry.gbd        = child;

            int occurrences = child_count[entry.key_name].occur;
            if (occurrences != 1) {
                entry.childCount = child_count[entry.key_name].count;
                child_count[entry.key_name].count++;
            }
            char *display = NULL;
            if (entry.type == GB_DB) {
                display = get_container_description(entry.gbd);
            }
            else {
                display = get_dbentry_content(entry.gbd, entry.type, true, simpleDump);
            }
            if (display) entry.content = display;
            sorted_children.push_back(entry);
        }

        list_entry::sort_order = (SortOrder)aw_root->awar(AWAR_DBB_ORDER)->read_int();
        if (list_entry::sort_order != SORT_NONE) {
            sort(sorted_children.begin(), sorted_children.end());
        }

        for (vector<list_entry>::iterator ch = sorted_children.begin(); ch != sorted_children.end(); ++ch) {
            const list_entry&  entry            = *ch;
            const char        *key_name         = entry.key_name;
            char              *numbered_keyname = 0;

            if (entry.childCount >= 0) {
                numbered_keyname = GBS_global_string_copy("%s[%i]", key_name, entry.childCount);
            }

            key_name = numbered_keyname ? numbered_keyname : key_name;
            const char *displayed = GBS_global_string("%-*s   %-*s   %s", maxkeylen, key_name, maxtypelen, GB_get_type_name(entry.gbd), entry.content.c_str());
            id->insert(displayed, key_name);

            free(numbered_keyname);
        }
    }
    id->insert_default("", "");
    id->update();

    free(path);
    awt_assert(!GB_have_error());
}

static void order_changed_cb(AW_root *aw_root) {
    DB_browser *browser = get_the_browser();
    update_browser_selection_list(aw_root, browser->get_browser_list());
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

static bool    inside_path_change = false;
static GBDATA *gb_tracked_node    = NULL;

static void selected_node_modified_cb(GBDATA *gb_node, GB_CB_TYPE cb_type) {
    awt_assert(gb_node == gb_tracked_node);

    if (cb_type & GB_CB_DELETE) {
        gb_tracked_node = NULL; // no need to remove callback ?
    }

    if (!inside_path_change) { // ignore refresh callbacks triggered by dbbrowser-awars
        static bool avoid_recursion = false;
        if (!avoid_recursion) {
            LocallyModify<bool> flag(avoid_recursion, true);
            GlobalStringBuffers *old_buffers = GBS_store_global_buffers();

            AW_root *aw_root   = AW_root::SINGLETON;
            AW_awar *awar_path = aw_root->awar_no_error(AWAR_DBB_PATH);
            if (awar_path) { // avoid crash during exit
                AW_awar    *awar_child = aw_root->awar(AWAR_DBB_BROWSE);
                const char *child      = awar_child->read_char_pntr();

                if (child[0]) {
                    const char *path     = awar_path->read_char_pntr();
                    const char *new_path;

                    if (!path[0] || !path[1]) {
                        new_path = GBS_global_string("/%s", child);
                    }
                    else {
                        new_path = GBS_global_string("%s/%s", path, child);
                    }

                    char *fixed_path = GBS_string_eval(new_path, "//=/", NULL);
                    awar_path->write_string(fixed_path);
                    free(fixed_path);
                }
                else {
                    awar_path->touch();
                }
            }
            GBS_restore_global_buffers(old_buffers);
        }
    }
}
static void untrack_node() {
    if (gb_tracked_node) {
        GB_remove_callback(gb_tracked_node, GB_CB_ALL, makeDatabaseCallback(selected_node_modified_cb));
        gb_tracked_node = NULL;
    }
}
static void track_node(GBDATA *gb_node) {
    untrack_node();
    GB_ERROR error = GB_add_callback(gb_node, GB_CB_ALL, makeDatabaseCallback(selected_node_modified_cb));
    if (error) {
        aw_message(error);
    }
    else {
        gb_tracked_node = gb_node;
    }
}

static void child_changed_cb(AW_root *aw_root) {
    char *child = aw_root->awar(AWAR_DBB_BROWSE)->read_string();
    if (strncmp(child, BROWSE_CMD_PREFIX, sizeof(BROWSE_CMD_PREFIX)-1) == 0) { // a symbolic browser command
        execute_browser_command(get_the_browser()->get_window(aw_root), child);
    }
    else {
        char *path = aw_root->awar(AWAR_DBB_PATH)->read_string();

        if (strcmp(path, HISTORY_PSEUDO_PATH) == 0) {
            if (child[0]) {
                LocallyModify<bool> flag(inside_path_change, true);
                aw_root->awar(AWAR_DBB_PATH)->write_string(child);
            }
        }
        else {
            static bool avoid_recursion = false;
            if (!avoid_recursion) {
                LocallyModify<bool> flag(avoid_recursion, true);

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

                DB_browser *browser = get_the_browser();
                GBDATA     *gb_main = browser->get_db();

                if (gb_main) {
                    GB_transaction  ta(gb_main);
                    GBDATA         *gb_selected_node = GB_search_numbered(gb_main, fullpath, GB_FIND);

                    string info;
                    info += GBS_global_string("Fullpath  | '%s'\n", fullpath);

                    if (gb_selected_node == 0) {
                        info += "Address   | NULL\n";
                        info += GBS_global_string("Error     | %s\n", GB_have_error() ? GB_await_error() : "<none>");
                    }
                    else {
                        add_to_history(aw_root, fullpath);

                        info += GBS_global_string("Address   | %p\n", gb_selected_node);
                        info += GBS_global_string("Key index | %i\n", GB_get_quark(gb_selected_node));


                        GB_SizeInfo sizeInfo;
                        bool        collect_recursive = aw_root->awar(AWAR_DBB_RECSIZE)->read_int();

                        GB_TYPES type = GB_read_type(gb_selected_node);
                        if (collect_recursive || type != GB_DB) {
                            sizeInfo.collect(gb_selected_node);
                        }

                        string structure_add;
                        string childs;

                        if (type == GB_DB) {
                            long struct_size = GB_calc_structure_size(gb_selected_node);
                            if (collect_recursive) {
                                structure_add = GBS_global_string(" (this: %s)", GBS_readable_size(struct_size, "b"));
                            }
                            else {
                                sizeInfo.structure = struct_size;
                            }

                            info += "Node type | container\n";

                            childs = GBS_global_string("Childs    | %li", GB_number_of_subentries(gb_selected_node));
                            if (collect_recursive) {
                                childs = childs+" (rec: " + GBS_readable_size(sizeInfo.containers, "containers");
                                childs = childs+" + " + GBS_readable_size(sizeInfo.terminals, "terminals");
                                childs = childs+" = " + GBS_readable_size(sizeInfo.terminals+sizeInfo.containers, "nodes")+')';
                            }
                            childs += '\n';

                            {
                                LocallyModify<bool> flag2(inside_path_change, true);
                                aw_root->awar(AWAR_DBB_BROWSE)->write_string("");
                                aw_root->awar(AWAR_DBB_PATH)->write_string(fullpath);
                            }
                        }
                        else {
                            info += GBS_global_string("Node type | data [type=%s]\n", GB_get_type_name(gb_selected_node));
                        }

                        long overall = sizeInfo.mem+sizeInfo.structure;

                        info += GBS_global_string("Data size | %7s\n", GBS_readable_size(sizeInfo.data, "b"));
                        info += GBS_global_string("Memory    | %7s  %5.2f%%\n", GBS_readable_size(sizeInfo.mem, "b"), double(sizeInfo.mem)/overall*100+.0049);
                        info += GBS_global_string("Structure | %7s  %5.2f%%", GBS_readable_size(sizeInfo.structure, "b"), double(sizeInfo.structure)/overall*100+.0049) + structure_add + '\n';
                        info += GBS_global_string("Overall   | %7s\n", GBS_readable_size(overall, "b"));
                        if (sizeInfo.data) {
                            info += GBS_global_string("CompRatio | %5.2f%% (mem-only: %5.2f%%)\n", double(overall)/sizeInfo.data*100+.0049, double(sizeInfo.mem)/sizeInfo.data*100+.0049);
                        }
                        info += childs;

                        {
                            bool is_tmp             = GB_is_temporary(gb_selected_node);
                            bool not_tmp_but_in_tmp = !is_tmp && GB_in_temporary_branch(gb_selected_node);

                            if (is_tmp || not_tmp_but_in_tmp) {
                                info += GBS_global_string("Temporary | yes%s\n", not_tmp_but_in_tmp ? " (in temporary branch)" : "");
                            }
                        }

                        info += GBS_global_string("Security  | read=%i write=%i delete=%i\n",
                                                  GB_read_security_read(gb_selected_node),
                                                  GB_read_security_write(gb_selected_node),
                                                  GB_read_security_delete(gb_selected_node));

                        char *callback_info = GB_get_callback_info(gb_selected_node);
                        if (callback_info) {
                            ConstStrArray callbacks;
                            GBT_splitNdestroy_string(callbacks, callback_info, "\n", true);

                            for (size_t i = 0; i<callbacks.size(); ++i) {
                                const char *prefix = i ? "         " : "Callbacks";
                                info               = info + prefix + " | " + callbacks[i] + '\n';
                            }

                            if (gb_selected_node == gb_tracked_node) {
                                info += "          | (Note: one callback was installed by this browser)\n";
                            }
                        }

                        if (type != GB_DB) {
                            MemDump  dump    = make_userdefined_MemDump(aw_root);
                            char    *content = get_dbentry_content(gb_selected_node, GB_read_type(gb_selected_node), false, dump);
                            info             = info+"\nContent:\n"+content+'\n';
                            free(content);
                        }
                    }

                    aw_root->awar(AWAR_DBB_INFO)->write_string(info.c_str());
                }
                free(fullpath);
            }
        }

        free(path);
    }
    free(child);
}

static void path_changed_cb(AW_root *aw_root) {
    static bool avoid_recursion = false;
    if (!avoid_recursion) {
        awt_assert(!GB_have_error());
        LocallyModify<bool> flag(avoid_recursion, true);

        DB_browser *browser    = get_the_browser();
        char       *goto_child = 0;

        GBDATA *gb_main = browser->get_db();
        if (gb_main) {
            GB_transaction  t(gb_main);
            AW_awar        *awar_path = aw_root->awar(AWAR_DBB_PATH);
            char           *path      = awar_path->read_string();
            GBDATA         *found     = GB_search_numbered(gb_main, path, GB_FIND);

            if (found && GB_read_type(found) != GB_DB) { // exists, but is not a container
                char *lslash = strrchr(path, '/');
                if (lslash) {
                    goto_child = strdup(lslash+1);
                    lslash[lslash == path] = 0; // truncate at last slash (but keep sole slash)
                    awar_path->write_string(path);
                }
            }

            if (found) {
                LocallyModify<bool> flag2(inside_path_change, true);
                add_to_history(aw_root, goto_child ? GBS_global_string("%s/%s", path, goto_child) : path);
                GBDATA *father = GB_get_father(found);
                track_node(father ? father : found);
            }
            else if (is_dbbrowser_pseudo_path(path)) {
                GB_clear_error(); // ignore error about invalid key
            }
            else if (GB_have_error()) {
                aw_message(GB_await_error());
            }
            browser->set_path(path);
            free(path);
        }

        update_browser_selection_list(aw_root, browser->get_browser_list());

        LocallyModify<bool> flag2(inside_path_change, true);
        aw_root->awar(AWAR_DBB_BROWSE)->rewrite_string(goto_child ? goto_child : "");
        awt_assert(!GB_have_error());
    }
}
static void db_changed_cb(AW_root *aw_root) {
    int         selected = aw_root->awar(AWAR_DBB_DB)->read_int();
    DB_browser *browser  = get_the_browser();

    LocallyModify<bool> flag(inside_path_change, true);
    browser->set_selected_db(selected);
    aw_root->awar(AWAR_DBB_PATH)->rewrite_string(browser->get_path());
}

void DB_browser::update_DB_selector() {
    if (!oms) oms = aww->create_option_menu(AWAR_DBB_DB, true);
    else aww->clear_option_menu(oms);

    int idx = 0;
    for (KnownDBiterator i = known_databases.begin(); i != known_databases.end(); ++i, ++idx) {
        const KnownDB& db = *i;
        aww->insert_option(db.get_description().c_str(), "", idx);
    }
    aww->update_option_menu();
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
            set_selected_db(wanted_db);

            char *wanted_path = aw_root->awar(AWAR_DBB_PATH)->read_string();
            known_databases[wanted_db].set_path(wanted_path);
            free(wanted_path);
        }

        AW_window_simple *aws = new AW_window_simple;
        aww                   = aws;
        aws->init(aw_root, "DB_BROWSER", "ARB database browser");
        aws->load_xfig("dbbrowser.fig");

        aws->at("close"); aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("db");
        update_DB_selector();

        aws->at("order");
        aws->create_option_menu(AWAR_DBB_ORDER, true);
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

        aws->label("Rec.size"); aws->create_toggle(AWAR_DBB_RECSIZE);

        aws->at("browse");
        browse_list = aws->create_selection_list(AWAR_DBB_BROWSE, true);
        update_browser_selection_list(aw_root, browse_list);

        aws->at("infoopt");
        aws->label("ASCII"); aws->create_toggle     (AWAR_DUMP_ASCII);
        aws->label("Hex");   aws->create_toggle     (AWAR_DUMP_HEX);
        aws->label("Sep?");  aws->create_toggle     (AWAR_DUMP_SPACE);
        aws->label("Width"); aws->create_input_field(AWAR_DUMP_WIDTH, 3);
        aws->label("Block"); aws->create_input_field(AWAR_DUMP_BLOCK, 3);

        aws->at("info");
        aws->create_text_field(AWAR_DBB_INFO, 40, 40);

        // add callbacks
        aw_root->awar(AWAR_DBB_BROWSE)->add_callback(child_changed_cb);
        aw_root->awar(AWAR_DBB_PATH)->add_callback(path_changed_cb);
        aw_root->awar(AWAR_DBB_DB)->add_callback(db_changed_cb);
        aw_root->awar(AWAR_DBB_ORDER)->add_callback(order_changed_cb);

        db_changed_cb(aw_root); // force update
    }
    return aww;
}

static void callallcallbacks(AW_window *aww, int mode) {
    static bool running = false; // avoid deadlock
    if (!running) {
        LocallyModify<bool> flag(running, true);
        aww->get_root()->callallcallbacks(mode);
    }
}



void AWT_create_debug_menu(AW_window *awmm) {
    awmm->create_menu("4debugz", "z", AWM_ALL);

    awmm->insert_menu_topic(awmm->local_id("-db_browser"), "Browse loaded database(s)", "B", NULL, AWM_ALL, create_db_browser);

    awmm->sep______________();
    {
        awmm->insert_sub_menu("Callbacks (dangerous! use at your own risk)", "C", AWM_ALL);
        awmm->insert_menu_topic("!run_all_cbs_alph",  "Call all callbacks (alpha-order)",     "a", "", AWM_ALL, makeWindowCallback(callallcallbacks, 0));
        awmm->insert_menu_topic("!run_all_cbs_nalph", "Call all callbacks (alpha-reverse)",   "l", "", AWM_ALL, makeWindowCallback(callallcallbacks, 1));
        awmm->insert_menu_topic("!run_all_cbs_loc",   "Call all callbacks (code-order)",      "c", "", AWM_ALL, makeWindowCallback(callallcallbacks, 2));
        awmm->insert_menu_topic("!run_all_cbs_nloc",  "Call all callbacks (code-reverse)",    "o", "", AWM_ALL, makeWindowCallback(callallcallbacks, 3));
        awmm->insert_menu_topic("!run_all_cbs_rnd",   "Call all callbacks (random)",          "r", "", AWM_ALL, makeWindowCallback(callallcallbacks, 4));
        awmm->sep______________();
        awmm->insert_menu_topic("!forget_called_cbs", "Forget called",     "F", "", AWM_ALL, makeWindowCallback(callallcallbacks, -1));
        awmm->insert_menu_topic("!mark_all_called",   "Mark all called",   "M", "", AWM_ALL, makeWindowCallback(callallcallbacks, -2));
        awmm->sep______________();
        {
            awmm->insert_sub_menu("Call repeated", "p", AWM_ALL);
            awmm->insert_menu_topic("!run_all_cbs_alph_inf",  "Call all callbacks (alpha-order repeated)",     "a", "", AWM_ALL, makeWindowCallback(callallcallbacks, 8|0));
            awmm->insert_menu_topic("!run_all_cbs_nalph_inf", "Call all callbacks (alpha-reverse repeated)",   "l", "", AWM_ALL, makeWindowCallback(callallcallbacks, 8|1));
            awmm->insert_menu_topic("!run_all_cbs_loc_inf",   "Call all callbacks (code-order repeated)",      "c", "", AWM_ALL, makeWindowCallback(callallcallbacks, 8|2));
            awmm->insert_menu_topic("!run_all_cbs_nloc_inf",  "Call all callbacks (code-reverse repeated)",    "o", "", AWM_ALL, makeWindowCallback(callallcallbacks, 8|3));
            awmm->insert_menu_topic("!run_all_cbs_rnd_inf",   "Call all callbacks (random repeated)",          "r", "", AWM_ALL, makeWindowCallback(callallcallbacks, 8|4));
            awmm->close_sub_menu();
        }
        awmm->close_sub_menu();
    }
    awmm->sep______________();

}

#if 0
void AWT_check_action_ids(AW_root *, const char *) {
}
#else
void AWT_check_action_ids(AW_root *aw_root, const char *suffix) {
    // check actions ids (see #428)
    // suffix is appended to filenames (needed if one application may have different states)
    GB_ERROR error = NULL;
    {
        SmartPtr<ConstStrArray> action_ids = aw_root->get_action_ids();

        const char *checksdir = GB_path_in_ARBLIB("macros/.checks");
        char       *save      = GBS_global_string_copy("%s/%s%s_action.ids", checksdir, aw_root->program_name, suffix);

        FILE *out       = fopen(save, "wt");
        if (!out) error = GB_IO_error("writing", save);
        else {
            for (size_t i = 0; i<action_ids->size(); ++i) {
                fprintf(out, "%s\n", (*action_ids)[i]);
            }
            fclose(out);
        }

        if (!error) {
            char *expected         = GBS_global_string_copy("%s.expected", save);
            bool  asExpected       = ARB_textfiles_have_difflines(expected, save, 0, 0);
            if (!asExpected) error = GBS_global_string("action ids differ from expected (see console)");
            free(expected);
        }

        if (!error) GB_unlink(save);
        free(save);
    }
    if (error) fprintf(stderr, "AWT_check_action_ids: Error: %s\n", error);
}
#endif

#endif // DEBUG

AW_root *AWT_create_root(const char *properties, const char *program, UserActionTracker *user_tracker, int *argc, char*** argv) {
    AW_root *aw_root = new AW_root(properties, program, false, user_tracker, argc, argv);
#if defined(DEBUG)
    AWT_announce_properties_to_browser(AW_ROOT_DEFAULT, properties);
#endif // DEBUG
    init_Advisor(aw_root);
    return aw_root;
}

void AWT_trigger_remote_action(UNFIXED, GBDATA *gb_main, const char *remote_action_spec) {
    /*! trigger one or several action(s) (e.g. menu entries) in remote applications
     * @param gb_main             database root
     * @param remote_action_spec  "app:action[;app:action]*"
     */

    ConstStrArray appAction;
    GBT_split_string(appAction, remote_action_spec, ";", true);

    GB_ERROR error = NULL;
    if (appAction.empty()) {
        error = "No action found";
    }
    else {
        for (unsigned a = 0; a<appAction.size() && !error; ++a) {
            ConstStrArray cmd;
            GBT_split_string(cmd, appAction[a], ":", true);

            if (cmd.size() != 2) {
                error = GBS_global_string("Invalid action '%s'", appAction[a]);
            }
            else {
                const char *app    = cmd[0];
                const char *action = cmd[1];

                ARB_timeout after(2000, MS);
                error = GBT_remote_action_with_timeout(gb_main, app, action, &after);
            }
        }
    }

    aw_message_if(error);
}

// ------------------------
//      callback guards

static void before_callback_guard() {
    if (GB_have_error()) {
        GB_ERROR error = GB_await_error();
        aw_message(GBS_global_string("Error not clear before calling callback\n"
                                     "Unhandled error was:\n"
                                     "%s", error));
#if defined(DEVEL_RALF)
        awt_assert(0);
#endif // DEVEL_RALF
    }
}
static void after_callback_guard() {
    if (GB_have_error()) {
        GB_ERROR error = GB_await_error();
        aw_message(GBS_global_string("Error not handled by callback!\n"
                                     "Unhandled error was:\n"
                                     "'%s'", error));
#if defined(DEVEL_RALF)
        awt_assert(0);
#endif // DEVEL_RALF
    }
}

void AWT_install_cb_guards() {
    awt_assert(!GB_have_error());
    AW_cb::set_AW_cb_guards(before_callback_guard, after_callback_guard);
}
void AWT_install_postcb_cb(AW_postcb_cb postcb_cb) {
    AW_cb::set_AW_postcb_cb(postcb_cb);
}

