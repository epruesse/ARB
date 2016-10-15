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
#include "PT_mem.h"

#include <BI_basepos.hxx>

#include <arbdbt.h>
#include <arb_file.h>
#include <arb_defs.h>
#include <arb_sleep.h>
#include <servercntrl.h>
#include <server.h>
#include <client.h>
#include <struct_man.h>
#include <ut_valgrinded.h>
#include <ptclean.h>

#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_TRY 10
#define TIME_OUT 1000*60*60*24

struct probe_struct_global psg;

// globals of gene-pt-server
int gene_flag = 0;

PT_main *aisc_main;

static time_t startTime;

static gene_struct_list    all_gene_structs;         // stores all gene_structs
gene_struct_index_arb      gene_struct_arb2internal; // sorted by arb species+gene name
gene_struct_index_internal gene_struct_internal2arb; // sorted by internal name

// ----------------------------------------------
//      global data initialization / cleanup

void probe_statistic_struct::setup() {
    cut_offs    = 0;
    single_node = 0;
    short_node  = 0;
    long_node   = 0;
    longs       = 0;
    shorts      = 0;
    shorts2     = 0;
    chars       = 0;

#ifdef ARB_64
    int_node = 0;
    ints     = 0;
    ints2    = 0;
    maxdiff  = 0;
#endif
}

void probe_struct_global::setup() {
    // init uninitialized data

    gb_shell = NULL;
    gb_main  = NULL;

    alignment_name = NULL;
    namehash       = NULL;

    data_count = 0;
    data       = NULL;

    max_size   = 0;
    char_count = 0;
    
    reversed = 0;

    pos_to_weight = NULL;

    sort_by = 0;

    main_probe  = NULL;
    server_name = NULL;
    link        = NULL;

    main.clear();

    com_so = NULL;
    pt.p1 = NULL;

    stat.setup();
}

void probe_struct_global::cleanup() {
    if (gb_main) {
        delete [] data;

        GB_close(gb_main);
        gb_main = NULL;
    }

    if (gb_shell) {
        delete gb_shell;
        gb_shell = NULL;
    }

    if (namehash) GBS_free_hash(namehash);

    free(ecoli);
    delete bi_ecoli;
    delete [] pos_to_weight;
    free(alignment_name);

    pt_assert(!com_so);

    setup();
}

Memory MEM;

static bool psg_initialized = false;
void PT_init_psg() {
    pt_assert(!psg_initialized);
    psg.setup();
    psg_initialized = true;
}

void PT_exit_psg() {
    pt_assert(psg_initialized);
    if (psg_initialized) {
        psg.cleanup();
        psg_initialized = false;
    }
    MEM.clear();
}

static void PT_exit() { 
    // unique exit point to ensure cleanup
    if (aisc_main) destroy_PT_main(aisc_main);
    if (psg_initialized) PT_exit_psg();
}

// ----------------------
//      Communication

static ARB_ERROR pt_init_main_struct(PT_main *, const char *filename) { // __ATTR__USERESULT
    ARB_ERROR error = probe_read_data_base(filename, true);
    if (!error) {
        GB_transaction ta(psg.gb_main);
        psg.alignment_name = GBT_get_default_alignment(psg.gb_main);
        error              = GB_incur_error_if(!psg.alignment_name);
    }

    if (!error) {
        printf("Building PT-Server for alignment '%s'...\n", psg.alignment_name);
        error = PT_init_input_data();
        PT_build_species_hash();
    }
    return error;
}

int server_shutdown(PT_main */*pm*/, aisc_string passwd) {
    // password check
    bool authorized = strcmp(passwd, "47@#34543df43%&3667gh") == 0;
    free(passwd);
    if (!authorized) return 1;

    fflush_all();
    fprintf(stderr, "\nARB_PT_SERVER: received shutdown message\n");

    // shutdown clients
    aisc_broadcast(psg.com_so, 0, "Used PT-server has been shut down");

    // shutdown server
    aisc_server_shutdown(psg.com_so);
    PT_exit();
    fflush_all();
    exit(EXIT_SUCCESS);
}

int broadcast(PT_main *main, int) {
    aisc_broadcast(psg.com_so, main->m_type, main->m_text);
    return 0;
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

__ATTR__USERESULT static ARB_ERROR parse_names_into_gene_struct(const char *map_str, gene_struct_list& listOfGenes) {
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

    if (err) err = GBS_global_string("while parsing name-mapping: %s", err.deliver());

    return err;
}

static GB_ERROR PT_init_map() { // goes to header: __ATTR__USERESULT
    GB_ERROR error = GB_push_transaction(psg.gb_main);
    if (!error) {
        GBDATA *gb_gene_map = GB_entry(psg.gb_main, "gene_map");

        if (gb_gene_map) {
            gene_flag = 1;
            
            GBDATA     *map_ptr_str = GB_entry(gb_gene_map, "map_string");
            const char *map_str     = GB_read_char_pntr(map_ptr_str);

            error = parse_names_into_gene_struct(map_str, all_gene_structs).deliver();

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

__ATTR__USERESULT static ARB_ERROR start_pt_server(const char *socket_name, const char *arbdb_name, const char *pt_name, const char *exename) {
    ARB_ERROR error;

    fprintf(stdout,
            "\n"
            "ARB POS_TREE SERVER v%i (C)1993-2013 by O.Strunk, J.Boehnel, R.Westram\n"
            "initializing:\n"
            "- opening connection...\n",
            PT_SERVER_VERSION);
    GB_sleep(1, SEC);

    Hs_struct *so = NULL;
    {
        const int MAX_STARTUP_SEC = 100;
        const int RETRY_AFTER_SEC = 5;
        for (int i = 0; i<MAX_STARTUP_SEC; i += RETRY_AFTER_SEC) {
            so = open_aisc_server(socket_name, TIME_OUT, 0);
            if (so) break;

            printf("  Cannot bind to socket (retry in %i seconds)\n", RETRY_AFTER_SEC);
            GB_sleep(RETRY_AFTER_SEC, SEC);
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
                printf("- updating postree (Reason: %s)\n", update_reason);

                char *quotedDatabaseArg = GBK_singlequote(GBS_global_string("-D%s", arbdb_name));

                // run build_clean
                char *cmd = GBS_global_string_copy("%s -build_clean %s", exename, quotedDatabaseArg);
                make_valgrinded_call(cmd);
                error           = GBK_system(cmd);
                free(cmd);

                // run build
                if (!error) {
                    cmd   = GBS_global_string_copy("%s -build %s", exename, quotedDatabaseArg);
                    make_valgrinded_call(cmd);
                    error = GBK_system(cmd);
                    free(cmd);
                }

                if (error) error = GBS_global_string("Failed to update postree (Reason: %s)", error.deliver());

                free(quotedDatabaseArg);
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

            if (!error) error = enter_stage_2_load_tree(aisc_main, pt_name);
            if (!error) error = PT_init_map();

            if (!error) {
                // all ok -> main "loop"
                {
                    time_t now; time(&now);
                    fflush_all();
                    printf("[startup took %s]\n", GBS_readable_timediff(difftime(now, startTime)));
                }

                printf("ok, server is running.\n"); // do NOT change or remove! others depend on it

                fflush_all();
                aisc_accept_calls(so);
            }
        }
        aisc_server_shutdown(psg.com_so);
    }
    return error;
}

static int get_DB_state(GBDATA *gb_main, ARB_ERROR& error) {
    pt_assert(!error);
    error     = GB_push_transaction(gb_main);
    int state = -1;
    if (!error) {
        GBDATA *gb_ptserver = GBT_find_or_create(gb_main, "ptserver", 7);
        long   *statePtr    = gb_ptserver ? GBT_readOrCreate_int(gb_ptserver, "dbstate", 0) : NULL;
        if (!statePtr) {
            error = GB_await_error();
        }
        else {
            state = *statePtr;
        }
    }
    error = GB_end_transaction(gb_main, error);
    return state;
}

static GB_ERROR set_DB_state(GBDATA *gb_main, int dbstate) {
    GB_ERROR error = GB_push_transaction(gb_main);
    if (!error) {
        GBDATA *gb_ptserver  = GBT_find_or_create(gb_main, "ptserver", 7);
        GBDATA *gb_state     = gb_ptserver ? GB_searchOrCreate_int(gb_ptserver, "dbstate", 0) : NULL;
        if (!gb_state) error = GB_await_error();
        else    error        = GB_write_int(gb_state, dbstate);
    }
    error = GB_end_transaction(gb_main, error);
    return error;
}

__ATTR__USERESULT static ARB_ERROR run_command(const char *exename, const char *command, const arb_params *params) {
    ARB_ERROR  error;
    char      *msg = NULL;

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

        if (strcmp(command, "-build_clean") == 0) {  // cleanup source DB
            error = probe_read_data_base(params->db_server, false);
            if (!error) {
                pt_assert(psg.gb_main);

                error = GB_no_transaction(psg.gb_main); // don't waste memory for transaction (no abort may happen)

                if (!error) {
                    int dbstate = get_DB_state(psg.gb_main, error);
                    if (!error && dbstate>0) {
                        if (dbstate == 1) {
                            fputs("Warning: database already has been prepared for ptserver\n", stdout);
                        }
                        else {
                            error = GBS_global_string("unexpected dbstate='%i'", dbstate);
                        }
                    }

                    if (!error && dbstate == 0) {
                        GB_push_my_security(psg.gb_main);

                        if (!error) error = cleanup_ptserver_database(psg.gb_main, PTSERVER);
                        if (!error) error = PT_prepare_data(psg.gb_main);

                        if (!error) error = set_DB_state(psg.gb_main, 1);

                        GB_pop_my_security(psg.gb_main);

                        if (!error) {
                            const char *mode = GB_supports_mapfile() ? "bfm" : "bf";
                            error            = GB_save_as(psg.gb_main, params->db_server, mode);
                        }
                    }
                }
            }
        }
        else if (strcmp(command, "-build") == 0) {  // build command
            if (!error) error = pt_init_main_struct(aisc_main, params->db_server);
            if (!error) {
                int dbstate = get_DB_state(psg.gb_main, error);
                if (!error && dbstate != 1) {
                    error = "database has not been prepared for ptserver";
                }
            }
            if (error) error = GBS_global_string("Gave up (Reason: %s)", error.deliver());

            if (!error) {
                ULONG ARM_size_kb = 0;
                {
                    const char *mapfile = GB_mapfile(psg.gb_main);
                    if (GB_is_regularfile(mapfile)) {
                        ARM_size_kb = GB_size_of_file(mapfile)/1024.0+0.5;
                    }
                    else {
                        fputs("Warning: cannot detect size of mapfile (have none).\n"
                              "         Ptserver will probably use too much memory.\n",
                              stderr);
                    }
                }

                error = enter_stage_1_build_tree(aisc_main, pt_name, ARM_size_kb);
                if (error) {
                    error = GBS_global_string("Failed to build index (Reason: %s)", error.deliver());
                }
                else {
                    msg = GBS_global_string_copy("PT_SERVER database \"%s\" has been created.", params->db_server);
                }
            }
        }
        else if (strcmp(command, "-QUERY") == 0) {
            error = pt_init_main_struct(aisc_main, params->db_server);
            if (error) error = GBS_global_string("Gave up (Reason: %s)", error.deliver());
            else {
                error = enter_stage_2_load_tree(aisc_main, pt_name); // now stage 2
#if defined(CALCULATE_STATS_ON_QUERY)
                if (!error) {
                    puts("[index loaded - calculating statistic]");
                    PT_dump_tree_statistics(pt_name);
                    puts("[statistic done]");
                }
#endif
            }
        }
        else {
            GB_ERROR openerr = NULL;
            psg.link         = aisc_open(socket_name, psg.main, AISC_MAGIC_NUMBER, &openerr);

            if (openerr) {
                error = openerr;
            }
            else {
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
                        fputs("There is another active server. Sending shutdown message..\n", stderr);
                        if (aisc_nput(psg.link, PT_MAIN, psg.main, MAIN_SHUTDOWN, "47@#34543df43%&3667gh", NULL)) {
                            fprintf(stderr,
                                    "%s: Warning: Problem connecting to the running %s\n"
                                    "             You might need to kill it manually to ensure proper operation\n",
                                    exename, exename);
                        }
                        aisc_close(psg.link, psg.main);
                        psg.link = 0;
                    }

                    if (start) {
                        error = start_pt_server(socket_name, params->db_server, pt_name, exename); // @@@ does not always return - fix! (see also server_shutdown())
                    }
                }
            }
        }

        free(pt_name);
    }

    if (error) msg = ARB_strdup(error.preserve());
    if (msg) {
        puts(msg);                      // log to console ..
        GBS_add_ptserver_logentry(msg); // .. and logfile

        char     *quotedErrorMsg = GBK_singlequote(GBS_global_string("arb_pt_server: %s", msg));
        GB_ERROR  msgerror       = GBK_system(GBS_global_string("arb_message %s &", quotedErrorMsg));    // send async to avoid deadlock
        if (msgerror) fprintf(stderr, "Error: %s\n", msgerror);
        free(quotedErrorMsg);
        free(msg);
    }

    return error;
}

int ARB_main(int argc, char *argv[]) {
    time(&startTime);

    int         exitcode = EXIT_SUCCESS;
    arb_params *params   = arb_trace_argv(&argc, (const char **)argv);
    const char *exename  = argv[0];

    PT_init_psg();
    GB_install_pid(0);          // not arb_clean able

    extern int aisc_core_on_error;
    aisc_core_on_error = 0;

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
            if (argc==2) command = ARB_strdup(argv[1]);
            else command         = ARB_strdup("-boot");
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
        else {
            time_t now; time(&now);
            fflush_all();
            printf("[ptserver '%s' took %s]\n", command, GBS_readable_timediff(difftime(now, startTime)));
            fflush(stdout);
        }
        free(command);
    }

    free_arb_params(params);
    PT_exit();
    fflush_all();
    return exitcode;
}
