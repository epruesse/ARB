//  ==================================================================== //
//                                                                       //
//    File      : psw_main.cxx                                           //
//    Purpose   : Worker process (handles requests from cgi scripts)     //
//    Time-stamp: <Sun Sep/14/2003 19:45 MET Coder@ReallySoft.de>        //
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

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "arbdb.h"
#include "smartptr.h"

#include "../global_defs.h"
#define SKIP_SETDATABASESTATE
#include "../common.h"

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

#define MAXPATH 1024

    // Note: When changing encoding please adjust
    //       encodePath() in ./PROBE_WEB/CLIENT/TreeNode.java

    const char *encodePath(const char *path, int pathlen) {
        // path contains strings like 'LRLLRRL'
        // 'L' is interpreted as 1 ('R' as 0)

        static char buffer[MAXPATH+1];

    }
    const char *decodePath(const char *encodedPath) {
        // does the opposite of encodePath

        static char buffer[MAXPATH+1];

    }


    GB_HASH *path_cache = 0;    // links encoded "path" content with "subtree" db-pointer

    GB_ERROR init_path_cache(GBDATA *gb_main) {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_subtrees = GB_search(gb_main, "subtree_counter", GB_FIND);
        if (gb_subtrees) {
            int subtrees = GB_read_int(gb_subtrees);
            path_cache   = GBS_create_hash(subtrees, 1);

            GBDATA *gb_subtree_with_probe = GB_search(gb_main, "subtree_with_probe", GB_FIND);
            if (gb_subtree_with_probe) {
                GB_ERROR error  = 0;
                for (GBDATA *gb_subtree = GB_find(gb_subtree_with_probe, "subtree", 0, down_level);
                     gb_subtree && !error;
                     gb_subtree = GB_find(gb_subtree, "subtree", 0, this_level|search_next))
                {
                    GBDATA *gb_path = GB_search(gb_subtree, "path", GB_FIND);
                    if (!gb_path) {
                        error = "subtree w/o path (database corrupt!)";
                    }
                    else {
                        const char *path = GB_read_char_pntr(gb_path);
                        GBS_write_hash(path_cache, path, (long)gb_subtree);
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

// -----------------------
//      Known commands

namespace {
    typedef                      GB_ERROR (*Command)(GBDATA *gb_main, const Arguments& args, FILE *out);
    typedef map<string, Command> Commands;

    Commands command_list;

    GB_ERROR CMD_getprobes(GBDATA *gb_main, const Arguments& args, FILE *out) {
        //         printf("getprobes()\n");
        //         dumpArguments(args, stdout);

        const char *path;
        GB_ERROR    error = expectArgument(args, "path", path);
        GB_transaction dummy(gb_main);

        if (!error) {
            GBDATA *gb_subtree = (GBDATA*)GBS_read_hash(path_cache, path);
            if (gb_subtree) {
                GBDATA *gb_probe_group_design = GB_search(gb_subtree, "probe_group_design", GB_FIND);
                list<const char *> found_probes;

                if (gb_probe_group_design) {
                    for (GBDATA *gb_probe = GB_find(gb_probe_group_design, "common", 0, down_level);
                         gb_probe && !error;
                         gb_probe = GB_find(gb_probe, "common", 0, this_level|search_next))
                    {
                        found_probes.push_back(GB_read_char_pntr(gb_probe));
                    }
                }

                if (found_probes.empty()) {
                    error = "No probes.";
                }
                else {
                    int count = found_probes.size();
                    fprintf(out, "result=ok\nfound=%i\n", count);

                    int c = 1;
                    for (list<const char*>::iterator p = found_probes.begin(); p != found_probes.end(); ++p, ++c) {
                        fprintf(out, "probe%04i=%s\n", c, *p);
                    }
                }
            }
            else {
                error = GBS_global_string("Unknown subtree '%s'", path);
            }
        }

        return error;
    }

    void init_commands() {
        command_list["getprobes"] = CMD_getprobes;
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
            printf("deleting '%s'\n", name);
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
                                request_error = GBS_global_string("No such command '%s'", command.c_str());
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
                        if (!error) printf("result written ('%s')\n", finaloutname);
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
                printf("[waiting for requests]\n");
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

    void Worker::findRequests(int probe_length) {
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

                if (found_length == (unsigned long)probe_length) { // for this worker?
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
#ifdef VERBOSE
                else {
                    if (filename[0] == '.' && (filename[1] == 0 || (filename[1] == '.' && filename[2] == 0))) {
                        ; // silently ignore . and ..
                    }
                    else {
                        printf("ignoring file '%s'\n", filename);
                    }
                }
#endif
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
            error = checkDatabaseType(gb_main, db_name, "probe_group_design_db", "complete");
            if (!error) error = init_path_cache(gb_main);

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

