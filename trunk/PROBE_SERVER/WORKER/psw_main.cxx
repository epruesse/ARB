//  ==================================================================== //
//                                                                       //
//    File      : psw_main.cxx                                           //
//    Purpose   : Worker process (handles requests from cgi scripts)     //
//    Time-stamp: <Fri May/16/2008 11:16 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <cstdio>
#include <string>
#include <queue>
#include <map>
#include <list>
#include <cctype>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "arbdb.h"
#include "smartptr.h"

#define psw_assert(x) arb_assert(x)

#include "../global_defs.h"
#define NEED_getDatabaseState
#include "../common.h"
#include "../mapping.h"

#if defined(DEBUG)
// #define VERBOSE
#endif // DEBUG

using namespace std;

// -------------------------------
//      Command line arguments

namespace {

    struct Parameter {
        string probe_db_name;
        int    probe_length;
        string working_dir;
    };

    static Parameter para;

    void helpArguments() {
        fprintf(stderr,
                "\n"
                "Usage: arb_probe_server_worker <probe_db_name> <probe_length> <working_dir>\n"
                "\n"
                "probe_db_name     name of Probe-Group-database to use\n"
                "probe_length      probe length\n"
                "working_dir       directory for requests and responses\n"
                "\n"
                "Note: you should start one server for each probe length!\n"
                "\n"
                );
    }

    GB_ERROR scanArguments(int argc,  char *argv[]) {
        GB_ERROR error = 0;

        if (argc == 4) {
            para.probe_db_name = argv[1];
            para.probe_length  = atoi(argv[2]);
            para.working_dir   = argv[3];
        }
        else {
            error = "Missing arguments";
        }

        if (error) helpArguments();
        return error;
    }
}

// ----------------
//      Toolkit

namespace {
    bool saveRename(const char *oldname, const char *newname) {
        int res = rename(oldname, newname);
        if (res == 0) {
            struct stat st;
            if (stat(newname, &st) == 0) {
                return true;
            }
        }
        return false;
    }
}

// -----------------------------------
//      Paths (encoding and cache)

namespace {

#include "../path_code.h"

    GB_HASH *path_cache          = 0; // links encoded "path" content with "subtree" db-pointer
    GB_HASH *group_cache         = 0; // links encoded "path" content with "subtree" db-pointer
//     GBDATA  *gb_is_inner_node = 0;

    GB_ERROR init_path_cache(GBDATA *gb_main) {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_subtrees_cnt = GB_search(gb_main, "subtree_counter", GB_FIND);

//         gb_is_inner_node = gb_subtrees; // the pointer is only used as flag!

        if (gb_subtrees_cnt) {
            int subtrees = GB_read_int(gb_subtrees_cnt);
            path_cache   = GBS_create_hash(subtrees, 1);

            GBDATA *gb_subtrees = GB_search(gb_main, "subtrees", GB_FIND);
            if (gb_subtrees) {
                GB_ERROR error  = 0;
                for (GBDATA *gb_subtree = GB_entry(gb_subtrees, "subtree");
                     gb_subtree && !error;
                     gb_subtree = GB_nextEntry(gb_subtree))
                {
                    GBDATA *gb_path = GB_search(gb_subtree, "path", GB_FIND);
                    if (!gb_path) {
                        error = "subtree w/o path (database corrupt!)";
                    }
                    else {
                        const char *path    = GB_read_char_pntr(gb_path);
                        // int         pathlen = GB_read_string_count(gb_path);
                        //                         const char *enc_path = encodePath(path, pathlen, error);
                        //                         if (enc_path) GBS_write_hash(path_cache, enc_path, (long)gb_subtree);
                        GBS_write_hash(path_cache, path, (long)gb_subtree);
                    }
                }
                return error;
            }
        }
        return GB_get_error();
    }

    GB_ERROR init_group_cache(GBDATA *gb_main) {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_probegroups = GB_search(gb_main, "probe_group_counter", GB_FIND);

        if (gb_probegroups) {
            int probegroups = GB_read_int(gb_probegroups);
            group_cache     = GBS_create_hash(probegroups, 1);

            GBDATA *gb_probe_groups = GB_search(gb_main, "probe_groups", GB_FIND);
            if (gb_probe_groups) {
                GB_ERROR error  = 0;
                for (GBDATA *gb_probe_group = GB_entry(gb_probe_groups, "probe_group");
                     gb_probe_group && !error;
                     gb_probe_group = GB_nextEntry(gb_probe_group))
                {
                    GBDATA *gb_id = GB_search(gb_probe_group, "id", GB_FIND);
                    if (gb_id) {
                        const char *group_id = GB_read_char_pntr(gb_id);
                        GBS_write_hash(group_cache, group_id, (long)gb_probe_group);
                    }
                }
                return error;
            }
        }
        return GB_get_error();
    }
}

// ------------------
//      Arguments

namespace {
    typedef map<string, string> Arguments;

    const char *getline(FILE *in) {
        const int    buffersize = 512;
        static char *buffer     = (char*)malloc(buffersize);

        char *result = fgets(buffer, buffersize, in);
        if (!result) return 0;

        int len = strlen(result);
        if (result[len-1] == '\n') {
            result[len-1] = 0;
        }
        else {
            fprintf(stderr, "buffer overflow (line to long)\n");
        }

        return buffer;
    }

    GB_ERROR readArguments(FILE *in, Arguments& args) {
        int      count = 0;
        GB_ERROR error = 0;

        for (const char *line = getline(in);
             line && !error;
             line = getline(in))
        {
            ++count;
            const char *eq = strchr(line, '=');
            if (!eq) {
                error = GBS_global_string("'=' expected in line %i (%s)", count, line);
            }
            else {
                args[string(line, eq-line)] = string(eq+1);
            }
        }
        return error;
    }

    void dumpArguments(const Arguments& args, FILE *out) {
        for (Arguments::const_iterator a = args.begin(); a != args.end(); ++a) {
            fprintf(out, "  '%s'='%s'\n", a->first.c_str(), a->second.c_str());
        }
    }

    GB_ERROR expectArgument(const Arguments& args, const string& argname, const char*& result) {
        Arguments::const_iterator found = args.find(argname);
        if (found == args.end()) {
            return GBS_global_string("argument '%s' expected", argname.c_str());
        }
        result = found->second.c_str();
        return 0;
    }
}

// -------------------------------------
//      Helper functions for commands

namespace {
    static GB_ERROR loadProbeListFromString(std::list<const char *>& probes, GBDATA *gb_father,
                                            const char *entry_name, const char *group_id)
    {
        GB_ERROR  error    = 0;
        GBDATA   *gb_entry = GB_entry(gb_father, entry_name);

        if (gb_entry) {
            char *probe_string = GB_read_string(gb_entry);
            if (!probe_string) {
                error = GB_get_error();                
                psw_assert(error);
            }
            else {
                for (const char *tok = strtok(probe_string, ",");
                     tok;
                     tok = strtok(0, ","))
                {
                    probes.push_back(tok);
                }
            }
        }
        else {
            fprintf(stderr, "no '%s' entry found for group_id '%s' -- ignored\n", entry_name, group_id);
        }

        return error;
    }

    
    GB_ERROR extractProbes(const char *group_id, list<const char *>& probes) {
        GB_ERROR  error          = 0;
        GBDATA   *gb_probe_group = (GBDATA*)GBS_read_hash(group_cache, group_id); // search for probe group

        if (!gb_probe_group) {
            error = GBS_global_string("Can't find probe group '%s'", group_id);
        }
        else {
            error = loadProbeListFromString(probes, gb_probe_group, "common_probes", group_id);
            
#if 0
            GBDATA *gb_probe_group_common = GB_search(gb_probe_group, "probe_group_common", GB_FIND);

            if (!gb_probe_group_common) {
                fprintf(stderr, "no 'probe_group_common' entry found for group_id '%s' -- ignored\n", group_id);
                // error = "no 'probe_group_common' entry found";
            }
            else {
                for (GBDATA *gb_probe = GB_entry(gb_probe_group_common, "probe");
                     gb_probe;
                     gb_probe = GB_nextEntry(gb_probe))
                {
                    probes.push_back(GB_read_char_pntr(gb_probe));
                }
            }
#endif
        }

        return error;
    }

    GB_ERROR extractSubtreeMembers(const char *decoded_path, int len, list<string>& members) {
        GB_ERROR  error      = 0;
        GBDATA   *gb_subtree = 0;
        {
            const char *encoded_path = encodePath(decoded_path, len, error);
            if (!error) gb_subtree = (GBDATA*)GBS_read_hash(path_cache, encoded_path);
        }

        if (!error) {
            if (!gb_subtree) {
                error = GBS_global_string("unknown subtree '%s'", decoded_path);
            }
            else {
                GBDATA *gb_member = GB_entry(gb_subtree, "member");

                if (gb_member) { // leaf node
                    SpeciesID     id   = GB_read_int(gb_member);
                    const string& name = PM_ID2name(id, error);
                    if (!error) members.push_back(name);
                }
                else {
                    char *sonpath = GBS_global_string_copy("%sL", decoded_path);

                    if (!error) error = extractSubtreeMembers(sonpath, len+1, members); // left son
                    sonpath[len]      = 'R';
                    if (!error) error = extractSubtreeMembers(sonpath, len+1, members); // right son

                    free(sonpath);
                }
            }
        }
        return error;
    }

    GB_ERROR extractIndependentMembers(const char *group_id, list<string>& members) {
        GB_ERROR  error          = 0;
        GBDATA   *gb_probe_group = (GBDATA*)GBS_read_hash(group_cache, group_id);

        if (!gb_probe_group) {
            error = GBS_global_string("Can't find probe group '%s'", group_id);
        }
        else {
            GBDATA *gb_members = GB_entry(gb_probe_group, "members");

            if (!gb_members) {
                error = GBS_global_string("group '%s' has no 'members' (internal error)", group_id);
            }
            else {
                const char *memberlist = GB_read_char_pntr(gb_members);
                psw_assert(memberlist);
                if (memberlist) {
                    const char *number = memberlist;

                    while (number && !error) {
                        SpeciesID     id   = atoi(number);
                        const string& name = PM_ID2name(id, error);

                        if (!error) {
                            members.push_back(name);

                            const char *komma = strchr(number, ',');

                            if (komma) number = komma+1;
                            else number       = 0;
                        }
                    }
                }
            }
        }

        return error;
    }
}


// -----------------------
//      Known commands

namespace {
    typedef                      GB_ERROR (*Command)(GBDATA *gb_main, const Arguments& args, FILE *out);
    typedef map<string, Command> Commands;

    Commands command_list;

    // returns a list of species for a specific probe-group
    GB_ERROR CMD_getmembers(GBDATA *gb_main, const Arguments& args, FILE *out) {
        const char     *id;
        GB_ERROR        error = expectArgument(args, "id", id);
        GB_transaction  dummy(gb_main);

        if (!error) {
            list<string> members;

            if (id[0] == 'p') { // probe group hitting complete subtree
                // @@@ FIXME: in final version this should raise an error (because it's not necessary - should be done by client)

                const char *decoded_path = decodePath(id+1, error);
                if (!error) error = extractSubtreeMembers(decoded_path, strlen(decoded_path), members);
            }
            else if (id[0] == 'g') {
                error = extractIndependentMembers(id, members);
            }
            else {
                error = GBS_global_string("Illegal group id '%s'", id);
            }

            if (!error) {
                if (members.empty()) {
                    error = "probe group with zero members (internal error)";
                }
                else {
                    fprintf(out, "result=ok\nmembercount=%zu\n", members.size());
                    int c = 1;
                    for (list<string>::iterator m = members.begin(); m != members.end(); ++m, ++c) {
                        fprintf(out, "m%i=%s\n", c, m->c_str());
                    }
                }
            }
        }
        return error;
    }

    // returns a list of probes for a specific node
    // (Note: Not a real command - used by CMD_getexactprobes and CMD_getnonexactprobes)
    GB_ERROR CMD_getprobes(GBDATA *gb_main, const Arguments& args, FILE *out, bool get_exact) {
        const char     *path;
        GB_ERROR        error = expectArgument(args, "path", path);
        GB_transaction  dummy(gb_main);

        if (!error) {
            GBDATA     *gb_subtree = (GBDATA*)GBS_read_hash(path_cache, path); // expects path to be in encoded format
            const char *enc_path   = path;

            if (!gb_subtree) {
                int         pathlen       = strlen(path);
                const char *test_enc_path = encodePath(path, pathlen, error);

                if (!error) {
                    enc_path = test_enc_path;
                    gb_subtree = (GBDATA*)GBS_read_hash(path_cache, enc_path);
                }

                fprintf(stderr, "testing whether '%s' is a plain path (%s) = %s\n", path, gb_subtree ? "yes" : "no", enc_path);
                error = 0;
            }

            if (!gb_subtree) {
                if (error) {
                    error = GBS_global_string("Illegal subtree path '%s' (enc_path=%s, reason: %s)", path, enc_path, error);
                }
                else {
                    const char *dec_path = decodePath(enc_path, error);
                    error                = GBS_global_string("Illegal subtree path '%s' (enc_path=%s, dec_path=%s, error=%s)", path, enc_path, dec_path, error);
                }
            }
            else {              // we have a legal subtree                
                GBDATA *gb_exact = GB_search(gb_subtree, "exact", GB_FIND);
                if (gb_exact) { // there are exact matches
                    size_t exact = GB_read_int(gb_exact);
                    if (exact == 0) {
                        error = "exact has illegal value 0";
                    }
                    else {
                        if (get_exact) {
                            list<const char *> exactProbes; // probes exactly hitting the subtree

                            psw_assert(exact != 0); // in that case gb_exact should be 0

                            char *group_id = GBS_global_string_copy("p%s", enc_path);
                            error          = extractProbes(group_id, exactProbes);

                            if (!error && (exactProbes.size() != exact)) {
                                error = GBS_global_string("Expected to find %zu probes (found: %zu)", exact, exactProbes.size());
                            }

                            if (!error) {
                                fprintf(out, "result=ok\nfound=%zu\n", exact);
                                int count = 0;

                                for (list<const char*>::iterator p = exactProbes.begin(); p != exactProbes.end(); ++p, ++count) {
                                    fprintf(out, "probe%i=%s,E,%s\n", count, *p, group_id);
                                }
                            }

                            free(group_id);
                        }
                        else {
                            // only non-exact probes are requested -> return none
                            fprintf(out, "result=ok\nfound=0\n");
                        }
                    }

                    if (error) {
                        GB_ERROR ignore;
                        error = GBS_global_string("%s (in subtree '%s'='%s')", error, enc_path, decodePath(enc_path, ignore));
                    }
                }
                else {
                    if (!get_exact) {
                        list<const char *>  coverageProbes; // probes covering most of the subtree (w/o non group hits)
                        list<const char *>  nonGroupHitProbes; // probes covering the whole subtree (with non group hits)
                        int                 coverage_percentage = 0;
                        int                 non_group_hits      = 0;
                        const char         *covering_group_id   = 0;
                        const char         *ngh_group_id        = 0;

                        if (GBDATA *gb_coverage = GB_entry(gb_subtree, "coverage")) {
                            int     coverage       = GB_read_int(gb_coverage);
                            GBDATA *gb_coverage_id = GB_entry(gb_subtree, "coverage_id");

                            if (!gb_coverage_id) {
                                error = "'coverage_id' expected";
                            }
                            else {
                                GBDATA *gb_speccount = GB_entry(gb_subtree, "speccount");
                                if (!gb_speccount) {
                                    error = "'speccount' expected";
                                }
                                else {
                                    int speccount     = GB_read_int(gb_speccount);
                                    covering_group_id = GB_read_char_pntr(gb_coverage_id);

                                    psw_assert(covering_group_id);
                                    psw_assert(speccount);
                                    psw_assert(coverage);

                                    coverage_percentage = int(double(coverage)/speccount*100.0+.5);
                                    error               = extractProbes(covering_group_id, coverageProbes);
                                }
                            }
                        }

                        if (!error) {
                            fprintf(out, "result=ok\nfound=%zu\n", coverageProbes.size()+nonGroupHitProbes.size());
                            int count = 0;

                            for (list<const char*>::iterator p = coverageProbes.begin(); p != coverageProbes.end(); ++p, ++count) {
                                fprintf(out, "probe%i=%s,C%i,%s\n", count, *p, coverage_percentage, covering_group_id);
                            }
                            for (list<const char*>::iterator p = nonGroupHitProbes.begin(); p != nonGroupHitProbes.end(); ++p, ++count) {
                                fprintf(out, "probe%i=%s,N%i,%s\n", count, *p, non_group_hits, ngh_group_id);
                            }
                        }
                    }
                    else {
                        // only exact probes are requested -> return none
                        fprintf(out, "result=ok\nfound=0\n");
                    }
                }
            }
        }

        return error;
    }

    GB_ERROR CMD_getexactprobes(GBDATA *gb_main, const Arguments& args, FILE *out) {
        return CMD_getprobes(gb_main, args, out, true);
    }
    GB_ERROR CMD_getnonexactprobes(GBDATA *gb_main, const Arguments& args, FILE *out) {
        return CMD_getprobes(gb_main, args, out, false);
    }

    void init_commands() {
        command_list["getexactprobes"]  = CMD_getexactprobes;
        command_list["getnonexactprobes"]  = CMD_getnonexactprobes;
        command_list["getmembers"] = CMD_getmembers;
    }
}

// -----------------------
//      class Request

namespace {
    class Request {
        string filename;
        bool   owned;           // whether we own the request file

        static unsigned long pid;

        bool get_ownership();

    public:
        Request(const string& filename_)
            : filename(filename_)
            , owned(false)
        {}
        ~Request();

        string answer(GBDATA *gb_main);
    };

    unsigned long Request::pid = (unsigned long)getpid();

    Request::~Request() {
        if (owned) {
            const char *name = filename.c_str();
            // printf("deleting '%s'\n", name);
            if (unlink(name) != 0) {
                const char *ioerr = strerror(errno);
                fprintf(stderr, "Error unlinking '%s' (%s)\n", name, ioerr);
            }
        }
#if defined(VERBOSE)
        else {
            printf("releasing non-owned request '%s'\n", filename.c_str());
        }
#endif // VERBOSE
    }

    bool Request::get_ownership() {
        if (!owned) {
            // try to get the unique ownership of the request file
            // by renaming it to a unique name

            const char *name        = filename.c_str();
            const char *accept_name = GBS_global_string("%s.%lu", name, pid);

            if (saveRename(name, accept_name)) {
                filename = accept_name;
                owned    = true;
            }
        }
        return owned;
    }

    string Request::answer(GBDATA *gb_main) {
        if (!get_ownership()) return "";

        const char *req_name    = filename.c_str();
        string      result_name = filename;
        size_t      ldot        = result_name.find_last_of('.');
        GB_ERROR    error       = 0;

        if (ldot != string::npos && ldot >= 4) {
            if (result_name.substr(ldot-4, 5) == ".req.") {
                Arguments args;
                GB_ERROR  request_error = 0;
                {
                    FILE *in = fopen(req_name, "rt");
                    if (!in) {
                        const char *ioerr = strerror(errno);
                        error             = GBS_global_string("Can't open '%s' (%s)", req_name, ioerr);
                    }
                    else {
                        request_error = readArguments(in, args);
                        fclose(in);
                    }
                }

                string command;
                if (!error && !request_error) {
                    Arguments::iterator cmd_arg = args.find("command");
                    if (cmd_arg != args.end()) {
                        command = cmd_arg->second;
                    }
                    else {
                        request_error = "argument 'command' expected";
                    }
                }

                if (!error) {
                    printf("answering request '%s' (command=%s)\n", req_name, command.c_str());
                    result_name[ldot-1] = 's'; // req -> res

                    const char *outname = result_name.c_str();
                    FILE       *out     = fopen(outname, "wt");

                    if (!out) {
                        error = GBS_global_string("Can't create result file '%s'", outname);
                    }
                    else {
                        if (!request_error) {
                            Commands::iterator cmd_found = command_list.find(command);
                            if (cmd_found == command_list.end()) {
                                request_error = GBS_global_string("Unknown command '%s'", command.c_str());
                            }
                            else {
                                Command cmd   = cmd_found->second;
                                request_error = cmd(gb_main, args, out);
                            }
                        }

                        if (request_error) {
                            fprintf(out, "result=error\nmessage=%s\n", request_error);
                        }
                        fclose(out);

                        char *finaloutname = strdup(outname);
                        finaloutname[ldot] = 0; // remove .PID from filename

                        if (!saveRename(outname, finaloutname)) {
                            const char *ioerr = strerror(errno);
                            error             = GBS_global_string("Error renaming '%s' -> '%s' (%s)\n", outname, finaloutname, ioerr);
                            if (unlink(outname) != 0) {
                                ioerr = strerror(errno);
                                fprintf(stderr, "Error unlinking useless result file '%s' (%s)\n", outname, ioerr);
                            }
                        }
                        // if (!error) printf("result written ('%s')\n", finaloutname);
                    }
                }
            }
        }

        return error ? error : "";
    }
}

// ----------------------
//      class Worker

namespace {
    typedef SmartPtr<Request> RequestPtr;
    typedef deque<RequestPtr> RequestQueue;

    class Worker {
        GB_ERROR      error;
        GBDATA       *gb_main;
        bool          terminate; // "shutdown" command reveived
        RequestQueue  requests; // waiting requests
        string        working_directory;
        int           probe_length;

        void findRequests(int probe_length);

    public:
        Worker(GBDATA *gb_main_, const string &working_directory_, int probe_length_)
            : error(0)
            , gb_main(gb_main_)
            , terminate(false)
            , working_directory(working_directory_)
            , probe_length(probe_length_)
        {
        }

        GB_ERROR get_error() const { return error; }
        bool ok() const { return !error && !terminate; }

        void work();
    };

    void Worker::work() {
        static bool waiting = false;
        findRequests(probe_length);
        if (requests.empty()) { // no request waiting
            if (!waiting) {
                printf("[%i] waiting for requests\n", probe_length);
                waiting = true;
            }
            sleep(1);           // sleep a second
        }
        else {
            waiting = false;
            while (!error && !requests.empty()) {
                RequestPtr req     = requests.front();
                string     message = req->answer(gb_main); // answer the request

                if (message == "terminate") {
                    terminate = true;
                }
                else {
                    if (message.length()) { // if message is not empty -> interpret as error
                        error = GBS_global_string(message.c_str());
                    }
                }

                requests.pop_front(); // and remove it
            }
        }
    }

    void Worker::findRequests(int probeLength) {
        const char *dirname = working_directory.c_str();
        DIR        *dirp    = opendir(dirname);

        if (!dirp) {
            error = GBS_global_string("Directory '%s' not found", dirname);
        }
        else {
            for (struct dirent *dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
                const char    *filename     = dp->d_name;
                unsigned long  found_length = strtoul(filename, 0, 10);
                bool           is_request   = false;

                if (found_length == (unsigned long)probeLength) { // for this worker?
                    const char *ext = strrchr(filename, '.');
                    if (ext && strcmp(ext, ".req") == 0) { // is it a request?
                        is_request = true;
                    }
                }

                if (is_request) {
                    requests.push_back(new Request(string(dirname)+'/'+filename));
#if defined(VERBOSE)
                    printf("detected request '%s'\n", filename);
#endif // VERBOSE
                }
#if defined(VERBOSE)
                else {
                    if (filename[0] == '.' && (filename[1] == 0 || (filename[1] == '.' && filename[2] == 0))) {
                        ; // silently ignore . and ..
                    }
                    else {
                        printf("ignoring file '%s'\n", filename);
                    }
                }
#endif // VERBOSE
            }

            closedir(dirp);
        }
    }
}

// -------------
//      main

int main(int argc, char *argv[])
{
    printf("arb_probe_group_worker v1.0 -- (C) 2003 by Ralf Westram\n");
    srand(time(0));

    GB_ERROR error = scanArguments(argc, argv);

    if (!error) {
        const char *db_name = para.probe_db_name.c_str();

        printf("Opening probe-database '%s'...\n",db_name);
        GBDATA *gb_main = GB_open(db_name, "rh");
        if (!gb_main) {
            error             = GB_get_error();
            if (!error) error = GB_export_error("Can't open database '%s'", db_name);
        }
        else {
            error             = checkDatabaseType(gb_main, db_name, "probe_group_design_db", "complete");
            initDecodeTable();
            if (!error) error = PM_initSpeciesMaps(gb_main);

            if (!error) error = init_path_cache(gb_main);
            if (!error) error = init_group_cache(gb_main);

            if (!error) {
                Worker worker(gb_main, para.working_dir, para.probe_length);
                init_commands();

                while (worker.ok()) worker.work();
                error = worker.get_error();
            }

            // exit
            if (gb_main) {
                GB_ERROR err2     = GB_close(gb_main);
                if (!error) error = err2;
            }
        }
    }

    if (error) {
        fprintf(stderr, "Error in arb_probe_server_worker: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

