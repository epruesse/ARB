// =============================================================== //
//                                                                 //
//   File      : PH_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "phylo.hxx"
#include "phwin.hxx"
#include "PH_display.hxx"

#include <awt_sel_boxes.hxx>
#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <awt.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>

#include <iostream>
#include <macros.hxx>
#include <aw_question.hxx>

using namespace std;

AW_HEADER_MAIN

GBDATA *GLOBAL_gb_main; // global gb_main for arb_phylo
char **filter_text;

static void create_filter_text()
{
    filter_text = (char **) calloc(FILTER_MODES, sizeof (char *));
    for (int i=0; i<FILTER_MODES; i++) filter_text[i] = new char[100];

    strcpy(filter_text[DONT_COUNT],           "don't count (ignore)                       ");
    strcpy(filter_text[SKIP_COLUMN_IF_MAX],   "if occurs most often => forget whole column");
    strcpy(filter_text[SKIP_COLUMN_IF_OCCUR], "if occurs => forget whole column           ");
    strcpy(filter_text[COUNT_DONT_USE_MAX],   "count, but do NOT use as maximum           ");
    strcpy(filter_text[TREAT_AS_UPPERCASE],   "treat as uppercase character               ");
    strcpy(filter_text[TREAT_AS_REGULAR],     "treat as regular character                 ");
}

static bool valid_alignment_selected(AW_root *aw_root, GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    const char     *aliname = aw_root->awar(AWAR_PHYLO_ALIGNMENT)->read_char_pntr();
    if (GBT_get_alignment_len(gb_main, aliname)<1) {
        GB_clear_error();
        return false;
    }
    return true;
}

static void startup_sequence_cb(AW_window *alisel_window, AW_window *main_window, PH_root *ph_root) {
    AW_root *aw_root = main_window->get_root();
    GBDATA  *gb_main = ph_root->get_gb_main();
    if (valid_alignment_selected(aw_root, gb_main)) {
        if (alisel_window) alisel_window->hide();

        char   *use = aw_root->awar(AWAR_PHYLO_ALIGNMENT)->read_string();
        PHDATA *phd = new PHDATA(aw_root, ph_root);

        GB_set_cache_size(gb_main, PH_DB_CACHE_SIZE);
        phd->load(use);
        phd->ROOT = phd;

        long len = PHDATA::ROOT->get_seq_len();
        aw_root->awar(AWAR_PHYLO_FILTER_STOPCOL)->write_int(len);
        aw_root->awar(AWAR_PHYLO_FILTER_STARTCOL)->set_minmax(0, len);
        aw_root->awar(AWAR_PHYLO_FILTER_STOPCOL)->set_minmax(0, len);

        main_window->activate();
        ph_view_species_cb();
    }
    else {
        if (alisel_window) {
            aw_message("Please select a valid alignment");
        }
        else {
            GBK_terminate("Expected to have a valid alignment");
        }
    }
}

__ATTR__NORETURN static void ph_exit(AW_window *aw_window, PH_root *ph_root) {
    AW_root *aw_root = aw_window->get_root();
    shutdown_macro_recording(aw_root);

    GBDATA *gb_main = ph_root->get_gb_main();
    if (gb_main) {
        aw_root->unlink_awars_from_DB(gb_main);
#if defined(DEBUG)
        AWT_browser_forget_db(gb_main);
#endif // DEBUG
        GB_close(gb_main);
    }

    exit(EXIT_SUCCESS);
}


void expose_cb() {
    if (PH_display::ph_display->displayed()!=NONE) {
        PH_display::ph_display->clear_window();
        PH_display::ph_display->display();
    }
}


static void resize_cb() {
    if (PH_display::ph_display) {
        PH_display::ph_display->resized();
        PH_display::ph_display->display();
    }
}

static GB_ERROR PH_create_ml_multiline_SAI(GB_CSTR sai_name, int nr, GBDATA **gb_sai_ptr, PH_root *ph_root) {
    GBDATA   *gb_sai = GBT_find_or_create_SAI(ph_root->get_gb_main(), sai_name);
    GBDATA   *gbd, *gb2;
    GB_ERROR  error  = ph_check_initialized();

    if (!error) {
        for (gbd = GB_child(gb_sai); gbd; gbd = gb2) {
            gb2 = GB_nextChild(gbd);

            const char *key = GB_read_key_pntr(gbd);
            if (!strcmp(key, "name")) continue;
            if (!strncmp(key, "ali_", 4)) continue;

            error = GB_delete(gbd);
            if (error) break;
        }
    }

    if (!error) {
        GBDATA *gb_ali = GB_search(gb_sai, PHDATA::ROOT->use, GB_FIND);
        if (gb_ali) {
            for (gbd = GB_child(gb_ali); gbd; gbd = gb2) {
                gb2 = GB_nextChild(gbd);

                const char *key = GB_read_key_pntr(gbd);
                if (!strcmp(key, "data")) continue;
                if (!strcmp(key, "_TYPE")) continue;

                error = GB_delete(gbd);
                if (error) break;
            }
        }
    }

    GBDATA *gb_data = 0, *gb_TYPE = 0;

    if (!error) {
        gb_data             = GBT_add_data(gb_sai, PHDATA::ROOT->use, "data", GB_STRING);
        if (!gb_data) error = GB_await_error();
    }

    if (!error) {
        gb_TYPE             = GBT_add_data(gb_sai, PHDATA::ROOT->use, "_TYPE", GB_STRING);
        if (!gb_TYPE) error = GB_await_error();
    }

    if (!error && !PHDATA::ROOT->markerline) {
        error = "Nothing calculated yet";
    }

    if (!error) {
        char *full_save_name = 0;
        FILE *saveResults    = 0;
        if (nr == 2) {
            char *save_name = GB_unique_filename("conservationProfile", "gnu");
            saveResults     = GB_fopen_tempfile(save_name, "w+", &full_save_name);
            GB_remove_on_exit(full_save_name);
            free(save_name);

            if (!saveResults) error = GB_await_error();
        }

        if (!error) {
            AW_window *main_win   = PH_used_windows::windowList->phylo_main_window;
            long       minhom     = main_win->get_root()->awar(AWAR_PHYLO_FILTER_MINHOM)->read_int();
            long       maxhom     = main_win->get_root()->awar(AWAR_PHYLO_FILTER_MAXHOM)->read_int();
            long       startcol   = main_win->get_root()->awar(AWAR_PHYLO_FILTER_STARTCOL)->read_int();
            long       stopcol    = main_win->get_root()->awar(AWAR_PHYLO_FILTER_STOPCOL)->read_int();
            float     *markerline = PHDATA::ROOT->markerline;
            long       len        = PHDATA::ROOT->get_seq_len();

            char *data = (char *)calloc(sizeof(char), (int)len+1);
            int   cnt  = 0;


            for (int x=0; x<len; x++) {
                char c;

                if (x<startcol || x>stopcol) {
                    c = '.';
                }
                else {
                    float ml = markerline[x];
                    if (nr==2 && ml>0.0) {
                        ph_assert(saveResults);
                        fprintf(saveResults, "%i\t%.2f\n", cnt, ml);
                        cnt++;
                    }

                    if (ml>=0.0 && ml>=minhom && ml<=maxhom) {
                        int digit = -1;
                        switch (nr) {
                            case 0: // hundred
                                if (ml>=100.0) digit = 1;
                                break;
                            case 1: // ten
                                if (ml>=10.0) digit = int(ml/10);
                                break;
                            case 2: // one
                                digit = int(ml);
                                break;
                            default:
                                ph_assert(0);
                                break;
                        }

                        if (digit<0) c = '-';
                        else         c = '0' + digit%10;
                    }
                    else {
                        c = '-';
                    }
                }

                data[x] = c;
            }
            data[len] = 0;

            if (saveResults) {
                fclose(saveResults);
                fprintf(stderr, "Note: Frequencies as well saved to '%s'\n", full_save_name);
            }

            error = GB_write_string(gb_data, data);
            if (!error) {
                const char *buffer = GBS_global_string("FMX: Filter by Maximum Frequency: "
                                                       "Start %li; Stop %li; Minhom %li%%; Maxhom %li%%",
                                                       startcol, stopcol, minhom, maxhom);
                error = GB_write_string(gb_TYPE, buffer);
            }
            free(data);
        }
        free(full_save_name);
    }

    if (!error) *gb_sai_ptr = gb_sai;
    return error;
}

static void PH_save_ml_multiline_cb(AW_window *aww, PH_root *ph_root) {
    GB_transaction ta(ph_root->get_gb_main());

    GB_ERROR  error     = 0;
    char     *fname     = aww->get_root()->awar(AWAR_PHYLO_MARKERLINENAME)->read_string();
    int       fname_len = strlen(fname);
    {
        char *digit_appended = (char*)malloc(fname_len+2);
        memcpy(digit_appended, fname, fname_len);
        strcpy(digit_appended+fname_len, "0");

        freeset(fname, digit_appended);
    }
    GBDATA *gb_sai[3];
    int i;
    for (i=0; !error && i<3; i++) {
        fname[fname_len] = '0'+i;
        error = PH_create_ml_multiline_SAI(fname, i, &gb_sai[i], ph_root);
    }

    delete fname;

    aw_message_if(ta.close(error));
}

static void PH_save_ml_cb(AW_window *aww, PH_root *ph_root) {
    GBDATA  *gb_main = ph_root->get_gb_main();
    GB_transaction ta(gb_main);

    char   *fname  = aww->get_root()->awar(AWAR_PHYLO_MARKERLINENAME)->read_string();
    GBDATA *gb_sai = GBT_find_or_create_SAI(gb_main, fname);

    GB_ERROR error = ph_check_initialized();

    if (!error) {
        for (GBDATA *gbd = GB_child(gb_sai), *gbnext; gbd; gbd = gbnext) {
            gbnext = GB_nextChild(gbd);

            const char *key = GB_read_key_pntr(gbd);
            if (!strcmp(key, "name")) continue;
            if (!strncmp(key, "ali_", 4)) continue;

            error = GB_delete(gbd);
            if (error) break;
        }
    }

    if (!error) {
        GBDATA *gb_ali = GB_search(gb_sai, PHDATA::ROOT->use, GB_FIND);
        if (gb_ali) {
            for (GBDATA *gbd = GB_child(gb_ali), *gbnext; gbd; gbd = gbnext) {
                gbnext = GB_nextChild(gbd);

                const char *key = GB_read_key_pntr(gbd);
                if (!strcmp(key, "bits")) continue;
                if (!strcmp(key, "_TYPE")) continue;

                error = GB_delete(gbd);
                if (error) break;
            }
        }
    }

    GBDATA *gb_bits = 0, *gb_TYPE = 0;

    if (!error) {
        gb_bits             = GBT_add_data(gb_sai, PHDATA::ROOT->use, "bits", GB_BITS);
        if (!gb_bits) error = GB_await_error();
    }

    if (!error) {
        gb_TYPE             = GBT_add_data(gb_sai, PHDATA::ROOT->use, "_TYPE", GB_STRING);
        if (!gb_TYPE) error = GB_await_error();
    }

    if (!error && !PHDATA::ROOT->markerline) {
        error = "Nothing calculated yet";
    }

    if (!error) {
        AW_window *main_win   = PH_used_windows::windowList->phylo_main_window;
        long       minhom     = main_win->get_root()->awar(AWAR_PHYLO_FILTER_MINHOM)->read_int();
        long       maxhom     = main_win->get_root()->awar(AWAR_PHYLO_FILTER_MAXHOM)->read_int();
        long       startcol   = main_win->get_root()->awar(AWAR_PHYLO_FILTER_STARTCOL)->read_int();
        long       stopcol    = main_win->get_root()->awar(AWAR_PHYLO_FILTER_STOPCOL)->read_int();
        long       len        = PHDATA::ROOT->get_seq_len();
        char      *bits       = (char *)calloc(sizeof(char), (int)len+1);
        int        x;
        float     *markerline = PHDATA::ROOT->markerline;

        for (x=0; x<len; x++) {
            int bit;

            if (x < startcol || x>stopcol) {
                bit = 0;
            }
            else {
                float ml = markerline[x];

                if (ml>=0.0 && ml>=minhom && ml<=maxhom) bit = 1;
                else                     bit = 0;

            }
            bits[x] = '0'+bit;
        }

        GB_write_bits(gb_bits, bits, len, "0");
        char buffer[1024];
        sprintf(buffer, "FMX: Filter by Maximum Frequency: "
                "Start %li; Stop %li; Minhom %li%%; Maxhom %li%%",
                startcol, stopcol, minhom, maxhom);

        GB_write_string(gb_TYPE, buffer);

        free(bits);
    }
    free(fname);
    aw_message_if(ta.close(error));
}


static AW_window *PH_save_markerline(AW_root *root, PH_root *ph_root, int multi_line) {
    // multi_line ==1 -> save three SAI's usable as column percentage

    root->awar_string(AWAR_PHYLO_MARKERLINENAME, "markerline", AW_ROOT_DEFAULT);

    AW_window_simple *aws = new AW_window_simple;

    if (multi_line) {
        aws->init(root, "EXPORT_FREQUENCY_LINES", "Export Frequency Lines");
    }
    else {
        aws->init(root, "EXPORT_MARKER_LINE", "Export Marker Line");
    }

    aws->load_xfig("phylo/save_markerline.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("ph_export_markerline.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("name");
    aws->create_input_field(AWAR_PHYLO_MARKERLINENAME);

    aws->at("box");
    awt_create_SAI_selection_list(ph_root->get_gb_main(), aws, AWAR_PHYLO_MARKERLINENAME, false);

    aws->at("save");
    if (multi_line) aws->callback(makeWindowCallback(PH_save_ml_multiline_cb, ph_root));
    else            aws->callback(makeWindowCallback(PH_save_ml_cb, ph_root));
    aws->create_button("EXPORT", "EXPORT", "E");

    return aws;
}

static AW_window *create_phyl_main_window(AW_root *aw_root, PH_root *ph_root) {
    AW_window_menu_modes *awm = new AW_window_menu_modes;
    awm->init(aw_root, "ARB_PHYLO", "ARB_PHYLO", 830, 630);

    // create menus and menu inserts with callbacks

    AW_gc_manager gcmiddle = AW_manage_GC(awm,
                                          awm->get_window_id(),
                                          awm->get_device(AW_MIDDLE_AREA),
                                          PH_GC_0, PH_GC_0_DRAG, AW_GCM_DATA_AREA,
                                          makeWindowCallback(resize_cb),
                                          false, // no color groups
                                          "#CC9AF8",
                                          "#SEQUENCE$#000000",
                                          "#MARKER$#FF0000",
                                          "NOT_MARKER$#A270C0",
                                          NULL);
    AW_manage_GC(awm,
                 awm->get_window_id(),
                 awm->get_device(AW_BOTTOM_AREA),
                 PH_GC_0, PH_GC_0_DRAG, AW_GCM_WINDOW_AREA,
                 makeWindowCallback(resize_cb),
                 false, // no color groups
                 "pink",
                 "#FOOTER",
                 NULL);


#if defined(DEBUG)
    AWT_create_debug_menu(awm);
#endif // DEBUG

    // File menu
    awm->create_menu("File", "F");
    awm->insert_menu_topic("export_filter", "Export Filter",      "E", "ph_export_markerline.hlp", AWM_ALL, makeCreateWindowCallback(PH_save_markerline, ph_root, 0));
    awm->insert_menu_topic("export_freq",   "Export Frequencies", "F", "ph_export_markerline.hlp", AWM_ALL, makeCreateWindowCallback(PH_save_markerline, ph_root, 1));
    insert_macro_menu_entry(awm, false);

    awm->insert_menu_topic("quit", "Quit", "Q", "quit.hlp", AWM_ALL, makeWindowCallback(ph_exit, ph_root));

    // Calculate menu
    awm->create_menu("Calculate", "C");
    awm->insert_menu_topic("calc_column_filter", "Column Filter", "F", "no help", AWM_ALL, makeWindowCallback(ph_view_filter_cb));

    // Config menu
    awm->create_menu("Config", "o");
    awm->insert_menu_topic("config_column_filter", "Column Filter", "F", "no help", AWM_ALL, PH_create_filter_window);

    // Properties menu
    awm->create_menu("Properties", "P");
#if defined(ARB_MOTIF)
    awm->insert_menu_topic("props_menu", "Frame settings ...",   "F", "props_frame.hlp", AWM_ALL, AW_preset_window);
#endif
    awm->insert_menu_topic("props_data", "Colors and Fonts ...", "C", "color_props.hlp", AWM_ALL, makeCreateWindowCallback(AW_create_gc_window, gcmiddle));
    awm->sep______________();
    AW_insert_common_property_menu_entries(awm);
    awm->sep______________();
    awm->insert_menu_topic("save_props", "Save Properties (phylo.arb)", "S", "savedef.hlp", AWM_ALL, AW_save_properties);


    // set window areas
    awm->set_info_area_height(30);
    awm->at(5, 2);
    awm->auto_space(5, -2);

    awm->callback(makeWindowCallback(ph_exit, ph_root));
    awm->button_length(0);
    awm->help_text("quit.hlp");
    awm->create_button("QUIT", "QUIT");
#if defined(ARB_GTK)
    awm->set_close_action("QUIT");
#endif

    awm->callback(makeHelpCallback("phylo.hlp"));
    awm->button_length(0);
    awm->create_button("HELP", "HELP", "H");


    awm->set_bottom_area_height(120);

    awm->set_expose_callback(AW_MIDDLE_AREA, makeWindowCallback(expose_cb));
    awm->set_resize_callback(AW_MIDDLE_AREA, makeWindowCallback(resize_cb));
    awm->set_expose_callback(AW_BOTTOM_AREA, makeWindowCallback(display_status_cb));
    awm->set_resize_callback(AW_BOTTOM_AREA, makeWindowCallback(display_status_cb));

    return awm;
}


static AW_window *create_select_alignment_window(AW_root *aw_root, AW_window *main_window, PH_root *ph_root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "SELECT_ALIGNMENT", "ARB_PHYLO: Select alignment");
    aws->load_xfig("phylo/select_ali.fig");
    aws->button_length(10);

    aws->at("which_alignment");
    awt_create_ALI_selection_list(ph_root->get_gb_main(), (AW_window *)aws, AWAR_PHYLO_ALIGNMENT, "*=");

    aws->auto_space(10, 10);

    aws->at("ok");
    aws->callback(makeWindowCallback(startup_sequence_cb, main_window, ph_root));
    aws->create_button("OK", "Ok", "D");

    aws->callback(makeWindowCallback(ph_exit, ph_root));
    aws->create_button("ABORT", "Abort", "D");

#if defined(ARB_GTK)
    aws->set_close_action("ABORT");
#endif

    return aws;
}



PH_used_windows::PH_used_windows()
{
    memset((char *) this, 0, sizeof(PH_used_windows));
}

// initialize 'globals'
PH_used_windows *PH_used_windows::windowList = 0;
PH_display *PH_display::ph_display=0;
PHDATA *PHDATA::ROOT = 0;

static void create_variables(AW_root *aw_root, AW_default def) {
    aw_root->awar_string(AWAR_PHYLO_ALIGNMENT,     "", def);
    aw_root->awar_string(AWAR_PHYLO_FILTER_FILTER, "", def);
    PH_create_filter_variables(aw_root, def);
}

int ARB_main(int argc, char *argv[]) {
    aw_initstatus();

    GB_shell shell;
    AW_root  *aw_root = AWT_create_root("phylo.arb", "ARB_PHYLO", need_macro_ability(), &argc, &argv);

    int exitcode = EXIT_SUCCESS;
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr, "Usage: arb_phylo [database]\n");
        exitcode = EXIT_FAILURE;
    }
    else {
        const char *db_server = (argc == 2 ? argv[1] : ":");

        PH_used_windows *puw = new PH_used_windows;
        PH_display      *phd = new PH_display;

        PH_root  *ph_root = new PH_root;
        GB_ERROR  error   = ph_root->open(db_server);

        if (!error) error = configure_macro_recording(aw_root, "ARB_PHYLO", ph_root->get_gb_main());
        if (!error) {
            GBDATA *gb_main = ph_root->get_gb_main();

            // create arb_phylo awars :
            create_variables(aw_root, AW_ROOT_DEFAULT);
            ARB_init_global_awars(aw_root, AW_ROOT_DEFAULT, gb_main);
#if defined(DEBUG)
            AWT_create_db_browser_awars(aw_root, AW_ROOT_DEFAULT);
#endif // DEBUG

#if defined(DEBUG)
            AWT_announce_db_to_browser(gb_main, GBS_global_string("ARB-database (%s)", db_server));
#endif // DEBUG

            create_filter_text();

            // create main window :

            puw->phylo_main_window = create_phyl_main_window(aw_root, ph_root);
            puw->windowList        = puw;

            phd->ph_display = phd;

            // loading database
            GB_push_transaction(gb_main);

            ConstStrArray alignment_names;
            GBT_get_alignment_names(alignment_names, gb_main);

            int num_alignments;
            for (num_alignments = 0; alignment_names[num_alignments] != 0; num_alignments++) {}

            if (num_alignments > 1) {
                char *defaultAli = GBT_get_default_alignment(gb_main);
                aw_root->awar(AWAR_PHYLO_ALIGNMENT)->write_string(defaultAli);
                create_select_alignment_window(aw_root, puw->phylo_main_window, ph_root)->show();
                free(defaultAli);
            }
            else {
                aw_root->awar(AWAR_PHYLO_ALIGNMENT)->write_string(alignment_names[0]);
                startup_sequence_cb(NULL, puw->phylo_main_window, ph_root);
            }
            GB_pop_transaction(gb_main);

            AWT_install_cb_guards();
            aw_root->main_loop();
        }

        if (error) {
            aw_popup_exit(error);
            exitcode = EXIT_FAILURE;
        }
    }

    delete aw_root;
    return exitcode;
}

