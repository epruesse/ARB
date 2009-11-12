#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
/* #include <malloc.h> */
#include <client_privat.h>
#include <client.h>
#include <arbdb.h>
#include <servercntrl.h>

/*
 * The following lines go to servercntrl.h
 * edit here, not there!!
 * call 'make proto' to update
 */

/* AISC_MKPT_PROMOTE:struct arb_params {*/
/* AISC_MKPT_PROMOTE:    char *species_name;*/
/* AISC_MKPT_PROMOTE:    char *extended_name;*/
/* AISC_MKPT_PROMOTE:    char *alignment;*/
/* AISC_MKPT_PROMOTE:    char *default_file;*/
/* AISC_MKPT_PROMOTE:    char *field;*/
/* AISC_MKPT_PROMOTE:    const char *field_default;*/
/* AISC_MKPT_PROMOTE:*/
/* AISC_MKPT_PROMOTE:    int  read_only;*/
/* AISC_MKPT_PROMOTE:*/
/* AISC_MKPT_PROMOTE:    char *job_server;*/
/* AISC_MKPT_PROMOTE:    char *db_server;*/
/* AISC_MKPT_PROMOTE:    char *mgr_server;*/
/* AISC_MKPT_PROMOTE:    char *pt_server;*/
/* AISC_MKPT_PROMOTE:*/
/* AISC_MKPT_PROMOTE:    char *tcp;*/
/* AISC_MKPT_PROMOTE:};*/

#define TRIES 1

struct gl_struct {
    aisc_com *link;
    long      locs;
    long      com;
} glservercntrl;


char *prefixSSH(const char *host, const char *command, int async) {
    /* 'host' is a hostname or 'hostname:port' (where hostname may be an IP)
       'command' is the command to be executed
       if 'async' is 1 -> append '&'

       returns a SSH system call for foreign host or
       a direct system call for the local machine
    */

    char *result    = 0;
    char  asyncChar = " &"[!!async];

    if (host && host[0]) {
        const char *hostPort = strchr(host, ':');
        char       *hostOnly = GB_strpartdup(host, hostPort ? hostPort-1 : 0);

        if (!GB_host_is_local(hostOnly)) {
            result = GBS_global_string_copy("ssh %s -n '%s' %c", hostOnly, command, asyncChar);
        }
        free(hostOnly);
    }

    if (!result) {
        result = GBS_global_string_copy("(%s) %c", command, asyncChar);
    }

    return result;
}

GB_ERROR arb_start_server(const char *arb_tcp_env, GBDATA *gbmain, int do_sleep)
{
    const char *tcp_id;
    GB_ERROR error = 0;

    if (!(tcp_id = GBS_read_arb_tcp(arb_tcp_env))) {
        error = GB_export_errorf("Entry '%s' in $(ARBHOME)/lib/arb_tcp.dat not found", arb_tcp_env);
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
            char *command = 0;
            int   delay   = 5;

            if (*tcp_id == ':') { /* local mode */
                command = GBS_global_string_copy("%s %s -T%s &",server, serverparams, tcp_id);
            }
            else {
                const char *port = strchr(tcp_id, ':');

                if (!port) {
                    error = GB_export_errorf("Error: Missing ':' in line '%s' file $(ARBHOME)/lib/arb_tcp.dat", arb_tcp_env);
                }
                else {
                    char *remoteCommand = GBS_global_string_copy("$ARBHOME/bin/%s %s -T%s", server, serverparams, port);

                    command = prefixSSH(tcp_id, remoteCommand, 1);
                    free(remoteCommand);
                }
            }

            if (!error) {
#if defined(DEBUG)
                printf("Starting server (cmd='%s')\n", command);
#endif /* DEBUG */
                if (!gbmain || GBCMC_system(gbmain,command)) system(command);
                if (do_sleep) sleep(delay);
            }
            free(command);
        }
        free(serverparams);
    }
    return error;
}

static GB_ERROR arb_wait_for_server(const char *arb_tcp_env, GBDATA *gbmain, const char *tcp_id, int magic_number, struct gl_struct *serverctrl, int wait) {
    serverctrl->link = aisc_open(tcp_id, &(serverctrl->com), magic_number);
    if (!serverctrl->link) { // no server running -> start one
        GB_ERROR error = arb_start_server(arb_tcp_env, gbmain, 0);
        if (error) return error;

        while (!serverctrl->link && wait) {
            sleep(1);
            wait--;
            if ((wait%10) == 0 && wait>0) {
                printf("Waiting for server '%s' to come up (%i seconds left)\n", arb_tcp_env, wait);
            }
            serverctrl->link  = aisc_open(tcp_id, &(serverctrl->com), magic_number);
        }
    }

    return 0;
}

GB_ERROR arb_look_and_start_server(long magic_number, const char *arb_tcp_env, GBDATA *gbmain) {
    GB_ERROR    error       = 0;
    const char *tcp_id      = GBS_read_arb_tcp(arb_tcp_env);
    const char *arb_tcp_dat = "$(ARBHOME)/lib/arb_tcp.dat";

    if (!tcp_id) {
        error = GBS_global_string("Entry '%s' not found in %s", arb_tcp_env, arb_tcp_dat);
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
                            system(copy_cmd);
                        }
                        if (GB_size_of_file(file) <= 0) {
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
            error = arb_wait_for_server(arb_tcp_env, gbmain, tcp_id, magic_number, &glservercntrl, 20);

            if (!error) {
                if (!glservercntrl.link) { // couldn't start server
                    error =
                        "I got some problems to start your server:\n"
                        "   Possible Reasons may be one or more of the following list:\n"
                        "   - there is no database in $ARBHOME/lib/pts/*\n"
                        "     update server <ARB_NTREE/Probes/PT_SERVER Admin/BUILD SERVER>\n"
                        "   - you are not allowed to run 'ssh host pt_server ....&'\n"
                        "     check file '/etc/hosts.equiv' (read man pages for help)\n"
                        "   - the permissions of $ARBHOME/lib/pts/* do not allow read access\n"
                        "   - the PT_SERVER host is not up\n"
                        "   - the tcp_id is already used by another program\n"
                        "     check $ARBHOME/lib/arb_tcp.dat and /etc/services\n";
                }
                else {
                    aisc_close(glservercntrl.link);
                    glservercntrl.link = 0;
                }
            }
        }
    }

    return error;
}

GB_ERROR arb_look_and_kill_server(int magic_number, const char *arb_tcp_env)
{
    const char *tcp_id;
    GB_ERROR    error = 0;

    if (!(tcp_id = GBS_read_arb_tcp(arb_tcp_env))) {
        error = GB_export_errorf("Missing line '%s' in $(ARBHOME)/lib/arb_tcp.dat:",arb_tcp_env);
    }
    else {
        const char *server = strchr(tcp_id, 0)+1;

        glservercntrl.link = (aisc_com *) aisc_open(tcp_id, &glservercntrl.com, magic_number);
        if (glservercntrl.link) {
            aisc_close(glservercntrl.link);
            glservercntrl.link = 0;

            const char *command = GBS_global_string("%s -kill -T%s &", server, tcp_id);
            /* sprintf(command, "%s -kill -T%s &", server, tcp_id); */
            if (system(command)) {
                error = GB_export_errorf("Cannot execute '%s'",command);
            }
        }
        else {
            error= GB_export_error("I cannot kill your server because I cannot find it");
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

struct arb_params *arb_trace_argv(int *argc, char **argv)
{
    struct arb_params *erg;
    int s,d;

    erg             = (struct arb_params *)calloc(sizeof(struct arb_params),1);
    erg->db_server  = strdup(":");
    erg->job_server = strdup("ARB_JOB_SERVER");
    erg->mgr_server = strdup("ARB_MGR_SERVER");
    erg->pt_server  = strdup("ARB_PT_SERVER");

    for (s=d=0; s<*argc; s++) {
        if (argv[s][0] == '-') {
            switch (argv[s][1]) {
                case 's': erg->species_name  = strdup(argv[s]+2);break;
                case 'e': erg->extended_name = strdup(argv[s]+2);break;
                case 'a': erg->alignment     = strdup(argv[s]+2);break;
                case 'd': erg->default_file  = strdup(argv[s]+2);break;
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
                case 'r': erg->read_only     = 1;break;
                case 'J': erg->job_server    = strdup(argv[s]+2);break;
                case 'D': erg->db_server     = strdup(argv[s]+2);break;
                case 'M': erg->mgr_server    = strdup(argv[s]+2);break;
                case 'P': erg->pt_server     = strdup(argv[s]+2);break;
                case 'T': {
                    char *ipport = argv[s]+2;
                    if (ipport[0] == ':' &&
                        ipport[1] >= '0' && ipport[1] <= '9') { /* port only -> assume localhost */
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

void free_arb_params(struct arb_params *params) {
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
