// ============================================================= //
//                                                               //
//   File      : DI_main.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <servercntrl.h>
#include <arbdb.h>
#include <awt.hxx>

#include <aw_preset.hxx>
#include <aw_root.hxx>
#include <macros.hxx>
#include <aw_question.hxx>
#include <awt_TreeAwars.hxx>


AW_HEADER_MAIN

AW_window *DI_create_matrix_window(AW_root *aw_root);
void       DI_create_matrix_variables(AW_root *aw_root, AW_default aw_def, AW_default db);

GBDATA *GLOBAL_gb_main; // global gb_main for arb_dist


static unsigned DI_timer(AW_root*, GBDATA *gb_main) {
    {
        GB_transaction ta(gb_main);
        GB_tell_server_dont_wait(gb_main); // trigger database callbacks
    }
    return 500; // recall after 500 ms
}

int ARB_main(int argc, char *argv[]) {
    aw_initstatus();

    GB_shell shell;
    AW_root *aw_root = AWT_create_root("dist.arb", "ARB_DIST", need_macro_ability(), &argc, &argv);

    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "Usage: arb_dist\n"
                "Is called from ARB.\n"
                );
        exit(EXIT_FAILURE);
    }

    GB_ERROR error = NULL;
    {
        arb_params *params = arb_trace_argv(&argc, (const char **)argv);
        if (argc==2) {
            freedup(params->db_server, argv[1]);
        }
        GLOBAL_gb_main = GB_open(params->db_server, "rw");
        if (!GLOBAL_gb_main) {
            error = GB_await_error();
        }
        else {
            error = configure_macro_recording(aw_root, "ARB_DIST", GLOBAL_gb_main);

#if defined(DEBUG)
            if (!error) AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB-database (%s)", params->db_server));
#endif // DEBUG
        }
        free_arb_params(params);
    }

    if (!error) {
        ARB_init_global_awars(aw_root, AW_ROOT_DEFAULT, GLOBAL_gb_main);
        AWT_initTreeAwarRegistry(GLOBAL_gb_main);
        DI_create_matrix_variables(aw_root, AW_ROOT_DEFAULT, GLOBAL_gb_main);

        AW_window *aww = DI_create_matrix_window(aw_root);
        aww->show();

        AWT_install_cb_guards();

        aw_root->add_timed_callback(2000, makeTimedCallback(DI_timer, GLOBAL_gb_main));
        aw_root->main_loop();
    }

    if (error) aw_popup_exit(error);
    return EXIT_SUCCESS;
}

