/*!
 *
 * \author Chris Hodges
 * \author Jörg Böhnel
 * \author Tilo Eißler
 */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdexcept>

#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ptpan_legacy.h"
#include <PT_server_prototypes.h>

#include <servercntrl.h>
#include <server.h>
#include <client.h>
#include <struct_man.h>

#include "ptpan_tree.h"
#include "dna5_alphabet_specifics.h"
#include "arbdb_data_retriever.h"
#include "ptpan_build_settings.h"
#include "ptpan_tree_builder.h"

#include <iostream>

// for ARB integration, global pointer is required
struct PTPanLegacy *PTPanLegacyGlobalPtr = NULL;

/*****************************************************************************
 Communication
 *******************************************************************************/

PT_main *aisc_main; /* must be named this way! */
extern int aisc_core_on_error;

/*!
 * \brief ...
 */
void global_free() {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    if (pl) {
        if (pl->pl_GbShell != NULL) {
            delete pl->pl_GbShell;
        }
        if (pl->pl_pt != NULL) {
            delete pl->pl_pt;
        }
        FreePTPanLegacy(pl);
    }
    if (aisc_main) {
        destroy_PT_main(aisc_main);
    }
}

/*!
 * \brief ...
 */
int server_shutdown(PT_main *, aisc_string passwd) {

    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;

    printf("EXTERN: server_shutdown\n");
    /** passwdcheck **/
    if (strcmp(passwd, "47@#34543df43%&3667gh")) {
        return 1;
    }

    printf("\nI got the shutdown message.\n");
    /** shoot clients **/
    aisc_broadcast(pl->pl_ComSocket, 0, "SERVER UPDATE BY ADMINISTRATOR!\n"
            "You'll get the latest version. Your on-screen\n"
            "information will be lost, sorry!");
    /** shutdown **/
    aisc_server_shutdown_and_exit(pl->pl_ComSocket, 0);
    return (0);
}

int broadcast(PT_main *main, int) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;

    printf("EXTERN: broadcast\n");
    aisc_broadcast(pl->pl_ComSocket, main->m_type, main->m_text);
    return (0);
}

#define PTPAN_LOGTAG "PTPAN"

/*
 * \brief Main
 */
int main(int argc, char *argv[]) {

    printf("\nTUM PeTer PAN SERVER v%s (C)\n\n", PtpanTree::version().data());
    printf("Use parameter '--help' for aditional command line parameters.\n");

    /* allocate the PTPanGlobal structure */
    struct PTPanLegacy *pl;
    if (!(pl = AllocPTPanLegacy())) {
        return 1;
    }
    PTPanLegacyGlobalPtr = pl;

    /* aisc init */
    GB_install_pid(0); /* not arb_clean able */
    aisc_core_on_error = 0;
    /* set global variable -- sigh */
    aisc_main = create_PT_main();

    /* first get the parameters */
    struct arb_params *arbParams = arb_trace_argv(&argc, argv);

    /* try to open com with any other pb server */
    /* check command line syntax */
    if (((argc < 2) && !arbParams->db_server)
            || (argc >= 2 && strcmp(argv[1], "--help") == 0)) {
        printf("Syntax: %s [-look/-build/-kill] -Dfile.arb -TSocketid\n",
                argv[0]);
        printf(" additional parameters:\n");
        printf(
                " --features\t\tInclude features (genes etc.) if available (genome databases) (default is false) [build only]\n");
        printf(
                " --size=\t\tmaximum memory usable size (in bytes) [build only]\n");
        printf(
                " --prune=\t\tPruning length of edges (default is 20) [build only]\n");
        printf(
                " --prefix=\t\tMaximum prefix length for partition (must be <= 27, default is 5) [build only]\n");
        printf(
                " --ratio=\t\tMemory utilization ratio during build (float value, e.g. 0.5 for 50%%).\n"
                        "\t\t\t range from ]0.0;1.0] USE CAREFULLY! [build only]\n");
        printf(
                " --threads=\t\tMaximum number of threads to use (default is number of cpu cores)\n");
        printf(
                " --force\t\tForce settings to be applied. "
                        "Otherwise, algorithm may e.g. use less threads for optimal performance. [build only]\n");
        printf(
                " --verbose|-v\t\tEnables verbose mode for some progress information output.\n");
        exit(-1);
    }
    // add default command flag
    STRPTR commandflag;
    if (argc >= 2) {
        commandflag = argv[1];
    } else {
        commandflag = (STRPTR) "-boot";
    }

    /* get server host name */
    if (!(pl->pl_ServerName = arbParams->tcp)) {
        if (!(pl->pl_ServerName = (STRPTR) GBS_read_arb_tcp(
                "ARB_PT_SERVER0"))) {
            GB_print_error(); /* no host name found */
            exit(-1);
        }
    }
    {
        /* generate tree filename */
        std::string dbName = std::string(arbParams->db_server);
        std::string indexName = dbName + std::string(".pan");

        /* check for other active servers */
        {
            aisc_com *ptlink;
            T_PT_MAIN ptmain;
            ptlink = (aisc_com *) aisc_open(pl->pl_ServerName, &ptmain,
                    AISC_MAGIC_NUMBER);
            if (ptlink) {
                if (strcasecmp(commandflag, "-look") == 0) {
                    if (arbParams) {
                        free_arb_params(arbParams);
                    }
                    global_free();
                    return 0; /* already another server */
                }
                printf(
                        "There is another active server. I'll try to terminate it violently...\n");
                aisc_nput(ptlink, PT_MAIN, ptmain, MAIN_SHUTDOWN,
                        "47@#34543df43%&3667gh", NULL);
                aisc_close(ptlink);
            }
        }
        if (strcmp(commandflag, "-kill") == 0) {
            if (arbParams) {
                free_arb_params(arbParams);
            }
            global_free();
            return 0;
        }

        BOOL build = FALSE;
        BOOL forcebuild = FALSE;

        if (!strncasecmp(commandflag, "-build", 6)) { /* build command */
            build = TRUE;
        }

        int num_threads = boost::thread::hardware_concurrency();
        for (int i = 0; i < argc; ++i) {
            if (strncmp(argv[i], "--threads=", 10) == 0) {
                std::string s = std::string(&argv[i][10]);
                num_threads = atoi(s.c_str());
            }
        }

        if (!build) {
            if (!strcasecmp(commandflag, "-QUERY")) {
                // TODO what to do here??
                //enter_stage_3_load_tree(aisc_main, tname); /* now stage 3 */
                if (arbParams) {
                    free_arb_params(arbParams);
                }
                global_free();
                return 0;
            }

            /* Check if index is up2date */
            {
                struct stat dbstat, idxstat;
                if (stat(dbName.data(), &dbstat)) {
                    printf("PT_PAN: error while stat source %s\n",
                            dbName.data());
                    if (pl->pl_ComSocket) {
                        aisc_server_shutdown_and_exit(pl->pl_ComSocket, 1);
                    }
                    if (arbParams) {
                        free_arb_params(arbParams);
                    }
                    global_free();
                    return 1;
                }

                if (stat(indexName.data(), &idxstat)) {
                    forcebuild = TRUE; /* there is no index at all! */
                } else {
                    if ((dbstat.st_mtime > idxstat.st_mtime)
                            || (idxstat.st_size == 0)) {
                        /* so the index file was older or of zero size */
                        printf("PT_PAN: Database %s has been modified\n"
                                "more recently than index %s.\n"
                                "Forcing rebuilding of index...\n",
                                dbName.data(), indexName.data());
                        forcebuild = TRUE;
                    } else {
                        try {
                            pl->pl_GbShell = new GB_shell();
                            GB_init_gb(); // needed for PT_new_design
                            pl->pl_pt = new PtpanTree(indexName);
                            pl->pl_pt->setNumberOfThreads(num_threads);
                        } catch (std::invalid_argument& e) {
                            printf("ERROR: %s", e.what());
                            if (arbParams) {
                                free_arb_params(arbParams);
                            }
                            global_free();
                            return 1;
                        } catch (...) {
                            // error openening index, so force re-build
                            forcebuild = TRUE;
                        }
                    }
                }
            }
        }
        if (forcebuild || build) {
            GBS_add_ptserver_logentry(PTPAN_LOGTAG, GBS_global_string("started index-build for '%s'", dbName.c_str()));
        
            AbstractAlphabetSpecifics *as =
                    (AbstractAlphabetSpecifics *) new Dna5AlphabetSpecifics();
            AbstractDataRetriever *dr =
                    (AbstractDataRetriever *) new ArbdbDataRetriever(dbName,
                            as);
            PtpanBuildSettings *settings = new PtpanBuildSettings(as,
                    indexName);
            if (!as || !dr || !settings) {
                printf("Failed to initialize ptpan build stuff!\n");
                if (arbParams) {
                    free_arb_params(arbParams);
                }
                global_free();
                return 1;
            }

            {
                std::string tmpDBName = dbName.substr(0, dbName.size() - 4);
                tmpDBName.append("_temp.arb");
#ifdef DEBUG
                std::cout << "temporary db name: " << tmpDBName << std::endl;
#endif
                struct stat tmpdbstat;
                if (!stat(tmpDBName.data(), &tmpdbstat)) {
#ifdef DEBUG
                    std::cout << "ARB wants a gene index out of genome db!"
                            << std::endl;
#endif
                    // we need a work-around for the case where the ARB
                    // created a (for us useless) gene db!
                    if (remove(dbName.data()) != 0) {
                        // error removing
                        printf("ERROR while removing ARB gene database.\n");
                        if (arbParams) {
                            free_arb_params(arbParams);
                        }
                        global_free();
                        return 1;
                    }
                    if (rename(tmpDBName.data(), dbName.data()) == 0) {
#ifdef DEBUG
                        printf("Database file successfully renamed!\n");
#endif
                    } else {
                        printf("ERROR renaming database file!\n");
                        if (arbParams) {
                            free_arb_params(arbParams);
                        }
                        global_free();
                        return 1;
                    }
                    settings->setIncludeFeatures(true);
                } else {
#ifdef DEBUG
                    std::cout
                            << "ARB wants a normal sequence index out of database!"
                            << std::endl;
#endif
                }
            }

            ULONG freeMem = GB_get_physical_memory();

#ifdef DEBUG
            printf("physical_memory=%lu k (%lu Mb)\n", freeMem, freeMem >> 10);
#endif // DEBUG
            // leave some memory for the OS (default 512 MB)
            freeMem = freeMem - (1UL << 19);
#ifdef DEBUG
            printf("Free memory %lu k (%lu MB)!\n", freeMem, freeMem >> 10);
#endif // DEBUG
            freeMem = freeMem * 1024; // make it bytes!
            settings->setMemorySize(freeMem);
            settings->setThreadNumber(num_threads);

            for (int i = 0; i < argc; ++i) {
                if (strncmp(argv[i], "--size=", 7) == 0) {
                    std::string s = std::string(&argv[i][7]);
                    // set the memory size in bytes which is used to
                    // calculate the max number of tuples!
                    settings->setMemorySize(atoi(s.c_str()));
                    printf("Setting main memory usable to: %lu (bytes)\n",
                            settings->getMemorySize());
                } else if (strncmp(argv[i], "--prune=", 8) == 0) {
                    std::string s = std::string(&argv[i][8]);
                    settings->setPruneLength(atoi(s.c_str()));
                    printf("Setting prune length to: %lu\n",
                            settings->getPruneLength());
                } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
                    std::string s = std::string(&argv[i][9]);
                    settings->setMaxPrefixLength(atoi(s.c_str()) + 1);
                    printf("Setting maximum prefix length to: %lu\n",
                            settings->getMaxPrefixLength() - 1);
                } else if (strncmp(argv[i], "--ratio=", 8) == 0) {
                    std::string s = std::string(&argv[i][8]);
                    double ratio = atof(s.c_str());
                    printf("found ratio '%4.2f'\n", ratio);
                    settings->setPartitionRatio(ratio);
                    printf("Setting partition memory ratio to: %4.2f\n",
                            settings->getPartitionRatio());
                } else if (strncmp(argv[i], "--threads=", 10) == 0) {
                    std::string s = std::string(&argv[i][10]);
                    settings->setThreadNumber(atoi(s.c_str()));
                    printf("Setting maximum number of threads to: %lu\n",
                            settings->getThreadNumber());
                } else if ((strncmp(argv[i], "-v", 2) == 0)
                        || (strncmp(argv[i], "--verbose", 9) == 0)) {
                    printf("Verbose mode!\n");
                    settings->setVerbose(true);
                } else if (strncmp(argv[i], "--force", 7) == 0) {
                    printf("Force settings!\n");
                    settings->setForce(true);
                } else if (strncmp(argv[i], "--features", 10) == 0) {
                    printf("Command line says we want features!\n");
                    settings->setIncludeFeatures(true);
                    dr->setIncludeFeatures(true);
#ifdef DEBUG
                } else {
                    if (i != 0) {
                        if (strncmp(argv[i], "-build", 6) != 0) { // ignore build param
                            std::cout << "Unknown param " << argv[i]
                                    << std::endl;
                        }
                    }
#endif
                }
            }

            try {
                PtpanTreeBuilder::buildPtpanTree(settings, as, dr,
                        "ARB integration of PTPan");
            } catch (std::exception& e) {
                delete as;
                delete dr;
                delete settings;
                printf("Building of PtPan index failed: %s\n", e.what());
                if (arbParams) {
                    free_arb_params(arbParams);
                }
                global_free();
                return 1;
            }

            delete as;
            delete dr;
            delete settings;

            GBS_add_ptserver_logentry(PTPAN_LOGTAG, GBS_global_string("finished index-build for '%s'", dbName.c_str()));

            if (build) {
                printf("PTPan index for database '%s' has been created.\n",
                       dbName.data());
                if (arbParams) {
                    free_arb_params(arbParams);
                }
                global_free();
                return 0;
            } else {
                // we came here by forcebuild, so load index ...
                try {
                    pl->pl_GbShell = new GB_shell();
                    GB_init_gb(); // needed for PT_new_design
                    pl->pl_pt = new PtpanTree(indexName);
                    pl->pl_pt->setNumberOfThreads(num_threads);
                } catch (std::invalid_argument& e) {
                    printf("ERROR: %s", e.what());
                    if (arbParams) {
                        free_arb_params(arbParams);
                    }
                    global_free();
                    return 1;
                } catch (...) {
                    // error opening index
                    printf("Fatal error, couldn't load index (%s).\n",
                            indexName.data());
                }
            }
        }
    }
    /* open the socket connection */
    printf("Opening connection ... ");
    {
        UWORD i;
        for (i = 0; i < 3; i++) {
            if ((pl->pl_ComSocket = open_aisc_server(pl->pl_ServerName,
                    1000 * 60 * 60 * 24, 0))) {
                break;
            } else {
                sleep(10);
            }
        }
        if (!pl->pl_ComSocket) {
            printf("PT_PAN: Gave up on opening the communication socket!\n");
            if (arbParams) {
                free_arb_params(arbParams);
            }
            global_free();
            return 0;
        }
    }

    /****** all ok: main loop ********/
    printf("ok, server is running.\n"); // do NOT change or remove! others depend on it
    fflush(stdout);

    aisc_accept_calls(pl->pl_ComSocket);
    aisc_server_shutdown_and_exit(pl->pl_ComSocket, 0);

    if (arbParams) {
        free_arb_params(arbParams);
    }
    global_free();

    return (0);
}

