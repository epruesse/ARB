// =============================================================== //
//                                                                 //
//   File      : arb_probe.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdb.h>
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

struct apd_sequence {
    apd_sequence *next;
    const char *sequence;
};

struct Params {
    int         DESIGNCPLIPOUTPUT;
    int         SERVERID;
    const char *DESINGNAMES;
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
    int         COMPLEMENT;
    int         WEIGHTED;

    apd_sequence *sequence;
} P;


struct gl_struct {
    aisc_com *link;
    T_PT_LOCS locs;
    T_PT_MAIN com;
    int pd_design_id;
} pd_gl;


int init_local_com_struct()
{
    const char *user = GB_getenvUSER();

    if (aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)) {
        return 1;
    }
    return 0;
}

static void aw_message(const char *error) {
    printf("%s\n", error);
}

static const char *AP_probe_pt_look_for_server() {
    const char *server_tag = GBS_ptserver_tag(P.SERVERID);
    GB_ERROR    error      = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag, 0);

    if (error) {
        aw_message(error);
        return 0;
    }
    return GBS_read_arb_tcp(server_tag);
}


static int probe_design_send_data(T_PT_PDC  pdc) {
    if (aisc_put(pd_gl.link, PT_PDC, pdc, PDC_CLIPRESULT, P.DESIGNCPLIPOUTPUT, NULL))
        return 1;

    return 0;
}

static char *AP_probe_design_event() {
    T_PT_PDC     pdc;
    T_PT_TPROBE  tprobe;
    bytestring   bs;
    char        *match_info;

    {
        const char *servername = AP_probe_pt_look_for_server();

        if (!servername) return NULL;
        pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com, AISC_MAGIC_NUMBER);
    }

    if (!pd_gl.link) {
        aw_message ("Cannot contact Probe bank server ");
        return NULL;
    }
    if (init_local_com_struct()) {
        aw_message ("Cannot contact Probe bank server (2)");
        return NULL;
    }

    bs.data = (char*)(P.DESINGNAMES);
    bs.size = strlen(bs.data)+1;

    aisc_create(pd_gl.link, PT_LOCS, pd_gl.locs,
                LOCS_PROBE_DESIGN_CONFIG, PT_PDC,   &pdc,
                PDC_PROBELENGTH, (long)P.DESIGNPROBELENGTH,
                PDC_MINTEMP,     (double)P.MINTEMP,
                PDC_MAXTEMP,     (double)P.MAXTEMP,
                PDC_MINGC,       P.MINGC/100.0,
                PDC_MAXGC,       P.MAXGC/100.0,
                PDC_MAXBOND,     (double)P.MAXBOND,
                NULL);
    aisc_put(pd_gl.link, PT_PDC, pdc,
             PDC_MINPOS,    P.MINPOS,
             PDC_MAXPOS,    P.MAXPOS,
             PDC_MISHIT,    P.MISHIT,
             PDC_MINTARGETS,    P.MINTARGETS/100.0,
             NULL);

    if (probe_design_send_data(pdc)) {
        aw_message ("Connection to PT_SERVER lost (1)");
        return NULL;
    }
    apd_sequence *s;
    for (s = P.sequence; s; s = s->next) {
        bytestring bs_seq;
        T_PT_SEQUENCE pts;
        bs_seq.data = (char*)s->sequence;
        bs_seq.size = strlen(bs_seq.data)+1;
        aisc_create(pd_gl.link, PT_PDC, pdc,
                    PDC_SEQUENCE, PT_SEQUENCE, &pts,
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
                      LOCS_ERROR,    &locs_error,
                      NULL)) {
            aw_message ("Connection to PT_SERVER lost (1)");
            return NULL;
        }
        if (*locs_error) {
            aw_message(locs_error);
        }
        free(locs_error);
    }

    aisc_get(pd_gl.link, PT_PDC, pdc,
              PDC_TPROBE, &tprobe,
             NULL);


    GBS_strstruct *outstr = GBS_stropen(1000);

    if (tprobe) {
        aisc_get(pd_gl.link, PT_TPROBE, tprobe,
                 TPROBE_INFO_HEADER,   &match_info,
                 NULL);
        GBS_strcat(outstr, match_info);
        GBS_chrcat(outstr, '\n');
        free(match_info);
    }


    while (tprobe) {
        if (aisc_get(pd_gl.link, PT_TPROBE, tprobe,
                     TPROBE_NEXT,      &tprobe,
                     TPROBE_INFO,      &match_info,
                     NULL)) break;
        GBS_strcat(outstr, match_info);
        GBS_chrcat(outstr, '\n');
        free(match_info);
    }

    aisc_close(pd_gl.link); pd_gl.link = 0;

    return GBS_strclose(outstr);
}

static char *AP_probe_match_event() {
    T_PT_MATCHLIST  match_list;
    char           *probe = 0;
    char           *locs_error;

    {
        const char *servername = AP_probe_pt_look_for_server();

        if (!servername) return NULL;
        pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com, AISC_MAGIC_NUMBER);
    }

    if (!pd_gl.link) {
        aw_message ("Cannot contact Probe bank server ");
        return NULL;
    }
    if (init_local_com_struct()) {
        aw_message ("Cannot contact Probe bank server (2)");
        return NULL;
    }

    if (aisc_nput(pd_gl.link, PT_LOCS, pd_gl.locs,
                  LOCS_MATCH_REVERSED,          (long)P.COMPLEMENT,
                  LOCS_MATCH_SORT_BY,           (long)P.WEIGHTED,
                  LOCS_MATCH_COMPLEMENT,        0L,
                  LOCS_MATCH_MAX_MISMATCHES,    (long)P.MISMATCHES,
                  LOCS_MATCH_MAX_SPECIES,       (long)100000,
                  LOCS_SEARCHMATCH,             P.SEQUENCE,
                  NULL)) {
        free(probe);
        aw_message ("Connection to PT_SERVER lost (2)");
        return NULL;
    }

    long match_list_cnt;
    bytestring bs;
    bs.data = 0;
    aisc_get(pd_gl.link, PT_LOCS, pd_gl.locs,
              LOCS_MATCH_LIST,  &match_list,
              LOCS_MATCH_LIST_CNT,  &match_list_cnt,
              LOCS_MATCH_STRING,    &bs,
              LOCS_ERROR,       &locs_error,
              NULL);
    if (*locs_error) {
        aw_message(locs_error);
    }
    free(locs_error);
    aisc_close(pd_gl.link);

    return bs.data; // freed by caller
}

static int          pargc;
static const char **pargv;
static int          helpflag;

static int getInt(const char *param, int val, int min, int max, const char *description) {
    if (helpflag) {
        printf("    %s=%3i [%3i:%3i] %s\n", param, val, min, max, description);
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
    if (helpflag) {
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

static int parseCommandLine(int argc, const char ** argv) {
    pargc = argc;
    pargv = argv;
    
    if (argc<=1) helpflag = 1;
    else helpflag         = 0;

    P.SERVERID = getInt("serverid", 0, 0, 100, "Server Id, look into $ARBHOME/lib/arb_tcp.dat");

    P.DESIGNCPLIPOUTPUT = getInt("designmaxhits", 100, 10, 10000, "Maximum Number of Probe Design Suggestions");
    P.DESINGNAMES       = getString("designnames", "",      "List of short names separated by '#'");
    P.sequence          = 0;
    while  ((P.DESIGNSEQUENCE = getString("designsequence", 0,      "Additional Sequences, will be added to the target group"))) {
        apd_sequence *s  = new apd_sequence;
        s->next          = P.sequence;
        P.sequence       = s;
        s->sequence      = P.DESIGNSEQUENCE;
        P.DESIGNSEQUENCE = 0;
    }
    P.DESIGNPROBELENGTH = getInt("designprobelength", 18, 10, 100, "Length of probe");
    P.MINTEMP           = getInt("designmintemp", 0, 0, 400, "Minimum melting temperature of probe");
    P.MAXTEMP           = getInt("designmaxtemp", 400, 0, 400, "Maximum melting temperature of probe");
    P.MINGC             = getInt("desingmingc", 30, 0, 100, "Minimum gc content");
    P.MAXGC             = getInt("desingmaxgc", 80, 0, 100, "Maximum gc content");
    P.MAXBOND           = getInt("desingmaxbond", 0, 0, 10, "Not implemented");

    P.MINPOS = getInt("desingminpos", 0, 0, 10000, "Minimum ecoli position");
    P.MAXPOS = getInt("desingmaxpos", 10000, 0, 10000, "Maximum ecoli position");

    P.MISHIT     = getInt("designmishit", 0, 0, 10000, "Number of allowed hits outside the selected group");
    P.MINTARGETS = getInt("designmintargets", 50, 0, 100, "Minimum percentage of hits within the selected species");

    P.SEQUENCE   = getString("matchsequence", "agtagtagt", "The sequence to search for");
    P.MISMATCHES = getInt("matchmismatches", 0, 0, 5, "Maximum Number of allowed mismatches");
    P.COMPLEMENT = getInt("matchcomplement", 0, 0, 1, "Match reversed and complemented probe");
    P.WEIGHTED   = getInt("matchweighted", 0, 0, 1,  "Use weighted mismatches");

    if (pargc>1) {
        printf("Unknown Parameter %s\n", pargv[1]);
        return false;
    }
    return !helpflag;
}

static char *execute() {
    char *answer;
    if (*P.DESINGNAMES || P.sequence) {
        answer = AP_probe_design_event();
    }
    else {
        answer = AP_probe_match_event();
    }
    return answer;
}

int main(int argc, const char ** argv) {
    bool ok = parseCommandLine(argc, argv);
    if (ok) {
        char *answer = execute();
        fputs(answer, stdout);
        free(answer);
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)

#include <test_unit.h>

static void test_ensure_newest_ptserver() {
    static bool killed = false;

    if (!killed) {
        // first kill pt-server (otherwise we may test an outdated pt-server)
        const char *server_tag = GBS_ptserver_tag(TEST_SERVER_ID);
        TEST_ASSERT_NO_ERROR(arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag, 0));
        killed = true;
    }
}

void TEST_SLOW_variable_defaults_in_server() {
    test_ensure_newest_ptserver();

    const char *server_tag = GBS_ptserver_tag(TEST_SERVER_ID);
    TEST_ASSERT_NO_ERROR(arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag, 0));

    const char *servername = GBS_read_arb_tcp(server_tag);;
    TEST_ASSERT_EQUAL(servername, "localhost:3200"); // as defined in ../lib/arb_tcp.dat@ARB_TEST_PT_SERVER

    long com;
    long locs;
    aisc_com *link = aisc_open(servername, &com, AISC_MAGIC_NUMBER);
    TEST_ASSERT(link);

    TEST_ASSERT_ZERO(aisc_create(link, PT_MAIN, com, MAIN_LOCS, PT_LOCS, &locs, NULL));

    {
#define LOCAL(rvar) (prev_read_##rvar)
#define TEST__READ(type,rvar,expected)                                  \
        do {                                                            \
            TEST_ASSERT_ZERO(aisc_get(link, PT_LOCS, locs, rvar, &(LOCAL(rvar)), NULL)); \
            TEST_ASSERT_EQUAL(LOCAL(rvar), expected);                   \
        } while(0)
#define TEST_WRITE(type,rvar,val)                                       \
        TEST_ASSERT_ZERO(aisc_put(link, PT_LOCS, locs, rvar, (type)val, NULL))
#define TEST_CHANGE(type,rvar,val)              \
        do {                                    \
            TEST_WRITE(type, rvar, val);        \
            TEST__READ(type, rvar, val);        \
        } while(0)
#define TEST_DEFAULT_CHANGE(type,remote_variable,default_value,other_value) \
        do {                                                            \
            const type DEFAULT_VALUE = default_value;                   \
            const type OTHER_VALUE   = other_value;                     \
            type LOCAL(remote_variable);                                \
            TEST__READ(type, remote_variable, DEFAULT_VALUE);           \
            TEST_CHANGE(type, remote_variable, OTHER_VALUE);            \
            TEST_CHANGE(type, remote_variable, DEFAULT_VALUE);          \
        } while(0)

        TEST_DEFAULT_CHANGE(long, LOCS_FF_PROBE_LEN, 12, 67);
        TEST_DEFAULT_CHANGE(char*, LOCS_LOGINTIME, "notime", "sometime");
    }

    TEST_ASSERT_ZERO(aisc_close(link));
}

// ----------------------------------
//      test probe design / match

static void test_arb_probe(int fake_argc, const char **fake_argv, const char *expected) {
    test_ensure_newest_ptserver();
    
    TEST_ASSERT_EQUAL(true, parseCommandLine(fake_argc, fake_argv));
    P.SERVERID   = TEST_SERVER_ID; // use test pt_server
    char *answer = execute();
    TEST_ASSERT_EQUAL(answer, expected);
    free(answer);
}

#define COUNT(array) sizeof(array)/sizeof(*array)

void TEST_SLOW_match_probe() {
    const char *arguments[] = {
        "fake", // "program"-name 
        "matchsequence=UAUCGGAGAGUUUGA", 
    };
    const char *expected =
        "    name---- fullname mis N_mis wmis pos rev          'UAUCGGAGAGUUUGA'\1"
        "BcSSSS00\1" "  BcSSSS00            0     0  0.0   2 0   .......UU-===============-UCAAGUCGA\1";

    test_arb_probe(COUNT(arguments), arguments, expected);
}

void TEST_SLOW_design_probe() {
    const char *arguments[] = {
        "fake", // "program"-name
        "designnames=ClnCorin#CltBotul#CPPParap#ClfPerfr",
        "designmintargets=100",
    };
    const char *expected = 
        "Probe design Parameters:\n"
        "Length of probe      18\n"
        "Temperature        [ 0.0 -400.0]\n"
        "GC-Content         [30.0 -80.0]\n"
        "E.Coli Position    [   0 -10000]\n"
        "Max Non Group Hits     0\n"
        "Min Group Hits       100%\n"
        "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species\n"
        "CGAAAGGAAGAUUAAUAC 18 A=    93   93    4 33.3 48.0    GUAUUAAUCUUCCUUUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "GAAAGGAAGAUUAAUACC 18 A+     1   94    4 33.3 48.0    GGUAUUAAUCUUCCUUUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "UCAAGUCGAGCGAUGAAG 18 B=    17   17    4 50.0 54.0    CUUCAUCGCUCGACUUGA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "AUCAAGUCGAGCGAUGAA 18 B-     1   16    4 44.4 52.0    UUCAUCGCUCGACUUGAU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  3,\n";

    test_arb_probe(COUNT(arguments), arguments, expected);
}

void TEST_SLOW_match_designed_probe() {
    const char *arguments[] = {
        "fake", // "program"-name 
        "matchsequence=UCAAGUCGAGCGAUGAAG", 
    };
    const char *expected =
    "    name---- fullname mis N_mis wmis pos rev          'UCAAGUCGAGCGAUGAAG'\1"
    "ClnCorin\1" "  ClnCorin            0     0  0.0  17 0   .GAGUUUGA-==================-UUCCUUCGG\1"
    "CltBotul\1" "  CltBotul            0     0  0.0  17 0   ........A-==================-CUUCUUCGG\1"
    "CPPParap\1" "  CPPParap            0     0  0.0  17 0   AGAGUUUGA-==================-UUCCUUCGG\1"
    "ClfPerfr\1" "  ClfPerfr            0     0  0.0  17 0   AGAGUUUGA-==================-UUUCCUUCG\1";

    test_arb_probe(COUNT(arguments), arguments, expected);
}


#endif
