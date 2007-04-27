#ifndef servercntrl_h_included
#define servercntrl_h_included

/************************************/
// #ifndef GB_INCLUDED
// typedef void GBDATA;
// #endif

// typedef struct gb_data_base_type GBDATA;

struct arb_params {
    char *species_name;
    char *extended_name;
    char *alignment;
    char *default_file;
    char *field;

    int  read_only;

    char *job_server;
    char *db_server;
    char *mgr_server;
    char *pt_server;

    char *tcp;
};

#ifdef __cplusplus
extern "C" {
#endif
    
    GB_ERROR           arb_start_server(const char *arb_tcp_env, GBDATA *gbmain, int do_sleep);
    GB_ERROR           arb_look_and_start_server(long magic_number, const char *arb_tcp_env, GBDATA *gbmain);
    GB_ERROR           arb_look_and_kill_server(int magic_number, const char *arb_tcp_env);
    void               arb_print_server_params(void);
    struct arb_params *arb_trace_argv(int *argc, char **argv);

#ifdef __cplusplus
}
#endif


#endif
