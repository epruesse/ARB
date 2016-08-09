// ============================================================= //
//                                                               //
//   File      : ED4_main.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_nds.hxx"
#include "ed4_visualizeSAI.hxx"
#include "ed4_ProteinViewer.hxx"
#include "ed4_protein_2nd_structure.hxx"
#include "ed4_dots.hxx"
#include "ed4_naligner.hxx"
#include "ed4_seq_colors.hxx"
#include "graph_aligner_gui.hxx"
#include <ed4_extern.hxx>

#include <st_window.hxx>
#include <gde.hxx>
#include <AW_helix.hxx>
#include <AP_pro_a_nucs.hxx>
#include <ad_config.h>
#include <awt_map_key.hxx>
#include <awt.hxx>

#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_advice.hxx>

#include <arbdbt.h>

#include <arb_defs.h>
#include <arb_global_defs.h>
#include <macros.hxx>
#include <aw_question.hxx>

AW_HEADER_MAIN

ED4_root *ED4_ROOT;
GBDATA   *GLOBAL_gb_main = NULL; // global gb_main for arb_edit4

int TERMINALHEIGHT;

int INFO_TERM_TEXT_YOFFSET;
int SEQ_TERM_TEXT_YOFFSET;

int MAXSEQUENCECHARACTERLENGTH; // greatest # of characters in a sequence string terminal
int MAXSPECIESWIDTH;
int MAXINFOWIDTH;               // # of pixels used to display sequence info ("CONS", "4data", etc.)

long ED4_counter = 0;

size_t         not_found_counter; // nr of species which haven't been found
GBS_strstruct *not_found_message;

long         max_seq_terminal_length; // global maximum of sequence terminal length
ED4_EDITMODE awar_edit_mode;
long         awar_edit_rightward;
bool         move_cursor;             // only needed for editing in consensus
bool         DRAW;

inline void replaceChars(char *s, char o, char n) {
    while (1) {
        char c = *s++;
        if (!c) {
            break;
        }
        if (c==o) {
            s[-1] = n;
        }
    }
}

inline void set_and_realloc_gde_array(uchar **&the_names, uchar **&the_sequences, long &allocated, long &numberspecies, long &maxalign,
                                      const char *name, int name_len, const char *seq, int seq_len)
{
    if (allocated==numberspecies) {
        long new_allocated = (allocated*3)/2;

        ARB_recalloc(the_names, allocated, new_allocated);
        ARB_recalloc(the_sequences, allocated, new_allocated);
        allocated = new_allocated;
    }

    the_names[numberspecies]     = (uchar*)ARB_strndup(name, name_len);
    the_sequences[numberspecies] = (uchar*)ARB_strndup(seq, seq_len);

    replaceChars((char*)the_names[numberspecies], ' ', '_');

    if (seq_len>maxalign) {
        maxalign = seq_len;
    }

    numberspecies++;
}

static char *add_area_for_gde(ED4_area_manager *area_man, uchar **&the_names, uchar **&the_sequences,
                              long &allocated, long &numberspecies, long &maxalign,
                              int show_sequence, int show_SAI, int show_helix, int show_consensus, int show_remark)
{
    ED4_terminal *terminal = area_man->get_first_terminal();
    ED4_terminal *last     = area_man->get_last_terminal();

    for (; terminal;) {
        if (terminal->is_species_name_terminal()) {
            ED4_species_manager *species_manager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
            ED4_species_name_terminal *species_name = species_manager->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
            int name_len;
            char *name = species_name->resolve_pointer_to_string_copy(&name_len);
            ED4_sequence_terminal *sequence_terminal;

            {
                ED4_base *sequence_term  = species_manager->search_spec_child_rek(ED4_L_SEQUENCE_STRING);
                if (!sequence_term) goto end_of_loop;
                sequence_terminal = sequence_term->to_sequence_terminal();
            }

            int show = -1;

            bool is_consensus = false;
            bool is_SAI       = false;

            if (terminal->is_consensus_terminal()) {
                is_consensus = true;
                show         = show_consensus;
            }
            else if (terminal->is_SAI_terminal()) {
                is_SAI = true;
                show   = show_SAI;
            }
            else {
                show = show_sequence;
            }

            e4_assert(show!=-1);

            if (show) {
                int seq_len;
                char *seq = 0;

                if (is_consensus) {
                    ED4_group_manager *group_manager = sequence_terminal->get_parent(ED4_L_GROUP)->to_group_manager();

                    group_manager->build_consensus_string(&seq_len);
                    e4_assert(strlen(seq) == size_t(seq_len));

                    ED4_group_manager *folded_group_man = sequence_terminal->is_in_folded_group();

                    if (folded_group_man) { // we are in a folded group
                        if (folded_group_man==group_manager) { // we are the consensus of the folded group
                            if (folded_group_man->is_in_folded_group()) { // a folded group inside a folded group -> do not show
                                freenull(seq);
                            }
                            else { // group folded but consensus shown -> add '-' before name
                                char *new_name = (char*)ARB_alloc(name_len+2);

                                sprintf(new_name, "-%s", name);
                                freeset(name, new_name);
                                name_len++;
                            }
                        }
                        else { // we are really inside a folded group -> don't show
                            freenull(seq);
                        }
                    }
                }
                else { // sequence
                    if (!sequence_terminal->is_in_folded_group()) {
                        seq = sequence_terminal->resolve_pointer_to_string_copy(&seq_len);
                    }
                }

                if (seq) {
                    set_and_realloc_gde_array(the_names, the_sequences, allocated, numberspecies, maxalign, name, name_len, seq, seq_len);
                    if (show_helix && !is_SAI && ED4_ROOT->helix->size()) {
                        char *helix = ED4_ROOT->helix->seq_2_helix(seq, '.');
                        set_and_realloc_gde_array(the_names, the_sequences, allocated, numberspecies, maxalign, name, name_len, helix, seq_len);
                        free(helix);
                    }
                    if (show_remark && !is_consensus) {
                        ED4_multi_sequence_manager *ms_man = sequence_terminal->get_parent(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                        ED4_base *remark_name_term = ms_man->search_ID("remark");
                        if (remark_name_term) {
                            ED4_base *remark_term = remark_name_term->get_next_terminal();
                            e4_assert(remark_term);

                            int remark_len;
                            char *remark = remark_term->resolve_pointer_to_string_copy(&remark_len);

                            replaceChars(remark, ' ', '_');
                            set_and_realloc_gde_array(the_names, the_sequences, allocated, numberspecies, maxalign, name, name_len, remark, remark_len);
                            free(remark);
                        }
                    }
                    free(seq);
                }
            }
            free(name);
        }
    end_of_loop :
        if (terminal==last) break;
        terminal = terminal->get_next_terminal();
    }

    return 0;
}

static char *ED4_create_sequences_for_gde(GBDATA **&the_species, uchar **&the_names, uchar **&the_sequences, long &numberspecies, long &maxalign) {
    int top     = ED4_ROOT->aw_root->awar("gde/top_area")->read_int();
    int tops    = ED4_ROOT->aw_root->awar("gde/top_area_sai")->read_int();
    int toph    = ED4_ROOT->aw_root->awar("gde/top_area_helix")->read_int();
    int topk    = ED4_ROOT->aw_root->awar("gde/top_area_kons")->read_int();
    int topr    = ED4_ROOT->aw_root->awar("gde/top_area_remark")->read_int();
    int middle  = ED4_ROOT->aw_root->awar("gde/middle_area")->read_int();
    int middles = ED4_ROOT->aw_root->awar("gde/middle_area_sai")->read_int();
    int middleh = ED4_ROOT->aw_root->awar("gde/middle_area_helix")->read_int();
    int middlek = ED4_ROOT->aw_root->awar("gde/middle_area_kons")->read_int();
    int middler = ED4_ROOT->aw_root->awar("gde/middle_area_remark")->read_int();

    numberspecies = 0;
    maxalign = 0;

    long allocated = 100;
    the_species    = 0;

    the_names     = (uchar**)ARB_calloc(allocated, sizeof(*the_names));
    the_sequences = (uchar**)ARB_calloc(allocated, sizeof(*the_sequences));

    char *err = add_area_for_gde(ED4_ROOT->top_area_man, the_names, the_sequences, allocated, numberspecies, maxalign, top, tops, toph, topk, topr);
    if (!err) {
        err = add_area_for_gde(ED4_ROOT->middle_area_man, the_names, the_sequences, allocated, numberspecies, maxalign, middle, middles, middleh, middlek, middler);
    }

    if (allocated!=(numberspecies+1)) {
        ARB_recalloc(the_names, allocated, numberspecies+1);
        ARB_recalloc(the_sequences, allocated, numberspecies+1);
    }

    return err;
}

void ED4_setup_gaps_and_alitype(const char *gap_chars, GB_alignment_type alitype) {
    BaseFrequencies::setup(gap_chars, alitype);
}

static void ED4_gap_chars_changed(AW_root *root) {
    char *gap_chars = root->awar_string(ED4_AWAR_GAP_CHARS)->read_string();
    ED4_setup_gaps_and_alitype(gap_chars, ED4_ROOT->alignment_type);
    free(gap_chars);
}

void ED4_with_all_edit_windows(void (*cb)(ED4_window *)) {
    for (ED4_window *win = ED4_ROOT->first_window; win; win = win->next) {
        ED4_LocalWinContext uses(win);
        cb(win);
    }
}

static void redraw_cursor(ED4_window *win) { win->cursor.redraw(); }
static void ED4_edit_direction_changed(AW_root * /* awr */) {
    ED4_with_all_edit_windows(redraw_cursor);
}

static void ed4_bind_mainDB_awar_callbacks(AW_root *root) {
    // callbacks to main DB awars are bound later
    // (otherwise its easy to crash the editor by clicking around in ARB_NTREE during editor startup)

    root->awar(AWAR_SET_CURSOR_POSITION)->add_callback(ED4_remote_set_cursor_cb);
    root->awar(AWAR_SPECIES_NAME)->add_callback(ED4_selected_species_changed_cb);
    root->awar(AWAR_SAI_NAME)->add_callback(ED4_selected_SAI_changed_cb);
}

static void ed4_create_mainDB_awars(AW_root *root) {
    // WARNING: do not bind callbacks here -> do it in ed4_bind_mainDB_awar_callbacks()

    root->awar_string(AWAR_ITARGET_STRING, "", GLOBAL_gb_main);

    root->awar_int(AWAR_CURSOR_POSITION,       info2bio(0), GLOBAL_gb_main);
    root->awar_int(AWAR_CURSOR_POSITION_LOCAL, 0, GLOBAL_gb_main);
    root->awar_int(AWAR_SET_CURSOR_POSITION,   1, GLOBAL_gb_main);

    root->awar_string(AWAR_FIELD_CHOSEN, NO_FIELD_SELECTED, GLOBAL_gb_main);

    root->awar_string(AWAR_SPECIES_NAME, "", GLOBAL_gb_main);
    root->awar_string(AWAR_SAI_NAME,     "", GLOBAL_gb_main);
    root->awar_string(AWAR_SAI_GLOBAL,   "", GLOBAL_gb_main);
}

static void ed4_create_all_awars(AW_root *root, const char *config_name) {
    // Note: cursor awars are created in window constructor

    root->awar_string(AWAR_EDIT_CONFIGURATION, config_name, AW_ROOT_DEFAULT);

    ed4_create_mainDB_awars(root);
    ED4_create_search_awars(root);

#if defined(DEBUG)
    AWT_create_db_browser_awars(root, AW_ROOT_DEFAULT);
#endif // DEBUG

    create_naligner_variables(root, AW_ROOT_DEFAULT);
    create_sina_variables(root, AW_ROOT_DEFAULT);

    awar_edit_mode = AD_ALIGN;

    int def_sec_level = 0;
#ifdef DEBUG
    def_sec_level = 6; // don't nag developers
#endif
    root->awar_int(AWAR_EDIT_SECURITY_LEVEL, def_sec_level, AW_ROOT_DEFAULT);

    root->awar_int(AWAR_EDIT_SECURITY_LEVEL_ALIGN,  def_sec_level, AW_ROOT_DEFAULT)->add_callback(ed4_changesecurity);
    root->awar_int(AWAR_EDIT_SECURITY_LEVEL_CHANGE, def_sec_level, AW_ROOT_DEFAULT)->add_callback(ed4_changesecurity);

    root->awar_int(AWAR_EDIT_MODE,   0)->add_callback(ed4_change_edit_mode);
    root->awar_int(AWAR_INSERT_MODE, 1)->add_callback(ed4_change_edit_mode);

    root->awar_int(AWAR_EDIT_RIGHTWARD, 1)->add_target_var(&awar_edit_rightward)->add_callback(ED4_edit_direction_changed);
    root->awar_int(AWAR_EDIT_TITLE_MODE, 0);

    root->awar_int(AWAR_EDIT_HELIX_SPACING,    0) ->set_minmax(-30, 30) ->add_target_var(&ED4_ROOT->helix_add_spacing)    ->add_callback(makeRootCallback(ED4_request_relayout));
    root->awar_int(AWAR_EDIT_TERMINAL_SPACING, 0) ->set_minmax(-30, 30) ->add_target_var(&ED4_ROOT->terminal_add_spacing) ->add_callback(makeRootCallback(ED4_request_relayout));

    ed4_changesecurity(root);

    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_GAPS,    0)->add_callback(makeRootCallback(ED4_compression_toggle_changed_cb, false));
    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_HIDE,    0)->add_callback(makeRootCallback(ED4_compression_toggle_changed_cb, true));
    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_TYPE,    0)->add_callback(ED4_compression_changed_cb);
    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT, 1)->add_callback(ED4_compression_changed_cb)->set_minmax(1, 99);

    root->awar_int(ED4_AWAR_DIGITS_AS_REPEAT, 0);
    root->awar_int(ED4_AWAR_FAST_CURSOR_JUMP, 0);
    root->awar_int(ED4_AWAR_CURSOR_TYPE, (int)ED4_RIGHT_ORIENTED_CURSOR);

    ED4_create_consensus_awars(root);
    ED4_create_NDS_awars(root);

    root->awar_string(ED4_AWAR_REP_SEARCH_PATTERN, ".");
    root->awar_string(ED4_AWAR_REP_REPLACE_PATTERN, "-");

    root->awar_int(ED4_AWAR_SCROLL_SPEED_X, 40);
    root->awar_int(ED4_AWAR_SCROLL_SPEED_Y, 20);
    root->awar_int(ED4_AWAR_SCROLL_MARGIN, 5);

    root->awar_string(ED4_AWAR_SPECIES_TO_CREATE, "");

    root->awar_string(ED4_AWAR_GAP_CHARS, ".-~?")->add_callback(ED4_gap_chars_changed);
    ED4_gap_chars_changed(root);
    root->awar_int(ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES, 0);

    {
        GB_ERROR gde_err = GDE_init(ED4_ROOT->aw_root, ED4_ROOT->props_db, GLOBAL_gb_main, ED4_create_sequences_for_gde, 0, GDE_WINDOWTYPE_EDIT4);
        if (gde_err) GBK_terminatef("Fatal error: %s", gde_err);
    }

    root->awar_string(ED4_AWAR_CREATE_FROM_CONS_REPL_EQUAL, "-");
    root->awar_string(ED4_AWAR_CREATE_FROM_CONS_REPL_POINT, "?");
    root->awar_int(ED4_AWAR_CREATE_FROM_CONS_CREATE_POINTS, 1);
    root->awar_int(ED4_AWAR_CREATE_FROM_CONS_ALL_UPPER, 1);
    root->awar_int(ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE, 0);

    ED4_createVisualizeSAI_Awars(root, AW_ROOT_DEFAULT);
    ED4_create_dot_missing_bases_awars(root, AW_ROOT_DEFAULT);

    // Create Awars To Be Used In Protein Viewer
    if (ED4_ROOT->alignment_type == GB_AT_DNA) {
        PV_CreateAwars(root, AW_ROOT_DEFAULT);
    }

    // create awars to be used for protein secondary structure match
    if (ED4_ROOT->alignment_type == GB_AT_AA) {
        root->awar_int(PFOLD_AWAR_ENABLE, 1);
        root->awar_string(PFOLD_AWAR_SELECTED_SAI, "PFOLD");
        root->awar_int(PFOLD_AWAR_MATCH_METHOD, SECSTRUCT_SEQUENCE);
        int pt;
        char awar[256];
        for (int i = 0; pfold_match_type_awars[i].name; i++) {
            e4_assert(i<PFOLD_PAIRS);
            pt = pfold_match_type_awars[i].value;
            sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].name);
            root->awar_string(awar, pfold_pairs[pt])->add_target_var(&pfold_pairs[pt]);
            sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].name);
            root->awar_string(awar, pfold_pair_chars[pt])->add_target_var(&pfold_pair_chars[pt]);
        }
        root->awar_string(PFOLD_AWAR_SYMBOL_TEMPLATE_2, PFOLD_PAIR_CHARS_2);
        root->awar_string(PFOLD_AWAR_SAI_FILTER, "pfold");
    }

    GB_ERROR error = ARB_init_global_awars(root, AW_ROOT_DEFAULT, GLOBAL_gb_main);
    if (error) aw_message(error);
}

const char *ED4_propertyName(int mode) {
    // mode == 0 -> alignment specific          (e.g. "edit4_ali_16s.arb")
    // mode == 1 -> alignment-type specific     (e.g. "edit4_rna.arb")
    // mode == 2 -> unspecific (normal)         (always "edit4.arb")
    //
    // Note : result is only valid until next call

    e4_assert(mode >= 0 && mode <= 2);

    const char *result;
    if (mode == 2) {
        result = "edit4.arb";
    }
    else {
        static char *ali_name = 0;
        static char *ali_type = 0;
        static char *result_copy = 0;

        if (!ali_name) {
            GB_transaction ta(GLOBAL_gb_main);
            ali_name = GBT_get_default_alignment(GLOBAL_gb_main);
            ali_type = GBT_get_alignment_type_string(GLOBAL_gb_main, ali_name);
            result_copy   = new char[21+strlen(ali_name)];
        }

        sprintf(result_copy, "edit4_%s.arb", mode == 0 ? ali_name : ali_type);
        result = result_copy;
    }

    return result;
}

static void ED4_postcbcb(AW_window *aww) {
    ED4_ROOT->announce_useraction_in(aww);
    ED4_trigger_instant_refresh();
}
static void seq_colors_changed_cb() {
    ED4_ROOT->request_refresh_for_sequence_terminals();
}

int ARB_main(int argc, char *argv[]) {
    const char *data_path = ":";
    char *config_name = NULL;

    if (argc > 1 && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
        fprintf(stderr,
                "Usage: arb_edit4 [options] database\n"
                "       database    name of database or ':' to connect to arb-database-server\n"
                "\n"
                "       options:\n"
                "       -c config   loads configuration 'config' (default: '" DEFAULT_CONFIGURATION "')\n"
            );
        return EXIT_SUCCESS;
    }

    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        config_name = new char[strlen(argv[2])+1];
        strcpy(config_name, argv[2]);
        argc -= 2; argv += 2;
    }
    else { // load default configuration if no command line is given
        config_name = ARB_strdup(DEFAULT_CONFIGURATION);
        printf("Using '%s'\n", DEFAULT_CONFIGURATION);
    }

    if (argc>1) {
        data_path = argv[1];
        argc--; argv++;
    }

    aw_initstatus();

    GB_shell  shell;
    ARB_ERROR error;
    GLOBAL_gb_main = GB_open(data_path, "rwt");
    if (!GLOBAL_gb_main) {
        error = GB_await_error();
    }
    else {
#if defined(DEBUG)
        AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB database (%s)", data_path));
#endif // DEBUG

        ED4_ROOT = new ED4_root(&argc, &argv);

        error = configure_macro_recording(ED4_ROOT->aw_root, "ARB_EDIT4", GLOBAL_gb_main);
        if (!error) {
            ED4_ROOT->database = new EDB_root_bact;
            error              = ED4_ROOT->init_alignment();
        }
        if (!error) {
            ed4_create_all_awars(ED4_ROOT->aw_root, config_name);

            ED4_ROOT->st_ml           = STAT_create_ST_ML(GLOBAL_gb_main);
            ED4_ROOT->sequence_colors = new ED4_seq_colors(ED4_G_SEQUENCES, seq_colors_changed_cb);

            ED4_ROOT->edk = new ed_key;
            ED4_ROOT->edk->create_awars(ED4_ROOT->aw_root);

            ED4_ROOT->helix = new AW_helix(ED4_ROOT->aw_root);

            {
                GB_ERROR warning = NULL;
                switch (ED4_ROOT->alignment_type) {
                    case GB_AT_RNA:
                    case GB_AT_DNA:
                        warning = ED4_ROOT->helix->init(GLOBAL_gb_main);
                        break;

                    case GB_AT_AA:
                        warning = ED4_pfold_set_SAI(&ED4_ROOT->protstruct, GLOBAL_gb_main, ED4_ROOT->alignment_name, &ED4_ROOT->protstruct_len);
                        break;

                    default:
                        e4_assert(0);
                        break;
                }

                if (warning) aw_message(warning); // write to console
                ED4_ROOT->create_new_window(); // create first editor window
                if (warning) { aw_message(warning); warning = 0; } // write again to status window
            }

            ED4_objspec::init_object_specs();
            {
                int found_config = 0;
                ED4_LocalWinContext uses(ED4_ROOT->first_window);

                if (config_name)
                {
                    GB_transaction ta(GLOBAL_gb_main);
                    GB_ERROR       cfg_error;
                    GBT_config     cfg(GLOBAL_gb_main, config_name, cfg_error);

                    if (cfg.exists()) {
                        ED4_ROOT->create_hierarchy(cfg.get_definition(GBT_config::MIDDLE_AREA), cfg.get_definition(GBT_config::TOP_AREA)); // create internal hierarchy
                        found_config = 1;
                    }
                    else {
                        aw_message(GBS_global_string("Could not find configuration '%s'", config_name));
                    }
                }

                if (!found_config) {
                    // create internal hierarchy

                    char *as_mid = ED4_ROOT->database->make_string();
                    char *as_top = ED4_ROOT->database->make_top_bot_string();

                    ED4_ROOT->create_hierarchy(as_mid, as_top);

                    delete as_mid;
                    delete as_top;
                }
            }

            // now bind DB depending callbacks
            ed4_bind_mainDB_awar_callbacks(ED4_ROOT->aw_root);

            // Create Additional sequence (aminoacid) terminals to be used in Protein Viewer
            if (ED4_ROOT->alignment_type == GB_AT_DNA) {
                PV_CallBackFunction(ED4_ROOT->aw_root);
            }

            AWT_install_postcb_cb(ED4_postcbcb);
            AWT_install_cb_guards();
            e4_assert(!ED4_WinContext::have_context()); // no global context shall be active
            ED4_ROOT->aw_root->main_loop(); // enter main-loop
        }

        shutdown_macro_recording(ED4_ROOT->aw_root);
        GB_close(GLOBAL_gb_main);
    }

    bool have_aw_root = ED4_ROOT && ED4_ROOT->aw_root;
    if (error) {
        if (have_aw_root) aw_popup_ok(error.deliver());
        else fprintf(stderr, "arb_edit4: Error: %s\n", error.deliver());
    }
    if (have_aw_root) delete ED4_ROOT->aw_root;

    return EXIT_SUCCESS;
}

