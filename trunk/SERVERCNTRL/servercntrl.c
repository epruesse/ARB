#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
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

GB_ERROR arb_start_server(const char *arb_tcp_env, GBDATA *gbmain, int sleepval)
{
    char           *tcp_id;
    char           *server;
    char           *file;
    char           *host;
    char           *p;
    char            command[1024];

    if (!(tcp_id = (char *) GBS_read_arb_tcp(arb_tcp_env))) {
        return GB_export_error("Entry '%s' in $(ARBHOME)/lib/arb_tcp.dat not found", arb_tcp_env);
    }
    server = tcp_id + strlen(tcp_id) + 1;
    if (*server) {
        file = server + strlen(server) + 1;
    } else {
        file = server;
    }

    if (*tcp_id == ':') {	/* local mode */
        sprintf(command,"%s %s -T%s &",server, file, tcp_id);
        if (!gbmain || GBCMC_system(gbmain,command)){
            system(command);
        }
        sleep(sleepval*4);
    } else {
        host = strdup(tcp_id);
        p = strchr(host, ':');
        if (!p) {
            return GB_export_error("Error: Missing ':' in line '%s' file $(ARBHOME)/lib/arb_tcp.dat", arb_tcp_env);
        }
        *p = 0;
        if (GB_host_is_local(host)){
            sprintf(command,"%s %s -T%s &",server, file, tcp_id);
        }else{
            sprintf(command,"rsh %s -n %s %s -T%s &",host, server, file, tcp_id);
        }
        if (!gbmain || GBCMC_system(gbmain,command)){
            system(command);
        }
        free(host);
        sleep(sleepval*16);
    }
    free(tcp_id);
    return 0;
}

int servercntrl_started;

GB_ERROR arb_look_and_start_server(long magic_number, const char *arb_tcp_env, GBDATA *gbmain){

    int             mytry;
    char           *tcp_id;
    char           *server;
    char           *file;

    servercntrl_started = 0;
    if (!(tcp_id = (char *) GBS_read_arb_tcp(arb_tcp_env))) {
        return GB_export_error("Entry '%s' in $(ARBHOME)/lib/arb_tcp.dat not found", arb_tcp_env);
    }
    server = tcp_id + strlen(tcp_id) + 1;
    if (*server) {
        file = server + strlen(server) + 1;
    } else {
        file = server;
    }
    if (strchr(file,'/')) file = strchr(file,'/');
    if (GB_size_of_file(file) <= 0) {
        return GB_export_error("Sorry there is no database for your template '%s' yet\n"
                               "	To create such a database and it's index file:\n"
                               "	1. Start ARB on the whole database you want to use for\n"
                               "		probe match / design\n"
                               "	2. Go to ARB_NT/etc/Probe_Functions/PT_SERVER Admin\n"
                               "	3. Select '%s' and press UPDATE SERVER\n"
                               "	4. Wait ( few hours )\n"
                               "	5. Meanwhile read the help file: PT_SERVER: What Why and How",
                               file, file);
    }

    for (glservercntrl.link = 0, mytry = 0; (!glservercntrl.link) && (mytry <= TRIES); mytry++) {
        glservercntrl.link = (aisc_com *) aisc_open(tcp_id,&glservercntrl.com, (int)magic_number);
        if ((mytry<TRIES)&&(!glservercntrl.link) ) {
            GB_ERROR error = arb_start_server(arb_tcp_env,gbmain,1);
            if (error) return error;
            servercntrl_started = 1;
        }
    }
    free(tcp_id);
    if (glservercntrl.link) {
        aisc_close(glservercntrl.link);
        glservercntrl.link = 0;
        return 0;
    }
    return GB_export_error("I got some problems to start your server:\n"
                           "	Possible Reasons:\n"
                           "	- there is no database in $ARBHOME/lib/pts/*\n"
                           "		update server <ARB_NT/ETC/PT_SERVER Admin/UPDATE SERVER>\n"
                           "	- you are not allowed to run 'rsh host pt_server ....&'\n"
                           "		check file '/etc/hosts.equiv' (read man pages for help)\n"
                           "	- the permissions of $ARBHOME/lib/pts/* do not allow read access\n"
                           "	- the PT_SERVER host is not up\n"
                           "	- the tcp_id is already used by another programm\n"
                           "		check $ARBHOME/lib/arb_tcp.dat and /etc/services\n");
}

GB_ERROR
arb_look_and_kill_server(int magic_number, char *arb_tcp_env)
{
    char           *tcp_id;
    char           *server;
    char            command[1024];
    GB_ERROR	error = 0;

    if (!(tcp_id = (char *) GBS_read_arb_tcp(arb_tcp_env))) {
        return GB_export_error(
                               "Missing line '%s' in $(ARBHOME)/lib/arb_tcp.dat:",arb_tcp_env);
    }
    server = tcp_id + strlen(tcp_id) + 1;

    glservercntrl.link = (aisc_com *) aisc_open(tcp_id, &glservercntrl.com, magic_number);
    if (glservercntrl.link) {
        sprintf(command,
                "%s -kill -T%s &",
                server, tcp_id);
        if (system(command)) {
            error = GB_export_error("Cannot execute '%s'",command);
        }
        aisc_close(glservercntrl.link);
        glservercntrl.link = 0;
    }else{
        error= GB_export_error("I cannot kill your server because I cannot find it");
    }
    free(tcp_id);
    return error;
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
                case 's':	erg->species_name = strdup(argv[s]+2);break;
                case 'e':	erg->extended_name = strdup(argv[s]+2);break;
                case 'a':	erg->alignment = strdup(argv[s]+2);break;
                case 'd':	erg->default_file = strdup(argv[s]+2);break;
                case 'r':	erg->read_only = 1;break;
                case 'J':	erg->job_server = strdup(argv[s]+2);break;
                case 'D':	erg->db_server = strdup(argv[s]+2);break;
                case 'M':	erg->mgr_server = strdup(argv[s]+2);break;
                case 'P':	erg->pt_server = strdup(argv[s]+2);break;
                case 'T':	erg->tcp = strdup(argv[s]+2);break;
                default:	argv[d++] = argv[s];
            }
        }else{
            argv[d++] = argv[s];
        }
    }
    *argc=d;
    return erg;
}
