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
#define TRIES 1

struct gl_struct {
    aisc_com       *link;
    long             locs;
    long             com;
}               glservercntrl;

GB_ERROR arb_start_server(const char *arb_tcp_env, GBDATA *gbmain, int do_sleep)
{
    const char *tcp_id;
    GB_ERROR error = 0;

    if (!(tcp_id = GBS_read_arb_tcp(arb_tcp_env))) {
        error = GB_export_error("Entry '%s' in $(ARBHOME)/lib/arb_tcp.dat not found", arb_tcp_env);
    }
    else {
        const char *server       = strchr(tcp_id, 0) + 1;
        char       *serverparams = 0;
        char       *command      = 0;

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

            serverparams    = (char*)malloc(alllen+1);
            serverparams[0] = 0; /* needed if no param exists */
            {
                char *sp = serverparams;

                param = strchr(server, 0)+1;
                plen  = strlen(param);

                while (plen) {
                    memcpy(sp, param, plen);
                    sp[plen]  = ' ';
                    sp       += plen+1;
                    param    += plen+1;
                    plen      = strlen(param);
                }
            }
        }

        if (*tcp_id == ':') {   /* local mode */
            command = GBS_global_string_copy("%s %s -T%s &",server, serverparams, tcp_id);
            if (!gbmain || GBCMC_system(gbmain,command)){
                system(command);
            }
            if (do_sleep) sleep(4);
        }
        else {
            char *host = strdup(tcp_id);
            char *p    = strchr(host, ':');

            if (!p) {
                error = GB_export_error("Error: Missing ':' in line '%s' file $(ARBHOME)/lib/arb_tcp.dat", arb_tcp_env);
            }
            else {
                *p = 0;
                if (GB_host_is_local(host)){
                    command = GBS_global_string_copy("%s %s -T%s &",server, serverparams, tcp_id);
#if defined(DEBUG)
                    printf("Contacting local host '%s' (cmd='%s')\n", host, command);
#endif /* DEBUG */
                }
                else {
                    command = GBS_global_string_copy("ssh %s -n %s %s -T%s &",host, server, serverparams, tcp_id);
#if defined(DEBUG)
                    printf("Contacting remote host '%s' (cmd='%s')\n", host, command);
#endif /* DEBUG */
                }
                if (!gbmain || GBCMC_system(gbmain,command)) {
                    system(command);
                }
                if (do_sleep) sleep(5);
            }
            free(host);
        }
        free(command);
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
            if (GB_size_of_file(file) <= 0) {
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
                                              " 2. Go to ARB_NT/Probes/PT_SERVER Admin\n"
                                              " 3. Select '%s' and press UPDATE SERVER\n"
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
                        "     update server <ARB_NT/Probes/PT_SERVER Admin/UPDATE SERVER>\n"
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
        error = GB_export_error("Missing line '%s' in $(ARBHOME)/lib/arb_tcp.dat:",arb_tcp_env);
    }
    else {
        const char *server = strchr(tcp_id, 0)+1;

        glservercntrl.link = (aisc_com *) aisc_open(tcp_id, &glservercntrl.com, magic_number);
        if (glservercntrl.link) {
            const char *command = GBS_global_string("%s -kill -T%s &", server, tcp_id);
            /* sprintf(command, "%s -kill -T%s &", server, tcp_id); */
            if (system(command)) {
                error = GB_export_error("Cannot execute '%s'",command);
            }
            aisc_close(glservercntrl.link);
            glservercntrl.link = 0;
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
           "    -f<field>       sets DB field to '<field>'\n"
           "    -r              read-only mode\n"
           "    -D<server>      sets DB-server to '<server>'  [default = ':']\n"
           "    -J<server>      sets job-server to '<server>' [default = 'ARB_JOB_SERVER']\n"
           "    -M<server>      sets MGR-server to '<server>' [default = 'ARB_MGR_SERVER']\n"
           "    -P<server>      sets PT-server to '<server>'  [default = 'ARB_PT_SERVER']\n"
           "    -T<host:port>   sets TCP connection to '<host:port>'\n"
           );
}

struct arb_params *arb_trace_argv(int *argc, char **argv)
{
    struct arb_params *erg;
    int	s,d;

    erg             = (struct arb_params *)calloc(sizeof(struct arb_params),1);
    erg->db_server  = GB_strdup(":");
    erg->job_server = GB_strdup("ARB_JOB_SERVER");
    erg->mgr_server = GB_strdup("ARB_MGR_SERVER");
    erg->pt_server  = GB_strdup("ARB_PT_SERVER");

    for (s=d=0; s<*argc; s++) {
        if (argv[s][0] == '-') {
            switch (argv[s][1]) {
                case 's':	erg->species_name  = strdup(argv[s]+2);break;
                case 'e':	erg->extended_name = strdup(argv[s]+2);break;
                case 'a':	erg->alignment     = strdup(argv[s]+2);break;
                case 'd':	erg->default_file  = strdup(argv[s]+2);break;
                case 'f':	erg->field         = strdup(argv[s]+2);break;
                case 'r':	erg->read_only     = 1;break;
                case 'J':	erg->job_server    = strdup(argv[s]+2);break;
                case 'D':	erg->db_server     = strdup(argv[s]+2);break;
                case 'M':	erg->mgr_server    = strdup(argv[s]+2);break;
                case 'P':	erg->pt_server     = strdup(argv[s]+2);break;
                case 'T':	erg->tcp           = strdup(argv[s]+2);break;
                default:	argv[d++]          = argv[s];
            }
        }else{
            argv[d++] = argv[s];
        }
    }
    *argc=d;
    return erg;
}
