#ifndef DEFS_H
#define DEFS_H

#define RNA     0
#define NONRNA  1

#define AUTHOR   1
#define JOURNAL  2
#define TITLE    3
#define STANDARD 4
#define PROCESS  5
#define ALL      0

#define SEQLINE 60

#define LINESIZE  126
#define LONGTEXT  5000
#define TOKENSIZE 80

// max. length of lines
#define EMBLMAXLINE  74
#define GBMAXLINE    78
#define MACKEMAXLINE 78

// indentation to part behind key
#define EMBLINDENT 5
#define GBINDENT   12

// indents for RDP-defined comments
#define RDP_SUBKEY_INDENT    2
#define RDP_CONTINUED_INDENT 6

#define p_nonkey_start 5

enum EntryState {
    ENTRY_NONE,
    ENTRY_STARTED,
    ENTRY_COMPLETED,
};

// --------------------
// Logging

#if defined(DEBUG)
// #define CALOG // be more verbose in debugging mode
#endif // DEBUG

#else
#error defs.h included twice
#endif // DEFS_H
