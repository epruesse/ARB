#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"
#include <arbdbt.h>
#include <servercntrl.h>
#include <server.h>
#include <client.h>
#include <struct_man.h>
#define MAX_TRY 10
#define TIME_OUT 1000*60*60*24

struct probe_struct_global psg;
char *pt_error_buffer = new char[1024];
ulong physical_memory = 0;

/*****************************************************************************
        Communication
*******************************************************************************/
char *pt_init_main_struct(PT_main *main, char *filename)
{
    main               = main;
    probe_read_data_base(filename);
    GB_begin_transaction(psg.gb_main);
    psg.alignment_name = GBT_get_default_alignment(psg.gb_main);
    GB_commit_transaction(psg.gb_main);
    printf("Building PT-Server for alignment '%s'...\n", psg.alignment_name);
    probe_read_alignments();
    PT_build_species_hash();
    return 0;
}
PT_main *aisc_main; /* muss so heissen */

extern "C" int server_shutdown(PT_main *pm,aisc_string passwd){
    /** passwdcheck **/
    pm = pm;
    if( strcmp(passwd, "47@#34543df43%&3667gh") ) return 1;
    printf("\nI got the shutdown message.\n");
    /** shoot clients **/
    aisc_broadcast(psg.com_so, 0,
                   "SERVER UPDATE BY ADMINISTRATOR!\nYou'll get the latest version. Your screen information will be lost, sorry!");
    /** shutdown **/
    aisc_server_shutdown(psg.com_so);
    exit(0);
    return 0;
}
extern "C" int broadcast(PT_main *main, int dummy )
{
    aisc_broadcast(psg.com_so, main->m_type, main->m_text);
    dummy = dummy;
    return 0;
}

void PT_init_psg(){
    memset((char *)&psg,0,sizeof(psg));
    int i;
    for (i=0;i<256;i++){
        psg.complement[i] = PT_complement(i);
    }
}

extern int aisc_core_on_error;

int main(int argc, char **argv)
{
    int                i;
    struct Hs_struct  *so;
     char             *name;
    char              *error;
    char              *aname,*tname;
    const char        *suffix;
    struct stat        s_source,s_dest;
    int                build_flag;
    struct arb_params *params;
    char              *command_flag;

#ifdef DEVEL_IDP
   
#endif

    params             = arb_trace_argv(&argc,argv);
    PT_init_psg();
    GB_install_pid(0);          /* not arb_clean able */
    aisc_core_on_error = 0;
    physical_memory    = GB_get_physical_memory();
#if defined(DEBUG)
    printf("physical_memory=%lu k (%lu Mb)\n", physical_memory, physical_memory/1024UL);
#endif // DEBUG

    /***** try to open com with any other pb server ******/
    if ((argc>2)                         ||
        ((argc<2) && !params->db_server) ||
        (argc >= 2 && strcmp(argv[1], "--help") == 0))
    {
        printf("Syntax: %s [-look/-build/-kill/-QUERY] -Dfile.arb -TSocketid\n",argv[0]);
        exit (-1);
    }
    if (argc==2) {
        command_flag = argv[1];
    }
    else {
        static char flag[] = "-boot";
        command_flag = flag;
    }

    if (!(name = params->tcp)) {
        if( !(name=(char *)GBS_read_arb_tcp("ARB_PT_SERVER0")) ){
            GB_print_error();
            exit(-1);
        }
    }

    aisc_main = create_PT_main();
    aname = params->db_server;
    suffix = ".pt";
    tname = (char *)calloc(sizeof(char),strlen(aname)+strlen(suffix)+1);
    sprintf(tname,"%s%s",aname,suffix);
    if (!strcmp(command_flag, "-build")) {  /* build command */
        if (( error = pt_init_main_struct(aisc_main, params->db_server) ))
        {
            printf("PT_SERVER: Gave up:\nERROR: %s\n", error);
            exit(0);
        }
        enter_stage_1_build_tree(aisc_main,tname);
        printf("PT_SERVER database '%s' has been created.\n", params->db_server);
        exit(0);
    }
    if (!strcmp(command_flag, "-QUERY")) {
        if (( error = pt_init_main_struct(aisc_main, params->db_server) ))
        {
            printf("PT_SERVER: Gave up:\nERROR: %s\n", error);
            exit(0);
        }
        enter_stage_3_load_tree(aisc_main,tname);           /* now stage 3 */
        PT_debug_tree();
        exit(0);
    }
    psg.link = (aisc_com *) aisc_open(name, &psg.main, AISC_MAGIC_NUMBER);
    if (psg.link) {
        if (!strcmp(command_flag, "-look"))
            exit(0);    /* already another serther */
        printf("There is another activ server. I try to kill him...\n");
        aisc_nput(psg.link, PT_MAIN, psg.main,
                  MAIN_SHUTDOWN, "47@#34543df43%&3667gh",
                  NULL);
        aisc_close(psg.link); psg.link = 0;
    }
    if (!strcmp(command_flag, "-kill")) {
        exit(0);
    }
    printf("\n TUM POS_TREE SERVER (Oliver Strunk) V 1.0 (C) 1993 \ninitializing:\n");
    printf("open connection...\n");
    sleep(1);
    for (i = 0, so = 0; (i < MAX_TRY) && (!so); i++) {
        so = open_aisc_server(name, TIME_OUT,0);
        if (!so)
            sleep(10);
    }
    if (!so) {
        printf("PT_SERVER: Gave up on opening the communication socket!\n");
        exit(0);
    }
    psg.com_so = so;
    /**** init server main struct ****/
    printf("Init internal structs...\n");
    /****** all ok: main'loop' ********/
    if (stat(aname,&s_source)) {
        aisc_server_shutdown(so);
        printf("PT_SERVER error while stat source %s\n",aname);
        exit(-1);
    }
    build_flag = 0;
    if (stat(tname,&s_dest)) {
        build_flag = 1;
    }else{
        if ( (s_source.st_mtime>s_dest.st_mtime) || (s_dest.st_size == 0)) {
            printf("PT_SERVER: Source %s has been modified more recently than %s\n",aname,tname);
            build_flag = 1;
        }
    }
    if (build_flag) {
        char buf[256];
        sprintf(buf,"%s -build -D%s",argv[0],params->db_server);
        system(buf);
    }

    if (( error = pt_init_main_struct(aisc_main, params->db_server) ))
    {
        aisc_server_shutdown(so);
        printf("PT_SERVER: Gave up:\nERROR: %s\n", error);
        exit(0);
    }
    enter_stage_3_load_tree(aisc_main,tname);
    printf("ok, server is running.\n");
    aisc_accept_calls(so);
    aisc_server_shutdown(so);

    return 0;       // never reached
}
