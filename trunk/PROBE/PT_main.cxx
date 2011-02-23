// =============================================================== //
//                                                                 //
//   File      : PT_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include <PT_server_prototypes.h>
#include "pt_prototypes.h"

#include <BI_helix.hxx>

#include <arbdbt.h>
#include <servercntrl.h>
#include <server.h>
#include <client.h>
#include <struct_man.h>

#include <unistd.h>
#include <sys/stat.h>

#define MAX_TRY 10
#define TIME_OUT 1000*60*60*24

struct probe_struct_global  psg;
ULONG                       physical_memory = 0;

// globals of gene-pt-server
int gene_flag = 0;

gene_struct_list           all_gene_structs;        // stores all gene_structs
gene_struct_index_arb      gene_struct_arb2internal; // sorted by arb species+gene name
gene_struct_index_internal gene_struct_internal2arb; // sorted by internal name

// ----------------------
//      Communication

ARB_ERROR pt_init_main_struct(PT_main *, const char *filename) { // __ATTR__USERESULT
    ARB_ERROR error = probe_read_data_base(filename);
    if (!error) {
        GB_begin_transaction(psg.gb_main);
        psg.alignment_name = GBT_get_default_alignment(psg.gb_main);
        GB_commit_transaction(psg.gb_main);
        printf("Building PT-Server for alignment '%s'...\n", psg.alignment_name);
        probe_read_alignments();
        PT_build_species_hash();
    }
    return error;
}

PT_main *aisc_main; // muss so heissen

extern "C" int server_shutdown(PT_main *pm, aisc_string passwd) {
    // password check
    pm = pm;
    if (strcmp(passwd, "47@#34543df43%&3667gh")) return 1;
    fprintf(stderr, "\nARB_PT_SERVER: received shutdown message\n");

    // shutdown clients
    aisc_broadcast(psg.com_so, 0, "Used PT-server has been shut down");

    // shutdown server
    aisc_server_shutdown(psg.com_so);
    PT_exit(EXIT_SUCCESS); // never returns
    return 0;
}

extern "C" int broadcast(PT_main *main, int) {
    aisc_broadcast(psg.com_so, main->m_type, main->m_text);
    return 0;
}

// ----------------------------------------------
//      global data initialization / cleanup

static bool psg_initialized = false;
void PT_init_psg() {
    pt_assert(!psg_initialized);
    memset((char *)&psg, 0, sizeof(psg));
    int i;
    for (i=0; i<256; i++) {
        psg.complement[i] = PT_complement(i);
    }
    psg_initialized = true;
}

void PT_exit_psg() {
    static bool executed = false;
    pt_assert(!executed);
    executed             = true;

    pt_assert(psg_initialized);
    if (psg_initialized) {
        if (psg.gb_main) {
            int count = GB_number_of_subentries(psg.gb_species_data);
            for (int i = 0; i < count; ++i) {
                free(psg.data[i].data);
                free(psg.data[i].name);
                free(psg.data[i].fullname);
            }
            free(psg.data);

            GB_close(psg.gb_main);
            psg.gb_main         = NULL;
            psg.gb_species_data = NULL;
            psg.gb_sai_data     = NULL;
        }
        
        if (psg.gb_shell) {
            delete psg.gb_shell;
            psg.gb_shell = NULL;
        }

        if (psg.namehash) GBS_free_hash(psg.namehash);

        free(psg.ecoli);
        delete psg.bi_ecoli;
        delete psg.pos_to_weight;
        free(psg.alignment_name);
        free(psg.ptmain);
        free(psg.com_so);

        memset((char *)&psg, 0, sizeof(psg)); 
        psg_initialized = false;
    }
}

void PT_exit(int exitcode) {
    // unique exit point to ensure cleanup
    if (psg_initialized) PT_exit_psg();
    if (aisc_main) destroy_PT_main(aisc_main);
    PTM_finally_free_all_mem();
    exit(exitcode);
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

STATIC_ATTRIBUTED(__ATTR__USERESULT, ARB_ERROR parse_names_into_gene_struct(const char *map_str, gene_struct_list& listOfGenes)) {
#define MAX_INAME_LEN 30
#define MAX_ONAME_LEN 30
#define MAX_GNAME_LEN 1024

    char iname[MAX_INAME_LEN+1]; // internal name
    char oname[MAX_ONAME_LEN+1]; // organism name
    char gname[MAX_GNAME_LEN+1]; // gene name

    ARB_ERROR err;

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
                err = "buffer overflow (some name too long)";
                break;
            }
        }
        else {
            err = GBS_global_string("expected at least 3 ';' in '%s'", map_str);
            break;
        }
        map_str = sep3+1;
    }

    if (err) err = GBS_global_string("while parsing name-mapping: %s\n", err.deliver());

    return err;
}

GB_ERROR PT_init_map() { // goes to header: __ATTR__USERESULT
    GB_ERROR error = GB_push_transaction(psg.gb_main);
    if (!error) {
        GBDATA *gb_gene_map = GB_entry(psg.gb_main, "gene_map");

        if (gb_gene_map) {
            gene_flag = 1;
            
            GBDATA     *map_ptr_str = GB_entry(gb_gene_map, "map_string");
            const char *map_str     = GB_read_char_pntr(map_ptr_str);

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
            if (list_size == 0) {
                error = "name map is empty";
            }
            else if (gene_struct_arb2internal.size() != list_size) {
                size_t dups = list_size-gene_struct_arb2internal.size();
                error       = GBS_global_string("detected %zi duplicated 'species/gene' combinations in name mapping", dups);
            }
        }
        else {
            gene_flag = 0;
        }
    }

    return GB_end_transaction(psg.gb_main, error);
}

extern int aisc_core_on_error;

static void initGlobals() {
    aisc_core_on_error = 0;

    physical_memory = GB_get_physical_memory();
#if defined(DEBUG)
    printf("physical_memory=%lu k (%lu Mb)\n", physical_memory, physical_memory/1024UL);
#endif // DEBUG
}

STATIC_ATTRIBUTED(__ATTR__USERESULT, ARB_ERROR start_pt_server(const char *socket_name, const char *arbdb_name, const char *pt_name, const char *exename)) {
    ARB_ERROR error;

    fputs("\n"
          "TUM POS_TREE SERVER (Oliver Strunk) V 1.0 (C) 1993 \n"
          "initializing:\n"
          "- opening connection...\n", stdout);
    sleep(1);
    
    Hs_struct *so = NULL;
    for (int i = 0; (i < MAX_TRY) && (!so); i++) {
        so = open_aisc_server(socket_name, TIME_OUT, 0);
        if (!so) {
            fputs("  Cannot bind to socket (retry in 10 seconds)\n", stdout);
            sleep(10);
        }
    }

    if (!so) error = "can't bind to socket";
    else {
        psg.com_so = so;

        struct stat arbdb_stat;
        if (stat(arbdb_name, &arbdb_stat)) {
            error = GB_IO_error("stat", arbdb_name);
            error = GBS_global_string("Source DB missing (%s)", error.deliver());
        }
        if (!error) {
            const char *update_reason = NULL;

            {
                struct stat pt_stat;
                if (stat(pt_name, &pt_stat)) {
                    update_reason = GB_IO_error("stat", pt_name);
                }
                else if (arbdb_stat.st_mtime > pt_stat.st_mtime) {
                    update_reason = GBS_global_string("'%s' has been modified more recently than '%s'", arbdb_name, pt_name);
                }
                else if (pt_stat.st_size == 0) {
                    update_reason = GBS_global_string("'%s' is empty", pt_name);
                }
            }

            if (update_reason) {
                printf("- updating postree (Reason: %s)", update_reason);
                error = GB_system(GBS_global_string("%s -build -D%s", exename, arbdb_name));
                if (error) error = GBS_global_string("Failed to update postree (Reason: %s)", error.deliver());
            }
        }
        if (!error) {
            // init server main struct
            fputs("- init internal structs...\n", stdout);
            {
                void *reserved_for_mmap = NULL;
                long  size_of_file      = GB_size_of_file(pt_name);
                if (size_of_file > 0) {
                    reserved_for_mmap = malloc(size_of_file);
                    if (!reserved_for_mmap) {
                        error = GBS_global_string("cannot reserve enough memory to map postree (needed=%li)", size_of_file);
                    }
                }

                if (!error) error = pt_init_main_struct(aisc_main, arbdb_name);
                free(reserved_for_mmap);
            }

            if (!error) error = enter_stage_3_load_tree(aisc_main, pt_name);
            if (!error) error = PT_init_map();
            
            if (!error) {
                // all ok -> main "loop"
                printf("ok, server is running.\n");             // do NOT change or remove! others depend on it
                fflush(stdout);
                aisc_accept_calls(so);
            }
        }
        aisc_server_shutdown(so);
    }
    return error;
}

STATIC_ATTRIBUTED(__ATTR__USERESULT, ARB_ERROR run_command(const char *exename, const char *command, const arb_params *params)) {
    ARB_ERROR error;

    // check that arb_pt_server knows its socket
    const char *socket_name = params->tcp;
    if (!socket_name) {
        socket_name = GBS_read_arb_tcp("ARB_PT_SERVER0");
        if (!socket_name) {
            error = GB_await_error();
            error = GBS_global_string("Don't know which socket to use (Reason: %s)", error.deliver());
        }
    }

    if (!error) {
        char *pt_name = GBS_global_string_copy("%s.pt", params->db_server);

        if (strcmp(command, "-build") == 0) {  // build command
            error = pt_init_main_struct(aisc_main, params->db_server);
            if (error) error = GBS_global_string("Gave up (Reason: %s)", error.deliver());
            else {
                error = enter_stage_1_build_tree(aisc_main, pt_name);
                if (error) {
                    error = GBS_global_string("Failed to build index (Reason: %s)", error.deliver());
                }
                else {
                    char *msg = GBS_global_string_copy("PT_SERVER database \"%s\" has been created.", params->db_server);
                    puts(msg);
                    GBS_add_ptserver_logentry(msg);

                    char *msg_command        = GBS_global_string_copy("arb_message '%s'", msg);
                    if (system(msg_command) != 0) fprintf(stderr, "Failed to run '%s'\n", msg_command);
                    free(msg_command);

                    free(msg);
                }
            }
        }
        else if (strcmp(command, "-QUERY") == 0) {
            error = pt_init_main_struct(aisc_main, params->db_server);
            if (error) error = GBS_global_string("Gave up (Reason: %s)", error.deliver());
            else {
                error = enter_stage_3_load_tree(aisc_main, pt_name); // now stage 3
                if (!error) {
                    printf("Tree loaded - performing checks..\n");
                    PT_debug_tree();
                    printf("Checks done");
                }
            }
        }
        else {
            psg.link = (aisc_com *) aisc_open(socket_name, &psg.main, AISC_MAGIC_NUMBER);

            bool running = psg.link;
            bool kill    = false;
            bool start   = false;

            if      (strcmp(command, "-look") == 0) { start = !running; }
            else if (strcmp(command, "-boot") == 0) { kill  = running; start = true; }
            else if (strcmp(command, "-kill") == 0) { kill  = running; }
            else { error = GBS_global_string("Unknown command '%s'", command); }

            if (!error) {
                if (kill) {
                    pt_assert(running);
                    fputs("There is another activ server. Sending shutdown message..\n", stderr);
                    if (aisc_nput(psg.link, PT_MAIN, psg.main, MAIN_SHUTDOWN, "47@#34543df43%&3667gh", NULL)) {
                        fprintf(stderr,
                                "%s: Warning: Problem connecting to the running %s\n"
                                "             You might need to kill it manually to ensure proper operation\n",
                                exename, exename);
                    }
                    aisc_close(psg.link);
                    psg.link = 0;
                }

                if (start) {
                    error = start_pt_server(socket_name, params->db_server, pt_name, exename); // @@@ does not always return - fix! (see also server_shutdown())
                }
            }
        }

        free(pt_name);
    }

    return error;
}

int main(int argc, char **argv) {
    int         exitcode = EXIT_SUCCESS;
    arb_params *params   = arb_trace_argv(&argc, argv);
    const char *exename  = argv[0];

    PT_init_psg();
    GB_install_pid(0);          // not arb_clean able
    initGlobals();

    // parse arguments
    char *command = NULL;
    {
        if ((argc>2)                         ||
            ((argc<2) && !params->db_server) ||
            (argc >= 2 && strcmp(argv[1], "--help") == 0))
        {
            fprintf(stderr, "Syntax: %s [-look/-build/-kill/-QUERY/-boot] -Dfile.arb -TSocketid\n", exename);
            exitcode = EXIT_FAILURE;
        }
        else {
            if (argc==2) command = strdup(argv[1]);
            else command         = strdup("-boot");
        }
    }

    if (!command) {
        fprintf(stderr, "%s: Error: No command specified on command line", exename);
    }
    else {
        aisc_main = create_PT_main();

        GB_ERROR error = run_command(exename, command, params).deliver();
        if (error) {
            fprintf(stderr, "%s: Error: %s\n", exename, error);
            exitcode = EXIT_FAILURE;
        }
        free(command);
    }

    free_arb_params(params);
    PT_exit(exitcode);
}
