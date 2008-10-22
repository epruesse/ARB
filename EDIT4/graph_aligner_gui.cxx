#include <stdio.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt_sel_boxes.hxx>
#include <servercntrl.h>
#include <PT_com.h>
#include <client.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include <string.h>

// need to use AWTC_faligner_cd defined here to get selected species
#include <awtc_fast_aligner.hxx>
#include "graph_aligner_gui.hxx"
#include "ed4_defs.hxx"

using std::cerr;
using std::cout;
using std::endl;

#define GA_AWAR_ROOT "galigner/"
#define GA_AWAR_TGT GA_AWAR_ROOT "target"
#define GA_AWAR_SAI GA_AWAR_ROOT "sai"
#define GA_AWAR_ALIGNMENT GA_AWAR_ROOT "alignment"
#define GA_AWAR_PROTECTION GA_AWAR_ROOT "protection"
#define GA_AWAR_TURN_CHECK GA_AWAR_ROOT "turncheck"
#define GA_AWAR_LOGLEVEL GA_AWAR_ROOT "loglevel"
#define GA_AWAR_REALIGN GA_AWAR_ROOT "realign"
#define GA_AWAR_PTLOAD GA_AWAR_ROOT "ptload"
#define GA_AWAR_COPYMARKREF GA_AWAR_ROOT "copymarkref"
#define GA_AWAR_GAP_PEN GA_AWAR_ROOT "gap_pen"
#define GA_AWAR_GAP_EXT GA_AWAR_ROOT "gap_ext"
#define GA_AWAR_ADVANCED GA_AWAR_ROOT "advanced"
#define GA_AWAR_FS_MIN GA_AWAR_ROOT "fs_min"
#define GA_AWAR_FS_MAX GA_AWAR_ROOT "fs_max"
#define GA_AWAR_FS_MSC GA_AWAR_ROOT "fs_msc"
#define GA_AWAR_MIN_FULL GA_AWAR_ROOT "min_full"
#define GA_AWAR_FULL_MINLEN GA_AWAR_ROOT "full_minlen"
#define GA_AWAR_OVERHANG GA_AWAR_ROOT "overhang"
#define GA_AWAR_THREADS GA_AWAR_ROOT "threads"
#define GA_AWAR_QSIZE GA_AWAR_ROOT "qsize"

void create_galigner_variables(AW_root *root, AW_default db1) {
    root->awar_int(GA_AWAR_TGT, 2, db1);
    root->awar_int(AWAR_PT_SERVER, 0, db1);
    root->awar_string(GA_AWAR_SAI, "none", db1);
    root->awar_int(GA_AWAR_PROTECTION, 0, db1);
    root->awar_string(GA_AWAR_LOGLEVEL, "normal", db1);
    root->awar_int(GA_AWAR_TURN_CHECK, 1, db1);
    root->awar_int(GA_AWAR_REALIGN, 1, db1);
    root->awar_int(GA_AWAR_PTLOAD, 0, db1);
    root->awar_int(GA_AWAR_COPYMARKREF, 0, db1);
    root->awar_float(GA_AWAR_GAP_PEN, 5.0, db1);
    root->awar_float(GA_AWAR_GAP_EXT, 2.0, db1);
    root->awar_int(GA_AWAR_ADVANCED, 0, db1);
    root->awar_int(GA_AWAR_FS_MIN, 15, db1);
    root->awar_int(GA_AWAR_FS_MAX, 40, db1);
    root->awar_float(GA_AWAR_FS_MSC, .7, db1);
    root->awar_int(GA_AWAR_MIN_FULL, 1, db1);
    root->awar_int(GA_AWAR_FULL_MINLEN, 1400, db1);
    root->awar_string(GA_AWAR_OVERHANG, "attach", db1);
    root->awar_int(GA_AWAR_THREADS, 1, db1);
    root->awar_int(GA_AWAR_QSIZE, 1, db1);
}

int galign_mask() {
    char       *galign      = GB_executable("galign");
    const char *fail_reason = 0;

    if (galign) {
        int       galign_vers = system("galign --int-version");
        const int expected    = 20;

        galign_vers = WEXITSTATUS(galign_vers);
        free(galign);

        if (galign_vers == expected) {
            printf("Found galign\n");
            return AWM_ALL;
        }
        fail_reason = GBS_global_string("galign version mismatch (found=%i, expected=%i)", galign_vers, expected);
    }
    else {
        fail_reason = "galign not found";
    }

    printf("Note: SILVA Aligner disabled (%s)\n", fail_reason);
    // @@@ print note about download possibility ?

    return AWM_DISABLED;
}

static void galigner_start(AW_window *window, AW_CL cd2) {
    AW_root *root = window->get_root();
    cerr << "Starting Aligner..." << endl;

    // make string from pt server selection
    int pt = root->awar(AWAR_PT_SERVER)->read_int();
    std::stringstream tmp;
    tmp << "ARB_PT_SERVER" << pt;
    const char *pt_server = GBS_read_arb_tcp(tmp.str().c_str());
    const char *pt_db = pt_server + strlen(pt_server) +1;
    pt_db += strlen(pt_db)+3;
    GB_ERROR gb_error;

    // start pt server if neccessary
    gb_error = arb_look_and_start_server(AISC_MAGIC_NUMBER,tmp.str().c_str(),
                                         GLOBAL_gb_main);
    if (gb_error) {
        cerr << "Canot contact PT server. Aborting" << endl;
        cerr << " ID: \"" << tmp.str().c_str()
             << "\" PORT: \"" << pt_server
             << "\" DB: \"" << pt_db  << endl
             << "\" GBERROR: \"" << gb_error << "\"" << endl;
        GB_export_error("Cannot contact PT server");

        return;
    }

    const char *turn_check;
    if (root->awar(GA_AWAR_TURN_CHECK)->read_int())
        turn_check = "all";
    else
        turn_check = "none";

    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        cerr << "Unable to create pipe! Aborting." << endl;
        //fixme do errorhandling
        return;
    }

    pid_t me = fork();
    if (!me) { // we are child
        close(pipe_fds[1]); // close write end of pipe
        dup2(pipe_fds[0], STDIN_FILENO); // copy read end over stdin

        if (execlp("galign", "galign",
                   "--arb-db", ":",
                   "--pt-port", pt_server,
                   "--pt-db", pt_db,
                   "--no-pt-start",
                   root->awar(GA_AWAR_PTLOAD)->read_int()?"--pt-load":"",
                   root->awar(GA_AWAR_COPYMARKREF)->read_int()?"--copymarkref":"",
                   "--arb-filter",
                   root->awar(GA_AWAR_SAI)->read_string(),
                   "--input-type", "arb",
                   "--turn", turn_check,
                   "--print",
                   root->awar(GA_AWAR_LOGLEVEL)->read_string(),
                   root->awar(GA_AWAR_REALIGN)->read_int()?"--realign":"",
                   "--protection-level",
                   root->awar(GA_AWAR_PROTECTION)->read_string(),
                   "--penalties",
                   root->awar(GA_AWAR_GAP_PEN)->read_string(),
                   root->awar(GA_AWAR_GAP_EXT)->read_string(),
                   "--familyparam",
                   root->awar(GA_AWAR_FS_MIN)->read_string(),
                   root->awar(GA_AWAR_FS_MAX)->read_string(),
                   root->awar(GA_AWAR_FS_MSC)->read_string(),
                   "--overhang-placement",
                   root->awar(GA_AWAR_OVERHANG)->read_string(),
                   "--ncpu",
                   root->awar(GA_AWAR_THREADS)->read_string(),
                   "--queue-size",
                   root->awar(GA_AWAR_QSIZE)->read_string(),
                   "--min-full",
                   root->awar(GA_AWAR_MIN_FULL)->read_string(),
                   "--full-minlen",
                   root->awar(GA_AWAR_FULL_MINLEN)->read_string(),
#ifdef DEBUG
                   "--arb-debug",
#endif
                   (char*) NULL) == -1) {
            perror("Executing galign failed");
        }
        close (pipe_fds[0]);
        _exit(0); // kill this process, but don't remove tempfiles, etc.
    }
    close(pipe_fds[0]); // close read end of pipe;

    std::vector<std::string> spec_names;
    switch(root->awar(GA_AWAR_TGT)->read_int()) {
    case 0: //current
    {
        char *spec_name = root->awar(AWAR_SPECIES_NAME)->read_string();
        if (spec_name)
            spec_names.push_back(std::string(spec_name));
    }
    break;
    case 1: //selected
    {
        struct AWTC_faligner_cd *cd = (struct AWTC_faligner_cd *)cd2;
        int num_selected = 0;
        GB_begin_transaction(GLOBAL_gb_main);
        for (GBDATA *gb_spec = cd->get_first_selected_species(&num_selected);
             gb_spec; gb_spec = cd->get_next_selected_species()) {
            GBDATA *gbd_name = GB_find(gb_spec, "name", down_level);
            if (gbd_name) {
                const char *str = GB_read_char_pntr(gbd_name);
                spec_names.push_back(std::string(str));
            }
        }
        GB_commit_transaction(GLOBAL_gb_main);
    }
    break;
    case 2: //marked
    {
        GB_begin_transaction(GLOBAL_gb_main);
        for (GBDATA *gb_spec = GBT_first_marked_species(GLOBAL_gb_main);
             gb_spec; gb_spec = GBT_next_marked_species(gb_spec)) {
            GBDATA *gbd_name = GB_find(gb_spec, "name", down_level);
            if (gbd_name) {
                const char *str = GB_read_char_pntr(gbd_name);
                spec_names.push_back(std::string(str));
            }
        }
        GB_commit_transaction(GLOBAL_gb_main);
    }
    break;
    }

    for(std::vector<std::string>::iterator it  = spec_names.begin();
        it != spec_names.end(); ++it) {
        write(pipe_fds[1], it->c_str(), it->size());
        write(pipe_fds[1], "\n", 1);
    }

    close(pipe_fds[1]);
    waitpid(me, NULL, 0);
    cerr << "ARB: Finished Aligning.";
}



static char* filter_sai(GBDATA *gb_extended, AW_CL /*cd*/) {
    char   *result = 0;
    GBDATA *gbd    = GB_search(gb_extended, "ali_16s/_TYPE", GB_FIND);
    if (gbd) {
        const char* type = GB_read_char_pntr(gbd);
        if (type && strncmp("PV", type, 2) == 0) {
            gbd    = GB_find(gb_extended, "name", down_level);
            result = GB_read_string(gbd);
        }
    }
    return result;
}

static AW_window* create_select_sai_window(AW_root *root) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    aws = new AW_window_simple;

    aws->init(root, "SELECT_SAI", "Select Sai");
    aws->load_xfig("selectSAI.fig");

    aws->at("selection");
    aws->callback((AW_CB0)AW_POPDOWN);

    awt_create_selection_list_on_extendeds(GLOBAL_gb_main, (AW_window*) aws,
                                           GA_AWAR_SAI, filter_sai, (AW_CL)root);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->window_fit();
    return (AW_window*) aws;
}

AW_window_simple*
new_galigner_simple(AW_root *root, AW_CL cd2, bool adv) {
    int closex, closey, startx, starty, winx, winy;
    const int hgap = 10;
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "galigner", "Graph Aligner");

    aws->button_length(12);
    aws->at(10,10);
    aws->auto_space(5,5);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "O");
    aws->get_at_position(&closex, &closey);

    aws->at_shift(10,0);
    aws->callback(show_galigner_window, cd2, 0);
    aws->label_length(0);
    aws->label("Show advanced options");
    aws->create_toggle(GA_AWAR_ADVANCED);

    aws->at_newline();
    aws->at_shift(0,hgap);
    aws->label_length(15);
    aws->create_toggle_field(GA_AWAR_TGT, "Align what?", "A");
    aws->insert_toggle("Current Species:", "C", 0);
    aws->insert_toggle("Selected Species", "S", 1);
    aws->insert_default_toggle("Marked Species", "M", 2);
    aws->update_toggle_field();

    aws->at_shift(0,3);
    aws->create_input_field(AWAR_SPECIES_NAME, 20);

    aws->at_newline();
    aws->at_shift(0,hgap);
    aws->button_length(24);
    aws->label("PT Server:");
    awt_create_selection_list_on_pt_servers(aws, AWAR_PT_SERVER, AW_TRUE);

    aws->at_newline();
    aws->callback(AW_POPUP, (AW_CL)create_select_sai_window, (AW_CL)0);
    aws->label("Pos. Var.:");
    aws->create_button("SELECT_SAI", GA_AWAR_SAI);
    aws->button_length(12);


    aws->at_newline();
    aws->label_length(0);
    aws->create_option_menu(GA_AWAR_OVERHANG, "Overhang placement");
    aws->insert_option("keep attached", 0, "attach");
    aws->insert_option("move to edge", 0, "edge");
    aws->insert_option("remove", 0, "remove");
    aws->update_option_menu();

    if (adv) {
        aws->at_newline();
        aws->at_shift(0,hgap);

        aws->label("Turn check");
        aws->create_toggle(GA_AWAR_TURN_CHECK);

        aws->at_newline();
        aws->label("Realign");
        aws->create_toggle(GA_AWAR_REALIGN);

        aws->at_newline();
        aws->label("Load reference sequences from PT-Server");
        aws->create_toggle(GA_AWAR_PTLOAD);

        aws->at_newline();
        aws->label("(Copy and) mark sequences used as reference");
        aws->create_toggle(GA_AWAR_COPYMARKREF);

        aws->at_newline();
        aws->at_shift(0,hgap);
        aws->label_length(0);

        aws->label("Gap insertion/extension penalties");
        aws->create_input_field(GA_AWAR_GAP_PEN, 5);
        aws->create_input_field(GA_AWAR_GAP_EXT, 5);

        aws->at_newline();
        aws->label("Family search min/min_score/max");
        aws->create_input_field(GA_AWAR_FS_MIN, 3);
        aws->create_input_field(GA_AWAR_FS_MSC, 3);
        aws->create_input_field(GA_AWAR_FS_MAX, 3);

        aws->at_newline();
        aws->label("Use at least");
        aws->create_input_field(GA_AWAR_MIN_FULL, 3);
        aws->label("sequences with at least");
        aws->create_input_field(GA_AWAR_FULL_MINLEN, 5);
        aws->label("bases as reference");

        aws->at_newline();
        aws->label("Aligner threads");
        aws->create_input_field(GA_AWAR_THREADS,3);
        aws->label("Queue size");
        aws->create_input_field(GA_AWAR_QSIZE,3);

        aws->at_shift(0,hgap);
    }

    aws->at_newline();
    aws->at_shift(0,hgap);

    aws->label_length(17);
    aws->create_option_menu(GA_AWAR_PROTECTION, "Protection Level", "P");
    aws->insert_option("0", 0, 0);
    aws->insert_option("1", 0, 1);
    aws->insert_option("2", 0, 2);
    aws->insert_option("3", 0, 3);
    aws->insert_option("4", 0, 4);
    aws->insert_option("5", 0, 5);
    aws->insert_option("6", 0, 6);
    aws->update_option_menu();

    aws->at_newline();
    aws->create_option_menu(GA_AWAR_LOGLEVEL, "Logging level", "L");
    aws->insert_option("silent", 0, "silent");
    aws->insert_option("quiet", 0, "quiet");
    aws->insert_default_option("normal", 0, "normal");
    aws->insert_option("verbose", 0, "verbose");
    aws->insert_option("debug", 0, "debug");
    aws->insert_option("debug_graph", 0, "debug_graph");
    aws->update_option_menu();

    aws->get_window_size(winx, winy);
    aws->get_at_position(&startx, &starty);

    aws->at(winx-closex+5,closey);
    aws->callback(AW_POPUP_HELP, (AW_CL) "galign_main.hlp");
    aws->create_button("HELP", "HELP");

    aws->at(winx-closex+5, starty);
    aws->callback(galigner_start, cd2);
    aws->highlight();
    aws->create_button("Start", "Start", "S");

    return aws;
}

void show_galigner_window(AW_window *aw, AW_CL cd2, AW_CL) {
    static AW_window_simple *ga_aws = 0;
    static AW_window_simple *ga_aws_adv = 0;

    AW_root *root = aw->get_root();
    if (root->awar(GA_AWAR_ADVANCED)->read_int()) {
        if (!ga_aws_adv)
            ga_aws_adv = new_galigner_simple(root, cd2, true);
        ga_aws_adv->show();
        if (ga_aws)
            ga_aws->hide();
    } else {
        if (!ga_aws)
            ga_aws = new_galigner_simple(root, cd2, false);
        ga_aws->show();
        if (ga_aws_adv)
            ga_aws_adv->hide();
    }
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :

