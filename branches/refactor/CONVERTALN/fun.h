#ifndef FUN_H
#define FUN_H

// forward decls for prototypes

enum Format {
    // input/output formats
    EMBL,
    GENBANK,
    MACKE,
    SWISSPROT,
    LAST_INPUT_FORMAT = SWISSPROT,

    // output-only formats
    GCG,
    NBRF,
    NEXUS,
    PHYLIP,
    PHYLIP2,
    PRINTABLE,
    STADEN,

    UNKNOWN,
};

class Reader;
struct Embl;
struct Emblref;
struct GenBank;
struct GenbankRef;
struct Macke;
struct Paup;
struct Seq;

#ifndef FILEBUFFER_H
#include <FileBuffer.h>
#endif

#ifndef PROTOTYPES_H
#include "prototypes.h"
#endif

#else
#error fun.h included twice
#endif // FUN_H
