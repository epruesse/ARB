#ifndef FUN_H
#define FUN_H

// forward decls for prototypes

class Reader;
class WrapMode;
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
