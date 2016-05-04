// =============================================================== //
//                                                                 //
//   File      : adtcp.cxx                                         //
//   Purpose   : arb_tcp.dat handling                              //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2007     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ctime>
#include <sys/stat.h>

#include <arbdbt.h>
#include <arb_str.h>

#include "gb_local.h"

#if defined(DEBUG)
// #define DUMP_ATD_ACCESS
#endif // DEBUG

// ------------------------------------------------------------
// Data representing current content of arb_tcp.dat

class ArbTcpDat : virtual Noncopyable {
    GB_ULONG  modtime;                              // modification time of read-in arb_tcp.dat
    char     *filename;                             // pathname of loaded arb_tcp.dat

    char **content; /* zero-pointer terminated array of multi-separated strings
                     * (strings have same format as the result of
                     * GBS_read_arb_tcp(), but also contain the first element,
                     * i.e. the server id)
                     */
    int serverCount;
    

    void freeContent() {
        if (content) {
            for (int c = 0; content[c]; c++) free(content[c]);
            freenull(content);
        }
    }
    GB_ERROR read(int *versionFound);

public:
    ArbTcpDat() : modtime(-1) , filename(NULL) , content(NULL), serverCount(-1) { }

    ~ArbTcpDat() {
        free(filename);
        freeContent();
    }

    GB_ERROR update();

    int get_server_count() const { return serverCount; }
    const char *get_serverID(int idx) const { gb_assert(idx >= 0 && idx<serverCount); return content[idx]; }
    const char *get_entry(const char *serverID) const;
    const char *get_filename() const { return filename; }

#if defined(DUMP_ATD_ACCESS)
    void dump();
#endif
};

const char *ArbTcpDat::get_entry(const char *serverID) const {
    const char *result = 0;
    if (content) {
        int c;
        for (c = 0; content[c]; c++) {
            const char *id = content[c];

            if (strcmp(id, serverID) == 0) {
                result = strchr(id, 0)+1; // return pointer to first parameter
                break;
            }
        }
    }
    return result;
}

#if defined(DUMP_ATD_ACCESS)
void ArbTcpDat::dump() {
    printf("filename='%s'\n", filename);
    printf("modtime='%lu'\n", modtime);
    if (content) {
        int c;
        for (c = 0; content[c]; c++) {
            char *data = content[c];
            char *tok  = data;
            printf("Entry #%i:\n", c);

            while (tok[0]) {
                printf("- '%s'\n", tok);
                tok = strchr(tok, 0)+1;
            }
        }
    }
    else {
        printf("No content\n");
    }
}
#endif // DUMP_ATD_ACCESS

#define MAXLINELEN 512
#define MAXTOKENS  10

GB_ERROR ArbTcpDat::read(int *versionFound) {
    // used to read arb_tcp.dat or arb_tcp_org.dat
    GB_ERROR  error = 0;
    FILE     *in    = fopen(filename, "rt");

    *versionFound = 1; // default to version 1 (old format)

#if defined(DUMP_ATD_ACCESS)
    printf("(re)reading %s\n", filename);
#endif // DUMP_ATD_ACCESS

    freeContent();

    if (!in) {
        error = GBS_global_string("Can't read '%s'", filename);
    }
    else {
        char   buffer[MAXLINELEN+1];
        char  *lp;
        int    lineNumber = 0;
        char **tokens     = (char**)malloc(MAXTOKENS*sizeof(*tokens));

        int    entries_allocated = 30;
        int    entries           = 0;
        char **entry             = (char**)malloc(entries_allocated*sizeof(*entry));

        if (!tokens || !entry) error = "Out of memory";

        for (lp = fgets(buffer, MAXLINELEN, in);
             lp && !error;
             lp = fgets(buffer, MAXLINELEN, in))
        {
            char *tok;
            int   tokCount = 0;
            int   t;

            lineNumber++;

            while ((tok = strtok(lp, " \t\n"))) {
                if (tok[0] == '#') break; // EOL comment -> stop
                if (tokCount >= MAXTOKENS) { error = "Too many tokens"; break; }
                tokens[tokCount] = tokCount ? GBS_eval_env(tok) : strdup(tok);
                if (!tokens[tokCount]) { error = GB_await_error(); break; }
                tokCount++;
                lp = 0;
            }

            if (!error && tokCount>0) {
                if (strcmp(tokens[0], "ARB_TCP_DAT_VERSION") == 0) {
                    if (tokCount>1) *versionFound = atoi(tokens[1]);
                }
                else {
                    char *data;
                    {
                        int allsize = 0;
                        int size[MAXTOKENS];

                        for (t = 0; t<tokCount; t++) {
                            size[t]  = strlen(tokens[t])+1;
                            allsize += size[t];
                        }
                        allsize++;      // additional zero byte

                        data = (char*)malloc(allsize);
                        {
                            char *d = data;
                            for (t = 0; t<tokCount; t++) {
                                memmove(d, tokens[t], size[t]);
                                d += size[t];
                            }
                            d[0] = 0;
                        }
                    }

                    if (entries == entries_allocated) {
                        entries_allocated = (int)(entries_allocated*1.5);
                        char **entry2     = (char**)realloc(entry, entries_allocated*sizeof(*entry));

                        if (!entry2) error = "Out of memory";
                        else entry = entry2;
                    }
                    if (!error) {
                        entry[entries++] = data;
                        data             = 0;
                    }
                    free(data);
                }
            }

            if (error) error = GBS_global_string("%s (in line %i of '%s')", error, lineNumber, filename);
            for (t = 0; t<tokCount; t++) freenull(tokens[t]);
        }

        content = (char**)realloc(entry, (entries+1)*sizeof(*entry));

        if (!content) {
            error       = "Out of memory";
            serverCount = 0;
            free(entry);
        }
        else {
            content[entries] = 0;
            serverCount      = entries;
        }

        free(tokens);
        fclose(in);
    }

#if defined(DUMP_ATD_ACCESS)
    dump();
#endif // DUMP_ATD_ACCESS
    return error;
}

char *GB_arbtcpdat_path() {
    return GB_lib_file(true, "", "arb_tcp.dat");
}

GB_ERROR ArbTcpDat::update() {
    // read arb_tcp.dat (once or if changed on disk)
    GB_ERROR error = 0;

    if (!filename) filename = GB_arbtcpdat_path();

    if (!filename) {
        error = "File $ARBHOME/lib/arb_tcp.dat missing or unreadable";
    }
    else {
        struct stat st;
        if (stat(filename, &st) == 0) {
            GB_ULONG mtime = st.st_mtime;
            if (modtime != mtime) { // read once or when changed
                int arb_tcp_version;
                error = read(&arb_tcp_version);

                if (!error) {
                    int expected_version = 2;
                    if (arb_tcp_version != expected_version) {
                        error = GBS_global_string("Expected arb_tcp.dat version %i\n"
                                                  "Your '%s' has version %i\n"
                                                  "To solve the problem\n"
                                                  "- either reinstall ARB and do not select\n"
                                                  "  'Use information of already installed ARB'\n"
                                                  "  (any changes to arb_tcp.dat will be lost)\n"
                                                  "- or backup your changed %s,\n"
                                                  "  replace it by the contents from $ARBHOME/lib/arb_tcp_org.dat\n"
                                                  "  and edit it to fit your needs.",
                                                  expected_version,
                                                  filename, arb_tcp_version,
                                                  filename);
                    }
                }
                modtime = error ? -1 : mtime;
            }
        }
        else {
            error = GBS_global_string("Can't stat '%s'", filename);
        }
    }
    
    if (error) {
        freenull(filename);
#if defined(DUMP_ATD_ACCESS)
        printf("error=%s\n", error);
#endif // DUMP_ATD_ACCESS
    }

    return error;
}

static ArbTcpDat arb_tcp_dat;

// --------------------------------------------------------------------------------

const char *GBS_scan_arb_tcp_param(const char *ipPort, const char *wantedParam) {
    /* search a specific server parameter in result from GBS_read_arb_tcp()
     * wantedParam may be sth like '-d' (case is ignored!)
     */
    const char *result = 0;
    if (ipPort) {
        const char *exe   = strchr(ipPort, 0)+1;
        const char *param = strchr(exe, 0)+1;
        size_t      plen  = strlen(param);
        size_t      wplen = strlen(wantedParam);

        while (plen) {
            if (strncasecmp(param, wantedParam, wplen) == 0) { // starts with wantedParam
                result = param+wplen;
                break;
            }
            param += plen+1;    // position behind 0-delimiter
            plen   = strlen(param);
        }
    }
    return result;
}

// AISC_MKPT_PROMOTE:#ifdef UNIT_TESTS // UT_DIFF
// AISC_MKPT_PROMOTE:#define TEST_SERVER_ID (-666)
// AISC_MKPT_PROMOTE:#define TEST_GENESERVER_ID (-667)
// AISC_MKPT_PROMOTE:#endif

const char *GBS_nameserver_tag(const char *add_field) {
    if (add_field && add_field[0]) {
        char *tag = GBS_global_string_copy("ARB_NAME_SERVER_%s", add_field);
        ARB_strupper(tag);
        RETURN_LOCAL_ALLOC(tag);
    }
    return "ARB_NAME_SERVER";
}

const char *GBS_ptserver_tag(int id) {
#ifdef UNIT_TESTS // UT_DIFF
    if (id == TEST_SERVER_ID) return "ARB_TEST_PT_SERVER";
    if (id == TEST_GENESERVER_ID) return "ARB_TEST_PT_SERVER_GENE";
#endif // UNIT_TESTS
    gb_assert(id >= 0);
    const int   MAXIDSIZE = 30;
    static char server_tag[MAXIDSIZE];
    // ASSERT_RESULT_BELOW(int, sprintf(server_tag, "ARB_PT_SERVER%i", id), MAXIDSIZE);
    ASSERT_RESULT_PREDICATE(isBelow<int>(MAXIDSIZE), sprintf(server_tag, "ARB_PT_SERVER%i", id));
    return server_tag;
}

const char *GBS_read_arb_tcp(const char *env) {
    /*! Find an entry in the $ARBHOME/lib/arb_tcp.dat file
     *
     * Be aware: GBS_read_arb_tcp returns a quite unusual string containing several string delimiters (0-characters).
     * It contains all words (separated by space or tab in arb_tcp.dat) of the corresponding line in arb_tcp.dat.
     * These words get concatenated (separated by 0 characters).
     *
     * The first word (which matches the parameter env) is skipped.
     * The second word (the socket info = "host:port") is returned directly as result.
     * The third word is the server executable name.
     * The fourth and following words are parameters to the executable.
     *
     * To access these words follow this example:
     *
     * const char *ipPort = GBS_read_arb_tcp(GBS_nameserver_tag(NULL));
     * if (ipPort) {
     *     const char *serverExe = strchr(ipPort, 0)+1;
     *     const char *para1     = strchr(serverExe, 0)+1; // always exists!
     *     const char *para2     = strchr(para1, 0)+1;
     *     if (para2[0]) {
     *         // para2 exists
     *     }
     * }
     *
     * see also GBS_read_arb_tcp_param() above
     *
     * Returns NULL on error (which is exported in that case)
     */

    const char *result = 0;
    GB_ERROR    error  = 0;

    if (strchr(env, ':')) {
        static char *resBuf = 0;
        freedup(resBuf, env);
        result = resBuf;
    }
    else {
        error = arb_tcp_dat.update(); // load once or reload after change on disk
        if (!error) {
            const char *user = GB_getenvUSER();
            if (!user) {
                error = "Environment variable 'USER' not defined";
            }
            else {
                char *envuser = GBS_global_string_copy("%s:%s", user, env);
                result        = arb_tcp_dat.get_entry(envuser);

                if (!result) { // no user-specific entry found
                    result = arb_tcp_dat.get_entry(env);
                    if (!result) {
                        error = GBS_global_string("Expected entry '%s' or '%s' in '%s'",
                                                  env, envuser, arb_tcp_dat.get_filename());
                    }
                }
                free(envuser);
            }
        }
    }

    gb_assert(result||error);
    if (error) {
        GB_export_error(error);
        result = 0;
    }
    return result;
}

const char * const *GBS_get_arb_tcp_entries(const char *matching) {
    /*! returns a list of all matching non-user-specific entries found in arb_tcp.dat
     * match is performed by GBS_string_matches (e.g. use "ARB_PT_SERVER*")
     */
    static const char **matchingEntries     = 0;
    static int          matchingEntriesSize = 0;

    GB_ERROR error = arb_tcp_dat.update();
    if (!error) {
        int count = arb_tcp_dat.get_server_count();

        if (matchingEntriesSize != count) {
            freeset(matchingEntries, (const char **)malloc((count+1)*sizeof(*matchingEntries)));
            matchingEntriesSize = count;
        }

        int matched = 0;
        for (int c = 0; c<count; c++) {
            const char *id = arb_tcp_dat.get_serverID(c);

            if (strchr(id, ':') == 0) { // not user-specific
                if (GBS_string_matches(id, matching, GB_MIND_CASE)) { // matches
                    matchingEntries[matched++] = id;
                }
            }
        }
        matchingEntries[matched] = 0; // end of array
    }
    if (error) GB_export_error(error);
    return error ? 0 : matchingEntries;
}

// ---------------------------
//      pt server related

const char *GBS_ptserver_logname() {
    RETURN_ONETIME_ALLOC(nulldup(GB_path_in_ARBLIB("pts/ptserver.log")));
}

void GBS_add_ptserver_logentry(const char *entry) {
    FILE *log = fopen(GBS_ptserver_logname(), "at");
    if (log) {
        chmod(GBS_ptserver_logname(), 0666);

        char    atime[256];
        time_t  t   = time(0);
        tm     *tms = localtime(&t);

        strftime(atime, 255, "%Y/%m/%d %k:%M:%S", tms);
        fprintf(log, "%s %s\n", atime, entry);
        fclose(log);
    }
    else {
        fprintf(stderr, "Failed to write to '%s'\n", GBS_ptserver_logname());
    }
}

char *GBS_ptserver_id_to_choice(int i, int showBuild) {
    /* Return a readable name for PTserver number 'i'
     * If 'showBuild' then show build info as well.
     *
     * Return NULL in case of error (which was exported then)
     */
    const char *ipPort = GBS_read_arb_tcp(GBS_ptserver_tag(i));
    char       *result = 0;

    if (ipPort) {
        const char *file     = GBS_scan_arb_tcp_param(ipPort, "-d");
        const char *nameOnly = strrchr(file, '/');

        if (nameOnly) nameOnly++;   // position behind '/'
        else nameOnly = file;       // otherwise show complete file

        {
            char *remote      = strdup(ipPort);
            char *colon       = strchr(remote, ':');
            if (colon) *colon = 0; // hide port

            if (strcmp(remote, "localhost") == 0) { // hide localhost
                result = nulldup(nameOnly);
            }
            else {
                result = GBS_global_string_copy("%s: %s", remote, nameOnly);
            }
            free(remote);
        }


        if (showBuild) {
            struct stat  st;

            if (stat(file, &st) == 0) { // xxx.arb present
                time_t  fileMod   = st.st_mtime;
                char   *serverDB  = GBS_global_string_copy("%s.pt", file);
                char   *newResult = 0;

                if (stat(serverDB, &st) == 0) { // pt-database present
                    if (st.st_mtime < fileMod) { // DB is newer than pt-database
                        newResult = GBS_global_string_copy("%s [starting or failed update]", result);
                    }
                    else {
                        char  atime[256];
                        tm   *tms = localtime(&st.st_mtime);

                        strftime(atime, 255, "%Y/%m/%d %k:%M", tms);
                        newResult = GBS_global_string_copy("%s [%s]", result, atime);
                    }
                }
                else { // check for running build
                    char *serverDB_duringBuild = GBS_global_string_copy("%s%%", serverDB);
                    if (stat(serverDB_duringBuild, &st) == 0) {
                        newResult = GBS_global_string_copy("%s [building..]", result);
                    }
                    free(serverDB_duringBuild);
                }

                if (newResult) freeset(result, newResult);
                free(serverDB);
            }
        }
    }

    return result;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

void TEST_GBS_servertags() {
    TEST_EXPECT_EQUAL(GBS_ptserver_tag(0), "ARB_PT_SERVER0");
    TEST_EXPECT_EQUAL(GBS_ptserver_tag(7), "ARB_PT_SERVER7");
    
    TEST_EXPECT_EQUAL(GBS_nameserver_tag(NULL),   "ARB_NAME_SERVER");
    TEST_EXPECT_EQUAL(GBS_nameserver_tag(""),     "ARB_NAME_SERVER");
    TEST_EXPECT_EQUAL(GBS_nameserver_tag("test"), "ARB_NAME_SERVER_TEST");
}

#endif // UNIT_TESTS
