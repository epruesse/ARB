// ================================================================ //
//                                                                  //
//   File      : WETC_main.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <awt.hxx>
#include <arbdb.h>

AW_HEADER_MAIN


int main(int argc, char **argv) {
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
            AW_root *aw_root = AWT_create_root(".arb_prop/ntree.arb", "ARB_WETC");

            AWT_show_file(aw_root, file);
            aw_root->window_hide();
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

