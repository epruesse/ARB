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

#include <consensus_config.h>

#define AWAR_MAX_FREQ_PREFIX      "tmp/CON_MAX_FREQ/"
#define AWAR_CONSENSUS_PREFIX     "con/"
#define AWAR_CONSENSUS_PREFIX_TMP "tmp/" AWAR_CONSENSUS_PREFIX

#define AWAR_MAX_FREQ_NO_GAPS  AWAR_MAX_FREQ_PREFIX "no_gaps"
#define AWAR_MAX_FREQ_SAI_NAME AWAR_MAX_FREQ_PREFIX "sai_name"

#define AWAR_CONSENSUS_SPECIES      AWAR_CONSENSUS_PREFIX_TMP "which_species"
#define AWAR_CONSENSUS_ALIGNMENT    AWAR_CONSENSUS_PREFIX_TMP "alignment"
#define AWAR_CONSENSUS_COUNTGAPS    AWAR_CONSENSUS_PREFIX "countgaps"
#define AWAR_CONSENSUS_GAPBOUND     AWAR_CONSENSUS_PREFIX "gapbound"
#define AWAR_CONSENSUS_GROUP        AWAR_CONSENSUS_PREFIX "group"
#define AWAR_CONSENSUS_FCONSIDBOUND AWAR_CONSENSUS_PREFIX "fconsidbound"
#define AWAR_CONSENSUS_FUPPER       AWAR_CONSENSUS_PREFIX "fupper"
#define AWAR_CONSENSUS_LOWER        AWAR_CONSENSUS_PREFIX "lower"
#define AWAR_CONSENSUS_RESULT       AWAR_CONSENSUS_PREFIX "result"
#define AWAR_CONSENSUS_NAME         AWAR_CONSENSUS_PREFIX_TMP "name"

enum {
    BAS_GAP,
    BAS_A,  BAS_C,  BAS_G,  BAS_T, BAS_N,

    MAX_BASES,
    MAX_AMINOS  = 27,
    MAX_GROUPS  = 40
};

struct ConsensusBuildParams { // @@@ DRY; copy of ../EDIT4/ED4_mini_classes.cxx@ConsensusBuildParams
    int countgaps; // @@@ -> bool
    int gapbound;
    int group; // @@@ -> bool
    int considbound;
    int upper;
    int lower;

    static void force_in_range(int low, int& val, int high) {
        val = std::min(std::max(low, val), high);
    }

    void make_valid() {
        force_in_range(0, gapbound,    100);
        force_in_range(0, considbound, 100);
        force_in_range(0, upper,       100);
        force_in_range(0, lower,       100);
    }

    ConsensusBuildParams(AW_root *awr) {
        countgaps   = strcmp(awr->awar(AWAR_CONSENSUS_COUNTGAPS)->read_char_pntr(), "on") == 0;
        gapbound    = awr->awar(AWAR_CONSENSUS_GAPBOUND)->read_int();
        group       = strcmp(awr->awar(AWAR_CONSENSUS_GROUP)->read_char_pntr(), "on") == 0;
        considbound = awr->awar(AWAR_CONSENSUS_FCONSIDBOUND)->read_float();
        upper       = awr->awar(AWAR_CONSENSUS_FUPPER)->read_float();
        lower       = awr->awar(AWAR_CONSENSUS_LOWER)->read_int();
    }
#if defined(UNIT_TESTS) // UT_DIFF
    ConsensusBuildParams() {
        // (should) use awar defaults // @@@ check
        countgaps   = 1;
        gapbound    = 60;
        group       = 1;
        considbound = 30;
        upper       = 95;
        lower       = 70;
    }
#endif
};

static void CON_evaluatestatistic(char*& result, int **statistic, char **groupflags, char *groupnames, int alignlength, double fupper, int lower, double fconsidbound, int gapbound, int countgap, int numgroups) {
    /*! calculates consensus from 'statistic'
     * @@@ doc params
     */
    int row=0;
    int j = 0;
    int groupfr[MAX_GROUPS]; // frequency of group
    int highestfr, highestgr;

    arb_progress progress("calculating result", alignlength);

    result=(char *)GB_calloc(alignlength+1, 1);

    for (int column=0; column<alignlength; column++) {
        long numentries = 0;
        for (row=0; statistic[row]; row++) numentries += statistic[row][column];
        if (numentries==0) {
            result[column]='.';
        }
        else {
            if (numentries-statistic[0][column]==0) {
                result[column]='='; // 100 per cent `-` -> `=` 
            }
            else {
                if (!countgap) {
                    numentries -= statistic[0][column];
                    statistic[0][column]=0;
                }

                if ((statistic[0][column]*100/numentries)>gapbound) {
                    result[column]='-';
                }
                else {
                    for (j=0; j<numgroups; j++) {
                        groupfr[j]=0;
                    }

                    row=0;
                    while (statistic[row]) {
                        if ((statistic[row][column]*100.0)>=fconsidbound*numentries) {
                            for (j=numgroups-1; j>=0; j--) {
                                if (groupflags[j][row]) {
                                    groupfr[j] += statistic[row][column];
                                }
                            }
                        }
                        row++;
                    }

                    highestfr=0;
                    highestgr=0;
                    for (j=0; j<numgroups; j++) {
                        if (groupfr[j] > highestfr) {
                            highestfr=groupfr[j];
                            highestgr=j;
                        }
                    }

                    if ((highestfr*100.0/numentries)>=fupper) {
                        result[column]=groupnames[highestgr];
                    }
                    else if ((highestfr*100/numentries)>=lower) {
                        char c=groupnames[highestgr];
                        if (c>='A' && c<='Z') {
                            c=c-'A'+'a';
                        }
                        result[column]=c;
                    }
                    else {
                        result[column]='.';
                    }
                }
            }
        }
        ++progress;
    }
}



static int CON_makegrouptable(char **gf, char *groupnames, int isamino, int groupallowed) {
    /*! initialize group tables
     * 'gf' will be allocated and initialized:
     *      'gf[GROUP][CTV] == 1' means: all 'c' with 'convtable[c] == CTV' are members of group 'GROUP'
     * 'groupnames' will be filled with groups "names" ( = single character codes)
     */
    for (int j=0; j<MAX_GROUPS; j++) {
        gf[j]=(char *)GB_calloc(MAX_GROUPS, 1); }

    if (!isamino)
    {
        strcpy(groupnames, "-ACGUMRWSYKVHDBN\0");
        for (int i=1; i<MAX_BASES; i++) {
            gf[i][i]=1; }
        if (groupallowed)
        {
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
            return (16);
        }
        return (5);
    }
    else
    {
        strcpy(groupnames, "-ABCDEFGHI*KLMN.PQRST.VWXYZADHIF\0");
        for (int i=1; i<MAX_AMINOS; i++) {
            gf[i][i]=1; }
        if (groupallowed)
#define SC(x, P) gf[x][P-'A'+1] = 1
        {
            SC(27, 'P'); SC(27, 'A'); SC(27, 'G'); SC(27, 'S'); SC(27, 'T');
            // PAGST
            SC(28, 'Q'); SC(28, 'N'); SC(28, 'E'); SC(28, 'D'); SC(28, 'B');
            SC(28, 'Z');   // QNEDBZ
            SC(29, 'H'); SC(29, 'K'); SC(29, 'R');
            // HKR
            SC(30, 'L'); SC(30, 'I'); SC(30, 'V'); SC(30, 'M');
            // LIVM
            SC(31, 'F'); SC(31, 'Y'); SC(31, 'W');
            // FYW
            return (32);
        }
#undef SC
        return (27);
    }
}


static long CON_makestatistic(GBDATA *gb_main, int **statistic, int *convtable, const char *align, int onlymarked) { // @@@ rename -> countAndGroupChars; fix param types (const, bool)
    /*! read sequence data and fill into 'statistic'
     */
    long maxalignlen = GBT_get_alignment_len(gb_main, align);

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
        GBDATA *alidata = GBT_find_sequence(gb_species, align);
        if (alidata) {
            unsigned char        c;
            const unsigned char *data = (const unsigned char *)GB_read_char_pntr(alidata);

            int i = 0;
            while ((c=data[i])) {
                if ((c=='-') || ((c>='a')&&(c<='z')) || ((c>='A')&&(c<='Z'))
                    || (c=='*')) {
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

static void CON_maketables(int *convtable, int **statistic, long maxalignlen, int isamino) {
    /*! initialize tables 'convtable' and 'statistic'.
     * convtable[c] == k means: character 'c' will (later) be counted in row 'k' of 'statistic'
     */
    int i;
    for (i=0; i<256; i++) { convtable[i]=0; }
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
        for (i=0; i<MAX_AMINOS-1; i++)
        {
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

static GB_ERROR CON_export(GBDATA *gb_main, const char *savename, const char *align, int **statistic, char *result, int *convtable, char *groupnames, int onlymarked, long nrofspecies, long maxalignlen, int countgaps, int gapbound, int groupallowed, double fconsidbound, double fupper, int lower, int resultiscomplex) {
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
    const char *countgapsstring = countgaps ? on : off;
    const char *simplifystring  = groupallowed ? on : off;

    sprintf(buffer, "CON: [species: %s]  [number: %ld]  [count gaps: %s] "
            "[threshold for gaps: %d]  [simplify: %s] "
            "[threshold for character: %f]  [upper: %f]  [lower: %d]",
            allvsmarked, nrofspecies, countgapsstring,
            gapbound, simplifystring,
            fconsidbound, fupper, lower);

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

    {
        char buffer2[256];
        sprintf(buffer2, "%s/FREQUENCIES", align);
        GBDATA *gb_graph = GB_search(gb_extended, buffer2, GB_FIND);
        if (gb_graph) GB_delete(gb_graph);  // delete old entry
    }
    // export additional information
    if (resultiscomplex) {
        GBDATA *gb_graph = GBT_add_data(gb_extended, align, "FREQUENCIES", GB_DB);
        char   *charname = (char *)GB_calloc(5, sizeof(char));

        // problem : aminos, especially '*' -> new order

        int *allreadycounted = (int*)GB_calloc((unsigned int)256, sizeof(char));
        int *neworder        = (int*)GB_calloc((unsigned int)256, sizeof(int));
        int  numdiffchars    = 1; // first additional row (nr. 0) is max-row

        for (int c=0; c<256; c++) {
            int k = convtable[c];
            if (k) {
                if (!(allreadycounted[k])) {
                    allreadycounted[k]       = 1;
                    neworder[numdiffchars++] = k;
                }
            }
        }

        float **additional = (float**)GB_calloc((unsigned int)numdiffchars, sizeof(float*));

        for (int group=0; group<numdiffchars; group++) {
            additional[group]=(float*)GB_calloc((unsigned int)maxalignlen, sizeof(float));
        }

        int *absolutrow = (int*)GB_calloc((unsigned int)maxalignlen, sizeof(int));

        for (long col=0; col<maxalignlen; col++) {
            int group2 = 1;
            int colsum = 0;
            while (neworder[group2]) {
                colsum += statistic[neworder[group2++]][col];
            }
            if (countgaps) colsum += statistic[0][col];
            absolutrow[col] = colsum;
        }

        for (long col=0; col<maxalignlen; col++) {
            if (absolutrow[col]) {
                int   group2  = 1;
                float highest = 0;
                int   diffchar;

                while ((diffchar=neworder[group2++])) {
                    float relative = (float)statistic[diffchar][col] / (float)absolutrow[col];
                    if (relative>highest) highest = relative;
                    additional[diffchar][col]     = relative;
                }
                additional[0][col]=highest;
            }
            else {
                additional[0][col]=0.0;
            }
        }

        GBDATA *gb_relative = GB_search(gb_graph, "MAX", GB_FLOATS);
        err = GB_write_floats(gb_relative, additional[0], maxalignlen);

        for (int group=1; group<numdiffchars; group++) {
            char ch = groupnames[neworder[group]];
            if (ch <'A' || ch>'Z') continue;
            
            sprintf(charname, "N%c", ch);
            gb_relative = GB_search(gb_graph, charname, GB_FLOATS);
            err = GB_write_floats(gb_relative, additional[group], maxalignlen);
        }

        free(charname);
        free(neworder);
        free(allreadycounted);

        for (int group=0; group<numdiffchars; group++) free(additional[group]);
        free(additional);
    }
    free(buffer);
    return err;
}


static void CON_cleartables(int **statistic, int isamino) {
    int i;
    int no_of_tables = isamino ? MAX_AMINOS : MAX_BASES;

    for (i = 0; i<no_of_tables; ++i) {
        free(statistic[i]);
    }
}

static void CON_calculate(GBDATA *gb_main, const ConsensusBuildParams& BK, const char *align, bool onlymarked, const char *sainame, bool resultiscomplex) {
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

        long nrofspecies = CON_makestatistic(gb_main, statistic, convtable, align, onlymarked);

        if (BK.lower>BK.upper) {
            error = "fault: lower greater than upper";
        }
        else {
            // creating the table for groups
            char *groupflags[40];
            char  groupnames[40];
            int   numgroups = CON_makegrouptable(groupflags, groupnames, isamino, BK.group);

            // calculate and export the result strings
            char *result = 0;
            CON_evaluatestatistic(result, statistic, groupflags, groupnames, (int)maxalignlen, BK.upper, BK.lower, BK.considbound, BK.gapbound, BK.countgaps, numgroups);

            error = CON_export(gb_main, sainame, align, statistic, result, convtable, groupnames, onlymarked, nrofspecies, maxalignlen, BK.countgaps, BK.gapbound, BK.group, BK.considbound, BK.upper, BK.lower, resultiscomplex);

            // freeing allocated memory
            free(result);
            for (int i=0; i<MAX_GROUPS; i++) free(groupflags[i]);
        }

        CON_cleartables(statistic, isamino);
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static void CON_calculate_cb(AW_window *aw) {
    AW_root *awr             = aw->get_root();
    char    *align           = awr->awar(AWAR_CONSENSUS_ALIGNMENT)->read_string();
    char    *sainame         = awr->awar(AWAR_CONSENSUS_NAME)->read_string();
    bool     onlymarked      = strcmp(awr->awar(AWAR_CONSENSUS_SPECIES)->read_char_pntr(), "marked") == 0;
    bool     resultiscomplex = strcmp(awr->awar(AWAR_CONSENSUS_RESULT)->read_char_pntr(), "complex") == 0;

    ConsensusBuildParams BK(awr);

    {
#if defined(ASSERTION_USED)
        GB_transaction ta(GLOBAL.gb_main);
        LocallyModify<bool> denyAwarReads(AW_awar::deny_read, true);
        LocallyModify<bool> denyAwarWrites(AW_awar::deny_write, true);
#endif

        CON_calculate(GLOBAL.gb_main, BK, align, onlymarked, sainame, resultiscomplex);
    }

    free(sainame);
    free(align);
}

void AP_create_consensus_var(AW_root *aw_root, AW_default aw_def)
{
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
    aw_root->awar_string(AWAR_CONSENSUS_RESULT,       "single line", aw_def);

    aw_root->awar_string(AWAR_MAX_FREQ_SAI_NAME, "MAX_FREQUENCY", aw_def);
    aw_root->awar_int   (AWAR_MAX_FREQ_NO_GAPS,  1,               aw_def);
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
    { AWAR_CONSENSUS_RESULT,  "result" },
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

    aws->at("result");
    aws->create_toggle_field(AWAR_CONSENSUS_RESULT, NULL, "");
    aws->insert_toggle("single line", "1", "single line");
    aws->insert_default_toggle("complex", "1", "complex");
    aws->update_toggle_field();

    aws->at("save_box");
    awt_create_SAI_selection_list(GLOBAL.gb_main, aws, AWAR_CONSENSUS_NAME, false);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, CONSENSUS_CONFIG_ID, consensus_config_mapping);

    return aws;
}

static void CON_calc_max_freq_cb(AW_window *aw) {
    /*! gets the maximum frequency for all columns
     */
    arb_assert(!GB_have_error());

    AW_root  *awr   = aw->get_root();
    GB_ERROR  error = NULL;

    GB_transaction ta(GLOBAL.gb_main);

    char *align       = GBT_get_default_alignment(GLOBAL.gb_main);
    long  maxalignlen = GBT_get_alignment_len(GLOBAL.gb_main, align);

    if (maxalignlen<=0) {
        GB_clear_error();
        error = "alignment doesn't exist!";
    }
    else {
        int isamino = GBT_is_alignment_protein(GLOBAL.gb_main, align);

        arb_progress progress("Calculating max. frequency");

        int *statistic[MAX_AMINOS+1];
        int  convtable[256];
        CON_maketables(convtable, statistic, maxalignlen, isamino);

        const int onlymarked  = 1;
        long      nrofspecies = CON_makestatistic(GLOBAL.gb_main, statistic, convtable, align, onlymarked);

        int end = isamino ? MAX_AMINOS : MAX_BASES;

        char *result1 = new char[maxalignlen+1];
        char *result2 = new char[maxalignlen+1];

        result1[maxalignlen] = 0;
        result2[maxalignlen] = 0;

        long no_gaps = awr->awar(AWAR_MAX_FREQ_NO_GAPS)->read_int();

        for (int pos = 0; pos < maxalignlen; pos++) {
            int sum = 0;
            int max = 0;

            for (int i=0; i<end; i++) {
                if (no_gaps && (i == convtable[(unsigned char)'-'])) continue;
                sum                              += statistic[i][pos];
                if (statistic[i][pos] > max) max  = statistic[i][pos];
            }
            if (sum == 0) {
                result1[pos] = '=';
                result2[pos] = '=';
            }
            else {
                result1[pos] = "01234567890"[(( 10*max)/sum)%11]; // @@@ why %11?
                result2[pos] = "01234567890"[((100*max)/sum)%10];
            }
        }

        const char *savename    = awr->awar(AWAR_MAX_FREQ_SAI_NAME)->read_char_pntr();
        GBDATA     *gb_extended = GBT_find_or_create_SAI(GLOBAL.gb_main, savename);
        if (!gb_extended) {
            error = GB_await_error();
        }
        else {
            GBDATA *gb_data1 = GBT_add_data(gb_extended, align, "data", GB_STRING);
            GBDATA *gb_data2 = GBT_add_data(gb_extended, align, "dat2", GB_STRING);

            error             = GB_write_string(gb_data1, result1);
            if (!error) error = GB_write_string(gb_data2, result2);

            GBDATA *gb_options = GBT_add_data(gb_extended, align, "_TYPE", GB_STRING);

            if (!error) {
                const char *type = GBS_global_string("MFQ: [species: %li] [exclude gaps: %li]", nrofspecies, no_gaps);
                error            = GB_write_string(gb_options, type);
            }
        }

        delete [] result1;
        delete [] result2;
        CON_cleartables(statistic, isamino);
    }

    error = ta.close(error);
    if (error) aw_message(error);

    free(align);

    arb_assert(!GB_have_error());
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
    aws->create_toggle(AWAR_MAX_FREQ_NO_GAPS);

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

void TEST_nucleotide_consensus() {
    // keep similar to ../EDIT4/ED4_mini_classes.cxx@TEST_nucleotide_consensus
    const char *sequence[] = {
        "-.AAAAAAAAAAcAAAAAAAAATTTTTTTTTTTTTTTTTAAAAAAAAgggggAAAAgAA---",
        "-.-AAAAAAAAAccAAAAAAAAggTTgTTTTgTTTTTTTcccAAAAAgggggAAAAgAA---",
        "-.--AAAAAAAAcccAAAAAAA-ggTggTTTggTTTTTTccccAAAAgggCCtAAAgAC---",
        "-.---AAAAAAAccccAAAAAA-ggggggTTgggTTTTTcccccAAAggCCC-tAACtC---",
        "-.----AAAAAAcccccAAAAA----ggggTggggTTTTGGGcccAAgCCCt-ttACtG---",
        "-.-----AAAAAccccccAAAA----ggggggggggTTgGGGGcccAcCCtt--tttCG---",
        "-.------AAAAcccccccAAA---------ggggggTgGGGGGccccCt----tt-gT---",
        "-.-------AAAccccccccAA---------ggggggggttGGGGccct------t--Tyyk",
        "-.--------AAcccccccccA----------------gttGGGGGct-------t---ymm",
        "-.---------Acccccccccc----------------gtAGGGGGG---------------",
    };
    const char *expected_consensus[] = {
        "=.---...aaaACccMMMMMaa-........g.kkk.uKb.ssVVmmss...-.ww...---", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "=.AAAAAAAAAACccMMMMMaaKgKugKKKuggKKKuuKb.ssVVmmssssBWWWWs..MMM", // countgaps=0
        "=.AAAAAAAAAACCCMMMMMAAKGKUGKKKUGGKKKUUKBsSSVVMMSSSSBWWWWSw-MMM", // countgaps=0,              considbound=26, lower=0, upper=75 (as described in #663)
        "=.--aaaaaAAACCCMMMMMAA-g-uggkuuggKKKuuKBsSSVVMMSssc--awWga----", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "=.--aaaaaAAACCMMMMMMMA-gkugkkkugKKKKKuKBNSVVVVMSsssbawwWswN---", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
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
            case 1: BK.countgaps   = 0; break;                                 // dont count gaps
            case 2: BK.considbound = 26; BK.lower = 0; BK.upper = 75; break;   // settings from #663
            case 3: BK.countgaps   = 1; BK.gapbound = 70; break;
            case 4: BK.considbound = 20; break;
            default: arb_assert(0); break;                                     // missing
        }

        {
            GB_transaction  ta(gb_main);
            const char     *sainame = "CONSENSUS";
            CON_calculate(gb_main, BK, aliname, false, sainame, false); // @@@ return and check error

            GBDATA     *gb_consensus = GBT_find_SAI(gb_main, sainame);
            GBDATA     *gb_seq       = GBT_find_sequence(gb_consensus, aliname);
            const char *consensus    = GB_read_char_pntr(gb_seq);

            TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        }
    }

    GB_close(gb_main);
}

void TEST_amino_consensus() {
    // keep similar to ../EDIT4/ED4_mini_classes.cxx@TEST_amino_consensus
    const char *sequence[] = {
        "-.ppppppppppQQQQQQQQQDDDDDELLLLLwwwwwwwwwwwwwwwwgggggggggggSSSe-PPP-DEL-",
        "-.-pppppppppkQQQQQQQQnDDDDELLLLLVVwwVwwwwVwwwwwwSgggggggggSSSee-QPP-DEL-",
        "-.--ppppppppkkQQQQQQQnnDDDELLLLL-VVwVVwwwVVwwwwwSSgggggggSSSeee-KQP-DEI-",
        "-.---pppppppkkkQQQQQQnnnDDELLLLL-VVVVVVwwVVVwwwwSSSgggggSSSeee--LQQ-DQI-",
        "-.----ppppppkkkkQQQQQnnnnDELLLLL----VVVVwVVVVwwweSSSgggSSSeee---WKQ-NQJ-",
        "-.-----pppppkkkkkQQQQnnnnnqiLLLL----VVVVVVVVVVwweeSSSggSSeee-----KQ-NQJ-",
        "-.------ppppkkkkkkQQQnnnnnqiiLLL---------VVVVVVweeeSSSgSeee------LK-NZJ-",
        "-.-------pppkkkkkkkQQnnnnnqiiiLL---------VVVVVVVeeeeSSSeee-------LK-NZJ-",
        "-.--------ppkkkkkkkkQnnnnnqiiiiL----------------eeeeeSSee--------WK-BZJ-",
        "-.---------pkkkkkkkkknnnnnqiiiii----------------eeeeeeSe---------WK-BZJ-",
    };
    const char *expected_consensus[] = {
        "=.---...pppPkkk...qqqnnDDDDIIIll-........v.....w...aaaAa......-=...=dD.=", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "=.PPPPPPPPPPkkk...qqqnnDDDDIIIll.v.wv...wvv...ww...aaaAa.....eE=...=dD.=", // countgaps=0
        "=.PPPPPPPPPPKKkkkqqQQNNDDDDIIILLvVvWVvvwWVVvvwWWeeaaAAAaaeeeeEE=--k=DD*=", // countgaps=0,              considbound=26, lower=0, upper=75
        "=.--pppppPPPKKkkkqqQQNNDDDDIIILL-v-wvvvwwvvvvwwweeaaAAAaaeeeeee=--k=DD*=", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "=.--pppppPPPKKkkkqqQQNDDDDDIIIIL-vvwvvvwwvvvvwwweeaaAAAaaaeeeee=-kk=DD*=", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
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
            case 1: BK.countgaps   = 0; break;                                 // dont count gaps
            case 2: BK.considbound = 26; BK.lower = 0; BK.upper = 75; break;   // settings from #663
            case 3: BK.countgaps   = 1; BK.gapbound = 70; break;
            case 4: BK.considbound = 20; break;
            default: arb_assert(0); break;                                     // missing
        }

        {
            GB_transaction  ta(gb_main);
            const char     *sainame = "CONSENSUS";
            CON_calculate(gb_main, BK, aliname, false, sainame, false); // @@@ return and check error

            GBDATA     *gb_consensus = GBT_find_SAI(gb_main, sainame);
            GBDATA     *gb_seq       = GBT_find_sequence(gb_consensus, aliname);
            const char *consensus    = GB_read_char_pntr(gb_seq);

            TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        }
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

