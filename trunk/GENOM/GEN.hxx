#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>

#define AWAR_GENOM "genom"
#define AWAR_GENOM_DB AWAR_GENOM "/db" // main flag (true=genom db, false/missing=normal db)

GB_ERROR GEN_read(GBDATA *gb_main, const char *filename, const char *ali_name);

#else
#error GEN.hxx included twice
#endif // GEN_HXX
