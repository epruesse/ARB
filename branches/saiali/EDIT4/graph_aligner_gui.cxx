// =============================================================== //
//                                                                 //
//   File      : graph_aligner_gui.cxx                             //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Elmar Pruesse in October 2008                        //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "graph_aligner_gui.hxx"

#include "ed4_defs.hxx"

// need to use AlignDataAccess defined here to get selected species
#include <fast_aligner.hxx>

#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <awt_sel_boxes.hxx>
#include <servercntrl.h>
#include <PT_com.h>
#include <client.h>
#include <arbdbt.h>
#include <arb_strbuf.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>


using std::cerr;
using std::cout;
using std::endl;

#define GA_AWAR_ROOT "sina/"
#define GA_AWAR_CMD GA_AWAR_ROOT "command"
#define GA_AWAR_TGT GA_AWAR_ROOT "target"
#define GA_AWAR_SAI GA_AWAR_ROOT "sai"
#define GA_AWAR_ALIGNMENT GA_AWAR_ROOT "alignment"
#define GA_AWAR_PROTECTION GA_AWAR_ROOT "protection"
#define GA_AWAR_TURN_CHECK GA_AWAR_ROOT "turncheck"
#define GA_AWAR_LOGLEVEL GA_AWAR_ROOT "loglevel"
#define GA_AWAR_REALIGN GA_AWAR_ROOT "realign"
#define GA_AWAR_PTLOAD GA_AWAR_ROOT "ptload"
#define GA_AWAR_COPYMARKREF GA_AWAR_ROOT "copymarkref"
#define GA_AWAR_MATCH_SCORE GA_AWAR_ROOT "match_score"
#define GA_AWAR_MISMATCH_SCORE GA_AWAR_ROOT "mismatch_score"
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
#define GA_AWAR_KMER_LEN GA_AWAR_ROOT "kmer_len"
#define GA_AWAR_KMER_MM GA_AWAR_ROOT "kmer_mm"
#define GA_AWAR_MIN_LEN GA_AWAR_ROOT "min_len"
#define GA_AWAR_WEIGHT GA_AWAR_ROOT "weight"
#define GA_AWAR_INSERT GA_AWAR_ROOT "insert"
#define GA_AWAR_LOWERCASE GA_AWAR_ROOT "lowercase"
#define GA_AWAR_AUTOFILTER GA_AWAR_ROOT "autofilter"
#define GA_AWAR_KMER_NOREL GA_AWAR_ROOT "kmer_norel"
#define GA_AWAR_KMER_NOFAST GA_AWAR_ROOT "kmer_nofast"
#define GA_AWAR_SHOW_DIST GA_AWAR_ROOT "show_dist"
#define GA_AWAR_SHOW_DIFF GA_AWAR_ROOT "show_diff"
#define GA_AWAR_COLOR GA_AWAR_ROOT "color"
#define GA_AWAR_GENE_START GA_AWAR_ROOT "gene_start"
#define GA_AWAR_GENE_END GA_AWAR_ROOT "gene_end"
#define GA_AWAR_FS_COVER_GENE GA_AWAR_ROOT "fs_cover_gene"

void create_sina_variables(AW_root *root, AW_default db1) {
    root->awar_string(GA_AWAR_CMD, "sina", db1);
    root->awar_int(GA_AWAR_TGT, 2, db1);
    root->awar_int(AWAR_PT_SERVER, 0, db1);
    root->awar_string(GA_AWAR_SAI, "none", db1);
    root->awar_int(GA_AWAR_PROTECTION, 0, db1);
    root->awar_string(GA_AWAR_LOGLEVEL, "3", db1); // @@@ change to int?
    root->awar_int(GA_AWAR_TURN_CHECK, 1, db1);
    root->awar_int(GA_AWAR_REALIGN, 1, db1);
    root->awar_int(GA_AWAR_PTLOAD, 0, db1);
    root->awar_int(GA_AWAR_COPYMARKREF, 0, db1);
    root->awar_float(GA_AWAR_GAP_PEN, 5.0, db1);
    root->awar_float(GA_AWAR_GAP_EXT, 2.0, db1);
    root->awar_float(GA_AWAR_MATCH_SCORE, 2.0, db1);
    root->awar_float(GA_AWAR_MISMATCH_SCORE, -1.0, db1);
    root->awar_int(GA_AWAR_ADVANCED, 0, db1);
    root->awar_int(GA_AWAR_FS_MIN, 40, db1);
    root->awar_int(GA_AWAR_FS_MAX, 40, db1);
    root->awar_float(GA_AWAR_FS_MSC, .7, db1);
    root->awar_int(GA_AWAR_MIN_FULL, 1, db1);
    root->awar_int(GA_AWAR_FULL_MINLEN, 1400, db1);
    root->awar_string(GA_AWAR_OVERHANG, "attach", db1);
    root->awar_int(GA_AWAR_THREADS, 1, db1);
    root->awar_int(GA_AWAR_QSIZE, 1, db1);
    root->awar_int(GA_AWAR_KMER_LEN, 10, db1);
    root->awar_int(GA_AWAR_KMER_MM, 0, db1);
    root->awar_int(GA_AWAR_MIN_LEN, 150, db1);
    root->awar_float(GA_AWAR_WEIGHT, 1, db1);
    root->awar_string(GA_AWAR_INSERT, "shift", db1);
    root->awar_string(GA_AWAR_LOWERCASE, "none", db1);
    root->awar_string(GA_AWAR_AUTOFILTER, "none", db1);
    root->awar_int(GA_AWAR_KMER_NOREL, 0, db1);
    root->awar_int(GA_AWAR_KMER_NOFAST, 0, db1);
    root->awar_int(GA_AWAR_SHOW_DIST, 0, db1);
    root->awar_int(GA_AWAR_SHOW_DIFF, 0, db1);
    root->awar_int(GA_AWAR_COLOR, 1, db1);
    root->awar_int(GA_AWAR_GENE_START, 0, db1);
    root->awar_int(GA_AWAR_GENE_END, 0, db1);
    root->awar_int(GA_AWAR_FS_COVER_GENE, 1, db1);
}

AW_active sina_mask(AW_root *root) {
    const char *sinaName    = root->awar(GA_AWAR_CMD)->read_char_pntr();
    char       *sina        = GB_executable(sinaName);
    const char *fail_reason = 0;

    if (sina) {
        int exitstatus = system(GBS_global_string("%s --has-cli-vers ARB5.99", sina));
        exitstatus     = WEXITSTATUS(exitstatus);

        switch (exitstatus) {
        case EXIT_SUCCESS:
            break;

        case EXIT_FAILURE:
            fail_reason = "Incompatible SINA and ARB versions";
            break;

        case 127:                                   // libs missing
        default:                                    // unexpected
            fail_reason = GBS_global_string("Could execute SINA binary '%s' (exitstatus was %i)",
                                            sinaName, exitstatus);
            break;
        }
        free(sina);
    }
    else {
        fail_reason = GBS_global_string("%s not found", sinaName);
    }

    if (fail_reason) {
        fprintf(stderr,
                "Note: SINA (SILVA Aligner) disabled (Reason: %s)\n"
                "      Visit http://www.ribocon.com/sina/ for more information.\n",
                fail_reason);
    }

    return fail_reason ? AWM_DISABLED : AWM_ALL;
}

inline const char *stream2static(const std::stringstream& str) {
    return GBS_global_string("%s", str.str().c_str());
}

static void sina_start(AW_window *window, AW_CL cl_AlignDataAccess) {
    cerr << "Starting SINA..." << endl;

    // make string from pt server selection
    AW_root *root = window->get_root();
    int      pt   = root->awar(AWAR_PT_SERVER)->read_int();

    std::stringstream ptnam;
    ptnam << "ARB_PT_SERVER" << pt;

    const char *pt_server = GBS_read_arb_tcp(ptnam.str().c_str());
    GB_ERROR    gb_error  = NULL;

    if (!pt_server) gb_error = "Unable to find definition for chosen PT-server";
    else {
        const char *pt_db = pt_server + strlen(pt_server) + 1;
        pt_db += strlen(pt_db)+3;

        // start pt server if necessary
        gb_error = arb_look_and_start_server(AISC_MAGIC_NUMBER, ptnam.str().c_str());
        if (gb_error) {
            std::stringstream tmp;
            tmp << "Cannot contact PT server. Aborting" << endl
                << " ID: \"" << tmp.str().c_str()
                << "\" PORT: \"" << pt_server
                << "\" DB: \"" << pt_db  << endl
                << "\" GBERROR: \"" << gb_error << "\"" << endl;
            gb_error = stream2static(tmp);
        }
        else {
            // create temporary file
            char* tmpfile;
            FILE* tmpfile_F;
            {
                char* tmpfile_tpl = GB_unique_filename("sina_select", "tmp");
                tmpfile_F         = GB_fopen_tempfile(tmpfile_tpl, "w", &tmpfile);
                free(tmpfile_tpl);
            }

            if (!tmpfile_F) {
                std::stringstream tmp;
                tmp << "Error: Unable to create temporary file \"" << tmpfile << "\"!";
                gb_error = stream2static(tmp);
            }
            else {
                GB_remove_on_exit(tmpfile);

                std::vector<std::string> spec_names;
                switch (root->awar(GA_AWAR_TGT)->read_int()) {
                    case 0: { // current
                        char *spec_name = root->awar(AWAR_SPECIES_NAME)->read_string();
                        if (spec_name) {
                            fwrite(spec_name, strlen(spec_name), 1, tmpfile_F);
                            fwrite("\n", 1, 1, tmpfile_F);
                        }
                        else {
                            gb_error = "Unable to get name of currently active species";
                        }
                        free(spec_name);
                        break;
                    }
                    case 1: { // selected
                        const AlignDataAccess *data_access = (const AlignDataAccess *)cl_AlignDataAccess;
                        GB_begin_transaction(GLOBAL_gb_main);
                        int num_selected = 0;
                        for (GBDATA *gb_spec = data_access->get_first_selected_species(&num_selected);
                             gb_spec;
                             gb_spec = data_access->get_next_selected_species())
                        {
                            GBDATA *gbd_name = GB_find(gb_spec, "name", SEARCH_CHILD);
                            if (gbd_name) {
                                const char *str = GB_read_char_pntr(gbd_name);
                                fwrite(str, strlen(str), 1, tmpfile_F);
                                fwrite("\n", 1, 1, tmpfile_F);
                            }
                        }
                        GB_commit_transaction(GLOBAL_gb_main);
                        break;
                    }
                    case 2: { // marked
                        GB_begin_transaction(GLOBAL_gb_main);
                        for (GBDATA *gb_spec = GBT_first_marked_species(GLOBAL_gb_main);
                             gb_spec; gb_spec = GBT_next_marked_species(gb_spec)) {
                            GBDATA *gbd_name = GB_find(gb_spec, "name", SEARCH_CHILD);
                            if (gbd_name) {
                                const char *str = GB_read_char_pntr(gbd_name);
                                fwrite(str, strlen(str), 1, tmpfile_F);
                                fwrite("\n", 1, 1, tmpfile_F);
                            }
                        }
                        GB_commit_transaction(GLOBAL_gb_main);
                        break;
                    }
                }
                fclose(tmpfile_F);

                if (!gb_error) {
                    // build command line
                    GBS_strstruct *cl = GBS_stropen(2000);

                    GBS_strcat(cl, root->awar(GA_AWAR_CMD)->read_char_pntr());
                    GBS_strcat(cl, " -i :");
                    GBS_strcat(cl, " --ptdb ");        GBS_strcat(cl,   root->awar(GA_AWAR_PTLOAD)->read_int() ? pt_db : ":");
                    GBS_strcat(cl, " --ptport ");      GBS_strcat(cl,   pt_server);
                    GBS_strcat(cl, " --turn ");        GBS_strcat(cl,   root->awar(GA_AWAR_TURN_CHECK)->read_int() ? "all" : "none");
                    GBS_strcat(cl, " --overhang ");    GBS_strcat(cl,   root->awar(GA_AWAR_OVERHANG)->read_char_pntr());
                    GBS_strcat(cl, " --filter ");      GBS_strcat(cl,   root->awar(GA_AWAR_SAI)->read_char_pntr());
                    GBS_strcat(cl, " --fs-min ");      GBS_intcat(cl,   root->awar(GA_AWAR_FS_MIN)->read_int());
                    GBS_strcat(cl, " --fs-msc ");      GBS_floatcat(cl, root->awar(GA_AWAR_FS_MSC)->read_float());
                    GBS_strcat(cl, " --fs-max ");      GBS_intcat(cl,   root->awar(GA_AWAR_FS_MAX)->read_int());
                    GBS_strcat(cl, " --fs-req 1");
                    GBS_strcat(cl, " --fs-req-full "); GBS_intcat(cl,   root->awar(GA_AWAR_MIN_FULL)->read_int());
                    GBS_strcat(cl, " --fs-full-len "); GBS_intcat(cl,   root->awar(GA_AWAR_FULL_MINLEN)->read_int());
                    GBS_strcat(cl, " --fs-kmer-len "); GBS_intcat(cl,   root->awar(GA_AWAR_KMER_LEN)->read_int());
                    GBS_strcat(cl, " --fs-kmer-mm ");  GBS_intcat(cl,   root->awar(GA_AWAR_KMER_MM)->read_int());
                    GBS_strcat(cl, " --fs-min-len ");  GBS_intcat(cl,   root->awar(GA_AWAR_MIN_LEN)->read_int());
                    GBS_strcat(cl, " --fs-weight ");   GBS_intcat(cl,   root->awar(GA_AWAR_WEIGHT)->read_float());
                    GBS_strcat(cl, " --pen-gap ");     GBS_floatcat(cl, root->awar(GA_AWAR_GAP_PEN)->read_float());
                    GBS_strcat(cl, " --pen-gapext ");  GBS_floatcat(cl, root->awar(GA_AWAR_GAP_EXT)->read_float());
                    GBS_strcat(cl, " --match-score "); GBS_floatcat(cl, root->awar(GA_AWAR_MATCH_SCORE)->read_float());
                    GBS_strcat(cl, " --mismatch-score "); GBS_floatcat(cl, root->awar(GA_AWAR_MISMATCH_SCORE)->read_float());
                    GBS_strcat(cl, " --prot-level ");  GBS_intcat(cl,   root->awar(GA_AWAR_PROTECTION)->read_int());
                    GBS_strcat(cl, " --select-file "); GBS_strcat(cl,   tmpfile);
                    GBS_strcat(cl, " --insertion ");   GBS_strcat(cl,   root->awar(GA_AWAR_INSERT)->read_char_pntr());
                    GBS_strcat(cl, " --lowercase ");   GBS_strcat(cl,   root->awar(GA_AWAR_LOWERCASE)->read_char_pntr());
                    GBS_strcat(cl, " --auto-filter-field "); GBS_strcat(cl, root->awar(GA_AWAR_AUTOFILTER)->read_char_pntr());
                    GBS_strcat(cl, " --gene-start ");  GBS_intcat(cl,   root->awar(GA_AWAR_GENE_START)->read_int());
                    GBS_strcat(cl, " --gene-end ");    GBS_intcat(cl,   root->awar(GA_AWAR_GENE_END)->read_int());
                    GBS_strcat(cl, " --fs-cover-gene ");GBS_intcat(cl,   root->awar(GA_AWAR_FS_COVER_GENE)->read_int());


                    if (root->awar(GA_AWAR_KMER_NOREL)->read_int()) GBS_strcat(cl, " --fs-kmer-norel ");
                    if (root->awar(GA_AWAR_KMER_NOFAST)->read_int()) GBS_strcat(cl, " --fs-kmer-no-fast ");
                    if (root->awar(GA_AWAR_SHOW_DIST)->read_int()) GBS_strcat(cl, " --show-dist ");
                    if (root->awar(GA_AWAR_SHOW_DIFF)->read_int()) GBS_strcat(cl, " --show-diff ");
                    if (root->awar(GA_AWAR_COLOR)->read_int())     GBS_strcat(cl, " --color");
                    if (root->awar(GA_AWAR_REALIGN)->read_int())     GBS_strcat(cl, " --realign");

                    gb_error = GB_xcmd(GBS_mempntr(cl), true, false);
                    GBS_strforget(cl);
                }

                if (!gb_error) aw_message("SINA finished aligning.");
            }
            free(tmpfile);
        }
    }
    
    aw_message_if(gb_error);
}



static char* filter_sai(GBDATA *gb_extended, AW_CL) {
    char   *result = 0;
    GBDATA *gbd    = GB_search(gb_extended, "ali_16s/_TYPE", GB_FIND);
    if (gbd) {
        const char* type = GB_read_char_pntr(gbd);
        if (type && strncmp("PV", type, 2) == 0) {
            gbd    = GB_find(gb_extended, "name", SEARCH_CHILD);
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

static AW_window_simple* new_sina_simple(AW_root *root, AW_CL cl_AlignDataAccess, bool adv) {
    int closex, closey, startx, starty, winx, winy;
    const int hgap = 10;
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "SINA", "SINA (SILVA Incremental Aligner)");

    aws->button_length(12);
    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "O");
    aws->get_at_position(&closex, &closey);

    aws->at_shift(10, 0);
    aws->callback(show_sina_window, cl_AlignDataAccess, 0);
    aws->label_length(0);
    aws->label("Show advanced options");
    aws->create_toggle(GA_AWAR_ADVANCED);

    aws->at_newline();
    aws->at_shift(0, hgap);
    aws->label_length(15);
    aws->create_toggle_field(GA_AWAR_TGT, "Align what?", "A");
    aws->insert_toggle("Current Species:", "C", 0);
    aws->insert_toggle("Selected Species", "S", 1);
    aws->insert_default_toggle("Marked Species", "M", 2);
    aws->update_toggle_field();

    aws->at_shift(0, 3);
    aws->create_input_field(AWAR_SPECIES_NAME, 20);

    aws->at_newline();
    aws->at_shift(0, hgap);
    aws->button_length(24);
    aws->label("PT Server:");
    awt_create_selection_list_on_pt_servers(aws, AWAR_PT_SERVER, true);

    aws->at_newline();
    aws->label_length(0);
    aws->create_option_menu(GA_AWAR_OVERHANG, "Overhang placement");
    aws->insert_option("keep attached", 0, "attach");
    aws->insert_option("move to edge", 0, "edge");
    aws->insert_option("remove", 0, "remove");
    aws->update_option_menu();

    aws->at_newline();
    aws->create_option_menu(GA_AWAR_INSERT, "Handling of unmappable insertions", "I");
    aws->insert_option("Shift surrounding bases", 0, "shift");
    aws->insert_option("Forbid during DP alignment", 0, "forbid");
    aws->insert_option("Delete bases", 0, "remove");
    aws->update_option_menu();

    aws->at_newline();
    aws->create_option_menu(GA_AWAR_LOWERCASE, "Character Case","C");
    aws->insert_option("Do not modify", 0, "original");
    aws->insert_option("Show unaligned bases as lower case", 0, "unaligned");
    aws->insert_option("Uppercase all", 0, "none");
    aws->update_option_menu();

    aws->at_newline();
    aws->label("Family conservation weight (0-1)");
    aws->create_input_field(GA_AWAR_WEIGHT, 3);

    aws->at_newline();
    aws->label("Size of full-length sequences");
    aws->create_input_field(GA_AWAR_FULL_MINLEN, 5);

    if (adv) {
        aws->at_newline();
        aws->at_shift(0, hgap);

        aws->at_newline();
        aws->callback(AW_POPUP, (AW_CL)create_select_sai_window, (AW_CL)0);
        aws->label("Pos. Var.:");
        aws->create_button("SELECT_SAI", GA_AWAR_SAI);
        aws->button_length(12);

        aws->at_newline();
        aws->label("Field used for automatic filter selection");
        aws->create_input_field(GA_AWAR_AUTOFILTER, 20);

        aws->label("Turn check");
        aws->create_toggle(GA_AWAR_TURN_CHECK);

        aws->at_newline();
        aws->label("Realign");
        aws->create_toggle(GA_AWAR_REALIGN);

        aws->at_newline();
        aws->label("Load reference sequences from PT-Server");
        aws->create_toggle(GA_AWAR_PTLOAD);

        /*
        aws->at_newline();
        aws->label("(Copy and) mark sequences used as reference");
        aws->create_toggle(GA_AWAR_COPYMARKREF);
        */

        aws->at_newline();
        aws->at_shift(0, hgap);
        aws->label_length(0);

        aws->label("Gap insertion/extension penalties");
        aws->create_input_field(GA_AWAR_GAP_PEN, 5);
        aws->create_input_field(GA_AWAR_GAP_EXT, 5);

        aws->at_newline();
        aws->label("Match score");
        aws->create_input_field(GA_AWAR_MATCH_SCORE, 3);
        aws->label("Mismatch score");
        aws->create_input_field(GA_AWAR_MISMATCH_SCORE, 3);

        aws->at_newline();
        aws->label("Family search min/min_score/max");
        aws->create_input_field(GA_AWAR_FS_MIN, 3);
        aws->create_input_field(GA_AWAR_FS_MSC, 3);
        aws->create_input_field(GA_AWAR_FS_MAX, 3);

        aws->at_newline();
        aws->label("Minimal number of full length sequences");
        aws->create_input_field(GA_AWAR_MIN_FULL, 3);

        aws->at_newline();
        aws->label("Family search oligo length / mismatches");
        aws->create_input_field(GA_AWAR_KMER_LEN, 3);
        aws->create_input_field(GA_AWAR_KMER_MM, 3);

        aws->at_newline();
        aws->label("Minimal reference sequence length");
        aws->create_input_field(GA_AWAR_MIN_LEN, 5);

        aws->at_newline();
        aws->label("Alignment bounds: start");
        aws->create_input_field(GA_AWAR_GENE_START, 6);
        aws->label("end");
        aws->create_input_field(GA_AWAR_GENE_END, 6);

        aws->at_newline();
        aws->label("Number of references required to touch bounds");
        aws->create_input_field(GA_AWAR_FS_COVER_GENE, 3);

        aws->at_newline();
        aws->label("Disable fast search");
        aws->create_toggle(GA_AWAR_KMER_NOFAST);

        aws->at_newline();
        aws->label("Score search results by absolute oligo match count");
        aws->create_toggle(GA_AWAR_KMER_NOREL);

        aws->at_newline();
        aws->label("SINA command");
        aws->create_input_field(GA_AWAR_CMD, 20);

        aws->at_shift(0, hgap);
    }

    aws->at_newline();
    aws->at_shift(0, hgap);

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

    /*
    aws->at_newline();
    aws->create_option_menu(GA_AWAR_LOGLEVEL, "Logging level", "L");
    aws->insert_option("silent", 0, "1");
    aws->insert_option("quiet", 0, "2");
    aws->insert_default_option("normal", 0, "3");
    aws->insert_option("verbose", 0, "4");
    aws->insert_option("debug", 0, "5");
    aws->insert_option("debug more", 0, "6");
    aws->update_option_menu();
    */

    aws->at_newline();
    aws->label("Show changed sections of alignment");
    aws->create_toggle(GA_AWAR_SHOW_DIFF);
    aws->label("color bases");
    aws->create_toggle(GA_AWAR_COLOR);

    aws->at_newline();
    aws->label("Show statistics");
    aws->create_toggle(GA_AWAR_SHOW_DIST);

    aws->get_window_size(winx, winy);
    aws->get_at_position(&startx, &starty);

    aws->at(winx-closex+5, closey);
    aws->callback(AW_POPUP_HELP, (AW_CL) "sina_main.hlp");
    aws->create_button("HELP", "HELP");

    aws->at(winx-closex+5, starty);
    aws->callback(sina_start, cl_AlignDataAccess);
    aws->highlight();
    aws->create_button("Start", "Start", "S");

    return aws;
}

void show_sina_window(AW_window *aw, AW_CL cl_AlignDataAccess, AW_CL) {
    static AW_window_simple *ga_aws = 0;
    static AW_window_simple *ga_aws_adv = 0;

    AW_root *root = aw->get_root();
    if (root->awar(GA_AWAR_ADVANCED)->read_int()) {
        if (!ga_aws_adv) ga_aws_adv = new_sina_simple(root, cl_AlignDataAccess, true);
        ga_aws_adv->show();
        if (ga_aws) ga_aws->hide();
    }
    else {
        if (!ga_aws) ga_aws = new_sina_simple(root, cl_AlignDataAccess, false);
        ga_aws->show();
        if (ga_aws_adv) ga_aws_adv->hide();
    }
}

