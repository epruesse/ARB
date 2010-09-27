/* ------------- File format converting subroutine ------------- */

#include <stdio.h>
#include <stdlib.h>
#include "convert.h"
#include "global.h"
#include "types.h"

/* -------------------------------------------------------------
 *      Function convert().
 *              For given input file and  type, convert the file
 *                      to desired out type and save the result in
 *                      the out file.
 */

static void convert_WRAPPED(char *inf, char *outf, int intype, int outype) {
    int dd;                     /* copy stdin to outfile after first line */

    dd = 0;
    if (outype == PHYLIP2) {
        outype = PHYLIP;
        dd = 1;
    }

    if (str_equal(inf, outf))
        error(45, "Input file and output file must be different file.\n");
    if (intype == GENBANK && outype == MACKE) {
        genbank_to_macke(inf, outf);
    }
    else if (intype == GENBANK && outype == GENBANK) {
        genbank_to_genbank(inf, outf);
    }
    else if (intype == GENBANK && outype == PAUP) {
        to_paup(inf, outf, GENBANK);
    }
    else if (intype == GENBANK && outype == PHYLIP) {
        to_phylip(inf, outf, GENBANK, dd);
    }
    else if (intype == GENBANK && outype == PROTEIN) {
        error(69, "Sorry, cannot convert from GENBANK to SWISSPROT, Exit");
    }
    else if (intype == GENBANK && outype == EMBL) {
        genbank_to_embl(inf, outf);
    }
    else if (intype == GENBANK && outype == PRINTABLE) {
        to_printable(inf, outf, GENBANK);
    }
    else if (intype == GENBANK && outype == ALMA) {
        genbank_to_alma(inf, outf);
    }
    else if (intype == MACKE && outype == GENBANK) {
        macke_to_genbank(inf, outf);
    }
    else if (intype == MACKE && outype == PAUP) {
        to_paup(inf, outf, MACKE);
    }
    else if (intype == MACKE && outype == PHYLIP) {
        to_phylip(inf, outf, MACKE, dd);
    }
    else if (intype == MACKE && outype == PROTEIN) {
        macke_to_embl(inf, outf);
    }
    else if (intype == MACKE && outype == EMBL) {
        macke_to_embl(inf, outf);
    }
    else if (intype == MACKE && outype == PRINTABLE) {
        to_printable(inf, outf, MACKE);
    }
    else if (intype == MACKE && outype == ALMA) {
        macke_to_alma(inf, outf);
    }
    else if (intype == PAUP && outype == GENBANK) {
        error(6, "Sorry, cannot convert from Paup to GENBANK, Exit.\n");
    }
    else if (intype == PAUP && outype == MACKE) {
        error(81, "Sorry, cannot convert from Paup to AE2, Exit.\n");
    }
    else if (intype == PAUP && outype == PHYLIP) {
        error(8, "Sorry, cannot convert from Paup to Phylip, Exit.\n");
    }
    else if (intype == PAUP && outype == PROTEIN) {
        error(71, "Sorry, cannot convert from Paup to SWISSPROT, Exit.");
    }
    else if (intype == PAUP && outype == EMBL) {
        error(78, "Sorry, cannot convert from Paup to SWISSPROT, Exit.");
    }
    else if (intype == PHYLIP && outype == GENBANK) {
        error(85, "Sorry, cannot convert from Phylip to GenBank, Exit.\n");
    }
    else if (intype == PHYLIP && outype == MACKE) {
        error(88, "Sorry, cannot convert from Phylip to AE2, Exit.\n");
    }
    else if (intype == PHYLIP && outype == PAUP) {
        error(89, "Sorry, cannot convert from Phylip to Paup, Exit.\n");
    }
    else if (intype == PHYLIP && outype == PROTEIN) {
        error(72, "Sorry, cannot convert from Phylip to SWISSPROT, Exit.\n");
    }
    else if (intype == PHYLIP && outype == EMBL) {
        error(79, "Sorry, cannot convert from Phylip to SWISSPROT, Exit.\n");
    }
    else if (intype == PROTEIN && outype == MACKE) {
        embl_to_macke(inf, outf, PROTEIN);
    }
    else if (intype == PROTEIN && outype == GENBANK) {
        error(78, "GenBank doesn't maintain protein data.");
    }
    else if (intype == PROTEIN && outype == PAUP) {
        to_paup(inf, outf, PROTEIN);
    }
    else if (intype == PROTEIN && outype == PHYLIP) {
        to_phylip(inf, outf, PROTEIN, dd);
    }
    else if (intype == PROTEIN && outype == EMBL) {
        error(58, "EMBL doesn't maintain protein data.");
    }
    else if (intype == PROTEIN && outype == PRINTABLE) {
        to_printable(inf, outf, PROTEIN);
    }
    else if (intype == EMBL && outype == EMBL) {
        embl_to_embl(inf, outf);
    }
    else if (intype == EMBL && outype == GENBANK) {
        embl_to_genbank(inf, outf);
    }
    else if (intype == EMBL && outype == MACKE) {
        embl_to_macke(inf, outf, EMBL);
    }
    else if (intype == EMBL && outype == PROTEIN) {
        embl_to_embl(inf, outf);
    }
    else if (intype == EMBL && outype == PAUP) {
        to_paup(inf, outf, EMBL);
    }
    else if (intype == EMBL && outype == PHYLIP) {
        to_phylip(inf, outf, EMBL, dd);
    }
    else if (intype == EMBL && outype == PRINTABLE) {
        to_printable(inf, outf, EMBL);
    }
    else if (intype == EMBL && outype == ALMA) {
        embl_to_alma(inf, outf);
    }
    else if (intype == ALMA && outype == MACKE) {
        alma_to_macke(inf, outf);
    }
    else if (intype == ALMA && outype == GENBANK) {
        alma_to_genbank(inf, outf);
    }
    else if (intype == ALMA && outype == PAUP) {
        to_paup(inf, outf, ALMA);
    }
    else if (intype == ALMA && outype == PHYLIP) {
        to_phylip(inf, outf, ALMA, dd);
    }
    else if (intype == ALMA && outype == PRINTABLE) {
        to_printable(inf, outf, ALMA);
    }
    else if (outype == GCG) {
        to_gcg(intype, inf);
    }
    else
        error(90, "Unidentified input type or output type, Exit\n");
}
void convert(const char *cinf, const char *coutf, int intype, int outype) {
    char *inf  = strdup(cinf);
    char *outf = strdup(coutf);

    convert_WRAPPED(inf, outf, intype, outype);

    free(outf);
    free(inf);
}

/* ------------------- COMMON SUBROUTINES ----------------------- */

/* --------------------------------------------------------------
 *      Function init().
 *      Initialize data structure at the very beginning of running
 *              any program.
 */
void init()
{

    /* initialize macke format */
    data.macke.seqabbr = NULL;
    data.macke.name = NULL;
    data.macke.atcc = NULL;
    data.macke.rna = NULL;
    data.macke.date = NULL;
    data.macke.nbk = NULL;
    data.macke.acs = NULL;
    data.macke.who = NULL;
    data.macke.remarks = NULL;
    data.macke.numofrem = 0;
    data.macke.rna_or_dna = 'd';
    data.macke.journal = NULL;
    data.macke.title = NULL;
    data.macke.author = NULL;
    data.macke.strain = NULL;
    data.macke.subspecies = NULL;
    /* initialize genbank format */
    data.gbk.locus = NULL;
    data.gbk.definition = NULL;
    data.gbk.accession = NULL;
    data.gbk.keywords = NULL;
    data.gbk.source = NULL;
    data.gbk.organism = NULL;
    data.gbk.numofref = 0;
    data.gbk.reference = NULL;
    data.gbk.comments.orginf.exist = 0;
    data.gbk.comments.orginf.source = NULL;
    data.gbk.comments.orginf.cc = NULL;
    data.gbk.comments.orginf.formname = NULL;
    data.gbk.comments.orginf.nickname = NULL;
    data.gbk.comments.orginf.commname = NULL;
    data.gbk.comments.orginf.hostorg = NULL;
    data.gbk.comments.seqinf.exist = 0;
    data.gbk.comments.seqinf.RDPid = NULL;
    data.gbk.comments.seqinf.gbkentry = NULL;
    data.gbk.comments.seqinf.methods = NULL;
    data.gbk.comments.seqinf.comp5 = ' ';
    data.gbk.comments.seqinf.comp3 = ' ';
    data.gbk.comments.others = NULL;
    /* initialize paup format */
    data.paup.ntax = 0;
    data.paup.equate = "~=.|><";
    data.paup.gap = '-';
    /* initial phylip data */
    /* initial embl data */
    data.embl.id = NULL;
    data.embl.dateu = NULL;
    data.embl.datec = NULL;
    data.embl.description = NULL;
    data.embl.os = NULL;
    data.embl.accession = NULL;
    data.embl.keywords = NULL;
    data.embl.dr = NULL;
    data.embl.numofref = 0;
    data.embl.reference = NULL;
    data.embl.comments.orginf.exist = 0;
    data.embl.comments.orginf.source = NULL;
    data.embl.comments.orginf.cc = NULL;
    data.embl.comments.orginf.formname = NULL;
    data.embl.comments.orginf.nickname = NULL;
    data.embl.comments.orginf.commname = NULL;
    data.embl.comments.orginf.hostorg = NULL;
    data.embl.comments.seqinf.exist = 0;
    data.embl.comments.seqinf.RDPid = NULL;
    data.embl.comments.seqinf.gbkentry = NULL;
    data.embl.comments.seqinf.methods = NULL;
    data.embl.comments.seqinf.comp5 = ' ';
    data.embl.comments.seqinf.comp3 = ' ';
    data.embl.comments.others = NULL;
    /* initial alma data */
    data.alma.id = NULL;
    data.alma.filename = NULL;
    data.alma.format = UNKNOWN;
    data.alma.defgap = '-';
    data.alma.num_of_sequence = 0;
    data.alma.sequence = NULL;
    /* initial NBRF data format */
    data.nbrf.id = NULL;
    data.nbrf.description = NULL;
    /* initial sequence data */
    data.numofseq = 0;
    data.seq_length = 0;
    data.max = INITSEQ;
    data.sequence = (char *)calloc(1, (unsigned)(sizeof(char) * INITSEQ + 1));

    data.ids = NULL;
    data.seqs = NULL;
    data.lengths = NULL;
    data.allocated = 0;
}

/* --------------------------------------------------------------
 *      Function init_seq_data().
 *              Init. seq. data.
 */
void init_seq_data()
{
    data.numofseq = 0;
    data.seq_length = 0;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <arbdbt.h> // before test_unit.h!
#include <test_unit.h>
#include <arb_defs.h>

// #define TEST_AUTO_UPDATE

struct FormatSpec {
    char        id; // GENBANK, MACKE, ...
    const char *name;
    const char *testfile; // existing testfile (or NULL)
};

#define FORMATSPEC____(tag) { tag, #tag, NULL }
#define FORMATSPEC_GOT(tag,file) { tag, #tag, "impexp/" file ".eft.exported" }

static FormatSpec format[] = {
    FORMATSPEC_GOT(ALMA, "ae2"),
    FORMATSPEC_GOT(EMBL, "embl"),
    FORMATSPEC_GOT(GCG, "gcg"),
    FORMATSPEC_GOT(GENBANK, "genbank"),
    FORMATSPEC_GOT(MACKE, "ae2"),
    FORMATSPEC____(NBRF),
    FORMATSPEC_GOT(PAUP, "paup"),
    FORMATSPEC_GOT(PHYLIP, "phylip"),
    FORMATSPEC____(PHYLIP2),
    FORMATSPEC____(PRINTABLE),
    FORMATSPEC____(PROTEIN), // SWISSPROT
    FORMATSPEC____(STADEN),
};
static const int fcount = ARRAY_ELEMS(format);

enum FormatNum { // same order as above
    NUM_ALMA, 
    NUM_EMBL, 
    NUM_GCG, 
    NUM_GENBANK, 
    NUM_MACKE, 
    NUM_NBRF, 
    NUM_PAUP, 
    NUM_PHYLIP, 
    NUM_PHYLIP2, 
    NUM_PRINTABLE, 
    NUM_PROTEIN, 
    NUM_STADEN, 
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


#define ID(f)    format[f].id
#define NAME(f)  format[f].name
#define INPUT(f) format[f].testfile


#ifdef TEST_AUTO_UPDATE
#define TEST_EXPECTED_CONVERSION(file)                                  \
    do {                                                                \
        char *expected = GBS_global_string_copy("%s.expected", file);   \
        system(GBS_global_string("cp %s %s", file, expected));          \
        free(expected);                                                 \
    } while (0)
#else
#define TEST_EXPECTED_CONVERSION(file)                                  \
    do {                                                                \
        char *expected = GBS_global_string_copy("%s.expected", toFile); \
        TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES(file, expected, 0); \
        free(expected);                                                 \
    } while (0)
#endif

static const char *test_convert(const char *inf, const char *outf, int intype, int outype) {
    const char *error = NULL;
    try { convert(inf, outf, intype, outype); }
    catch (Convaln_exception& exc) { error = GBS_global_string("%s (#%i)", exc.error, exc.error_code); }
    return error;
}

static void test_convert_by_format_num_WRAPPED(int from, int to) {
    char          *toFile       = GBS_global_string_copy("impexp/conv.%s_2_%s", NAME(from), NAME(to));
    bool           test_outfile = true;
    const char    *error        = test_convert(INPUT(from), toFile, ID(from), ID(to));
    Capabilities&  me           = cap[from][to];

    if (error) {
        if (me.supported) { /* error ok if unsupported */
            TEST_ERROR("convert() reports error: '%s'", error);
        }
        test_outfile = false;
    }

    if (test_outfile) {
        if (me.noOutput) {
            TEST_ASSERT(!GB_is_regularfile(toFile));
        }
        else {
            TEST_ASSERT(GB_is_regularfile(toFile));
            if (me.emptyOutput) {
                TEST_ASSERT_EQUAL(GB_size_of_file(toFile), 0);
            }
            else {                                                  
                TEST_EXPECTED_CONVERSION(toFile);                   
            }                                                       
            TEST_ASSERT_ZERO_OR_SHOW_ERRNO(unlink(toFile));         
        }
    }
    free(toFile);
}

static int crash_from = -1;
static int crash_to = -1;

static void crash_convert() {
    test_convert_by_format_num_WRAPPED(crash_from, crash_to);
}

#define TEST_ANNOTATE_ASSERT(a)

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

#define NOT_SUPPORTED(t1,t2)         cap[NUM_##t1][NUM_##t2].supported   = false
#define NO_OUTFILE_CREATED(t1,t2)    cap[NUM_##t1][NUM_##t2].noOutput    = true
#define EMPTY_OUTFILE_CREATED(t1,t2) cap[NUM_##t1][NUM_##t2].emptyOutput = true
#define CRASHES(t1,t2)               cap[NUM_##t1][NUM_##t2].crashes     = true
#define FCKDUP(t1,t2)                cap[NUM_##t1][NUM_##t2].broken      = true

void TEST_converter() {
    init_cap();

    NOT_SUPPORTED(GENBANK, PROTEIN);

    NOT_SUPPORTED(GENBANK, NBRF);
    NOT_SUPPORTED(GENBANK, STADEN);

    NOT_SUPPORTED(MACKE, NBRF);
    NOT_SUPPORTED(MACKE, STADEN);

    NOT_SUPPORTED(PHYLIP, ALMA);
    NOT_SUPPORTED(PHYLIP, EMBL);
    CRASHES(PHYLIP, GCG);
    NOT_SUPPORTED(PHYLIP, GENBANK);
    NOT_SUPPORTED(PHYLIP, MACKE);
    NOT_SUPPORTED(PHYLIP, NBRF);
    NOT_SUPPORTED(PHYLIP, PAUP);
    NOT_SUPPORTED(PHYLIP, PRINTABLE);
    NOT_SUPPORTED(PHYLIP, PROTEIN);
    NOT_SUPPORTED(PHYLIP, STADEN);
    
    NOT_SUPPORTED(PAUP, ALMA);
    NOT_SUPPORTED(PAUP, EMBL);
    NOT_SUPPORTED(PAUP, GENBANK);
    NOT_SUPPORTED(PAUP, MACKE);
    NOT_SUPPORTED(PAUP, NBRF);
    NOT_SUPPORTED(PAUP, PHYLIP);
    NOT_SUPPORTED(PAUP, PRINTABLE);
    NOT_SUPPORTED(PAUP, PROTEIN);
    NOT_SUPPORTED(PAUP, STADEN);

    NOT_SUPPORTED(EMBL, NBRF);
    NOT_SUPPORTED(EMBL, STADEN);

    NOT_SUPPORTED(GCG, ALMA);
    NOT_SUPPORTED(GCG, EMBL);
    NOT_SUPPORTED(GCG, GENBANK);
    NOT_SUPPORTED(GCG, MACKE);
    NOT_SUPPORTED(GCG, NBRF);
    NOT_SUPPORTED(GCG, PAUP);
    NOT_SUPPORTED(GCG, PHYLIP);
    NOT_SUPPORTED(GCG, PRINTABLE);
    NOT_SUPPORTED(GCG, PROTEIN);
    NOT_SUPPORTED(GCG, STADEN);
    
    NOT_SUPPORTED(ALMA, EMBL);
    NOT_SUPPORTED(ALMA, NBRF);
    NOT_SUPPORTED(ALMA, PROTEIN);
    NOT_SUPPORTED(ALMA, STADEN);

    // broken atm:
    FCKDUP(EMBL, GCG);
    FCKDUP(GENBANK, GCG);
    
    CRASHES(PAUP, GCG);
    CRASHES(ALMA, GCG);

    NO_OUTFILE_CREATED(MACKE, GCG);   // did not create file

    EMPTY_OUTFILE_CREATED(ALMA, GENBANK);
    EMPTY_OUTFILE_CREATED(ALMA, MACKE);
    EMPTY_OUTFILE_CREATED(ALMA, PRINTABLE);

    EMPTY_OUTFILE_CREATED(EMBL, GENBANK);
    EMPTY_OUTFILE_CREATED(EMBL, MACKE);
    EMPTY_OUTFILE_CREATED(EMBL, PROTEIN);

    EMPTY_OUTFILE_CREATED(GENBANK, MACKE);

    EMPTY_OUTFILE_CREATED(MACKE, EMBL);
    EMPTY_OUTFILE_CREATED(MACKE, GENBANK);
    EMPTY_OUTFILE_CREATED(MACKE, PROTEIN);

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
        for (int to = 0; to<fcount; to++) {
            if (from == to) continue;

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
            " - broken:       %3i (in some other way)\n",
            fcount,
            possible,
            unsupported,
            tested,
            crash,
            neverReturns,
            noInput,
            noOutput,
            emptyOutput,
            broken);

    int untested     = possible - tested;
    int max_untested = noInput+neverReturns+broken; // the 3 counters may overlop

    TEST_ASSERT_LOWER_EQUAL(untested, max_untested);
}

#endif // UNIT_TESTS
