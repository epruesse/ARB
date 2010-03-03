// ============================================================= //
//                                                               //
//   File      : db_server.cxx                                   //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <arbdb.h>
#include <servercntrl.h>

#define TIMEOUT 1000*60*2       /* save every 2 minutes */
#define LOOPS   30              /* wait 30*TIMEOUT (1 hour) till shutdown */

int main(int argc, char **argv)
{
    struct arb_params *params;
    GBDATA            *gb_extern;
    GBDATA            *gb_main;
    GB_ERROR           error;
    int                timer;
    int                last_update = 0;
    int                clock;

    if (argc <= 1 || strcmp(argv[1], "--help") == 0) {
        printf("Syntax of the arb_db_server:\n");
        printf("        arb_db_server [-D]database [-Tsocketid]\n");
        return -1;
    }

    params = arb_trace_argv(&argc, argv);
    if (!params->tcp) {
        static char arg[] = ":";
        params->tcp = arg;
    }

    if (argc==2) {
        params->db_server = (char *)strdup(argv[1]);
        argc--;
    }

    //

    gb_extern = GB_open(params->tcp, "rwc");
    if (gb_extern) {
        GB_close(gb_extern);
        return -1;
    }
    printf("Loading '%s'...\n", params->db_server);
    gb_main = GB_open(params->db_server, "rw");
    if (!gb_main) {
        printf("Error: File %s not found\n", params->db_server);
        return -1;
    }
    error = GBCMS_open(params->tcp, TIMEOUT, gb_main);
    if (error) {
        printf("Cannot start your arb_db_server: %s\n", error);
        return (-1);
    }

    printf("Database is saved after every transaction (one . one save)\n");

    for (timer = LOOPS; timer>0; timer--) { /* the server ends */
        GB_begin_transaction(gb_main);
        clock = GB_read_clock(gb_main);
        if (GB_read_clients(gb_main) >0) {
            timer = LOOPS;
        }
        GB_commit_transaction(gb_main);
        if (clock>last_update) {
            last_update = clock;
            printf("."); fflush(stdout);
            GB_save(gb_main, 0, "b");
            timer = LOOPS;
        }
        GBCMS_accept_calls(gb_main, false);
    }
    return (0);
}

extern "C" {
    int aisc_open() { return 0; }
    int aisc_close() { return 0; }
}
