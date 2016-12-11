// ================================================================ //
//                                                                  //
//   File      : arb_write_tree_comment.cxx                         //
//   Purpose   : append text to comment of existing tree            //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2016   //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arbdbt.h>


static void show_message(GBDATA *gb_main, const char *msg) {
    if (gb_main) {
        GBT_message(gb_main, msg);
    }
    else {
        fflush(stdout);
        printf("arb_write_tree_comment: %s\n", msg);
    }
}
static void show_error(GBDATA *gb_main, GB_ERROR error) {
    if (error) show_message(gb_main, GBS_global_string("Error running arb_write_tree_comment (%s)", error));
}

static void error_with_usage(GBDATA *gb_main, GB_ERROR error) {
    fputs("Usage: arb_write_tree_comment [options] treeName textToAppend\n"
          "Purpose: appends 'textToAppend' to comment of existing tree 'treeName'\n"
          "Available options:\n"
          "    -db database savename    specify database and savename (default is 'running ARB')\n"
          "    -plain                   do NOT prefix timestamp before textToAppend\n"
          , stdout);

    show_error(gb_main, error);
}

struct parameters {
    const char *dbname;
    const char *dbsavename;
    const char *tree_name;
    const char *text;

    bool stamp;

    parameters()
        : dbname(":"),
          dbsavename(NULL),
          tree_name(NULL),
          text(NULL),
          stamp(true)
    {
    }

#define SHIFT_ARGS(off) do { argc -= off; argv += off; } while (0)
#define SHIFT_NONSWITCHES(off) do { nonSwitches -= off; nonSwitch += off; } while (0)

    GB_ERROR scan(int argc, char **argv) {
        GB_ERROR error = NULL;

        const char  *nonSwitch_buf[20];
        const char **nonSwitch   = nonSwitch_buf;
        int          nonSwitches = 0;

        SHIFT_ARGS(1);              // position onto first argument

        while (argc>0 && !error) {
            if (strcmp("-db", argv[0]) == 0) {
                if (argc<3) error = "-db expects two arguments (database and savename)";
                else {
                    dbname     = argv[1];
                    dbsavename = argv[2];
                    SHIFT_ARGS(3);
                }
            }
            else if (strcmp("-plain", argv[0]) == 0) {
                stamp = false;
                SHIFT_ARGS(1);
            }
            else {
                nonSwitch[nonSwitches++] = argv[0];
                SHIFT_ARGS(1);
            }
        }

        if (!error) {
            if (!nonSwitches) error = "Missing argument 'treeName'";
            else {
                tree_name = nonSwitch[0];
                SHIFT_NONSWITCHES(1);
            }
        }
        if (!error) {
            if (!nonSwitches) error = "Missing argument 'textToAppend'";
            else {
                text = nonSwitch[0];
                SHIFT_NONSWITCHES(1);
            }
        }
        if (!error && nonSwitches>0) {
            error = GBS_global_string("unexpected argument(s): %s ..", nonSwitch[0]);
        }
        return error;
    }
};

int main(int argc, char **argv) {
    parameters param;
    GB_ERROR   error = param.scan(argc, argv);

    GBDATA   *gb_main      = NULL;
    GBDATA   *gb_msg_main  = NULL;
    bool      connectToArb = strcmp(param.dbname, ":") == 0;
    GB_shell  shell;

    if (!error || connectToArb) {
        gb_main                       = GB_open(param.dbname, connectToArb ? "r" : "rw");
        if (connectToArb) gb_msg_main = gb_main;
    }

    if (error) error_with_usage(gb_main, error);
    else {
        if (!gb_main) {
            if (connectToArb) error = "you have to start an arbdb server first";
            else error = GBS_global_string("can't open db (Reason: %s)", GB_await_error());
        }

        if (!error) {
            error = GBT_log_to_tree_remark(gb_main, param.tree_name, param.text, param.stamp);
        }
    }

    if (gb_main) {
        if (!error && !connectToArb) {
            error = GB_save_as(gb_main, param.dbsavename, "a");
            if (error) show_error(gb_main, error);
        }
        GB_close(gb_main);
    }

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
