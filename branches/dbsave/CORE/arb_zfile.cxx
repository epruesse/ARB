// ================================================================ //
//                                                                  //
//   File      : arb_zfile.cxx                                      //
//   Purpose   : Compressed file I/O                                //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2015   //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "arb_zfile.h"
#include "arb_file.h"
#include "arb_msg.h"
#include "arb_assert.h"

using namespace std;

FILE *ARB_zfopen(const char *name, const char *mode, FileCompressionMode cmode, GB_ERROR& error) {
    arb_assert(!error);

    if (strchr(mode, 'a')) {
        error = "Cannot append to file using ARB_zfopen";
        return NULL;
    }

    bool forOutput = strchr(mode, 'w');

    FILE *fp = NULL;

    if (cmode == ZFILE_AUTODETECT) {
        if (forOutput) {
            error = "Autodetecting compression mode only works for input files";
        }
        else {
            fp = fopen(name, mode);
            if (!fp) error = GB_IO_error("opening", name);
            else {
                // detect compression and set 'cmode'
                const size_t MAGICSIZE = 5;
                char         buffer[MAGICSIZE];

                size_t bytes_read = fread(buffer, 1, MAGICSIZE, fp);
                fclose(fp);
                fp = NULL;

                if      (bytes_read>=2 && strncmp(buffer, "\x1f\x8b", 2) == 0) cmode = ZFILE_GZIP;
                else if (bytes_read>=2 && strncmp(buffer, "BZ",       2) == 0) cmode = ZFILE_BZIP2;
                else {
                    cmode = ZFILE_UNCOMPRESSED;
                }
            }
        }
    }

    if (cmode == ZFILE_UNCOMPRESSED) {
        fp = fopen(name, mode);
        if (!fp) error = GB_IO_error("opening", name);
    }
    else {
        if (!error) {
            char *iPipe = NULL;
            char *oPipe = NULL;

            switch (cmode) {
                case ZFILE_GZIP:
                    if (forOutput) oPipe = GBS_global_string_copy("gzip > %s", name);
                    else           iPipe = GBS_global_string_copy("gunzip < %s", name);
                    break;

                case ZFILE_BZIP2:
                    if (forOutput) oPipe = GBS_global_string_copy("bzip2 > %s", name);
                    else           iPipe = GBS_global_string_copy("bunzip2 < %s", name);
                    break;

                default:
                    error = GBS_global_string("Invalid compression mode (%i)", int(cmode));
                    break;

#if defined(USE_BROKEN_COMPRESSION)
                case ZFILE_BROKEN:
                    if (forOutput) {
                        oPipe = GBS_global_string_copy("arb_weirdo > %s", name);
                    }
                    else {
                        iPipe = GBS_global_string_copy("arb_weirdo < %s", name);
                    }
                    break;
#endif
            }

            if (!error) {
                arb_assert(contradicted(oPipe, iPipe)); // need one!

                if (oPipe) { // write to pipe
                    fp = popen(oPipe, mode);
                    if (!fp) {
                        error = GB_IO_error("writing to pipe", oPipe);
                    }
                }
                else { // read from pipe
                    fp = popen(iPipe, mode);
                    if (!fp) {
                        error = GB_IO_error("reading from pipe", iPipe);
                    }
                }
            }

            free(oPipe);
            free(iPipe);
        }
    }

    arb_assert(contradicted(fp, error));
    return fp;
}

GB_ERROR ARB_zfclose(FILE *fp, const char *filename) {
    bool fifo = GB_is_fifo(fp);
    int  res;
    if (fifo) {
        res = pclose(fp);
    }
    else {
        res = fclose(fp);
    }

    GB_ERROR error = NULL;
    if (res != 0) {
        int exited   = WIFEXITED(res);
        int signaled = WIFSIGNALED(res);
        int status   = WEXITSTATUS(res);

        if (exited) {
            if (status) {
                if (fifo) {
                    error = GBS_global_string("pipe writing to '%s' failed with exitcode=%i (broken pipe?)", filename, status); // @@@ show pipe-command here
                }
            }
        }
        if (!error) error = GB_IO_error("closing", filename);
#if defined(DEBUG)
        error = GBS_global_string("%s (res=%i, exited=%i, signaled=%i, status=%i)", error, res, exited, signaled, status);
#endif
    }
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static char *fileContent(FILE *in, size_t& bytes_read) {
    const size_t  BUFFERSIZE = 1000;
    char         *buffer     = (char*)malloc(BUFFERSIZE+1);
    bytes_read               = fread(buffer, 1, BUFFERSIZE, in);
    arb_assert(bytes_read<BUFFERSIZE);
    buffer[bytes_read]       = 0;
    return buffer;
}

#define TEST_EXPECT_ZFOPEN_FAILS(name,mode,cmode,errpart) do{     \
        GB_ERROR  error = NULL;                                   \
        FILE     *fp    = ARB_zfopen(name, mode, cmode, error);   \
                                                                  \
        if (fp) {                                                 \
            TEST_EXPECT_NULL(error);                              \
            error = ARB_zfclose(fp, name);                        \
        }                                                         \
        else {                                                    \
            TEST_EXPECT_NULL(fp);                                 \
        }                                                         \
        TEST_REJECT_NULL(error);                                  \
        TEST_EXPECT_CONTAINS(error, errpart);                     \
    }while(0)

void TEST_compressed_io() {
    const char *inText  = "general/text.input";
    const char *outFile = "compressed.out";

    TEST_EXPECT_ZFOPEN_FAILS("",      "",  ZFILE_UNCOMPRESSED, "Invalid argument");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "a", ZFILE_UNCOMPRESSED, "Cannot append to file using ARB_zfopen");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "r", ZFILE_UNDEFINED,    "Invalid compression mode");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "w", ZFILE_AUTODETECT,   "only works for input files");

#if defined(USE_BROKEN_COMPRESSION)
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "r", ZFILE_BROKEN,   "broken pipe");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "w", ZFILE_BROKEN,   "broken pipe");
#endif

    char         *testText;
    const size_t  TEST_TEXT_SIZE = 428;
    {
        GB_ERROR  error = NULL;
        FILE     *in    = ARB_zfopen(inText, "r", ZFILE_UNCOMPRESSED, error);
        TEST_EXPECT_NULL(error);
        TEST_REJECT_NULL(in);

        size_t bytes_read;
        testText = fileContent(in, bytes_read);
        TEST_EXPECT_EQUAL(bytes_read, TEST_TEXT_SIZE);

        TEST_EXPECT_NO_ERROR(ARB_zfclose(in, inText));
    }

    for (FileCompressionMode cmode = FileCompressionMode(ZFILE_AUTODETECT+1);
         cmode != ZFILE_UNDEFINED;
         cmode  = FileCompressionMode(cmode+1))
    {
        TEST_ANNOTATE(GBS_global_string("cmode=%i", int(cmode)));

        {
            GB_ERROR  error = NULL;
            FILE     *out   = ARB_zfopen(outFile, "w", cmode, error);

            TEST_REJECT(error);
            TEST_REJECT_NULL(out);

            TEST_EXPECT_DIFFERENT(EOF, fputs(testText, out));
            TEST_EXPECT_NO_ERROR(ARB_zfclose(out, outFile));
        }

        for (int detect = 0; detect<=1; ++detect) {
            TEST_ANNOTATE(GBS_global_string("cmode=%i detect=%i", int(cmode), detect));

            GB_ERROR  error = NULL;
            FILE     *in    = ARB_zfopen(outFile, "r", detect ? ZFILE_AUTODETECT : cmode, error);

            TEST_REJECT(error);
            TEST_REJECT_NULL(in);

            size_t  bytes_read;
            char   *content = fileContent(in, bytes_read);
            TEST_EXPECT_NO_ERROR(ARB_zfclose(in, outFile));
            TEST_EXPECT_EQUAL(content, testText); // if this fails for detect==1 -> detection does not work
            free(content);
        }
    }


    free(testText);
    TEST_EXPECT_DIFFERENT(GB_unlink(outFile), -1);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

