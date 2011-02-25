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
#include <aw_awars.hxx>
#include <arb_defs.h>

int main(int, char **) {
    fputs("don't call us\n", stderr);
    return EXIT_SUCCESS;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

// --------------------------------------------------------------------------------

#define RUN_TOOL(cmdline)      GB_system(cmdline)
#define TEST_RUN_TOOL(cmdline) TEST_ASSERT_NO_ERROR(RUN_TOOL(cmdline))

void TEST_ascii_2_bin_2_ascii() {
    const char *ascii_ORG = "TEST_loadsave_ascii.arb";
    const char *ascii     = "bin2ascii.arb";
    const char *binary    = "ascii2bin.arb";

    TEST_RUN_TOOL(GBS_global_string("arb_2_bin   %s %s", ascii_ORG, binary));
    TEST_RUN_TOOL(GBS_global_string("arb_2_ascii %s %s", binary, ascii));

    TEST_ASSERT_FILES_EQUAL(ascii, ascii_ORG);

    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(ascii));
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(binary));
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink("ascii2bin.ARF"));
}

void TEST_arb_gene_probe() {
    const char *genome   = "tools/gene_probe.arb";
    const char *out      = "tools/gene_probe_out.arb";
    const char *expected = "tools/gene_probe_expected.arb";

    TEST_RUN_TOOL(GBS_global_string("arb_gene_probe %s %s", genome, out));
    TEST_ASSERT_FILES_EQUAL(out, expected);
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(out));
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink("tools/gene_probe_out.ARM"));
}

void TEST_arb_primer() {
    const char *primer_db       = "TEST_nuc.arb";
    const char *primer_stdin    = "tools/arb_primer.in";
    const char *primer_out      = "tools/arb_primer.out";
    const char *primer_expected = "tools/arb_primer_expected.out";

    TEST_RUN_TOOL(GBS_global_string("arb_primer %s < %s", primer_db, primer_stdin));
    TEST_ASSERT_FILES_EQUAL(primer_out, primer_expected);
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(primer_out));
}

static GB_ERROR removeVaryingDateFromTreeRemarks(const char *dbname) {
    GB_ERROR  error     = NULL;
    GB_shell  shell;
    GBDATA   *gb_main   = GB_open(dbname, "rw");
    if (!gb_main) error = GB_await_error();
    else {
        {
            GB_transaction  ta(gb_main);
            GBDATA         *gb_tree_data    = GB_entry(gb_main, "tree_data");
            const char     *truncate_after  = "\nunittest-tree\n";
            size_t          truncate_offset = strlen(truncate_after);

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

    TEST_ASSERT_NO_ERROR(removeVaryingDateFromTreeRemarks(dbout));
    TEST_ASSERT_TEXTFILES_EQUAL(dbout, dbexpected);
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(dbout));
}

#define TEST_ARB_REPLACE(infile,expected,args) do {                     \
        char *tmpfile = GBS_global_string_copy("%s.tmp", expected);     \
        TEST_RUN_TOOL(GBS_global_string("cp %s %s", infile, tmpfile));  \
        TEST_RUN_TOOL(GBS_global_string("arb_replace %s %s", args, tmpfile)); \
        TEST_ASSERT_TEXTFILES_EQUAL(tmpfile, expected);                 \
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(tmpfile));             \
        free(tmpfile);                                                  \
    } while(0)

void TEST_arb_replace() {
    const char *infile = "tools/gene_probe.arb";
    const char *file1  = "tools/arb_replace_1.out";
    const char *file2  = "tools/arb_replace_2.out";

    TEST_ARB_REPLACE(infile, "tools/arb_replace_1.out", "'gene=GONE'");
    TEST_ARB_REPLACE(file1,  infile,                    "-l 'GONE=gene'");
    TEST_ARB_REPLACE(file1,  file2,                     "-L 'GONE=gene:\"*\"=( * )'");
}

// --------------------------------------------------------------------------------

class CommandOutput {
    char     *stdoutput; // output from command
    char     *stderrput;
    GB_ERROR  error;

    void appendError(GB_ERROR err) {
        if (!error) error = err;
        else error = GBS_global_string("%s\n%s", error, err);
    }

    char *readAndUnlink(const char *file) {
        char *content = GB_read_file(file);
        if (!content) appendError(GB_await_error());
        int res = GB_unlink(file);
        if (res == -1) appendError(GB_await_error());
        return content;
    }
public:
    CommandOutput(const char *command)
        : stdoutput(NULL), stderrput(NULL), error(NULL)
    {
        char *escaped = GBS_string_eval(command, "'=\\\\'", NULL);
        if (!escaped) {
            appendError(GB_await_error());
        }
        else {
            char *cmd = GBS_global_string_copy("bash -c '%s >stdout.log 2>stderr.log'", escaped);

            appendError(GB_system(cmd));
            free(cmd);
            free(escaped);

            stdoutput = readAndUnlink("stdout.log");
            stderrput = readAndUnlink("stderr.log");
        }
        if (error) {
            printf("command '%s'\n"
                   "stdout='%s'\n"
                   "stderr='%s'\n", 
                   command, stdoutput, stderrput);
        }
    }
    ~CommandOutput() {
        free(stderrput);
        free(stdoutput);
    }

    GB_ERROR get_error() const { return error; }
    const char *get_stdoutput() const { return stdoutput; }
    const char *get_stderrput() const { return stderrput; }
};

// --------------------------------------------------------------------------------

static bool test_strcontains(const char *str, const char *part)  {
    const char *found = strstr(str, part);
    if (!found) {
        printf("string '%s'\ndoes not contain '%s'\n", str, part);
    }
    return found;
}

#define TEST_ASSERT_CONTAINS(str, part) TEST_ASSERT(test_strcontains(str, part))

#define TEST_OUTPUT_EQUALS(cmd, expected_std, expected_err)             \
    do {                                                                \
        CommandOutput out(cmd);                                         \
        TEST_ASSERT_NO_ERROR(out.get_error());                          \
        if (expected_std) { TEST_ASSERT_EQUAL(out.get_stdoutput(), expected_std); } \
        if (expected_err) { TEST_ASSERT_EQUAL(out.get_stderrput(), expected_err); } \
    } while(0)

#define TEST_OUTPUT_CONTAINS(cmd, expected_std, expected_err)           \
    do {                                                                \
        CommandOutput out(cmd);                                         \
        TEST_ASSERT_NO_ERROR(out.get_error());                          \
        if (expected_std) { TEST_ASSERT_CONTAINS(out.get_stdoutput(), expected_std); } \
        if (expected_err) { TEST_ASSERT_CONTAINS(out.get_stderrput(), expected_err); } \
    } while(0)

#define TEST_OUTPUT_HAS_CHECKSUM(cmd, checksum)                         \
    do {                                                                \
        CommandOutput out(cmd);                                         \
        TEST_ASSERT_NO_ERROR(out.get_error());                          \
        uint32_t css = GBS_checksum(out.get_stdoutput(), false, "");    \
        uint32_t cse = GBS_checksum(out.get_stderrput(), false, "");    \
        TEST_ASSERT_EQUAL(css^cse, uint32_t(checksum));                 \
    } while(0)

#define TEST_STDOUT_EQUALS(cmd, expected_std) TEST_OUTPUT_EQUALS(cmd, expected_std, NULL)
#define TEST_STDERR_EQUALS(cmd, expected_err) TEST_OUTPUT_EQUALS(cmd, NULL, expected_err)

#define TEST_STDOUT_CONTAINS(cmd, part) TEST_OUTPUT_CONTAINS(cmd, part, "")

// --------------------------------------------------------------------------------

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

                       "    name---- fullname mis N_mis wmis pos rev          'UAUCGGAGAGUUUGA'"
                       "BcSSSS00"
                       "  BcSSSS00            0     0  0.0   2 0   .......UU-===============-UCAAGUCGA"
                       );

    TEST_STDOUT_EQUALS("arb_probe"
                       " serverid=-666"
                       " designnames=ClnCorin#CltBotul#CPPParap#ClfPerfr"
                       " designmintargets=100",

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
                       "AUCAAGUCGAGCGAUGAA 18 B-     1   16    4 44.4 52.0    UUCAUCGCUCGACUUGAU |  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  3,\n"
                       );
}

#define IN_DB     "tools/dnarates.arb"
#define OUT_DB    "tools/dnarates_result.arb"
#define WANTED_DB "tools/dnarates_expected.arb"

void TEST_SLOW_arb_dna_rates() {
    TEST_STDOUT_CONTAINS("arb_dnarates tools/dnarates.inp " IN_DB " " OUT_DB, "\nWriting 'POS_VAR_BY_ML_1'\n");
    TEST_ASSERT_FILES_EQUAL(OUT_DB, WANTED_DB);
    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(OUT_DB));
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
    TEST_OUTPUT_HAS_CHECKSUM("arb_export_rates -d " RATES_DB " -r POS_VAR_BY_PARSIMONY", 0xb375ecc6);
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
    TEST_ASSERT__BROKEN(0); // the test above returns a wrong result (commas are missing)

    TEST_OUTPUT_EQUALS("arb_export_tree \"\" " TREE_DB,
                       ";\n",                                                                    // shall export an empty newick tree
                       "");                                                                      // without error!
    TEST_OUTPUT_EQUALS("arb_export_tree tree_nosuch " TREE_DB,
                       ";\n",                                                                    // shall export an empty newick tree
                       "arb_export_tree: Tree 'tree_nosuch' does not exist in DB '" TREE_DB "'\n"); // with error!
}

#endif // UNIT_TESTS

