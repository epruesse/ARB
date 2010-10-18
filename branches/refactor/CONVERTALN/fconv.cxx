// ------------- File format converting subroutine -------------

#include "defs.h"
#include "fun.h"
#include "global.h"
#include <static_assert.h>

static const char *format2name(int format_type) {
    switch (format_type) {
        case EMBL:      return "EMBL";
        case GCG:       return "GCG";
        case GENBANK:   return "GENBANK";
        case MACKE:     return "MACKE";
        case NBRF:      return "NBRF";
        case NEXUS:     return "NEXUS";
        case PHYLIP2:   return "PHYLIP2";
        case PHYLIP:    return "PHYLIP";
        case PRINTABLE: return "PRINTABLE";
        case SWISSPROT: return "SWISSPROT";
        case STADEN:    return "STADEN";
        default: ca_assert(0);
    }
    return NULL;
}

void throw_conversion_not_supported(int input_format, int output_format) { // __ATTR__NORETURN
    throw_errorf(90, "Conversion from %s to %s is not supported",
                 format2name(input_format), format2name(output_format));
}
void throw_conversion_failure(int input_format, int output_format) {
    throw_errorf(91, "Conversion from %s to %s fails",
                 format2name(input_format), format2name(output_format));
}
void throw_incomplete_entry() {
    throw_error(84, "Reached EOF before complete entry has been read");
}

static int log_processed_counter = 0;
static int log_seq_counter       = 0;

void log_processed(int seqCount) {
#if defined(CALOG)
    fprintf(stderr, "Total %d sequences have been processed\n", seqCount);
#endif // CALOG

    log_processed_counter++;
    log_seq_counter += seqCount;

    if (seqCount == 0) {
        throw_error(99, "No sequences have been processed");
    }
}

void convert(const char *inf, const char *outf, int intype, int outype) {
    // convert the file 'inf' (assuming it has type 'intype')
    // to desired 'outype' and save the result in 'outf'.

    int dd = 0; // copy stdin to outfile after first line
    if (outype == PHYLIP2) {
        outype = PHYLIP;
        dd = 1;
    }

    if (str_equal(inf, outf))
        throw_error(45, "Input file and output file must be different file");

    bool converted = true;
    switch (intype) {
        case EMBL:
            switch (outype) {
                case EMBL:
                case SWISSPROT: embl_to_embl(inf, outf); break;
                case GENBANK:   embl_to_genbank(inf, outf); break;
                case MACKE:     embl_to_macke(inf, outf, EMBL); break;
                default: converted = false; break;
            }
            break;

        case GENBANK:
            switch (outype) {
                case EMBL:      genbank_to_embl(inf, outf); break;
                    // SWISSPROT is skipped here intentially (original code said: not supported by GENEBANK)
                case GENBANK:   genbank_to_genbank(inf, outf); break;
                case MACKE:     genbank_to_macke(inf, outf); break;
                default: converted = false; break;
            }
            break;

        case MACKE:
            switch (outype) {
                case EMBL:
                case SWISSPROT: macke_to_embl(inf, outf); break;
                case GENBANK:   macke_to_genbank(inf, outf); break;
                default: converted = false; break;
            }
            break;

        case SWISSPROT:
            switch (outype) {
                case MACKE:     embl_to_macke(inf, outf, SWISSPROT); break;
                default: converted = false; break;
            }
            break;

        default: converted = false; break;
    }

    if (!converted) {
        converted = true;
        switch (outype) {
            case GCG:       to_gcg(inf, outf, intype); break;
            case NEXUS:     to_paup(inf, outf, intype); break;
            case PHYLIP:    to_phylip(inf, outf, intype, dd); break;
            case PRINTABLE: to_printable(inf, outf, intype); break;
            default: converted = false; break;
        }
    }

    if (!converted) {
        throw_conversion_not_supported(intype, outype);
    }
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <arbdbt.h> // before test_unit.h!
#include <test_unit.h>

struct FormatSpec {
    char        id;             // GENBANK, MACKE, ...
    const char *name;
    const char *testfile;       // existing testfile (or NULL)
    int         sequence_count; // number of sequences in 'testfile'
};

#define FORMATSPEC_OUT_ONLY(tag)                { tag, #tag, NULL, 1 }
#define FORMATSPEC_GOT______(tag,file)          { tag, #tag, "impexp/" file ".eft.exported", 1 }
#define FORMATSPEC_GOT_PLAIN(tag,file,seqcount) { tag, #tag, "impexp/" file, seqcount}

static FormatSpec format_spec[] = {
    // input formats
    // FORMATSPEC_GOT______(GENBANK, "genbank"),
    FORMATSPEC_GOT_PLAIN(GENBANK, "genbank.input", 3),
    FORMATSPEC_GOT_PLAIN(EMBL, "embl.input", 5),
    FORMATSPEC_GOT_PLAIN(MACKE, "macke.input", 5),
    FORMATSPEC_GOT_PLAIN(SWISSPROT, "swissprot.input", 1), // SWISSPROT

    // output formats
    FORMATSPEC_OUT_ONLY(GCG),
    FORMATSPEC_OUT_ONLY(NEXUS),
    FORMATSPEC_OUT_ONLY(PHYLIP),
    FORMATSPEC_OUT_ONLY(PHYLIP2),
    FORMATSPEC_OUT_ONLY(NBRF),
    FORMATSPEC_OUT_ONLY(PRINTABLE),
    FORMATSPEC_OUT_ONLY(STADEN),
};
static const int fcount = ARRAY_ELEMS(format_spec);

enum FormatNum { // same order as above
    NUM_GENBANK,
    NUM_EMBL,
    NUM_MACKE,
    NUM_SWISSPROT,

    NUM_GCG,
    NUM_NEXUS,
    NUM_PHYLIP,

    NUM_PHYLIP2,
    NUM_NBRF,
    NUM_PRINTABLE,
    NUM_STADEN,

    FORMATNUM_COUNT,
};

struct Capabilities {
    bool supported;
    bool haveInputData;
    bool emptyOutput;
    bool noOutput;
    bool crashes;
    bool neverReturns;
    bool broken; // is some other way

    Capabilities() :
        supported(true),
        haveInputData(true),
        emptyOutput(false),
        noOutput(false),
        crashes(false),
        neverReturns(false),
        broken(false)
    {}

    bool shall_be_tested() { return haveInputData && !(neverReturns || broken); }
};

static Capabilities cap[fcount][fcount];
#define CAP(from,to) (cap[NUM_##from][NUM_##to])

#define ID(f)    format_spec[f].id
#define NAME(f)  format_spec[f].name
#define INPUT(f) format_spec[f].testfile
#define EXSEQ(f) format_spec[f].sequence_count

// ----------------------------------
//      update .expected files ?

// #define TEST_AUTO_UPDATE // never does update if undefined
// #define UPDATE_ONLY_IF_MISSING
#define UPDATE_ONLY_IF_MORE_THAN_DATE_DIFFERS

inline bool more_than_date_differs(const char *file, const char *expected) {
    return !GB_test_textfile_difflines(file, expected, 0, 1);
}

#if defined(TEST_AUTO_UPDATE)
inline bool want_auto_update(const char *file, const char *expected) {
    bool shall_update = true;

    file     = file;
    expected = expected;

#if defined(UPDATE_ONLY_IF_MISSING)
    shall_update = shall_update && !GB_is_regularfile(expected);
#endif
#if defined(UPDATE_ONLY_IF_MORE_THAN_DATE_DIFFERS)
    shall_update = shall_update && more_than_date_differs(file, expected);
#endif
    return shall_update;
}
#else // !TEST_AUTO_UPDATE
inline bool want_auto_update(const char */*file*/, const char */*expected*/) {
    return false;
}
#endif

static void test_expected_conversion(const char *file, const char *flavor) {
    char *expected;
    if (flavor) expected = GBS_global_string_copy("%s.%s.expected", file, flavor);
    else expected = GBS_global_string_copy("%s.expected", file);

    bool shall_update = want_auto_update(file, expected);
    if (shall_update) {
        // TEST_ASSERT(0); // completely avoid real update
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(system(GBS_global_string("cp %s %s", file, expected)));
    }
    else {
        TEST_ASSERT(!more_than_date_differs(file, expected));
    }
    free(expected);
}

static const char *test_convert(const char *inf, const char *outf, int intype, int outype) {
    const char *error = NULL;
    try { convert(inf, outf, intype, outype); }
    catch (Convaln_exception& exc) { error = GBS_global_string("%s (#%i)", exc.get_msg(), exc.get_code()); }
    return error;
}

static void test_convert_by_format_num_WRAPPED(int from, int to) {
    char *toFile = GBS_global_string_copy("impexp/conv.%s_2_%s", NAME(from), NAME(to));
    if (GB_is_regularfile(toFile)) TEST_ASSERT_ZERO_OR_SHOW_ERRNO(unlink(toFile));

    int old_processed_counter = log_processed_counter;
    int old_seq_counter       = log_seq_counter;

    const char *error = test_convert(INPUT(from), toFile, ID(from), ID(to));

    int converted_seqs = log_seq_counter-old_seq_counter;
    int expected_seqs  = EXSEQ(from);
    if (to == NUM_GCG) expected_seqs = 1; // we stop after first file (useless to generate numerous files)

    Capabilities& me = cap[from][to];

    if (me.supported) {
        if (error) TEST_ERROR("convert() reports error: '%s' (for supported conversion)", error);
        if (me.noOutput) {
            TEST_ASSERT(!GB_is_regularfile(toFile));
        }
        else {
            TEST_ASSERT(GB_is_regularfile(toFile));
            if (me.emptyOutput) {
                TEST_ASSERT_EQUAL(GB_size_of_file(toFile), 0);
            }
            else { // expecting conversion worked
                TEST_ASSERT_EQUAL(converted_seqs, expected_seqs);
                TEST_ASSERT_EQUAL(log_processed_counter, old_processed_counter+1);

                TEST_ASSERT_LOWER_EQUAL(10, GB_size_of_file(toFile)); // less than 10 bytes
                test_expected_conversion(toFile, NULL);
            }
            TEST_ASSERT_ZERO_OR_SHOW_ERRNO(unlink(toFile));
        }
    }
    else {
        if (!error) TEST_ERROR("No error for unsupported conversion '%s'", GBS_global_string("%s -> %s", NAME(from), NAME(to)));
        TEST_ASSERT(strstr(error, "supported")); // wring error
        TEST_ASSERT(!GB_is_regularfile(toFile)); // unsupported produced output
    }
    TEST_ASSERT(me.supported == !error);

    free(toFile);
}

#if defined(ENABLE_CRASH_TESTS)
static int crash_from = -1;
static int crash_to   = -1;

static void crash_convert() {
    test_convert_by_format_num_WRAPPED(crash_from, crash_to);
}
#endif // ENABLE_CRASH_TESTS

static void test_convert_by_format_num(int from, int to) {
    TEST_ANNOTATE_ASSERT(GBS_global_string("while converting %s -> %s", NAME(from), NAME(to)));
    if (cap[from][to].crashes) {
#if defined(ENABLE_CRASH_TESTS)
        crash_from = from;
        crash_to   = to;
        TEST_ASSERT_SEGFAULT(crash_convert);
#endif // ENABLE_CRASH_TESTS
    }
    else {
        test_convert_by_format_num_WRAPPED(from, to);
    }
}

static void init_cap() {
    for (int from = 0; from<fcount; from++) {
        for (int to = 0; to<fcount; to++) {
            Capabilities& me = cap[from][to];

            if (to == NUM_PHYLIP2) me.neverReturns = true;
            if (!INPUT(from)) me.haveInputData     = false;
        }
    }
}

inline bool isInputFormat(int num) { return INPUT(num); }

#define NOT_SUPPORTED(t1,t2)         TEST_ASSERT(isInputFormat(NUM_##t1)); cap[NUM_##t1][NUM_##t2].supported   = false
#define NO_OUTFILE_CREATED(t1,t2)    TEST_ASSERT(isInputFormat(NUM_##t1)); cap[NUM_##t1][NUM_##t2].noOutput    = true
#define EMPTY_OUTFILE_CREATED(t1,t2) TEST_ASSERT(isInputFormat(NUM_##t1)); cap[NUM_##t1][NUM_##t2].emptyOutput = true
#define CRASHES(t1,t2)               TEST_ASSERT(isInputFormat(NUM_##t1)); cap[NUM_##t1][NUM_##t2].crashes     = true
#define FCKDUP(t1,t2)                TEST_ASSERT(isInputFormat(NUM_##t1)); cap[NUM_##t1][NUM_##t2].broken      = true

static int will_convert(int from) {
    int will = 0;
    for (int to = 0; to<fcount; to++) {
        Capabilities& me = cap[from][to];
        if (me.supported && me.shall_be_tested()) {
            will++;
        }
    }
    return will;
}

void TEST_0_converter() {
    COMPILE_ASSERT(FORMATNUM_COUNT == fcount);

    init_cap();

    NOT_SUPPORTED(GENBANK, SWISSPROT);

    NOT_SUPPORTED(GENBANK, NBRF);
    NOT_SUPPORTED(GENBANK, STADEN);

    NOT_SUPPORTED(MACKE, NBRF);
    NOT_SUPPORTED(MACKE, STADEN);

    NOT_SUPPORTED(EMBL, NBRF);
    NOT_SUPPORTED(EMBL, STADEN);

    NOT_SUPPORTED(SWISSPROT, GENBANK);
    NOT_SUPPORTED(SWISSPROT, EMBL);
    NOT_SUPPORTED(SWISSPROT, NBRF);
    NOT_SUPPORTED(SWISSPROT, STADEN);

    // unsupported self-conversions
    NOT_SUPPORTED(MACKE, MACKE);
    NOT_SUPPORTED(SWISSPROT, SWISSPROT);

    int possible     = 0;
    int tested       = 0;
    int unsupported  = 0;
    int crash        = 0;
    int neverReturns = 0;
    int broken       = 0;
    int noInput      = 0;
    int noOutput     = 0;
    int emptyOutput  = 0;

    for (int from = 0; from<fcount; from++) {
        TEST_ANNOTATE_ASSERT(GBS_global_string("while converting from '%s'", NAME(from)));
        if (isInputFormat(from)) {
            if (will_convert(from)<1) {
                TEST_ERROR("Conversion from %s seems unsupported", NAME(from));
            }
        }
        for (int to = 0; to<fcount; to++) {
            possible++;
            Capabilities& me = cap[from][to];

            if (me.shall_be_tested()) {
                test_convert_by_format_num(from, to);
                tested++;
            }

            unsupported  += !me.supported;
            crash        += me.crashes;
            neverReturns += me.neverReturns;
            broken       += me.broken;
            noInput      += !me.haveInputData;
            noOutput     += me.noOutput;
            emptyOutput  += me.emptyOutput;
        }
    }

    fprintf(stderr,
            "Conversion test summary:\n"
            " - formats:      %3i\n"
            " - conversions:  %3i (possible)\n"
            " - unsupported:  %3i\n"
            " - tested:       %3i\n"
            " - crash:        %3i\n"
            " - neverReturns: %3i (would never return - not checked)\n"
            " - noInputFile:  %3i\n"
            " - noOutput:     %3i\n"
            " - emptyOutput:  %3i\n"
            " - broken:       %3i (in some other way)\n"
            " - converted:    %3i\n",
            fcount,
            possible,
            unsupported,
            tested,
            crash,
            neverReturns,
            noInput,
            noOutput,
            emptyOutput,
            broken,
            tested-(emptyOutput+noOutput+unsupported));

    int untested     = possible - tested;
    int max_untested = noInput+neverReturns+broken; // the 3 counters may overlop

    TEST_ASSERT_LOWER_EQUAL(untested, max_untested);
}

#endif // UNIT_TESTS
