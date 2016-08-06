// =============================================================== //
//                                                                 //
//   File      : AW_rename.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AW_rename.hxx"

#include <aw_awars.hxx>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>

#include <names_client.h>
#include <servercntrl.h>
#include <client.h>

#include <cctype>
#include <ctime>

static const char *get_addid(GBDATA *gb_main) {
    GB_transaction ta(gb_main);
    GBDATA *gb_addid = GB_search(gb_main, AWAR_NAMESERVER_ADDID, GB_FIND);
    return gb_addid ? GB_read_char_pntr(gb_addid) : 0;
}

static GB_ERROR set_addid(GBDATA *gb_main, const char *addid) {
    GB_ERROR error    = GB_push_transaction(gb_main);
    if (!error) error = GBT_write_string(gb_main, AWAR_NAMESERVER_ADDID, addid ? addid : "");
    return GB_end_transaction(gb_main, error);
}

const char *AW_get_nameserver_addid(GBDATA *gb_main) {
    // return the additional field used for nameserver connection
    const char *addid = get_addid(gb_main);

    aw_assert(addid); // the current DB has no entry AWAR_NAMESERVER_ADDID! (programmers error)
    if (!addid) addid = ""; // NDEBUG fallback

    return addid;
}

GB_ERROR AW_select_nameserver(GBDATA *gb_main, GBDATA *gb_other_main) {
    // if entry AWAR_NAMESERVER_ADDID isn't defined yet, try to detect a reasonable value
    // from arb_tcp.dat. Ask user if multiple servers are defined.
    //
    // if gb_other_main is defined try to use value from there.

    const char *addid   = get_addid(gb_main);
    GB_ERROR    error   = 0;

    if (!addid && gb_other_main && gb_other_main != gb_main) {
        // look whether main DB has a defined addid
        addid = get_addid(gb_other_main);
        set_addid(gb_main, addid);
    }

    if (!addid) {
        const char * const *nameservers = GBS_get_arb_tcp_entries("ARB_NAME_SERVER*");

        if (!nameservers) error = GB_await_error();
        else {
            int serverCount = 0;

            for (int c = 0; nameservers[c]; c++) serverCount++;

            if (serverCount == 0) {
                error = GBS_global_string("No nameserver defined.");
            }
            else {
                char **fieldNames = (char **)ARB_alloc(serverCount*sizeof(*fieldNames));
                for (int c = 0; c<serverCount; c++) {
                    const char *ipport = GBS_read_arb_tcp(nameservers[c]);
                    if (!ipport) {
                        error = GB_await_error();
                        fieldNames[c] = NULL;
                    }
                    else {
                        fieldNames[c] = nulldup(GBS_scan_arb_tcp_param(ipport, "-f"));      // may return 0

                        // parameter -f contains default value (e.g. '-fstart=1')
                        if (fieldNames[c]) {
                            char *equal = strchr(fieldNames[c], '=');
                            if (equal) equal[0] = 0;
                        }
                    }
                }

                if (!error) {
                    if (serverCount == 1) { // exactly 1 server defined -> don't ask
                        error = set_addid(gb_main, fieldNames[0]);
                    }
                    else { // let the user select which nameserver to use
                        aw_assert(serverCount>1);

                        int         len     = serverCount; // commas+0term
                        const char *nofield = "None (only 'acc')";

                        for (int c = 0; c<serverCount; c++) {
                            if (fieldNames[c]) len += strlen(fieldNames[c]);
                            else len += strlen(nofield);
                        }

                        char *buttons = (char*)ARB_alloc(len);
                        buttons[0]    = 0;
                        for (int c = 0; c<serverCount; c++) {
                            if (c) strcat(buttons, ",");
                            strcat(buttons, fieldNames[c] ? fieldNames[c] : nofield);
                        }

                        int answer = aw_question("nameserv_select",
                                                 "Select if and which additional DB field you want to use",
                                                 buttons, false, "namesadmin.hlp");

                        error = set_addid(gb_main, fieldNames[answer]);

                        free(buttons);
                    }
                }

                for (int c = 0; c<serverCount; c++) free(fieldNames[c]);
                free(fieldNames);
            }
        }
    }

    return error;
}


// ------------------------------------
//      class NameServerConnection

class NameServerConnection : virtual Noncopyable {
    aisc_com   *link;
    T_AN_LOCAL  locs;
    T_AN_MAIN   com;
    int         persistent;     // if true -> connection will not be closed
    time_t      linktime;       // time, when link has been established

    int init_local_com_names()
    {
        if (!link) return 1;    //!* create and init local com structure **
        if (aisc_create(link, AN_MAIN, com,
                        MAIN_LOCAL, AN_LOCAL, locs,
                        LOCAL_WHOAMI, "i bin der arb_tree",
                        NULL)) {
            return 1;
        }
        return 0;
    }

    GB_ERROR reconnect(GBDATA *gb_main) { // reconnect ignoring consistency
        int old_persistent = persistent;

        printf("Reconnecting name server\n");

        persistent = 0; // otherwise disconnect() won't disconnect
        disconnect();
        persistent = old_persistent; // restore previous persistence

        return connect(gb_main);
    }

    char *fieldUsedByServer(GB_ERROR& err) {
        char *field = 0;
        if (aisc_get(link, AN_MAIN, com,
                     MAIN_ADD_FIELD, &field,
                     NULL)) {
            err = "Connection Problems with the NAME_SERVER";
            aw_assert(field == 0);
        }
        return field;
    }

    GB_ERROR expectServerUsesField(const char *expected_field) {
        GB_ERROR  err          = 0;
        char     *server_field = fieldUsedByServer(err);

        if (!err && strcmp(expected_field, server_field) != 0) {
            err = GBS_global_string("Additional field doesn't match (expected='%s', server uses='%s')", expected_field, server_field);
        }
        free(server_field);
        return err;
    }

public:

    NameServerConnection() {
        link       = 0;
        locs.clear();
        com.clear();
        persistent = 0;
    }
    virtual ~NameServerConnection() {
        aw_assert(persistent == 0); // forgot to remove persistence ?
        disconnect();
    }

    GB_ERROR connect(GBDATA *gb_main) {
        aw_assert(!GB_have_error());

        arb_progress::show_comment("Connecting to name server");

        GB_ERROR err = 0;
        if (!link) {
            const char *add_field = AW_get_nameserver_addid(gb_main);
            const char *server_id = GBS_nameserver_tag(add_field);

            err = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_id);

            if (!err) {
                const char *ipport = GBS_read_arb_tcp(server_id);
                if (!ipport) err = GB_await_error();
                else {
                    link     = aisc_open(ipport, com, AISC_MAGIC_NUMBER, &err);
                    linktime = time(0);

                    if (!err) {
                        if (init_local_com_names()) {
                            err = GBS_global_string("Can't connect %s %s", server_id, ipport);
                        }
                        else {
                            err = expectServerUsesField(add_field);
                        }
                    }
                }
            }
        }
        else {
            long linkAge     = int(time(0)-linktime);
            bool doReconnect = false;

#if defined(DEBUG) && 0
            // print information about name-server link age
            static long lastage = -1;
            if (linkAge != lastage) {
                printf("Age of NameServerConnection: %li\n", linkAge);
                lastage = linkAge;
            }
#endif // DEBUG

            if (linkAge > (5*60)) { // perform a reconnect after 5 minutes
                // Reason : The pipe to the name server breaks after some time
                doReconnect = true;
            }
            else {
                const char *add_field = AW_get_nameserver_addid(gb_main);
                GB_ERROR    error     = expectServerUsesField(add_field);

                if (error) {
                    printf("Error: %s\n", error);
                    doReconnect = true;
                }
            }

            if (doReconnect) {
                err = reconnect(gb_main);
            }
        }
        aw_assert(!GB_have_error());
        return err;
    }

    void disconnect() {
        if (persistent == 0) {
            if (link) {
                aisc_close(link, com);
                locs.clear();
                com.clear();
            }
            link = 0;
        }
    }

    void persistence(bool persist) {
        if (persist) {
            ++persistent;
        }
        else {
            --persistent;
            if (persistent <= 0) {
                persistent = 0;
                disconnect();
            }
        }
    }


    aisc_com *getLink() { return link; }
    const T_AN_LOCAL& getLocs() const { return locs; } 
};

static NameServerConnection name_server;

PersistentNameServerConnection::PersistentNameServerConnection() {
    name_server.persistence(true);
}
PersistentNameServerConnection::~PersistentNameServerConnection() {
    name_server.persistence(false);
}

// --------------------------------------------------------------------------------

GB_ERROR AW_test_nameserver(GBDATA *gb_main) {
    return name_server.connect(gb_main);
}

// --------------------------------------------------------------------------------

GB_ERROR AWTC_generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, const char *addid, char*& new_name) {
    // create a unique short name for 'full_name'
    // the result is written into 'new_name' (as malloc-copy)
    // if fails: GB_ERROR!=0 && new_name==0
    // acc and addid may be 0

    new_name = 0;
    if (!acc) acc = "";

    arb_progress progress("Generating species ID");

    GB_ERROR err = name_server.connect(gb_main);
    if (err) return err;

    static char *shrt = 0;
    if (strlen(full_name)) {
        if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                      LOCAL_FULL_NAME,  full_name,
                      LOCAL_ACCESSION,  acc,
                      LOCAL_ADDID,      addid ? addid : "",
                      LOCAL_ADVICE,     "",
                      NULL)) {
            err = "Connection Problems with the NAME_SERVER";
        }
        if (aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_GET_SHORT,   &shrt,
                     NULL)) {
            err = "Connection Problems with the NAME_SERVER";
        }
    }

    if (err) {
        free(shrt);
    }
    else {
        if (shrt) {
            new_name = shrt;
            shrt = 0;
        }
        else {
            err = GB_export_errorf("Failed to generate species ID (shortname) for '%s'", full_name);
        }
    }

    name_server.disconnect();

    return err;
}

GB_ERROR AWTC_recreate_name(GBDATA *gb_species) {
    GBDATA       *gb_main = GB_get_root(gb_species);
    arb_progress  progress("Recreating species ID");
    GB_ERROR      error   = name_server.connect(gb_main);

    if (!error) {
        const char *add_field = AW_get_nameserver_addid(gb_main);
        char       *ali_name  = GBT_get_default_alignment(gb_main);

        GBDATA *gb_name      = GB_entry(gb_species, "name");
        GBDATA *gb_full_name = GB_entry(gb_species, "full_name");
        GBDATA *gb_acc       = GBT_gen_accession_number(gb_species, ali_name);
        GBDATA *gb_addfield  = add_field[0] ? GB_entry(gb_species, add_field) : 0;

        char *name      = gb_name ?     GB_read_string   (gb_name)     : strdup("");
        char *full_name = gb_full_name ? GB_read_string  (gb_full_name) : strdup("");
        char *acc       = gb_acc ?      GB_read_string   (gb_acc)      : strdup("");
        char *addid     = gb_addfield ? GB_read_as_string(gb_addfield) : strdup("");

        long   deleted = 0;
        char *shrt    = 0;

        if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                      LOCAL_FULL_NAME,  full_name,
                      LOCAL_ACCESSION,  acc,
                      LOCAL_ADDID,      addid,
                      LOCAL_ADVICE,     "",
                      NULL) != 0 ||
            aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_DEL_SHORT,   &deleted,
                     NULL)  != 0 ||
            aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_GET_SHORT,   &shrt,
                     NULL)  != 0)
        {
            error = "Connection Problems with the NAME_SERVER";
        }
        name_server.disconnect();

        if (!error) {
            GBT_begin_rename_session(gb_main, 0);
            error = GBT_rename_species(name, shrt, true);
            if (error) {
                if (GBT_find_species(gb_main, shrt)) { // it was a rename error
                    int done = 0;
                    error    = 0;
                    for (int count = 2; !done && !error && count<10; count++) {
                        const char *other_short = GBS_global_string("%s.%i", shrt, count);
                        if (!GBT_find_species(gb_main, other_short)) {
                            error            = GBT_rename_species(name, other_short, true);
                            if (!error) done = 1;
                        }
                    }

                    if (!done && !error) {
                        error = "Failed to regenerate species ID. Please use 'Species/Synchronize IDs'";
                    }
                }
            }

            if (error) GBT_abort_rename_session();
            else error = GBT_commit_rename_session();
        }

        free(shrt);
        free(addid);
        free(acc);
        free(full_name);
        free(name);
    }

    return error;
}

char *AWTC_create_numbered_suffix(GB_HASH *species_name_hash, const char *shortname, GB_ERROR& warning) {
    char *newshort = 0;
    if (GBS_read_hash(species_name_hash, shortname)) {
        int i;
        newshort = (char *)ARB_alloc(strlen(shortname)+20);
        for (i = 1; ; i++) {
            sprintf(newshort, "%s.%i", shortname, i);
            if (!GBS_read_hash(species_name_hash, newshort))break;
        }

        warning =
            "There are duplicated species!\n"
            "The IDs of these species ('name') contain a '.' character followed by a number.\n"
            "We strongly recommend you try understand and solve this problem\n"
            "(see HELP in 'Species/Synchronize IDs' window)";
    }
    return newshort;
}

GB_ERROR AWTC_pars_names(GBDATA *gb_main, bool *isWarningPtr) {
    // rename species according to name_server
    // 'isWarning' is set to true, in case of duplicates-warning

    arb_progress gen_progress("Generating new shortnames (IDs)");
    GB_ERROR     err       = name_server.connect(gb_main);
    bool         isWarning = false;

    if (!err) {
        err = GBT_begin_rename_session(gb_main, 1);
        if (!err) {
            char     *ali_name = GBT_get_default_alignment(gb_main);
            long      spcount  = GBT_get_species_count(gb_main);
            GB_HASH  *hash     = GBS_create_hash(spcount, GB_IGNORE_CASE);
            GB_ERROR  warning  = 0;

            if (spcount) {
                arb_progress  progress("Renaming species", spcount);
                const char   *add_field = AW_get_nameserver_addid(gb_main);

                for (GBDATA *gb_species = GBT_first_species(gb_main);
                     gb_species && !err;
                     gb_species = GBT_next_species(gb_species))
                {
                    GBDATA *gb_name      = GB_entry(gb_species, "name");
                    GBDATA *gb_full_name = GB_entry(gb_species, "full_name");
                    GBDATA *gb_acc       = GBT_gen_accession_number(gb_species, ali_name);
                    GBDATA *gb_addfield  = add_field[0] ? GB_entry(gb_species, add_field) : 0;

                    char *name      = gb_name      ? GB_read_string   (gb_name)     : strdup("");
                    char *full_name = gb_full_name ? GB_read_string   (gb_full_name) : strdup("");
                    char *acc       = gb_acc       ? GB_read_string   (gb_acc)      : strdup("");
                    char *addid     = gb_addfield  ? GB_read_as_string(gb_addfield) : strdup(""); // empty value will be set to default by nameserver

                    char *shrt = 0;

                    if (full_name[0] || acc[0] || addid[0]) {
                        if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                                      LOCAL_FULL_NAME,  full_name,
                                      LOCAL_ACCESSION,  acc,
                                      LOCAL_ADDID,      addid,
                                      LOCAL_ADVICE,     name,
                                      NULL)) {
                            err = "Connection Problems with the NAME_SERVER";
                        }
                        if (aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                                     LOCAL_GET_SHORT,   &shrt,
                                     NULL)) {
                            err = "Connection Problems with the NAME_SERVER";
                        }
                    }
                    else {
                        shrt = strdup(name);
                    }
                    if (!err) {
                        char *newshrt = AWTC_create_numbered_suffix(hash, shrt, warning);
                        if (newshrt) freeset(shrt, newshrt);

                        GBS_incr_hash(hash, shrt);
                        err = GBT_rename_species(name, shrt, true);
                    }

                    free(shrt);
                    free(addid);
                    free(acc);
                    free(full_name);
                    free(name);

                    progress.inc_and_check_user_abort(err);
                }
            }
            else {
                gen_progress.sub_progress_skipped(); // trigger skipped subcounter
            }

            if (err) GBT_abort_rename_session();
            else err = GBT_commit_rename_session();

            GBS_free_hash(hash);
            free(ali_name);

            if (!err) {
                err = warning;
                if (warning) isWarning = true;
            }
        }
        name_server.disconnect();
    }

    if (isWarningPtr) *isWarningPtr = isWarning;
    gen_progress.done(); // needed if err

    return err;
}


static void awt_rename_cb(AW_window *aww, GBDATA *gb_main) {
    GB_ERROR error = AWTC_pars_names(gb_main);
    if (error) aw_message(error);
    aww->get_root()->awar(AWAR_TREE_REFRESH)->touch();
}


AW_window *AWTC_create_rename_window(AW_root *root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "AUTORENAME_SPECIES", "Synchronize species IDs");

    aws->load_xfig("awtc/autoren.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("rename.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("go");
    aws->highlight();
    aws->callback(makeWindowCallback(awt_rename_cb, gb_main));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

UniqueNameDetector::UniqueNameDetector(GBDATA *gb_item_data, int additionalEntries) {
    hash = GBS_create_hash(GB_number_of_subentries(gb_item_data)+additionalEntries, GB_IGNORE_CASE);

    for (GBDATA *gb_item = GB_child(gb_item_data); gb_item; gb_item = GB_nextChild(gb_item)) {
        GBDATA *gb_name = GB_entry(gb_item, "name");
        if (gb_name) { // item has name -> insert to hash
            GBS_write_hash(hash, GB_read_char_pntr(gb_name), 1);
        }
    }
}

UniqueNameDetector::~UniqueNameDetector() { GBS_free_hash(hash); }

static char *makeUniqueShortName(const char *prefix, UniqueNameDetector& existing) {
    // generates a non-existing short-name (name starts with prefix)
    //
    // returns NULL if it fails

    char *result     = 0;
    int   prefix_len = strlen(prefix);

    aw_assert(prefix_len<8); // prefix has to be shorter than 8 chars!
    if (prefix_len<8) {
        const int max_nums[8] = { 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10 };
        static int next_try[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

        int  max_num = max_nums[prefix_len];
        char short_name[9];
        strcpy(short_name, prefix);

        char *dig_pos = short_name+prefix_len;
        int   num     = next_try[prefix_len];
        int   stop    = num ? num-1 : max_num;

        while (num != stop) {
            sprintf(dig_pos, "%i", num);
            ++num;
            if (!existing.name_known(short_name)) {
                result = strdup(short_name);
                break;
            }
            if (num == max_num && stop != max_num) num = 0;
        }
        if (num == max_num) num = 0;
        next_try[prefix_len] = num;
    }
    return result;
}

char *AWTC_makeUniqueShortName(const char *prefix, UniqueNameDetector& existingNames) {
    // generates a unique species name from prefix
    // (prefix will be fillup with zero digits and then shortened down to first char)
    //
    // returns NULL if failed (and exports error)

    int  len = strlen(prefix);
    char p[9];
    strncpy(p, prefix, 8);

    if (len>8) len = 8;
    else {
        if (len == 0) p[len++] = 'x'; // don't use digit as first character
        while (len<8) p[len++] = '0';
    }

    p[len] = 0;

    char *result = 0;

    for (int l = len-1; l>0 && !result; --l) {
        p[l]   = 0;
        result = makeUniqueShortName(p, existingNames);
    }

    aw_assert(!result || strlen(result) <= 8);
    if (!result) GB_export_errorf("Failed to create unique species ID (prefix='%s')", prefix);

    return result;
}

char *AWTC_generate_random_name(UniqueNameDetector& existingNames) {
    char *new_species_name = 0;
    char  short_name[9];
    int   count            = 10000;

    short_name[8] = 0;
    while (count--) {
        short_name[0] = 'a'+GB_random(26); // first character has to be alpha

        for (int x=1; x<8; ++x) {
            int r = GB_random(36); // rest may be alphanumeric
            short_name[x] = r<10 ? ('0'+r) : ('a'+r-10);
        }

        if (!existingNames.name_known(short_name)) {
            new_species_name = strdup(short_name);
            break;
        }
    }

    if (!new_species_name) {
        aw_message("Failed to generate a random name - retrying (this might hang forever)");
        return AWTC_generate_random_name(existingNames);
    }

    return new_species_name;
}

int AWTC_name_quality(const char *short_name) {
    // result 0 = ok for external tools
    //        1 = ok for ARB
    //        2 = not ok

    int len         = -1;
    int alnum_count = 0;
    int ascii_count = 0;

    while (char c = short_name[++len]) {
        alnum_count += (isalnum(c) != 0);
        ascii_count += (c > 32 && c < 127);
    }

    if (len>0) {
        if (len <= 8) {
            if (len == alnum_count) return 0; // ok for external programs
        }
        if (len == ascii_count) return 1; // ok for ARB
    }
    return 2; // not ok
}
