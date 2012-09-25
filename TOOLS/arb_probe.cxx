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

    int ITERATE;
    int ITERATE_AMOUNT;
    int ITERATE_RESET;
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
                pd_gl.link = aisc_open(servername, pd_gl.com, AISC_MAGIC_NUMBER);

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
    ~PTserverConnection() {
        if (need_close) {
            aisc_close(pd_gl.link, pd_gl.com);
            pd_gl.link = 0;
        }
        --count;
    }
};
int PTserverConnection::count = 0;

static char *AP_probe_iterate_event(ARB_ERROR& error) {
    PTserverConnection contact(error);

    char *result = NULL;
    if (!error) {
        T_PT_PEP pep;
        if (aisc_create(pd_gl.link, PT_LOCS, pd_gl.locs,
                        LOCS_PROBE_FIND_CONFIG, PT_PEP, pep,
                        PEP_PLENGTH,  (long)P.ITERATE,
                        PEP_NUMGET,   (long)P.ITERATE_AMOUNT,
                        PEP_RESTART,  (long)P.ITERATE_RESET,
                        PEP_READABLE, (long)1,
                        NULL))
        {
            error = "Connection to PT_SERVER lost (1)";
        }

        if (!error) {
            aisc_put(pd_gl.link, PT_PEP, pep,
                     PEP_FIND_PROBES, 0,
                     NULL);

            char *pep_result;
            if (aisc_get(pd_gl.link, PT_PEP, pep,
                         PEP_RESULT, &pep_result,
                         NULL)) {
                error = "Connection to PT_SERVER lost (2)";
            }

            if (!error) result = pep_result;
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
                    if (*locs_error) {
                        error = locs_error;
                    }
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
                    char *match_info;
                    aisc_get(pd_gl.link, PT_TPROBE, tprobe,
                             TPROBE_INFO_HEADER,   &match_info,
                             NULL);
                    GBS_strcat(outstr, match_info);
                    GBS_chrcat(outstr, '\n');
                    free(match_info);
                }


                while (tprobe.exists()) {
                    char *match_info;
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
            char           *locs_error;
            T_PT_MATCHLIST  match_list;
            long            match_list_cnt;

            aisc_get(pd_gl.link, PT_LOCS, pd_gl.locs,
                     LOCS_MATCH_LIST,     match_list.as_result_param(),
                     LOCS_MATCH_LIST_CNT, &match_list_cnt,
                     LOCS_MATCH_STRING,   &bs,
                     LOCS_ERROR,          &locs_error,
                     NULL);
            if (*locs_error) error = locs_error;
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

    P.ITERATE=        getInt("iterate",        0,   1,  20,      "Iterate over probes of given length");
    P.ITERATE_AMOUNT= getInt("iterate_amount", 100, 1,  INT_MAX, "Number of results per answer");
    P.ITERATE_RESET=  getInt("iterate_reset",  0,   0,  1,       "First call should reset the iterator, consecutive calls shouldn't");

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
        TEST_ASSERT(parseCommandLine(ARRAY_ELEMS(args), args));

        // test default values here
        TEST_ASSERT_EQUAL(P.ACCEPTN, 1);
        TEST_ASSERT_EQUAL(P.LIMITN, 4);
        TEST_ASSERT_EQUAL(P.MISMATCHES, 0);
        TEST_ASSERT_EQUAL(P.MAXRESULT, 1000000);
    }

    {
        const char *args[] = {NULL, "serverid=4", "matchmismatches=2"};
        TEST_ASSERT(parseCommandLine(ARRAY_ELEMS(args), args));
        TEST_ASSERT_EQUAL(P.SERVERID, 4);
        TEST_ASSERT_EQUAL(P.MISMATCHES, 2);
        TEST_ASSERT_EQUAL(args[1], "serverid=4"); // check array args was not modified
    }

    {
        const char *args[] = { NULL, "matchacceptN=0", "matchlimitN=5"};
        TEST_ASSERT(parseCommandLine(ARRAY_ELEMS(args), args));
        TEST_ASSERT_EQUAL(P.ACCEPTN, 0);
        TEST_ASSERT_EQUAL(P.LIMITN, 5);
    }

    {
        const char *args[] = { NULL, "matchmaxresults=100"};
        TEST_ASSERT(parseCommandLine(ARRAY_ELEMS(args), args));
        TEST_ASSERT_EQUAL(P.MAXRESULT, 100);
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

#define TEST_PART1(fake_argc,fake_argv)                                                 \
    int       serverid = test_setup(use_gene_ptserver);                                 \
    TEST_ASSERT_EQUAL(true, parseCommandLine(fake_argc, fake_argv));                    \
    TEST_ASSERT((serverid == TEST_SERVER_ID)||(serverid == TEST_GENESERVER_ID));        \
    P.SERVERID         = serverid;                                                      \
    ARB_ERROR error;                                                                    \
    char      *answer   = execute(error);                                               \
    TEST_ASSERT_NO_ERROR(error.deliver())


#define TEST_ARB_PROBE(fake_argc,fake_argv,expected) do {               \
        TEST_PART1(fake_argc,fake_argv);                                \
        TEST_ASSERT_EQUAL(answer, expected);                            \
        free(answer);                                                   \
    } while(0)

#define TEST_ARB_PROBE__BROKEN(fake_argc,fake_argv,expected) do {       \
        TEST_PART1(fake_argc,fake_argv);                                \
        TEST_ASSERT_EQUAL__BROKEN(answer, expected);                    \
        free(answer);                                                   \
    } while(0)

#define TEST_ARB_PROBE_FILT(fake_argc,fake_argv,filter,expected) do {   \
        TEST_PART1(fake_argc,fake_argv);                                \
        char  *filtered   = filter(answer);                             \
        TEST_ASSERT_EQUAL(filtered, expected);                          \
        free(filtered);                                                 \
        free(answer);                                                   \
    } while(0)

typedef const char *CCP;

void TEST_SLOW_match_geneprobe() {
    bool use_gene_ptserver = true;
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=NNUCNN",
            "matchacceptN=4", 
            "matchlimitN=5", 
        };
        CCP expectd = "    organism genename------- mis N_mis wmis pos gpos rev          'NNUCNN'\1"
            "genome2\1" "  genome2  gene3             0     4  0.0   2    1 0   .........-UU==GG-UUGAUC\1"
            "genome2\1" "  genome2  joined1           0     4  0.0   2    1 0   .........-UU==GG-UUGAUCCUG\1"
            "genome2\1" "  genome2  gene2             0     4  0.0  10    4 0   ......GUU-GA==CU-GCCA\1"
            "genome2\1" "  genome2  intergene_19_65   0     4  0.0  31   12 0   GGUUACUGC-AU==GG-UGUUCGCCU\1"
            "genome1\1" "  genome1  intergene_17_65   0     4  0.0  31   14 0   GGUUACUGC-UA==GG-UGUUCGCCU\1"
            "genome2\1" "  genome2  intergene_19_65   0     4  0.0  38   19 0   GCAUUCGGU-GU==GC-CUAAGCACU\1"
            "genome1\1" "  genome1  intergene_17_65   0     4  0.0  38   21 0   GCUAUCGGU-GU==GC-CUAAGCCAU\1"
            "genome1\1" "  genome1  intergene_17_65   0     4  0.0  56   39 0   AGCCAUGCG-AG==AU-AUGUA\1" "";
        
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
            "genome1\1" "  genome1  joined1           0     2  0.0   5    2 0   ........C-U====G-AUCCUGC\1"
            "genome2\1" "  genome2  intergene_19_65   0     2  0.0  21    2 0   ........G-A====A-CUGCAUUCG\1"
            "genome1\1" "  genome1  intergene_17_65   0     2  0.0  21    4 0   ......CAG-A====A-CUGCUAUCG\1"
            "genome2\1" "  genome2  gene3             0     2  0.0   5    4 0   ......UUU-C====G-AUC\1"
            "genome2\1" "  genome2  joined1           0     2  0.0   5    4 0   ......UUU-C====G-AUCCUGCCA\1" "";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd);
    }

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UGAUCCU", // exists in data
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'UGAUCCU'\1"
            "genome1\1" "  genome1  gene2      0     0  0.0   9    1 0   .........-=======-GC\1"
            "genome2\1" "  genome2  gene2      0     0  0.0   9    3 0   .......GU-=======-GCCA\1" "";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // @@@ defect: probe exists as well in 'joined1' (of both genomes)
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GAUCCU",
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'GAUCCU'\1"
            "genome2\1" "  genome2  gene2      0     0  0.0  10    4 0   ......GUU-======-GCCA\1" "";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // @@@ defect: probe is part of above probe, but reports less hits
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UUUCGG", // exists only in genome2 
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'UUUCGG'\1"
            "genome2\1" "  genome2  gene3      0     0  0.0   2    1 0   .........-======-UUGAUC\1"
            "genome2\1" "  genome2  joined1    0     0  0.0   2    1 0   .........-======-UUGAUCCUG\1" "";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // @@@ defect: also exists in genome2/gene1
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=AUCCUG", 
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'AUCCUG'\1"
            "genome2\1" "  genome2  gene2      0     0  0.0  11    5 0   .....GUUG-======-CCA\1" "";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // @@@ defect: exists in 'gene2' and 'joined1' of both genomes
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UUGAUCCUGC",
        };
        CCP expectd = "    organism genename mis N_mis wmis pos gpos rev          'UUGAUCCUGC'\1"
            "genome2\1" "  genome2  gene2      0     0  0.0   8    2 0   ........G-==========-CA\1"
            "genome1\1" "  genome1  joined1    0     0  0.0   8    5 0   .....CUGG-==========-\1" "";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd); // @@@ defect: also exists in 'genome2/joined1'
    }
}

void TEST_SLOW_match_probe() {
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
            "BcSSSS00\1" "  BcSSSS00            0     1  0.0 176   162 0   CGGCUGGAU-==C========-U\1" ""; // only N-mismatch accepted

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.0 176   162 0   CGGCUGGAU-==C========-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     2  0.0 176   162 0   CGGCUGGAU-==C=======N-N\1"
            "ClfPerfr\1" "  ClfPerfr            1     1  1.1 176   162 0   AGAUUAAUA-=CC========-U\1";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.0 176   162 0   CGGCUGGAU-==C========-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     2  0.0 176   162 0   CGGCUGGAU-==C=======N-N\1"
            "DlcTolu2\1" "  DlcTolu2            0     3  0.0 176   162 0   CGGCUGGAU-==C======NN-N\1"
            "ClfPerfr\1" "  ClfPerfr            1     1  1.1 176   162 0   AGAUUAAUA-=CC========-U\1";

        arguments[2] = "matchmismatches=0";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2";  TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=UCACCUCCUUUC", // contains no N
            NULL // matchmismatches
        };

        CCP expectd0 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.0 175   161 0   GCGGCUGGA-===========N-N\1";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.0 175   161 0   GCGGCUGGA-===========N-N\1"
            "DlcTolu2\1" "  DlcTolu2            0     2  0.0 175   161 0   GCGGCUGGA-==========NN-N\1" "";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.0 175   161 0   GCGGCUGGA-===========N-N\1"
            "DlcTolu2\1" "  DlcTolu2            0     2  0.0 175   161 0   GCGGCUGGA-==========NN-N\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C=========-U\1" "";

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
            "BcSSSS00\1" "  BcSSSS00            0     1  0.0 176   162 0   CGGCUGGAU-==C========-U\1" "";
 
        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     1  0.0 176   162 0   CGGCUGGAU-==C========-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     2  0.0 176   162 0   CGGCUGGAU-==C=======N-N\1"
            "ClfPerfr\1" "  ClfPerfr            1     1  1.1 176   162 0   AGAUUAAUA-=CC========-U\1" "";

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
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U\1" "";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.0 175   161 0   GCGGCUGGA-===========N-N\1" "";

        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'UCACCUCCUUUC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 175   161 0   GCGGCUGGA-============-U\1"
            "PbcAcet2\1" "  PbcAcet2            0     1  0.0 175   161 0   GCGGCUGGA-===========N-N\1"
            "DlcTolu2\1" "  DlcTolu2            0     2  0.0 175   161 0   GCGGCUGGA-==========NN-N\1"
            "ClfPerfr\1" "  ClfPerfr            2     0  2.2 175   161 0   AAGAUUAAU-A=C=========-U\1" "";
        
        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
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
            "BcSSSS00\1" "  BcSSSS00            0     2  0.0 176   162 0   CGGCUGGAU-==C======U=-U\1" "";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     2  0.0 176   162 0   CGGCUGGAU-==C======U=-U\1"
            "DlcTolu2\1" "  DlcTolu2            0     3  0.0 176   162 0   CGGCUGGAU-==C=======N-N\1"
            "PbcAcet2\1" "  PbcAcet2            0     3  0.0 176   162 0   CGGCUGGAU-==C======UN-N\1"
            "ClfPerfr\1" "  ClfPerfr            1     2  1.1 176   162 0   AGAUUAAUA-=CC======U=-U\1" "";
        
        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCUCCUUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     2  0.0 176   162 0   CGGCUGGAU-==C======U=-U\1"
            "DlcTolu2\1" "  DlcTolu2            0     3  0.0 176   162 0   CGGCUGGAU-==C=======N-N\1"
            "PbcAcet2\1" "  PbcAcet2            0     3  0.0 176   162 0   CGGCUGGAU-==C======UN-N\1"
            "ClfPerfr\1" "  ClfPerfr            1     2  1.1 176   162 0   AGAUUAAUA-=CC======U=-U\1" "";
        
        arguments[2] = "matchmismatches=0"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd0);
        arguments[2] = "matchmismatches=1"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd1);
        arguments[2] = "matchmismatches=2"; TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expectd2);
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
            "BcSSSS00\1" "  BcSSSS00            0     5  0.0 176   162 0   CGGCUGGAU-==C=U=CU=U=-U\1" "";

        CCP expectd1 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCNCNNUNC'\1"
            "BcSSSS00\1" "  BcSSSS00            0     5  0.0 176   162 0   CGGCUGGAU-==C=U=CU=U=-U\1"
            "DlcTolu2\1" "  DlcTolu2            0     6  0.0 176   162 0   CGGCUGGAU-==C=U=CU==N-N\1"
            "PbcAcet2\1" "  PbcAcet2            0     6  0.0 176   162 0   CGGCUGGAU-==C=U=CU=UN-N\1"
            "LgtLytic\1" "  LgtLytic            1     5  0.6  31    26 0   GUCGAACGG-==G=A=AG=Cu-AGCUUGCUA\1"
            "ClfPerfr\1" "  ClfPerfr            1     5  0.6 111    99 0   CGGCUGGAU-==U=AuAA=G=-AGCGAUUGG\1"; // one hit is truncated here
        
        CCP expectd2 = "    name---- fullname mis N_mis wmis pos ecoli rev          'CANCNCNNUNC'\1"
            "HllHalod\1" "  HllHalod            2     5  1.6  45    40 0   AAACGAUGG-a=G=UuGC=U=-CAGGCGUCG\1"
            "VblVulni\1" "  VblVulni            2     5  1.6  49    44 0   AGCACAGAG-a=A=UuGU=U=-UCGGGUGGC\1"
            "VbrFurni\1" "  VbrFurni            2     5  1.7  40    35 0   CGGCAGCGA-==A=AuUGAA=-CUUCGGGGG\1"
            "LgtLytic\1" "  LgtLytic            2     5  1.7 101    89 0   GGGGAAACU-==AGCuAA=A=-CGCAUAAUC\1"
            "ClfPerfr\1" "  ClfPerfr            2     5  2.0 172   158 0   AGGAAGAUU-a=UaC=CC=C=-UUUCU\1"; // many hits are truncated here

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
    bool use_gene_ptserver = false;
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
        "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species\n"
        "CGAAAGGAAGAUUAAUAC 18 A=    94   82    4 33.3 48.0    GUAUUAAUCUUCCUUUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "GAAAGGAAGAUUAAUACC 18 A+     1   83    4 33.3 48.0    GGUAUUAAUCUUCCUUUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "UCAAGUCGAGCGAUGAAG 18 B=    18   17    4 50.0 54.0    CUUCAUCGCUCGACUUGA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "AUCAAGUCGAGCGAUGAA 18 B-     1   16    4 44.4 52.0    UUCAUCGCUCGACUUGAU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  3,\n";

    TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);

    // ------------------------------------------------------
    //      design MANY probes to test location specifier

    {
        char *positions = extract_locations(expected);
        TEST_ASSERT_EQUAL(positions, "A=94A+1B=18B-1");
        free(positions);
    }

    const char *arguments_loc[] = {
        "prgnamefake",
        // "designnames=Stsssola#Stsssola", // @@@ crashes the ptserver
        "designnames=CPPParap#PsAAAA00",
        "designmintargets=50", // hit at least 1 of the 2 targets
        "designmingc=0", "designmaxgc=100", // allow all GCs
        "designmishit=99999",  // allow enough outgroup hits
        "designmaxhits=99999", // do not limit results
        "designprobelength=3",
    };

    const char *expected_loc = 
        "A=96B=141C=20D=107B+1E=110F=84G=9H=150I=145C+1D+1J=17K=122L=72C-1M=33N=114E+1O=163"
        "P=24E+2F+1Q=138R=54O+1S=49A-1T=87G+1J-1N-1U=79F+2U-1V=129I+2H-2C+2W=12D-1D+2C-2R+1"
        "P-1J-2O+2V-2X=92W+2W+1Y=125Z=176D-2H+1a=104H-1L+1";

    TEST_ARB_PROBE_FILT(ARRAY_ELEMS(arguments_loc), arguments_loc, extract_locations, expected_loc);
}

void TEST_SLOW_match_designed_probe() {
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
    bool use_gene_ptserver = false;
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=20",
            "iterate_amount=10",
            "iterate_reset=1", 
        };
        CCP expected =
            "AAACCGGGGCTAATACCGGA.;AAACGACTGTTAATACCGCA.;AAACGATGGAAGCTTGCTTC.;AAACGATGGCTAATACCGCA.;AAACGGATTAGCGGCGGGAC.;"
            "AAACGGGCGCTAATACCGCA.;AAACGGTCGCTAATACCGGA.;AAACGGTGGCTAATACCGCA.;AAACGTACGCTAATACCGCA.;AAACTCAAGCTAATACCGCA.";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=15",
            "iterate_amount=20",
            "iterate_reset=1", 
        };
        CCP expected =
            "AAACCGGGGCTAATA.;AAACGACTGTTAATA.;AAACGATGGAAGCTT.;AAACGATGGCTAATA.;AAACGGATTAGCGGC.;AAACGGGCGCTAATA.;AAACGGTCGCTAATA.;"
            "AAACGGTGGCTAATA.;AAACGTACGCTAATA.;AAACTCAAGCTAATA.;AAACTCAGGCTAATA.;AAACTGGAGAGTTTG.;AAACTGTAGCTAATA.;AAACTTGTTTCTCGG.;"
            "AAAGAGGTGCTAATA.;AAAGCTTGCTTTCTT.;AAAGGAACGCTAATA.;AAAGGAAGATTAATA.;AAAGGACAGCTAATA.;AAAGGGACTTCGGTC.";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=10",
            "iterate_amount=30",
            "iterate_reset=1", 
        };
        CCP expected =
            "AAACCGGGGC.;AAACGACTGT.;AAACGATGGA.;AAACGATGGC.;AAACGGATTA.;AAACGGGCGC.;AAACGGTCGC.;AAACGGTGGC.;AAACGTACGC.;AAACTCAAGC.;"
            "AAACTCAGGC.;AAACTGGAGA.;AAACTGTAGC.;AAACTTGTTT.;AAAGAGGTGC.;AAAGCTTGCT.;AAAGGAACGC.;AAAGGAAGAT.;AAAGGACAGC.;AAAGGGACTT.;"
            "AAAGGGATTG.;AAAGGGGCTT.;AAAGGGGTGC.;AAAGTCTTCG.;AAAGTGGAGC.;AAAGTGGCGC.;AAATTGAGAG.;AACAAGGAAC.;AACAAGGTAA.;AACAAGGTAG.";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=3",
            "iterate_amount=70",
            "iterate_reset=1", 
        };
        CCP expected =
            "AAA.;AAC.;AAG.;AAT.;ACA.;ACC.;ACG.;ACT.;AGA.;AGC.;AGG.;AGT.;ATA.;ATC.;ATG.;ATT.;"
            "CAA.;CAC.;CAG.;CAT.;CCA.;CCC.;CCG.;CCT.;CGA.;CGC.;CGG.;CGT.;CTA.;CTC.;CTG.;CTT.;"
            "GAA.;GAC.;GAG.;GAT.;GCA.;GCC.;GCG.;GCT.;GGA.;GGC.;GGG.;GGT.;GTA.;GTC.;GTG.;GTT.;"
            "TAA.;TAC.;TAG.;TAT.;TCA.;TCC.;TCG.;TCT.;TGA.;TGC.;TGG.;TGT.;TTA.;TTC.;TTG.;TTT.";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
    {
        const char *arguments[] = {
            "prgnamefake",
            "iterate=2",
            "iterate_amount=20",
            "iterate_reset=1", 
        };
        CCP expected =
            "AA.;AC.;AG.;AT.;"
            "CA.;CC.;CG.;CT.;"
            "GA.;GC.;GG.;GT.;"
            "TA.;TC.;TG.;TT.";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }
}

void TEST_SLOW_unmatched_probes() {
    bool use_gene_ptserver = false;

    // match (parts of) an oligo which occurs in 3 species at 3'-end of alignment: GGCGCUGGAUCACCUCCUUU

    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GGCGCUGGAUCACCUCCUUU",
        };
        CCP expected = "    name---- fullname mis N_mis wmis pos ecoli rev          'GGCGCUGGAUCACCUCCUUU'\1"
            "VbrFurni\1" "  VbrFurni            0     0  0.0 166   152 0   GGGGAACCU-====================-\1"
            "VblVulni\1" "  VblVulni            0     0  0.0 166   152 0   GGGGAACCU-====================-\1"
            "VbhChole\1" "  VbhChole            0     0  0.0 166   152 0   GGGGAACCU-====================-\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
    }

    // now search part of the above 20-mer (not starting at pos == 0)
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GCGCUGGAUC",
        };

        // ptserver only reports one match ( = first match)
        // Reason: postree only contains ONE occurrance (when oligo is inside last 20 positions of a seq)

        CCP received = "    name---- fullname mis N_mis wmis pos ecoli rev          'GCGCUGGAUC'\1"
            "VbrFurni\1" "  VbrFurni            0     0  0.0 167   153 0   GGGAACCUG-==========-ACCUCCUUU\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, received);

        // would expect to get 3 matches 
        CCP expected = "    name---- fullname mis N_mis wmis pos ecoli rev          'GCGCUGGAUC'\1"
            "VbrFurni\1" "  VbrFurni            0     0  0.0 167   153 0   GGGAACCUG-==========-ACCUCCUUU\1"
            "VblVulni\1" "  VblVulni            0     0  0.0 167   153 0   GGGGAACCU-==========-ACCUCCUUU\1"
            "VbhChole\1" "  VbhChole            0     0  0.0 167   153 0   GGGGAACCU-==========-ACCUCCUUU\1";

        TEST_ARB_PROBE__BROKEN(ARRAY_ELEMS(arguments), arguments, expected);
    }

    // now search part of the above 20-mer (starting at pos == 0) -> works
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=GGCGCUGGAU",
        };

        CCP expected = "    name---- fullname mis N_mis wmis pos ecoli rev          'GGCGCUGGAU'\1"
            "VbrFurni\1" "  VbrFurni            0     0  0.0 166   152 0   GGGGAACCU-==========-CACCUCCUU\1"
            "VblVulni\1" "  VblVulni            0     0  0.0 166   152 0   GGGGAACCU-==========-CACCUCCUU\1"
            "VbhChole\1" "  VbhChole            0     0  0.0 166   152 0   GGGGAACCU-==========-CACCUCCUU\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);

    }
    // if the oligo also occurs somewhere else, it is found in all 3 species (VbrFurni, VblVulni + VbhChole)
    {
        const char *arguments[] = {
            "prgnamefake",
            "matchsequence=CACCUCCUUU",
            "matchmismatches=0", 
            "matchacceptN=0",
        };

        CCP expected = "    name---- fullname mis N_mis wmis pos ecoli rev          'CACCUCCUUU'\1"
            "BcSSSS00\1" "  BcSSSS00            0     0  0.0 176   162 0   CGGCUGGAU-==========-CU\1"
            "PbcAcet2\1" "  PbcAcet2            0     0  0.0 176   162 0   CGGCUGGAU-==========-NN\1"
            "Stsssola\1" "  Stsssola            0     0  0.0 176   162 0   CGGCUGGAU-==========-\1"
            "DcdNodos\1" "  DcdNodos            0     0  0.0 176   162 0   CGGUUGGAU-==========-\1"
            "VbrFurni\1" "  VbrFurni            0     0  0.0 176   162 0   GCGCUGGAU-==========-\1"
            "VblVulni\1" "  VblVulni            0     0  0.0 176   162 0   GCGCUGGAU-==========-\1"
            "VbhChole\1" "  VbhChole            0     0  0.0 176   162 0   GCGCUGGAU-==========-\1";

        TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);

    }
}

void TEST_SLOW_variable_defaults_in_server() {
    test_setup(false);

    const char *server_tag = GBS_ptserver_tag(TEST_SERVER_ID);
    TEST_ASSERT_NO_ERROR(arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag));

    const char *servername = GBS_read_arb_tcp(server_tag);;
    TEST_ASSERT_EQUAL(servername, "localhost:3200"); // as defined in ../lib/arb_tcp.dat@ARB_TEST_PT_SERVER

    T_PT_MAIN com;
    T_PT_LOCS locs;
    aisc_com *link = aisc_open(servername, com, AISC_MAGIC_NUMBER);
    TEST_ASSERT(link);

    TEST_ASSERT_ZERO(aisc_create(link, PT_MAIN, com,
                                 MAIN_LOCS, PT_LOCS, locs,
                                 NULL));

    {
#define LOCAL(rvar) (prev_read_##rvar)

        
#define FREE_LOCAL_long(rvar) 
#define FREE_LOCAL_charp(rvar) free(LOCAL(rvar)) 
#define FREE_LOCAL(type,rvar) FREE_LOCAL_##type(rvar)

#define TEST__READ(type,rvar,expected)                                  \
        do {                                                            \
            TEST_ASSERT_ZERO(aisc_get(link, PT_LOCS, locs, rvar, &(LOCAL(rvar)), NULL)); \
            TEST_ASSERT_EQUAL(LOCAL(rvar), expected);                   \
            FREE_LOCAL(type,rvar);                                      \
        } while(0)
#define TEST_WRITE(type,rvar,val)                                       \
        TEST_ASSERT_ZERO(aisc_put(link, PT_LOCS, locs, rvar, (type)val, NULL))
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

    TEST_ASSERT_ZERO(aisc_close(link, com));
    link = 0;
}

#endif
