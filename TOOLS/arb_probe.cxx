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
#include <arb_defs.h>

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

static const char *AP_probe_pt_look_for_server(ARB_ERROR& error) {
    const char *server_tag = GBS_ptserver_tag(P.SERVERID);

    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag, 0);
    if (error) return NULL;
    
    return GBS_read_arb_tcp(server_tag);
}

static char *AP_probe_design_event(ARB_ERROR& error) {
    T_PT_PDC     pdc;
    T_PT_TPROBE  tprobe;
    bytestring   bs;
    char        *match_info;

    {
        const char *servername = AP_probe_pt_look_for_server(error);
        if (!servername) return NULL;

        pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com, AISC_MAGIC_NUMBER);
    }

    if (!pd_gl.link) {
        error = "Cannot contact PT_SERVER [1]";
        return NULL;
    }
    if (init_local_com_struct()) {
        error = "Cannot contact PT_SERVER [2]";
        return NULL;
    }

    bs.data = (char*)(P.DESIGNNAMES);
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

    if (aisc_put(pd_gl.link, PT_PDC, pdc,
                 PDC_MIN_ECOLIPOS,  (long)P.MINPOS,
                 PDC_MAX_ECOLIPOS,  (long)P.MAXPOS,
                 PDC_MISHIT,        (long)P.MISHIT,
                 PDC_MINTARGETS,    P.MINTARGETS/100.0,
                 PDC_CLIPRESULT,    (long)P.DESIGNCLIPOUTPUT, 
                 NULL) != 0) {
        error = "Connection to PT_SERVER lost (1)";
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

static char *AP_probe_match_event(ARB_ERROR& error) {
    T_PT_MATCHLIST  match_list;
    char           *probe = 0;

    {
        const char *servername = AP_probe_pt_look_for_server(error);
        if (!servername) return NULL;

        pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com, AISC_MAGIC_NUMBER);
    }

    if (!pd_gl.link) {
        error = "Cannot contact PT_SERVER [1]";
        return NULL;
    }
    if (init_local_com_struct()) {
        error = "Cannot contact PT_SERVER [2]";
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
        error = "Connection to PT_SERVER lost (1)";
        return NULL;
    }

    long match_list_cnt;
    
    bytestring bs;
    bs.data = 0;
    {
        char *locs_error;
        aisc_get(pd_gl.link, PT_LOCS, pd_gl.locs,
                 LOCS_MATCH_LIST,      &match_list,
                 LOCS_MATCH_LIST_CNT,  &match_list_cnt,
                 LOCS_MATCH_STRING,    &bs,
                 LOCS_ERROR,           &locs_error,
                 NULL);
        if (*locs_error) error = locs_error;
        free(locs_error);
    }
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

#ifdef UNIT_TESTS
    const int minServerID   = TEST_SERVER_ID;
#else // !UNIT_TESTS
    const int minServerID   = 0;
#endif

    P.SERVERID = getInt("serverid", 0, minServerID, 100, "Server Id, look into $ARBHOME/lib/arb_tcp.dat");
#ifdef UNIT_TESTS
    if (P.SERVERID<0) { arb_assert(P.SERVERID == TEST_SERVER_ID); }
#endif

    P.DESIGNCLIPOUTPUT = getInt("designmaxhits", 100, 10, 10000, "Maximum Number of Probe Design Suggestions");
    P.DESIGNNAMES       = getString("designnames", "",      "List of short names separated by '#'");
    P.sequence          = 0;
    while  ((P.DESIGNSEQUENCE = getString("designsequence", 0,      "Additional Sequences, will be added to the target group"))) {
        apd_sequence *s  = new apd_sequence;
        s->next          = P.sequence;
        P.sequence       = s;
        s->sequence      = P.DESIGNSEQUENCE;
        P.DESIGNSEQUENCE = 0;
    }
    P.DESIGNPROBELENGTH = getInt("designprobelength", 18, 2, 100, "Length of probe");
    P.MINTEMP           = getInt("designmintemp", 0, 0, 400, "Minimum melting temperature of probe");
    P.MAXTEMP           = getInt("designmaxtemp", 400, 0, 400, "Maximum melting temperature of probe");
    P.MINGC             = getInt("designmingc", 30, 0, 100, "Minimum gc content");
    P.MAXGC             = getInt("designmaxgc", 80, 0, 100, "Maximum gc content");
    P.MAXBOND           = getInt("designmaxbond", 0, 0, 10, "Not implemented");

    P.MINPOS = getInt("designminpos", -1, -1, INT_MAX, "Minimum ecoli position (-1=none)");
    P.MAXPOS = getInt("designmaxpos", -1, -1, INT_MAX, "Maximum ecoli position (-1=none)");

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

static char *execute(ARB_ERROR& error) {
    char *answer;
    if (*P.DESIGNNAMES || P.sequence) {
        answer = AP_probe_design_event(error);
    }
    else {
        answer = AP_probe_match_event(error);
    }
    return answer;
}

int main(int argc, const char ** argv) {
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

#include <test_unit.h>

static void test_setup() {
    static bool setup = false;
    if (!setup) {
        TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver"); // first call will recreate the test pt-server
        setup = true;
    }
}

void TEST_SLOW_variable_defaults_in_server() {
    test_setup();

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

        TEST_DEFAULT_CHANGE(const long, long, LOCS_FF_PROBE_LEN, 12, 67);
        typedef char *charp;
        typedef const char *ccharp;
        TEST_DEFAULT_CHANGE(ccharp, charp, LOCS_LOGINTIME, "notime", "sometime");
    }

    TEST_ASSERT_ZERO(aisc_close(link));
}

// ----------------------------------
//      test probe design / match

#define TEST_PART1(fake_argc,fake_argv)                                 \
    test_setup();                                                       \
    TEST_ASSERT_EQUAL(true, parseCommandLine(fake_argc, fake_argv));    \
    P.SERVERID = TEST_SERVER_ID;                                        \
    ARB_ERROR  error;                                                   \
    char      *answer = execute(error);                                 \
    TEST_ASSERT_NO_ERROR(error.deliver());                              \


#define TEST_ARB_PROBE(fake_argc,fake_argv,expected) do {               \
        TEST_PART1(fake_argc,fake_argv);                                \
        TEST_ASSERT_EQUAL(answer, expected);                            \
        free(answer);                                                   \
    } while(0)

#define TEST_ARB_PROBE_FILT(fake_argc,fake_argv,filter,expected) do {   \
        TEST_PART1(fake_argc,fake_argv);                                \
        char  *filtered   = filter(answer);                             \
        TEST_ASSERT_EQUAL(filtered, expected);                          \
        free(filtered);                                                 \
        free(answer);                                                   \
    } while(0)
    
void TEST_SLOW_match_probe() {
    const char *arguments[] = {
        "fake", // "program"-name 
        "matchsequence=UAUCGGAGAGUUUGA", 
    };
    const char *expected =
        "    name---- fullname mis N_mis wmis pos rev          'UAUCGGAGAGUUUGA'\1"
        "BcSSSS00\1" "  BcSSSS00            0     0  0.0   3 0   .......UU-===============-UCAAGUCGA\1";

    TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
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
        "E.Coli Position    [any]\n"
        "Max Non Group Hits     0\n"
        "Min Group Hits       100%\n"
        "Target             le     apos ecol grps  G+C 4GC+2AT Probe sequence     | Decrease T by n*.3C -> probe matches n non group species\n"
        "CGAAAGGAAGAUUAAUAC 18 A=    94   94    4 33.3 48.0    GUAUUAAUCUUCCUUUCG |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "GAAAGGAAGAUUAAUACC 18 A+     1   95    4 33.3 48.0    GGUAUUAAUCUUCCUUUC |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "UCAAGUCGAGCGAUGAAG 18 B=    18   18    4 50.0 54.0    CUUCAUCGCUCGACUUGA |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\n"
        "AUCAAGUCGAGCGAUGAA 18 B-     1   17    4 44.4 52.0    UUCAUCGCUCGACUUGAU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  3,\n";

    TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);

    // ------------------------------------------------------
    //      design MANY probes to test location specifier

    {
        char *positions = extract_locations(expected);
        TEST_ASSERT_EQUAL(positions, "A=94A+1B=18B-1");
        free(positions);
    }

    const char *arguments_loc[] = {
        "fake", // "program"-name
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
    const char *arguments[] = {
        "fake", // "program"-name 
        "matchsequence=UCAAGUCGAGCGAUGAAG", 
    };
    const char *expected =
    "    name---- fullname mis N_mis wmis pos rev          'UCAAGUCGAGCGAUGAAG'\1"
    "ClnCorin\1" "  ClnCorin            0     0  0.0  18 0   .GAGUUUGA-==================-UUCCUUCGG\1"
    "CltBotul\1" "  CltBotul            0     0  0.0  18 0   ........A-==================-CUUCUUCGG\1"
    "CPPParap\1" "  CPPParap            0     0  0.0  18 0   AGAGUUUGA-==================-UUCCUUCGG\1"
    "ClfPerfr\1" "  ClfPerfr            0     0  0.0  18 0   AGAGUUUGA-==================-UUUCCUUCG\1";

    TEST_ARB_PROBE(ARRAY_ELEMS(arguments), arguments, expected);
}


#endif
