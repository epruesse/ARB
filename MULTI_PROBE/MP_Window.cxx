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
#include <ad_cb.h>
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
        result_window->callback(makeHelpCallback("multiproberesults.hlp"));
        result_window->create_button("HELP", "HELP");

        result_window->at("Comment");
        result_window->callback(makeWindowCallback(MP_Comment, (const char *)NULL));
        result_window->create_input_field(MP_AWAR_RESULTPROBESCOMMENT);

        result_window->at("box");
        aw_root->awar(MP_AWAR_RESULTPROBES)->add_callback(MP_result_combination_chosen);
        result_probes_list = result_window->create_selection_list(MP_AWAR_RESULTPROBES, true);
        result_probes_list->set_file_suffix("mpr");
        result_probes_list->insert_default("", "");

        const StorableSelectionList *storable_probes_list = new StorableSelectionList(TypedSelectionList("mpr", result_probes_list, "multiprobes", "multi_probes"));

        result_window->at("buttons");
        result_window->callback(makeCreateWindowCallback(create_load_box_for_selection_lists, storable_probes_list));
        result_window->create_button("LOAD_RPL", "LOAD");

        result_window->callback(makeCreateWindowCallback(create_save_box_for_selection_lists, storable_probes_list));
        result_window->create_button("SAVE_RPL", "SAVE");

        result_window->callback(makeWindowCallback(awt_clear_selection_list_cb, result_probes_list));
        result_window->create_button("CLEAR", "CLEAR");

        result_window->callback(makeWindowCallback(MP_delete_selected, result_probes_list));
        result_window->create_button("DELETE", "DELETE");

        // change comment :

        result_window->button_length(8);

        result_window->at("comment");
        result_window->callback(makeWindowCallback(MP_Comment, "Bad"));
        result_window->create_button("MARK_AS_BAD", "BAD");

        result_window->callback(makeWindowCallback(MP_Comment, "???"));
        result_window->create_button("MARK_AS_GOOD", "???");

        result_window->callback(makeWindowCallback(MP_Comment, "Good"));
        result_window->create_button("MARK_AS_BEST", "Good");

        result_window->at("auto");
        result_window->create_toggle(MP_AWAR_AUTOADVANCE);

        // tree actions :

        result_window->button_length(3);

        result_window->at("ct_back");
        result_window->callback(makeWindowCallback(MP_show_probes_in_tree_move, true, result_probes_list));
        result_window->create_button("COLOR_TREE_BACKWARD", "#rightleft_small.xpm");

        result_window->at("ct_fwd");
        result_window->callback(makeWindowCallback(MP_show_probes_in_tree_move, false, result_probes_list));
        result_window->create_button("COLOR_TREE_FORWARD", "#leftright_small.xpm");

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
                char T_or_U = T_or_U_for_load ? T_or_U_for_load : 'U';

                std::string sep = reg_saved.subexpr_match(2)->extract(line[i]);

                int quality   = atoi(reg_saved.subexpr_match(1)->extract(line[i]).c_str());
                int singlemis = atoi(reg_saved.subexpr_match(3)->extract(line[i]).c_str());
                int ecoli     = atoi(reg_saved.subexpr_match(4)->extract(line[i]).c_str());

                std::string probe = reg_saved.subexpr_match(5)->extract(line[i]);

                if (sep[0] == ',') { // old format (saved probe instead of probe-target)
                    size_t  plen   = probe.length();
                    char   *dprobe = GB_strndup(probe.c_str(), plen);

                    GBT_reverseComplementNucSequence(dprobe, plen, T_or_U);
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
            RegExpr reg_designed("^([A-Z]+)"            // subexpr #1 (target)
                                 "[[:space:]]+[0-9]+"
                                 "[[:space:]]+[A-Z][=+-]"
                                 "[[:space:]]*[0-9]+"
                                 "[[:space:]]+([0-9]+)" // subexpr #2 (ecoli pos)
                                 "([[:space:]]+[0-9]+){1,2}" // accept with and without quality entry
                                 "([[:space:]]+[0-9.]+){2}"
                                 "[[:space:]]+[A-Z]+"
                                 "[[:space:]]+[|]", true);

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
                            error = GBS_global_string("probe string mismatch (probe='%s', parsed_probe='%s', parsed from='%s')",
                                                      probe, parsed_probe.c_str(), line[i]);
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

static void track_ali_change_cb(AW_root*, GBDATA *gb_main) {
    GB_transaction     ta(gb_main);
    char              *aliname = GBT_get_default_alignment(gb_main);
    GB_alignment_type  alitype = GBT_get_alignment_type(gb_main, aliname);

    GBT_determine_T_or_U(alitype, &T_or_U_for_load, "reverse-complement"); // T_or_U_for_load is set to 0 in error-case
    GB_clear_error();

    free(aliname);
}

static void MP_collect_probes(AW_window*, awt_collect_mode mode) {
    switch (mode) {
        case ACM_ADD:
            if (!probelist->default_is_selected()) {
                int                        idx = probelist->get_index_of_selected();
                AW_selection_list_iterator sel(probelist, idx);
                selected_list->insert(sel.get_displayed(), sel.get_value());
                MP_delete_selected(NULL, probelist);
            }
            break;

        case ACM_REMOVE:
            if (!selected_list->default_is_selected()) {
                int                        idx = selected_list->get_index_of_selected();
                AW_selection_list_iterator sel(selected_list, idx);
                probelist->insert(sel.get_displayed(), sel.get_value());
                MP_delete_selected(NULL, selected_list);
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

    aw_root->awar(AWAR_DEFAULT_ALIGNMENT)->add_callback(makeRootCallback(track_ali_change_cb, gb_main));
    track_ali_change_cb(aw_root, gb_main);

    result_window = NULL;

    aws = new AW_window_simple;
    aws->init(aw_root, "MULTIPROBE", "MULTI_PROBE");
    aws->load_xfig("multiprobe.fig");

    aws->at("close");
    aws->callback(MP_close_main);
    aws->create_button("CLOSE", "CLOSE");

    aws->at("help");
    aws->callback(makeHelpCallback("multiprobe.hlp"));
    aws->create_button("HELP", "HELP");

    aws->button_length(7);
    aws->at("Selectedprobes");
    aw_root->awar(MP_AWAR_SELECTEDPROBES)->add_callback(MP_selected_chosen);
    selected_list = aws->create_selection_list(MP_AWAR_SELECTEDPROBES, max_seq_col, max_seq_hgt, true);

    const StorableSelectionList *storable_selected_list = new StorableSelectionList(TypedSelectionList("prb", selected_list, "probes", "selected_probes"), mp_list2file, mp_file2list);

    selected_list->insert_default("", "");

    aws->at("Probelist");
    probelist = aws->create_selection_list(MP_AWAR_PROBELIST, true);
    const StorableSelectionList *storable_probelist = new StorableSelectionList(TypedSelectionList("prb", probelist, "probes", "all_probes"), mp_list2file, mp_file2list);
    probelist->insert_default("", "");

    aws->at("collect");
    awt_create_collect_buttons(aws, true, MP_collect_probes);
    
    aws->auto_space(5, 5);
    aws->button_length(7);

    for (int rightSide = 0; rightSide <= 1; ++rightSide) {
        const StorableSelectionList *storableList = rightSide ? storable_selected_list : storable_probelist;
        const char                  *id_suffix    = rightSide ? "SELECTED_PROBES" : "PROBES";

        AW_selection_list *sellist = storableList->get_typedsellist().get_sellist();

        aws->at(rightSide ? "RightButtons" : "LeftButtons");

        aws->callback(makeCreateWindowCallback(create_load_box_for_selection_lists, storableList));
        aws->create_button(GBS_global_string("LOAD_%s", id_suffix), "LOAD");

        aws->callback(makeCreateWindowCallback(create_save_box_for_selection_lists, storableList));
        aws->create_button(GBS_global_string("SAVE_%s", id_suffix), "SAVE");

        aws->callback(makeWindowCallback(awt_clear_selection_list_cb, sellist));
        aws->create_button(GBS_global_string("CLEAR_%s", id_suffix), "CLEAR");

        aws->callback(makeWindowCallback(MP_delete_selected, sellist));
        aws->create_button(GBS_global_string("DELETE_%s", id_suffix), "DELETE");
    }

    aws->at("Quality");
    aws->callback(MP_cache_sonden); // @@@ do not bind callback to option menus (rel. #559) // @@@ search for occurrences arb-wide
    aws->create_option_menu(MP_AWAR_QUALITY, true);
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
    awt_create_PTSERVER_selection_button(aws, MP_AWAR_PTSERVER);
    aw_root->awar(MP_AWAR_PTSERVER)->add_callback(MP_cache_sonden2); // remove cached probes when changing pt-server

    aws->at("NoOfProbes");
    aws->create_option_menu(MP_AWAR_NOOFPROBES, true);
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
    aws->callback(makeWindowCallback(MP_compute, gb_main));
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
    aws->create_option_menu(MP_AWAR_OUTSIDEMISMATCHES, true);
    aws->insert_option("3.0", "", (float)3.0);
    aws->insert_option("2.5", "", (float)2.5);
    aws->insert_option("2.0", "", (float)2.0);
    aws->insert_option("1.5", "", (float)1.5);
    aws->insert_option("1.0", "", (float)1.0);
    aws->update_option_menu();

    // max mismatches for group
    aws->at("Greyzone");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu(MP_AWAR_GREYZONE, true);
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
#include <command_output.h>

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

static arb_test::match_expectation inputConvertsInto(const char *input, const char *expected_result) {
    ConstStrArray lines;
    GBT_split_string(lines, input, "\n", true);

    using namespace   arb_test;
    expectation_group expected;

    StrArray display, value;
    expected.add(doesnt_report_error(mp_file2list(lines, display, value)));

    char *displ_as_string = array2string(display);
    char *value_as_string = array2string(value);

    expected.add(that(displ_as_string).is_equal_to(expected_result));
    expected.add(that(value_as_string).is_equal_to(expected_result));

    free(value_as_string);
    free(displ_as_string);

    return all().ofgroup(expected);
}

#define TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(input,expected)         TEST_EXPECTATION(inputConvertsInto(input, expected))
#define TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS__BROKEN(input,expected) TEST_EXPECTATION__BROKEN(inputConvertsInto(input, expected))

void TEST_load_probe_design_results() {
    {
        const char *expected =
            "3#0#  521#GCAGCCGCGGUAAUACGG\n"
            "3#0#  510#ACUCCGUGCCAGCAGCCG\n"
            "3#0#  511#CUCCGUGCCAGCAGCCGC\n"
            "3#0#  512#UCCGUGCCAGCAGCCGCG\n"
            "3#0#  513#CCGUGCCAGCAGCCGCGG\n"
            "3#0#  509#AACUCCGUGCCAGCAGCC\n";

        const char *old_probeDesignSave =
            "Probe design Parameters:\n"
            "Length of probe      18\n"
            "Temperature        [30.0 -100.0]\n"
            "GC-Content         [50.0 -100.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits     0\n"
            "Min Group Hits        50%\n"
            "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species\n"
            "GCAGCCGCGGUAAUACGG 18 A=  4398  521   23 66.7 60.0    CCGUAUUACCGCGGCUGC |  0;  0;  0;  0;  0;  0;  0;  0; 35; 35; 35; 38; 74; 74; 74; 77;113;113;113;148;\n"
            "ACUCCGUGCCAGCAGCCG 18 B=  3852  510   23 72.2 62.0    CGGCUGCUGGCACGGAGU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80; 80;120;120;120;200;200;200;200;201;\n"
            "CUCCGUGCCAGCAGCCGC 18 B+     4  511   23 77.8 64.0    GCGGCUGCUGGCACGGAG |  0;  0;  0;  0;  0; 40; 40; 40; 40; 80; 80; 80;160;160;160;160;201;201;201;201;\n"
            "UCCGUGCCAGCAGCCGCG 18 B+     7  512   23 77.8 64.0    CGCGGCUGCUGGCACGGA |  0;  0;  0;  0;  0; 40; 40; 40;120;120;120;120;160;160;161;201;201;201;202;202;\n"
            "CCGUGCCAGCAGCCGCGG 18 B+     9  513   23 83.3 66.0    CCGCGGCUGCUGGCACGG |  0;  0;  0;  0;  0; 80; 80; 80; 80;120;120;121;161;161;161;162;203;203;204;204;\n"
            "AACUCCGUGCCAGCAGCC 18 B-     1  509   22 66.7 60.0    GGCUGCUGGCACGGAGUU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80;120;120;120;120;160;160;160;240;240;\n";

        TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(old_probeDesignSave, expected);


        const char *old_multiprobeInputSave = // old multi-probe saved probe (i.e. not target) sequences -> load shall correct that
            "3,0#   521#CCGUAUUACCGCGGCUGC\n"
            "3,0#   510#CGGCUGCUGGCACGGAGU\n"
            "3,0#   511#GCGGCUGCUGGCACGGAG\n"
            "3,0#   512#CGCGGCUGCUGGCACGGA\n"
            "3,0#   513#CCGCGGCUGCUGGCACGG\n"
            "3,0#   509#GGCUGCUGGCACGGAGUU\n";

        {
            LocallyModify<char> TorU(T_or_U_for_load, 'U');
            TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(old_multiprobeInputSave, expected);
        }

        const char *new_probeDesignSave_v1 =
            "Probe design Parameters:,\n"
            "Length of probe      18,\n"
            "Temperature        [30.0 -100.0],\n"
            "GC-Content         [50.0 -100.0],\n"
            "E.Coli Position    [any],\n"
            "Max Non Group Hits     0,\n"
            "Min Group Hits        50%,\n"
            "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species,\n"
            "GCAGCCGCGGUAAUACGG 18 A=  4398  521   23 66.7 60.0    CCGUAUUACCGCGGCUGC |  0;  0;  0;  0;  0;  0;  0;  0; 35; 35; 35; 38; 74; 74; 74; 77;113;113;113;148;,GCAGCCGCGGUAAUACGG\n"
            "ACUCCGUGCCAGCAGCCG 18 B=  3852  510   23 72.2 62.0    CGGCUGCUGGCACGGAGU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80; 80;120;120;120;200;200;200;200;201;,ACUCCGUGCCAGCAGCCG\n"
            "CUCCGUGCCAGCAGCCGC 18 B+     4  511   23 77.8 64.0    GCGGCUGCUGGCACGGAG |  0;  0;  0;  0;  0; 40; 40; 40; 40; 80; 80; 80;160;160;160;160;201;201;201;201;,CUCCGUGCCAGCAGCCGC\n"
            "UCCGUGCCAGCAGCCGCG 18 B+     7  512   23 77.8 64.0    CGCGGCUGCUGGCACGGA |  0;  0;  0;  0;  0; 40; 40; 40;120;120;120;120;160;160;161;201;201;201;202;202;,UCCGUGCCAGCAGCCGCG\n"
            "CCGUGCCAGCAGCCGCGG 18 B+     9  513   23 83.3 66.0    CCGCGGCUGCUGGCACGG |  0;  0;  0;  0;  0; 80; 80; 80; 80;120;120;121;161;161;161;162;203;203;204;204;,CCGUGCCAGCAGCCGCGG\n"
            "AACUCCGUGCCAGCAGCC 18 B-     1  509   22 66.7 60.0    GGCUGCUGGCACGGAGUU |  0;  0;  0;  0;  0; 40; 40; 40; 80; 80; 80;120;120;120;120;160;160;160;240;240;,AACUCCGUGCCAGCAGCC\n";

        TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(new_probeDesignSave_v1, expected);


        const char *new_multiprobeInputSave =
            "3#0#  521#GCAGCCGCGGUAAUACGG\n"
            "3#0#  510#ACUCCGUGCCAGCAGCCG\n"
            "3#0#  511#CUCCGUGCCAGCAGCCGC\n"
            "3#0#  512#UCCGUGCCAGCAGCCGCG\n"
            "3#0#  513#CCGUGCCAGCAGCCGCGG\n"
            "3#0#  509#AACUCCGUGCCAGCAGCC\n";

        TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(new_multiprobeInputSave, expected);
    }
}

static const char *recent_expected =
    "3#0#   82#CGAAAGGAAGAUUA\n"
    "3#0#   82#CGAAAGGAAGAUUAA\n"
    "3#0#   86#AGGAAGAUUAAUACC\n"
    "3#0#   87#GGAAGAUUAAUACC\n"
    "3#0#   21#GUCGAGCGAUGAAG\n"
    "3#0#   20#AGUCGAGCGAUGAAG\n"
    "3#0#   20#AGUCGAGCGAUGAA\n"
    "3#0#   19#AAGUCGAGCGAUGAA\n"
    "3#0#   18#CAAGUCGAGCGAUGA\n"
    "3#0#   19#AAGUCGAGCGAUGA\n"
    "3#0#   17#UCAAGUCGAGCGAUG\n"
    "3#0#   18#CAAGUCGAGCGAUG\n"
    "3#0#   16#AUCAAGUCGAGCGAU\n"
    "3#0#   17#UCAAGUCGAGCGAU\n"
    "3#0#   16#AUCAAGUCGAGCGA\n";

static const char *recent_probeDesignSave =
    "Probe design parameters:,\n"
    "Length of probe    14-15,\n"
    "Temperature        [ 0.0 -400.0],\n"
    "GC-content         [30.0 - 80.0],\n"
    "E.Coli position    [any],\n"
    "Max. nongroup hits 0,\n"
    "Min. group hits    100% (max. rejected coverage: 75%),\n"
    "Target          le apos ecol qual grps   G+C temp  Probe sequence | Decrease T by n*.3C -> probe matches n non group species,\n"
    "CGAAAGGAAGAUUA  14 A=94   82   77    4  35.7 38.0  UAAUCUUCCUUUCG | - - - - - - - - - - - - - - - - - - - -,CGAAAGGAAGAUUA\n"
    "CGAAAGGAAGAUUAA 15 A+ 0   82   77    4  33.3 40.0 UUAAUCUUCCUUUCG | - - - - - - - - - - - - - - - - - - - -,CGAAAGGAAGAUUAA\n"
    "AGGAAGAUUAAUACC 15 A+ 4   86   77    4  33.3 40.0 GGUAUUAAUCUUCCU | - - - - - - - - - - - - - - - - - - - -,AGGAAGAUUAAUACC\n"
    "GGAAGAUUAAUACC  14 A+ 5   87   77    4  35.7 38.0  GGUAUUAAUCUUCC | - - - - - - - - - - - - - - - - - - - -,GGAAGAUUAAUACC\n"
    "GUCGAGCGAUGAAG  14 B=22   21   77    4  57.1 44.0  CUUCAUCGCUCGAC | - - - - - - - - - - - - - - - - - - - 2,GUCGAGCGAUGAAG\n"
    "AGUCGAGCGAUGAAG 15 B- 1   20   73    4  53.3 46.0 CUUCAUCGCUCGACU | - - - - - - - - - - - - - - - - - - 2 2,AGUCGAGCGAUGAAG\n"
    "AGUCGAGCGAUGAA  14 B- 1   20   57    4  50.0 42.0  UUCAUCGCUCGACU | - - - - - - - - - - - - - - 2 2 2 2 2 2,AGUCGAGCGAUGAA\n"
    "AAGUCGAGCGAUGAA 15 B- 2   19   53    4  46.7 44.0 UUCAUCGCUCGACUU | - - - - - - - - - - - - - 2 2 2 2 2 2 2,AAGUCGAGCGAUGAA\n"
    "CAAGUCGAGCGAUGA 15 B- 3   18   41    4  53.3 46.0 UCAUCGCUCGACUUG | - - - - - - - - - - 2 2 2 2 2 2 2 2 2 2,CAAGUCGAGCGAUGA\n"
    "AAGUCGAGCGAUGA  14 B- 2   19   41    4  50.0 42.0  UCAUCGCUCGACUU | - - - - - - - - - - 2 2 2 2 2 2 2 2 2 2,AAGUCGAGCGAUGA\n"
    "UCAAGUCGAGCGAUG 15 B- 4   17   25    4  53.3 46.0 CAUCGCUCGACUUGA | - - - - - - 2 2 2 2 2 2 2 2 2 2 2 2 9 9,UCAAGUCGAGCGAUG\n"
    "CAAGUCGAGCGAUG  14 B- 3   18   25    4  57.1 44.0  CAUCGCUCGACUUG | - - - - - - 2 2 2 2 2 2 2 2 2 2 2 2 2 2,CAAGUCGAGCGAUG\n"
    "AUCAAGUCGAGCGAU 15 B- 5   16    5    4  46.7 44.0 AUCGCUCGACUUGAU | - 2 2 2 2 2 2 2 9 9 9 9 9 9 9 9 9 9 9 9,AUCAAGUCGAGCGAU\n"
    "UCAAGUCGAGCGAU  14 B- 4   17    5    4  50.0 42.0  AUCGCUCGACUUGA | - 2 2 2 2 2 2 2 2 9 9 9 9 9 9 9 9 9 9 9,UCAAGUCGAGCGAU\n"
    "AUCAAGUCGAGCGA  14 B- 5   16    5    4  50.0 42.0  UCGCUCGACUUGAU | - 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9,AUCAAGUCGAGCGA";

void TEST_AFTER_SLOW_recent_probe_design_result() {
    // --------------------------------------------------------------------------------
    // whenever probe design output changes, copy current 'recent_probeDesignSave' and
    // 'recent_expected' into TEST_load_probe_design_results, to ensure ARB can load
    // any saved probe design ever created with ARB.

    TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(recent_probeDesignSave, recent_expected);
}

void TEST_SLOW_design_probes_and_load_result() {
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");

    CommandOutput designed_probes("arb_probe serverid=-666 designprobelength=14 designmaxprobelength=15 designnames=ClnCorin#CltBotul#CPPParap#ClfPerfr designmintargets=100", true);
    TEST_EXPECT_NO_ERROR(designed_probes.get_error());

    // Simulate result of designing probes in ARB_NT and saving the result to a file:
    char *saved_design_result = NULL; // content of that file
    {
        ConstStrArray lines;
        GBT_split_string(lines, designed_probes.get_stdoutput(), "\n", true);

        StrArray saved_lines;

        for (size_t i = 0; i<lines.size(); ++i) {
            char *probe; // same as awar-value of probe-design-resultlist in ARB_NT
            {
                size_t plen = strspn(lines[i], "acgtuACGTU");
                if (plen<10) { // no probe at start // @@@ 10 is min. probelen, use a global definition here!
                    probe = strdup("");
                }
                else {
                    probe = GB_strndup(lines[i], plen);
                }
            }

            char *conv4save = GBS_string_eval(lines[i], ":,=;", NULL); // saving selection list converts comma to semicolon
            arb_assert(conv4save);

            saved_lines.put(GBS_global_string_copy("%s,%s", conv4save, probe));

            free(conv4save);
            free(probe);
        }

        saved_design_result = GBT_join_strings(saved_lines, '\n');
        TEST_EXPECT_EQUAL(saved_design_result, recent_probeDesignSave); // see comment in TEST_LATE_recent_probe_design_result
    }

    TEST_EXPECT_LOADS_INTO_MULTIPROBE_AS(saved_design_result, recent_expected);
    free(saved_design_result);
}


#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
