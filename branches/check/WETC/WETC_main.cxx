// ================================================================ //
//                                                                  //
//   File      : WETC_main.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arbdb.h>
#include <awt.hxx>
#include <aw_window.hxx>
#include <aw_root.hxx>

AW_HEADER_MAIN


int ARB_main(int argc, char *argv[]) {
    GB_shell shell;
    AW_root *aw_root = AWT_create_root("ntree.arb", "ARB_WETC", new NullTracker, &argc, &argv); // no macro recording here

    GB_ERROR error = NULL;

    if (argc != 3) {
        error = "Missing arguments";
    }
    else {
        const char *com  = argv[1];
        const char *file = argv[2];

        if (strcmp(com, "-fileedit") != 0) {
            error = GBS_global_string("Unexpected parameter '%s'", com);
        }
        else {
            AWT_show_file(aw_root, file);
            aw_root->window_hide(NULL);
            AWT_install_cb_guards();
            aw_root->main_loop();
        }
    }

    if (error) {
        fprintf(stderr,
                "Syntax: arb_wetc -fileedit filename\n"
                "Error: %s\n",
                error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

