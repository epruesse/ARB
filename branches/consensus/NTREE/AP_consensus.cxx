// ================================================================= //
//                                                                   //
//   File      : AP_consensus.cxx                                    //
//   Purpose   : calculate consensus SAIs                            //
//                                                                   //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "NT_local.h"

#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_awar.hxx>

#include <arbdbt.h>

#include <arb_strbuf.h>
#include <arb_defs.h>
#include <arb_progress.h>

#include <awt_config_manager.hxx>
#include <awt_misc.hxx>
#include <awt_sel_boxes.hxx>


#define AWAR_MAX_FREQ_PREFIX      "tmp/CON_MAX_FREQ/"
#define AWAR_CONSENSUS_PREFIX     "con/"
#define AWAR_CONSENSUS_PREFIX_TMP "tmp/" AWAR_CONSENSUS_PREFIX

#define AWAR_MAX_FREQ_IGNORE_GAPS AWAR_MAX_FREQ_PREFIX "no_gaps"
#define AWAR_MAX_FREQ_SAI_NAME    AWAR_MAX_FREQ_PREFIX "sai_name"

#define AWAR_CONSENSUS_SPECIES      AWAR_CONSENSUS_PREFIX_TMP "which_species"
#define AWAR_CONSENSUS_ALIGNMENT    AWAR_CONSENSUS_PREFIX_TMP "alignment"
#define AWAR_CONSENSUS_COUNTGAPS    AWAR_CONSENSUS_PREFIX "countgaps"
#define AWAR_CONSENSUS_GAPBOUND     AWAR_CONSENSUS_PREFIX "gapbound"
#define AWAR_CONSENSUS_GROUP        AWAR_CONSENSUS_PREFIX "group"
#define AWAR_CONSENSUS_FCONSIDBOUND AWAR_CONSENSUS_PREFIX "fconsidbound"
#define AWAR_CONSENSUS_FUPPER       AWAR_CONSENSUS_PREFIX "fupper"
#define AWAR_CONSENSUS_LOWER        AWAR_CONSENSUS_PREFIX "lower"
#define AWAR_CONSENSUS_NAME         AWAR_CONSENSUS_PREFIX_TMP "name"

#define CONSENSUS_AWAR_SOURCE CAS_NTREE
#include <consensus.h>
#include <consensus_config.h>
#include <chartable.h>

static int CON_insertSequences(GBDATA *gb_main, const char *aliname, long maxalignlen, bool onlymarked, BaseFrequencies& freqs) {
    /*! read sequence data and fill into 'freqs'
     * @param gb_main       database
     * @param aliname       name of alignment
     * @param maxalignlen   length of alignment
     * @param onlymarked    true -> marked only
     * @param freqs         sequences are inserted here (has to be empty)
     * @return number of inserted sequences
     */
    int nrofspecies = onlymarked ? GBT_count_marked_species(gb_main) : GBT_get_species_count(gb_main);

    arb_progress progress(nrofspecies);
    progress.auto_subtitles("Examining sequence");

    GBDATA *gb_species = onlymarked ? GBT_first_marked_species(gb_main) : GBT_first_species(gb_main);
    while (gb_species) {
        GBDATA *alidata = GBT_find_sequence(gb_species, aliname);
        if (alidata) {
            const char *data   = GB_read_char_pntr(alidata);
            size_t      length = GB_read_string_count(alidata);

            nt_assert(long(length)<=maxalignlen);
            freqs.add(data, length);
        }
        gb_species = onlymarked ? GBT_next_marked_species(gb_species) : GBT_next_species(gb_species);
        ++progress;
    }

    int inserted = freqs.added_sequences();
    if (nrofspecies < inserted) {
        GBT_message(gb_main, GBS_global_string("Only %i of %i %sspecies contain data in alignment '%s'",
                                               inserted, nrofspecies, onlymarked ? "marked " : "", aliname));
        progress.done();
    }

    return inserted;
}

static GB_ERROR CON_export(GBDATA *gb_main, const char *savename, const char *align, const char *result, bool onlymarked, long nrofspecies, const ConsensusBuildParams& BK) {
    /*! writes consensus SAI to DB
     * @param gb_main      database
     * @param savename     name of SAI to save to
     * @param align        alignment name
     * @param result       SAI data to write
     * @param onlymarked   true -> was calculated on marked only (used for SAI comment)
     * @param nrofspecies  number of used sequences (used for SAI comment; if less than 20 -> add an explicit list to field '_SPECIES')
     * @param BK           parameters used for consensus calculation (used for SAI comment)
     * @return error if something goes wrong
     */
    const char *off = "off";
    const char *on  = "on";

    char *buffer = (char *)GB_calloc(2000, sizeof(char));

    GBDATA   *gb_extended = GBT_find_or_create_SAI(gb_main, savename);
    GBDATA   *gb_data     = GBT_add_data(gb_extended, align, "data", GB_STRING);
    GB_ERROR  err         = GB_write_string(gb_data, result);           // @@@ result is ignored
    GBDATA   *gb_options  = GBT_add_data(gb_extended, align, "_TYPE", GB_STRING);

    const char *allvsmarked     = onlymarked ? "marked" : "all";
    const char *countgapsstring = BK.countgaps ? on : off;
    const char *simplifystring  = BK.group ? on : off;

    sprintf(buffer, "CON: [species: %s]  [number: %ld]  [count gaps: %s] "
            "[threshold for gaps: %d]  [simplify: %s] "
            "[threshold for group: %d]  [upper: %d]  [lower: %d]",
            allvsmarked, nrofspecies, countgapsstring,
            BK.gapbound, simplifystring,
            BK.considbound, BK.upper, BK.lower);

    err = GB_write_string(gb_options, buffer);

    GBDATA *gb_names = GB_search(GB_get_father(gb_options), "_SPECIES", GB_FIND);
    if (gb_names) GB_delete(gb_names); // delete old entry

    if (nrofspecies<20) {
        GBDATA        *gb_species;
        GBS_strstruct *strstruct = GBS_stropen(1000);

        if (onlymarked) gb_species = GBT_first_marked_species(gb_main);
        else gb_species            = GBT_first_species(gb_main);

        while (gb_species) {
            if (GBT_find_sequence(gb_species, align)) {
                GBDATA     *gb_speciesname = GB_search(gb_species, "name", GB_FIND);
                const char *name           = GB_read_char_pntr(gb_speciesname);

                GBS_strcat(strstruct, name);
                GBS_chrcat(strstruct, ' ');
            }
            if (onlymarked) gb_species = GBT_next_marked_species(gb_species);
            else gb_species            = GBT_next_species(gb_species);
        }

        char *allnames = GBS_strclose(strstruct);
        err            = GBT_write_string(GB_get_father(gb_options), "_SPECIES", allnames);
        free(allnames);
    }

    // remove data relicts from "complex consensus" (no longer supported)
    {
        char buffer2[256];
        sprintf(buffer2, "%s/FREQUENCIES", align);
        GBDATA *gb_graph = GB_search(gb_extended, buffer2, GB_FIND);
        if (gb_graph) GB_delete(gb_graph);  // delete old entry
    }

    free(buffer);
    return err;
}

static GB_ERROR CON_calculate(GBDATA *gb_main, const ConsensusBuildParams& BK, const char *aliname, bool onlymarked, const char *sainame) {
    /*! calculates the consensus and writes it to SAI 'sainame'
     * Description how consensus is calculated: ../HELP_SOURCE/oldhelp/consensus_def.hlp // @@@ update after #663 differences are resolved
     * @param gb_main     database
     * @param BK          parameters for consensus calculation
     * @param aliname     alignment name
     * @param onlymarked  true -> use marked sequences only
     * @param sainame     name of destination SAI
     * @return error if something goes wrong
     */
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);

    long maxalignlen = GBT_get_alignment_len(gb_main, aliname);
    if (maxalignlen <= 0) error = GB_export_errorf("alignment '%s' doesn't exist", aliname);

    if (!error) {
        arb_progress progress("Calculating consensus");

        GB_alignment_type alitype = GBT_get_alignment_type(gb_main, aliname);
        BaseFrequencies::setup("-.", alitype);

        BaseFrequencies freqs(maxalignlen);
        int nrofspecies = CON_insertSequences(gb_main, aliname, maxalignlen, onlymarked, freqs);

        if (BK.lower>BK.upper) {
            error = "fault: lower greater than upper";
        }
        else {
            char *result = freqs.build_consensus_string(BK);
            error = CON_export(gb_main, sainame, aliname, result, onlymarked, nrofspecies, BK);
            free(result);
        }
    }

    error = GB_end_transaction(gb_main, error);

    return error;
}

static void CON_calculate_cb(AW_window *aw) {
    AW_root *awr        = aw->get_root();
    char    *aliname    = awr->awar(AWAR_CONSENSUS_ALIGNMENT)->read_string();
    char    *sainame    = awr->awar(AWAR_CONSENSUS_NAME)->read_string();
    bool     onlymarked = strcmp(awr->awar(AWAR_CONSENSUS_SPECIES)->read_char_pntr(), "marked") == 0;

    ConsensusBuildParams BK(awr);

    {
#if defined(ASSERTION_USED)
        GB_transaction ta(GLOBAL.gb_main);
        LocallyModify<bool> denyAwarReads(AW_awar::deny_read, true);
        LocallyModify<bool> denyAwarWrites(AW_awar::deny_write, true);
#endif

        GB_ERROR error = CON_calculate(GLOBAL.gb_main, BK, aliname, onlymarked, sainame);
        aw_message_if(error);
    }

    free(sainame);
    free(aliname);
}

void AP_create_consensus_var(AW_root *aw_root, AW_default aw_def) {
    GB_transaction ta(GLOBAL.gb_main);
    {
        char *defali = GBT_get_default_alignment(GLOBAL.gb_main);
        aw_root->awar_string(AWAR_CONSENSUS_ALIGNMENT, defali, aw_def);
        free(defali);
    }
    aw_root->awar_string(AWAR_CONSENSUS_SPECIES,      "marked",      aw_def);
    aw_root->awar_string(AWAR_CONSENSUS_GROUP,        "off",         aw_def);
    aw_root->awar_string(AWAR_CONSENSUS_COUNTGAPS,    "on",          aw_def);
    aw_root->awar_float (AWAR_CONSENSUS_FUPPER,       95,            aw_def);
    aw_root->awar_int   (AWAR_CONSENSUS_LOWER,        70,            aw_def);
    aw_root->awar_int   (AWAR_CONSENSUS_GAPBOUND,     60,            aw_def);
    aw_root->awar_float (AWAR_CONSENSUS_FCONSIDBOUND, 30,            aw_def);
    aw_root->awar_string(AWAR_CONSENSUS_NAME,         "CONSENSUS",   aw_def);

    aw_root->awar_string(AWAR_MAX_FREQ_SAI_NAME,    "MAX_FREQUENCY", aw_def);
    aw_root->awar_int   (AWAR_MAX_FREQ_IGNORE_GAPS, 1,               aw_def);
}

static AWT_config_mapping_def consensus_config_mapping[] = {
    { AWAR_CONSENSUS_COUNTGAPS,    CONSENSUS_CONFIG_COUNTGAPS },
    { AWAR_CONSENSUS_GAPBOUND,     CONSENSUS_CONFIG_GAPBOUND },
    { AWAR_CONSENSUS_GROUP,        CONSENSUS_CONFIG_GROUP },
    { AWAR_CONSENSUS_FCONSIDBOUND, CONSENSUS_CONFIG_CONSIDBOUND },
    { AWAR_CONSENSUS_FUPPER,       CONSENSUS_CONFIG_UPPER },
    { AWAR_CONSENSUS_LOWER,        CONSENSUS_CONFIG_LOWER },

    // make sure the keywords of the following entries
    // DIFFER from those defined at ../TEMPLATES/consensus_config.h@CommonEntries

    { AWAR_CONSENSUS_SPECIES, "species" },
    { AWAR_CONSENSUS_NAME,    "name" },

    { 0, 0 }
};

AW_window *AP_create_con_expert_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "BUILD_CONSENSUS", "CONSENSUS OF SEQUENCES");
    aws->load_xfig("consensus/expert.fig");
    aws->button_length(6);

    aws->at("cancel");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("consensus.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(10);
    aws->at("showgroups");
    aws->callback(AWT_create_IUPAC_info_window);
    aws->create_button("SHOW_IUPAC", "show\nIUPAC...", "s");

    aws->at("which_species");
    aws->create_toggle_field(AWAR_CONSENSUS_SPECIES, NULL, "");
    aws->insert_toggle("all", "1", "all");
    aws->insert_default_toggle("marked",   "1", "marked");
    aws->update_toggle_field();

    aws->at("which_alignment");
    awt_create_ALI_selection_list(GLOBAL.gb_main, (AW_window *)aws, AWAR_CONSENSUS_ALIGNMENT, "*=");

    aws->button_length(15);

    // activation of consensus calculation by button ...
    aws->at("calculate"); aws->callback((AW_CB0)CON_calculate_cb);
    aws->create_button("GO", "GO", "G");

    aws->at("group");
    aws->create_toggle_field(AWAR_CONSENSUS_GROUP, NULL, "");
    aws->insert_toggle("on", "1", "on");
    aws->insert_default_toggle("off", "1", "off");
    aws->update_toggle_field();

    aws->at("countgaps");
    aws->create_toggle_field(AWAR_CONSENSUS_COUNTGAPS, NULL, "");
    aws->insert_toggle("on", "1", "on");
    aws->insert_default_toggle("off", "1", "off");
    aws->update_toggle_field();

    aws->at("upper");
    aws->create_input_field(AWAR_CONSENSUS_FUPPER, 4);

    aws->at("lower");
    aws->create_input_field(AWAR_CONSENSUS_LOWER, 4);

    aws->at("considbound");
    aws->create_input_field(AWAR_CONSENSUS_FCONSIDBOUND, 4);

    aws->at("gapbound");
    aws->create_input_field(AWAR_CONSENSUS_GAPBOUND, 4);

    aws->at("name");
    aws->create_input_field(AWAR_CONSENSUS_NAME, 10);

    aws->at("save_box");
    awt_create_SAI_selection_list(GLOBAL.gb_main, aws, AWAR_CONSENSUS_NAME, false);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, CONSENSUS_CONFIG_ID, consensus_config_mapping);

    return aws;
}

static GB_ERROR CON_calc_max_freq(GBDATA *gb_main, bool ignore_gaps, const char *savename, const char *aliname) {
    /*! calculates the maximum frequency for each column and write to SAI
     * @param gb_main      database
     * @param ignore_gaps  true -> ignore gaps; see ../HELP_SOURCE/oldhelp/max_freq.hlp@Gaps
     * @param savename     name of destination SAI
     * @param aliname      name of alignment to use
     * @return error if something goes wrong
     */
    arb_assert(!GB_have_error());

    GB_ERROR       error = NULL;
    GB_transaction ta(gb_main);

    long maxalignlen = GBT_get_alignment_len(gb_main, aliname);
    if (maxalignlen<=0) {
        GB_clear_error();
        error = "alignment doesn't exist!";
    }
    else {
        arb_progress progress("Calculating max. frequency");

        GB_alignment_type alitype = GBT_get_alignment_type(gb_main, aliname);
        BaseFrequencies::setup("-.", alitype);

        const int onlymarked  = 1;
        BaseFrequencies freqs(maxalignlen);
        long nrofspecies = CON_insertSequences(gb_main, aliname, maxalignlen, onlymarked, freqs);

        char *result1 = new char[maxalignlen+1];
        char *result2 = new char[maxalignlen+1];

        result1[maxalignlen] = 0;
        result2[maxalignlen] = 0;

        for (int pos = 0; pos < maxalignlen; pos++) {
            double mf = freqs.max_frequency_at(pos, ignore_gaps);
            if (mf) {
                if (mf<0.1) mf = 0.1; // hack: otherwise SAI will contain '0' (meaning 100% frequency)

                result1[pos] = "?1234567890"[int( 10*mf)%11]; // %11 maps 100% -> '0'
                result2[pos] = "0123456789" [int(100*mf)%10];
            }
            else {
                result1[pos] = '=';
                result2[pos] = '=';
            }
        }

        GBDATA *gb_extended = GBT_find_or_create_SAI(gb_main, savename);
        if (!gb_extended) {
            error = GB_await_error();
        }
        else {
            GBDATA *gb_data1 = GBT_add_data(gb_extended, aliname, "data", GB_STRING);
            GBDATA *gb_data2 = GBT_add_data(gb_extended, aliname, "dat2", GB_STRING);

            error             = GB_write_string(gb_data1, result1);
            if (!error) error = GB_write_string(gb_data2, result2);

            GBDATA *gb_options = GBT_add_data(gb_extended, aliname, "_TYPE", GB_STRING);

            if (!error) {
                const char *type = GBS_global_string("MFQ: [species: %li] [ignore gaps: %s]", nrofspecies, ignore_gaps ? "yes" : "no");
                error            = GB_write_string(gb_options, type);
            }
        }

        delete [] result1;
        delete [] result2;
    }

    error = ta.close(error);
    arb_assert(!GB_have_error());

    return error;
}

static void CON_calc_max_freq_cb(AW_window *aw) {
    AW_root    *awr         = aw->get_root();
    bool        ignore_gaps = awr->awar(AWAR_MAX_FREQ_IGNORE_GAPS)->read_int();
    const char *savename    = awr->awar(AWAR_MAX_FREQ_SAI_NAME)->read_char_pntr();
    char       *aliname     = GBT_get_default_alignment(GLOBAL.gb_main);

    GB_ERROR error = CON_calc_max_freq(GLOBAL.gb_main, ignore_gaps, savename, aliname);
    if (error) aw_message(error);

    free(aliname);
}

AW_window *AP_create_max_freq_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "MAX_FREQUENCY", "MAX FREQUENCY");
    aws->load_xfig("consensus/max_freq.fig");

    GB_push_transaction(GLOBAL.gb_main);

    aws->button_length(6);

    aws->at("cancel"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("max_freq.hlp"));
    aws->create_button("HELP", "HELP", "H");

    // activation of consensus calculation by button ...
    aws->at("go"); aws->callback((AW_CB0)CON_calc_max_freq_cb);
    aws->create_button("GO", "GO", "C");

    aws->at("save");
    aws->create_input_field(AWAR_MAX_FREQ_SAI_NAME, 1);

    aws->at("sai");
    awt_create_SAI_selection_list(GLOBAL.gb_main, aws, AWAR_MAX_FREQ_SAI_NAME, false);

    aws->at("gaps");
    aws->create_toggle(AWAR_MAX_FREQ_IGNORE_GAPS);

    GB_pop_transaction(GLOBAL.gb_main);

    return aws;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static GBDATA *create_simple_seq_db(const char *aliname, const char *alitype, const char **sequence, int sequenceCount, int sequenceLength) {
    GBDATA *gb_main = GB_open("nosuch.arb", "wc");

    {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_species_data = GBT_get_species_data(gb_main);
        int             specCounter     = 0;

        TEST_EXPECT_RESULT__NOERROREXPORTED(GBT_create_alignment(gb_main, aliname, sequenceLength, true, 6, alitype));

        for (int s = 0; s<sequenceCount; ++s) {
            GBDATA *gb_species = GBT_find_or_create_species_rel_species_data(gb_species_data, GBS_global_string("name%04i", ++specCounter));
            GBDATA *gb_data    = GBT_add_data(gb_species, aliname, "data", GB_STRING);

            TEST_EXPECT_EQUAL(strlen(sequence[s]), sequenceLength);
            TEST_EXPECT_NO_ERROR(GB_write_string(gb_data, sequence[s]));
        }
    }

#if 0
    // save DB (to view data; should be inactive when committed)
    char *dbname = GBS_global_string_copy("cons_%s.arb", alitype);
    TEST_EXPECT_NO_ERROR(GB_save(gb_main, dbname, "a"));
    free(dbname);
#endif

    return gb_main;
}

static void read_frequency(GBDATA *gb_main, const char *sainame, const char *aliname, const char*& data, const char*& dat2) {
    GB_transaction ta(gb_main);

    GBDATA *gb_maxFreq = GBT_find_SAI(gb_main, sainame);
    GBDATA *gb_ali     = GB_entry(gb_maxFreq, aliname);
    GBDATA *gb_data    = GB_entry(gb_ali, "data");
    GBDATA *gb_dat2    = GB_entry(gb_ali, "dat2");

    data = GB_read_char_pntr(gb_data);
    dat2 = GB_read_char_pntr(gb_dat2);
}

void TEST_nucleotide_consensus_and_maxFrequency() {
    const char *sequence[] = {
        "-.AAAAAAAAAAcAAAAAAAAATTTTTTTTTTTTTTTTTAAAAAAAAgggggAAAAgAA----m-----yykm-mmmAAAAAAAAAmmmmmmmmmNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNKKKKKKKKKWWWWWWWWW",
        "-.-AAAAAAAAAccAAAAAAAAggTTgTTTTgTTTTTTTcccAAAAAgggggAAAAgAA----k-----kykr-rrrAAAAAAAAmmmmmmmmmT-NNNNNNNNNANNNNNbNNNNNNNNkNNNNNNNNaNNNNNNNNbKKKKKKKKbWWWWWWWW",
        "-.--AAAAAAAAcccAAAAAAA-ggTggTTTggTTTTTTccccAAAAgggCCtAAAgAC----m-----sykw-wvsAAAAAAAmmmmmmmmmTT--NNNNNNNNCANNNNbbNNNNNNNkkNNNNNNNaaNNNNNNNbbKKKKKKKbbWWWWWWW",
        "-.---AAAAAAAccccAAAAAA-ggggggTTgggTTTTTcccccAAAggCCC-tAACtC----k----yyyys-smvAAAAAAmmmmmmmmmTTT---NNNNNNNGCANNNbbbNNNNNNkkkNNNNNNaaaNNNNNNbbbKKKKKKbbbWWWWWW",
        "-.----AAAAAAcccccAAAAA----ggggTggggTTTTGGGcccAAgCCCt-ttACtG----m---nkkkky-yrmAAAAAmmmmmmmmmTTTT----NNNNNNTGCANNbbbbNNNNNkkkkNNNNNaaaaNNNNNbbbbKKKKKbbbbWWWWW",
        "-.-----AAAAAccccccAAAA----ggggggggggTTgGGGGcccAcCCtt--tttCG----k--nnssssk-kvrAAAAmmmmmmmmmTTTTT-----NNNNN-TGCANbbbbbNNNNkkkkkNNNNaaaaaNNNNbbbbbKKKKbbbbbWWWW",
        "-.------AAAAcccccccAAA---------ggggggTgGGGGGccccCt----tt-gT----mydddyyyy-vvmsAAAmmmmmmmmmTTTTTT------NNNN-ATGCAbbbbbbNNNkkkkkkNNNaaaaaaNNNbbbbbbKKKbbbbbbWWW",
        "-.-------AAAccccccccAA---------ggggggggttGGGGccct------t--T-yykkkbbbkkkk-hhrvAAmmmmmmmmmTTTTTTT-------NNN-C-TGCbbbbbbbNNkkkkkkkNNaaaaaaaNNbbbbbbbKKbbbbbbbWW",
        "-.--------AAcccccccccA----------------gttGGGGGct-------t----ymmmmnnnssss-ddvmAmmmmmmmmmTTTTTTTT--------NN-G--TGbbbbbbbbNkkkkkkkkNaaaaaaaaNbbbbbbbbKbbbbbbbbW",
        "-.---------Acccccccccc----------------gtAGGGGGG----------------k---------bbmrmmmmmmmmmTTTTTTTTT---------N-T---Tbbbbbbbbbkkkkkkkkkaaaaaaaaabbbbbbbbbbbbbbbbbb",
    };
    const char *expected_frequency[] = {
        // considering gaps:
        "0=9876567890098765678986665444576545675336544565434475454320888277654333462439988776654434567899876543222523222333322222444433332987765443333444444333444444",
        "0=0000000000000000000000000000000000000000000000000000000000000500000055005565050505055050000000000000025050025210098765752075207257025702568013568568013568",
        // ignoring gaps:
        "==000000000009876567895757865688765678533654456554536655542=552233223333222439988776654434567892222222222222222333322222444433332987765443333444444333444444",
        "==000000000000000000000505360637520257000000000502036075025=005530983388555565050505055050000005555555555555555210098765752075207257025702568013568568013568",
    };
    const char *expected_consensus[] = {
        "==----..aaaACccMMMMMaa----.....g.kkk.uKb.ssVVmmss...-.ww...=---.---..byk.-.mVAaaaaMMMMmmHH..uuu----............BBbb.....Kkkkkk...aaaa.....BkkkkkkkKB....wwww", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "==AAAAAAAAAACccMMMMMaaKgKugKKKuggKKKuuKb.ssVVmmssssBWWWWs..=Y.......BByk...mVAaaaaMMMMmmHH..uuu................BBbb.....Kkkkkk...aaaa.....BkkkkkkkKB....wwww", // countgaps=0
        "==AAAAAAAAAACCCMMMMMAAKGKUGKKKUGGKKKUUKBsSSVVMMSSSSBWWWWSwa=YcaaykkkBBYKaaaMVAAAAAMMMMMMHHuuuUUaaaaaaaaaaaaaaaaBBBBBBBBcKKKKKkkkkAAAaaaaaaBBKKKKKKKBBuuuwWWW", // countgaps=0,              considbound=26, lower=0, upper=75 (as described in #663)
        "==AAAAAAAAAACCCMMMMMAAKKKKGKKKUGKKKKKUKBsSSVVMMSSSSBWWWWSwN=YHNNykkkBBYKNNNVVAAAAMMMMMMMHHHuuUUNNNNNNNNNNNNNNNNBBBBBBBBBKKKKKkkkkAAAaaaaaaBBKKKKKKKBBuuwwWWW", // countgaps=0,              considbound=25, lower=0, upper=75
        "==---aaaaAAACCCMMMMMAA-gkugkkkuggKKKuuKBsSSVVMMSsssb-wwWswa=---a--kkbBykaaaMVAAAAAMMMMMMHHuuuUU---aaaaaaaaaaaaaBBBBBBBBcKKKKKkkkkAAAaaaaaaBBKKKKKKKBBuuuwWWW", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "==---aaaaAAACCMMMMMMMA-kkkgkkkugKKKKKuKBNSVVVVMSsssb-wwWswN=---N--nnbBBBnnNVVAAAMMMMMMMHHHHHuUU---nnnnNNNnNnNNNBBBBBBBNNKKKKKkkNNAAAaaaaNNBBBBKKKKKBBBNwwWWW", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
    };
    const size_t seqlen         = strlen(sequence[0]);
    const int    sequenceCount  = ARRAY_ELEMS(sequence);
    const int    consensusCount = ARRAY_ELEMS(expected_consensus);

    // create DB
    GB_shell    shell;
    const char *aliname = "ali_nuc";
    GBDATA     *gb_main = create_simple_seq_db(aliname, "rna", sequence, sequenceCount, seqlen);

    ConsensusBuildParams BK;
    for (int c = 0; c<consensusCount; ++c) {
        TEST_ANNOTATE(GBS_global_string("c=%i", c));
        switch (c) {
            case 0: break;                                                     // use default settings
            case 1: BK.countgaps   = false; break;                             // dont count gaps
            case 2: BK.considbound = 26; BK.lower = 0; BK.upper = 75; break;   // settings from #663
            case 3: BK.considbound = 25; break;
            case 4: BK.considbound = 26; BK.countgaps = true; BK.gapbound = 70; break;
            case 5: BK.considbound = 20; break;
            default: arb_assert(0); break;                                     // missing
        }

        {
            GB_transaction  ta(gb_main);
            const char     *sainame = "CONSENSUS";
            TEST_EXPECT_NO_ERROR(CON_calculate(gb_main, BK, aliname, false, sainame));

            GBDATA     *gb_consensus = GBT_find_SAI(gb_main, sainame);
            GBDATA     *gb_seq       = GBT_find_sequence(gb_consensus, aliname);
            const char *consensus    = GB_read_char_pntr(gb_seq);

            TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        }
    }

    // test max.frequency
    const char *sainame = "MAXFREQ";
    for (int ignore_gaps = 0; ignore_gaps<=1; ++ignore_gaps) {
        TEST_ANNOTATE(GBS_global_string("ignore_gaps=%i", ignore_gaps));
        TEST_EXPECT_NO_ERROR(CON_calc_max_freq(gb_main, ignore_gaps, sainame, aliname));
        const char *data, *dat2;
        read_frequency(gb_main, sainame, aliname, data, dat2);
        TEST_EXPECT_EQUAL(data, expected_frequency[ignore_gaps*2]);
        TEST_EXPECT_EQUAL(dat2, expected_frequency[ignore_gaps*2+1]);
    }

    GB_close(gb_main);
}

void TEST_amino_consensus_and_maxFrequency() {
    const char *sequence[] = {
        "-.ppppppppppQQQQQQQQQDDDDDELLLLLwwwwwwwwwwwwwwwwgggggggggggSSSe-PPP-DELp",
        "-.-pppppppppkQQQQQQQQnDDDDELLLLLVVwwVwwwwVwwwwwwSgggggggggSSSee-QPP-DELa",
        "-.--ppppppppkkQQQQQQQnnDDDELLLLL-VVwVVwwwVVwwwwwSSgggggggSSSeee-KQP-DEIg",
        "-.---pppppppkkkQQQQQQnnnDDELLLLL-VVVVVVwwVVVwwwwSSSgggggSSSeee--LQQ-DQIs",
        "-.----ppppppkkkkQQQQQnnnnDELLLLL----VVVVwVVVVwwweSSSgggSSSeee---WKQ-NQJt",
        "-.-----pppppkkkkkQQQQnnnnnqiLLLL----VVVVVVVVVVwweeSSSggSSeee-----KQ-NQJq",
        "-.------ppppkkkkkkQQQnnnnnqiiLLL---------VVVVVVweeeSSSgSeee------LK-NZJn",
        "-.-------pppkkkkkkkQQnnnnnqiiiLL---------VVVVVVVeeeeSSSeee-------LK-NZJe",
        "-.--------ppkkkkkkkkQnnnnnqiiiiL----------------eeeeeSSee--------WK-BZJd",
        "-.---------pkkkkkkkkknnnnnqiiiii----------------eeeeeeSe---------WK-BZJb",
        "-.ppppppppppQQQQQQQQQDDDDDELLLLLwwwwwwwwwwwwwwwwgggggggggggSSSe-PPP-DELz",
        "-.-pppppppppkQQQQQQQQnDDDDELLLLLVVwwVwwwwVwwwwwwSgggggggggSSSee-QPP-DELh",
        "-.--ppppppppkkQQQQQQQnnDDDELLLLL-VVwVVwwwVVwwwwwSSgggggggSSSeee-KQP-DEIk",
        "-.---pppppppkkkQQQQQQnnnDDELLLLL-VVVVVVwwVVVwwwwSSSgggggSSSeee--LQQ-DQIr",
        "-.----ppppppkkkkQQQQQnnnnDELLLLL----VVVVwVVVVwwweSSSgggSSSeee---WKQ-NQJl",
        "-.-----pppppkkkkkQQQQnnnnnqiLLLL----VVVVVVVVVVwweeSSSggSSeee-----KQ-NQJi",
        "-.------ppppkkkkkkQQQnnnnnqiiLLL---------VVVVVVweeeSSSgSeee------LK-NZJv",
        "-.-------pppkkkkkkkQQnnnnnqiiiLL---------VVVVVVVeeeeSSSeee-------LK-NZJm",
        "-.--------ppkkkkkkkkQnnnnnqiiiiL----------------eeeeeSSee--------WK-BZJf",
        "-.---------pkkkkkkkkknnnnnqiiiii----------------eeeeeeSe---------WK-BZJy",
    };
    const char *expected_frequency[] = {
        // considering gaps:
        "0=9876567890987656789987655567898666544457654567654456743334567052404451",
        "0=0000000000000000000000000000000000000000000000000000000000000000000000",
        // ignoring gaps:
        "==0000000000987656789987655567895757865688765678654456743345670=224=4451",
        "==0000000000000000000000000000000505360637520257000000003720050=000=0000",
    };
    const char *expected_consensus[] = {
        "==----..aaaAhhh...dddDDDDDDIIIII----.....i.....f...aaaAa.....--=.X.=DD-.", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "==AAAAAAAAAAhhh...dddDDDDDDIIIII.i.fi...fii...ff...aaaAa.....dD=XX.=DDI.", // countgaps=0
        "==AAAAAAAAAAHHhhdddDDDDDDDDIIIIIiIiFIiifFIIiifFFdaaaAAAaaaaadDD=XXh=DDId", // countgaps=0,              considbound=26, lower=0, upper=75
        "==---aaaaAAAHHhhdddDDDDDDDDIIIII-iifiiiffiiiifffdaaaAAAaaaaadd-=xXh=DDid", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "==---aaaaAAAHHhhdddDDDDDDDDIIIII-iifiiiffiiiifffdaaaAAAaaaaadd-=aah=DDid", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
        "==---aaaaAAAHHhhXddDDDDDDDDIIIII-ixfiixffiiiXfffdXaaAAAaaaaxdd-=xXX=DDiX", // countgaps=1, gapbound=70, considbound=51, lower=0, upper=75
    };
    const size_t seqlen         = strlen(sequence[0]);
    const int    sequenceCount  = ARRAY_ELEMS(sequence);
    const int    consensusCount = ARRAY_ELEMS(expected_consensus);

    // create DB
    GB_shell    shell;
    const char *aliname = "ali_ami";
    GBDATA     *gb_main = create_simple_seq_db(aliname, "ami", sequence, sequenceCount, seqlen);

    ConsensusBuildParams BK;
    for (int c = 0; c<consensusCount; ++c) {
        TEST_ANNOTATE(GBS_global_string("c=%i", c));
        switch (c) {
            case 0: break;                                                     // use default settings
            case 1: BK.countgaps   = false; break;                             // dont count gaps
            case 2: BK.considbound = 26; BK.lower = 0; BK.upper = 75; break;   // settings from #663
            case 3: BK.countgaps   = true; BK.gapbound = 70; break;
            case 4: BK.considbound = 20; break;
            case 5: BK.considbound = 51; break;
            default: arb_assert(0); break;                                     // missing
        }

        {
            GB_transaction  ta(gb_main);
            const char     *sainame = "CONSENSUS";
            TEST_EXPECT_NO_ERROR(CON_calculate(gb_main, BK, aliname, false, sainame));

            GBDATA     *gb_consensus = GBT_find_SAI(gb_main, sainame);
            GBDATA     *gb_seq       = GBT_find_sequence(gb_consensus, aliname);
            const char *consensus    = GB_read_char_pntr(gb_seq);

            TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        }
    }

    // test max.frequency
    const char *sainame = "MAXFREQ";
    for (int ignore_gaps = 0; ignore_gaps<=1; ++ignore_gaps) {
        TEST_ANNOTATE(GBS_global_string("ignore_gaps=%i", ignore_gaps));
        TEST_EXPECT_NO_ERROR(CON_calc_max_freq(gb_main, ignore_gaps, sainame, aliname));
        const char *data, *dat2;
        read_frequency(gb_main, sainame, aliname, data, dat2);
        TEST_EXPECT_EQUAL(data, expected_frequency[ignore_gaps*2]);
        TEST_EXPECT_EQUAL(dat2, expected_frequency[ignore_gaps*2+1]);
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

