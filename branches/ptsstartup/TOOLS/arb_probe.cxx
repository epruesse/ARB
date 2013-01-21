// =============================================================== //
//                                                                 //
//   File      : arb_probe.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <PT_com.h>
#include <arbdb.h>

#include <client.h>
#include <servercntrl.h>

#include <arb_defs.h>
#include <arb_strbuf.h>
#include <algorithm>
#include <string> // need to include before test_unit.h
#include <unistd.h>

struct apd_sequence {
    apd_sequence *next;
    const char *sequence;
};

struct Params {
    int         DESIGNCLIPOUTPUT;
    int         SERVERID;
    const char *DESIGNNAMES;
    int         DESIGNPROBELENGTH;
    const char *DESIGNSEQUENCE;

    int         MINTEMP;
    int         MAXTEMP;
    int         MINGC;
    int         MAXGC;
    int         MAXBOND;
    int         MINPOS;
    int         MAXPOS;
    int         MISHIT;
    int         MINTARGETS;
    const char *SEQUENCE;
    int         MISMATCHES;
    int         ACCEPTN;
    int         LIMITN;
    int         MAXRESULT;
    int         COMPLEMENT;
    int         WEIGHTED;

    apd_sequence *sequence;

    int         ITERATE;
    int         ITERATE_AMOUNT;
    int         ITERATE_READABLE;
    const char *ITERATE_SEPARATOR;
    const char *ITERATE_TU;

    const char *DUMP;
};


struct gl_struct {
    aisc_com  *link;
    T_PT_MAIN  com;
    T_PT_LOCS  locs;
    int        pd_design_id;

    gl_struct()
        : link(0),
          pd_design_id(0)
    {
    }

};

static Params    P;
static gl_struct pd_gl;

static int init_local_com_struct() {
    const char *user = GB_getenvUSER();

    if (aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, pd_gl.locs,
                    LOCS_USER, user,
                    NULL)) {
        return 1;
    }

    return 0;
}

static const char *AP_probe_pt_look_for_server(ARB_ERROR& error) {
    const char *server_tag = GBS_ptserver_tag(P.SERVERID);

    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag);
    if (error) return NULL;
    
    return GBS_read_arb_tcp(server_tag);
}

class PTserverConnection {
    static int count;
    bool       need_close;
public:
    PTserverConnection(ARB_ERROR& error)
        : need_close(false)
    {
        if (count) {
            error = "Only 1 PTserverConnection allowed";
        }
        else {
            ++count;
            const char *servername = AP_probe_pt_look_for_server(error);
            if (servername) {
                GB_ERROR openerr = NULL;
                pd_gl.link       = aisc_open(servername, pd_gl.com, AISC_MAGIC_NUMBER, &openerr);
                if (openerr) {
                    error = openerr;
                }
                else {
                    if (!pd_gl.link) {
                        error = "Cannot contact PT_SERVER [1]";
                    }
                    else if (init_local_com_struct()) {
                        error = "Cannot contact PT_SERVER [2]";
                    }
                    else {
                        need_close = true;
                    }
                }
            }
        }
    }
    ~PTserverConnection() {
        if (need_close) {
            aisc_close(pd_gl.link, pd_gl.com);
            pd_gl.link = 0;
        }
        --count;
    }
};
int PTserverConnection::count = 0;

static char *AP_dump_index_event(ARB_ERROR& error) {
    PTserverConnection contact(error);

    char *result = NULL;
    if (!error) {
        aisc_put(pd_gl.link, PT_MAIN, pd_gl.com,
                 MAIN_DUMP_NAME, P.DUMP,
                 NULL);

        if (aisc_get(pd_gl.link, PT_MAIN, pd_gl.com,
                     MAIN_DUMP_INDEX, &result,
                     NULL)) {
            error = "Connection to PT_SERVER lost (1)";
        }
        else {
            result = strdup("ok");
        }
    }
    return result;
}

static char *AP_probe_iterate_event(ARB_ERROR& error) {
    PTserverConnection contact(error);

    char *result = NULL;
    if (!error) {
        T_PT_PEP pep;
        int      length = P.ITERATE;

        if (aisc_create(pd_gl.link, PT_LOCS, pd_gl.locs,
                        LOCS_PROBE_FIND_CONFIG, PT_PEP, pep,
                        PEP_PLENGTH,   (long)length,
                        PEP_RESTART,   (long)1,
                        PEP_READABLE,  (long)P.ITERATE_READABLE,
                        PEP_TU,        (long)P.ITERATE_TU[0],
                        PEP_SEPARATOR, (long)P.ITERATE_SEPARATOR[0],
                        NULL))
        {
            error = "Connection to PT_SERVER lost (1)";
        }

        if (!error) {
            int amount          = P.ITERATE_AMOUNT;
            int amount_per_call = AISC_MAX_STRING_LEN/(length+2);

            GBS_strstruct out(50000);
            bool          first = true;

            while (amount && !error) {
                int this_amount = std::min(amount, amount_per_call);

                aisc_put(pd_gl.link, PT_PEP, pep,
                         PEP_NUMGET,      (long)this_amount,
                         PEP_FIND_PROBES, 0,
                         NULL);

                char *pep_result = 0;
                if (aisc_get(pd_gl.link, PT_PEP, pep,
                             PEP_RESULT, &pep_result,
                             NULL)) {
                    error = "Connection to PT_SERVER lost (2)";
                }

                if (!error) {
                    if (pep_result[0]) {
                        if (first) first = false;
                        else out.put(P.ITERATE_SEPARATOR[0]);

                        out.cat(pep_result);
                        amount -= this_amount;
                    }
                    else {
                        amount = 0; // terminate loop
                    }
                }
                free(pep_result);
            }

            if (!error) {
                result = out.release();
            }
        }
    }

    if (error) freenull(result);
    return result;
}

static char *AP_probe_design_event(ARB_ERROR& error) {
    PTserverConnection contact(error);

    if (!error) {
        bytestring bs;
        bs.data = (char*)(P.DESIGNNAMES);
        bs.size = strlen(bs.data)+1;

        T_PT_PDC pdc;
        if (aisc_create(pd_gl.link, PT_LOCS, pd_gl.locs,
                        LOCS_PROBE_DESIGN_CONFIG, PT_PDC, pdc,
                        PDC_PROBELENGTH,  (long)P.DESIGNPROBELENGTH,
                        PDC_MINTEMP,      (double)P.MINTEMP,
                        PDC_MAXTEMP,      (double)P.MAXTEMP,
                        PDC_MINGC,        P.MINGC/100.0,
                        PDC_MAXGC,        P.MAXGC/100.0,
                        PDC_MAXBOND,      (double)P.MAXBOND,
                        PDC_MIN_ECOLIPOS, (long)P.MINPOS,
                        PDC_MAX_ECOLIPOS, (long)P.MAXPOS,
                        PDC_MISHIT,       (long)P.MISHIT,
                        PDC_MINTARGETS,   P.MINTARGETS/100.0,
                        PDC_CLIPRESULT,   (long)P.DESIGNCLIPOUTPUT,
                        NULL))
        {
            error = "Connection to PT_SERVER lost (1)";
        }

        if (!error) {
            for (apd_sequence *s = P.sequence; s; s = s->next) {
                bytestring bs_seq;
                T_PT_SEQUENCE pts;
                bs_seq.data = (char*)s->sequence;
                bs_seq.size = strlen(bs_seq.data)+1;
                aisc_create(pd_gl.link, PT_PDC, pdc,
                            PDC_SEQUENCE, PT_SEQUENCE, pts,
                            SEQUENCE_SEQUENCE, &bs_seq,
                            NULL);
            }

            aisc_put(pd_gl.link, PT_PDC, pdc,
                     PDC_NAMES, &bs,
                     PDC_GO, 0,
                     NULL);

            {
                char *locs_error = 0;
                if (aisc_get(pd_gl.link, PT_LOCS, pd_gl.locs,
                             LOCS_ERROR, &locs_error,
                             NULL)) {
                    error = "Connection to PT_SERVER lost (2)";
                }
                else {
                    if (*locs_error) error = GBS_static_string(locs_error);
                    free(locs_error);
                }
            }

            if (!error) {
                T_PT_TPROBE tprobe;
                aisc_get(pd_gl.link, PT_PDC, pdc,
                         PDC_TPROBE, tprobe.as_result_param(),
                         NULL);


                GBS_strstruct *outstr = GBS_stropen(1000);

                if (tprobe.exists()) {
                    char *match_info = 0;
                    aisc_get(pd_gl.link, PT_TPROBE, tprobe,
                             TPROBE_INFO_HEADER, &match_info,
                             NULL);
                    GBS_strcat(outstr, match_info);
                    GBS_chrcat(outstr, '\n');
                    free(match_info);
                }


                while (tprobe.exists()) {
                    char *match_info = 0;
                    if (aisc_get(pd_gl.link, PT_TPROBE, tprobe,
                                 TPROBE_NEXT, tprobe.as_result_param(),
                                 TPROBE_INFO, &match_info,
                                 NULL)) break;
                    GBS_strcat(outstr, match_info);
                    GBS_chrcat(outstr, '\n');
                    free(match_info);
                }

                return GBS_strclose(outstr);
            }
        }
    }
    return NULL;
}

static char *AP_probe_match_event(ARB_ERROR& error) {
    PTserverConnection contact(error);

    if (!error &&
        aisc_nput(pd_gl.link, PT_LOCS, pd_gl.locs,
                  LOCS_MATCH_REVERSED,       (long)P.COMPLEMENT,
                  LOCS_MATCH_SORT_BY,        (long)P.WEIGHTED,
                  LOCS_MATCH_COMPLEMENT,     0L,
                  LOCS_MATCH_MAX_MISMATCHES, (long)P.MISMATCHES,
                  LOCS_MATCH_N_ACCEPT,       (long)P.ACCEPTN,
                  LOCS_MATCH_N_LIMIT,        (long)P.LIMITN,
                  LOCS_MATCH_MAX_HITS,       (long)P.MAXRESULT,
                  LOCS_SEARCHMATCH,          P.SEQUENCE,
                  NULL)) {
        error = "Connection to PT_SERVER lost (1)";
    }

    if (!error) {
        bytestring bs;
        bs.data = 0;
        {
            char           *locs_error = 0;
            T_PT_MATCHLIST  match_list;
            long            match_list_cnt;

            aisc_get(pd_gl.link, PT_LOCS, pd_gl.locs,
                     LOCS_MATCH_LIST,     match_list.as_result_param(),
                     LOCS_MATCH_LIST_CNT, &match_list_cnt,
                     LOCS_MATCH_STRING,   &bs,
                     LOCS_ERROR,          &locs_error,
                     NULL);
            if (*locs_error) error = GBS_static_string(locs_error);
            free(locs_error);
        }

        if (!error) return bs.data; // freed by caller
        free(bs.data);
    }
    return NULL;
}

static int          pargc;
static const char **pargv = NULL;
static bool         showhelp;

static int getInt(const char *param, int val, int min, int max, const char *description) {
    if (showhelp) {
        printf("    %s=%i [%i .. %i] %s\n", param, val, min, max, description);
        return 0;
    }
    int   i;
    const char *s = 0;

    arb_assert(pargc >= 1);     // otherwise s stays 0

    for (i=1; i<pargc; i++) {
        s = pargv[i];
        if (*s == '-') s++;
        if (!strncasecmp(s, param, strlen(param))) break;
    }
    if (i==pargc) return val;
    s += strlen(param);
    if (*s != '=') return val;
    s++;
    val = atoi(s);
    pargc--;        // remove parameter
    for (; i<pargc; i++) {
        pargv[i] = pargv[i+1];
    }

    if (val<min) val = min;
    if (val>max) val = max;
    return val;
}

static const char *getString(const char *param, const char *val, const char *description) {
    if (showhelp) {
        if (!val) val = "";
        printf("    %s=%s   %s\n", param, val, description);
        return 0;
    }
    int   i;
    const char *s = 0;

    arb_assert(pargc >= 1);     // otherwise s stays 0

    for (i=1; i<pargc; i++) {
        s = pargv[i];
        if (*s == '-') s++;
        if (!strncasecmp(s, param, strlen(param))) break;
    }
    if (i==pargc) return val;
    s += strlen(param);
    if (*s != '=') return val;
    s++;
    pargc--;        // remove parameter
    for (; i<pargc; i++) {
        pargv[i] = pargv[i+1];
    }
    return s;
}

static bool parseCommandLine(int argc, const char * const * const argv) {
    pargc = argc;
    
    // copy argv (since parser will remove matched arguments)
    free(pargv);
    pargv = (const char **)malloc(sizeof(*pargv)*pargc);
    for (int i=0; i<pargc; i++) pargv[i] = argv[i];

    showhelp = (pargc <= 1);

#ifdef UNIT_TESTS
    const int minServerID   = TEST_GENESERVER_ID;
#else // !UNIT_TESTS
    const int minServerID   = 0;
#endif

    P.SERVERID = getInt("serverid", 0, minServerID, 100, "Server Id, look into $ARBHOME/lib/arb_tcp.dat");
#ifdef UNIT_TESTS
    if (P.SERVERID<0) { arb_assert(P.SERVERID == TEST_SERVER_ID || P.SERVERID == TEST_GENESERVER_ID); }
#endif

    P.DESIGNCLIPOUTPUT = getInt("designmaxhits", 100, 10, 10000, "Maximum Number of Probe Design Suggestions");
    P.DESIGNNAMES      = getString("designnames", "",            "List of short names separated by '#'");

    P.sequence = 0;
    while  ((P.DESIGNSEQUENCE = getString("designsequence", 0, "Additional Sequences, will be added to the target group"))) {
        apd_sequence *s  = new apd_sequence;
        s->next          = P.sequence;
        P.sequence       = s;
        s->sequence      = P.DESIGNSEQUENCE;
        P.DESIGNSEQUENCE = 0;
    }
    P.DESIGNPROBELENGTH = getInt("designprobelength", 18,  2,  100,     "Length of probe");
    P.MINTEMP           = getInt("designmintemp",     0,   0,  400,     "Minimum melting temperature of probe");
    P.MAXTEMP           = getInt("designmaxtemp",     400, 0,  400,     "Maximum melting temperature of probe");
    P.MINGC             = getInt("designmingc",       30,  0,  100,     "Minimum gc content");
    P.MAXGC             = getInt("designmaxgc",       80,  0,  100,     "Maximum gc content");
    P.MAXBOND           = getInt("designmaxbond",     0,   0,  10,      "Not implemented");
    P.MINPOS            = getInt("designminpos",      -1,  -1, INT_MAX, "Minimum ecoli position (-1=none)");
    P.MAXPOS            = getInt("designmaxpos",      -1,  -1, INT_MAX, "Maximum ecoli position (-1=none)");
    P.MISHIT            = getInt("designmishit",      0,   0,  10000,   "Number of allowed hits outside the selected group");
    P.MINTARGETS        = getInt("designmintargets",  50,  0,  100,     "Minimum percentage of hits within the selected species");

    P.SEQUENCE = getString("matchsequence",   "agtagtagt", "The sequence to search for");

    P.MISMATCHES = getInt("matchmismatches", 0,       0, 5,       "Maximum Number of allowed mismatches");
    P.COMPLEMENT = getInt("matchcomplement", 0,       0, 1,       "Match reversed and complemented probe");
    P.WEIGHTED   = getInt("matchweighted",   0,       0, 1,       "Use weighted mismatches");
    P.ACCEPTN    = getInt("matchacceptN",    1,       0, 20,      "Amount of N-matches not counted as mismatch");
    P.LIMITN     = getInt("matchlimitN",     4,       0, 20,      "Limit for N-matches. If reached N-matches are mismatches");
    P.MAXRESULT  = getInt("matchmaxresults", 1000000, 0, INT_MAX, "Max. number of matches reported (0=unlimited)");

    P.ITERATE          = getInt("iterate",          0,   1, 20,      "Iterate over probes of given length");
    P.ITERATE_AMOUNT   = getInt("iterate_amount",   100, 1, INT_MAX, "Number of results per answer");
    P.ITERATE_READABLE = getInt("iterate_readable", 1,   0, 1,       "readable results");

    P.ITERATE_TU        = getString("iterate_tu",        "T", "use T or U in readable result");
    P.ITERATE_SEPARATOR = getString("iterate_separator", ";", "Number of results per answer");

    P.DUMP = getString("dump", "", "dump ptserver index to file (may be huge!)");

    if (pargc>1) {
        printf("Unknown (or duplicate) parameter %s\n", pargv[1]);
        return false;
    }
    return !showhelp;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_BASIC_parseCommandLine() {
    {
        const char *args[] = { NULL, "serverid=0"};
        TEST_EXPECT(parseCommandLine(ARRAY_ELEMS(args), args));

        // test default values here
        TEST_EXPECT_EQUAL(P.ACCEPTN, 1);
        TEST_EXPECT_EQUAL(P.LIMITN, 4);
        TEST_EXPECT_EQUAL(P.MISMATCHES, 0);
        TEST_EXPECT_EQUAL(P.MAXRESULT, 1000000);
    }

    {
        const char *args[] = {NULL, "serverid=4", "matchmismatches=2"};
        TEST_EXPECT(parseCommandLine(ARRAY_ELEMS(args), args));
        TEST_EXPECT_EQUAL(P.SERVERID, 4);
        TEST_EXPECT_EQUAL(P.MISMATCHES, 2);
        TEST_EXPECT_EQUAL(args[1], "serverid=4"); // check array args was not modified
    }

    {
        const char *args[] = { NULL, "matchacceptN=0", "matchlimitN=5"};
        TEST_EXPECT(parseCommandLine(ARRAY_ELEMS(args), args));
        TEST_EXPECT_EQUAL(P.ACCEPTN, 0);
        TEST_EXPECT_EQUAL(P.LIMITN, 5);
    }

    {
        const char *args[] = { NULL, "matchmaxresults=100"};
        TEST_EXPECT(parseCommandLine(ARRAY_ELEMS(args), args));
        TEST_EXPECT_EQUAL(P.MAXRESULT, 100);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


static char *execute(ARB_ERROR& error) {
    char *answer;
    if (*P.DESIGNNAMES || P.sequence) {
        answer = AP_probe_design_event(error);
    }
    else if (P.ITERATE>0) {
        answer = AP_probe_iterate_event(error);
    }
    else if (P.DUMP[0]) {
        answer = AP_dump_index_event(error);
    }
    else {
        answer = AP_probe_match_event(error);
    }
    pd_gl.locs.clear();
    return answer;
}

int ARB_main(int argc, const char *argv[]) {
    bool ok = parseCommandLine(argc, argv);
    if (ok) {
        ARB_ERROR  error;
        char      *answer = execute(error);

        arb_assert(contradicted(answer, error));
        
        if (!answer) {
            fprintf(stderr,
                    "arb_probe: Failed to process your request\n"
                    "           Reason: %s",
                    error.deliver());
            ok = false;
        }
        else {
            fputs(answer, stdout);
            free(answer);
            error.expect_no_error();
        }
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static int test_setup(bool use_gene_ptserver) {
    static bool setup[2] = { false, false };
    if (!setup[use_gene_ptserver]) {
        TEST_SETUP_GLOBAL_ENVIRONMENT(use_gene_ptserver ? "ptserver_gene" : "ptserver"); // first call will recreate the test pt-server
        setup[use_gene_ptserver] = true;
    }
    return use_gene_ptserver ? TEST_GENESERVER_ID : TEST_SERVER_ID;
}

// ----------------------------------
//      test probe design / match

#define TEST_RUN_ARB_PROBE__INT(fake_argc,fake_argv)                                    \
    int serverid = test_setup(use_gene_ptserver);                                       \
    TEST_EXPECT_EQUAL(true, parseCommandLine(fake_argc, fake_argv));                    \
    TEST_EXPECT((serverid == TEST_SERVER_ID)||(serverid == TEST_GENESERVER_ID));        \
    P.SERVERID = serverid;                                                              \
    ARB_ERROR error;                                                                    \
    char *answer = execute(error)

#define TEST_RUN_ARB_PROBE__REPORTS_ERROR(fake_argc,fake_argv,expected_error)   \
    TEST_RUN_ARB_PROBE__INT(fake_argc,fake_argv);                               \
    free(answer);                                                               \
    TEST_EXPECT_ERROR_CONTAINS(error.deliver(), expected_error)

#define TEST_RUN_ARB_PROBE(fake_argc,fake_argv)                 \
    TEST_RUN_ARB_PROBE__INT(fake_argc,fake_argv);               \
    TEST_EXPECT_NO_ERROR(error.deliver())

#define TEST_ARB_PROBE(fake_argc,fake_argv,expected) do {       \
        TEST_RUN_ARB_PROBE(fake_argc,fake_argv);                \
        TEST_EXPECT_EQUAL(answer, expected);                    \
        free(answer);                                           \
    } while(0)

#define TEST_ARB_PROBE__BROKEN(fake_argc,fake_argv,expected) do {       \
        TEST_RUN_ARB_PROBE(fake_argc,fake_argv);                        \
        TEST_EXPECT_EQUAL__BROKEN(answer, expected);                    \
        free(answer);                                                   \
    } while(0)

#define TEST_ARB_PROBE_FILT(fake_argc,fake_argv,filter,expected) do {   \
        TEST_RUN_ARB_PROBE(fake_argc,fake_argv);                        \
        char  *filtered   = filter(answer);                             \
        TEST_EXPECT_EQUAL(filtered, expected);                          \
        free(filtered);                                                 \
        free(answer);                                                   \
    } while(0)

typedef const char *CCP;

void TEST_SLOW_match_geneprobe() {
    // test here runs versus database ../UNIT_TESTER/run/TEST_gpt_src.arb

    bool use_gene_ptserver = true;
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=NNUCNN",
            "matchacceptN=4",
            "matchlimitN=5",
        };
        CCP expectd = "    organism genename------- mis N_mis wmis pos gpos rev          'NNUCNN'\1"
            "genome2\1" "  genome2  gene3             0     4  2.6   2    1 0   .........-UU==GG-UUGAUC.\1"
            "genome2\1" "  genome2  joined1           0     4  2.6   2    1 0   .........-UU==GG-UUGAUCCUG\1"
            "genome2\1" "  genome2  gene1             0     4  2.6   2    2 0   ........A-UU==GG-U.\1"
            "genome2\1" "  genome2  intergene_19_65   0     4  2.7  31   12 0   GGUUACUGC-AU==GG-UGUUCGCCU\1"
            "genome1\1" "  genome1  intergene_17_65   0     4  2.7  31   14 0   GGUUACUGC-UA==GG-UGUUCGCCU\1"
            "genome2\1" "  genome2  intergene_19_65   0     4  2.7  38   19 0   GCAUUCGGU-GU==GC-CUAAGCACU\1"
            "genome1\1" "  genome1  intergene_17_65   0     4  2.7  38   21 0   GCUAUCGGU-GU==GC-CUAAGCCAU\1"
            "genome2\1" "  genome2  gene3             0     4  2.9  10    9 0   .UUUCGGUU-GA==..-\1"
            "genome2\1" "  genome2  intergene_19_65   0     4  3.1  56   37 0   AGCACUGCG-AG==AU-AUGUA.\1"
            "genome1\1" "  genome1  intergene_17_65   0     4  3.1  56   39 0   AGCCAUGCG-AG==AU-AUGUA.\1"
            "genome1\1" "  genome1  gene2             0     4  3.1  10    2 0   ........U-GA==CU-GC.\1"
            "genome2\1" "  genome2  gene2             0     4  3.1  10    4 0   ......GUU-GA==CU-GCCA.\1"
            "genome1\1" "  genome1  joined1           0     4  3.1  10    7 0   ...CUGGUU-GA==CU-GC.\1"
            "genome2\1" "  genome2  joined1           0     4  3.1  10    9 0   .UUUCGGUU-GA==CU-GCCA.\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=NGGUUN",
            "matchacceptN=2",
            "matchlimitN=3",
        };
        CCP expectd = "    organism genename------- mis N_mis wmis pos gpos rev          'NGGUUN'\1"
            "genome1\1" "  genome1  gene3             0     2  1.3   5    2 0   ........C-U====G-A.\1"
            "genome1\1" "  genome1  joined1           0     2  1.3   5    2 0   ........C-U====G-AUCCUGC.\1"
            "genome2\1" "  genome2  gene3             0     2  1.4   5    4 0   ......UUU-C====G-AUC.\1"
            "genome2\1" "  genome2  joined1           0     2  1.4   5    4 0   ......UUU-C====G-AUCCUGCCA\1"
            "genome2\1" "  genome2  intergene_19_65   0     2  1.8  21    2 0   ........G-A====A-CUGCAUUCG\1"
            "genome1\1" "  genome1  intergene_17_65   0     2  1.8  21    4 0   ......CAG-A====A-CUGCUAUCG\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UGAUCCU", // exists in data
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'UGAUCCU'\1"
            "genome1\1" "  genome1  gene2      0     0  0.0   9    1 0   .........-=======-GC.\1"
            "genome2\1" "  genome2  gene2      0     0  0.0   9    3 0   .......GU-=======-GCCA.\1"
            "genome1\1" "  genome1  joined1    0     0  0.0   9    6 0   ....CUGGU-=======-GC.\1"
            "genome2\1" "  genome2  joined1    0     0  0.0   9    8 0   ..UUUCGGU-=======-GCCA.\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // [fixed: now reports hits in  'joined1' (of both genomes)]
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GAUCCU",
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'GAUCCU'\1"
            "genome1\1" "  genome1  gene2      0     0  0.0  10    2 0   ........U-======-GC.\1"
            "genome2\1" "  genome2  gene2      0     0  0.0  10    4 0   ......GUU-======-GCCA.\1"
            "genome1\1" "  genome1  joined1    0     0  0.0  10    7 0   ...CUGGUU-======-GC.\1"
            "genome2\1" "  genome2  joined1    0     0  0.0  10    9 0   .UUUCGGUU-======-GCCA.\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // [fixed: now reports as much hits as previous test; expected cause probe is part of above probe]
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UUUCGG", // exists only in genome2 
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'UUUCGG'\1"
            "genome2\1" "  genome2  gene3      0     0  0.0   2    1 0   .........-======-UUGAUC.\1"
            "genome2\1" "  genome2  joined1    0     0  0.0   2    1 0   .........-======-UUGAUCCUG\1"
            "genome2\1" "  genome2  gene1      0     0  0.0   2    2 0   ........A-======-U.\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // [fixed: now reports hit in genome2/gene1]
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=AUCCUG", 
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'AUCCUG'\1"
            "genome1\1" "  genome1  gene2      0     0  0.0  11    3 0   .......UG-======-C.\1"
            "genome2\1" "  genome2  gene2      0     0  0.0  11    5 0   .....GUUG-======-CCA.\1"
            "genome1\1" "  genome1  joined1    0     0  0.0  11    8 0   ..CUGGUUG-======-C.\1"
            "genome2\1" "  genome2  joined1    0     0  0.0  11   10 0   UUUCGGUUG-======-CCA.\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // [fixed: now reports hits in 'gene2' and 'joined1' of both genomes]
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UUGAUCCUGC",
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'UUGAUCCUGC'\1"
            "genome2\1" "  genome2  gene2      0     0  0.0   8    2 0   ........G-==========-CA.\1"
            "genome1\1" "  genome1  joined1    0     0  0.0   8    5 0   .....CUGG-==========-.\1"
            "genome2\1" "  genome2  joined1    0     0  0.0   8    7 0   ...UUUCGG-==========-CA.\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // [fixed: now reports hit in 'genome2/joined1']
    }
}

void TEST_SLOW_match_probe() {
    // test here runs versus database ../UNIT_TESTER/run/TEST_pt_src.arb

    bool use_gene_ptserver = false;
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UAUCGGAGAGUUUGA",
        };
        CCP expected = "    name---- fullname mis N_mis wmis pos ecoli rev          'UAUCGGAGAGUUUGA'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0   3     2 0   .......UU-===============-UCAAGUCGA\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }

    // ----------------------------------------------------------------------------
    //      match with old(=default) N-mismatch-behavior (accepting 1 N-match)

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=CANCUCCUUUC", // contains 1 N
            NULL // matchmismatches
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.9 176   162 0   CGGCUGGAU-==C========-U.\1"; // only N-mismatch accepted

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.9 176   162 0   CGGCUGGAU-==C========-U.\1"
            "ClfPerfr\1" "  ClfPerfr            1     1  2.0 176   162 0   AGAUUAAUA-=CC========-U.\1"
            "PbcAcet2\1" "  PbcAcet2            1     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "PbrPropi\1" "  PbrPropi            1     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "Stsssola\1" "  Stsssola            1     2  1.6 176   162 0   CGGCUGGAU-==C=======.-\1"
            "DcdNodos\1" "  DcdNodos            1     2  1.6 176   162 0   CGGUUGGAU-==C=======.-\1"
            "VbrFurni\1" "  VbrFurni            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VblVulni\1" "  VblVulni            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VbhChole\1" "  VbhChole            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.9 176   162 0   CGGCUGGAU-==C========-U.\1"
            "ClfPerfr\1" "  ClfPerfr            1     1  2.0 176   162 0   AGAUUAAUA-=CC========-U.\1"
            "PbcAcet2\1" "  PbcAcet2            1     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "PbrPropi\1" "  PbrPropi            1     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "Stsssola\1" "  Stsssola            1     2  1.6 176   162 0   CGGCUGGAU-==C=======.-\1"
            "DcdNodos\1" "  DcdNodos            1     2  1.6 176   162 0   CGGUUGGAU-==C=======.-\1"
            "VbrFurni\1" "  VbrFurni            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VblVulni\1" "  VblVulni            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VbhChole\1" "  VbhChole            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "AclPleur\1" "  AclPleur            2     2  2.7 176   162 0   CGGUUGGAU-==C======A.-\1"
            "PtVVVulg\1" "  PtVVVulg            2     2  2.7 176   162 0   CGGUUGGAU-==C======A.-\1"
            "DlcTolu2\1" "  DlcTolu2            2     3  2.3 176   162 0   CGGCUGGAU-==C======NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            2     3  2.3 176   162 0   CGGCUGGAU-==C======..-\1"
            "HllHalod\1" "  HllHalod            2     3  2.3 176   162 0   CGGCUGGAU-==C======..-\1"
            "CPPParap\1" "  CPPParap            2     3  2.3 177   163 0   CGGNUGGAU-==C======..-\1";

        CCP expectd3 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.9 176   162 0   CGGCUGGAU-==C========-U.\1"
            "ClfPerfr\1" "  ClfPerfr            1     1  2.0 176   162 0   AGAUUAAUA-=CC========-U.\1"
            "PbcAcet2\1" "  PbcAcet2            1     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "PbrPropi\1" "  PbrPropi            1     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "Stsssola\1" "  Stsssola            1     2  1.6 176   162 0   CGGCUGGAU-==C=======.-\1"
            "DcdNodos\1" "  DcdNodos            1     2  1.6 176   162 0   CGGUUGGAU-==C=======.-\1"
            "VbrFurni\1" "  VbrFurni            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VblVulni\1" "  VblVulni            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VbhChole\1" "  VbhChole            1     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "AclPleur\1" "  AclPleur            2     2  2.7 176   162 0   CGGUUGGAU-==C======A.-\1"
            "PtVVVulg\1" "  PtVVVulg            2     2  2.7 176   162 0   CGGUUGGAU-==C======A.-\1"
            "DlcTolu2\1" "  DlcTolu2            2     3  2.3 176   162 0   CGGCUGGAU-==C======NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            2     3  2.3 176   162 0   CGGCUGGAU-==C======..-\1"
            "HllHalod\1" "  HllHalod            2     3  2.3 176   162 0   CGGCUGGAU-==C======..-\1"
            "CPPParap\1" "  CPPParap            2     3  2.3 177   163 0   CGGNUGGAU-==C======..-\1"
            "VblVulni\1" "  VblVulni            3     1  3.6  49    44 0   AGCACAGAG-a=A==uG====-UCGGGUGGC\1";

        arguments[2] = "matchmismatches=0";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
        arguments[2] = "matchmismatches=3";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd3);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UCACCUCCUUUC", // contains no N
            NULL // matchmismatches
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            0     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            0     1  0.7 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            0     1  0.7 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            0     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            0     1  0.7 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            0     1  0.7 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "AclPleur\1" "  AclPleur            1     1  1.8 175   161 0   GCGGUUGGA-==========A.-\1"
            "PtVVVulg\1" "  PtVVVulg            1     1  1.8 175   161 0   GCGGUUGGA-==========A.-\1"
            "DlcTolu2\1" "  DlcTolu2            1     2  1.4 175   161 0   GCGGCUGGA-==========NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     2  1.4 175   161 0   GCGGCUGGA-==========..-\1"
            "HllHalod\1" "  HllHalod            1     2  1.4 175   161 0   GCGGCUGGA-==========..-\1"
            "CPPParap\1" "  CPPParap            1     2  1.4 176   162 0   GCGGNUGGA-==========..-\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            0     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            0     1  0.7 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            0     1  0.7 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            0     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "AclPleur\1" "  AclPleur            1     1  1.8 175   161 0   GCGGUUGGA-==========A.-\1"
            "PtVVVulg\1" "  PtVVVulg            1     1  1.8 175   161 0   GCGGUUGGA-==========A.-\1"
            "DlcTolu2\1" "  DlcTolu2            1     2  1.4 175   161 0   GCGGCUGGA-==========NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     2  1.4 175   161 0   GCGGCUGGA-==========..-\1"
            "HllHalod\1" "  HllHalod            1     2  1.4 175   161 0   GCGGCUGGA-==========..-\1"
            "CPPParap\1" "  CPPParap            1     2  1.4 176   162 0   GCGGNUGGA-==========..-\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C=========-U.\1"
            "LgtLytic\1" "  LgtLytic            2     3  2.1 175   161 0   GCGGCUGGA-=========N..-\1"
            "PslFlave\1" "  PslFlave            2     3  2.1 175   161 0   GCGGCUGGA-=========...-\1";

        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UCACCUCCUUUC", // contains no N
            NULL,                         // matchmismatches
            "matchweighted=1",            // use weighted mismatches
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.2 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            0     1  0.2 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            0     1  0.2 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            0     1  0.2 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.2 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            0     1  0.2 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            0     1  0.2 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            0     1  0.2 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "DlcTolu2\1" "  DlcTolu2            1     2  0.6 175   161 0   GCGGCUGGA-==========NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     2  0.6 175   161 0   GCGGCUGGA-==========..-\1"
            "HllHalod\1" "  HllHalod            1     2  0.6 175   161 0   GCGGCUGGA-==========..-\1"
            "CPPParap\1" "  CPPParap            1     2  0.6 176   162 0   GCGGNUGGA-==========..-\1"
            "AclPleur\1" "  AclPleur            1     1  0.9 175   161 0   GCGGUUGGA-==========A.-\1"
            "PtVVVulg\1" "  PtVVVulg            1     1  0.9 175   161 0   GCGGUUGGA-==========A.-\1"
            "LgtLytic\1" "  LgtLytic            2     3  1.3 175   161 0   GCGGCUGGA-=========N..-\1"
            "PslFlave\1" "  PslFlave            2     3  1.3 175   161 0   GCGGCUGGA-=========...-\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  1.3 175   161 0   AAGAUUAAU-A=C=========-U.\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.2 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            0     1  0.2 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            0     1  0.2 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            0     1  0.2 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            0     1  0.2 175   161 0   GGCGCUGGA-===========.-\1"
            "DlcTolu2\1" "  DlcTolu2            1     2  0.6 175   161 0   GCGGCUGGA-==========NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     2  0.6 175   161 0   GCGGCUGGA-==========..-\1"
            "HllHalod\1" "  HllHalod            1     2  0.6 175   161 0   GCGGCUGGA-==========..-\1"
            "CPPParap\1" "  CPPParap            1     2  0.6 176   162 0   GCGGNUGGA-==========..-\1"
            "AclPleur\1" "  AclPleur            1     1  0.9 175   161 0   GCGGUUGGA-==========A.-\1"
            "PtVVVulg\1" "  PtVVVulg            1     1  0.9 175   161 0   GCGGUUGGA-==========A.-\1"
            "LgtLytic\1" "  LgtLytic            2     3  1.3 175   161 0   GCGGCUGGA-=========N..-\1"
            "PslFlave\1" "  PslFlave            2     3  1.3 175   161 0   GCGGCUGGA-=========...-\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  1.3 175   161 0   AAGAUUAAU-A=C=========-U.\1"
            "AclPleur\1" "  AclPleur            5     0  2.4  50    45 0   GAAGGGAGC-=ug=u=u====G-CCGACGAGU\1"
            "PtVVVulg\1" "  PtVVVulg            5     0  2.4  50    45 0   GGAGAAAGC-=ug=u=u===g=-UGACGAGCG\1";

        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }

    // ----------------------------------------------
    //      do not accept any N-matches as match

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=CANCUCCUUUC", // contains 1 N
            NULL, // matchmismatches
            "matchacceptN=0",
        };

        CCP expectd0 = ""; // nothing matches

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            1     1  0.9 176   162 0   CGGCUGGAU-==C========-U.\1";
 
        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            1     1  0.9 176   162 0   CGGCUGGAU-==C========-U.\1"
            "ClfPerfr\1" "  ClfPerfr            2     1  2.0 176   162 0   AGAUUAAUA-=CC========-U.\1"
            "PbcAcet2\1" "  PbcAcet2            2     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "PbrPropi\1" "  PbrPropi            2     2  1.6 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "Stsssola\1" "  Stsssola            2     2  1.6 176   162 0   CGGCUGGAU-==C=======.-\1"
            "DcdNodos\1" "  DcdNodos            2     2  1.6 176   162 0   CGGUUGGAU-==C=======.-\1"
            "VbrFurni\1" "  VbrFurni            2     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VblVulni\1" "  VblVulni            2     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1"
            "VbhChole\1" "  VbhChole            2     2  1.6 176   162 0   GCGCUGGAU-==C=======.-\1";

        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UCACCUCCUUUC", // contains no N
            NULL, // matchmismatches
            "matchacceptN=0",
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1" "";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            1     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            1     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            1     1  0.7 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            1     1  0.7 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            1     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            1     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            1     1  0.7 175   161 0   GGCGCUGGA-===========.-\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U.\1"
            "PbcAcet2\1" "  PbcAcet2            1     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "PbrPropi\1" "  PbrPropi            1     1  0.7 175   161 0   GCGGCUGGA-===========N-N.\1"
            "Stsssola\1" "  Stsssola            1     1  0.7 175   161 0   GCGGCUGGA-===========.-\1"
            "DcdNodos\1" "  DcdNodos            1     1  0.7 175   161 0   GCGGUUGGA-===========.-\1"
            "VbrFurni\1" "  VbrFurni            1     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VblVulni\1" "  VblVulni            1     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "VbhChole\1" "  VbhChole            1     1  0.7 175   161 0   GGCGCUGGA-===========.-\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C=========-U.\1"
            "AclPleur\1" "  AclPleur            2     1  1.8 175   161 0   GCGGUUGGA-==========A.-\1"
            "PtVVVulg\1" "  PtVVVulg            2     1  1.8 175   161 0   GCGGUUGGA-==========A.-\1"
            "DlcTolu2\1" "  DlcTolu2            2     2  1.4 175   161 0   GCGGCUGGA-==========NN-N.\1"
            "FrhhPhil\1" "  FrhhPhil            2     2  1.4 175   161 0   GCGGCUGGA-==========..-\1"
            "HllHalod\1" "  HllHalod            2     2  1.4 175   161 0   GCGGCUGGA-==========..-\1"
            "CPPParap\1" "  CPPParap            2     2  1.4 176   162 0   GCGGNUGGA-==========..-\1";
        
        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UCACCUCCUUUCU", // contains no N
            NULL, // matchmismatches
            "matchacceptN=0",
        };

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUCU'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-=============-.\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUCU'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-=============-.\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C==========-.\1"
            "PbcAcet2\1" "  PbcAcet2            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "PbrPropi\1" "  PbrPropi            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "Stsssola\1" "  Stsssola            2     2  1.4 175   161 0   GCGGCUGGA-===========..-\1"
            "DcdNodos\1" "  DcdNodos            2     2  1.4 175   161 0   GCGGUUGGA-===========..-\1"
            "VbrFurni\1" "  VbrFurni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VblVulni\1" "  VblVulni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VbhChole\1" "  VbhChole            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1";
        
        CCP expectd3 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUCU'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-=============-.\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C==========-.\1"
            "PbcAcet2\1" "  PbcAcet2            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "PbrPropi\1" "  PbrPropi            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "Stsssola\1" "  Stsssola            2     2  1.4 175   161 0   GCGGCUGGA-===========..-\1"
            "DcdNodos\1" "  DcdNodos            2     2  1.4 175   161 0   GCGGUUGGA-===========..-\1"
            "VbrFurni\1" "  VbrFurni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VblVulni\1" "  VblVulni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VbhChole\1" "  VbhChole            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "AclPleur\1" "  AclPleur            3     2  2.5 175   161 0   GCGGUUGGA-==========A..-\1"
            "PtVVVulg\1" "  PtVVVulg            3     2  2.5 175   161 0   GCGGUUGGA-==========A..-\1"
            "DlcTolu2\1" "  DlcTolu2            3     3  2.1 175   161 0   GCGGCUGGA-==========NNN-.\1"
            "FrhhPhil\1" "  FrhhPhil            3     3  2.1 175   161 0   GCGGCUGGA-==========...-\1"
            "HllHalod\1" "  HllHalod            3     3  2.1 175   161 0   GCGGCUGGA-==========...-\1"
            "CPPParap\1" "  CPPParap            3     3  2.1 176   162 0   GCGGNUGGA-==========...-\1";

        CCP expectd4 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUCU'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-=============-.\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C==========-.\1"
            "PbcAcet2\1" "  PbcAcet2            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "PbrPropi\1" "  PbrPropi            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "Stsssola\1" "  Stsssola            2     2  1.4 175   161 0   GCGGCUGGA-===========..-\1"
            "DcdNodos\1" "  DcdNodos            2     2  1.4 175   161 0   GCGGUUGGA-===========..-\1"
            "VbrFurni\1" "  VbrFurni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VblVulni\1" "  VblVulni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VbhChole\1" "  VbhChole            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "AclPleur\1" "  AclPleur            3     2  2.5 175   161 0   GCGGUUGGA-==========A..-\1"
            "PtVVVulg\1" "  PtVVVulg            3     2  2.5 175   161 0   GCGGUUGGA-==========A..-\1"
            "DlcTolu2\1" "  DlcTolu2            3     3  2.1 175   161 0   GCGGCUGGA-==========NNN-.\1"
            "FrhhPhil\1" "  FrhhPhil            3     3  2.1 175   161 0   GCGGCUGGA-==========...-\1"
            "HllHalod\1" "  HllHalod            3     3  2.1 175   161 0   GCGGCUGGA-==========...-\1"
            "CPPParap\1" "  CPPParap            3     3  2.1 176   162 0   GCGGNUGGA-==========...-\1"
            "LgtLytic\1" "  LgtLytic            4     4  2.8 175   161 0   GCGGCUGGA-=========N...-\1"
            "PslFlave\1" "  PslFlave            4     4  2.8 175   161 0   GCGGCUGGA-=========....-\1";

        CCP expectd5 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUCU'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-=============-.\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C==========-.\1"
            "PbcAcet2\1" "  PbcAcet2            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "PbrPropi\1" "  PbrPropi            2     2  1.4 175   161 0   GCGGCUGGA-===========NN-.\1"
            "Stsssola\1" "  Stsssola            2     2  1.4 175   161 0   GCGGCUGGA-===========..-\1"
            "DcdNodos\1" "  DcdNodos            2     2  1.4 175   161 0   GCGGUUGGA-===========..-\1"
            "VbrFurni\1" "  VbrFurni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VblVulni\1" "  VblVulni            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "VbhChole\1" "  VbhChole            2     2  1.4 175   161 0   GGCGCUGGA-===========..-\1"
            "AclPleur\1" "  AclPleur            3     2  2.5 175   161 0   GCGGUUGGA-==========A..-\1"
            "PtVVVulg\1" "  PtVVVulg            3     2  2.5 175   161 0   GCGGUUGGA-==========A..-\1"
            "DlcTolu2\1" "  DlcTolu2            3     3  2.1 175   161 0   GCGGCUGGA-==========NNN-.\1"
            "FrhhPhil\1" "  FrhhPhil            3     3  2.1 175   161 0   GCGGCUGGA-==========...-\1"
            "HllHalod\1" "  HllHalod            3     3  2.1 175   161 0   GCGGCUGGA-==========...-\1"
            "CPPParap\1" "  CPPParap            3     3  2.1 176   162 0   GCGGNUGGA-==========...-\1"
            "LgtLytic\1" "  LgtLytic            4     4  2.8 175   161 0   GCGGCUGGA-=========N...-\1"
            "PslFlave\1" "  PslFlave            4     4  2.8 175   161 0   GCGGCUGGA-=========....-\1"
            "PtVVVulg\1" "  PtVVVulg            5     0  2.6  50    45 0   GGAGAAAGC-=ug=u=u===g==-GACGAGCGG\1"
            "AclPleur\1" "  AclPleur            5     0  3.5  46    41 0   ACGGGAAGG-gag=u=G======-UUGCCGACG\1"
            "PtVVVulg\1" "  PtVVVulg            5     0  4.0  45    40 0   ...AGGAGA-Aag=u=G======-UGCUGACGA\1"
            "VblVulni\1" "  VblVulni            5     0  4.3  48    43 0   CAGCACAGA-ga=a==uG=====-CGGGUGGCG\1";

        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
        arguments[2] = "matchmismatches=3"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd3);
        arguments[2] = "matchmismatches=4"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd4);
        arguments[2] = "matchmismatches=5"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd5);
    }

    // ----------------------------------
    //      accept several N-matches

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=CANCUCCUUNC", // contains 2 N
            NULL, // matchmismatches
            "matchacceptN=2",
            "matchlimitN=4",
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     2  1.7 176   162 0   CGGCUGGAU-==C======U=-U.\1";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     2  1.7 176   162 0   CGGCUGGAU-==C======U=-U.\1"
            "ClfPerfr\1" "  ClfPerfr            1     2  2.8 176   162 0   AGAUUAAUA-=CC======U=-U.\1"
            "DlcTolu2\1" "  DlcTolu2            1     3  2.4 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     3  2.4 176   162 0   CGGCUGGAU-==C======..-\1"
            "HllHalod\1" "  HllHalod            1     3  2.4 176   162 0   CGGCUGGAU-==C======..-\1"
            "CPPParap\1" "  CPPParap            1     3  2.4 177   163 0   CGGNUGGAU-==C======..-\1"
            "PbcAcet2\1" "  PbcAcet2            1     3  2.4 176   162 0   CGGCUGGAU-==C======UN-N.\1"
            "PbrPropi\1" "  PbrPropi            1     3  2.4 176   162 0   CGGCUGGAU-==C======UN-N.\1"
            "Stsssola\1" "  Stsssola            1     3  2.4 176   162 0   CGGCUGGAU-==C======U.-\1"
            "DcdNodos\1" "  DcdNodos            1     3  2.4 176   162 0   CGGUUGGAU-==C======U.-\1"
            "VbrFurni\1" "  VbrFurni            1     3  2.4 176   162 0   GCGCUGGAU-==C======U.-\1"
            "VblVulni\1" "  VblVulni            1     3  2.4 176   162 0   GCGCUGGAU-==C======U.-\1"
            "VbhChole\1" "  VbhChole            1     3  2.4 176   162 0   GCGCUGGAU-==C======U.-\1"
            "AclPleur\1" "  AclPleur            1     3  2.5 176   162 0   CGGUUGGAU-==C======A.-\1"
            "PtVVVulg\1" "  PtVVVulg            1     3  2.5 176   162 0   CGGUUGGAU-==C======A.-\1";
        
        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     2  1.7 176   162 0   CGGCUGGAU-==C======U=-U.\1"
            "ClfPerfr\1" "  ClfPerfr            1     2  2.8 176   162 0   AGAUUAAUA-=CC======U=-U.\1"
            "DlcTolu2\1" "  DlcTolu2            1     3  2.4 176   162 0   CGGCUGGAU-==C=======N-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     3  2.4 176   162 0   CGGCUGGAU-==C======..-\1"
            "HllHalod\1" "  HllHalod            1     3  2.4 176   162 0   CGGCUGGAU-==C======..-\1"
            "CPPParap\1" "  CPPParap            1     3  2.4 177   163 0   CGGNUGGAU-==C======..-\1"
            "PbcAcet2\1" "  PbcAcet2            1     3  2.4 176   162 0   CGGCUGGAU-==C======UN-N.\1"
            "PbrPropi\1" "  PbrPropi            1     3  2.4 176   162 0   CGGCUGGAU-==C======UN-N.\1"
            "Stsssola\1" "  Stsssola            1     3  2.4 176   162 0   CGGCUGGAU-==C======U.-\1"
            "DcdNodos\1" "  DcdNodos            1     3  2.4 176   162 0   CGGUUGGAU-==C======U.-\1"
            "VbrFurni\1" "  VbrFurni            1     3  2.4 176   162 0   GCGCUGGAU-==C======U.-\1"
            "VblVulni\1" "  VblVulni            1     3  2.4 176   162 0   GCGCUGGAU-==C======U.-\1"
            "VbhChole\1" "  VbhChole            1     3  2.4 176   162 0   GCGCUGGAU-==C======U.-\1"
            "AclPleur\1" "  AclPleur            1     3  2.5 176   162 0   CGGUUGGAU-==C======A.-\1"
            "PtVVVulg\1" "  PtVVVulg            1     3  2.5 176   162 0   CGGUUGGAU-==C======A.-\1";
        
        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GAGCGGUCAG", // similar to a region where dots occur in seqdata
            NULL, // matchmismatches
            "matchacceptN=0",
            "matchlimitN=4",
        };

        CCP expectd0 = "";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'GAGCGGUCAG'\1"
            "BcSSSS00\1" "  BcSSSS00            1     0  1.1  25    21 0   GAUCAAGUC-======A===-AUGGGAGCU\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'GAGCGGUCAG'\1"
            "BcSSSS00\1" "  BcSSSS00            1     0  1.1  25    21 0   GAUCAAGUC-======A===-AUGGGAGCU\1"
            "Bl0LLL00\1" "  Bl0LLL00            2     0  2.2  25    21 0   GAUCAAGUC-======A=C=-ACGGGAGCU\1";

        CCP expectd3 = "    name---- fullname mis N_mis wmis pos ecoli rev          'GAGCGGUCAG'\1"
            "BcSSSS00\1" "  BcSSSS00            1     0  1.1  25    21 0   GAUCAAGUC-======A===-AUGGGAGCU\1"
            "Bl0LLL00\1" "  Bl0LLL00            2     0  2.2  25    21 0   GAUCAAGUC-======A=C=-ACGGGAGCU\1"
            "VbrFurni\1" "  VbrFurni            3     0  2.4  68    60 0   GGAUUUGUU-=g====CG==-CGGCGGACG\1"
            "FrhhPhil\1" "  FrhhPhil            3     0  2.8  82    70 0   ACGAGUGGC-=gA===C===-UUGGAAACG\1"
            "ClfPerfr\1" "  ClfPerfr            3     0  3.2  86    74 0   CGGCGGGAC-=g==CU====-AACCUGCGG\1"
            "HllHalod\1" "  HllHalod            3     0  3.6  25    21 0   GAUCAAGUC-======Aa=C-GAUGGAAGC\1"
            "DlcTolu2\1" "  DlcTolu2            3     0  3.6  95    83 0   GGACUGCCC-==Aa==A===-CUAAUACCG\1"
            "FrhhPhil\1" "  FrhhPhil            3     0  4.0  25    21 0   GAUCAAGUC-==A====a=C-AGGUCUUCG\1"
            "AclPleur\1" "  AclPleur            3     0  4.0  29    24 0   GAUCAAGUC-==A====a=C-GGGAAGGGA\1"
            "ClnCorin\1" "  ClnCorin            3     0  4.1  25    21 0   GAUCAAGUC-=====A=G=A-GUUCCUUCG\1"
            "CltBotul\1" "  CltBotul            3     0  4.1  25    21 0   .AUCAAGUC-=====A=G=A-GCUUCUUCG\1"
            "CPPParap\1" "  CPPParap            3     0  4.1  25    21 0   GAUCAAGUC-=====A=G=A-GUUCCUUCG\1"
            "ClfPerfr\1" "  ClfPerfr            3     0  4.1  25    21 0   GAUCAAGUC-=====A=G=A-GUUUCCUUC\1"
            "DlcTolu2\1" "  DlcTolu2            3     0  4.1 157   143 0   GUAGCCGUU-===GAA====-CGGCUGGAU\1"
            "PslFlave\1" "  PslFlave            3     3  2.4  25    21 0   GAUCAAGUC-=======...-GGGAGCUUG\1"  // @@@ display is wrong (right seqpart should not be displayed)
            "PtVVVulg\1" "  PtVVVulg            3     3  2.4  29    24 0   GAUCAAGUC-=======...-GAGAAAGCU\1"; // @@@ same

        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
        arguments[2] = "matchmismatches=3"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd3);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GAGCGGUCAGGAG", // as above, but continues behind '...'
            NULL, // matchmismatches
            "matchweighted=1",            // use weighted mismatches
        };

        CCP expectd2 = "";
        CCP expectd3 = "    name---- fullname mis N_mis wmis pos ecoli rev          'GAGCGGUCAGGAG'\1"
            "ClnCorin\1" "  ClnCorin            6     0  3.4  77    66 0   AUGGAUUAG-Cg====A=g==CC-UUUCGAAAG\1"
            "CltBotul\1" "  CltBotul            6     0  3.4  77    66 0   GUGGAUUAG-Cg====A=g==CC-UUUCGAAAG\1"
            "CPPParap\1" "  CPPParap            6     0  3.4  77    66 0   ACGGAUUAG-Cg====A=g==CC-UUCCGAAAG\1"
            "BcSSSS00\1" "  BcSSSS00            3     0  3.4  25    21 0   GAUCAAGUC-======A===AU=-GGAGCUUGC\1";

        CCP expectd4 = "    name---- fullname mis N_mis wmis pos ecoli rev          'GAGCGGUCAGGAG'\1"
            "ClnCorin\1" "  ClnCorin            6     0  3.4  77    66 0   AUGGAUUAG-Cg====A=g==CC-UUUCGAAAG\1"
            "CltBotul\1" "  CltBotul            6     0  3.4  77    66 0   GUGGAUUAG-Cg====A=g==CC-UUUCGAAAG\1"
            "CPPParap\1" "  CPPParap            6     0  3.4  77    66 0   ACGGAUUAG-Cg====A=g==CC-UUCCGAAAG\1"
            "BcSSSS00\1" "  BcSSSS00            3     0  3.4  25    21 0   GAUCAAGUC-======A===AU=-GGAGCUUGC\1"
            "ClfPerfr\1" "  ClfPerfr            6     0  3.9 121   108 0   AUCAUAAUG-C====A=ug==gU-GAAGUCGUA\1"
            "ClnCorin\1" "  ClnCorin            6     0  3.9 122   109 0   CGCAUAAGA-C====A=ug==gU-GAAGUCGUA\1"
            "CltBotul\1" "  CltBotul            6     0  3.9 122   109 0   CUCAUAAGA-C====A=ug==gU-GAAGUCGUA\1"
            "CPPParap\1" "  CPPParap            6     0  3.9 122   109 0   GCAUAAGAU-C====A=ug==gU-AAGUCGUAA\1"
            "DcdNodos\1" "  DcdNodos            6     0  4.2  77    66 0   GUAACCUAG-Ug====A=g=AC=-UAUGGAAAC\1"
            "PsAAAA00\1" "  PsAAAA00            6     0  4.2  77    66 0   UGGAUUCAG-Cg====A=g=AC=-UCCGGAAAC\1"
            "PslFlave\1" "  PslFlave            6     0  4.2  77    66 0   CUGAUUCAG-Cg====A=g=AC=-UUUCGAAAG\1"
            "FrhhPhil\1" "  FrhhPhil            5     0  4.2 149   135 0   UAACAAUGG-U===C==ag===A-CCUGCGGCU\1"
            "AclPleur\1" "  AclPleur            4     0  4.3  29    24 0   GAUCAAGUC-==A====a=C=g=-AAGGGAGCU\1"
            "PbcAcet2\1" "  PbcAcet2            6     0  4.3 149   135 0   GUAACAAGG-U===C==ag==gA-ACCUGCGGC\1"
            "PbrPropi\1" "  PbrPropi            6     0  4.3 149   135 0   GUAACAAGG-U===C==ag==gA-ACCUGCGGC\1"
            "Stsssola\1" "  Stsssola            6     0  4.3 149   135 0   GUAACAAGG-U===C==ag==gA-ACCUGCGGC\1"
            "LgtLytic\1" "  LgtLytic            6     0  4.3 149   135 0   GUAACAAGG-U===C==ag==gA-ACCUGCGGC\1"
            "PslFlave\1" "  PslFlave            6     0  4.3 149   135 0   GUAACAAGG-U===C==ag==gA-ACCUGCGGC\1"
            "HllHalod\1" "  HllHalod            6     0  4.3 149   135 0   GUAACAAGG-U===C==ag==gA-ACCUGCGGC\1"
            "VbrFurni\1" "  VbrFurni            5     0  4.4  68    60 0   GGAUUUGUU-=g====CG==Cg=-CGGACGGAC\1"
            "AclPleur\1" "  AclPleur            6     0  4.4  35    30 0   GUCGAACGG-U=A===ga===gA-GCUUGCUUU\1"
            "PslFlave\1" "  PslFlave            6     6  4.4  25    21 0   GAUCAAGUC-=======......-AGCUUGCUC\1" // @@@ display is wrong (right seqpart should not be displayed)
            "PtVVVulg\1" "  PtVVVulg            6     6  4.4  29    24 0   GAUCAAGUC-=======......-AAAGCUUGC\1" // @@@ same
            "VbrFurni\1" "  VbrFurni            6     0  4.4 149   135 0   GUAACAAGG-U====C=ag==gA-ACCUGGCGC\1"
            "VblVulni\1" "  VblVulni            6     0  4.4 149   135 0   GUAACAAGG-U====C=ag==gA-ACCUGGCGC\1"
            "VbhChole\1" "  VbhChole            6     0  4.4 149   135 0   GUAACAAGG-U====C=ag==gA-ACCUGGCGC\1";

        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
        arguments[2] = "matchmismatches=3"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd3);
        arguments[2] = "matchmismatches=4"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd4);
    }

    // --------------------------
    //      truncate results

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=CANCNCNNUNC", // contains 5N
            NULL, // matchmismatches
            "matchacceptN=5",
            "matchlimitN=7",
            "matchmaxresults=5",
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCNCNNUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     5  4.2 176   162 0   CGGCUGGAU-==C=U=CU=U=-U.\1";

        // many hits are truncated here: 
        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCNCNNUNC'\1"
            "DlcTolu2\1" "  DlcTolu2            1     6  4.9 176   162 0   CGGCUGGAU-==C=U=CU==N-N.\1"
            "FrhhPhil\1" "  FrhhPhil            1     6  4.9 176   162 0   CGGCUGGAU-==C=U=CU=..-\1"
            "HllHalod\1" "  HllHalod            1     6  4.9 176   162 0   CGGCUGGAU-==C=U=CU=..-\1"
            "CPPParap\1" "  CPPParap            1     6  4.9 177   163 0   CGGNUGGAU-==C=U=CU=..-\1"
            "AclPleur\1" "  AclPleur            1     6  5.0 176   162 0   CGGUUGGAU-==C=U=CU=A.-\1";
        
        // many hits are truncated here: 
        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCNCNNUNC'\1"
            "HllHalod\1" "  HllHalod            2     5  5.1  45    40 0   AAACGAUGG-a=G=UuGC=U=-CAGGCGUCG\1"
            "VblVulni\1" "  VblVulni            2     5  5.4  49    44 0   AGCACAGAG-a=A=UuGU=U=-UCGGGUGGC\1"
            "VbrFurni\1" "  VbrFurni            2     5  5.7  40    35 0   CGGCAGCGA-==A=AuUGAA=-CUUCGGGGG\1"
            "LgtLytic\1" "  LgtLytic            2     5  6.2 101    89 0   GGGGAAACU-==AGCuAA=A=-CGCAUAAUC\1"
            "ClfPerfr\1" "  ClfPerfr            2     5  6.5 172   158 0   AGGAAGAUU-a=UaC=CC=C=-UUUCU.\1";

        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }
}

static char *extract_locations(const char *probe_design_result) {
    const char *Target = strstr(probe_design_result, "\nTarget");
    if (Target) {
        const char *designed = strchr(Target+7, '\n');
        if (designed) {
            ++designed;
            char *result = strdup("");

            while (designed) {
                const char *eol       = strchr(designed, '\n');
                const char *space1    = strchr(designed, ' ');          if (!space1) break; // 1st space between probe and len
                const char *nonspace  = space1+strspn(space1, " ");     if (!nonspace) break;
                const char *space2    = strchr(nonspace, ' ');          if (!space2) break; // 1st space between len and "X="
                const char *nonspace2 = space2+strspn(space2, " ");     if (!nonspace2) break;
                const char *space3    = strchr(nonspace2, ' ');         if (!space3) break; // 1st space between "X=" and abs
                const char *nonspace3 = space3+strspn(space3, " ");     if (!nonspace3) break;
                const char *space4    = strchr(nonspace3, ' ');         if (!space4) break; // 1st space between abs and rest

                char *abs = GB_strpartdup(nonspace3, space4-1);

                freeset(result, GBS_global_string_copy("%s%c%c%s", result, space2[1], space2[2], abs));
                free(abs);

                designed = eol ? eol+1 : NULL;
            }

            return result;
        }
    }
    return strdup("can't extract");
}

void TEST_SLOW_design_probe() {
    // test here runs versus database ../UNIT_TESTER/run/TEST_pt_src.arb

    bool use_gene_ptserver = false;
    {
        const char *arguments[] = {
            "prgnamefake",
            "designnames=ClnCorin#CltBotul#CPPParap#ClfPerfr",
            "designmintargets=100",
        };
        const char *expected =
            "Probe design Parameters:\n"
            "Length of probe      18\n"
            "Temperature        [ 0.0 -400.0]\n"
            "GC-Content         [30.0 -80.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits     0\n"
            "Min Group Hits       100%\n"
            "Target             le     apos ecol qual grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species\n"
            "AUCAAGUCGAGCGAUGAA 18 A=    17   16   77    4 44.4    52.0 UUCAUCGCUCGACUUGAU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGAAAGGAAGAUUAAUAC 18 B=    94   82   77    4 33.3    48.0 GUAUUAAUCUUCCUUUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GAAAGGAAGAUUAAUACC 18 B+     1   83   77    4 33.3    48.0 GGUAUUAAUCUUCCUUUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UCAAGUCGAGCGAUGAAG 18 A+     1   17   77    4 50.0    54.0 CUUCAUCGCUCGACUUGA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);

        // test extraction of positions:
        {
            char *positions = extract_locations(expected);
            TEST_EXPECT_EQUAL(positions, "A=17B=94B+1A+1");
            free(positions);
        }
    }

    // test vs bug (fails with [8988] .. [9175])
    {
        const char *arguments[] = {
            "prgnamefake",
            "designnames=VbhChole#VblVulni",
            "designmintargets=50", // hit at least 1 of the 2 targets
            "designmingc=60", "designmaxgc=75", // specific GC range
        };

        const char *expected =
            "Probe design Parameters:\n"
            "Length of probe      18\n"
            "Temperature        [ 0.0 -400.0]\n"
            "GC-Content         [60.0 -75.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits     0\n"
            "Min Group Hits        50%\n"
            "Target             le     apos ecol qual grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species\n"
            "AAGUCGAGCGGCAGCACA 18 A=    20   19   25    2 61.1    58.0 UGUGCUGCCGCUCGACUU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "AGCGGCAGCACAGAGGAA 18 A+     6   21   20    1 61.1    58.0 UUCCUCUGUGCUGCCGCU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "AGUCGAGCGGCAGCACAG 18 A+     1   20   39    2 66.7    60.0 CUGUGCUGCCGCUCGACU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CAAGUCGAGCGGCAGCAC 18 A-     1   18   13    2 66.7    60.0 GUGCUGCCGCUCGACUUG |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "CGAGCGGCAGCACAGAGA 18 A+     4   21   20    1 66.7    60.0 UCUCUGUGCUGCCGCUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGAGCGGCAGCACAGAGG 18 A+     4   21   20    1 72.2    62.0 CCUCUGUGCUGCCGCUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGGCAGCACAGAGGAACU 18 A+     8   23   20    1 61.1    58.0 AGUUCCUCUGUGCUGCCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CUUGUUCCUUGGGUGGCG 18 B=    52   47   20    1 61.1    58.0 CGCCACCCAAGGAACAAG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CUUGUUUCUCGGGUGGCG 18 B-     0   47   20    1 61.1    58.0 CGCCACCCGAGAAACAAG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GAGCGGCAGCACAGAGAA 18 A+     5   21   20    1 61.1    58.0 UUCUCUGUGCUGCCGCUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GAGCGGCAGCACAGAGGA 18 A+     5   21   20    1 66.7    60.0 UCCUCUGUGCUGCCGCUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GCGGCAGCACAGAGAAAC 18 A+     7   22   20    1 61.1    58.0 GUUUCUCUGUGCUGCCGC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GCGGCAGCACAGAGGAAC 18 A+     7   22   20    1 66.7    60.0 GUUCCUCUGUGCUGCCGC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GUCGAGCGGCAGCACAGA 18 A+     2   21   39    2 66.7    60.0 UCUGUGCUGCCGCUCGAC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GUUCCUUGGGUGGCGAGC 18 B+     3   50   20    1 66.7    60.0 GCUCGCCACCCAAGGAAC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GUUUCUCGGGUGGCGAGC 18 B+     3   50   20    1 66.7    60.0 GCUCGCCACCCGAGAAAC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UCAAGUCGAGCGGCAGCA 18 A-     2   17    3    2 61.1    58.0 UGCUGCCGCUCGACUUGA |  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "UCCUUGGGUGGCGAGCGG 18 B+    11   58   20    1 72.2    62.0 CCGCUCGCCACCCAAGGA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UCGAGCGGCAGCACAGAG 18 A+     3   21   39    2 66.7    60.0 CUCUGUGCUGCCGCUCGA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UGUUCCUUGGGUGGCGAG 18 B+     2   49   20    1 61.1    58.0 CUCGCCACCCAAGGAACA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UGUUUCUCGGGUGGCGAG 18 B+     2   49   20    1 61.1    58.0 CUCGCCACCCGAGAAACA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UUCCUUGGGUGGCGAGCG 18 B+    10   57   20    1 66.7    60.0 CGCUCGCCACCCAAGGAA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UUCUCGGGUGGCGAGCGG 18 B+    11   58   20    1 72.2    62.0 CCGCUCGCCACCCGAGAA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }

    // test error case
    {
        const char *arguments_bad_design[] = {
            "prgnamefake",
            "designnames=CPPParap#PsAAAA00",
            "designprobelength=3",
        };
        const char *expected_error = "Probe length 3 is below the minimum probe length of 8";

        TEST_RUN_ARB_PROBE__REPORTS_ERROR(ARRAY_ELEMS(arguments_bad_design), arguments_bad_design, expected_error);
    }

    // design MANY probes to test location specifier
    {
        const char *arguments_loc[] = {
            "prgnamefake",
            // "designnames=Stsssola#Stsssola", // @@@ crashes the ptserver
            "designnames=CPPParap#PsAAAA00",
            "designmintargets=50", // hit at least 1 of the 2 targets
            "designmingc=0", "designmaxgc=100", // allow all GCs
            "designmintemp=30", "designmaxtemp=100", // allow all temp above 30 deg
            "designmishit=7",  // allow enough outgroup hits
            "designmaxhits=99999", // do not limit results
            "designprobelength=9",
        };

        const char *expected_loc =
            "A=96A+1B=84B-0A+2A-8C=63D=111A-4E=153F=51G=24G+4H=125D+1A-3B-4B+1C-8B+1A+3A-7A-1I=37B-1"
            "B-1B+3G+5E-1A-8H-1B-5F+1F-2A-2B-2B-2B+2F-5B+3C-7F-6B+2A+4G-2J=74C-1F-1G-1F+3F-0J-1C-2";

        TEST_ARB_PROBE_FILT(ARRAY_ELEMS(arguments_loc), arguments_loc, extract_locations, expected_loc);
    }

    // same as above
    {
        const char *arguments[] = {
            "prgnamefake",
            "designnames=CPPParap#PsAAAA00",
            "designmintargets=50", // hit at least 1 of the 2 targets
            "designmingc=0", "designmaxgc=100", // allow all GCs
            "designmintemp=30", "designmaxtemp=100", // allow all temp above 30 deg
            "designmishit=7",  // allow enough outgroup hits
            "designmaxhits=99999", // do not limit results
            "designprobelength=9",
        };

        const char *expected =
            "Probe design Parameters:\n"
            "Length of probe       9\n"
            "Temperature        [30.0 -100.0]\n"
            "GC-Content         [ 0.0 -100.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits     7\n"
            "Min Group Hits        50%\n"
            "Target    le     apos ecol qual grps  G+C 4GC+2AT Probe sequence | Decrease T by n*.3C -> probe matches n non group species\n"
            "AAACGGGCG  9 A=    96   84    9    1 66.7    30.0 CGCCCGUUU |  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "AACGGGCGC  9 A+     1   85   14    1 77.8    32.0 GCGCCCGUU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,\n"
            "ACGGACGUC  9 B=    84   72    7    1 66.7    30.0 GACGUCCGU |  0,  0,  0,  0,  0,  0,  2,  2,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5, 13,\n"
            "ACGGGCCUU  9 B-     0   72   20    1 66.7    30.0 AAGGCCCGU |  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "ACGGGCGCU  9 A+     2   86   12    1 77.8    32.0 AGCGCCCGU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,\n"
            "ACGUCCGGA  9 A-     8   76   14    1 66.7    30.0 UCCGGACGU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1, 11, 11, 12, 12, 17,\n"
            "CAGCGGCGG  9 C=    63   58    7    1 88.9    34.0 CCGCCGCUG |  2,  2,  2,  2,  2,  2,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,\n"
            "CCGCAUACG  9 D=   111   99   20    1 66.7    30.0 CGUAUGCGG |  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,  5,  5,  5,  5,  5,\n"
            "CCGGAAACG  9 A-     4   80    7    1 66.7    30.0 CGUUUCCGG |  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  2,  2,  2,  6,  6,  8,  8,  9,  9,  9,\n"
            "CCGUAGGAG  9 E=   153  139    3    1 66.7    30.0 CUCCUACGG |  1,  1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,\n"
            "CCUUCGGGA  9 F=    51   46   20    1 66.7    30.0 UCCCGAAGG |  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  5,  5, 13,\n"
            "CGAGCGAUG  9 G=    24   21   20    1 66.7    30.0 CAUCGCUCG |  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "CGAGCGGAU  9 G+     4   23   20    1 66.7    30.0 AUCCGCUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGAUUGGGG  9 H=   125  112   20    1 66.7    30.0 CCCCAAUCG |  3,  3,  3,  3,  3,  3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,\n"
            "CGCAUACGC  9 D+     1  100   20    1 66.7    30.0 GCGUAUGCG |  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,\n"
            "CGGAAACGG  9 A-     3   81    7    1 66.7    30.0 CCGUUUCCG |  0,  0,  0,  0,  0,  0,  2,  2,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,\n"
            "CGGACGGGC  9 B-     4   69   20    1 88.9    34.0 GCCCGUCCG |  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,\n"
            "CGGACGUCC  9 B+     1   73    7    1 77.8    32.0 GGACGUCCG |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  3,  3,  3,  3, 12, 12, 12,\n"
            "CGGGAACGG  9 C-     8   50    7    1 77.8    32.0 CCGUUCCCG |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "CGGGCCUUC  9 B+     1   73    7    1 77.8    32.0 GAAGGCCCG |  1,  1,  1,  1,  1,  1,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "CGGGCGCUA  9 A+     3   87   20    1 77.8    32.0 UAGCGCCCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGUCCGGAA  9 A-     7   77   14    1 66.7    30.0 UUCCGGACG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,\n"
            "GAAACGGGC  9 A-     1   83    5    1 66.7    30.0 GCCCGUUUC |  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "GAAGGGAGC  9 I=    37   32   20    1 66.7    30.0 GCUCCCUUC |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "GACGGACGU  9 B-     1   71    9    1 66.7    30.0 ACGUCCGUC |  2,  2,  2,  2,  2,  2,  2,  2, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14,\n"
            "GACGGGCCU  9 B-     1   71   20    1 77.8    32.0 AGGCCCGUC |  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "GACGUCCGG  9 B+     3   75   14    1 77.8    32.0 CCGGACGUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  2, 14,\n"
            "GAGCGGAUG  9 G+     5   24   20    1 66.7    30.0 CAUCCGCUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GCCGUAGGA  9 E-     1  138    3    1 66.7    30.0 UCCUACGGC |  1,  1,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,\n"
            "GCCUUCCGA  9 A-     8   76    7    1 66.7    30.0 UCGGAAGGC |  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "GCGAUUGGG  9 H-     1  111   20    1 66.7    30.0 CCCAAUCGC |  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "GCGGACGGG  9 B-     5   68   20    1 88.9    34.0 CCCGUCCGC |  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,\n"
            "GCUCCUGGA  9 F+     1   47   20    1 66.7    30.0 UCCAGGAGC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GCUUGCUCC  9 F-     2   44   20    1 66.7    30.0 GGAGCAAGC |  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  7,  7,  7,  7,  8,  8,  8,\n"
            "GGAAACGGG  9 A-     2   82   20    1 66.7    30.0 CCCGUUUCC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GGACGGACG  9 B-     2   70   20    1 77.8    32.0 CGUCCGUCC |  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,\n"
            "GGACGGGCC  9 B-     2   70   20    1 88.9    34.0 GGCCCGUCC |  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "GGACGUCCG  9 B+     2   74   14    1 77.8    32.0 CGGACGUCC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  3,  3,  3,\n"
            "GGAGCUUGC  9 F-     5   41   20    1 66.7    30.0 GCAAGCUCC |  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,\n"
            "GGCCUUCCG  9 B+     3   75    7    1 77.8    32.0 CGGAAGGCC |  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "GGGAACGGA  9 C-     7   51    7    1 66.7    30.0 UCCGUUCCC |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "GGGAGCUUG  9 F-     6   40   20    1 66.7    30.0 CAAGCUCCC |  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,\n"
            "GGGCCUUCC  9 B+     2   74    7    1 77.8    32.0 GGAAGGCCC |  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,\n"
            "GGGCGCUAA  9 A+     4   88    7    1 66.7    30.0 UUAGCGCCC |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "GUCGAGCGA  9 G-     2   21    3    1 66.7    30.0 UCGCUCGAC |  3,  3, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 14,\n"
            "UAGCGGCGG  9 J=    74   63    7    1 77.8    32.0 CCGCCGCUA |  4,  4,  4,  4,  4,  4, 10, 10, 10, 10, 10, 10, 10, 12, 12, 12, 12, 12, 12, 12,\n"
            "UCAGCGGCG  9 C-     1   57    7    1 77.8    32.0 CGCCGCUGA |  2,  2,  2,  2,  2,  2,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,\n"
            "UCCUUCGGG  9 F-     1   45   20    1 66.7    30.0 CCCGAAGGA |  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,\n"
            "UCGAGCGGA  9 G-     1   21   20    1 66.7    30.0 UCCGCUCGA |  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,\n"
            "UCGGGAACG  9 F+     3   49    7    1 66.7    30.0 CGUUCCCGA |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "UGCUCCUGG  9 F-     0   46   20    1 66.7    30.0 CCAGGAGCA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "UUAGCGGCG  9 J-     1   62   20    1 66.7    30.0 CGCCGCUAA |  4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,\n"
            "UUCAGCGGC  9 C-     2   56   20    1 66.7    30.0 GCCGCUGAA |  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,\n";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }

    // same as above (with probelen == 8)
    {
        const char *arguments[] = {
            "prgnamefake",
            "designnames=CPPParap#PsAAAA00",
            "designmintargets=50", // hit at least 1 of the 2 targets
            "designmingc=0", "designmaxgc=100", // allow all GCs
            "designmintemp=30", "designmaxtemp=100", // allow all temp above 30 deg
            // "designmishit=7",
            "designmishit=15", // @@@ reports more results than with 7 mishits, but no mishits reported below!
            "designmaxhits=99999", // do not limit results
            "designprobelength=8",
        };

        const char *expected =
            "Probe design Parameters:\n"
            "Length of probe       8\n"
            "Temperature        [30.0 -100.0]\n"
            "GC-Content         [ 0.0 -100.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits    15\n"
            "Min Group Hits        50%\n"
            "Target   le     apos ecol qual grps  G+C 4GC+2AT Probe sequence | Decrease T by n*.3C -> probe matches n non group species\n"
            "ACGGGCGC  8 A=    98   86   20    1 87.5    30.0 GCGCCCGU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n" // @@@ outgroup hit detection obviously fails for probelength 8!
            "AGCGGCGG  8 B=    75   64   39    2 87.5    30.0 CCGCCGCU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CAGCGGCG  8 C=    63   58   20    1 87.5    30.0 CGCCGCUG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGGACGGG  8 B+     5   69   20    1 87.5    30.0 CCCGUCCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGGCGGAC  8 B+     2   66   39    2 87.5    30.0 GUCCGCCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "CGGGCGCU  8 A+     1   87   20    1 87.5    30.0 AGCGCCCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GACGGGCC  8 D=    83   71   20    1 87.5    30.0 GGCCCGUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GCGGACGG  8 B+     4   68   39    2 87.5    30.0 CCGUCCGC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GCGGCGGA  8 B+     1   65   39    2 87.5    30.0 UCCGCCGC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GGACGGGC  8 B+     7   70   20    1 87.5    30.0 GCCCGUCC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
            "GGCGGACG  8 B+     3   67   39    2 87.5    30.0 CGUCCGCC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "designnames=ClnCorin#CPPParap#ClfPerfr",
            "designprobelength=16",
            "designmintargets=100",
            "designmishit=2",
        };
        const char *expected =
            "Probe design Parameters:\n"
            "Length of probe      16\n"
            "Temperature        [ 0.0 -400.0]\n"
            "GC-Content         [30.0 -80.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits     2\n"
            "Min Group Hits       100%\n"
            "Target           le     apos ecol qual grps  G+C 4GC+2AT Probe sequence   | Decrease T by n*.3C -> probe matches n non group species\n"
            "AAGGAAGAUUAAUACC 16 A=    97   85   58    3 31.2    42.0 GGUAUUAAUCUUCCUU |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "AAGUCGAGCGAUGAAG 16 B=    20   19   58    3 50.0    48.0 CUUCAUCGCUCGACUU |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "AGUCGAGCGAUGAAGU 16 B+     1   20   19    3 50.0    48.0 ACUUCAUCGCUCGACU |  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "AUCAAGUCGAGCGAUG 16 B-     3   16   52    3 50.0    48.0 CAUCGCUCGACUUGAU |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  8,  8,  8,\n"
            "CAAGUCGAGCGAUGAA 16 B-     1   18   58    3 50.0    48.0 UUCAUCGCUCGACUUG |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "CGAAAGGAAGAUUAAU 16 A-     3   82   58    3 31.2    42.0 AUUAAUCUUCCUUUCG |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "GAUCAAGUCGAGCGAU 16 B-     4   15    4    3 50.0    48.0 AUCGCUCGACUUGAUC |  0,  2,  2,  2,  2,  3,  3,  3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,\n"
            "GUCGAGCGAUGAAGUU 16 B+     2   21   31    3 50.0    48.0 AACUUCAUCGCUCGAC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "UCAAGUCGAGCGAUGA 16 B-     2   17   58    3 50.0    48.0 UCAUCGCUCGACUUGA |  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\n"
            "UGAUCAAGUCGAGCGA 16 B-     5   14    4    3 50.0    48.0 UCGCUCGACUUGAUCA |  0,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,\n";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "designnames=HllHalod#VbhChole#VblVulni#VbrFurni#PtVVVulg",
            "designprobelength=14",
            "designmintargets=100",
            "designmishit=4",
            "designmingc=70",
            "designmaxgc=100",
        };
        const char *expected =
            "Probe design Parameters:\n"
            "Length of probe      14\n"
            "Temperature        [ 0.0 -400.0]\n"
            "GC-Content         [70.0 -100.0]\n"
            "E.Coli Position    [any]\n"
            "Max Non Group Hits     4\n"
            "Min Group Hits       100%\n"
            "Target         le     apos ecol qual grps  G+C 4GC+2AT Probe sequence | Decrease T by n*.3C -> probe matches n non group species\n"
            "AGCGGCGGACGGAC 14 A=    75   64   96    5 78.6    50.0 GUCCGUCCGCCGCU |  4,  7,  7,  7,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10,\n"
            "CGAGCGGCGGACGG 14 A-     1   63   21    5 85.7    52.0 CCGUCCGCCGCUCG |  0,  0,  0,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  9,  9,  9,  9,  9,  9,  9,\n"
            "GAGCGGCGGACGGA 14 A-     0   64   21    5 78.6    50.0 UCCGUCCGCCGCUC |  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  5,  5,  9,  9, 10, 10, 10, 10, 10, 10,\n";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
}

void TEST_SLOW_match_designed_probe() {
    // test here runs versus database ../UNIT_TESTER/run/TEST_pt_src.arb

    bool use_gene_ptserver = false;
    const char *arguments[] = {
        "prgnamefake",
        "matchsequence=UCAAGUCGAGCGAUGAAG",
    };
    CCP expected = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCAAGUCGAGCGAUGAAG'\1"
        "ClnCorin\1" "  ClnCorin            0     0  0.0  18    17 0   .GAGUUUGA-==================-UUCCUUCGG\1"
        "CltBotul\1" "  CltBotul            0     0  0.0  18    17 0   ........A-==================-CUUCUUCGG\1"
        "CPPParap\1" "  CPPParap            0     0  0.0  18    17 0   AGAGUUUGA-==================-UUCCUUCGG\1"
        "ClfPerfr\1" "  ClfPerfr            0     0  0.0  18    17 0   AGAGUUUGA-==================-UUUCCUUCG\1";

    TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
}

void TEST_SLOW_get_existing_probes() {
    // test here runs versus database ../UNIT_TESTER/run/TEST_pt_src.arb

    bool use_gene_ptserver = false;
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=20",
            "iterate_amount=10",
        };
        CCP expected =
            "AAACCGGGGCTAATACCGGA;AAACGACTGTTAATACCGCA;AAACGATGGAAGCTTGCTTC;AAACGATGGCTAATACCGCA;AAACGGATTAGCGGCGGGAC;"
            "AAACGGGCGCTAATACCGCA;AAACGGTCGCTAATACCGGA;AAACGGTGGCTAATACCGCA;AAACGTACGCTAATACCGCA;AAACTCAAGCTAATACCGCA";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=15",
            "iterate_amount=20",
            "iterate_separator=:",
        };
        CCP expected =
            "AAACCGGGGCTAATA:AAACGACTGTTAATA:AAACGATGGAAGCTT:AAACGATGGCTAATA:AAACGGATTAGCGGC:AAACGGGCGCTAATA:AAACGGTCGCTAATA:"
            "AAACGGTGGCTAATA:AAACGTACGCTAATA:AAACTCAAGCTAATA:AAACTCAGGCTAATA:AAACTGGAGAGTTTG:AAACTGTAGCTAATA:AAACTTGTTTCTCGG:"
            "AAAGAGGTGCTAATA:AAAGCTTGCTTTCTT:AAAGGAACGCTAATA:AAAGGAAGATTAATA:AAAGGACAGCTAATA:AAAGGGACTTCGGTC";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=10",
            "iterate_amount=5",
            "iterate_readable=0",
        };
        CCP expected =
            "\2\2\2\3\3\4\4\4\4\3;\2\2\2\3\4\2\3\5\4\5;\2\2\2\3\4\2\5\4\4\2;\2\2\2\3\4\2\5\4\4\3;\2\2\2\3\4\4\2\5\5\2";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=3",
            "iterate_amount=70",
            "iterate_tu=U",
        };
        CCP expected =
            "AAA;AAC;AAG;AAU;ACA;ACC;ACG;ACU;AGA;AGC;AGG;AGU;AUA;AUC;AUG;AUU;"
            "CAA;CAC;CAG;CAU;CCA;CCC;CCG;CCU;CGA;CGC;CGG;CGU;CUA;CUC;CUG;CUU;"
            "GAA;GAC;GAG;GAU;GCA;GCC;GCG;GCU;GGA;GGC;GGG;GGU;GUA;GUC;GUG;GUU;"
            "UAA;UAC;UAG;UAU;UCA;UCC;UCG;UCU;UGA;UGC;UGG;UGU;UUA;UUC;UUG;UUU";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=2",
            "iterate_amount=20",
        };
        CCP expected =
            "AA;AC;AG;AT;"
            "CA;CC;CG;CT;"
            "GA;GC;GG;GT;"
            "TA;TC;TG;TT";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
}

// #define TEST_AUTO_UPDATE // uncomment to auto-update expected index dumps

void TEST_index_dump() {
    for (int use_gene_ptserver = 0; use_gene_ptserver <= 1; use_gene_ptserver++) {
        const char *dumpfile     = use_gene_ptserver ? "index_gpt.dump" : "index_pt.dump";
        char       *dumpfile_exp = GBS_global_string_copy("%s.expected", dumpfile);

        const char *arguments[] = {
            "prgnamefake",
            GBS_global_string_copy("dump=%s", dumpfile),
        };
        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, "ok");

#if defined(TEST_AUTO_UPDATE)
        TEST_COPY_FILE(dumpfile, dumpfile_exp);
#else // !defined(TEST_AUTO_UPDATE)
        TEST_EXPECT_TEXTFILES_EQUAL(dumpfile_exp, dumpfile);
#endif
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(dumpfile));

        free((char*)arguments[1]);
        free(dumpfile_exp);
    }
}

#undef TEST_AUTO_UPDATE

// --------------------------------------------------------------------------------

#include <arb_strarray.h>
#include <set>
#include <iterator>

using namespace std;

typedef set<string> Matches;

inline void parseMatches(char*& answer, Matches& matches) {
    ConstStrArray match_results;

    GBT_splitNdestroy_string(match_results, answer, "\1", false);

    for (size_t i = 1; i<match_results.size(); i += 2) {
        if (match_results[i][0]) {
            matches.insert(match_results[i]);
        }
    }
}

inline void getMatches(const char *probe, Matches& matches) {
    bool  use_gene_ptserver = false;
    char *matchseq          = GBS_global_string_copy("matchsequence=%s", probe);
    const char *arguments[] = {
        "prgnamefake",
        matchseq,
    };
    TEST_RUN_ARB_PROBE(ARRAY_ELEMS(arguments), arguments);
    parseMatches(answer, matches);
    free(matchseq);
}

inline string matches2hitlist(const Matches& matches) {
    string hitlist;
    if (!matches.empty()) {
        for (Matches::const_iterator m = matches.begin(); m != matches.end(); ++m) {
            hitlist = hitlist + ',' + *m;
        }
        hitlist = hitlist.substr(1);
    }
    return hitlist;
}

inline void extract_first_but_not_second(const Matches& first, const Matches& second, Matches& result) {
    set_difference(first.begin(), first.end(),
                   second.begin(), second.end(),
                   inserter(result, result.begin()));
}

// --------------------------------------------------------------------------------

// #define TEST_INDEX_COMPLETENESS // only uncomment temporarily (slow as hell)

#if defined(TEST_INDEX_COMPLETENESS)

void TEST_SLOW_find_unmatched_probes() {
    bool use_gene_ptserver = false;

    // get all 20mers indexed in ptserver:
    ConstStrArray fullProbes;
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=20",
            "iterate_amount=1000000",
            "iterate_tu=U",
        };
        TEST_RUN_ARB_PROBE(ARRAY_ELEMS(arguments), arguments);

        GBT_splitNdestroy_string(fullProbes, answer, ";", false);
        TEST_EXPECT_EQUAL(fullProbes.size(), 2040);
    }

    for (size_t lp = 0; lp<fullProbes.size(); ++lp) { // with all 20mers existing in ptserver
        const char *fullProbe = fullProbes[lp];

        size_t fullLen = strlen(fullProbe);
        TEST_EXPECT_EQUAL(fullLen, 20);

        Matches fullHits;
        getMatches(fullProbe, fullHits);
        TEST_EXPECT(fullHits.size()>0);

        bool fewerHitsSeen = false;

        for (size_t subLen = fullLen-1; !fewerHitsSeen && subLen >= 10; --subLen) { // for each partial sub-probe of 20mer
            char subProbe[20];
            subProbe[subLen] = 0;

            for (size_t pos = 0; pos+subLen <= fullLen; ++pos) {
                Matches subHits, onlyFull;

                memcpy(subProbe, fullProbe+pos, subLen);
                getMatches(subProbe, subHits);
                extract_first_but_not_second(fullHits, subHits, onlyFull);

                if (!onlyFull.empty()) {
                    fprintf(stderr, "TEST_PARTIAL_COVERS_FULL_PROBE__BROKEN(\"%s\", \"%s\");\n", subProbe, fullProbe);
                    fewerHitsSeen = true; // only list first wrong hit for each fullProbe
                }
            }
        }
    }
}

#endif
// --------------------------------------------------------------------------------

static arb_test::match_expectation partial_covers_full_probe(const char *part, const char *full) {
    using namespace arb_test;
    using namespace std;
    
    expectation_group expected;
    expected.add(that(strstr(full, part)).does_differ_from_NULL());
    expected.add(that(strlen(part)).is_less_than(strlen(full)));

    {
        Matches matchFull, matchPart;
        getMatches(full,  matchFull);
        getMatches(part, matchPart);

        expected.add(that(matchFull.empty()).is_equal_to(false));

        Matches onlyFirst;
        extract_first_but_not_second(matchFull, matchPart, onlyFirst);

        string only_hit_by_full = matches2hitlist(onlyFirst);
        expected.add(that(only_hit_by_full).is_equal_to(""));
    }


    return all().ofgroup(expected);
}

#define TEST_PARTIAL_COVERS_FULL_PROBE(part,full)         TEST_EXPECTATION(partial_covers_full_probe(part, full))
#define TEST_PARTIAL_COVERS_FULL_PROBE__BROKEN(part,full) TEST_EXPECTATION__BROKEN(partial_covers_full_probe(part, full))

void TEST_SLOW_unmatched_probes() {
    TEST_PARTIAL_COVERS_FULL_PROBE("CCUCCUUUCU",          "GAUUAAUACCCCUCCUUUCU");

    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUC",          "GGGGAACCUGCGGUUGGAUC");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCA",         "GGGAACCUGCGGUUGGAUCA");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCAC",        "GGAACCUGCGGUUGGAUCAC");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACC",       "GAACCUGCGGUUGGAUCACC");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACCU",      "AACCUGCGGUUGGAUCACCU");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACCUC",     "ACCUGCGGUUGGAUCACCUC");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACCUCC",    "CCUGCGGUUGGAUCACCUCC");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACCUCCU",   "CUGCGGUUGGAUCACCUCCU");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACCUCCUU",  "UGCGGUUGGAUCACCUCCUU");
    TEST_PARTIAL_COVERS_FULL_PROBE("CGGUUGGAUCACCUCCUUA", "GCGGUUGGAUCACCUCCUUA");

    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUC",          "GGGGAACCUGGCGCUGGAUC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCA",         "GGGAACCUGGCGCUGGAUCA");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCAC",        "GGAACCUGGCGCUGGAUCAC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACC",       "GAACCUGGCGCUGGAUCACC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACCU",      "AACCUGGCGCUGGAUCACCU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACCUC",     "ACCUGGCGCUGGAUCACCUC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACCUCC",    "CCUGGCGCUGGAUCACCUCC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACCUCCU",   "CUGGCGCUGGAUCACCUCCU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACCUCCUU",  "UGGCGCUGGAUCACCUCCUU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCGCUGGAUCACCUCCUUU", "GGCGCUGGAUCACCUCCUUU");

    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCAC",         "GGAACCUGCGGCUGGAUCAC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACC",        "GAACCUGCGGCUGGAUCACC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCU",       "AACCUGCGGCUGGAUCACCU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCUC",      "ACCUGCGGCUGGAUCACCUC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCUCC",     "CCUGCGGCUGGAUCACCUCC");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCUCCU",    "CUGCGGCUGGAUCACCUCCU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCUCCUU",   "UGCGGCUGGAUCACCUCCUU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCUCCUUU",  "GCGGCUGGAUCACCUCCUUU");
    TEST_PARTIAL_COVERS_FULL_PROBE("GCUGGAUCACCUCCUUUC", "CGGCUGGAUCACCUCCUUUC");

    // the data of the following tests is located right in front of the dots inserted in [8962]
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAG",          "AAUUGAAGAGUUUGAUCAAG");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGU",         "AUUGAAGAGUUUGAUCAAGU");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUC",        "UUGAAGAGUUUGAUCAAGUC");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCG",       "UGAAGAGUUUGAUCAAGUCG");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCGA",      "GAAGAGUUUGAUCAAGUCGA");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCGAG",     "AAGAGUUUGAUCAAGUCGAG");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCGAGC",    "AGAGUUUGAUCAAGUCGAGC");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCGAGCG",   "GAGUUUGAUCAAGUCGAGCG");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCGAGCGG",  "AGUUUGAUCAAGUCGAGCGG");
    TEST_PARTIAL_COVERS_FULL_PROBE("UUUGAUCAAGUCGAGCGGU", "GUUUGAUCAAGUCGAGCGGU");
}

void TEST_SLOW_variable_defaults_in_server() {
    test_setup(false);

    const char *server_tag = GBS_ptserver_tag(TEST_SERVER_ID);
    TEST_EXPECT_NO_ERROR(arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag));

    const char *servername = GBS_read_arb_tcp(server_tag);;
    TEST_EXPECT_CONTAINS(servername, "/UNIT_TESTER/sockets/ptserver.socket"); // as defined in ../lib/arb_tcp.dat@ARB_TEST_PT_SERVER

    T_PT_MAIN  com;
    T_PT_LOCS  locs;
    GB_ERROR   error = NULL;
    aisc_com  *link  = aisc_open(servername, com, AISC_MAGIC_NUMBER, &error);
    TEST_EXPECT_NO_ERROR(error);
    TEST_REJECT_NULL(link);

    TEST_EXPECT_ZERO(aisc_create(link, PT_MAIN, com,
                                 MAIN_LOCS, PT_LOCS, locs,
                                 NULL));

    {
#define LOCAL(rvar) (prev_read_##rvar)

        
#define FREE_LOCAL_long(rvar) 
#define FREE_LOCAL_charp(rvar) free(LOCAL(rvar)) 
#define FREE_LOCAL(type,rvar) FREE_LOCAL_##type(rvar)

#define TEST__READ(type,rvar,expected)                                  \
        do {                                                            \
            TEST_EXPECT_ZERO(aisc_get(link, PT_LOCS, locs, rvar, &(LOCAL(rvar)), NULL)); \
            TEST_EXPECT_EQUAL(LOCAL(rvar), expected);                   \
            FREE_LOCAL(type,rvar);                                      \
        } while(0)
#define TEST_WRITE(type,rvar,val)                                       \
        TEST_EXPECT_ZERO(aisc_put(link, PT_LOCS, locs, rvar, (type)val, NULL))
#define TEST_CHANGE(type,rvar,val)              \
        do {                                    \
            TEST_WRITE(type, rvar, val);        \
            TEST__READ(type, rvar, val);        \
        } while(0)
#define TEST_DEFAULT_CHANGE(ctype,type,remote_variable,default_value,other_value) \
        do {                                                            \
            ctype DEFAULT_VALUE = default_value;                   \
            ctype OTHER_VALUE   = other_value;                     \
            type LOCAL(remote_variable);                                \
            TEST__READ(type, remote_variable, DEFAULT_VALUE);           \
            TEST_CHANGE(type, remote_variable, OTHER_VALUE);            \
            TEST_CHANGE(type, remote_variable, DEFAULT_VALUE);          \
        } while(0)

        TEST_DEFAULT_CHANGE(const long, long, LOCS_MATCH_REVERSED, 1, 67);
        typedef char *charp;
        typedef const char *ccharp;
        TEST_DEFAULT_CHANGE(ccharp, charp, LOCS_LOGINTIME, "notime", "sometime");
    }

    TEST_EXPECT_ZERO(aisc_close(link, com));
    link = 0;
}

#endif
