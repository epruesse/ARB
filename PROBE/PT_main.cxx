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

struct probe_struct_global  psg;
char                       *pt_error_buffer = new char[1024];
ulong                       physical_memory = 0;

// globals of gene-pt-server
int gene_flag = 0;

gene_struct_list           all_gene_structs; // stores all gene_structs
gene_struct_index_arb      gene_struct_arb2internal; // sorted by arb speces+gene name
gene_struct_index_internal gene_struct_internal2arb; // sorted by internal name

// list<gene_struct *>  names_list_idp;
// GBDATA *map_ptr_idp;

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
    aisc_server_shutdown_and_exit(psg.com_so, EXIT_SUCCESS); // never returns
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

// ------------------------------------------------------------------------------ name mapping
// the mapping is generated in ../TOOLS/arb_gene_probe.cxx

inline const char *find_sep(const char *str, char sep) {
    // sep may occur escaped (by \)
    const char *found = strchr(str, sep);
    while (found) {
        if (found>str && found[-1] == '\\') { // escaped separator
            found = strchr(found+1, sep);
        }
        else {
            break;              // non-escaped -> report
        }
    }
    return found;
}

inline bool copy_to_buf(const char *start, const char *behindEnd, int MAXLEN, char *destBuf) {
    int len = behindEnd-start;
    if (len>MAXLEN) {
        return false;
    }
    memcpy(destBuf, start, len);
    destBuf[len] = 0;
    return true;
}

static void parse_names_into_gene_struct(const char *map_str, gene_struct_list& listOfGenes) {
#define MAX_INAME_LEN 8
#define MAX_ONAME_LEN 8
#define MAX_GNAME_LEN 1024

    char iname[MAX_INAME_LEN+1]; // internal name
    char oname[MAX_ONAME_LEN+1]; // organism name
    char gname[MAX_GNAME_LEN+1]; // gene name

    const char *err = 0;

    while (*map_str) {
        const char *sep1 = strchr(map_str, ';');
        const char *sep2 = sep1 ? strchr(sep1+1, ';') : 0;
        const char *sep3 = sep2 ? find_sep(sep2+1, ';') :  0;

        if (sep3) {
            bool ok = copy_to_buf(map_str, sep1, MAX_INAME_LEN, iname);
            ok = ok && copy_to_buf(sep1+1, sep2, MAX_ONAME_LEN, oname);
            ok = ok && copy_to_buf(sep2+1, sep3, MAX_GNAME_LEN, gname);

            if (ok) {
                char *unesc               = GBS_unescape_string(gname, ";", '\\');
                listOfGenes.push_back(gene_struct(iname, oname, unesc));
                free(unesc);
            }
            else {
                err = "buffer overflow";
                break;
            }
        }
        else {
            err = GBS_global_string("expected at least 3 ';' in '%s'", map_str);
            break;
        }
        map_str = sep3+1;
    }

    if (err) {
        printf("Error parsing name-mapping (%s)\n", err);
        exit(EXIT_FAILURE);
    }

}

void PT_init_map(){
    GB_push_transaction(psg.gb_main);
    GBDATA *map_ptr_idp = GB_find(psg.gb_main,"gene_map",0,down_level);

    if (map_ptr_idp != NULL) {
        gene_flag               = 1;
        GBDATA *    map_ptr_str = GB_find(map_ptr_idp,"map_string",0,down_level);
        const char *map_str;
        map_str                 = GB_read_char_pntr(map_ptr_str);

        parse_names_into_gene_struct(map_str, all_gene_structs);

        // build indices :
        gene_struct_list::const_iterator end = all_gene_structs.end();

        for (gene_struct_list::const_iterator gs = all_gene_structs.begin(); gs != end; ++gs) {
            if (gene_struct_internal2arb.find(&*gs) != gene_struct_internal2arb.end()) {
                fprintf(stderr, "  Duplicated internal entry for '%s'\n", gs->get_internal_gene_name());
            }
            gene_struct_internal2arb.insert(&*gs);
            if (gene_struct_arb2internal.find(&*gs) != gene_struct_arb2internal.end()) {
                fprintf(stderr, "  Duplicated entry for '%s/%s'\n", gs->get_arb_species_name(), gs->get_arb_gene_name());
            }
            gene_struct_arb2internal.insert(&*gs);
        }

        size_t list_size = all_gene_structs.size();
        bool   err       = false;
        if (list_size == 0) {
            fprintf(stderr, "Internal error - empty name map.\n");
            err = true;
        }
        else {
            fprintf(stderr, "Name map size = %u\n", list_size);
        }
        if (gene_struct_arb2internal.size() != list_size) {
            fprintf(stderr, "Internal error - detected %i duplicated 'species/gene' combinations in name mapping.\n",
                    list_size-gene_struct_arb2internal.size());
            err = true;
        }
        if (gene_struct_internal2arb.size() != list_size) {
            fprintf(stderr, "Internal error - detected %i duplicated internal gene name in name mapping.\n",
                    list_size-gene_struct_internal2arb.size());
            err = true;
        }
        if (err) {
            fprintf(stderr, "Sorry - PT server cannot start.\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        gene_flag = 0;
    }
    GB_pop_transaction(psg.gb_main);
}

extern int aisc_core_on_error;

int main(int argc, char **argv)
{
    int                i;
    struct Hs_struct  *so;
    const char        *name;
    char              *error;
    char              *aname,*tname;
    const char        *suffix;
    struct stat        s_source,s_dest;
    int                build_flag;
    struct arb_params *params;
    char              *command_flag;

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
        exit(EXIT_FAILURE);
    }
    if (argc==2) {
        command_flag = argv[1];
    }
    else {
        static char flag[] = "-boot";
        command_flag = flag;
    }

    if (!(name = params->tcp)) {
        if( !(name=GBS_read_arb_tcp("ARB_PT_SERVER0")) ){
            GB_print_error();
            exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
        }
        enter_stage_1_build_tree(aisc_main,tname);

        {
            char *msg = GBS_global_string_copy("PT_SERVER database \"%s\" has been created.", params->db_server);
            puts(msg);
            GBS_add_ptserver_logentry(msg);

            char *command = GBS_global_string_copy("arb_message '%s'", msg);

            if (system(command) != 0) {
                fprintf(stderr, "Failed to run '%s'\n", command);
            }
            free(command);
            free(msg);
        }

        exit(EXIT_SUCCESS);
    }
    if (!strcmp(command_flag, "-QUERY")) {
        if (( error = pt_init_main_struct(aisc_main, params->db_server) ))
        {
            printf("PT_SERVER: Gave up:\nERROR: %s\n", error);
            exit(EXIT_FAILURE);
        }
        enter_stage_3_load_tree(aisc_main,tname); /* now stage 3 */
        printf("Tree loaded - performing checks..\n");
        PT_debug_tree();
        printf("Checks done");
        exit(EXIT_SUCCESS);
    }
    psg.link = (aisc_com *) aisc_open(name, &psg.main, AISC_MAGIC_NUMBER);
    if (psg.link) {
        if (strcmp(command_flag, "-look") == 0) {
            exit(EXIT_SUCCESS); /* already another server */
        }
        printf("There is another activ server. I try to kill him...\n");
        aisc_nput(psg.link, PT_MAIN, psg.main,
                  MAIN_SHUTDOWN, "47@#34543df43%&3667gh",
                  NULL);
        aisc_close(psg.link); psg.link      = 0;
    }
    if (strcmp(command_flag, "-kill") == 0) {
        exit(EXIT_SUCCESS);
    }
    printf("\nTUM POS_TREE SERVER (Oliver Strunk) V 1.0 (C) 1993 \ninitializing:\n");
    printf("open connection...\n");
    sleep(1);
    for (i = 0, so = 0; (i < MAX_TRY) && (!so); i++) {
        so = open_aisc_server(name, TIME_OUT,0);
        if (!so)
            sleep(10);
    }
    if (!so) {
        printf("PT_SERVER: Gave up on opening the communication socket!\n");
        exit(EXIT_FAILURE);
    }
    psg.com_so = so;
    /**** init server main struct ****/
    printf("Init internal structs...\n");
    /****** all ok: main'loop' ********/
    if (stat(aname,&s_source)) {
        printf("PT_SERVER error while stat source %s\n",aname);
        aisc_server_shutdown_and_exit(so, EXIT_FAILURE); // never returns
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
        printf("Executing '%s'\n", buf);
        if (system(buf) != 0) {
            printf("Error building pt-server.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (( error = pt_init_main_struct(aisc_main, params->db_server) ))
    {
        printf("PT_SERVER: Gave up:\nERROR: %s\n", error);
        aisc_server_shutdown_and_exit(so, EXIT_FAILURE); // never returns
    }
    enter_stage_3_load_tree(aisc_main,tname);

    PT_init_map();

    printf("ok, server is running.\n");
    aisc_accept_calls(so);
    aisc_server_shutdown_and_exit(so, EXIT_SUCCESS); // never returns
}
