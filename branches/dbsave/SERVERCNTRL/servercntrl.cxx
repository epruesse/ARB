// ============================================================= //
//                                                               //
//   File      : servercntrl.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <servercntrl.h>

#include <client_privat.h>
#include <client.h>

#include <arbdb.h>
#include <arb_file.h>
#include <arb_sleep.h>
#include <ut_valgrinded.h>

/* The following lines go to servercntrl.h
 * edit here, not there!!
 * call 'make proto' to update
 */

// AISC_MKPT_PROMOTE:#ifndef ARBDB_BASE_H
// AISC_MKPT_PROMOTE:#include <arbdb_base.h>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:struct arb_params {
// AISC_MKPT_PROMOTE:    char *species_name;
// AISC_MKPT_PROMOTE:    char *extended_name;
// AISC_MKPT_PROMOTE:    char *alignment;
// AISC_MKPT_PROMOTE:    char *default_file;
// AISC_MKPT_PROMOTE:    char *field;
// AISC_MKPT_PROMOTE:    const char *field_default;
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:    int  read_only;
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:    char *job_server;
// AISC_MKPT_PROMOTE:    char *db_server;
// AISC_MKPT_PROMOTE:    char *mgr_server;
// AISC_MKPT_PROMOTE:    char *pt_server;
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:    char *tcp;
// AISC_MKPT_PROMOTE:};
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:enum SpawnMode {
// AISC_MKPT_PROMOTE:    WAIT_FOR_TERMINATION,
// AISC_MKPT_PROMOTE:    SPAWN_ASYNCHRONOUS,
// AISC_MKPT_PROMOTE:    SPAWN_DAEMONIZED,
// AISC_MKPT_PROMOTE:};

#define TRIES 1

static struct gl_struct {
    aisc_com   *link;
    AISC_Object  com;

    gl_struct()
        : link(0), com(0x10000*1) // faked type id (matches main object)
    { }
} glservercntrl;


inline void make_async_call(char*& command) {
    freeset(command, GBS_global_string_copy("( %s ) &", command));
}

char *createCallOnSocketHost(const char *host, const char *remotePrefix, const char *command, SpawnMode spawnmode, const char *logfile) {
    /*! transforms a shell-command
     * - use ssh if host is not local
     * - add wrappers for detached execution
     * @param host          socket specification (may be 'host:port', 'host' or ':portOrSocketfile')
     * @param remotePrefix  prefixed to command if it is executed on remote host (e.g. "$ARBHOME/bin/" -> uses value of environment variable ARBHOME on remote host!)
     * @param command       the shell command to execute
     * @param spawnmode     how to spawn:
     *      WAIT_FOR_TERMINATION = run "command"
     *      SPAWN_ASYNCHRONOUS = run "( command ) &"
     *      SPAWN_DAEMONIZED   = do not forward kill signals, remove from joblist and redirect output to logfile
     * @param logfile       use with SPAWN_DAEMONIZED (mandatory; NULL otherwise)
     * @return a SSH system call for remote hosts and direct system calls for the local machine
     */

    arb_assert((remotePrefix[0] == 0) || (strchr(remotePrefix, 0)[-1] == '/'));
    arb_assert(correlated(spawnmode == SPAWN_DAEMONIZED, logfile));

    char *call = 0;
    if (host && host[0]) {
        const char *hostPort = strchr(host, ':');
        char       *hostOnly = GB_strpartdup(host, hostPort ? hostPort-1 : 0);

        if (hostOnly[0] && !GB_host_is_local(hostOnly)) {
            char *quotedRemoteCommand = GBK_singlequote(GBS_global_string("%s%s", remotePrefix, command));
            call                      = GBS_global_string_copy("ssh %s -n %s", hostOnly, quotedRemoteCommand);
            free(quotedRemoteCommand);
        }
        free(hostOnly);
    }

    if (!call) {
        call = strdup(command);
        make_valgrinded_call(call); // only on local host
    }

    switch (spawnmode) {
        case WAIT_FOR_TERMINATION:
            break;

        case SPAWN_ASYNCHRONOUS:
            make_async_call(call);
            break;

        case SPAWN_DAEMONIZED: {
            char *quotedLogfile         = GBK_singlequote(logfile);
            char *quotedDaemonizingCall = GBK_singlequote(GBS_global_string("nohup %s &>>%s & disown", call, quotedLogfile));

            freeset(call, GBS_global_string_copy("bash -c %s", quotedDaemonizingCall));

            free(quotedDaemonizingCall);
            free(quotedLogfile);
            break;
        }
    }

    return call;
}

GB_ERROR arb_start_server(const char *arb_tcp_env, int do_sleep)
{
    const char *tcp_id;
    GB_ERROR    error = 0;

    if (!(tcp_id = GBS_read_arb_tcp(arb_tcp_env))) {
        error = GB_await_error();
    }
    else {
        const char *server       = strchr(tcp_id, 0) + 1;
        char       *serverparams = 0;

        /* concatenate all params behind server
           Note :  changed behavior on 2007/Mar/09 -- ralf
           serverparams now is one space if nothing defined in arb_tcp.dat
           (previously was same as 'server' - most likely a bug)
        */
        {
            const char *param  = strchr(server, 0)+1;
            size_t      plen   = strlen(param);
            size_t      alllen = 0;

            while (plen) {
                param  += plen+1;
                alllen += plen+1;
                plen    = strlen(param);
            }

            serverparams = (char*)malloc(alllen+1);
            {
                char *sp = serverparams;

                param = strchr(server, 0)+1;
                plen  = strlen(param);
                if (!plen) sp++;
                else do {
                    memcpy(sp, param, plen);
                    sp[plen]  = ' ';
                    sp       += plen+1;
                    param    += plen+1;
                    plen      = strlen(param);
                } while (plen);
                sp[-1] = 0;
            }
        }

        {
            char       *command = 0;
            const char *port    = strchr(tcp_id, ':');

            if (!port) {
                error = GB_export_errorf("Error: Missing ':' in socket definition of '%s' in file $(ARBHOME)/lib/arb_tcp.dat", arb_tcp_env);
            }
            else {
                // When arb is called from arb_launcher, ARB_SERVER_LOG gets set to the name of a logfile.
                // If ARB_SERVER_LOG is set here -> start servers daemonized here (see #492 for motivation)

                const char *serverlog    = GB_getenv("ARB_SERVER_LOG");
                SpawnMode   spawnmode    = serverlog ? SPAWN_DAEMONIZED : SPAWN_ASYNCHRONOUS;
                char       *plainCommand = GBS_global_string_copy("%s %s '-T%s'", server, serverparams, port); // Note: quotes around -T added for testing only (remove@will)
                command                  = createCallOnSocketHost(tcp_id, "$ARBHOME/bin/", plainCommand, spawnmode, serverlog);
                free(plainCommand);
            }

            if (!error) {
                error = GBK_system(command);
                if (do_sleep) GB_sleep(5, SEC);
            }
            free(command);
        }
        free(serverparams);
    }
    return error;
}

static GB_ERROR arb_wait_for_server(const char *arb_tcp_env, const char *tcp_id, int magic_number, struct gl_struct *serverctrl, int wait) {
    GB_ERROR error   = NULL;
    serverctrl->link = aisc_open(tcp_id, serverctrl->com, magic_number, &error);

    if (!error && !serverctrl->link) { // no server running -> start one
        error = arb_start_server(arb_tcp_env, 0);
        while (!error && !serverctrl->link && wait) {
            GB_sleep(1, SEC);
            wait--;
            if ((wait%10) == 0 && wait>0) {
                printf("Waiting for server '%s' to come up (%i seconds left)\n", arb_tcp_env, wait);
            }
            serverctrl->link  = aisc_open(tcp_id, serverctrl->com, magic_number, &error);
        }
    }

    return error;
}

GB_ERROR arb_look_and_start_server(long magic_number, const char *arb_tcp_env) {
    arb_assert(!GB_have_error());

    GB_ERROR    error       = 0;
    const char *tcp_id      = GBS_read_arb_tcp(arb_tcp_env);
    const char *arb_tcp_dat = "$(ARBHOME)/lib/arb_tcp.dat";

    if (!tcp_id) {
        error = GBS_global_string("Entry '%s' not found in %s (%s)", arb_tcp_env, arb_tcp_dat, GB_await_error());
    }
    else {
        const char *file = GBS_scan_arb_tcp_param(tcp_id, "-d"); // find parameter behind '-d'

        if (!file) {
            error = GBS_global_string("Parameter -d missing for entry '%s' in %s", arb_tcp_env, arb_tcp_dat);
        }
        else {
            if (strcmp(file, "!ASSUME_RUNNING") == 0) {
                // assume pt-server is running on a host,  w/o access to common network drive
                // i.e. we cannot check for the existence of the database file
            }
            else if (GB_size_of_file(file) <= 0) {
                if (strncmp(arb_tcp_env, "ARB_NAME_SERVER", 15) == 0) {
                    char *dir       = strdup(file);
                    char *lastSlash = strrchr(dir, '/');

                    if (lastSlash) {
                        lastSlash[0]         = 0; // cut off file
                        {
                            const char *copy_cmd = GBS_global_string("cp %s/names.dat.template %s", dir, file);
                            error                = GBK_system(copy_cmd);
                        }
                        if (!error && GB_size_of_file(file) <= 0) {
                            error = GBS_global_string("Cannot copy nameserver template (%s/names.dat.template missing?)", dir);
                        }
                    }
                    else {
                        error = GBS_global_string("Can't determine directory from '%s'", dir);
                    }
                    free(dir);
                }
                else if (strncmp(arb_tcp_env, "ARB_PT_SERVER", 13) == 0) {
                    const char *nameOnly    = strrchr(file, '/');
                    if (!nameOnly) nameOnly = file;

                    error = GBS_global_string("PT_server '%s' has not been created yet.\n"
                                              " To create it follow these steps:\n"
                                              " 1. Start ARB on the whole database you want to use for probe match/design\n"
                                              " 2. Go to ARB_NTREE/Probes/PT_SERVER Admin\n"
                                              " 3. Select '%s' and press BUILD SERVER\n"
                                              " 4. Wait (up to hours, depending on your DB size)\n"
                                              " 5. Meanwhile read the help file: PT_SERVER: What Why and How",
                                              file, nameOnly);
                }
                else {
                    error = GBS_global_string("The file '%s' is missing. \nUnable to start %s", file, arb_tcp_env);
                }
            }
        }

        if (!error) {
            error = arb_wait_for_server(arb_tcp_env, tcp_id, magic_number, &glservercntrl, 20);

            if (!error) {
                if (!glservercntrl.link) { // couldn't start server
                    error =                                                            // |
                        "ARB has problems to start a server! Possible reasons may be one\n"
                        "or several of the following list:\n"
                        "- the tcp_id (socket number) is already used by another program\n"
                        "  (doesnt apply to user-specific PTSERVERs; check $ARBHOME/lib/arb_tcp.dat versus /etc/services)\n"
                        "- the server exited with error or has crashed.\n"
                        "  In case of PTSERVER, the failure might be caused by:\n"
                        "  - missing database in $ARBHOME/lib/pts/* (solution: update ptserver database)\n"
                        "  - wrong permissions of $ARBHOME/lib/pts/* (no read access)\n"
                        "  If you recently installed a new arb version, arb will continue\n"
                        "  to use your previous 'arb_tcp.dat', which might be out-of-date.\n"
                        "  Backup and remove it, then restart ARB. If it works now,\n"
                        "  compare your old 'arb_tcp.dat' with the new one for changes.\n"
                        "- When using remote servers: login or network problems\n"
                        ;
                }
                else {
                    aisc_close(glservercntrl.link, glservercntrl.com);
                    glservercntrl.link = 0;
                }
            }
        }
    }

    arb_assert(!GB_have_error());
    return error;
}

GB_ERROR arb_look_and_kill_server(int magic_number, const char *arb_tcp_env) {
    const char *tcp_id;
    GB_ERROR    error = 0;

    if (!(tcp_id = GBS_read_arb_tcp(arb_tcp_env))) {
        error = GB_await_error();
    }
    else {
        const char *server = strchr(tcp_id, 0)+1;

        glservercntrl.link = aisc_open(tcp_id, glservercntrl.com, magic_number, &error);
        if (glservercntrl.link) {
            aisc_close(glservercntrl.link, glservercntrl.com);
            glservercntrl.link = 0;

            const char *command = GBS_global_string("%s -kill -T%s &", server, tcp_id);
            if (system(command) != 0) {
                error = GBS_global_string("Failed to execute '%s'", command);
            }
        }
        else {
            error = "Server is not running";
        }
    }
    return error;
}

void arb_print_server_params() {
    printf("General server parameters (some maybe unused by this server):\n"
           "    -s<name>        sets species name to '<name>'\n"
           "    -e<name>        sets extended name to '<name>'\n"
           "    -a<ali>         sets alignment to '<ali>'\n"
           "    -d<file>        sets default file to '<file>'\n"
           "    -f<field>=<def> sets DB field to '<field>' (using <def> as default)\n"
           "    -r              read-only mode\n"
           "    -D<server>      sets DB-server to '<server>'  [default = ':']\n"
           "    -J<server>      sets job-server to '<server>' [default = 'ARB_JOB_SERVER']\n"
           "    -M<server>      sets MGR-server to '<server>' [default = 'ARB_MGR_SERVER']\n"
           "    -P<server>      sets PT-server to '<server>'  [default = 'ARB_PT_SERVER']\n"
           "    -T<[host]:port>   sets TCP connection to '<[host]:port>'\n"
           );
}

arb_params *arb_trace_argv(int *argc, const char **argv)
{
    int s, d;

    arb_params *erg = (arb_params *)calloc(sizeof(arb_params), 1);
    erg->db_server  = strdup(":");
    erg->job_server = strdup("ARB_JOB_SERVER");
    erg->mgr_server = strdup("ARB_MGR_SERVER");
    erg->pt_server  = strdup("ARB_PT_SERVER");

    for (s=d=0; s<*argc; s++) {
        if (argv[s][0] == '-') {
            switch (argv[s][1]) {
                case 's': erg->species_name  = strdup(argv[s]+2); break;
                case 'e': erg->extended_name = strdup(argv[s]+2); break;
                case 'a': erg->alignment     = strdup(argv[s]+2); break;
                case 'd': erg->default_file  = strdup(argv[s]+2); break;
                case 'f': {
                    char *eq;
                    erg->field = strdup(argv[s]+2);

                    eq = strchr(erg->field, '=');
                    if (eq) {
                        erg->field_default = eq+1;
                        eq[0]              = 0;
                    }
                    else {
                        erg->field_default = 0; // this is illegal - error handling done in caller
                    }
                    break;
                }
                case 'r': erg->read_only     = 1; break;
                case 'J': freedup(erg->job_server, argv[s]+2); break;
                case 'D': freedup(erg->db_server, argv[s]+2); break;
                case 'M': freedup(erg->mgr_server, argv[s]+2); break;
                case 'P': freedup(erg->pt_server, argv[s]+2); break;
                case 'T': {
                    const char *ipport = argv[s]+2;
                    if (ipport[0] == ':' &&
                        ipport[1] >= '0' && ipport[1] <= '9') { // port only -> assume localhost
                        erg->tcp = GBS_global_string_copy("localhost%s", ipport);
                    }
                    else {
                        erg->tcp = strdup(ipport);
                    }
                    break;
                }
                default:
                    argv[d++] = argv[s];
                    break;
            }
        }
        else {
            argv[d++] = argv[s];
        }
    }
    *argc = d;
    return erg;
}

void free_arb_params(arb_params *params) {
    free(params->species_name);
    free(params->extended_name);
    free(params->alignment);
    free(params->default_file);
    free(params->field);
    free(params->job_server);
    free(params->db_server);
    free(params->mgr_server);
    free(params->pt_server);
    free(params->tcp);

    free(params);
}

// --------------------------------------------------------------------------------

#if defined(UNIT_TESTS) 

// If you need tests in AISC_COM/C/client.c, put them here instead.

#include <test_unit.h>

void TEST_servercntrl() {
    // TEST_EXPECT(0);
}

#endif // UNIT_TESTS

