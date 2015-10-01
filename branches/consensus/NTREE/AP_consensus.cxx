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

enum {
    BAS_GAP,
    BAS_A,  BAS_C,  BAS_G,  BAS_T, BAS_N,

    MAX_BASES,
    MAX_AMINOS  = 27,
    MAX_GROUPS  = 40
};

static char *CON_evaluatestatistic(const int *const*statistic, const char *const*groupflags, const char *groupnames, int alignlength, int numgroups, const ConsensusBuildParams& BK) {
    /*! calculates consensus from 'statistic'
     * @@@ doc params
     */
    arb_progress progress("calculating result", alignlength);

    char *result = (char *)GB_calloc(alignlength+1, 1);

    for (int column=0; column<alignlength; column++) {
        long numentries = 0;
        for (int row = 0; statistic[row]; ++row) numentries += statistic[row][column];
        if (numentries==0) {
            result[column] = '.';
        }
        else {
            int gaps = statistic[0][column];
            arb_assert(numentries>=gaps);

            if (numentries == gaps) {
                result[column] = '='; // 100% gaps
            }
            else {
                if (!BK.countgaps) {
                    numentries -= gaps;
                    gaps        = 0;
                }

                if ((gaps*100/numentries) > BK.gapbound) { // @@@ why not >=? (as below for other thresholds)
                    result[column] = '-';
                }
                else {
                    int group_freq[MAX_GROUPS]; // frequency of group
                    for (int j = 0; j<numgroups; ++j) {
                        group_freq[j] = 0;
                    }

                    for (int row = 1; statistic[row]; ++row) {
                        if (statistic[row][column]*100 >= BK.considbound*numentries) {
                            for (int j = numgroups-1; j>=0; --j) { // @@@ why reverse?
                                if (groupflags[j][row]) {
                                    group_freq[j] += statistic[row][column];
                                }
                            }
                        }
                    }

                    int maxFreq       = 0;
                    int maxFreq_group = 0;
                    for (int j = 0; j<numgroups; ++j) {
                        if (group_freq[j] > maxFreq) {
                            maxFreq       = group_freq[j];
                            maxFreq_group = j;
                        }
                    }

                    int percent = maxFreq*100/numentries;
                    if      (percent >= BK.upper) result[column] = groupnames[maxFreq_group];
                    else if (percent >= BK.lower) result[column] = tolower(groupnames[maxFreq_group]);
                    else                          result[column] = '.';
                }
            }
        }
        ++progress;
    }

    return result;
}



static int CON_makegrouptable(char **const gf, char *const groupnames, bool isamino, bool groupallowed) {
    /*! initialize group tables
     * 'gf' will be allocated and initialized:
     *      'gf[GROUP][CTV] == 1' means: all 'c' with 'convtable[c] == CTV' are members of group 'GROUP'
     * 'groupnames' will be filled with groups "names" ( = single character codes)
     */
    for (int j=0; j<MAX_GROUPS; j++) gf[j]=(char *)GB_calloc(MAX_GROUPS, 1);

    if (!isamino) {
        strcpy(groupnames, "-ACGUMRWSYKVHDBN\0");

        for (int i=1; i<MAX_BASES; i++) gf[i][i]=1;

        if (groupallowed) {
            gf[5][BAS_A]=1; gf[5][BAS_C]=1;
            gf[6][BAS_A]=1; gf[6][BAS_G]=1;
            gf[7][BAS_A]=1; gf[7][BAS_T]=1;
            gf[8][BAS_C]=1; gf[8][BAS_G]=1;
            gf[9][BAS_C]=1; gf[9][BAS_T]=1;
            gf[10][BAS_G]=1; gf[10][BAS_T]=1;
            gf[11][BAS_A]=1; gf[11][BAS_C]=1; gf[11][BAS_G]=1;
            gf[12][BAS_A]=1; gf[12][BAS_C]=1; gf[12][BAS_T]=1;
            gf[13][BAS_A]=1; gf[13][BAS_G]=1; gf[13][BAS_T]=1;
            gf[14][BAS_C]=1; gf[14][BAS_G]=1; gf[14][BAS_T]=1;
            gf[15][BAS_A]=1; gf[15][BAS_C]=1;
            gf[15][BAS_G]=1; gf[15][BAS_T]=1;
            return 16;
        }
        return 5;
    }
    else {
        strcpy(groupnames, "-ABCDEFGHI*KLMN.PQRST.VWXYZADHIF\0");

        for (int i=1; i<MAX_AMINOS; i++) gf[i][i] = 1;

#define SC(x, P) gf[x][P-'A'+1] = 1
        if (groupallowed) {
            SC(27, 'P'); SC(27, 'A'); SC(27, 'G'); SC(27, 'S'); SC(27, 'T');                // PAGST
            SC(28, 'Q'); SC(28, 'N'); SC(28, 'E'); SC(28, 'D'); SC(28, 'B'); SC(28, 'Z');   // QNEDBZ
            SC(29, 'H'); SC(29, 'K'); SC(29, 'R');                                          // HKR
            SC(30, 'L'); SC(30, 'I'); SC(30, 'V'); SC(30, 'M');                             // LIVM
            SC(31, 'F'); SC(31, 'Y'); SC(31, 'W');                                          // FYW

            return 32;
        }
#undef SC
        return 27;
    }
}


static int CON_makestatistic(GBDATA *gb_main, int *const*const statistic, const int *convtable, const char *aliname, bool onlymarked) {
    /*! read sequence data and fill into 'statistic'
     */
    long maxalignlen = GBT_get_alignment_len(gb_main, aliname);

    int nrofspecies;
    if (onlymarked) {
        nrofspecies = GBT_count_marked_species(gb_main);
    }
    else {
        nrofspecies = GBT_get_species_count(gb_main);
    }

    arb_progress progress(nrofspecies);
    progress.auto_subtitles("Examining sequence");

    GBDATA *gb_species;
    if (onlymarked) {
        gb_species = GBT_first_marked_species(gb_main);
    }
    else {
        gb_species = GBT_first_species(gb_main);
    }

    while (gb_species) {
        GBDATA *alidata = GBT_find_sequence(gb_species, aliname);
        if (alidata) {
            unsigned char        c;
            const unsigned char *data = (const unsigned char *)GB_read_char_pntr(alidata);

            int i = 0;
            while ((c=data[i])) {
                if ((c=='-') || ((c>='a')&&(c<='z')) || ((c>='A')&&(c<='Z')) || (c=='*')) {
                    if (i > maxalignlen) break;
                    statistic[convtable[c]][i] += 1;
                }
                i++;
            }
        }
        if (onlymarked) {
            gb_species = GBT_next_marked_species(gb_species);
        }
        else {
            gb_species = GBT_next_species(gb_species);
        }
        ++progress;
    }
    return nrofspecies;
}

static int CON_insertSequences(GBDATA *gb_main, const char *aliname, bool onlymarked, BaseFrequencies& freqs) { // @@@ pass maxalignlen?
    /*! read sequence data and fill into 'freqs'
     */
    long maxalignlen = GBT_get_alignment_len(gb_main, aliname);
    int  nrofspecies = onlymarked ? GBT_count_marked_species(gb_main) : GBT_get_species_count(gb_main);

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
    return nrofspecies;
}

static void CON_maketables(int *const convtable, int **const statistic, long maxalignlen, bool isamino) {
    /*! initialize tables 'convtable' and 'statistic'.
     * convtable[c] == k means: character 'c' will (later) be counted in row 'k' of 'statistic'
     */
    int i;
    for (i=0; i<256; i++) convtable[i]=0;
    if (!isamino) {
        for (i='a'; i <= 'z'; i++) convtable[i] = BAS_N;
        for (i='A'; i <= 'Z'; i++) convtable[i] = BAS_N;

        convtable['a']=BAS_A;   convtable['A']=BAS_A;
        convtable['c']=BAS_C;   convtable['C']=BAS_C;
        convtable['g']=BAS_G;   convtable['G']=BAS_G;
        convtable['t']=BAS_T;   convtable['T']=BAS_T;
        convtable['u']=BAS_T;   convtable['U']=BAS_T;


        for (i=0; i<MAX_BASES; i++) {
            statistic[i]=(int*)GB_calloc((unsigned int)maxalignlen, sizeof(int));
        }
        statistic[MAX_BASES]=NULL;
    }
    else {
        for (i=0; i<MAX_AMINOS-1; i++) {
            convtable['a'+i]=i+1;
            convtable['A'+i]=i+1;
        }
        for (i=0; i<MAX_AMINOS; i++) {
            statistic[i]=(int*)GB_calloc((unsigned int)maxalignlen, sizeof(int));
        }
        statistic[MAX_AMINOS]=NULL;
        convtable['*']=10; // 'J' // @@@ hack? careful when introducing 'J' later
    }
}

static GB_ERROR CON_export(GBDATA *gb_main, const char *savename, const char *align, const char *result, bool onlymarked, long nrofspecies, const ConsensusBuildParams& BK) {
    /*! writes consensus SAI to DB
     * @@@ document params
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


static void CON_cleartables(int **const statistic, bool isamino) {
    int i;
    int no_of_tables = isamino ? MAX_AMINOS : MAX_BASES;

    for (i = 0; i<no_of_tables; ++i) {
        free(statistic[i]);
    }
}

static void CON_calculate(GBDATA *gb_main, const ConsensusBuildParams& BK, const char *align, bool onlymarked, const char *sainame) { // @@@ rewrite using BaseFrequencies
    /*! calculates the consensus and writes it to SAI 'sainame'
     * @@@ document params
     * Description how consensus is calculated: ../HELP_SOURCE/oldhelp/consensus_def.hlp // @@@ update after #663 differences are resolved
     */
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);

    long maxalignlen = GBT_get_alignment_len(gb_main, align);
    if (maxalignlen <= 0) error = GB_export_errorf("alignment '%s' doesn't exist", align);

    if (!error) {
        int isamino = GBT_is_alignment_protein(gb_main, align); // @@@ -> bool

        // creating the table for characters and allocating memory for 'statistic'
        int *statistic[MAX_AMINOS+1];
        int  convtable[256];
        CON_maketables(convtable, statistic, maxalignlen, isamino);

        // filling the statistic table
        arb_progress progress("Calculating consensus");

        int nrofspecies = CON_makestatistic(gb_main, statistic, convtable, align, onlymarked);

        if (BK.lower>BK.upper) {
            error = "fault: lower greater than upper";
        }
        else {
            // creating the table for groups
            char *groupflags[40];
            char  groupnames[40];
            int   numgroups = CON_makegrouptable(groupflags, groupnames, isamino, BK.group);

            // calculate and export the result strings
            char *result = CON_evaluatestatistic(statistic, groupflags, groupnames, maxalignlen, numgroups, BK);

            error = CON_export(gb_main, sainame, align, result, onlymarked, nrofspecies, BK);

            // freeing allocated memory
            free(result);
            for (int i=0; i<MAX_GROUPS; i++) free(groupflags[i]);
        }

        CON_cleartables(statistic, isamino);
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static void CON_calculate_cb(AW_window *aw) {
    AW_root *awr        = aw->get_root();
    char    *align      = awr->awar(AWAR_CONSENSUS_ALIGNMENT)->read_string();
    char    *sainame    = awr->awar(AWAR_CONSENSUS_NAME)->read_string();
    bool     onlymarked = strcmp(awr->awar(AWAR_CONSENSUS_SPECIES)->read_char_pntr(), "marked") == 0;

    ConsensusBuildParams BK(awr);

    {
#if defined(ASSERTION_USED)
        GB_transaction ta(GLOBAL.gb_main);
        LocallyModify<bool> denyAwarReads(AW_awar::deny_read, true);
        LocallyModify<bool> denyAwarWrites(AW_awar::deny_write, true);
#endif

        CON_calculate(GLOBAL.gb_main, BK, align, onlymarked, sainame);
    }

    free(sainame);
    free(align);
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
    /*! gets the maximum frequency for all columns
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
        long nrofspecies = CON_insertSequences(gb_main, aliname, onlymarked, freqs);

        char *result1 = new char[maxalignlen+1];
        char *result2 = new char[maxalignlen+1];

        result1[maxalignlen] = 0;
        result2[maxalignlen] = 0;

        for (int pos = 0; pos < maxalignlen; pos++) {
            double mf = freqs.max_frequency_at(pos, ignore_gaps);
            if (mf) {
                // @@@ change unused character to '?' below:
                result1[pos] = "01234567890"[int( 10*mf)%11]; // %11 maps 100% -> '0'
                result2[pos] = "01234567890"[int(100*mf)%10];
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
    // keep similar to ../SL/CONSENSUS/chartable.cxx@TEST_nucleotide_consensus_input
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
        "=.---...aaaACccMMMMMaa-........g.kkk.uKb.ssVVmmss...-.ww...=---M--...mmm..MMMaaMMMMMmmmmm...uuu---...mmmM......MMMMMMMMMMMMMMMMMMaaMMMMMmmMMMMMMMMMMMMMMMMMM", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "=.AAAAAAAAAACccMMMMMaaKgKugKKKuggKKKuuKb.ssVVmmssssBWWWWs..=MMMMMMMMMMMMMMMMMaaMMMMMmmmmm...uuuMMMMMMMMMM......MMMMMMMMMMMMMMMMMMaaMMMMMmmMMMMMMMMMMMMMMMMMM", // countgaps=0
        "=.AAAAAAAAAACCCMMMMMAAKGKUGKKKUGGKKKUUKBsSSVVMMSSSSBWWWWSw-=MMMMMMMMMMMMMMMMMAAMMMMMMMMMmmuuuUUMMMMMMMMMM--mmmmMMMMMMMMMMMMMMMMMMAAMMMMMMMMMMMMMMMMMMMMMMMMM", // countgaps=0,              considbound=26, lower=0, upper=75 (as described in #663)
        "=.AAAAAAAAAACCCMMMMMAAKKKKGKKKUGKKKKKUKBsSSVVMMSSSSBWWWWSwN=MMMMMMMMMMMMMMMMMAAMMMMMMMMMmmuuuUUMMMMMMMMMM--mmmmMMMMMMMMMMMMMMMMMMAAMMMMMMMMMMMMMMMMMMMMMMMMM", // countgaps=0,              considbound=25, lower=0, upper=75
        "=.--aaaaaAAACCCMMMMMAA-g-uggkuuggKKKuuKBsSSVVMMSssc--awWga-=---MmmmmmMMMmmMMMAAMMMMMMMMMmmuuuUU--mmmmmMMM--mmmmMMMMMMMMMMMMMMMMMMAAMMMMMMMMMMMMMMMMMMMMMMMMM", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "=.--aaaaaAAACCMMMMMMMA-gkugkkkugKKKKKuKBNSVVVVMSsssbawwWswN=---MmmmmmMMMmmMMMAMMMMMMMMMMmmuuuUU--mmmmmMMM-NmmmmMMMMMMMMMMMMMMMMMMAMMMMMMMMMMMMMMMMMMMMMMMMMM", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
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
            CON_calculate(gb_main, BK, aliname, false, sainame); // @@@ return and check error

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
    // keep similar to ../SL/CONSENSUS/chartable.cxx@TEST_amino_consensus_input
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
        "0=9876567890987656789987655567898666544457654567654456743334567052404450",
        "0=0000000000000000000000000000000000000000000000000000000000000000000005",
        // ignoring gaps:
        "==0000000000987656789987655567895757865688765678654456743345670=224=4450",
        "==0000000000000000000000000000000505360637520257000000003720050=000=0005",
    };
    const char *expected_consensus[] = {
        "=.---...pppPkkk...qqqnnDDDDIIIll-........v.....w...aaaAa......-=...=dD..", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "=.PPPPPPPPPPkkk...qqqnnDDDDIIIll.v.wv...wvv...ww...aaaAa.....eE=...=dD..", // countgaps=0
        "=.PPPPPPPPPPKKkkkqqQQNNDDDDIIILLvVvWVvvwWVVvvwWWeeaaAAAaaeeeeEE=--k=DD*-", // countgaps=0,              considbound=26, lower=0, upper=75
        "=.--pppppPPPKKkkkqqQQNNDDDDIIILL-v-wvvvwwvvvvwwweeaaAAAaaeeeeee=--k=DD*-", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "=.--pppppPPPKKkkkqqQQNDDDDDIIIIL-vvwvvvwwvvvvwwweeaaAAAaaaeeeee=-kk=DD*-", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
        "=.-----ppPPPKKkk-qqQQNNnn---llLL---------vv---wwe----gg--------=---=--*-", // countgaps=1, gapbound=70, considbound=51, lower=0, upper=75
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
            CON_calculate(gb_main, BK, aliname, false, sainame); // @@@ return and check error

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

