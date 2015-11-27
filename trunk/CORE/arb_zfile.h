// ================================================================ //
//                                                                  //
//   File      : arb_zfile.h                                        //
//   Purpose   : Compressed file I/O                                //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2015   //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_ZFILE_H
#define ARB_ZFILE_H

#ifndef ARB_CORE_H
#include "arb_core.h"
#endif

#if defined(UNIT_TESTS)
#define USE_BROKEN_COMPRESSION
#endif

enum FileCompressionMode {
    ZFILE_AUTODETECT,

    ZFILE_UNCOMPRESSED,

    // -------------------- [real compressions]
    ZFILE_GZIP,
    ZFILE_BZIP2,
    ZFILE_XZ,
    // -------------------- [real compressions]

    ZFILE_UNDEFINED,
#if defined(USE_BROKEN_COMPRESSION)
    ZFILE_BROKEN, // a broken pipe
#endif
    ZFILE_REAL_CMODES = ZFILE_UNDEFINED-ZFILE_UNCOMPRESSED-1, // number of supported modes
};

FILE     *ARB_zfopen(const char *name, const char *mode, FileCompressionMode cmode, GB_ERROR& error);
GB_ERROR  ARB_zfclose(FILE *fp, const char *filename);

#else
#error arb_zfile.h included twice
#endif // ARB_ZFILE_H
