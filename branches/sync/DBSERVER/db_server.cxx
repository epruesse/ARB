// ============================================================= //
//                                                               //
//   File      : db_server.cxx                                   //
//   Purpose   : CL ARB database server                          //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <arbdb.h>
#include <ad_cb.h>
#include <servercntrl.h>
#include <ut_valgrinded.h>
#include <arb_file.h>
#include <arb_sleep.h>
#include <arb_diff.h>

#define TIMEOUT 1000*60*2       // save every 2 minutes
#define LOOPS   30              // wait 30*TIMEOUT (1 hour) till shutdown

inline GBDATA *dbserver_container(GBDATA *gb_main) {
    return GB_search(gb_main, "/tmp/dbserver", GB_CREATE_CONTAINER);
}
inline GBDATA *dbserver_entry(GBDATA *gb_main, const char *entry) {
    GBDATA *gb_entry = GB_searchOrCreate_string(dbserver_container(gb_main), entry, "<undefined>");
#if defined(DEBUG)
    if (gb_entry) {
        GBDATA *gb_brother = GB_nextEntry(gb_entry);
        arb_assert(!gb_brother);
    }
#endif
    return gb_entry;
}

inline GBDATA *get_command_entry(GBDATA *gb_main) { return dbserver_entry(gb_main, "cmd"); }
inline GBDATA *get_param_entry(GBDATA *gb_main) { return dbserver_entry(gb_main, "param"); }
inline GBDATA *get_result_entry(GBDATA *gb_main) { return dbserver_entry(gb_main, "result"); }

inline GB_ERROR init_data(GBDATA *gb_main) {
    GB_ERROR error = NULL;

    if (!get_command_entry(gb_main) || !get_param_entry(gb_main) || !get_result_entry(gb_main)) {
        error = GB_await_error();
    }
    
    return error;
}

inline bool served(GBDATA *gb_main) {
    GB_begin_transaction(gb_main);
    GB_commit_transaction(gb_main);
    return GBCMS_accept_calls(gb_main, false);
}

static bool do_shutdown       = false;
static bool command_triggered = false;

static const char *savemode = "b";

static void command_cb() {
    command_triggered = true;
}

static void react_to_command(GBDATA *gb_main) {
    static bool in_reaction = false;

    if (!in_reaction) {
        LocallyModify<bool> flag(in_reaction, true);

        command_triggered = false;

        GB_ERROR  error        = NULL;

        GB_begin_transaction(gb_main);
        GBDATA   *gb_cmd_entry = get_command_entry(gb_main);
        GB_commit_transaction(gb_main);

        char *command;
        {
            GB_transaction ta(gb_main);

            command = GB_read_string(gb_cmd_entry);
            if (command[0]) {
                error             = GB_write_string(gb_cmd_entry, "");
                if (!error) error = GB_write_string(get_result_entry(gb_main), "busy");
            }
        }

        if (command[0]) {
            if      (strcmp(command, "shutdown") == 0) do_shutdown = true;
            else if (strcmp(command, "ping")     == 0) {} // ping is a noop
            else if (strcmp(command, "save")     == 0) {
                fprintf(stdout, "arb_db_server: save requested (savemode is \"%s\")\n", savemode);

                char *param = NULL;
                {
                    GB_transaction  ta(gb_main);
                    GBDATA         *gb_param = get_param_entry(gb_main);
                    if (!gb_param) error     = GB_await_error();
                    else {
                        param             = GB_read_string(gb_param);
                        if (!param) error = GB_await_error();
                        else error        = GB_write_string(gb_param, "");
                    }
                }

                if (!error) {
                    if (param[0]) error = GB_save(gb_main, param, savemode); // @@@ support compression here?
                    else error          = "No savename specified";
                }

                printf("arb_db_server: save returns '%s'\n", error);

                free(param);
            }
            else {
                error = GBS_global_string("Unknown command '%s'", command);
            }
        }
        {
            GB_transaction ta(gb_main);

            GB_ERROR err2             = GB_write_string(get_result_entry(gb_main), error ? error : "ok");
            if (!error && err2) error = GBS_global_string("could not write result (reason: %s)", err2);
            if (error) fprintf(stderr, "arb_db_server: failed to react to command '%s' (reason: %s)\n", command, error);
        }
        free(command);
    }
}

static GB_ERROR server_main_loop(GBDATA *gb_main) {
    GB_ERROR error = NULL;
    {
        GB_transaction ta(gb_main);

        GBDATA *cmd_entry = get_command_entry(gb_main);
        error             = GB_add_callback(cmd_entry, GB_CB_CHANGED, makeDatabaseCallback(command_cb));
    }

    while (!error) {
        served(gb_main);
        if (command_triggered) {
            react_to_command(gb_main);
        }
        if (do_shutdown) {
            int clients;
            do {
                clients = GB_read_clients(gb_main);
                if (clients>0) {
                    fprintf(stdout, "arb_db_server: shutdown requested (waiting for %i clients)\n", clients);
                    served(gb_main);
                }
            }
            while (clients>0);
            fputs("arb_db_server: all clients left, performing shutdown\n", stdout);
            break;
        }
    }
    return error;
}

static GB_ERROR check_socket_available(const arb_params& params) {
    GBDATA *gb_extern = GB_open(params.tcp, "rwc");
    if (gb_extern) {
        GB_close(gb_extern);
        return GBS_global_string("socket '%s' already in use", params.tcp);
    }
    GB_clear_error();
    return NULL;
}

static GB_ERROR run_server(const arb_params& params) {
    GB_shell shell;
    GB_ERROR error = check_socket_available(params);
    if (!error) {
        printf("Loading '%s'...\n", params.default_file);
        GBDATA *gb_main     = GB_open(params.default_file, "rw");
        if (!gb_main) error = GBS_global_string("Can't open DB '%s' (reason: %s)", params.default_file, GB_await_error());
        else {
            error = GBCMS_open(params.tcp, TIMEOUT, gb_main);
            if (error) error = GBS_global_string("Error starting server: %s", error);
        }

        if (!error) {
            GB_transaction ta(gb_main);
            error = init_data(gb_main);
        }
        if (!error) {
            error = server_main_loop(gb_main);
            fprintf(stderr, "server_main_loop returns with '%s'\n", error);
        }

        if (gb_main) {
            GBCMS_shutdown(gb_main);
            GB_close(gb_main);
        }
    }
    return error;
}

static GB_ERROR run_command(const arb_params& params, const char *command) {
    GB_ERROR  error     = NULL;
    GB_shell  shell;
    GBDATA   *gb_main   = GB_open(params.tcp, "rw");
    if (!gb_main) error = GB_await_error();
    else {
        {
            GB_transaction ta(gb_main);

            GBDATA *gb_cmd_entry = get_command_entry(gb_main);
            error                = gb_cmd_entry ? GB_write_string(gb_cmd_entry, command) : GB_await_error();

            if (!error) {
                if (strcmp(command, "save") == 0) {
                    GBDATA *gb_param     = get_param_entry(gb_main);
                    if (!gb_param) error = GB_await_error();
                    else    error        = GB_write_string(gb_param, params.default_file); // send save-name
                }
            }
        }

        if (!error) {
            GB_transaction ta(gb_main);

            GBDATA *gb_result_entry     = get_result_entry(gb_main);
            if (!gb_result_entry) error = GB_await_error();
            else {
                const char *result = GB_read_char_pntr(gb_result_entry);

                if (strcmp(result, "ok") != 0) {
                    error = GBS_global_string("Error from server: %s", result);
                }
            }
        }

        GB_close(gb_main);
    }
    return error;
}

static void show_help() {
    fputs("arb_db_server 2.0 -- ARB-database server\n", stdout);
    fputs("options:\n", stdout);
    fputs("    -h         show this help\n", stdout);
    fputs("    -A         use ASCII-DB-version\n", stdout);
    fputs("    -Ccmd      execute command 'cmd' on running server\n", stdout);
    fputs("               known command are:\n", stdout);
    fputs("                   ping      test if server is up (crash or failure if not)\n", stdout);
    fputs("                   save      save the database (use -d to change name)\n", stdout);
    fputs("                   shutdown  shutdown running arb_db_server\n", stdout);
    arb_print_server_params();
}

int ARB_main(int argc, char *argv[]) {
    arb_params *params = arb_trace_argv(&argc, (const char **)argv);

    bool        help  = false;
    const char *cmd   = NULL;  // run server command

    GB_ERROR error = NULL;
    while (argc>1 && !error) {
        const char *arg = argv[1];
        if (arg[0] == '-') {
            char sw_char = arg[1];
            switch (sw_char) {
                case 'h': help     = true; break;
                case 'C': cmd      = arg+2; break;
                case 'A': savemode = "a"; break;

                default:
                    error = GBS_global_string("Unknown switch '-%c'", sw_char);
                    break;
            }
        }
        else {
            error = GBS_global_string("Unknown argument '%s'", arg);
        }
        argc--; argv++;
    }

    if (!error) {
        if (help) show_help();
        else if (cmd) error = run_command(*params, cmd);
        else          error = run_server(*params);
    }

    free_arb_params(params);
    if (error) {
        fprintf(stderr, "Error in arb_db_server: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include <unistd.h>
#include <sys/wait.h>

inline GB_ERROR valgrinded_system(const char *cmdline) {
    char *cmddup = ARB_strdup(cmdline);
    make_valgrinded_call(cmddup);

    GB_ERROR error = GBK_system(cmddup);
    free(cmddup);
    return error;
}

#define RUN_TOOL(cmdline)            valgrinded_system(cmdline)
#define TEST_RUN_TOOL(cmdline)       TEST_EXPECT_NO_ERROR(RUN_TOOL(cmdline))
#define TEST_RUN_TOOL_FAILS(cmdline) TEST_EXPECT_ERROR_CONTAINS(RUN_TOOL(cmdline), "System call failed")

inline bool server_is_down(const char *tcp) {
    char     *ping_cmd = ARB_strdup(GBS_global_string("arb_db_server -T%s -Cping", tcp));
    GB_ERROR  error    = GBK_system(ping_cmd); // causes a crash in called command (as long as server is not up)
    free(ping_cmd);
    return error;
}

static int entry_changed_cb_called = 0;
static void entry_changed_cb() {
    entry_changed_cb_called++;
}

void TEST_SLOW_dbserver() {
    // MISSING_TEST(TEST_dbserver);
    TEST_RUN_TOOL("arb_db_server -h");
    TEST_RUN_TOOL_FAILS("arb_db_server -X");
    TEST_RUN_TOOL_FAILS("arb_db_server brzl");

    char *sock = ARB_strdup(GB_path_in_ARBHOME("UNIT_TESTER/sockets/dbserver.socket"));
    char *tcp  = GBS_global_string_copy(":%s", sock);
    char *db   = ARB_strdup(GB_path_in_ARBHOME("UNIT_TESTER/run/TEST_loadsave.arb"));

    char *shutdown_cmd = ARB_strdup(GBS_global_string("arb_db_server -T%s -Cshutdown", tcp));

// #define DEBUG_SERVER // uncomment when debugging server manually

#if !defined(DEBUG_SERVER)
    TEST_EXPECT(server_is_down(tcp));
#endif

    pid_t child_pid = fork();
    if (child_pid) { // parent ("the client")
        bool down = true;
        int max_wait = (60*1000)/25; // set timeout to ~60 seconds
        while (down) {
            GB_sleep(25, MS);
            down = server_is_down(tcp);
            TEST_EXPECT(max_wait-->0);
        }
        // ok - server is up

        {
            char *bad_cmd = ARB_strdup(GBS_global_string("arb_db_server -T%s -Cbad", tcp));
            TEST_RUN_TOOL_FAILS(bad_cmd);
            free(bad_cmd);
        }

        { // now directly connect to server
            GB_shell  shell;
            GB_ERROR  error      = NULL;
            GBDATA   *gb_main1   = GB_open(tcp, "rw"); // first client
            if (!gb_main1) error = GB_await_error();
            else {
                GBDATA *gb_main2     = GB_open(tcp, "rw"); // second client
                if (!gb_main2) error = GB_await_error();
                else {
                    // test existing entries
                    {
                        GB_transaction  ta(gb_main1);
                        GBDATA         *gb_ecoli = GB_search(gb_main1, "/extended_data/extended/ali_16s/data", GB_FIND);

                        TEST_REJECT_NULL(gb_ecoli);

                        const char *ecoli = GB_read_char_pntr(gb_ecoli);
                        if (!ecoli) error = GB_await_error();
                        else        TEST_EXPECT_EQUAL(GBS_checksum(ecoli, 0, NULL), 0x3558760cU);
                    }
                    {
                        GB_transaction  ta(gb_main2);
                        GBDATA         *gb_alitype = GB_search(gb_main2, "/presets/alignment/alignment_type", GB_FIND);

                        TEST_REJECT_NULL(gb_alitype);

                        const char *alitype = GB_read_char_pntr(gb_alitype);
                        if (!alitype) error = GB_await_error();
                        else        TEST_EXPECT_EQUAL(alitype, "rna");
                    }

                    // test value written in client1 is changed in client2
                    const char *test_entry   = "/tmp/TEST_SLOW_dbserver";
                    const char *test_content = "hello world!";

                    GBDATA *gb_entry1;
                    GBDATA *gb_entry2;

                    if (!error) {
                        GB_transaction ta(gb_main1);

                        gb_entry1             = GB_search(gb_main1, test_entry, GB_STRING);
                        if (!gb_entry1) error = GB_await_error();
                        else error            = GB_write_string(gb_entry1, test_content);
                    }

                    if (!error) { // test value arrived in other client
                        GB_transaction ta(gb_main2);

                        gb_entry2             = GB_search(gb_main2, test_entry, GB_STRING);
                        if (!gb_entry2) error = GB_await_error();
                        else {
                            const char *read_content = GB_read_char_pntr(gb_entry2);
                            if (!read_content) {
                                error = GB_await_error();
                            }
                            else {
                                TEST_EXPECT_EQUAL(read_content, test_content);
                            }
                        }
                    }

                    // test change-callback gets triggered
                    if (!error) {

                        TEST_EXPECT_EQUAL(entry_changed_cb_called, 0);
                        {
                            GB_transaction ta(gb_entry1);
                            error = GB_add_callback(gb_entry1, GB_CB_CHANGED, makeDatabaseCallback(entry_changed_cb));
                        }
                        TEST_EXPECT_EQUAL(entry_changed_cb_called, 0);
                        {
                            GB_transaction ta(gb_entry2);
                            GB_touch(gb_entry2);
                        }
                        TEST_EXPECT_EQUAL(entry_changed_cb_called, 0); // client1 did not transact yet
                        delete new GB_transaction(gb_main1);
                        TEST_EXPECT_EQUAL(entry_changed_cb_called, 1);
                    }

                    GB_close(gb_main2);
                }
                GB_close(gb_main1);
            }

            TEST_EXPECT_NO_ERROR(error);
        }

        // test remote save
        {
            char *savename = ARB_strdup(GB_path_in_ARBHOME("UNIT_TESTER/run/TEST_arbdbserver_save.arb"));
            char *expected = ARB_strdup(GB_path_in_ARBHOME("UNIT_TESTER/run/TEST_arbdbserver_save_expected.arb"));
            char *save_cmd = ARB_strdup(GBS_global_string("arb_db_server -T%s -Csave -d%s", tcp, savename));
            char *bad_savecmd = ARB_strdup(GBS_global_string("arb_db_server -T%s -Csave", tcp));

            TEST_RUN_TOOL(save_cmd);
            TEST_EXPECT(GB_is_regularfile(savename));

// #define TEST_AUTO_UPDATE
#if defined(TEST_AUTO_UPDATE)
            TEST_COPY_FILE(savename, expected);
#else // !defined(TEST_AUTO_UPDATE)
            TEST_EXPECT_FILES_EQUAL(savename, expected);
#endif
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(savename));

            TEST_RUN_TOOL_FAILS(bad_savecmd);
            
            free(bad_savecmd);
            free(save_cmd);
            free(expected);
            free(savename);
        }

        // stop the server
        TEST_RUN_TOOL(shutdown_cmd);
        while (child_pid != wait(NULL)) {} // wait for child to finish = wait for server to terminate
    }
    else { // child ("the server")
#if !defined(DEBUG_SERVER)
        GB_sleep(100, MS);
        TEST_RUN_TOOL(GBS_global_string("arb_db_server -T%s -d%s -A", tcp, db)); // start the server (in ASCII-mode)
#endif
        exit(EXIT_SUCCESS);
    }

    TEST_EXPECT(server_is_down(tcp));
    
#define socket_disappeared(sock) (GB_time_of_file(sock) == 0)

    TEST_EXPECT(socket_disappeared(sock));

    free(shutdown_cmd);
    free(db);
    free(tcp);
    free(sock);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
