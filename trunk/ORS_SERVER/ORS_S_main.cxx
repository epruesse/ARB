/* 
#################################
#                               #
#            M A I N            #
#   Main Part of ORS SERVER     #
#                               #
################################# */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include <ors_lib.h>
#include <ors_server.h>
#include <ors_client.h>
#include "ors_s.h"
#include <arbdb.h>
#include <ors_prototypes.h>
#include <server.h>
#include <client.h>
#include <servercntrl.h>
#include <struct_man.h>
#include "ors_s_common.hxx"
#include "ors_s_proto.hxx"


struct ORS_gl_struct ORS_global;

ORS_main *aisc_main; /* do not change this name! */

/***************************************************
  Shutdown
****************************************************/
int ors_server_shutdown(void) {
    GB_ERROR error;

    error = OS_save_userdb();
    if (error) GB_print_error();
    error = OS_save_probedb();
    if (error) GB_print_error();
    aisc_server_shutdown_and_exit(ORS_global.server_communication, EXIT_SUCCESS); // never returns
}

/***************************************************
  Shutdown
****************************************************/
extern "C"
int server_shutdown(ORS_main *pm,aisc_string passwd){
    /** passwdcheck **/
    if( strcmp(passwd, "asdsfdgsfvheryvtrsdgfdxfg") ) return 1;
    printf("\nI got the shutdown message.\n");

    /** shoot clients **/
    aisc_broadcast(ORS_global.server_communication, 0,
        "SERVER SHUTDOWN BY ADMINISTRATOR!\n");

    /** shutdown **/
    ors_server_shutdown();
    exit(0);
    pm = pm;
    return 0;   /* Never Reached */
}

/***************************************************

****************************************************/
extern "C" int server_save(ORS_main * /*locs*/, int )
{
    return 0;
}

/***************************************************
   Search Probe: Request Data from Database Server 
****************************************************/
extern "C" bytestring * search_probe(ORS_local *locs) {
    static bytestring bs = {0,0};
    delete bs.data;
    bs.data = strdup(locs->query_string.data);
    bs.size = strlen(bs.data)+1;
    return &bs;
}

/*****************************************************************************
  FIND DAILYPW: read userpath from hash table
        create error message if necessary
                        returns pointer on hash elem
*****************************************************************************/
extern "C" char * OS_find_dailypw(ORS_local *locs) {
    ORS_main *pm = (ORS_main *)locs->mh.parent->parent;

    struct passwd_struct *pws = (struct passwd_struct *)GBS_read_hash((GB_HASH *)pm->pwds,locs->dailypw);
    if (!pws) {
        delete locs->error;
        locs->error=strdup("Your daily password is not valid. Please login again!");
        return "";
    }
    if (ORS_strcmp(pws->remote_host, locs->remote_host)) {
        delete locs->error;
        locs->error=strdup("Wrong host!");
        return "";
    }
    pws->last_access_time = GB_time_of_day();

    return pws->userpath;
}

/*****************************************************************************
  LOCS ERROR: create error message (strdup)
                        strdup(error) or strdup("")
*****************************************************************************/
void OS_locs_error(ORS_local *locs, char *msg) {
    delete locs->error;
    if (!msg) locs->error=strdup("");
    else locs->error = strdup(msg);
}

/*****************************************************************************
  LOCS ERROR: create error message (user pointer, do not strdup)
                            error or strdup("")
*****************************************************************************/
void OS_locs_error_pointer(ORS_local *locs, char *msg) {
    delete locs->error;
    if (!msg) locs->error=strdup("");
    else locs->error = msg;
}

/**************************
  generate a new dailypw 
***************************/
char *gen_dailypw(int debug){
    char first_buf[20];
    char next_buf[200];
    int i;
    char *dirt[10] = {  "Iiji1JT",
                "jLjOij",
                "IjIjTj",
                "l1iTj0",
                "IiTJ1IT",
                "lJOljI",
                "i0IOjjii",
                "10iJkLJL",
                "I0jljijl",
                "IjLi0ull1" };

    sprintf(first_buf,"%ld",(unsigned long)(GB_time_of_day()%16704356)*75 + rand()%9999999);
    // TODO:  - time of day plus ein millisekundenanteil oder so
    //        - ausserdem: in wilde il, ll, jl, ij, ji-Kombinationen splitten
    //        - AM BESTEN: USERNAME bzw. PASSWORD mit eincodieren!! d.h. keine ms noetig!
    // z.B. IiTJIOTij0JlIjliJT 

    strcpy(next_buf,"");
    for (i=0; first_buf[i]; i++) {
        strcat(next_buf,dirt[(int)first_buf[i] - (int)'0']);
    }
    if (debug) {
        first_buf[3]=0;
        return strdup(first_buf);
    }
    
    return strdup(next_buf);
}


/*#####################################
  ##            M A I N              ##
  #####################################*/

int main(int argc,char **argv)
    {
    char *name;
    int i;
    struct Hs_struct *so;
    struct arb_params *params;

    long current_time, last_save_time = GB_time_of_day();

    write_logfile(0, "START SERVER ...");

    params = arb_trace_argv(&argc,argv);
    if (!params->default_file) {
        printf("No file: Syntax: %s -boot/-kill/-look -dfile\n",argv[0]);
        exit(-1);
    }

    if (argc ==1) {
        argv[1] = "-look";
        argc = 2;
    }

    if (argc!=2) {
        write_logfile(0, "START SERVER ... Wrong Parameters");
        printf("Wrong Parameters %i Syntax: %s -boot/-kill/-look -dfile\n",argc,argv[0]);
        exit(-1);
    }

    aisc_main = create_ORS_main();
    aisc_main->pwds = (long)GBS_create_hash(1024,0);



    /***** try to open com with any other pb server ******/
    if (params->tcp) {
        name = params->tcp;
    }else{
        if( !(name=(char *)ORS_read_a_line_in_a_file(ORS_LIB_PATH "CONFIG","ORS_SERVER")) ){    

            GB_print_error();
            exit(-1);
        }else{
            name=strdup(name);
        }
    }

    ORS_global.cl_link = (aisc_com *)aisc_open(name,&ORS_global.cl_main,AISC_MAGIC_NUMBER);

    if (ORS_global.cl_link) {
        if( !strcmp(argv[1],"-look")) {
            aisc_close(ORS_global.cl_link);
            exit(0);
        }

        write_logfile(0, "START SERVER ... Try to kill other server...");
        printf("There is another activ server. I try to kill him...\n");
        aisc_nput(ORS_global.cl_link, ORS_MAIN, ORS_global.cl_main,
            MAIN_SHUTDOWN, "asdsfdgsfvheryvtrsdgfdxfg",
            NULL);
        aisc_close(ORS_global.cl_link);
        sleep(1);
    }
    if( ((strcmp(argv[1],"-kill")==0)) ||
        ((argc==3) && (strcmp(argv[2],"-kill")==0))){
            printf("Now I kill myself!\n");
            exit(0);
    }
    for(i=0, so=0; (i<MAX_TRY) && (!so); i++){
        so = open_aisc_server(name, TIME_OUT,0);
        if(!so) sleep(10);
    }
    if(!so){
        write_logfile(0, "START SERVER ... Gave up on comm. socket");
        printf("ORS_SERVER: Gave up on opening the communication socket!\n");
        exit(0);
    }
    ORS_global.server_communication = so;

    //aisc_main->server_file = strdup(params->default_file);

    OS_read_probedb_fields_into_pdb_list(aisc_main);

    GB_ERROR error = OS_open_userdb();
    if (error) GB_print_error();

    error = OS_open_probedb();
    if (error) GB_print_error();

    write_logfile(0, "START SERVER ... OK");
        
    while (i == i) {
        aisc_accept_calls(so);
        current_time = GB_time_of_day();
        if (current_time - last_save_time >= 1800) {
            // TODO: eine einfache save-Funktion GB_save_quick??
            if (MSG) printf("Saving probe database...\n");
            OS_save_probedb();
            if (MSG) printf("Saving user database...\n");
            OS_save_userdb();
            last_save_time = current_time;
        }
        if (aisc_main->ploc_st.cnt <=0) {
            printf("Waiting, no clients");
        }
    }

    return 0;
}

