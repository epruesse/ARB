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
#include <arb_defs.h>

struct TypeSwitch { const char *switchtext; int format_num; };

TypeSwitch known_in_type[] = { // see fconv.cxx@format_spec
    { "GenBank",   GENBANK   },
    { "EMBL",      EMBL      },
    { "AE2",       MACKE     },
    { "SwissProt", SWISSPROT },
};

TypeSwitch known_out_type[] = {
    { "GenBank",   GENBANK   },
    { "EMBL",      EMBL      },
    { "AE2",       MACKE     },
    { "NEXUS",     NEXUS     },
    { "PHYLIP",    PHYLIP    },
    { "PHYLIP2",   PHYLIP2   },
    { "GCG",       GCG       },
    { "PRINTABLE", PRINTABLE },
};

static void show_command_line_usage() {
    fputs("Command line usage:\n"
          "  $ arb_convert_aln -INFMT input_file -OUTFMT output_file\n"
          "  where\n"
          "      INFMT  may be 'GenBank', 'EMBL', 'AE2' or 'SwissProt' and\n"
          "      OUTFMT may be 'GenBank', 'EMBL', 'AE2', 'NEXUS', 'PHYLIP', 'GCG' or 'Printable'\n"
          "  (Note: you may abbreviate the format names)\n"
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

static void change_file_suffix(char *old_file, char *file_name, int type) {
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

static void ask_for_conversion_params(int& argc, char**& argv, int& intype, int& outtype) {
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
    switch (choice[0]) {
        case '\0': // [default]
        case '1': intype = GENBANK; break;
        case '2': intype = EMBL; break;
        case '3': intype = MACKE; break;
        case '4': intype = SWISSPROT; break;
        case '5': exit(0); // ok - interactive mode only
        default: throw_errorf(16, "Unknown input format selection '%s'", choice);
    }
    argv = (char **)calloc(1, sizeof(char *) * 5);

    fputs("\nInput file name? ", stderr);
    Getstr(temp, LINESIZE);
    argv[2] = nulldup(temp);

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
          "  (A)  PHYLIP2 (insert stdin in first line)\n"
          "  (6)  GCG\n"
          "  (7)  Printable\n"
          "  (8)  Quit\n"
          "  ? ", stderr);

    Getstr(choice, LINESIZE);
    switch (choice[0]) {
        case '1': outtype = GENBANK; break;
        case '2': outtype = EMBL; break;
        case '\0': // [default]
        case '3': outtype = MACKE; break;
        case '4': outtype = NEXUS; break;
        case '5': outtype = PHYLIP; break;
        case 'A': outtype = PHYLIP2; break;
        case '6': outtype = GCG; break;
        case '7': outtype = PRINTABLE; break;
        case '8': exit(0); // ok - interactive mode only
        default: throw_errorf(66, "Unknown output format selection '%s'", choice);
    }
    change_file_suffix(argv[2], temp, outtype);
    if (outtype != GCG) {
        fprintf(stderr, "\nOutput file name [%s]? ", temp);
        Getstr(temp, LINESIZE);
        if (str0len(temp) == 0)
            change_file_suffix(argv[2], temp, outtype);
    }
    argv[4] = nulldup(temp);
    argc = 5;
}

static int strcasecmp_start(const char *s1, const char *s2) {
    int cmp = 0;
    for (int p = 0; !cmp; p++) {
        cmp = tolower(s1[p])-tolower(s2[p]);
        if (!s1[p]) return 0;
    }
    return cmp;
}

static bool is_abbrev_switch(const char *arg, const char *switchtext)  {
    return arg[0] == '-' && strcasecmp_start(arg+1, switchtext) == 0;
}

static int parse_type(const char *arg, const TypeSwitch*const& known_type, size_t count) {
    for (size_t i = 0; i<count; ++i) {
        const TypeSwitch& type = known_type[i];
        if (is_abbrev_switch(arg, type.switchtext)) {
            return type.format_num;
        }
    }
    return UNKNOWN;
}

static int parse_intype(const char *arg) {
    int type = parse_type(arg, known_in_type, ARRAY_ELEMS(known_in_type));
    if (type == UNKNOWN) throw_errorf(67, "UNKNOWN input file type '%s'", arg);
    return type;
}

static int parse_outtype(const char *arg) {
    int type = parse_type(arg, known_out_type, ARRAY_ELEMS(known_out_type));
    if (type == UNKNOWN) throw_errorf(68, "UNKNOWN output file type '%s'", arg);
    return type;
}

static bool is_help_req(const char *arg) {
    return strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0;
}
static bool command_line_conversion(int argc, char** argv, int& intype, int& outtype) {
    for (int c = 1; c<argc; c++) {
        if (is_help_req(argv[c])) {
            show_command_line_usage();
            return false;
        }
    }

    if (argc != 5) throw_errorf(69, "arb_convert_aln expects exactly 4 parameters (you specified %i). Try '--help'", argc-1);

    intype  = parse_intype(argv[1]);
    outtype = parse_outtype(argv[3]);

    return true;
}

static void do_conversion(const char *inName, const char *ouName, int inType, int ouType) {
#ifdef CALOG
    if (ouType != GCG)
        fprintf(stderr, "\n\nConvert file %s to file %s.\n", inName, ouName);
    else
        fprintf(stderr, "\n\nConvert file %s to GCG files\n", inName);
#endif

    // check if output file exists and filename's validation
    valid_name_or_die(ouName);
    if (file_exists(ouName)) warningf(151, "Output file %s exists, will be overwritten.", ouName);

    // file format transfer...
    convert(inName, ouName, inType, ouType);
}

int main(int argc, char *argv[]) {
    int exitcode = EXIT_SUCCESS;
    try {
        int  intype  = UNKNOWN;
        int  outtype = UNKNOWN;

        if (argc < 2) {
            ask_for_conversion_params(argc, argv, intype, outtype); // modifies argc/argv!

            if (argc == 4) { // default output file
                const char **argv_new = (const char **)calloc(sizeof(char *), 5);

                memcpy(argv_new, argv, sizeof(char *) * 4);
                argv_new[4] = "";
                argv = (char **)argv_new;
            }
            do_conversion(argv[2], argv[4], intype, outtype);
        }
        else {
            if (command_line_conversion(argc, argv, intype, outtype)) {
                do_conversion(argv[2], argv[4], intype, outtype);
            }
        }
    }
    catch (Convaln_exception& err) {
        fprintf(stderr, "ERROR(%d): %s\n", err.error_code, err.error);
        exitcode = EXIT_FAILURE;
    }
    return exitcode;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <test_unit.h>

void TEST_BASIC_switch_parsing() {
    TEST_ASSERT(strcasecmp_start("GenBank", "GenBank") == 0);
    TEST_ASSERT(strcasecmp_start("GEnbaNK", "genBANK") == 0);
    TEST_ASSERT(strcasecmp_start("Ge", "GenBank") == 0);
    TEST_ASSERT(strcasecmp_start("GenBank", "NEXUS") < 0);
    TEST_ASSERT(strcasecmp_start("NEXUS", "GenBank") > 0);

    TEST_ASSERT(!is_abbrev_switch("notAswitch", "notAswitch"));
    TEST_ASSERT(!is_abbrev_switch("-GenbankPlus", "Genbank"));
    TEST_ASSERT(!is_abbrev_switch("-Ge", "NEXUS"));

    TEST_ASSERT(is_abbrev_switch("-Ge", "Genbank"));
    TEST_ASSERT(is_abbrev_switch("-N", "NEXUS"));
    TEST_ASSERT(is_abbrev_switch("-NEXUS", "NEXUS"));

    TEST_ASSERT_EQUAL(parse_outtype("-PH"), PHYLIP);
    TEST_ASSERT_EQUAL(parse_outtype("-PHYLIP"), PHYLIP);
    TEST_ASSERT_EQUAL(parse_outtype("-PHYLIP2"), PHYLIP2);
    TEST_ASSERT_EQUAL(parse_outtype("-phylip"), PHYLIP);
}

#endif // UNIT_TESTS
