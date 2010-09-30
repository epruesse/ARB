/* ------------------------------------------------------------ */
/*                                  */
/*  Format Conversion Program.              */
/*                              */
/*      Woese Lab., Dept. of Microbiology, UIUC     */
/*                              */
/* ------------------------------------------------------------ */
/* if using Dec. Unix, <sys/time.h> doesn't not exist */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "convert.h"
#include "global.h"
#include "types.h"

/* ---------------------------------------------------------------
 *  Main program:
 *      File conversion;  convert from one file format to
 *      the other.  eg.  Genbank<-->Macke format.
 */
static int main_WRAPPED(int argc, char *argv[]) {
    int  intype, outtype;
    char temp[LINENUM];
    char choice[LINENUM];

    intype = outtype = UNKNOWN;
    if (argc < 3 || argv[1][0] != '-') {
        fputs("---------------------------------------------------------------\n"
              "\n"
              "  convert_aln - an alignment and file converter written by\n"
              "  WenMin Kuan for the RDP database project.\n"
              "\n"
              "  Modified for use in ARB by Ralf Westram\n"
              "  Report errors or deficiencies to devel@arb-home.de\n"
              "\n"
              "  Command line usage:\n"
              "\n"
              "  $ arb_convert_aln -infmt input_file -outfmt output_file\n"
              "  where\n"
              "  infmt  may be GenBank, EMBL, AE2 or SwissProt and\n"
              "  outfmt may be GenBank, EMBL, AE2, PAUP, PHYLIP, GCG or Printable\n"
              "\n"
              "---------------------------------------------------------------\n"
              "\n"
              "Select input format (<CR> means default)\n"
              "\n"
              "  (1)  GenBank [default]\n"
              "  (2)  EMBL\n"
              "  (3)  AE2\n"
              "  (4)  SwissProt\n"
              "  (5)  Quit\n"
              "  ? ", stderr);

        Getstr(choice, LINENUM);
        switch (choice[0]) {
            case '\0': // [default]
            case '1':
                intype = GENBANK;
                break;
            case '2':
                intype = EMBL;
                break;
            case '3':
                intype = MACKE;
                break;
            case '4':
                intype = PROTEIN;
                break;
            case '5':
                exit(0); // ok - interactive mode only
            default:
                throw_errorf(16, "Unknown input format selection '%s'", choice);
        }
        argv = (char **)calloc(1, sizeof(char *) * 5);

        fprintf(stderr, "\nInput file name? ");
        Getstr(temp, LINENUM);
        argv[2] = Dupstr(temp);
        if (!file_exists(temp))
            throw_error(77, "Input file not found");

        /* output file information */
        fputs("\n"
              "Select output format (<CR> means default)\n"
              "\n"
              "  (1)  GenBank\n"
              "  (2)  EMBL\n"
              "  (3)  AE2 [default]\n"
              "  (4)  PAUP\n"
              "  (5)  PHYLIP\n"
              "  (A)  PHYLIP2 (insert stdin in first line)\n"
              "  (6)  GCG\n"
              "  (7)  Printable\n"
              "  (8)  Quit\n"
              "  ? ", stderr);

        Getstr(choice, LINENUM);
        switch (choice[0]) {
            case '1':
                outtype = GENBANK;
                break;
            case '2':
                outtype = EMBL;
                break;
            case '\0': // [default]
            case '3':
                outtype = MACKE;
                break;
            case '4':
                outtype = PAUP;
                break;
            case '5':
                outtype = PHYLIP;
                break;
            case 'A':
                outtype = PHYLIP2;
                break;
            case '6':
                outtype = GCG;
                break;
            case '7':
                outtype = PRINTABLE;
                break;
            case '8':
                exit(0); // ok - interactive mode only
            default:
                throw_errorf(66, "Unknown output format selection '%s'", choice);
        }
        change_file_suffix(argv[2], temp, outtype);
        if (outtype != GCG) {
            fprintf(stderr, "\nOutput file name [%s]? ", temp);
            Getstr(temp, LINENUM);
            if (Lenstr(temp) == 0)
                change_file_suffix(argv[2], temp, outtype);
        }
        argv[4] = Dupstr(temp);
        argc = 5;
    }
    else { /* process command line */
        /* input file */
        if      (argv[1][1] == 'G' || argv[1][1] == 'g') intype = GENBANK;
        else if (argv[1][1] == 'E' || argv[1][1] == 'e') intype = EMBL;
        else if ((argv[1][1] == 'A' || argv[1][1] == 'a') && (argv[1][2] == 'E' || argv[1][2] == 'e')) intype = MACKE;
        else if (argv[1][1] == 'S' || argv[1][1] == 's') intype = PROTEIN;
        else throw_error(67, "UNKNOWN input file type");

        /* output file */
        if ((argv[3][1] == 'G' || argv[3][1] == 'g') && (argv[3][2] == 'E' || argv[3][2] == 'e')) outtype = GENBANK;
        else if ((argv[3][1] == 'E' || argv[3][1] == 'e')) outtype = EMBL;
        else if ((argv[3][1] == 'A' || argv[3][1] == 'a') && (argv[3][2] == 'E' || argv[3][2] == 'e')) outtype = MACKE;
        else if ((argv[3][1] == 'P' || argv[3][1] == 'p') && (argv[3][2] == 'a' || argv[3][2] == 'A')) outtype = PAUP;
        else if (!strncasecmp(argv[3] + 1, "ph", 2)) {
            if (!strcasecmp(argv[3] + 1, "phylip2")) {
                outtype = PHYLIP2;
            }
            else {
                outtype = PHYLIP;
            }
        }
        else if ((argv[3][1] == 'G' || argv[3][1] == 'g') && (argv[3][2] == 'C' || argv[3][2] == 'c')) outtype = GCG;
        else if ((argv[3][1] == 'P' || argv[3][1] == 'p') && (argv[3][2] == 'R' || argv[3][2] == 'r')) outtype = PRINTABLE;
        else throw_error(68, "UNKNOWN output file file");
    } 

    if (argc == 4) {            /* default output file */
        const char **argv_new = (const char **)calloc(sizeof(char *), 5);

        memcpy(argv_new, argv, sizeof(char *) * 4);
        argv_new[4] = "";
        argv = (char **)argv_new;
    }

#ifdef log
    if (outtype != GCG)
        fprintf(stderr, "\n\nConvert file %s to file %s.\n", argv[2], argv[4]);
    else
        fprintf(stderr, "\n\nConvert file %s to GCG files\n", argv[2]);
#endif

    /* check if output file exists and filename's validation */
    if (file_exists(argv[4])) {
        warningf(151, "Output file %s exists, will be overwritten.", argv[4]);
    }

    /* file format transfer... */
    convert(argv[2], argv[4], intype, outtype);
    return 0;

}

int main(int argc, char *argv[]) {
    int exitcode = EXIT_FAILURE;
    try {
        exitcode = main_WRAPPED(argc, argv);
    }
    catch (Convaln_exception& err) {
        fprintf(stderr, "ERROR(%d): %s\n", err.error_code, err.error);
    }
    return exitcode;
}

/* ---------------------------------------------------------------
 *  Function file_type
 *      According to the first line in the file to decide
 *      the file type.  File type could be Genbank, Macke,...
 */
int file_type(char *filename) {
    char token[LINENUM];
    FILE *fp = open_input_or_die(filename);

    fscanf(fp, "%s", token);
    fclose(fp);

    if (str_equal(token, "LOCUS"))
        return (GENBANK);
    else if (str_equal(token, "#-"))
        return (MACKE);
    else if (str_equal(token, "ID"))
        return (PROTEIN);
    else if (str_equal(token, "#NEXUS"))
        return (PAUP);
    else if (isnum(token))
        return (PHYLIP);
    else
        return (UNKNOWN);
}

/* -----------------------------------------------------------------
 *  Function isnum().
 *      Return TRUE if the string is an integer number.
 */
int isnum(char *string)
{
    int indi, length, flag;

    for (indi = 0, length = Lenstr(string), flag = 1; flag && indi < length; indi++)
        if (!isdigit(string[indi]))
            flag = 0;

    return (flag);
}

/* ------------------------------------------------------------------
 *  Function file_exists().
 *      Check if file is already existed and also check file
 *          name is valid or not.
 */
bool file_exists(char *file_name) {
    if (Lenstr(file_name) <= 0) {
        throw_errorf(152, "illegal file name: %s", file_name);
    }

    FILE *ifp    = fopen(file_name, "r");
    bool  exists = ifp != NULL;
    if (ifp) fclose(ifp);

    return exists;
}

/* -------------------------------------------------------------------
 *  Function change_file_suffix().
 *      Define the default file name by changing suffix.
 */
void change_file_suffix(char *old_file, char *file_name, int type)
{
    int indi, indj;

    for (indi = Lenstr(old_file) - 1; indi >= 0 && old_file[indi] != '.'; indi--)
        if (indi == 0)
            Cpystr(file_name, old_file);
        else {
            for (indj = 0; indj < (indi - 1); indj++)
                file_name[indj] = old_file[indj];
            file_name[indj] = '\0';
        }
    switch (type) {
        case GENBANK:
            Catstr(file_name, ".GB");
            break;
        case MACKE:
            Catstr(file_name, ".aln");
            break;
        case PAUP:
            Catstr(file_name, ".PAUP");
            break;
        case PHYLIP:
            Catstr(file_name, ".PHY");
            break;
        case EMBL:
            Catstr(file_name, ".EMBL");
            break;
        case PRINTABLE:
            Catstr(file_name, ".prt");
            break;
        default:
            Catstr(file_name, ".???");
    }
}
