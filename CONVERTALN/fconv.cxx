// ------------- File format converting subroutine -------------

#include "defs.h"
#include "fun.h"
#include "global.h"
#include <static_assert.h>
#include <unistd.h>
#include <arb_diff.h>

static const char *format2name(Format type) {
    switch (type) {
        case EMBL:      return "EMBL";
        case GCG:       return "GCG";
        case GENBANK:   return "GENBANK";
        case MACKE:     return "MACKE";
        case NEXUS:     return "NEXUS";
        case PHYLIP:    return "PHYLIP";
        case FASTDNAML: return "FASTDNAML";
        case PRINTABLE: return "PRINTABLE";
        case SWISSPROT: return "SWISSPROT";

        case UNKNOWN: ca_assert(0);
    }
    return NULL;
}

void throw_conversion_not_supported(Format inType, Format ouType) { // __ATTR__NORETURN
    throw_errorf(90, "Conversion from %s to %s is not supported",
                 format2name(inType), format2name(ouType));
}
void throw_conversion_failure(Format inType, Format ouType) { // __ATTR__NORETURN
    throw_errorf(91, "Conversion from %s to %s fails",
                 format2name(inType), format2name(ouType));
}
void throw_conversion_not_implemented(Format inType, Format ouType) { // __ATTR__NORETURN
    throw_errorf(92, "Conversion from %s to %s is not implemented (but is expected to be here)",
                 format2name(inType), format2name(ouType));
}
void throw_unsupported_input_format(Format inType) {  // __ATTR__NORETURN
    throw_errorf(93, "Unsupported input format %s", format2name(inType));
}

void throw_incomplete_entry() { // __ATTR__NORETURN
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
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arbdbt.h> // before test_unit.h!
#include <arb_file.h>
#include <test_unit.h>


#define TEST_THROW // comment out to temp. disable intentional throws

struct FormatSpec {
    Format      type;           // GENBANK, MACKE, ...
    const char *name;
    const char *testfile;       // existing testfile (or NULL)
    int         sequence_count; // number of sequences in 'testfile'
};

#define FORMATSPEC_OUT_ONLY(tag)                { tag, #tag, NULL, 1 }
#define FORMATSPEC_GOT______(tag,file)          { tag, #tag, "impexp/" file ".eft.exported", 1 }
#define FORMATSPEC_GOT_PLAIN(tag,file,seqcount) { tag, #tag, "impexp/" file, seqcount }

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
    FORMATSPEC_OUT_ONLY(PRINTABLE),
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

    NUM_PRINTABLE,

    FORMATNUM_COUNT,
};

struct Capabilities {
    bool supported;
    bool neverReturns;

    Capabilities() :
        supported(true),
        neverReturns(false)
    {}

    bool shall_be_tested() {
#if defined(TEST_THROW)
        return !neverReturns;
#else // !defined(TEST_THROW)
        return supported && !neverReturns;
#endif
    }
};

static Capabilities cap[fcount][fcount];
#define CAP(from,to) (cap[NUM_##from][NUM_##to])

#define TYPE(f)  format_spec[f].type
#define NAME(f)  format_spec[f].name
#define INPUT(f) format_spec[f].testfile
#define EXSEQ(f) format_spec[f].sequence_count

// ----------------------------------
//      update .expected files ?

// #define TEST_AUTO_UPDATE // never does update if undefined
// #define UPDATE_ONLY_IF_MISSING
#define UPDATE_ONLY_IF_MORE_THAN_DATE_DIFFERS

inline bool more_than_date_differs(const char *file, const char *expected) {
    return !ARB_textfiles_have_difflines(file, expected, 0, 1);
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
inline bool want_auto_update(const char * /* file */, const char * /* expected */) {
    return false;
}
#endif

static void test_expected_conversion(const char *file, const char *flavor) {
    char *expected;
    if (flavor) expected = GBS_global_string_copy("%s.%s.expected", file, flavor);
    else expected = GBS_global_string_copy("%s.expected", file);

    bool shall_update = want_auto_update(file, expected);
    if (shall_update) {
        // TEST_EXPECT(0); // completely avoid real update
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(system(GBS_global_string("cp %s %s", file, expected)));
    }
    else {
        TEST_REJECT(more_than_date_differs(file, expected));
    }
    free(expected);
}

static const char *test_convert(const char *inf, const char *outf, Format inType, Format ouType) {
    const char *error = NULL;
    try {
        convert(FormattedFile(inf ? inf : "infilename", inType),
                FormattedFile(outf ? outf : "outfilename", ouType));
    }
    catch (Convaln_exception& exc) { error = GBS_global_string("%s (#%i)", exc.get_msg(), exc.get_code()); }
    return error;
}

static void test_convert_by_format_num(int from, int to) {
    char *toFile = GBS_global_string_copy("impexp/conv.%s_2_%s", NAME(from), NAME(to));
    if (GB_is_regularfile(toFile)) TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(toFile));

    int old_processed_counter = log_processed_counter;
    int old_seq_counter       = log_seq_counter;

    const char *error = test_convert(INPUT(from), toFile, TYPE(from), TYPE(to));

    int converted_seqs = log_seq_counter-old_seq_counter;
    int expected_seqs  = EXSEQ(from);
    if (to == NUM_GCG) expected_seqs = 1; // we stop after first file (useless to generate numerous files)

    Capabilities& me = cap[from][to];

    if (me.supported) {
        if (error) TEST_ERROR("convert() reports error: '%s' (for supported conversion)", error);
        TEST_EXPECT(GB_is_regularfile(toFile));
        TEST_EXPECT_EQUAL(converted_seqs, expected_seqs);
        TEST_EXPECT_EQUAL(log_processed_counter, old_processed_counter+1);

        TEST_EXPECT_LESS_EQUAL(10, GB_size_of_file(toFile)); // less than 10 bytes
        test_expected_conversion(toFile, NULL);
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(toFile));
    }
    else {
        if (!error) TEST_ERROR("No error for unsupported conversion '%s'", GBS_global_string("%s -> %s", NAME(from), NAME(to)));
        TEST_REJECT_NULL(strstr(error, "supported")); // wrong error
        TEST_REJECT(GB_is_regularfile(toFile)); // unsupported produced output
    }
    TEST_EXPECT_EQUAL(me.supported, !error);

#if defined(TEST_THROW)
    {
        // test if conversion from empty and text file fails

        const char *fromFile = "general/empty.input";

        error = test_convert(fromFile, toFile, TYPE(from), TYPE(to));
        TEST_REJECT_NULL(error);

        fromFile = "general/text.input";
        error = test_convert(fromFile, toFile, TYPE(from), TYPE(to));
        TEST_REJECT_NULL(error);
    }
#endif

    free(toFile);
}

inline bool isInputFormat(int num) { return is_input_format(TYPE(num)); }

static void init_cap() {
    for (int from = 0; from<fcount; from++) {
        for (int to = 0; to<fcount; to++) {
            Capabilities& me = cap[from][to];
            if (!isInputFormat(from)) me.supported = false;
        }
    }
}

#define NOT_SUPPORTED(t1,t2) TEST_EXPECT(isInputFormat(NUM_##t1)); cap[NUM_##t1][NUM_##t2].supported = false

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

void TEST_SLOW_converter() {
    STATIC_ASSERT(FORMATNUM_COUNT == fcount);

    init_cap();

    NOT_SUPPORTED(GENBANK, SWISSPROT);
    NOT_SUPPORTED(EMBL, SWISSPROT);
    NOT_SUPPORTED(SWISSPROT, GENBANK);
    NOT_SUPPORTED(SWISSPROT, EMBL);

    int possible     = 0;
    int tested       = 0;
    int unsupported  = 0;
    int neverReturns = 0;

    for (int from = 0; from<fcount; from++) {
        TEST_ANNOTATE(GBS_global_string("while converting from '%s'", NAME(from)));
        if (isInputFormat(from)) {
            if (will_convert(from)<1) {
                TEST_ERROR("Conversion from %s seems unsupported", NAME(from));
            }
        }
        for (int to = 0; to<fcount; to++) {
            possible++;
            Capabilities& me = cap[from][to];

            if (me.shall_be_tested()) {
                TEST_ANNOTATE(GBS_global_string("while converting %s -> %s", NAME(from), NAME(to)));
                test_convert_by_format_num(from, to);
                tested++;
            }

            unsupported  += !me.supported;
            neverReturns += me.neverReturns;
        }
    }
    TEST_ANNOTATE(NULL);

    fprintf(stderr,
            "Conversion test summary:\n"
            " - formats:      %3i\n"
            " - conversions:  %3i (possible)\n"
            " - unsupported:  %3i\n"
            " - tested:       %3i\n"
            " - neverReturns: %3i (would never return - not checked)\n"
            " - converted:    %3i\n",
            fcount,
            possible,
            unsupported,
            tested,
            neverReturns,
            tested-unsupported);

    int untested = possible - tested;
    TEST_EXPECT_EQUAL(untested, neverReturns);
}

#endif // UNIT_TESTS
