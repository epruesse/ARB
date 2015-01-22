//**********************************************************************
/*                                                                      */
// Description: Ascii to PostScript printer program.
// File: bounty:/archive/src/a2ps/Last/a2ps.c
// Created: Fri Nov 5 8:20 1993 by miguel@bountyimag.fr (Miguel Santana)
// Version: 4.3
/*                                                                      */
// Edit history:
// 1) Derived of shell program written by evan@csli (Evan Kirshenbaum).
//    Written in C for improve speed execution and portability. Many
//    improvements have been added.
// Fixes by Oscar Nierstrasz @ cui.uucp:
// 2) Fixed incorrect handling of stdin (removed error if no file names)
// 3) Added start_page variable to eliminate blank pages printed for
//      files that are exactly multiples of 132 lines (e.g., man pages)
// Modified by santana@imag.fr:
// 4) Added new options at installation : sheet format (height/width in
//    inches), page format (number of columns per line and of lines per
//    page).
// Modified by santana@imag.fr:
// 5) Added new option to print n copies of a same document.
// 6) Cut long filenames if don't fit in the page header.
// Modified by Tim Clark (T.Clark@warwick.ac.uk):
// 7) Two additional modes of printing (portrait and wide format modes)
// 8) Fixed to cope with filenames which contain a character which must
//    be escaped in a PostScript string.
// Modified by santana@imag.fr to
// 9) Added new option to suppress heading printing.
// 10) Added new option to suppress page surrounding border printing.
// 11) Added new option to change font size. Lines and columns are
//     automatically adjusted, depending on font size and printing mode
// 12) Minor changes (best layout, usage message, etc).
// Modified by tullemans@apolloway.prl.philips.nl
// 13) Backspaces (^H) are now handled correctly.
// Modified by Johan Vromans (jv@mh.nl) to
// 14) Added new option to give a header title that replaces use of
//     filename.
// Modified by craig.r.stevenson@att.com to
// 15) Print last modification date/time in header
// 16) Printing current date/time on left side of footer (optional)
// Modified by erikt@cs.umu.se:
// 17) Added lpr support for the BSD version
// 18) Added som output of pages printed.
// Modified by wstahw@lso.win.tue.nl:
// 19) Added option to allowing the printing of 2 files in one sheet
// Modified by mai@wolfen.cc.uow.oz
// 20) Added an option to set the lines per page to a specified value.
// 21) Added support for printing nroff manuals
// Modified by santana@imag.fr
// 22) Integration of changes.
// 23) No more standard header file (printed directly by a2ps).
// 24) New format for command options.
// 25) Other minor changes.
// Modified by Johan Garpendahl (garp@isy.liu.se) and santana@imag.fr:
// 26) Added 8-bit characters printing as ISO-latin 1 chars
// Modified by John Interrante (interran@uluru.stanford.edu) and
// santana@imag.fr:
// 27) Two pages per physical page in portrait mode
// Modified by santana@imag.fr:
// 28) New option for two-sided printing
// 29) Several fixes
// Modified by Chris Adamo (adamo@ll.mit.edu) and
//     Larry Barbieri (lbarbieri@ll.mit.edu) 3/12/93
// 30) Output format enhancements.
// 31) Added login_id flag (for SYSV and BSD only) for printing user's
//     login ID at top of page.  Added command line parameter (-nL) to
//     suppress this feature.
// 33) Added filename_footer flag for printing file name at bottom
//     of page.  Added command line parameter (-nu) to suppress this
//     feature.
// 34) Added -B (-nB) options to enable (disable) bold font
// Modified by santana@imag.fr:
// 35) Adapted to respect Adobe conventions for page independence. A2ps
//     output can be now used by other Postscript processors.
// 36) Names of most postscript variables have been coded in order to
//     reduce the size of the output.
// 37) Ansi C compilers are now automatically taken into account.
// 38) Enhanced routine for cutting long filenames
// 39) Added -q option to print files in quiet mode (no summary)
// 40) Fixed some little bugs (counters, modification time for stdin,
//     character separator when printing line numbers and cutting a
//     line).
// 41) Some minor changes (new preprocessing variables, formatting)
/*                                                                      */
//**********************************************************************

/*
 * Copyright (c) 1993, 1994, Miguel Santana, M.Santana@frgu.bull.fr
 *
 * Permission is granted to use, copy, and distribute this software for
 * non commercial use, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code. Please report bugs and changes to
 * M.Santana@frgu.bull.fr
 *
 * This software is provided "as is" without express or implied warranty.
 */


//**********************************************************************
/*                                                                      */
//                      I n c l u d e   f i l e s
/*                                                                      */
//**********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef __STDC__
#include <stdlib.h>
#include <time.h>
#include <string.h>
#else
#include <strings.h>
#ifdef SYSV
#include <sys/timeb.h>
#include <time.h>
#include <sys/utsname.h>
#else
#ifndef BSD
#define BSD     1
#endif
#include <sys/time.h>
#endif
#endif


//**********************************************************************
/*                                                                      */
//           P r e p r o c e s s i n g   d e f i n i t i o n s
/*                                                                      */
//**********************************************************************

/*
 * Common definitions
 */
#define FALSE           0
#define TRUE            1
#ifndef NULL
#define NULL            0
#endif
#ifndef NUL
#define NUL             '\0'
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS    0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE    1
#endif


/*
 * Version
 */
#define VERSION "4.3"
#define LPR_PRINT 1

/*
 * Default page dimensions
 */
#ifndef WIDTH
#define WIDTH   8.27
#endif

#ifndef HEIGHT
#define HEIGHT  11.0
#endif

#ifndef MARGIN
#define MARGIN  .5
#endif


/*
 * Pathname separator for file system
 */
#ifndef DIR_SEP
#define DIR_SEP '/'
#endif


/*
 * Printing parameters
 */
#if LPR_PRINT

#ifndef LPR_COMMAND
#define LPR_COMMAND     "lpr"
#endif

#ifndef LPR_OPT
#define LPR_OPT "-l"
#endif

#if defined(ONESIDED) && defined(TWOSIDED)
#define RECTO_VERSO_PRINTING
#ifndef TWOSIDED_DFLT
#define TWOSIDED_DFLT   TRUE
#endif
#endif

#endif


/*
 * Configuration values
 */
#define PORTRAIT_HEADER         0.29
#define LANDSCAPE_HEADER        0.22
#define PIXELS_INCH             72
#define MAXFILENAME             32
#define MAX_LINES               320             // max. lines per page
#define MAN_LINES               66              // no lines for a man
#define IS_ROMAN                0               // normal font
#define IS_BOLD                 1               // bold sequence flag
#if defined(SYSV) || defined(BSD)
#define MAX_HOSTNAME            40
#endif


//**********************************************************************
/*                                                                      */
//                 G l o b a l   d e f i n i t i o n s
/*                                                                      */
//**********************************************************************


/*
 * Global types
 */
typedef enum { BOLD, NORMAL } WEIGHT;           // font weights


/*
 * Function declarations.
 */

static void print_page_prologue(int side);
static void print_standard_prologue(char *datestring);
static void startpage();
static void endpage();

#if defined(SYSV) || defined(BSD)
char *getlogin();
#endif
#if defined(BSD)
int   gethostname(char *name, int namelen);
#endif

/*
 * Flags related to options.
 */
static int numbering = FALSE;          // Line numbering option
static int folding = TRUE;             // Line folding option
static int restart = FALSE;            // Don't restart page number after each file
static int only_printable = FALSE;     // Replace non printable char by space
static int interpret = TRUE;           // Interpret TAB, FF and BS chars option
static int print_binaries = FALSE;     // Force printing for binary files
static int landscape = TRUE;           // Otherwise portrait format sheets
static int new_landscape = TRUE;       // To scrute changes of landscape option
static int twinpages = TRUE;           // 2 pages per sheet if true, 1 otherwise
static int new_twinpages = TRUE;       // To scrute changes of twinpages option
static int twinfiles = FALSE;          // Allow 2 files per sheet
static int no_header = FALSE;          // TRUE if user doesn't want the header
static int no_border = FALSE;          // Don't print the surrounding border ?
static int printdate = FALSE;          // Print current date as footnote
static int filename_footer = TRUE;     // Print file name at bottom of page
static int no_summary = FALSE;         // Quiet mode?
static WEIGHT fontweight = NORMAL;     // Control font weight: BOLD or NORMAL
static WEIGHT new_fontweight = NORMAL; // To scrute changes of bold option
#if defined(SYSV) || defined(BSD)
int login_id = TRUE;            // Print login ID at top of page
#endif
#if LPR_PRINT
static int lpr_print = TRUE;           // Fork a lpr process to do the printing
#ifdef RECTO_VERSO_PRINTING
int rectoverso = TWOSIDED_DFLT; // Two-side printing
#endif
#endif
static int ISOlatin1 = FALSE;          // Print 8-bit characters?


/*
 * Counters of different kinds.
 */
static int column = 0;                 // Column number (in current line)
static int line = 0;                   // Line number (in current page)
static int line_number = 0;            // Source line number
static int pages = 0;                  // Number of logical pages printed
static int sheets = 0;                 // Number of physical pages printed
static int old_pages, old_sheets;      // Value before printing current file
static int sheetside = 0;              // Side of the sheet currently printing
static int linesperpage;               // Lines per page
static int lines_requested = 0;        // Lines per page requested by the user
static int new_linesrequest = 0;       // To scrute new values for lines_requested
static int columnsperline;             // Characters per output line
static int nonprinting_chars, chars;   // Number of nonprinting and total chars
static int copies_number = 1;          // Number of copies to print
static int column_width = 8;           // Default column tab width (8)


/*
 * Other global variables.
 */
static int first_page;                 // First page for a file
static int no_files = TRUE;            // No file until now
static int prefix_width;               // Width in characters for line prefix
static float fontsize = 0.0;           // Size of a char for body font
static float new_fontsize = 0.0;       // To scrute new values for fontsize
static char *command;                  // Name of a2ps program
static char *lpr_opt = NULL;           // Options to lpr
static char *header_text = NULL;       // Allow for different header text
static float header_size;              // Size of the page header
static char *prologue = NULL;          // postscript header file
static char current_filename[MAXFILENAME+1];   // Name of the file being printed
static char currentdate[18];           // Date for today
static char filedate[18];              // Last modification time for current file
#if defined(SYSV) || defined(BSD)
char *login = NULL;             // user's login name and host machine
#endif


/*
 * Sheet dimensions
 */
static double page_height = HEIGHT;    // Paper height
static double page_width = WIDTH;      // Paper width


//**********************************************************************
/*                                                                      */
/*                                                                      */
//**********************************************************************

/*
 * Print a usage message.
 */
static void usage(int failure) {
    // failure: Must we exit with a failure code?
    fprintf(stderr, "A2ps v%s usage: %s [pos. or global options] [ f1 [ [pos. options] f2 ...] ]\n", VERSION, command);
    fprintf(stderr, "pos.   =  -#num\t\tnumber of copies to print\n");
    fprintf(stderr, "          -1\t\tone page per sheet\n");
    fprintf(stderr, "          -2\t\tTWIN PAGES per sheet\n");
    fprintf(stderr, "          -d\t-nd\tprint (DON'T PRINT) current date at the bottom\n");
    fprintf(stderr, "          -Fnum\t\tfont size, num is a float number\n");
    fprintf(stderr, "          -Hstr\t\tuse str like header title for subsequent files\n");
#if defined(SYSV) || defined(BSD)
    fprintf(stderr, "          \t-nL\tdon't print login ID on top of page\n");
#endif
    fprintf(stderr, "          -l\t\tprint in LANDSCAPE mode\n");
    fprintf(stderr, "          -lnum\t\tuse num lines per page\n");
    fprintf(stderr, "          -m\t\tprocess the file as a man\n");
    fprintf(stderr, "          -n\t-nn\tNUMBER (don't number) line files\n");
    fprintf(stderr, "          -p\t\tprint in portrait mode\n");
    fprintf(stderr, "          -q\t\tprint in quiet mode (no summary)\n");
    fprintf(stderr, "          -s\t-ns\tPRINT (don't print) surrounding borders\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "global =  -?\t\tprint this information\n");
    fprintf(stderr, "          -B\t-nB\tprint (DON'T PRINT) in bold font\n");
    fprintf(stderr, "          -b\t-nb\tforce (DON'T FORCE) binary printing\n");
    fprintf(stderr, "          -c\t-nc\tallow (DON'T ALLOW) two files on the same sheet\n");
    fprintf(stderr, "          -f\t-nf\tFOLD (don't fold) lines\n");
    fprintf(stderr, "          \t-nH\tdon't print any header\n");
    fprintf(stderr, "          -h\t\tprint this information\n");
    fprintf(stderr, "          -Ifile\tinclude this file as a2ps prologue\n");
    fprintf(stderr, "          -i\t-ni\tINTERPRET (don't interpret) tab, bs and ff chars\n");
#if LPR_PRINT
    fprintf(stderr, "          -Pprinter -nP\tSEND (don't send) directly to the printer");
#ifdef LPR_OPT
    if (LPR_OPT != NULL && sizeof(LPR_OPT) > 0)
        fprintf(stderr, "\n\t\t\t(with options '%s' and -Pprinter)", LPR_OPT);
#endif
    fprintf(stderr, "\n");
#endif
    fprintf(stderr, "          -r\t-nr\tRESTART (don't restart) page number after each file\n");
#ifdef RECTO_VERSO_PRINTING
#ifdef TWOSIDED_DFLT
    fprintf(stderr, "          -s1\t-s2\tone-sided (TWO-SIDED) printing\n");
#else
    fprintf(stderr, "          -s1\t-s2\tONE-SIDED (two-sided) printing\n");
#endif
#endif
    fprintf(stderr, "          -tnum\t\tset tab size to n\n");
    fprintf(stderr, "          \t-nu\tdon't print a filename footer\n");
    fprintf(stderr, "          -v\t-nv\tVISIBLE (blank) display of unprintable chars\n");
    fprintf(stderr, "          -8\t-n8\tdisplay (DON'T DISPLAY) 8-bit chars\n");
    exit(failure);
}

/*
 * Set an option only if it's global.
 */
static void set_global_option(char *arg) {
    switch (arg[1]) {
        case '?':                               // help
        case 'h':
            usage(EXIT_SUCCESS);
        case 'b':                               // print binary files
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            print_binaries = TRUE;
            break;
        case 'c':                               // allow two files per sheet
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            twinfiles = TRUE;
            break;
        case 'f':                               // fold lines too large
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            folding = TRUE;
            break;
        case 'I':                               // include this file as a2ps prologue
            if (arg[2] == NUL)
                usage(EXIT_FAILURE);
            prologue = arg+2;
            break;
        case 'i':                               // interpret control chars
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            interpret = TRUE;
            break;
        case 'n':
            if (arg[2] == NUL)
                return;
            if (arg[3] != NUL)
                usage(EXIT_FAILURE);
            switch (arg[2]) {
                case 'b':                       // don't print binaries
                    print_binaries = FALSE;
                    break;
                case 'c':                       // don't allow 2 files/sheet
                    twinfiles = FALSE;
                    break;
                case 'f':                       // cut lines too long
                    folding = FALSE;
                    break;
                case 'H':                       // don't print header
                    no_header = TRUE;
                    break;
                case 'i':                       // don't interpret ctrl chars
                    interpret = FALSE;
                    break;
#if LPR_PRINT
                case 'P':                       // don't lpr
                    lpr_print = FALSE;
                    break;
#endif
                case 'r':                       // don't restart sheet number
                    restart = FALSE;
                    break;
                case 'v':                       // only printable chars
                    only_printable = TRUE;
                    break;
                case '8':                       // don't print 8-bit chars
                    ISOlatin1 = FALSE;
                    break;
                case 'B':
                case 'd':
                case 'L':
                case 'm':
                case 'n':
                case 's':
                case 'u':
                    if (arg[3] != NUL)
                        usage(EXIT_FAILURE);
                    return;
                default:
                    usage(EXIT_FAILURE);
            }
            break;
#if LPR_PRINT
        case 'P':                                       // fork a process to print
            if (arg[2] != NUL) {
                lpr_opt = (char *)malloc(strlen(arg)+1);
                strcpy(lpr_opt, arg);
            }
            lpr_print = TRUE;
            break;
#endif
        case 'q':                                       // don't print a summary
            no_summary = TRUE;
            break;
        case 'r':                                       // restart sheet number
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            restart = TRUE;
            break;
        case 's':
            if (arg[2] == NUL)
                return;
#ifdef RECTO_VERSO_PRINTING
            if (arg[3] == NUL) {
                if (arg[2] == '1') {            // one-sided printing
                    rectoverso = FALSE;
                    break;
                }
                if (arg[2] == '2') {            // two-sided printing
                    rectoverso = TRUE;
                    break;
                }
            }
#endif
            usage(EXIT_FAILURE);
            break;
        case 't':                               // set tab size
            if (arg[2] == NUL || (column_width = atoi(arg+2)) <= 0)
                usage(EXIT_FAILURE);
            break;
        case 'v':                               // print control chars
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            only_printable = FALSE;
            break;
        case '8':                               // print 8-bit chars
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            ISOlatin1 = TRUE;
            break;
        case '1':
        case '2':
        case 'B':
        case 'd':
        case 'm':
        case 'p':
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
        case '#':
        case 'F':
        case 'H':
        case 'l':
            return;
        default:
            usage(EXIT_FAILURE);
    }
    arg[0] = NUL;
}

/*
 * Set an option of the command line. This option will be applied
 * to all files that will be found in the rest of the command line.
 * The -H option is the only exception: it is applied only to the
 * file.
 */
static void set_positional_option(char *arg) {
    int copies;
    int lines;
    float size;

    switch (arg[1]) {
        case NUL:                               // global option
            break;
        case '#':                               // n copies
            if (sscanf(&arg[2], "%d", &copies) != 1 || copies <= 0)
                fprintf(stderr, "Bad number of copies: '%s'. Ignored\n", &arg[2]);
            else
                copies_number = copies;
            printf("/#copies %d def\n", copies_number);
            break;
        case '1':                               // 1 logical page per sheet
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            new_twinpages = FALSE;
            break;
        case '2':                               // twin pages
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            new_twinpages = TRUE;
            break;
        case 'B':
            new_fontweight = BOLD;              // use bold font
            break;
        case 'd':                               // print current date/time
            printdate = TRUE;
            break;
        case 'F':                               // change font size
            if (arg[2] == NUL || sscanf(&arg[2], "%f", &size) != 1 || size == 0.0) {
                fprintf(stderr, "Wrong value for option -F: '%s'. Ignored\n",
                        &arg[2]);
                break;
            }
            new_fontsize = size;
            break;
        case 'H':                               // header text
            header_text = arg+2;
            break;
        case 'l':
            if (arg[2] == NUL) {                // landscape format
                new_landscape = TRUE;
                break;
            }
            // set lines per page
            // Useful with preformatted files. Scaling is automatically
            // done when necessary.
            if (sscanf(&arg[2], "%d", &lines) != 1
                || lines < 0 || lines > MAX_LINES)
            {
                fprintf(stderr, "Wrong value for option -l: '%s'. Ignored\n",
                        &arg[2]);
                break;
            }
            new_linesrequest = lines;
            break;
        case 'm':                               // Process file as a man
            new_linesrequest = MAN_LINES;
            numbering = FALSE;
            break;
        case 'n':                               // number file lines
            if (arg[2] == NUL) {
                numbering = TRUE;
                break;
            }
            switch (arg[2]) {
                case 'B':                       // disable bold text
                    new_fontweight = NORMAL;
                    break;
                case 'd':                       // don't print date/time
                    printdate = FALSE;
                    break;
#if defined(SYSV) || defined(BSD)
                case 'L':                       // no login name in footer
                    login_id = FALSE;
                    break;
#endif
                case 'l':                       // portrait format
                    new_landscape = FALSE;
                    break;
                case 'm':                       // stop processing as a man
                    new_linesrequest = 0;
                    break;
                case 'n':                       // don't number lines
                    numbering = FALSE;
                    break;
                case 'p':                       // landscape format
                    new_landscape = TRUE;
                    break;
                case 's':                       // no surrounding border
                    no_border = TRUE;
                    break;
                case 'u':                       // no filename in footer
                    filename_footer = FALSE;
                    break;
                default:
                    usage(EXIT_FAILURE);
            }
            break;
        case 'p':                               // portrait format
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            new_landscape = FALSE;
            break;
        case 's':                               // surrounding border
            if (arg[2] != NUL)
                usage(EXIT_FAILURE);
            no_border = FALSE;
            break;
        default:
            usage(EXIT_FAILURE);
    }
}


//**************************************************************
//                      Service routines
//**************************************************************

/*
 * This routine buffers a line of input, release one character at a time
 * or a whole sequence of characters with some meaning like bold sequences
 * produced by nroff (no others sequences are recognized by the moment):
 *        <c><\b><c><\b><c><\b><c>
 */
static int mygetc(int *statusp) {
#define BUFFER_SIZE     512
    static int curr = 0;
    static int size = 0;
    static unsigned char buffer[BUFFER_SIZE+1];
    int c;

    *statusp = IS_ROMAN;

    // Read a new line, if necessary
    if (curr >= size) {
        if (fgets((char *)buffer, BUFFER_SIZE+1, stdin) == NULL)
            return  EOF;
        size = strlen((char *)buffer);
        if (size < BUFFER_SIZE && buffer[size-1] != '\n') {
            buffer[size] = '\n';
            buffer[++size] = '\0';
        }
        curr = 0;
    }
    if (buffer[curr+1] != '\b')         // this is not a special sequence
        return  buffer[curr++];

    // Check if it is a bold sequence
    c = buffer[curr++];
    if (c               == buffer[curr+1] &&
        buffer[curr]    == buffer[curr+2] &&
        c               == buffer[curr+3] &&
        buffer[curr]    == buffer[curr+4] &&
        c               == buffer[curr+5])
    {
        *statusp = IS_BOLD;
        curr += 6;
    }

    // Return the first character of the sequence
    return  c;
}

/*
 * Test if we have a binary file.
 */
static int is_binaryfile(char *name) {
    if (chars > 120 || pages > 1) {
        first_page = FALSE;
        if (chars && !print_binaries && (nonprinting_chars*100 / chars) >= 60) {
            fprintf(stderr, "%s is a binary file: printing aborted\n", name);
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Cut long filenames.
 */
static void cut_filename(char *old_name, char *new_name) {
    char *p;
    int   i;
    char *separator;

    if ((i = strlen(old_name)) <= MAXFILENAME) {
        strcpy(new_name, old_name);
        return;
    }
    p = old_name + (i-1);
    separator = NULL;
    i = 1;
    while (p >= old_name && i < MAXFILENAME) {
        if (*p == DIR_SEP)
            separator = p;
        p--;
        i++;
    }
    if (separator != NULL)
        p = separator;
    else if (p >= old_name)
        while (p >= old_name && *p != DIR_SEP) p--;

    for (i = 0, p++; *p != NUL; i++)
        *new_name++ = *p++;
    *new_name = NUL;
}

/*
 * Print a char in a form accepted by postscript printers.
 */
static int printchar(unsigned char c) {

    if (c >= ' ' && c < 0177) {
        if (c == '(' || c == ')' || c == '\\')
            putchar('\\');
        putchar(c);
        return 0;
    }

    if (ISOlatin1 && (c > 0177)) {
        printf("\\%o", c);
        return 0;
    }

    if (only_printable) {
        putchar(' ');
        return 1;
    }

    if (c > 0177) {
        printf("M-");
        c &= 0177;
    }
    if (c < ' ') {
        putchar('^');
        if ((c = c + '@') == '(' || c == ')' || c == '\\')
            putchar('\\');
        putchar(c);
    }
    else if (c == 0177)
        printf("^?");
    else {
        if (c == '(' || c == ')' || c == '\\')
            putchar('\\');
        putchar(c);
    }

    return 1;
}

/*
 * Begins a new logical page.
 */
static void skip_page() {
    if (twinpages == FALSE || sheetside == 0) {
        printf("%%%%Page: %d %d\n", sheets+1, sheets+1);
        printf("/pagesave save def\n");
        // Reinitialize state variables for each new sheet
        print_page_prologue(0);
    }
    startpage();
}

/*
 * Fold a line too long.
 */
static int fold_line(char *name) {
    column = 0;
    printf(") s\n");
    if (++line >= linesperpage) {
        endpage();
        skip_page();
        if (first_page && is_binaryfile(name))
            return FALSE;
        line = 0;
    }
    if (numbering)
        printf("(    +");
    else
        printf("( ");

    return TRUE;
}

/*
 * Cut a textline too long to the size of a page line.
 */
static int cut_line() {
    int c;
    int status;

    while ((c = mygetc(&status)) != EOF && c != '\n' && c != '\f') ;
    return c;
}


//**************************************************************
//                      "Postscript" routines.
//**************************************************************

/*
 * Print a physical page.
 */
static void printpage() {
    sheetside = 0;
    sheets++;
    printf("/sd 0 def\n");
    if (no_border == FALSE)
        printf("%d sn\n", sheets - (restart ? old_sheets : 0));
    if (printdate)
        printf("cd\n");
    if (filename_footer && landscape)
        printf("fnf\n");
#if defined(SYSV) || defined(BSD)
    if (login_id)
        printf("lg lgp\n");
#endif
    printf("pagesave restore\n");
    printf("showpage\n");
}

/*
 * Prints page header and page border and
 * initializes printing of the file lines.
 */
void startpage() {
    if (sheetside == 0) {
#ifdef RECTO_VERSO_PRINTING
        if (rectoverso && (sheets & 0x1)) {
            // Shift to left backside pages.
            printf("rm neg 0 translate\n");
        }
#endif
        if (landscape) {
            printf("sw 0 translate\n");
            printf("90 rotate\n");
        }
    }
    pages++;
    if (no_header == FALSE)
        printf("%d hp\n", pages - old_pages);
    if (no_border == FALSE) {
        printf("border\n");
        if (no_header == FALSE)
            printf("hborder\n");
    }
    printf("/x0 x %d get bm add def\n", sheetside);
    printf("/y0 y %d get bm bfs add %s add sub def\n",
           sheetside, no_header ? "0" : "hs");
    printf("x0 y0 moveto\n");
    printf("bf setfont\n");
}

/*
 * Terminates printing, flushing last page.
 */
static void cleanup() {
    if (twinpages && sheetside == 1)
        printpage();
#ifdef RECTO_VERSO_PRINTING
    if (!twinfiles && rectoverso && (sheets & 0x1) != 0) {
        sheetside = 0;
        sheets++;
        printf("%%%%Page: %d %d\n", sheets, sheets);
        printf("showpage\n");
    }
#endif
}

/*
 * Adds a sheet number to the page (footnote) and prints the formatted
 * page (physical impression). Activated at the end of each source page.
 */
void endpage() {
    if (twinpages && sheetside == 0) {
        sheetside = 1;
        printf("/sd 1 def\n");
    }
    else
        printpage();
}


//**************************************************************
//              Printing a file
//**************************************************************

/*
 * Print the file prologue.
 */
static void init_file_printing(char *name, char *title) {
    int new_format, new_font;
    char *string;
    int lines;
    float char_width;
    struct stat statbuf;

    // Print last page of previous file, if necessary
    if (pages > 0 && !twinfiles)
        cleanup();

    // Initialize variables related to the format
    new_format = FALSE;
    if (new_landscape != landscape || new_twinpages != twinpages) {
        landscape = new_landscape;
        twinpages = new_twinpages;
        new_format = TRUE;
    }

    // Initialize variables related to the header
    if (no_header && name == title)
        header_size = 0.0;
    else {
        if (landscape || twinpages)
            header_size = LANDSCAPE_HEADER * PIXELS_INCH;
        else
            header_size = PORTRAIT_HEADER * PIXELS_INCH;
        cut_filename(title, current_filename);
    }

    // Initialize variables related to the font size
    new_font = FALSE;
    if (fontsize != new_fontsize || new_format ||
        lines_requested != new_linesrequest || fontweight != new_fontweight)
    {
        if (new_fontsize == 0.0 || (fontsize == new_fontsize && new_format))
            new_fontsize = landscape ? 6.8 : twinpages ? 6.4 : 9.0;
        if (lines_requested != new_linesrequest) {
            if ((lines_requested = new_linesrequest) != 0) {
                // Scale fontsize
                if (landscape)
                    lines = (int)((page_width-header_size) / new_fontsize) - 1;
                else if (twinpages)
                    lines = (int)(((page_height - 2*header_size) / 2) / new_fontsize) - 2;
                else
                    lines = (int)((page_height-header_size) / new_fontsize) - 1;
                new_fontsize *= (float)lines / (float)lines_requested;
            }
        }
        fontsize = new_fontsize;
        fontweight = new_fontweight;
        new_font = TRUE;
    }

    // Initialize file printing, if there is any change
    if (new_format || new_font) {
        char_width = 0.6 * fontsize;
        if (landscape) {
            linesperpage = (int)((page_width - header_size) / fontsize) - 1;
            if (! twinpages)
                columnsperline = (int)(page_height / char_width) - 1;
            else
                columnsperline = (int)((page_height / 2) / char_width) - 1;
        }
        else {
            if (!twinpages)
                linesperpage = (int)((page_height - header_size) / fontsize) - 1;
            else
                linesperpage = (int)(((page_height - 2*header_size) / 2) / fontsize)
                    - 2;
            columnsperline = (int)(page_width / char_width) - 1;
        }
        if (lines_requested > 0)
            linesperpage = lines_requested;
        if (linesperpage <= 0 || columnsperline <= 0) {
            fprintf(stderr, "Font %g too big !!\n", fontsize);
            exit(EXIT_FAILURE);
        }
    }

    // Retrieve file modification date and hour
    if (fstat(fileno(stdin), &statbuf) == -1) {
        fprintf(stderr, "Error getting file modification time\n");
        exit(EXIT_FAILURE);
    }
    // Do we have a pipe?
    if (S_ISFIFO(statbuf.st_mode))
        strcpy(filedate, currentdate);
    else {
        string = ctime(&statbuf.st_mtime);
        sprintf(filedate, "%.6s %.4s %.5s", string+4, string+20, string+11);
    }
}

/*
 * Print the prologue necessary for printing each physical page.
 * Adobe convention for page independence is enforced through this routine.
 */
void print_page_prologue(int side) {
    // side: Logical page to print (left/right)

    // General format
    printf("/twp %s def\n", twinpages ? "true" : "false");
    printf("/fnfs %d def\n", landscape ? 11 : twinpages ? 10 : 15);
    printf("/dfs fnfs 0.8 mul def\n");
    printf("/df /Helvetica dfs getfont def\n");
    printf("/dw df setfont td stringwidth pop def\n");
    printf("/sfnf filenmfontname fnfs getfont def\n");
    printf("/hm fnfs 0.25 mul def\n");
    // Header size
    if (header_size == 0.0)
        printf("/hs 0.0 def\n");
    else
        printf("/hs %g inch def\n",
               landscape || twinpages ? LANDSCAPE_HEADER : PORTRAIT_HEADER);
    // Font sizes
    printf("/bfs %g def\n", fontsize);
    printf("/bdf /Courier-Bold bfs getfont def\n");
    printf("/bm bfs 0.7 mul def\n");
    printf("/bf %s bfs getfont def\n",
           fontweight == NORMAL ? "/CourierBack" : "/Courier-Bold");
    // Page attributes
    printf("/l %d def\n", linesperpage);
    printf("/c %d def\n", columnsperline);
    printf("/pw\n");
    printf("   bf setfont (0) stringwidth pop c mul bm dup add add\n");
    printf("   def\n");
    printf("/ph\n");
    printf("   bfs l mul bm dup add add hs add\n");
    printf("   def\n");
    printf("/fns\n");
    printf("      pw\n");
    printf("      fnfs 4 mul dw add (Page 999) stringwidth pop add\n");
    printf("    sub\n");
    printf("  def\n");
    printf("/tm margin twp {3} {2} ifelse div def\n");
    printf("/sd %d def\n", side);
    if (landscape) {
        printf("/y [ rm ph add bm add\n");
        printf("          dup ] def\n");
        printf("/sny dfs dfs add def\n");
        printf("/snx sh tm dfs add sub def\n");
        printf("/dy sny def\n");
        printf("/dx tm dfs add def\n");
        if (twinpages) {
            printf("/x [ tm                     %% left page\n");
            printf("          dup 2 mul pw add  %% right page\n");
            printf("        ] def\n");
        }
        else {
            printf("/x [ tm dup ] def\n");
        }
        printf("/scx sh 2 div def\n");
    }
    else {
        printf("/x [ lm dup ] def\n");
        printf("/sny tm dfs 2 mul sub def\n");
        printf("/snx sw rm sub dfs sub def\n");
        printf("/dy sny def\n");
        printf("/dx lm def\n");
        if (twinpages) {
            printf("/y [ tm ph add 2 mul %% up\n");
            printf("          tm ph add  %% down\n");
            printf("        ] def\n");
        }
        else {
            printf("\n%% Only one logical page\n");
            printf("/y [ sh tm sub dup ] def\n");
        }
        printf("/scx sw 2 div def\n");
    }
    printf("/fny dy def\n");
    printf("/fnx scx def\n");
    printf("/ly fnfs 2 div y sd get add def\n");
    printf("/lx snx def\n");
    printf("/d (%s) def\n", filedate);
    printf("( %s ) fn\n", current_filename);
}

/*
 * Print one file.
 */
static void print_file(char *name, char *header) {
    int c;
    int nchars;
    int start_line, start_page;
    int continue_exit;
    int status, new_status;

    // Reinitialize postscript variables depending on positional options
    init_file_printing(name, header == NULL ? name : header);

    // If we are in compact mode and the file beginning is to be printed
    // in the middle of a twinpage, we have to print a new page prologue
    if (twinfiles && sheetside == 1)
        print_page_prologue(1);

    /*
     * Boolean to indicates that previous char is \n (or interpreted \f)
     * and a new page would be started, if more text follows
     */
    start_page = FALSE;

    /*
     * Printing binary files is not very useful. We stop printing
     * if we detect one of these files. Our heuristic to detect them:
     * if 75% characters of first page are non-printing characters,
     * the file is a binary file.
     * Option -b force binary files impression.
     */
    nonprinting_chars = chars = 0;

    // Initialize printing variables
    column = 0;
    line = line_number = 0;
    first_page = TRUE;
    start_line = TRUE;
    prefix_width = numbering ? 6 : 1;

    // Start printing
    skip_page();

    // Process each character of the file
    status = IS_ROMAN;
    c = mygetc(&new_status);
    while (c != EOF) {
        /*
         * Preprocessing (before printing):
         * - TABs expansion (see interpret option)
         * - FF and BS interpretation
         * - replace non printable characters by a space or a char sequence
         *   like:
         *     ^X for ascii codes < 0x20 (X = [@, A, B, ...])
         *     ^? for del char
         *     M-c for ascii codes > 0x3f
         * - prefix parents and backslash ['(', ')', '\'] by backslash
         *   (escape character in postscript)
         */
        // Form feed
        if (c == '\f' && interpret) {
            // Close current line
            if (!start_line) {
                printf(") s\n");
                start_line = TRUE;
            }
            // start a new page ?
            if (start_page)
                skip_page();
            // Close current page and begin another
            endpage();
            start_page = TRUE;
            // Verification for binary files
            if (first_page && is_binaryfile(name))
                return;
            line = 0;
            column = 0;
            if ((c = mygetc(&new_status)) == EOF)
                break;
        }

        // Start a new line ?
        if (start_line) {
            if (start_page) {
                // only if there is something to print!
                skip_page();
                start_page = FALSE;
            }
            if (numbering)
                printf("(%4d|", ++line_number);
            else
                printf("( ");
            start_line = FALSE;
        }

        // Is a new font ? This feature is used only to detect bold
        // sequences produced by nroff (man pages), in connexion with
        // mygetc.
        if (status != new_status) {
            printf(")\n");
            printf("%s", status == IS_ROMAN ? "b" : "st");
            printf(" (");
            status = new_status;
        }

        // Interpret each character
        switch (c) {
            case '\b':
                if (!interpret)
                    goto print;
                // A backspace is converted to 2 chars ('\b'). These chars
                // with the Courier backspace font produce correct under-
                // lined strings.
                if (column)
                    column--;
                putchar('\\');
                putchar('b');
                break;
            case '\n':
                column = 0;
                start_line = TRUE;
                printf(") s\n");
                if (++line >= linesperpage) {
                    endpage();
                    start_page = TRUE;
                    if (first_page && is_binaryfile(name))
                        return;
                    line = 0;
                }
                break;
            case '\t':
                if (interpret) {
                    continue_exit = FALSE;
                    do {
                        if (++column + prefix_width > columnsperline) {
                            if (folding) {
                                if (fold_line(name) == FALSE)
                                    return;
                            }
                            else {
                                c = cut_line();
                                continue_exit = TRUE;
                                break;
                            }
                        }
                        putchar(' ');
                    } while (column % column_width);
                    if (continue_exit)
                        continue;
                    break;
                }
            default:
            print :
                if (only_printable) {
                    nchars = 1;
                }
                else if (! ISOlatin1) {
                    nchars = c > 0177 ? 2 : 0;
                    nchars += (c&0177) < ' ' || (c&0177) == 0177 ? 2 : 1;
                }
                else {
                    nchars = c < ' ' || (c >= 0177 && c < 144) ? 2 : 1;
                }

                if (prefix_width + (column += nchars) > columnsperline) {
                    if (folding) {
                        if (fold_line(name) == FALSE) {
                            return;
                        }
                    }
                    else {
                        c = cut_line();
                        new_status = IS_ROMAN;
                        continue;
                    }
                }
                nonprinting_chars += printchar(c);
                chars++;
                break;
        }
        c = mygetc(&new_status);
    }

    if (!start_line)
        printf(") s\n");
    if (!start_page)
        endpage();
}


//**************************************************************
//              Print a postscript prologue for a2ps.
//**************************************************************

/*
 * Print the a2ps prologue.
 */
static void print_prologue() {
    int             c;
    FILE           *f = NULL;
    char           *datestring;
#if defined(SYSV) || defined(BSD)
    char           *logname, *host;
    int             rt;
#endif
#if defined(SYSV)
    struct utsname  snames;
#endif

    // Retrieve date and hour
#if defined(__STDC__)
    time_t date;

    if (time(&date) == -1) {
        fprintf(stderr, "Error calculating time\n");
        exit(EXIT_FAILURE);
    }
    datestring = ctime(&date);
#else
#ifdef BSD
    struct timeval date;
    struct tm *p;

    (void) gettimeofday(&date, (struct timezone *)0);
    p = localtime(&date.tv_sec);
    datestring = asctime(p);
#else
#ifdef SYSV
    struct timeb date;

    (void)ftime(&date);
    datestring = ctime(&date.time);
#else

    datestring = "--- --- -- --:--:-- ----";
#endif
#endif
#endif

#if defined(SYSV) || defined(BSD)
    // Retrieve user's login name and hostname
    logname = getlogin();
    host = (char *)malloc(MAX_HOSTNAME);
    if (host != NULL) {
#if defined(SYSV)
        if ((rt = uname(&snames)) == -1 || snames.nodename[0] == NULL) {
            free(host);
            host = NULL;
        }
        else
            strcpy(host, snames.nodename);
#else
        if ((rt = gethostname(host, MAX_HOSTNAME)) == -1 || host[0] == NULL) {
            free(host);
            host = NULL;
        }
#endif
    }
#endif

    // Print a general prologue
    if (prologue == NULL)
        print_standard_prologue(datestring);
    else if ((f = fopen(prologue, "r")) != NULL) {
        // Header file printing
        while ((c = getc(f)) != EOF)
            putchar(c);
    }
    else {
        fprintf(stderr, "Postscript header missing: %s\n", prologue);
        exit(EXIT_FAILURE);
    }

    // Completes the prologue with a2ps static variables
    printf("\n%% Initialize page description variables.\n");
    printf("/x0 0 def\n");
    printf("/y0 0 def\n");
    printf("/sh %g inch def\n", (double)HEIGHT);
    printf("/sw %g inch def\n", (double)WIDTH);
    printf("/margin %g inch def\n", (double)MARGIN);
    printf("/rm margin 3 div def\n");
    printf("/lm margin 2 mul 3 div def\n");
    printf("/d () def\n");

    // And print them
    sprintf(currentdate, "%.6s %.4s %.5s",
            datestring+4, datestring+20, datestring+11);
    printf("/td (%s) def\n", currentdate);

#if defined(SYSV) || defined(BSD)
    // Add the user's login name string to the Postscript output
    if (logname != NULL || host != NULL) {
        if (logname != NULL && host != NULL)
            printf("/lg (Printed by %s from %s) def\n", logname, host);
        else if (logname != NULL)
            printf("/lg (Printed by %s) def\n", logname);
        else
            printf("/lg (Printed from %s) def\n", host);
    }

    // If the host string was allocated via malloc, release the memory
    if (host != NULL)
        free(host);
#endif

    // Close prolog
    printf("%%%%EndProlog\n\n");

    // Go on
    printf("/docsave save def\n");
    if (f) fclose(f);
}

/*
 * Print the standard prologue.
 */
void print_standard_prologue(char *datestring) {
    printf("%%!PS-Adobe-3.0\n");
    printf("%%%%Creator: A2ps version %s\n", VERSION);
    printf("%%%%CreationDate: %.24s\n", datestring);
    printf("%%%%Pages: (atend)\n");
    printf("%%%%DocumentFonts: Courier Courier-Bold Helvetica Helvetica-Bold\n");
    printf("%%%%EndComments\n");
    printf("%% Copyright (c) 1993, 1994, Miguel Santana, M.Santana@frgu.bull.fr\n");
    printf("\n/$a2psdict 100 dict def\n");
    printf("$a2psdict begin\n");
    printf("\n%% General macros.\n");
    printf("/xdef {exch def} bind def\n");
    printf("/getfont {exch findfont exch scalefont} bind def\n");

    if (ISOlatin1) {
        printf("\n%% Set up ISO Latin 1 character encoding\n");
        printf("/reencodeISO {\n");
        printf("        dup dup findfont dup length dict begin\n");
        printf("        { 1 index /FID ne { def }{ pop pop } ifelse\n");
        printf("        } forall\n");
        printf("        /Encoding ISOLatin1Encoding def\n");
        printf("        currentdict end definefont\n");
        printf("} def\n");
        printf("/Helvetica-Bold reencodeISO def\n");
        printf("/Helvetica reencodeISO def\n");
        printf("/Courier reencodeISO def\n");
        printf("/Courier-Bold reencodeISO def\n");
    }

    printf("\n%% Create Courier backspace font\n");
    printf("/backspacefont {\n");
    printf("    /Courier findfont dup length dict begin\n");
    printf("    { %% forall\n");
    printf("        1 index /FID eq { pop pop } { def } ifelse\n");
    printf("    } forall\n");
    printf("    currentdict /UniqueID known { %% if\n");
    printf("        /UniqueID UniqueID 16#800000 xor def\n");
    printf("    } if\n");
    printf("    CharStrings length 1 add dict begin\n");
    printf("        CharStrings { def } forall\n");
    printf("        /backspace { -600 0 0 0 0 0 setcachedevice } bind def\n");
    printf("        currentdict\n");
    printf("    end\n");
    printf("    /CharStrings exch def\n");
    printf("    /Encoding Encoding 256 array copy def\n");
    printf("    Encoding 8 /backspace put\n");
    printf("    currentdict\n");
    printf("    end\n");
    printf("    definefont pop\n");
    printf("} bind def\n");

    printf("\n%% FUNCTIONS\n");
    printf("\n%% Function filename: Initialize file printing.\n");
    printf("/fn\n");
    printf("{ /filenm xdef\n");
    printf("  /filenmwidth filenm stringwidth pop def\n");
    printf("  /filenmfont\n");
    printf("       filenmwidth fns gt\n");
    printf("       {\n");
    printf("           filenmfontname\n");
    printf("           fnfs fns mul filenmwidth div\n");
    printf("         getfont\n");
    printf("       }\n");
    printf("       { sfnf }\n");
    printf("     ifelse\n");
    printf("  def\n");
    printf("} bind def\n");
    printf("\n%% Function header: prints page header. no page\n");
    printf("%% is passed as argument.\n");
    printf("/hp\n");
    printf("  { x sd get  y sd get hs sub 1 add  moveto\n");
    printf("    df setfont\n");
    printf("    gsave\n");
    printf("      x sd get y sd get moveto\n");
    printf("      0 hs 2 div neg rmoveto \n");
    printf("      hs setlinewidth\n");
    printf("      0.95 setgray\n");
    printf("      pw 0 rlineto stroke\n");
    printf("    grestore\n");
    printf("    gsave\n");
    printf("      dfs hm rmoveto\n");
    printf("      d show                                %% date/hour\n");
    printf("    grestore\n");
    printf("    gsave\n");
    printf("      pnum cvs pop                          %% page pop up\n");
    printf("        pw (Page 999) stringwidth pop sub\n");
    printf("        hm\n");
    printf("      rmoveto\n");
    printf("      (Page ) show pnum show                %% page number\n");
    printf("    grestore\n");
    printf("    empty pnum copy pop\n");
    printf("    gsave\n");
    printf("      filenmfont setfont\n");
    printf("         fns filenm stringwidth pop sub 2 div dw add\n");
    printf("          bm 2 mul \n");
    printf("        add \n");
    printf("        hm\n");
    printf("      rmoveto\n");
    printf("        filenm show                 %% file name\n");
    printf("      grestore\n");
    printf("    } bind def\n");
    printf("\n%% Function border: prints border page\n");
    printf("/border \n");
    printf("{ x sd get y sd get moveto\n");
    printf("  gsave                             %% print four sides\n");
    printf("    0.7 setlinewidth                %% of the square\n");
    printf("    pw 0 rlineto\n");
    printf("    0 ph neg rlineto\n");
    printf("    pw neg 0 rlineto\n");
    printf("    closepath stroke\n");
    printf("  grestore\n");
    printf("} bind def\n");
    printf("\n%% Function hborder: completes border of the header.\n");
    printf("/hborder \n");
    printf("{ gsave\n");
    printf("    0.7 setlinewidth\n");
    printf("    0 hs neg rmoveto\n");
    printf("    pw 0 rlineto\n");
    printf("    stroke\n");
    printf("  grestore\n");
    printf("} bind def\n");
    printf("\n%% Function sheetnumber: prints the sheet number.\n");
    printf("/sn\n");
    printf("    { snx sny moveto\n");
    printf("      df setfont\n");
    printf("      pnum cvs\n");
    printf("      dup stringwidth pop (0) stringwidth pop sub neg 0 rmoveto show\n");
    printf("      empty pnum copy pop\n");
    printf("    } bind def\n");
    printf("\n%% Function loginprint: prints the login id of the requestor.\n");
    printf("/lgp\n");
    printf("    { lx ly moveto\n");
    printf("      df setfont\n");
    printf("      dup stringwidth pop neg 0 rmoveto show\n");
    printf("    } bind def\n");
    printf("\n%% Function currentdate: prints the current date.\n");
    printf("/cd\n");
    printf("    { dx dy moveto\n");
    printf("      df setfont\n");
    printf("      (Printed: ) show\n");
    printf("      td show\n");
    printf("    } bind def\n");
    printf("\n%% Function filename_footer: prints the file name at bottom of page.\n");
    printf("/fnf\n");
    printf("    { fnx fny moveto\n");
    printf("      df setfont\n");
    printf("      filenm center show\n");
    printf("    } bind def\n");
    printf("\n%% Function center: centers text.\n");
    printf("/center\n");
    printf("    { dup stringwidth pop\n");
    printf("      2 div neg 0 rmoveto\n");
    printf("    } bind def\n");
    printf("\n%% Function s: print a source line\n");
    printf("/s  { show\n");
    printf("      /y0 y0 bfs sub def\n");
    printf("      x0 y0 moveto\n");
    printf("    } bind def\n");
    printf("\n%% Functions b and st: change to bold or standard font\n");
    printf("/b  { show\n");
    printf("      bdf setfont\n");
    printf("    } bind def\n");
    printf("/st { show\n");
    printf("      bf setfont\n");
    printf("    } bind def\n");
    printf("\n%% Strings used to make easy printing numbers\n");
    printf("/pnum 12 string def\n");
    printf("/empty 12 string def\n");
    printf("\n%% Global initializations\n");
    printf("\n/CourierBack backspacefont\n");
    printf("/filenmfontname /Helvetica-Bold def\n");
    printf("/inch {72 mul} bind def\n");

    printf("\n%%\n");
    printf("%% Meaning of some variables and functions (coded names)\n");
    printf("%%\n");
    printf("%%  twp:            twinpages?\n");
    printf("%%  sd:             sheet side\n");
    printf("%%  l:              line counter\n");
    printf("%%  c:              column counter\n");
    printf("%%  d:              date\n");
    printf("%%  td:             current date (for today)\n");
    printf("%%  lg:             login name\n");
    printf("%%  fn:             filename printing function\n");
    printf("%%  sn:             sheetnumber printing function\n");
    printf("%%  cd:             current date printing function\n");
    printf("%%  fnf:            filename footer printing function\n");
    printf("%%  lgp:            login printing function\n");
    printf("%%  hp:             header printing function\n");
    printf("%%  y:              y coordinate for the logical page\n");
    printf("%%  x:              x coordinate for the logical page\n");
    printf("%%  sny:            y coordinate for the sheet number\n");
    printf("%%  snx:            x coordinate for the sheet number\n");
    printf("%%  dy:             y coordinate for the date\n");
    printf("%%  dx:             x coordinate for the date\n");
    printf("%%  ly:             y coordinate for the login\n");
    printf("%%  lx:             x coordinate for the login\n");
    printf("%%  scx:            x coordinate for the sheet center\n");
    printf("%%  fny:            y coordinate for the filename (footer)\n");
    printf("%%  fnx:            x coordinate for the filename (footer)\n");
    printf("%%  fnfs:           filename font size\n");
    printf("%%  bfs:            body font size\n");
    printf("%%  dfs:            date font size\n");
    printf("%%  bfs:            body font size\n");
    printf("%%  df:             date font\n");
    printf("%%  bf:             body font\n");
    printf("%%  bdf:            bold font\n");
    printf("%%  sfnf:           standard filename font\n");
    printf("%%  dw:             date width\n");
    printf("%%  pw:             page width\n");
    printf("%%  sw:             sheet width\n");
    printf("%%  ph:             page height\n");
    printf("%%  sh:             sheet height\n");
    printf("%%  hm:             header margin\n");
    printf("%%  tm:             top margin\n");
    printf("%%  bm:             body margin\n");
    printf("%%  rm:             right margin\n");
    printf("%%  lm:             left margin\n");
    printf("%%  hs:             header size\n");
    printf("%%  fns:            filename size\n");
}


/*
 * Main routine for a2ps.
 */
int ARB_main(int argc, char *cargv[]) {
    char       **argv = (char**)cargv;
    int          narg;
    char        *arg;
    int          total;
#if LPR_PRINT
    int          fd[2];
    const char  *lpr_args[10];
#endif

    // Process global options
    command = argv[0];
    arg = argv[narg = 1];
    while (narg < argc) {
        if (arg[0] == '-')
            set_global_option(arg);
        arg = argv[++narg];
    }

#if LPR_PRINT
    // Start lpr process
    if (lpr_print) {
        if (pipe(fd) != 0) {
            fprintf(stderr, "Could not create pipe (Reason: %s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) {
            dup2(fd[0], 0);
            close(fd[0]); close(fd[1]);
            narg = 0;
            lpr_args[narg++] = LPR_COMMAND;
#ifdef LPR_OPT
            lpr_args[narg++] = LPR_OPT;
#endif
            if (lpr_opt)
                lpr_args[narg++] = lpr_opt;
#ifdef RECTO_VERSO_PRINTING
            if (rectoverso)
                lpr_args[narg++] = TWOSIDED;
            else
                lpr_args[narg++] = ONESIDED;
#endif
            lpr_args[narg] = (char *)0;
            execvp(LPR_COMMAND, (char**)lpr_args);
            fprintf(stderr, "Error starting lpr process \n");
            exit(EXIT_FAILURE);
        }
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
    }
#endif

    // Initialize variables not depending of positional options
    landscape = twinpages = -1; // To force format switching
    fontsize = -1.0;                    // To force fontsize switching
    page_height = (double)(HEIGHT - MARGIN) * PIXELS_INCH;
    page_width = (double)(WIDTH - MARGIN) * PIXELS_INCH;

    // Postscript prologue printing
    print_prologue();

    // Print files designated or standard input
    arg = argv[narg = 1];
    while (narg < argc) {
        if (arg[0] != NUL) {
            if (arg[0] == '-')
                set_positional_option(arg);
            else {
                if (freopen(arg, "r", stdin) == NULL) {
                    fprintf(stderr, "Error opening %s\n", arg);
                    cleanup();
                    printf("\n%%%%Trailer\ndocsave restore end\n\4");
                    exit(EXIT_FAILURE);
                }
                no_files = FALSE;

                // Save counters values
                old_pages = pages;
                if (twinfiles && twinpages)
                    old_sheets = sheets;
                else
                    old_sheets = sheets + sheetside;

                // Print the file
                print_file(arg, header_text);

                // Print the number of pages and sheets printed
                if (no_summary == FALSE) {
                    total = pages - old_pages;
                    fprintf(stderr, "[%s: %d page%s on ", arg,
                            total, total == 1 ? "" : "s");
                    total = sheets - old_sheets + sheetside;
#ifdef RECTO_VERSO_PRINTING
                    if (rectoverso)
                        total = (total+1) / 2;
#endif
                    fprintf(stderr, "%d sheet%s]\n", total, total == 1 ? "" : "s");
                }

                // Reinitialize header title
                header_text = NULL;
            }
        }
        arg = argv[++narg];
    }
    if (no_files)
        print_file((char*)"stdin", header_text);

    // Print the total number of pages printed
    if (no_summary == FALSE && pages != old_pages) {
        fprintf(stderr, "[Total: %d page%s on ", pages, pages == 1 ? "" : "s");
        total = sheets + sheetside;
#ifdef RECTO_VERSO_PRINTING
        if (rectoverso)
            total = (total+1) / 2;
#endif
        fprintf(stderr, "%d sheet%s]\n", total, total == 1 ? "" : "s");
    }

    // And stop
    cleanup();
    printf("\n%%%%Trailer\n");
    printf("%%%%Pages: %d\n", sheets + sheetside);
    printf("docsave restore end\n");

    exit(EXIT_SUCCESS);
}
