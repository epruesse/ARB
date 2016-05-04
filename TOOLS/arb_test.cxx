// =============================================================== //
//                                                                 //
//   File      : arb_test.cxx                                      //
//   Purpose   : unit tester for tools                             //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2011  //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <arb_defs.h>
#include <arb_sleep.h>
#include <arb_diff.h>
#include <unistd.h>

int ARB_main(int , char *[]) {
    fputs("don't call us\n", stderr);
    return EXIT_SUCCESS;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arb_file.h>
#include <test_unit.h>
#include <ut_valgrinded.h>

// --------------------------------------------------------------------------------


inline void make_valgrinded_call_from_pipe(char *&command) {
    using namespace utvg;
    static valgrind_info valgrind;
    if (valgrind.wanted) {
        char *pipeSym = strchr(command, '|');
        if (pipeSym) {
            char *left  = GB_strpartdup(command, pipeSym-1);
            char *right = strdup(pipeSym+1);

            make_valgrinded_call(left);
            make_valgrinded_call_from_pipe(right);

            freeset(command, GBS_global_string_copy("%s | %s", left, right));

            free(right);
            free(left);
        }
        else {
            make_valgrinded_call(command);
        }
    }
}

inline GB_ERROR valgrinded_system(const char *cmdline) {
    char *cmddup = strdup(cmdline);
    make_valgrinded_call_from_pipe(cmddup);

    GB_ERROR error = GBK_system(cmddup);
    free(cmddup);
    return error;
}

#define RUN_TOOL(cmdline)                valgrinded_system(cmdline)
#define RUN_TOOL_NEVER_VALGRIND(cmdline) GBK_system(cmdline)

#define TEST_RUN_TOOL(cmdline)                TEST_EXPECT_NO_ERROR(RUN_TOOL(cmdline))
#define TEST_RUN_TOOL_NEVER_VALGRIND(cmdline) TEST_EXPECT_NO_ERROR(RUN_TOOL_NEVER_VALGRIND(cmdline))

void TEST_SLOW_ascii_2_bin_2_ascii() {
    const char *ascii_ORG  = "TEST_loadsave_ascii.arb";
    const char *ascii      = "bin2ascii.arb";
    const char *binary     = "ascii2bin.arb";
    const char *binary_2ND = "ascii2bin2.arb";

    // test conversion file -> file
    TEST_RUN_TOOL(GBS_global_string("arb_2_bin   %s %s", ascii_ORG, binary));
    TEST_RUN_TOOL(GBS_global_string("arb_2_ascii %s %s", binary, ascii));

    TEST_EXPECT_FILES_EQUAL(ascii, ascii_ORG);

    // test conversion (bin->ascii->bin) via stream (this tests 'arb_repair')
    TEST_RUN_TOOL(GBS_global_string("arb_2_ascii %s - | arb_2_bin - %s", binary, binary_2ND));

    // TEST_EXPECT_FILES_EQUAL(binary, binary_2ND); // can't compare binary files (they contain undefined bytes)

    // instead convert back to ascii and compare result with original
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(ascii));
    TEST_RUN_TOOL(GBS_global_string("arb_2_ascii %s %s", binary_2ND, ascii));
    TEST_EXPECT_FILES_EQUAL(ascii, ascii_ORG);

    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(ascii));
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(binary));
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(binary_2ND));
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink("ascii2bin.ARF"));
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink("ascii2bin2.ARF"));
}

void TEST_arb_primer() {
    const char *primer_db       = "TEST_nuc.arb";
    const char *primer_stdin    = "tools/arb_primer.in";
    const char *primer_out      = "tools/arb_primer.out";
    const char *primer_expected = "tools/arb_primer_expected.out";

    TEST_RUN_TOOL(GBS_global_string("arb_primer %s < %s", primer_db, primer_stdin));
    TEST_EXPECT_FILES_EQUAL(primer_out, primer_expected);
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(primer_out));
}

static GB_ERROR removeVaryingDateFromTreeRemarks(const char *dbname) {
    GB_ERROR  error     = NULL;
    GB_shell  shell;
    GBDATA   *gb_main   = GB_open(dbname, "rw");
    if (!gb_main) error = GB_await_error();
    else {
        {
            GB_transaction ta(gb_main);

            GBDATA     *gb_tree_data    = GBT_get_tree_data(gb_main);
            const char *truncate_after  = "\nunittest-tree\n";
            size_t      truncate_offset = strlen(truncate_after);

            if (!gb_tree_data) error = GB_await_error();
            else {
                for (GBDATA *gb_tree = GB_child(gb_tree_data);
                     gb_tree && !error;
                     gb_tree = GB_nextChild(gb_tree))
                {
                    GBDATA *gb_remark = GB_entry(gb_tree, "remark");
                    if (!gb_remark) {
                        error = "could not find 'remark' entry";
                    }
                    else {
                        char *remark = GB_read_string(gb_remark);
                        char *found  = strstr(remark, truncate_after);

                        if (found) {
                            strcpy(found+truncate_offset, "<date removed for testing>");
                            error                  = GB_write_string(gb_remark, remark);
                        }
                        free(remark);
                    }
                }
            }

            ta.close(error);
        }
        if (!error) error = GB_save_as(gb_main, dbname, "a");
        GB_close(gb_main);
    }
    return error;
}

// #define TEST_AUTO_UPDATE_TREE // uncomment to auto-update expected tree

void TEST_SLOW_arb_read_tree() {
    struct {
        const char *basename;
        const char *extraArgs;
    }
    run[] = {
        { "newick",           "" },
        { "newick_sq",        "-commentFromFile general/text.input" },
        { "newick_dq",        "-scale 0.5" },
        { "newick_group",     "-scale 10 -consense 10" },
        { "newick_len",       "" },
        { "newick_len_group", "" },
    };

    const char *dbin       = "min_ascii.arb";
    const char *dbout      = "tools/read_tree_out.arb";
    const char *dbexpected = "tools/read_tree_out_expected.arb";

    for (size_t b = 0; b<ARRAY_ELEMS(run); ++b) {
        const char *basename  = run[b].basename;
        const char *extraArgs = run[b].extraArgs;
        char       *treefile  = GBS_global_string_copy("tools/%s.tree", basename);
        char       *treename  = GBS_global_string_copy("tree_%s", basename);

        TEST_RUN_TOOL(GBS_global_string("arb_read_tree -db %s %s %s %s \"test %s\" %s",
                                                   dbin, dbout, treename, treefile, basename, extraArgs));

        dbin = dbout; // use out-db from previous loop ( = write all trees into one db)

        free(treename);
        free(treefile);
    }

    TEST_EXPECT_NO_ERROR(removeVaryingDateFromTreeRemarks(dbout));
#if defined(TEST_AUTO_UPDATE_TREE)
    system(GBS_global_string("cp %s %s", dbout, dbexpected));
#else // !defined(TEST_AUTO_UPDATE_TREE)
    TEST_EXPECT_TEXTFILES_EQUAL(dbexpected, dbout);
#endif
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(dbout));
}

#define TEST_ARB_REPLACE(infile,expected,args) do {                     \
        char *tmpfile = GBS_global_string_copy("%s.tmp", expected);     \
        TEST_RUN_TOOL_NEVER_VALGRIND(GBS_global_string("cp %s %s", infile, tmpfile));  \
        TEST_RUN_TOOL(GBS_global_string("arb_replace %s %s", args, tmpfile)); \
        TEST_EXPECT_TEXTFILES_EQUAL(tmpfile, expected);                 \
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(tmpfile));             \
        free(tmpfile);                                                  \
    } while(0)

void TEST_arb_replace() {
    const char *infile = "tools/arb_replace.in";
    const char *file1  = "tools/arb_replace_1.out";
    const char *file2  = "tools/arb_replace_2.out";

    TEST_ARB_REPLACE(infile, "tools/arb_replace_1.out", "'gene=GONE'");
    TEST_ARB_REPLACE(file1,  infile,                    "-l 'GONE=gene'");
    TEST_ARB_REPLACE(file1,  file2,                     "-L 'GONE=gene:\"*\"=( * )'");
}

// --------------------------------------------------------------------------------

#include "command_output.h"

void TEST_arb_message() {
    TEST_STDERR_EQUALS("arb_message \"this is the test message\"",
                       "arb_message: this is the test message\n");
}

void TEST_SLOW_arb_probe() {
    // test called here currently are duplicating the tests in
    // arb_probe.cxx@TEST_SLOW_match_probe
    // and arb_probe.cxx@TEST_SLOW_design_probe
    //
    // Here test of functionality is secondary.
    // The primary goal here is to test calling the tools (i.e. arb_probe) 
    
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");
    TEST_STDOUT_EQUALS("arb_probe"
                       " serverid=-666"
                       " matchsequence=UAUCGGAGAGUUUGA",

                       /* ---- */ "    name---- fullname mis N_mis wmis pos ecoli rev          'UAUCGGAGAGUUUGA'\1"
                       "BcSSSS00\1" "  BcSSSS00            0     0  0.0   3     2 0   .......UU-===============-UCAAGUCGA\1"
        );

    TEST_STDOUT_EQUALS("arb_probe"
                       " serverid=-666"
                       " designnames=ClnCorin#CltBotul#CPPParap#ClfPerfr"
                       " designmintargets=100",

                       "Probe design parameters:\n"
                       "Length of probe    18\n"
                       "Temperature        [ 0.0 -400.0]\n"
                       "GC-content         [30.0 - 80.0]\n"
                       "E.Coli position    [any]\n"
                       "Max. nongroup hits 0\n"
                       "Min. group hits    100% (max. rejected coverage: 75%)\n"
                       "Target             le apos ecol qual grps   G+C temp     Probe sequence | Decrease T by n*.3C -> probe matches n non group species\n"
                       "CGAAAGGAAGAUUAAUAC 18 A=94   82   77    4  33.3 48.0 GUAUUAAUCUUCCUUUCG | - - - - - - - - - - - - - - - - - - - -\n"
                       "GAAAGGAAGAUUAAUACC 18 A+ 1   83   77    4  33.3 48.0 GGUAUUAAUCUUCCUUUC | - - - - - - - - - - - - - - - - - - - -\n"
                       "UCAAGUCGAGCGAUGAAG 18 B=18   17   61    4  50.0 54.0 CUUCAUCGCUCGACUUGA | - - - - - - - - - - - - - - - 2 2 2 2 2\n"
                       "AUCAAGUCGAGCGAUGAA 18 B- 1   16   45    4  44.4 52.0 UUCAUCGCUCGACUUGAU | - - - - - - - - - - - 2 2 2 2 2 2 2 2 2\n"
                       );
}

#define IN_DB     "tools/dnarates.arb"
#define OUT_DB    "tools/dnarates_result.arb"
#define WANTED_DB "tools/dnarates_expected.arb"

// #define TEST_AUTO_UPDATE_SAI // uncomment to auto-update expected SAI

void TEST_SLOW_arb_dna_rates() {
    TEST_STDOUT_CONTAINS("arb_dnarates tools/dnarates.inp " IN_DB " " OUT_DB, "\nWriting 'POS_VAR_BY_ML_1'\n");

#if defined(TEST_AUTO_UPDATE_SAI)
    TEST_COPY_FILE(OUT_DB, WANTED_DB);
#else // !defined(TEST_AUTO_UPDATE_SAI)
    TEST_EXPECT_TEXTFILES_EQUAL(WANTED_DB, OUT_DB);
#endif
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(OUT_DB));
}

#define RATES_DB "tools/exportrates.arb"

void TEST_arb_export_rates() {
    // Note: just testing against regression here.
    // Since the output is quite longish, we just test the checksums of the results.
    //
    // If one of the checksums changes unexpectedly and you want to see more details about the change, 
    // - go back to a revision with a correct checksum,
    // - add passing TEST_OUTPUT_EQUALS for broken command and
    // - move that test to broken revision.

    TEST_OUTPUT_HAS_CHECKSUM("arb_export_rates -d " RATES_DB " POS_VAR_BY_PARSIMONY", 0xc75a5fad);
    TEST_OUTPUT_HAS_CHECKSUM("arb_export_rates -d " RATES_DB " -r POS_VAR_BY_PARSIMONY", 0xd69fb01e);
    TEST_OUTPUT_HAS_CHECKSUM("arb_export_rates -d " RATES_DB " -r \"\"", 0xad0461ce);
}

#define TREE_DB "tools/tree.arb"

void TEST_arb_export_tree() {
    TEST_STDOUT_EQUALS("arb_export_tree tree_mini " TREE_DB,
                       "((( 'VibFurni' :0.02952, 'VibVulni' :0.01880):0.04015, 'VibChole' :0.03760):1.00000,( 'AcnPleur' :0.12011, 'PrtVulga' :0.06756):1.00000, 'HlmHalod' :1.00000);\n");
    TEST_STDOUT_EQUALS("arb_export_tree --bifurcated tree_mini " TREE_DB,
                       "(((( 'VibFurni' :0.02952, 'VibVulni' :0.01880):0.04015, 'VibChole' :0.03760):0.04610,( 'AcnPleur' :0.12011, 'PrtVulga' :0.06756):0.01732):0.07176, 'HlmHalod' :0.12399)'inner';\n");
    TEST_STDOUT_EQUALS("arb_export_tree --doublequotes tree_mini " TREE_DB,
                       "((( \"VibFurni\" :0.02952, \"VibVulni\" :0.01880):0.04015, \"VibChole\" :0.03760):1.00000,( \"AcnPleur\" :0.12011, \"PrtVulga\" :0.06756):1.00000, \"HlmHalod\" :1.00000);\n");

    TEST_STDOUT_EQUALS("arb_export_tree --nobranchlens tree_mini " TREE_DB,
                       "((( 'VibFurni'  'VibVulni' ) 'VibChole' ),( 'AcnPleur'  'PrtVulga' ), 'HlmHalod' );\n");
    TEST_EXPECT__BROKEN(0); // the test above returns a wrong result (commas are missing)

    TEST_OUTPUT_EQUALS("arb_export_tree \"\" " TREE_DB,
                       ";\n",                                                                    // shall export an empty newick tree
                       "");                                                                      // without error!
    TEST_OUTPUT_EQUALS("arb_export_tree tree_nosuch " TREE_DB,
                       ";\n",                                                                    // shall export an empty newick tree
                       "arb_export_tree from '" TREE_DB "': ARB ERROR: Failed to read tree 'tree_nosuch' (Reason: tree not found)\n"); // with error!
}

static char *notification_result = NULL;
static void test_notification_cb(const char *message, void *cd) {
    const char *cds     = (const char *)cd;
    notification_result = GBS_global_string_copy("message='%s' cd='%s'", message, cds);
}

#define INIT_NOTIFICATION                                                                       \
    GB_shell shell;                                                                             \
    GBDATA *gb_main = GBT_open("nosuch.arb", "crw");                                            \
    const char *cd  = "some argument";                                                          \
    char *cmd = GB_generate_notification(gb_main, test_notification_cb, "the note", (void*)cd)

#define EXIT_NOTIFICATION       \
    GB_close(gb_main);          \
    free(cmd)
    
#define TEST_DBSERVER_OPEN(gbmain) TEST_EXPECT_NO_ERROR(GBCMS_open(":", 0, gbmain))
#define TEST_DBSERVER_SERVE_UNTIL(gbmain, cond) do {                    \
        bool success            = GBCMS_accept_calls(gb_main, false);   \
        while (success) success = GBCMS_accept_calls(gb_main, true);    \
        GB_sleep(10, MS);                                               \
    } while(!(cond))

#define TEST_DBSERVER_CLOSE(gbmain) GBCMS_shutdown(gb_main)

void TEST_close_with_pending_notification() {
    INIT_NOTIFICATION;
    EXIT_NOTIFICATION;
}
void TEST_close_after_pending_notification_removed() {
    INIT_NOTIFICATION;
    TEST_EXPECT_NO_ERROR(GB_remove_last_notification(gb_main));
    EXIT_NOTIFICATION;
}
void TEST_arb_notify() {
    INIT_NOTIFICATION;

    TEST_DBSERVER_OPEN(gb_main);

    TEST_EXPECT_NULL(notification_result);
    TEST_EXPECT_NO_ERROR(GBK_system(GBS_global_string("%s &", cmd))); // async call to arb_notify

    TEST_DBSERVER_SERVE_UNTIL(gb_main, notification_result);
    TEST_DBSERVER_CLOSE(gb_main);

    TEST_EXPECT_EQUAL(notification_result, "message='the note' cd='some argument'");
    freenull(notification_result);

    EXIT_NOTIFICATION;
}

#endif // UNIT_TESTS

