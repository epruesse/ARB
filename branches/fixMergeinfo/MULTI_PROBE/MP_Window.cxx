// ================================================================ //
//                                                                  //
//   File      : MP_Window.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "MP_externs.hxx"
#include "MultiProbe.hxx"
#include "mp_proto.hxx"

#include <awt_sel_boxes.hxx>
#include <awt_modules.hxx>
#include <aw_select.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_awar_defs.hxx>

#include <arb_strarray.h>
#include <arb_defs.h>
#include <arb_strbuf.h>
#include <arbdbt.h>
#include <RegExpr.hxx>

// **************************************************************************

struct mp_gl_struct mp_pd_gl; // global link

AW_selection_list *selected_list;
AW_selection_list *probelist;
AW_selection_list *result_probes_list;


AW_window_simple *MP_Window::create_result_window(AW_root *aw_root) {
    if (!result_window) {
        result_window = new AW_window_simple;
        result_window->init(aw_root, "MULTIPROBE_RESULTS", "MultiProbe combination results");
        result_window->load_xfig("mp_results.fig");

        result_window->auto_space(5, 5);
        
        result_window->button_length(7);
        result_window->at("close");
        result_window->callback(AW_POPDOWN);
        result_window->create_button("CLOSE", "CLOSE");

        result_window->at("Help");
        result_window->callback(AW_POPUP_HELP, (AW_CL)"multiproberesults.hlp");
        result_window->create_button("HELP", "HELP");

        result_window->at("Comment");
        result_window->callback(MP_Comment, (AW_CL)0);
        result_window->create_input_field(MP_AWAR_RESULTPROBESCOMMENT);

        result_window->at("box");
        result_window->callback(MP_result_chosen);
        result_probes_list = result_window->create_selection_list(MP_AWAR_RESULTPROBES);
        result_probes_list->set_file_suffix("mpr");
        result_probes_list->insert_default("", "");

        const StorableSelectionList *storable_probes_list = new StorableSelectionList(TypedSelectionList("mpr", result_probes_list, "multiprobes", "multi_probes"));

        result_window->at("buttons");
        result_window->callback(AW_POPUP, (AW_CL)create_load_box_for_selection_lists, (AW_CL)storable_probes_list);
        result_window->create_button("LOAD_RPL", "LOAD");

        result_window->callback(AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)storable_probes_list);
        result_window->create_button("SAVE_RPL", "SAVE");

        result_window->callback(awt_clear_selection_list_cb, (AW_CL)result_probes_list);
        result_window->create_button("CLEAR", "CLEAR");

        result_window->callback(MP_delete_selected, AW_CL(result_probes_list));
        result_window->create_button("DELETE", "DELETE");

        // change comment :

        result_window->button_length(8);

        result_window->at("comment");
        result_window->callback(MP_Comment, (AW_CL) "Bad");
        result_window->create_button("MARK_AS_BAD", "BAD");

        result_window->callback(MP_Comment, (AW_CL) "???");
        result_window->create_button("MARK_AS_GOOD", "???");

        result_window->callback(MP_Comment, (AW_CL) "Good");
        result_window->create_button("MARK_AS_BEST", "Good");

        result_window->at("auto");
        result_window->create_toggle(MP_AWAR_AUTOADVANCE);

        // tree actions :

        result_window->button_length(3);

        result_window->at("ct_back");
        result_window->callback(MP_show_probes_in_tree_move, (AW_CL)1, (AW_CL)result_probes_list);
        result_window->create_button("COLOR_TREE_BACKWARD", "#rightleft_small.bitmap");

        result_window->at("ct_fwd");
        result_window->callback(MP_show_probes_in_tree_move, (AW_CL)0, (AW_CL)result_probes_list);
        result_window->create_button("COLOR_TREE_FORWARD", "#leftright_small.bitmap");

        result_window->button_length(8);

        result_window->at("ColorTree");
        result_window->button_length(4);
        result_window->callback(MP_show_probes_in_tree);
        result_window->create_button("COLOR_TREE", "GO");

        result_window->at("MarkTree");
        result_window->callback(MP_mark_probes_in_tree);
        result_window->create_button("MARK_TREE", "GO");

        result_window->at("GroupAll");
        result_window->callback(MP_group_all_except_marked);
        result_window->create_button("GROUP_UNMARKED", "GO");

        result_window->at("StandardColor");
        result_window->callback(MP_normal_colors_in_tree);
        result_window->create_button("RESET_COLORS", "GO");
    }
    return result_window;
}

// --------------------------------------------------------------------------------
// Format of probe-list for multi-probes:
//
// The saved format is identical to the internal format (of sellist entries; where value always equals displayed!)
//     "quality#singlemismatch#ecolipos#target"
//
// When loading input probes, several other formats are accepted:
// 
//     "quality,singlemismatch#ecolipos#probe"                      (old save format)
//     "target le pos apos ecol grps GC 4gc2at probe | ..."         (save of probe design; old format)
//     "target le pos apos ecol grps GC 4gc2at probe | ...,target"  (save of probe design)
//
// above
//   'target' is the target-string of the 'probe'. Internally MP works with target strings,
//   so when loading the old save-format, 'probe' gets reverse-complemented into 'target'


#define SPACED(expr) "[[:space:]]*" expr "[[:space:]]*"

inline char *gen_display(int quality, int singleMis, int ecoliPos, const char *probe) {
    return GBS_global_string_copy("%i#%i#%5i#%s", quality, singleMis, ecoliPos, probe);
}

static GB_ERROR mp_list2file(const CharPtrArray& display, const CharPtrArray& value, StrArray& line) {
    GB_ERROR error = NULL;

    if (value.empty()) error = "nothing to save";

    for (size_t i = 0; i<display.size() && !error; ++i) {
        line.put(strdup(display[i]));
    }

    return error;
}

static char T_or_U_for_load = 0;

static GB_ERROR mp_file2list(const CharPtrArray& line, StrArray& display, StrArray& value) {
    GB_ERROR error = NULL;

    if (line.empty()) error = "empty file";

    // detect format
    if (!error) {
        // 1. try to read probes saved from multiprobes inputlist
        RegExpr reg_saved("^" SPACED("([0-9]+)") "([,#])" SPACED("([0-9])+") "#" SPACED("([0-9]+)") "#" SPACED("([A-Z]+)") "$", true);
        bool    isSavedFormat = true;

        for (size_t i = 0; i<line.size() && isSavedFormat; ++i) {
            const RegMatch *match = reg_saved.match(line[i]);
            if (!match || reg_saved.subexpr_count() != 5) {
                isSavedFormat = false;
            }
            else {
                std::string sep = reg_saved.subexpr_match(2)->extract(line[i]);

                int quality   = atoi(reg_saved.subexpr_match(1)->extract(line[i]).c_str());
                int singlemis = atoi(reg_saved.subexpr_match(3)->extract(line[i]).c_str());
                int ecoli     = atoi(reg_saved.subexpr_match(4)->extract(line[i]).c_str());

                std::string probe = reg_saved.subexpr_match(5)->extract(line[i]);

                if (sep[0] == ',') { // old format (saved probe instead of probe-target)
                    size_t  plen   = probe.length();
                    char   *dprobe = GB_strndup(probe.c_str(), plen);

                    mp_assert(T_or_U_for_load);

                    GBT_reverseComplementNucSequence(dprobe, plen, T_or_U_for_load);
                    probe = dprobe;
                    free(dprobe);
                }

                char *entry = gen_display(quality, singlemis, ecoli, probe.c_str());
                display.put(entry); // transfers ownership - dont free!
                value.put(strdup(entry));
            }
        }

        if (!isSavedFormat) {
            // delete attempt to read saved format:
            display.clear();
            value.clear();

            // try to read designed list 
            RegExpr reg_designed("^([A-Z]+)[[:space:]]+[0-9]+[[:space:]]+[A-Z][=+-][[:space:]]+[0-9]+[[:space:]]+([0-9]+)[[:space:]]+[0-9]+[[:space:]]+[0-9.]+[[:space:]]+[0-9.]+[[:space:]]+[A-Z]+[[:space:]]+[|]", true);

            for (size_t i = 0; i<line.size() && !error; ++i) {
                char *probe       = NULL;
                char *description = NULL;
                bool  new_format  = false;

                const char *comma = strchr(line[i], ',');
                if (comma) {
                    description = GB_strpartdup(line[i], comma-1);

                    const char *cprobe = comma+1;
                    while (cprobe[0] == ' ') ++cprobe;
                    probe = strdup(cprobe);
                    
                    new_format = true;
                }
                else {
                    description = strdup(line[i]);
                }

                const RegMatch *match = reg_designed.match(description);
                if (match) { // line from probe design (old + new format)
                    mp_assert(match->didMatch());

                    match = reg_designed.subexpr_match(1);
                    mp_assert(match->didMatch());
                    std::string parsed_probe = match->extract(description);

                    if (new_format) { // already got probe value -> compare
                        if (strcmp(probe, parsed_probe.c_str()) != 0) {
                            error = GBS_global_string("probe string mismatch (in '%s')", line[i]);
                        }
                    }
                    else {
                        probe = strdup(parsed_probe.c_str());
                    }

                    if (!error) {
                        int quality, ecoli;
                        
                        match   = reg_designed.subexpr_match(2);
                        mp_assert(match->didMatch());
                        ecoli   = atoi(match->extract(description).c_str());
                        quality = 3;

                        char *entry = gen_display(quality, 0, ecoli, probe);
                        display.put(entry); // transfers ownership - dont free!
                        value.put(strdup(entry));
                    }
                }
                else if (new_format && probe[0]) {
                    error = GBS_global_string("can't parse line '%s'", line[i]);
                }
                // (when loading old format -> silently ignore non-matching lines)

                free(probe);
                free(description);
            }
        }
    }

    return error;
}

void MP_Window::build_pt_server_list() {
    int     i;
    char    *choice;

#if defined(WARN_TODO)
#warning why option_menu ? better use selection list ( awt_create_selection_list_on_pt_servers )
#endif

    aws->at("PTServer");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu(MP_AWAR_PTSERVER);

    for (i=0; ; i++) {
        choice = GBS_ptserver_id_to_choice(i, 1);
        if (! choice) break;

        aws->insert_option(choice, "", i);
        delete choice;
    }

    aws->update_option_menu();
}

static void track_ali_change_cb(GBDATA *gb_ali, int*, GB_CB_TYPE) {
    GB_transaction     ta(gb_ali);
    GBDATA            *gb_main = GB_get_root(gb_ali);
    char              *aliname = GBT_get_default_alignment(gb_main);
    GB_alignment_type  alitype = GBT_get_alignment_type(gb_main, aliname);
    GB_ERROR           error   = GBT_determine_T_or_U(alitype, &T_or_U_for_load, "reverse-complement");

    if (error) aw_message(error);
    free(aliname);
}

static void install_track_ali_type_callback(GBDATA *gb_main) {
    GB_transaction ta(gb_main);
    GB_ERROR       error = NULL;

    GBDATA *gb_ali = GB_search(gb_main, "presets/use", GB_FIND);
    if (!gb_ali) {
        error = GB_await_error();
    }
    else {
        error = GB_add_callback(gb_ali, GB_CB_CHANGED, track_ali_change_cb, 0);
        track_ali_change_cb(gb_ali, 0, GB_CB_CHANGED);
    }

    if (error) {
        aw_message(GBS_global_string("Cannot install ali-callback (Reason: %s)", error));
    }
}

static void MP_collect_probes(AW_window*, awt_collect_mode mode, AW_CL) {
    switch (mode) {
        case ACM_ADD:
            if (!probelist->default_is_selected()) {
                int                        idx = probelist->get_index_of_selected();
                AW_selection_list_iterator sel(probelist, idx);
                selected_list->insert(sel.get_displayed(), sel.get_value());
                MP_delete_selected(NULL, AW_CL(probelist));
            }
            break;

        case ACM_REMOVE:
            if (!selected_list->default_is_selected()) {
                int                        idx = selected_list->get_index_of_selected();
                AW_selection_list_iterator sel(selected_list, idx);
                probelist->insert(sel.get_displayed(), sel.get_value());
                MP_delete_selected(NULL, AW_CL(selected_list));
            }
            break;
            
        case ACM_FILL:
            probelist->move_content_to(selected_list);
            break;

        case ACM_EMPTY:
            selected_list->move_content_to(probelist);
            break;
    }

    selected_list->sort(false, true);

    probelist->update();
    selected_list->update();
}

MP_Window::MP_Window(AW_root *aw_root, GBDATA *gb_main) {
    int max_seq_col = 35;
    int max_seq_hgt = 15;

#if defined(DEBUG)
    static bool initialized = false;
    mp_assert(!initialized); // this function may only be called once!
    initialized             = true;
#endif

    install_track_ali_type_callback(gb_main);
    
    result_window = NULL;

    aws = new AW_window_simple;
    aws->init(aw_root, "MULTIPROBE", "MULTI_PROBE");
    aws->load_xfig("multiprobe.fig");

    aws->at("close");
    aws->callback(MP_close_main);
    aws->create_button("CLOSE", "CLOSE");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"multiprobe.hlp");
    aws->create_button("HELP", "HELP");

    aws->button_length(7);
    aws->at("Selectedprobes");
    aws->callback(MP_selected_chosen);
    selected_list = aws->create_selection_list(MP_AWAR_SELECTEDPROBES, max_seq_col, max_seq_hgt);
    const StorableSelectionList *storable_selected_list = new StorableSelectionList(TypedSelectionList("prb", selected_list, "probes", "selected_probes"), mp_list2file, mp_file2list);

    selected_list->insert_default("", "");

    aws->at("Probelist");
    probelist = aws->create_selection_list(MP_AWAR_PROBELIST);
    const StorableSelectionList *storable_probelist = new StorableSelectionList(TypedSelectionList("prb", probelist, "probes", "all_probes"), mp_list2file, mp_file2list);
    probelist->insert_default("", "");

    aws->at("collect");
    awt_create_collect_buttons(aws, true, MP_collect_probes, 0);
    
    aws->auto_space(5, 5);
    aws->button_length(7);

    for (int rightSide = 0; rightSide <= 1; ++rightSide) {
        const StorableSelectionList *storableList = rightSide ? storable_selected_list : storable_probelist;
        const char                  *id_suffix    = rightSide ? "SELECTED_PROBES" : "PROBES";

        AW_selection_list *sellist = storableList->get_typedsellist().get_sellist();

        aws->at(rightSide ? "RightButtons" : "LeftButtons");

        aws->callback(AW_POPUP, (AW_CL)create_load_box_for_selection_lists, (AW_CL)storableList);
        aws->create_button(GBS_global_string("LOAD_%s", id_suffix), "LOAD");

        aws->callback(AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)storableList);
        aws->create_button(GBS_global_string("SAVE_%s", id_suffix), "SAVE");

        aws->callback(awt_clear_selection_list_cb, (AW_CL)sellist);
        aws->create_button(GBS_global_string("CLEAR_%s", id_suffix), "CLEAR");

        aws->callback(MP_delete_selected, (AW_CL)sellist);
        aws->create_button(GBS_global_string("DELETE_%s", id_suffix), "DELETE");
    }

    aws->at("Quality");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu(MP_AWAR_QUALITY);
    aws->insert_option("High Priority", "", 5);
    aws->insert_option("       4", "", 4);
    aws->insert_option("Normal 3", "", 3);
    aws->insert_option("       2", "", 2);
    aws->insert_option("Low Prio. 1", "", 1);
    aws->update_option_menu();

    aws->at("add");
    aws->callback(MP_new_sequence);
    aws->create_autosize_button("ADD_PROBE", "ADD");
    aws->at("seqin");
    aws->create_input_field(MP_AWAR_SEQIN, 25);

    // --------------------------------
    //      multi probe parameters

    aws->at("PTServer");
    awt_create_selection_list_on_pt_servers(aws, MP_AWAR_PTSERVER, true);
    aw_root->awar(MP_AWAR_PTSERVER)->add_callback(MP_cache_sonden2); // remove cached probes when changing pt-server

    aws->at("NoOfProbes");
    aws->create_option_menu(MP_AWAR_NOOFPROBES);
    aws->callback(MP_cache_sonden);
    aws->insert_option("Compute  1 probe ", "", 1);
    char str[50];
    for (int i=2; i<=MAXPROBECOMBIS; i++) {
        sprintf(str, "%2d-probe-combinations", i);
        aws->insert_option(str, "", i);
    }
    aws->update_option_menu();

    aws->button_length(10);
    aws->at("Compute");
    aws->callback(MP_compute, (AW_CL)gb_main);
    aws->highlight();
    aws->help_text("Compute possible Solutions");
    aws->create_button("GO", "GO");

    aws->button_length(20);
    aws->at("Results");
    aws->callback(MP_popup_result_window);
    aws->create_button("OPEN_RESULT_WIN", "Open result window");

    aws->at("Komplement");
    aws->callback(MP_cache_sonden);
    aws->create_toggle(MP_AWAR_COMPLEMENT);

    aws->at("WeightedMismatches");
    aws->callback(MP_cache_sonden);
    aws->create_toggle(MP_AWAR_WEIGHTEDMISMATCHES);

    // max non group hits
    aws->at("Border1");
    aws->callback(MP_cache_sonden);
    aws->create_input_field(MP_AWAR_QUALITYBORDER1, 6);

    aws->at("OutsideMismatches");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu(MP_AWAR_OUTSIDEMISMATCHES);
    aws->insert_option("3.0", "", (float)3.0);
    aws->insert_option("2.5", "", (float)2.5);
    aws->insert_option("2.0", "", (float)2.0);
    aws->insert_option("1.5", "", (float)1.5);
    aws->insert_option("1.0", "", (float)1.0);
    aws->update_option_menu();

    // max mismatches for group
    aws->at("Greyzone");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu(MP_AWAR_GREYZONE);
    aws->insert_default_option("0.0", "", (float)0.0);
    for (float lauf=0.1; lauf<(float)1.0; lauf+=0.1) {
        char strs[20];
        sprintf(strs, "%.1f", lauf);
        aws->insert_option(strs, "", lauf);
    }
    aws->update_option_menu();

}


MP_Window::~MP_Window()
{
    if (result_window)  result_window->hide();
    if (aws)            aws->hide();

    delete result_window;
    delete aws;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

inline void array2cpa(const char **content, int count, ConstStrArray& array) {
    array.erase();
    for (int i = 0; i<count; ++i) {
        array.put(content[i]);
    }
}

inline char *array2string(const CharPtrArray& array) {
    GBS_strstruct out(1000);

    for (size_t i = 0; i<array.size(); ++i) {
        out.cat(array[i]);
        out.put('\n');
    }

    return out.release();
}

#define TEST_ASSERT_ARRAY_EQUALS(array,expected) do {   \
        char *arrayAsString = array2string(array);      \
        TEST_ASSERT_EQUAL(arrayAsString, expected);     \
        free(arrayAsString);                            \
    } while(0)

void TEST_probe_lists() {
    ConstStrArray lines;

    const char *expected =
        "3#0#  521#GCAGCCGCGGUAAUACGG\n"
        "3#0#  510#ACUCCGUGCCAGCAGCCG\n"
        "3#0#  511#CUCCGUGCCAGCAGCCGC\n"
        "3#0#  512#UCCGUGCCAGCAGCCGCG\n"
        "3#0#  513#CCGUGCCAGCAGCCGCGG\n"
        "3#0#  509#AACUCCGUGCCAGCAGCC\n";

    const char *old_probeDesignSave[] = {
        "Probe design Parameters:",
        "Length of probe      18",
        "Temperature        [30.0 -100.0]",
        "GC-Content         [50.0 -100.0]",
        "E.Coli Position    [any]",
        "Max Non Group Hits     0",
        "Min Group Hits        50%",
        "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species",
        "GCAGCCGCGGUAAUACGG 18 A=  4398  521   23 66.7 60.0    CCGUAUUACCGCGGCUGC |  0;  0;  0;  0;  0;  0;  0;  0; 35; 35; 35; 38; 74; 74; 74; 77;113;113;113;148;",
        "ACUCCGUGCCAGCAGCCG 18 B=  3852  510   23 72.2 62.0    CGGCUGCUGGCACGGAGU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80; 80;120;120;120;200;200;200;200;201;",
        "CUCCGUGCCAGCAGCCGC 18 B+     4  511   23 77.8 64.0    GCGGCUGCUGGCACGGAG |  0;  0;  0;  0;  0; 40; 40; 40; 40; 80; 80; 80;160;160;160;160;201;201;201;201;",
        "UCCGUGCCAGCAGCCGCG 18 B+     7  512   23 77.8 64.0    CGCGGCUGCUGGCACGGA |  0;  0;  0;  0;  0; 40; 40; 40;120;120;120;120;160;160;161;201;201;201;202;202;",
        "CCGUGCCAGCAGCCGCGG 18 B+     9  513   23 83.3 66.0    CCGCGGCUGCUGGCACGG |  0;  0;  0;  0;  0; 80; 80; 80; 80;120;120;121;161;161;161;162;203;203;204;204;",
        "AACUCCGUGCCAGCAGCC 18 B-     1  509   22 66.7 60.0    GGCUGCUGGCACGGAGUU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80;120;120;120;120;160;160;160;240;240;",
    };
    array2cpa(old_probeDesignSave, ARRAY_ELEMS(old_probeDesignSave), lines);
    {
        StrArray display, value;
        TEST_ASSERT_NO_ERROR(mp_file2list(lines, display, value));
        TEST_ASSERT_ARRAY_EQUALS(display, expected);
        TEST_ASSERT_ARRAY_EQUALS(value, expected);
    }

    const char *old_multiprobeInputSave[] = { // old multi-probe saved probe (i.e. not target) sequences -> load shall correct that
        "3,0#   521#CCGUAUUACCGCGGCUGC",
        "3,0#   510#CGGCUGCUGGCACGGAGU",
        "3,0#   511#GCGGCUGCUGGCACGGAG",
        "3,0#   512#CGCGGCUGCUGGCACGGA",
        "3,0#   513#CCGCGGCUGCUGGCACGG",
        "3,0#   509#GGCUGCUGGCACGGAGUU",
    };
    array2cpa(old_multiprobeInputSave, ARRAY_ELEMS(old_multiprobeInputSave), lines);
    {
        StrArray display, value;
        LocallyModify<char> TorU(T_or_U_for_load, 'U');
        TEST_ASSERT_NO_ERROR(mp_file2list(lines, display, value));
        TEST_ASSERT_ARRAY_EQUALS(display, expected);
        TEST_ASSERT_ARRAY_EQUALS(value, expected);
    }

    const char *new_probeDesignSave[] = {
        "Probe design Parameters:,",
        "Length of probe      18,",
        "Temperature        [30.0 -100.0],",
        "GC-Content         [50.0 -100.0],",
        "E.Coli Position    [any],",
        "Max Non Group Hits     0,",
        "Min Group Hits        50%,",
        "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species,",
        "GCAGCCGCGGUAAUACGG 18 A=  4398  521   23 66.7 60.0    CCGUAUUACCGCGGCUGC |  0;  0;  0;  0;  0;  0;  0;  0; 35; 35; 35; 38; 74; 74; 74; 77;113;113;113;148;,GCAGCCGCGGUAAUACGG",
        "ACUCCGUGCCAGCAGCCG 18 B=  3852  510   23 72.2 62.0    CGGCUGCUGGCACGGAGU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80; 80;120;120;120;200;200;200;200;201;,ACUCCGUGCCAGCAGCCG",
        "CUCCGUGCCAGCAGCCGC 18 B+     4  511   23 77.8 64.0    GCGGCUGCUGGCACGGAG |  0;  0;  0;  0;  0; 40; 40; 40; 40; 80; 80; 80;160;160;160;160;201;201;201;201;,CUCCGUGCCAGCAGCCGC",
        "UCCGUGCCAGCAGCCGCG 18 B+     7  512   23 77.8 64.0    CGCGGCUGCUGGCACGGA |  0;  0;  0;  0;  0; 40; 40; 40;120;120;120;120;160;160;161;201;201;201;202;202;,UCCGUGCCAGCAGCCGCG",
        "CCGUGCCAGCAGCCGCGG 18 B+     9  513   23 83.3 66.0    CCGCGGCUGCUGGCACGG |  0;  0;  0;  0;  0; 80; 80; 80; 80;120;120;121;161;161;161;162;203;203;204;204;,CCGUGCCAGCAGCCGCGG",
        "AACUCCGUGCCAGCAGCC 18 B-     1  509   22 66.7 60.0    GGCUGCUGGCACGGAGUU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80;120;120;120;120;160;160;160;240;240;,AACUCCGUGCCAGCAGCC",
    };
    array2cpa(new_probeDesignSave, ARRAY_ELEMS(new_probeDesignSave), lines);
    {
        StrArray display, value;
        TEST_ASSERT_NO_ERROR(mp_file2list(lines, display, value));
        TEST_ASSERT_ARRAY_EQUALS(display, expected);
        TEST_ASSERT_ARRAY_EQUALS(value, expected);
    }

    const char *new_multiprobeInputSave[] = {
        "3#0#  521#GCAGCCGCGGUAAUACGG",
        "3#0#  510#ACUCCGUGCCAGCAGCCG",
        "3#0#  511#CUCCGUGCCAGCAGCCGC",
        "3#0#  512#UCCGUGCCAGCAGCCGCG",
        "3#0#  513#CCGUGCCAGCAGCCGCGG",
        "3#0#  509#AACUCCGUGCCAGCAGCC",
    };
    array2cpa(new_multiprobeInputSave, ARRAY_ELEMS(new_multiprobeInputSave), lines);
    {
        StrArray display, value;
        TEST_ASSERT_NO_ERROR(mp_file2list(lines, display, value));
        TEST_ASSERT_ARRAY_EQUALS(display, expected);
        TEST_ASSERT_ARRAY_EQUALS(value, expected);
    }


}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
