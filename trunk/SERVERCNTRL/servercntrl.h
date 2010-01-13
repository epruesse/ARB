/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef SERVERCNTRL_H
#define SERVERCNTRL_H

#ifdef __cplusplus
extern "C" {
#endif


/* servercntrl.cxx */

struct arb_params {
    char *species_name;
    char *extended_name;
    char *alignment;
    char *default_file;
    char *field;
    const char *field_default;

    int  read_only;

    char *job_server;
    char *db_server;
    char *mgr_server;
    char *pt_server;

    char *tcp;
};

char *prefixSSH(const char *host, const char *command, int async);
GB_ERROR arb_start_server(const char *arb_tcp_env, GBDATA *gbmain, int do_sleep);
GB_ERROR arb_look_and_start_server(long magic_number, const char *arb_tcp_env, GBDATA *gbmain);
GB_ERROR arb_look_and_kill_server(int magic_number, const char *arb_tcp_env);
void arb_print_server_params(void);
struct arb_params *arb_trace_argv(int *argc, char **argv);
void free_arb_params(struct arb_params *params);

#ifdef __cplusplus
}
#endif

#else
#error servercntrl.h included twice
#endif /* SERVERCNTRL_H */
