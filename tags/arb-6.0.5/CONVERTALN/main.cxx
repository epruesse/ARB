// ------------------------------------------------------------
//
// Format Conversion Program.
//
// Woese Lab., Dept. of Microbiology, UIUC
// Modified for use in ARB by ARB team
//
// ------------------------------------------------------------

#include "defs.h"
#include "fun.h"
#include "global.h"

Convaln_exception *Convaln_exception::thrown = NULL;

struct TypeSwitch { const char *switchtext; Format format; };

static TypeSwitch convertible_type[] = { // see fconv.cxx@format_spec
    { "GenBank",   GENBANK   },
    { "EMBL",      EMBL      },
    { "AE2",       MACKE     },
    { "SwissProt", SWISSPROT },
    { "NEXUS",     NEXUS     },
    { "PHYLIP",    PHYLIP    },
    { "FASTDNAML", FASTDNAML },
    { "GCG",       GCG       },
    { "PRINTABLE", PRINTABLE },
};

static void show_command_line_usage() {
    fputs("Command line usage:\n"
          "  $ arb_convert_aln [--arb-notify] -INFMT input_file -OUTFMT output_file\n"
          "\n"
          "  where\n"
          "      INFMT  may be 'GenBank', 'EMBL', 'AE2' or 'SwissProt' and\n"
          "      OUTFMT may be 'GenBank', 'EMBL', 'AE2', 'NEXUS', 'PHYLIP', 'FASTDNAML', 'GCG' or 'Printable'\n"
          "  (Note: you may abbreviate the format names)\n"
          "\n"
          "  FASTDNAML writes a PHYLIP file with content from STDIN appended at end of first line (used for arb_fastdnaml).\n"
          "\n"
          "  if argument '--arb-notify' is given, arb_convert_aln assumes it has been started by ARB\n"
          "  and reports errors using the 'arb_message' script.\n"
          , stderr);
}

static void valid_name_or_die(const char *file_name) {
    if (str0len(file_name) <= 0) {
        throw_errorf(152, "illegal file name: %s", file_name);
    }
}
static bool file_exists(const char *file_name) {
    FILE *ifp    = fopen(file_name, "r");
    bool  exists = ifp != NULL;
    if (ifp) fclose(ifp);

    return exists;
}

static void change_file_suffix(const char *old_file, char *file_name, int type) {
    // Define the default file name by changing suffix.
    int indi, indj;

    for (indi = str0len(old_file) - 1; indi >= 0 && old_file[indi] != '.'; indi--)
        if (indi == 0)
            strcpy(file_name, old_file);
        else {
            for (indj = 0; indj < (indi - 1); indj++)
                file_name[indj] = old_file[indj];
            file_name[indj] = '\0';
        }
    switch (type) {
        case GENBANK:
            strcat(file_name, ".GB");
            break;
        case MACKE:
            strcat(file_name, ".aln");
            break;
        case NEXUS:
            strcat(file_name, ".NEXUS");
            break;
        case PHYLIP:
            strcat(file_name, ".PHY");
            break;
        case EMBL:
            strcat(file_name, ".EMBL");
            break;
        case PRINTABLE:
            strcat(file_name, ".prt");
            break;
        default:
            strcat(file_name, ".???");
    }
}

static void ask_for_conversion_params(FormattedFile& in, FormattedFile& out) {
    char temp[LINESIZE];
    char choice[LINESIZE];

    fputs("---------------------------------------------------------------\n"
          "\n"
          "  convert_aln - an alignment and file converter written by\n"
          "  WenMin Kuan for the RDP database project.\n"
          "\n"
          "  Modified for use in ARB by Oliver Strunk & Ralf Westram\n"
          "  Report errors or deficiencies to devel@arb-home.de\n"
          "\n"
          , stderr);
    show_command_line_usage();
    fputs("\n"
          "---------------------------------------------------------------\n"
          "\n"
          "Select input format (<CR> means default)\n"
          "\n"
          "  (1)  GenBank [default]\n"
          "  (2)  EMBL\n"
          "  (3)  AE2\n"
          "  (4)  SwissProt\n"
          "  (5)  Quit\n"
          "  ? "
          , stderr);

    Getstr(choice, LINESIZE);
    {
        Format inType = UNKNOWN;
        switch (choice[0]) {
            case '\0': // [default]
            case '1': inType = GENBANK; break;
            case '2': inType = EMBL; break;
            case '3': inType = MACKE; break;
            case '4': inType = SWISSPROT; break;
            case '5': exit(0); // ok - interactive mode only
            default: throw_errorf(16, "Unknown input format selection '%s'", choice);
        }

        fputs("\nInput file name? ", stderr);
        Getstr(temp, LINESIZE);
        in.init(temp, inType);
    }

    valid_name_or_die(temp);
    if (!file_exists(temp)) throw_error(77, "Input file not found");

    // output file information
    fputs("\n"
          "Select output format (<CR> means default)\n"
          "\n"
          "  (1)  GenBank\n"
          "  (2)  EMBL\n"
          "  (3)  AE2 [default]\n"
          "  (4)  NEXUS (Paup)\n"
          "  (5)  PHYLIP\n"
          "  (6)  GCG\n"
          "  (7)  Printable\n"
          "  (8)  Quit\n"
          "  ? ", stderr);

    Getstr(choice, LINESIZE);
    {
        Format ouType = UNKNOWN;
        switch (choice[0]) {
            case '1': ouType = GENBANK; break;
            case '2': ouType = EMBL; break;
            case '\0': // [default]
            case '3': ouType = MACKE; break;
            case '4': ouType = NEXUS; break;
            case '5': ouType = PHYLIP; break;
            case '6': ouType = GCG; break;
            case '7': ouType = PRINTABLE; break;
            case '8': exit(0); // ok - interactive mode only
            default: throw_errorf(66, "Unknown output format selection '%s'", choice);
        }
        change_file_suffix(in.name(), temp, ouType);
        if (ouType != GCG) {
            fprintf(stderr, "\nOutput file name [%s]? ", temp);
            Getstr(temp, LINESIZE);
            if (str0len(temp) == 0)
                change_file_suffix(in.name(), temp, ouType);
        }
        out.init(temp, ouType);
    }
}

static int strcasecmp_start(const char *s1, const char *s2) {
    int cmp = 0;
    for (int p = 0; !cmp; p++) {
        cmp = tolower(s1[p])-tolower(s2[p]);
        if (!s1[p]) return 0;
    }
    return cmp;
}

static bool is_abbrev_switch(const char *arg, const char *switchtext) {
    return arg[0] == '-' && strcasecmp_start(arg+1, switchtext) == 0;
}

static Format parse_type(const char *arg) {
    for (size_t i = 0; i<ARRAY_ELEMS(convertible_type); ++i) {
        const TypeSwitch& type = convertible_type[i];
        if (is_abbrev_switch(arg, type.switchtext)) {
            return type.format;
        }
    }
    return UNKNOWN;
}

static Format parse_intype(const char *arg) {
    Format type = parse_type(arg);
    if (!is_input_format(type)) throw_errorf(65, "Unsupported input file type '%s'", arg);
    if (type == UNKNOWN) throw_errorf(67, "UNKNOWN input file type '%s'", arg);
    return type;
}

static Format parse_outtype(const char *arg) {
    Format type = parse_type(arg);
    if (type == UNKNOWN) throw_errorf(68, "UNKNOWN output file type '%s'", arg);
    return type;
}

static bool is_help_req(const char *arg) {
    return strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0;
}
static bool command_line_conversion(int argc, const char * const *argv, FormattedFile& in, FormattedFile& out) {
    for (int c = 1; c<argc; c++) {
        if (is_help_req(argv[c])) {
            show_command_line_usage();
            return false;
        }
    }

    if (argc != 5) throw_errorf(69, "arb_convert_aln expects exactly 4 parameters (you specified %i). Try '--help'", argc-1);

    in.init(argv[2], parse_intype(argv[1]));
    out.init(argv[4], parse_outtype(argv[3]));

    return true;
}

static void do_conversion(const FormattedFile& in, const FormattedFile& out) {
#ifdef CALOG
    fprintf(stderr, "\n\nConvert file %s to file %s.\n", in.name(), out.name());
#endif

    // check if output file exists and filename's validation
    valid_name_or_die(out.name());
    if (file_exists(out.name())) warningf(151, "Output file %s exists, will be overwritten.", out.name());

    // file format transfer...
    convert(in, out);
}

int ARB_main(int argc, char *argv[]) {
    int  exitcode        = EXIT_SUCCESS;
    bool use_arb_message = false;
    try {
        FormattedFile in;
        FormattedFile out;

        if (argc>1 && strcmp(argv[1], "--arb-notify") == 0) {
            use_arb_message = true;
            argc--; argv++;
        }

        if (argc < 2) {
            ask_for_conversion_params(in, out);
            do_conversion(in, out);
        }
        else {
            if (command_line_conversion(argc, argv, in, out)) {
                do_conversion(in, out);
            }
        }
    }
    catch (Convaln_exception& err) {
        fprintf(stderr, "ERROR(%d): %s\n", err.get_code(), err.get_msg());
        if (use_arb_message) {
            char *escaped = strdup(err.get_msg());
            for (int i = 0; escaped[i]; ++i) if (escaped[i] == '\"') escaped[i] = '\'';

            char *command        = strf("arb_message \"Error: %s (in arb_convert_aln; code=%d)\"", escaped, err.get_code());
            if (system(command) != 0) fprintf(stderr, "ERROR running '%s'\n", command);
            free(command);

            free(escaped);
        }
        exitcode = EXIT_FAILURE;
    }
    return exitcode;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_BASIC_switch_parsing() {
    TEST_EXPECT_ZERO(strcasecmp_start("GenBank", "GenBank"));
    TEST_EXPECT_ZERO(strcasecmp_start("GEnbaNK", "genBANK"));
    TEST_EXPECT_ZERO(strcasecmp_start("Ge", "GenBank"));
    TEST_EXPECT(strcasecmp_start("GenBank", "NEXUS") < 0);
    TEST_EXPECT(strcasecmp_start("NEXUS", "GenBank") > 0);

    TEST_REJECT(is_abbrev_switch("notAswitch", "notAswitch"));
    TEST_REJECT(is_abbrev_switch("-GenbankPlus", "Genbank"));
    TEST_REJECT(is_abbrev_switch("-Ge", "NEXUS"));

    TEST_EXPECT(is_abbrev_switch("-Ge", "Genbank"));
    TEST_EXPECT(is_abbrev_switch("-N", "NEXUS"));
    TEST_EXPECT(is_abbrev_switch("-NEXUS", "NEXUS"));

    TEST_EXPECT_EQUAL(parse_outtype("-PH"), PHYLIP);
    TEST_EXPECT_EQUAL(parse_outtype("-PHYLIP"), PHYLIP);
    TEST_EXPECT_EQUAL(parse_outtype("-phylip"), PHYLIP);
}

#endif // UNIT_TESTS
