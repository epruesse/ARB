// =============================================================== //
//                                                                 //
//   File      : MP_noclass.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "MultiProbe.hxx"
#include "mp_proto.hxx"
#include "MP_externs.hxx"
#include "MP_probe.hxx"

#include <aw_select.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <TreeCallbacks.hxx>

#include <client.h>
#include <servercntrl.h>

#include <arb_progress.h>
#include <arb_defs.h>
#include <arb_strarray.h>

#include <ctime>

void MP_close_main(AW_window *aww) { // @@@ implement via on_hide
    AWT_canvas  *scr = mp_main->get_canvas();

    if (mp_main->get_mp_window()->get_result_window())
        mp_main->get_mp_window()->get_result_window()->hide();

    GB_transaction dummy(scr->gb_main);

    AP_tree *ap_tree = AWT_TREE(scr)->get_root_node();
    if (ap_tree) ap_tree->calc_color();

    if (scr->gb_main)
        scr->gfx->update(scr->gb_main);

    scr->refresh();

    AW_POPDOWN(aww);
    mp_global->invalidate_probe_cache();
}

static char *MP_get_probes(const char *str) {
    const char *result = strrchr(str, '#');

    if (!result) {
        result = str;
    }
    else {
        ++result;
        result += strspn(result, " \t"); // skip over whitespace
    }
    return strdup(result);
}

void MP_gen_quality(AW_root *awr, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    bool firsttime = true; // @@@ was this static in the past?

    if (firsttime) {
        firsttime = false; // @@@ unused
        return;
    }

    char *probe, *new_qual, *ecol_pos;
    char *selected = awr->awar(MP_AWAR_SELECTEDPROBES)->read_string();

    if (!selected || !selected[0])
        return;

    probe = MP_get_probes(selected);
    if (!probe || !probe[0])
        return;

    ecol_pos = MP_get_comment(3, selected);

    selected_list->delete_value(selected);

    new_qual = new char[5 + 7 + strlen(probe)];     // 5 = Zahl und Separator und Zahl und Separator und Nullzeichen
    sprintf(new_qual, "%1ld#%1ld#%6d#%s", mp_gl_awars.probe_quality, mp_gl_awars.singlemismatches, atoi(ecol_pos), probe);
    delete probe;

    selected_list->insert(new_qual, new_qual);
    selected_list->insert_default("", "");
    selected_list->sort(false, true);
    selected_list->update();
    awr->awar(MP_AWAR_SELECTEDPROBES)->write_string(new_qual);
    delete new_qual;
    delete ecol_pos;
}

void MP_modify_selected(AW_root *awr, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    // setzt den 2.Parameter in selected_list
    char       *com1, *com2, *com3, *probes, temp[120];
    List<char> *l   = new List<char>;

    AW_selection_list_iterator selentry(selected_list);

    const char *ptr2;
    while ((ptr2 = selentry.get_value())) {
        ++selentry;

        com1 =  MP_get_comment(1, ptr2);
        com2 =  MP_get_comment(2, ptr2);
        com3 =  MP_get_comment(3, ptr2);
        probes = MP_get_probes(ptr2);

        if (!probes || !probes[0])
            break;

        sprintf(temp, "%1d#%1ld#%6d#%s", atoi(com1), mp_gl_awars.no_of_mismatches, atoi(com3), probes);

        l->insert_as_last(strdup(temp));

        delete probes;
        delete com1;
        delete com2;
        delete com3;
    }

    selected_list->clear();

    ptr2 = l->get_first();
    while (ptr2) {
        l->remove_first();
        selected_list->insert(ptr2, ptr2);
        delete ptr2;
        ptr2 = l->get_first();
    }

    selected_list->insert_default("", "");
    selected_list->sort(false, true);
    selected_list->update();
    awr->awar(MP_AWAR_SELECTEDPROBES)->write_string("");

    delete l;
}

void MP_gen_singleprobe(AW_root *awr, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    char *probe, *new_sing;
    char *selected = awr->awar(MP_AWAR_SELECTEDPROBES)->read_string();

    if (!selected || !selected[0])
        return;

    probe = MP_get_probes(selected);
    selected_list->delete_value(selected);

    new_sing = new char[5 + 7 + strlen(probe)];     // 5 = Zahl und Separator und Zahl und Separator und Nullzeichen
    sprintf(new_sing, "%1ld#%1ld#%6ld#%s", mp_gl_awars.probe_quality, mp_gl_awars.singlemismatches, mp_gl_awars.ecolipos, probe);
    delete probe;

    selected_list->insert(new_sing, new_sing);
    selected_list->insert_default("", "");
    selected_list->sort(false, true);
    selected_list->update();
    awr->awar(MP_AWAR_SELECTEDPROBES)->write_string(new_sing);
    delete new_sing;
}

void MP_popup_result_window(AW_window */*aww*/) {
    mp_main->get_mp_window()->create_result_window(mp_main->get_aw_root())->activate();
}

bool MP_aborted(int gen_cnt, double avg_fit, double min_fit, double max_fit, arb_progress& progress) {
    char view_text[150];

    if (gen_cnt == 0)
        sprintf(view_text, "Evaluating first generation");
    else
        sprintf(view_text, "Gen:%d Avg:%5i Min:%5i Max:%5i", gen_cnt, int(avg_fit), int(min_fit), int(max_fit));

    progress.subtitle(view_text);
    return progress.aborted();
}

__ATTR__USERESULT static GB_ERROR calc_multiprobes(const CharPtrArray& input_probes, StrArray& results) {
    mp_assert(mp_global);
    size_t count = input_probes.size();

#if defined(DEBUG)
    for (size_t i = 0; i<count; ++i) {
        fprintf(stderr, "\"%s\",\n", input_probes[i]);
    }
#endif


    char **probe_field     = new char*[count];
    int   *bew_array       = new int[count];
    int   *single_mismatch = new int[count];
    int   *ecolipos        = new int[count];

    int added = 0;
    for (size_t i = 0; i<count; ++i) {
        char *probe = MP_get_probes(input_probes[i]);
        if (probe && strchr(" \t", probe[0]) == 0) {
            char *qual   = MP_get_comment(1, input_probes[i]);
            bew_array[added] = atoi(qual);
            free(qual);

            char *smis         = MP_get_comment(2, input_probes[i]);
            single_mismatch[added] = atoi(smis); // single mismatch kann zwar eingestellt werden, aber wird noch nicht uebergeben
            free(smis);

            char *ecoli         = MP_get_comment(3, input_probes[i]);
            ecolipos[added] = atoi(ecoli);
            free(ecoli);

            probe_field[added] = probe;

            added++;
        }
        else {
            free(probe);
        }
    }

    ProbeValuation multiprobes(probe_field, added, bew_array, single_mismatch, ecolipos); // takes ownership of params
    return multiprobes.evaluate(results);
}


static long remove_from_hash(const char *species_name, long val, void *cl_hash) {
    GB_HASH *hash = (GB_HASH*)cl_hash;
    GBS_write_hash(hash, species_name, 0);
    return val;
}

void MP_init_and_calculate_and_display_multiprobes(AW_window *, AW_CL cl_gb_main) {
    GBDATA  *gb_main = (GBDATA*)cl_gb_main;
    AW_root *aw_root = mp_main->get_aw_root();

    if (mp_global) {
        delete mp_global;
        mp_global = NULL;
    }
    mp_global = new MP_Global;

    {
        GB_transaction ta(gb_main);
        GB_HASH *marked_species   = GBT_create_marked_species_hash(gb_main);
        GB_HASH *unmarked_species = GBT_create_species_hash(gb_main);

        GBS_hash_do_loop(marked_species, remove_from_hash, unmarked_species); // remove marked from unmarked

        mp_global->set_marked_species(marked_species);
        mp_global->set_unmarked_species(unmarked_species);
    }

    size_t selected_probes_count = selected_list->size(); // @@@ move check into calc_multiprobes
    if ((int)selected_probes_count < mp_gl_awars.no_of_probes) {
        aw_message("Not enough probes selected for computation !!!");
        return;
    }

    mp_global->invalidate_probe_cache();

    AW_window *aww2 = mp_main->get_mp_window()->create_result_window(aw_root);

    result_probes_list->clear();
    result_probes_list->insert_default("", "");
    result_probes_list->update();

    arb_progress progress("Computing multiprobes");

    StrArray input_probes;
    selected_list->to_array(input_probes, true);

    StrArray results;
    GB_ERROR error = calc_multiprobes(input_probes, results);

    result_probes_list->clear();
    for (size_t i = 0; i<results.size(); ++i) {
        result_probes_list->insert(results[i], results[i]);
    }
    result_probes_list->insert_default("", "");
    result_probes_list->update();

    mp_main->get_mp_window()->get_result_window()->activate();

    if (error) aw_message(error);

    aww2->activate();
}

class BaseTab {
    bool isBase[256];
public:
    BaseTab() {
        for (int i=0; i<256; i++) {
            isBase[i] = false;
        }

        const char *true_chars = "atgucnATGUCN";
        for (int i = 0; true_chars[i]; ++i) {
            isBase[safeCharIndex(true_chars[i])] = true;
        }
    }
    bool is_base(char c) const { return isBase[safeCharIndex(c)]; }
};

inline bool is_probe(char *seq) {
    static BaseTab tab;

    bool result = false;
    if (seq) {
        char *seq2 = MP_get_probes(seq);
        if (seq2 && seq2[0]) {

            char *s = seq2;
            result  = true;
            while (*s && result) result = result && tab.is_base(*s++);
        }
        free(seq2);
    }
    return result;
}

void MP_new_sequence(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char    *seq     = aw_root->awar(MP_AWAR_SEQIN)->read_string();

    if (is_probe(seq)) {
        char *new_seq = GBS_global_string_copy("%1ld#%1ld#%6d#%s", mp_gl_awars.probe_quality, mp_gl_awars.singlemismatches, 0, seq);

        selected_list->insert(new_seq, new_seq);
        selected_list->sort(false, true);
        selected_list->update();

        aw_root->awar(MP_AWAR_SELECTEDPROBES)->write_string(new_seq);
        free(new_seq);
    }
    else {
        aw_message(GBS_global_string("'%s' is no valid probe", seq));
    }
    free(seq);
}

void MP_color_next_probes_in_tree(AW_window *aww, AW_CL cl_backward, AW_CL cl_result_probes_list) {
    bool               backward           = bool(cl_backward);
    AW_selection_list *resultProbesList = (AW_selection_list*)cl_result_probes_list;

    resultProbesList->move_selection(backward ? -1 : 1);
    MP_color_probes_in_tree(aww);
}

static MultiProbeCombinations *combinationsOfSelectedMultiprobe(GB_ERROR& error) {
    mp_assert(!error);

    MultiProbeCombinations *topf = NULL;
    if (!mp_global) {
        error = "No multiprobes calculated yet";
    }
    else {
        char *probeSpec;
        char *mismatchSpec = NULL;
        {
            char *sel = mp_main->get_aw_root()->awar(MP_AWAR_RESULTPROBES)->read_string();
            probeSpec = MP_get_probes(sel);   // haelt jetzt Sondenstring
            if (!probeSpec || !probeSpec[0]) {
                free(probeSpec);
                probeSpec = NULL;
                error     = "No probe combination selected";
            }
            else {
                mismatchSpec = MP_get_comment(2, sel);
            }
            free(sel);
        }

        if (!error) {
            char **probe_field = new char*[MAXPROBECOMBIS];
            int   *mismatches  = new int[MAXPROBECOMBIS];

            for (int i=0; i<MAXPROBECOMBIS; i++) {
                probe_field[i] = NULL;
                mismatches[i]  = 0;
            }

            {
                char *currMismatch = mismatchSpec;
                char *currProbe    = probeSpec;
                int   probe_count  = 0;

                for (int i=0; i< MAXPROBECOMBIS; i++) {     // generiert  probe_field und mismatches array
                    {
                        char *behind            = strchr(currMismatch, ' ');
                        if (behind) *(behind++) = 0;
                        mismatches[i]           = atoi(currMismatch);
                        currMismatch            = behind;
                    }

                    probe_field[i] = NULL;

                    char *the_probe = currProbe;
                    currProbe       = strchr(currProbe, ' ');

                    if (currProbe) {
                        *(currProbe++) = 0;
                        while (*currProbe == ' ' || *currProbe == '\t') currProbe++;
                    }
                    else {
                        probe_field[i] = strdup(the_probe);
                        probe_count++;
                        break;
                    }

                    if (the_probe && the_probe[0]) {
                        probe_field[i] = strdup(the_probe);
                        probe_count++;
                    }
                }

                // @@@ what todo if 'probe_count' mismatches ? 
            }

            if (!error) {
                ProbeCache *probeCache = mp_global->get_probe_cache(error);

                if (!error) {
                    topf = new MultiProbeCombinations(probeCache->get_TargetGroup());

                    mp_assert(!error);
                    for (int i=0; i<MAXPROBECOMBIS && !error; i++) {
                        if (probe_field[i]) {
                            topf->add_probe(probe_field[i], mismatches[i], mismatches[i] + mp_gl_awars.outside_mismatches_difference, error);
                        }
                    }

                    if (error) {
                        delete topf;
                        topf = NULL;
                    }
                }
            }

            for (int i = 0; i<MAXPROBECOMBIS; ++i) {
                free(probe_field[i]);
            }

            delete [] mismatches;
            delete [] probe_field;
        }

        free(probeSpec);
        free(mismatchSpec);
    }

    mp_assert(contradicted(topf, error));
    
    return topf;
}

void MP_color_probes_in_tree(AW_window */*aww*/) {
    GB_ERROR                error  = NULL;
    MultiProbeCombinations *combis = combinationsOfSelectedMultiprobe(error);

    if (error) {
        aw_message(error);
    }
    else {
        AWT_canvas     *scr = mp_main->get_canvas();
        GB_transaction  ta(scr->gb_main);

        AWT_TREE(scr)->get_root_node()->calc_color_probes(combis->get_color_hash());
        if (scr->gb_main) scr->gfx->update(scr->gb_main);
        scr->refresh();
    }

    delete combis;
}

void MP_mark_probes_in_tree(AW_window *aww) {
    GB_ERROR                error  = NULL;
    MultiProbeCombinations *combis = combinationsOfSelectedMultiprobe(error);

    if (error) {
        aw_message(error);
    }
    else {
        AWT_canvas *scr = mp_main->get_canvas();
        {
            GB_transaction ta(scr->gb_main);
            const GB_HASH *col_hash = combis->get_color_hash();
            for (GBDATA *gb_species = GBT_first_species(scr->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
                GB_write_flag(gb_species, GBS_read_hash(col_hash, GBT_read_name(gb_species)) > AWT_GC_BLACK);
            }
        }

        {
            GB_transaction ta(scr->gb_main);
            if (scr->gb_main) scr->gfx->update(scr->gb_main);
            scr->refresh();
        }
    }

    MP_normal_colors_in_tree(aww);

    delete combis;
}

void MP_Comment(AW_window *, AW_CL cl_com) {
    // Comment fuer Auswahl eintragen

    AW_root *awr       = mp_main->get_aw_root();
    char    *selprobes = awr->awar(MP_AWAR_RESULTPROBES)->read_string();
    char    *new_val   = MP_get_probes(selprobes);

    if (new_val && new_val[0]) {
        char *new_list_entry;
        {
            char       *edited_comment = NULL;
            const char *comment        = NULL;

            if (cl_com) {
                comment = (char*)cl_com;
            }
            else {
                edited_comment = awr->awar(MP_AWAR_RESULTPROBESCOMMENT)->read_string();
                for (int i = 0; edited_comment[i]; ++i) { // remove all '#' from new comment
                    if (edited_comment[i] == SEPARATOR[0]) {
                        edited_comment[i] = '|';
                    }
                }
            }

            char *misms = MP_get_comment(2, selprobes);
            char *ecol  = MP_get_comment(3, selprobes);

            new_list_entry = GBS_global_string_copy("%-20s" SEPARATOR "%s" SEPARATOR "%s" SEPARATOR "%s",
                              comment, misms, ecol, new_val);
            free(ecol);
            free(misms);
            free(edited_comment);
        }

        {
            bool autoadvance = awr->awar(MP_AWAR_AUTOADVANCE)->read_int();

            if (autoadvance) result_probes_list->move_selection(1);

            result_probes_list->delete_value(selprobes);
            result_probes_list->insert(new_list_entry, new_list_entry);
            result_probes_list->update();

            if (!autoadvance) awr->awar(MP_AWAR_RESULTPROBES)->write_string(new_list_entry);
        }
        free(new_list_entry);
    }

    free(new_val);
    free(selprobes);
}

void MP_selected_chosen(AW_window */*aww*/) {
    AW_root *aw_root  = mp_main->get_aw_root();
    char    *selected = aw_root->awar(MP_AWAR_SELECTEDPROBES)->read_string();
    if (selected && selected[0]) {
        char *probe = MP_get_comment(3, selected);
        if (probe) {
            aw_root->awar(MP_AWAR_ECOLIPOS)->write_int(atoi(probe));
            free(probe);

            if (probe) {
                probe = MP_get_comment(1, selected);
                aw_root->awar(MP_AWAR_QUALITY)->write_int(atoi(probe));
                free(probe);

                if (probe) {
                    probe = MP_get_comment(2, selected);
                    aw_root->awar(MP_AWAR_SINGLEMISMATCHES)->write_int(atoi(probe));
                    free(probe);

                    if (probe) {
                        probe = MP_get_probes(selected);
                        aw_root->awar_string(MP_AWAR_SEQIN)->write_string(MP_get_probes(probe));
                        free(probe);
                        return;
                    }
                }
            }
        }
        aw_message("can't parse entry");
    }
}

void MP_group_all_except_marked(AW_window * /* aww */) {
    AWT_canvas  *scr = mp_main->get_canvas();
    NT_group_not_marked_cb(0, scr);
}

void MP_normal_colors_in_tree(AW_window */*aww*/) {
    AWT_canvas  *scr = mp_main->get_canvas();
    GB_transaction dummy(scr->gb_main);

    AWT_TREE(scr)->get_root_node()->calc_color();

    if (scr->gb_main)
        scr->gfx->update(scr->gb_main);

    scr->refresh();
}

void MP_delete_selected(AW_window*, AW_CL cl_sellist) {
    AW_selection_list *sellist = (AW_selection_list*)cl_sellist;

    int idx = sellist->get_index_of_selected();
    sellist->delete_element_at(idx);
    sellist->select_element_at(idx);
    sellist->update();
}


char *MP_get_comment(int which, const char *str) {
    // Parses information from strings like "val1#val2#val3#probes".
    // 'which' == 1 -> 'val1'
    // 'which' == 2 -> 'val2'
    // ...
    // Values may be present or not (i.e. 'probes', 'val1#probes', ... are also valid as 'str')

    char       *result  = NULL;
    const char *numsign = strchr(str, '#');

    mp_assert(which >= 1);

    if (numsign) {
        if (which == 1) {
            result = GB_strpartdup(str, numsign-1);
        }
        else {
            result = MP_get_comment(which-1, numsign+1);
        }
    }

    return result;
}

void MP_result_chosen(AW_window */*aww*/) {
    AW_root *aw_root = mp_main->get_aw_root();
    char    *str     = aw_root->awar(MP_AWAR_RESULTPROBES)->read_as_string();
    char    *new_str;

    new_str = MP_get_comment(1, str);

    aw_root->awar(MP_AWAR_RESULTPROBESCOMMENT)->write_string(new_str);
    free(str);
    free(new_str);
}



//
// functions concerning the server
//

int MP_init_local_com_struct() {
    const char *user = GB_getenvUSER();

    if (aisc_create(mp_pd_gl.link, PT_MAIN, mp_pd_gl.com,
                    MAIN_LOCS, PT_LOCS, mp_pd_gl.locs,
                    LOCS_USER, user,
                    NULL)) {
        return 1;
    }
    return 0;
}

const char *MP_probe_pt_look_for_server(GB_ERROR& error) {
    mp_assert(!error);

    const char *server_tag = GBS_ptserver_tag(mp_gl_awars.ptserver);
    error                  = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag);

    const char *result = NULL;
    if (!error) result = GBS_read_arb_tcp(server_tag);
    return result;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#define MP_GET_COMMENT_EQUAL(which, str, expect)  \
    do {                                          \
        char *got = MP_get_comment(which, str);   \
        TEST_ASSERT_EQUAL(got, expect);           \
        free(got);                                \
    } while(0)                                    \
 
#define MP_GET_PROBES_EQUAL(str, expect)          \
    do {                                          \
        char *got = MP_get_probes(str);           \
        TEST_ASSERT_EQUAL(got, expect);           \
        free(got);                                \
    } while(0)                                    \
 
void TEST_MP_get_comment_and_probes() {
    char *probes_only    = strdup("ACGT TGCA ATCG");
    char *probes_comment = strdup("val1#val2#val3#ACGT TGCA ATCG");
    char *probes_1 = strdup("one#     \t    ACGT TGCA ATCG");
    char *probes_2 = strdup("one#two#\tACGT TGCA ATCG");

    MP_GET_PROBES_EQUAL(probes_only,    "ACGT TGCA ATCG");
    MP_GET_PROBES_EQUAL(probes_comment, "ACGT TGCA ATCG");
    MP_GET_PROBES_EQUAL(probes_1,       "ACGT TGCA ATCG");
    MP_GET_PROBES_EQUAL(probes_2,       "ACGT TGCA ATCG");

    MP_GET_COMMENT_EQUAL(1, probes_comment, "val1");
    MP_GET_COMMENT_EQUAL(2, probes_comment, "val2");
    MP_GET_COMMENT_EQUAL(3, probes_comment, "val3");

    MP_GET_COMMENT_EQUAL(1, probes_only, (const char *)NULL);
    MP_GET_COMMENT_EQUAL(2, probes_only, (const char *)NULL);
    MP_GET_COMMENT_EQUAL(3, probes_only, (const char *)NULL);

    MP_GET_COMMENT_EQUAL(1, probes_1, "one");
    MP_GET_COMMENT_EQUAL(2, probes_1, (const char *)NULL);
    MP_GET_COMMENT_EQUAL(3, probes_1, (const char *)NULL);

    MP_GET_COMMENT_EQUAL(1, probes_2, "one");
    MP_GET_COMMENT_EQUAL(2, probes_2, "two");
    MP_GET_COMMENT_EQUAL(3, probes_2, (const char *)NULL);

    free(probes_2);
    free(probes_1);
    free(probes_comment);
    free(probes_only);
}

void TEST_compute_multiprobes() {
    // test data is based on ../UNIT_TESTER/run/TEST_pt_src.arb

    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");

    const char *marked_species_list   = "DcdNodos,PsAAAA00,AclPleur,ClfPerfr,VblVulni,CltBotul,VbrFurni,ClnCorin,VbhChole,CPPParap,PtVVVulg,FrhhPhil";
    const char *unmarked_species_list = "HllHalod,PslFlave,Bl0LLL00,DlcTolu2,PbcAcet2,LgtLytic,DsssDesu,PbrPropi,BcSSSS00,Stsssola";

    const char *inputProbes[] = {
        "3#0#   11#GUUUGAUCAAGUCGAGCG",
        "3#0#  113#CUGGGGUGAAGUCGUAAC",
        "3#0#  111#GACUGGGGUGAAGUCGUA",
        "3#0#  108#CAUGACUGGGGUGAAGUC",
        "3#0#  109#AUGACUGGGGUGAAGUCG",
        "3#0#  110#UGACUGGGGUGAAGUCGU",
        "3#0#  112#ACUGGGGUGAAGUCGUAA",
    };

    // memset(&mp_gl_awars, 0, sizeof(mp_gl_awars));
    // see MP_main.cxx@MP_AWAR_SEQIN
    mp_gl_awars.weightedmismatches            = 2;
    mp_gl_awars.complement                    = 0; // 1;
    mp_gl_awars.no_of_mismatches              = 0;
    mp_gl_awars.no_of_probes                  = 3;
    mp_gl_awars.outside_mismatches_difference = 1.0;
    mp_gl_awars.qualityborder_best            = 5;
    mp_gl_awars.emphasis                      = 0;
    mp_gl_awars.greyzone                      = 0.0;
    mp_gl_awars.ecolipos                      = 0;

    mp_gl_awars.ptserver = TEST_SERVER_ID;

    ConstStrArray input_probes;
    input_probes.put_array_elems(inputProbes, ARRAY_ELEMS(inputProbes));

    mp_global = new MP_Global;

    {
        ConstStrArray marked_species;
        GBT_split_string(marked_species, marked_species_list, ",", false);

        GB_HASH *marked_species_hash = GBS_create_hash(marked_species.size(), GB_IGNORE_CASE);
        for (size_t i = 0; i<marked_species.size(); ++i) GBS_write_hash(marked_species_hash, marked_species[i], 1);
        mp_global->set_marked_species(marked_species_hash);
    }
    {
        ConstStrArray unmarked_species;
        GBT_split_string(unmarked_species, unmarked_species_list, ",", false);

        GB_HASH *unmarked_species_hash = GBS_create_hash(unmarked_species.size(), GB_IGNORE_CASE);
        for (size_t i = 0; i<unmarked_species.size(); ++i) GBS_write_hash(unmarked_species_hash, unmarked_species[i], 1);
        mp_global->set_unmarked_species(unmarked_species_hash);
    }

    arb_progress progress("test multiprobes");

    StrArray results;
    TEST_ASSERT_NO_ERROR(calc_multiprobes(input_probes, results));

    {
        char       *result_string = GBT_join_names(results, ',');
        const char *expected      =
            "20.200000           #0 0 0#    11    111    108#GUUUGAUCAAGUCGAGCG GACUGGGGUGAAGUCGUA CAUGACUGGGGUGAAGUC,"
            "19.800000           #0 0 0#    11    108    110#GUUUGAUCAAGUCGAGCG CAUGACUGGGGUGAAGUC UGACUGGGGUGAAGUCGU,"
            "19.500000           #0 0 0#    11    113    108#GUUUGAUCAAGUCGAGCG CUGGGGUGAAGUCGUAAC CAUGACUGGGGUGAAGUC,"
            "19.500000           #0 0 0#    11    108    112#GUUUGAUCAAGUCGAGCG CAUGACUGGGGUGAAGUC ACUGGGGUGAAGUCGUAA,"
            "19.000000           #0 0 0#    11    108    109#GUUUGAUCAAGUCGAGCG CAUGACUGGGGUGAAGUC AUGACUGGGGUGAAGUCG,"
            "17.100000           #0 0 0#    11    111    110#GUUUGAUCAAGUCGAGCG GACUGGGGUGAAGUCGUA UGACUGGGGUGAAGUCGU,"
            "16.900000           #0 0 0#    11    111    109#GUUUGAUCAAGUCGAGCG GACUGGGGUGAAGUCGUA AUGACUGGGGUGAAGUCG,"
            "16.800000           #0 0 0#    11    113    110#GUUUGAUCAAGUCGAGCG CUGGGGUGAAGUCGUAAC UGACUGGGGUGAAGUCGU,"
            "16.800000           #0 0 0#    11    110    112#GUUUGAUCAAGUCGAGCG UGACUGGGGUGAAGUCGU ACUGGGGUGAAGUCGUAA,"
            "16.600000           #0 0 0#    11    113    109#GUUUGAUCAAGUCGAGCG CUGGGGUGAAGUCGUAAC AUGACUGGGGUGAAGUCG,"
            "16.600000           #0 0 0#    11    109    112#GUUUGAUCAAGUCGAGCG AUGACUGGGGUGAAGUCG ACUGGGGUGAAGUCGUAA,"
            "16.500000           #0 0 0#    11    109    110#GUUUGAUCAAGUCGAGCG AUGACUGGGGUGAAGUCG UGACUGGGGUGAAGUCGU,"
            "15.800000           #0 0 0#    11    113    111#GUUUGAUCAAGUCGAGCG CUGGGGUGAAGUCGUAAC GACUGGGGUGAAGUCGUA,"
            "15.800000           #0 0 0#    11    111    112#GUUUGAUCAAGUCGAGCG GACUGGGGUGAAGUCGUA ACUGGGGUGAAGUCGUAA,"
            "14.000000           #0 0 0#    11    113    112#GUUUGAUCAAGUCGAGCG CUGGGGUGAAGUCGUAAC ACUGGGGUGAAGUCGUAA,"
            "12.700000           #0 0 0#   111    108    110#GACUGGGGUGAAGUCGUA CAUGACUGGGGUGAAGUC UGACUGGGGUGAAGUCGU,"
            "12.500000           #0 0 0#   113    108    110#CUGGGGUGAAGUCGUAAC CAUGACUGGGGUGAAGUC UGACUGGGGUGAAGUCGU,"
            "12.500000           #0 0 0#   108    110    112#CAUGACUGGGGUGAAGUC UGACUGGGGUGAAGUCGU ACUGGGGUGAAGUCGUAA,"
            "12.400000           #0 0 0#   111    108    109#GACUGGGGUGAAGUCGUA CAUGACUGGGGUGAAGUC AUGACUGGGGUGAAGUCG,"
            "12.200000           #0 0 0#   113    111    108#CUGGGGUGAAGUCGUAAC GACUGGGGUGAAGUCGUA CAUGACUGGGGUGAAGUC,"
            "12.200000           #0 0 0#   113    108    109#CUGGGGUGAAGUCGUAAC CAUGACUGGGGUGAAGUC AUGACUGGGGUGAAGUCG,"
            "12.200000           #0 0 0#   111    108    112#GACUGGGGUGAAGUCGUA CAUGACUGGGGUGAAGUC ACUGGGGUGAAGUCGUAA,"
            "12.200000           #0 0 0#   108    109    112#CAUGACUGGGGUGAAGUC AUGACUGGGGUGAAGUCG ACUGGGGUGAAGUCGUAA,"
            "10.600000           #0 0 0#   113    108    112#CUGGGGUGAAGUCGUAAC CAUGACUGGGGUGAAGUC ACUGGGGUGAAGUCGUAA,"
            "10.300000           #0 0 0#   108    109    110#CAUGACUGGGGUGAAGUC AUGACUGGGGUGAAGUCG UGACUGGGGUGAAGUCGU,"
            "7.300000            #0 0 0#   113    111    110#CUGGGGUGAAGUCGUAAC GACUGGGGUGAAGUCGUA UGACUGGGGUGAAGUCGU,"
            "7.300000            #0 0 0#   113    109    110#CUGGGGUGAAGUCGUAAC AUGACUGGGGUGAAGUCG UGACUGGGGUGAAGUCGU,"
            "7.300000            #0 0 0#   113    110    112#CUGGGGUGAAGUCGUAAC UGACUGGGGUGAAGUCGU ACUGGGGUGAAGUCGUAA,"
            "7.300000            #0 0 0#   111    109    110#GACUGGGGUGAAGUCGUA AUGACUGGGGUGAAGUCG UGACUGGGGUGAAGUCGU,"
            "7.300000            #0 0 0#   111    110    112#GACUGGGGUGAAGUCGUA UGACUGGGGUGAAGUCGU ACUGGGGUGAAGUCGUAA,"
            "7.300000            #0 0 0#   109    110    112#AUGACUGGGGUGAAGUCG UGACUGGGGUGAAGUCGU ACUGGGGUGAAGUCGUAA,"
            "7.000000            #0 0 0#   113    111    109#CUGGGGUGAAGUCGUAAC GACUGGGGUGAAGUCGUA AUGACUGGGGUGAAGUCG,"
            "7.000000            #0 0 0#   113    109    112#CUGGGGUGAAGUCGUAAC AUGACUGGGGUGAAGUCG ACUGGGGUGAAGUCGUAA,"
            "7.000000            #0 0 0#   111    109    112#GACUGGGGUGAAGUCGUA AUGACUGGGGUGAAGUCG ACUGGGGUGAAGUCGUAA,"
            "6.400000            #0 0 0#   113    111    112#CUGGGGUGAAGUCGUAAC GACUGGGGUGAAGUCGUA ACUGGGGUGAAGUCGUAA";

        TEST_ASSERT_EQUAL(result_string, expected);

        free(result_string);
    }

    delete mp_global; mp_global = NULL;
}

#endif
