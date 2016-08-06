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
#include "arb_misc.h"
#include "arb_assert.h"
#include "arb_mem.h"

#include <string>
#include <map>

using namespace std;

class zinfo {
    // info stored for each sucessfully opened file
    // to support proper error message on close.
    bool   writing; // false -> reading
    string filename;
    string pipe_cmd;
public:
    zinfo() {}
    zinfo(bool writing_, const char *filename_, const char *pipe_cmd_)
        : writing(writing_),
          filename(filename_),
          pipe_cmd(pipe_cmd_)
    {}

    bool isOutputPipe() const { return writing; }
    const char *get_filename() const { return filename.c_str(); }
    const char *get_pipecmd() const { return pipe_cmd.c_str(); }
};
static map<FILE*,zinfo> zfile_info;

FILE *ARB_zfopen(const char *name, const char *mode, FileCompressionMode cmode, GB_ERROR& error, bool hideStderr) {
    arb_assert(!error);

    if (strchr(mode, 'a')) {
        error = "Cannot append to file using ARB_zfopen";
        return NULL;
    }
    if (strchr(mode, 't')) {
        error = "Cannot use textmode for ARB_zfopen";
        return NULL;
    }
    if (strchr(mode, '+')) {
        error = "Cannot open file in read and write mode with ARB_zfopen";
        return NULL;
    }

    bool forOutput = strchr(mode, 'w');
    FILE *fp       = NULL;

    if (cmode == ZFILE_AUTODETECT) {
        if (forOutput) {
            error = "Autodetecting compression mode only works for input files";
        }
        else {
            fp = fopen(name, "rb");
            if (!fp) error = GB_IO_error("opening", name);
            else {
                // detect compression and set 'cmode'
                const size_t MAGICSIZE = 5;
                char         buffer[MAGICSIZE];

                size_t bytes_read = fread(buffer, 1, MAGICSIZE, fp);
                fclose(fp);
                fp = NULL;

                if      (bytes_read>=2 && strncmp(buffer, "\x1f\x8b",    2) == 0) cmode = ZFILE_GZIP;
                else if (bytes_read>=2 && strncmp(buffer, "BZ",          2) == 0) cmode = ZFILE_BZIP2;
                else if (bytes_read>=5 && strncmp(buffer, "\xfd" "7zXZ", 5) == 0) cmode = ZFILE_XZ;
                else {
                    cmode = ZFILE_UNCOMPRESSED;
                }
            }
        }
    }

    if (cmode == ZFILE_UNCOMPRESSED) {
        fp             = fopen(name, mode);
        if (!fp) error = GB_IO_error("opening", name);
        else {
            zfile_info[fp] = zinfo(forOutput, name, "");
        }
    }
    else {
        if (!error) {
            const char *compressor      = NULL; // command used to compress (and decompress)
            const char *decompress_flag = "-d"; // flag needed to decompress (assumes none to compress)

            switch (cmode) {
                case ZFILE_GZIP:  {
                    static char *pigz = ARB_executable("pigz", ARB_getenv_ignore_empty("PATH"));
                    compressor = pigz ? pigz : "gzip";
                    break;
                }
                case ZFILE_BZIP2: compressor = "bzip2"; break;
                case ZFILE_XZ:    compressor = "xz";    break;

                default:
                    error = GBS_global_string("Invalid compression mode (%i)", int(cmode));
                    break;

#if defined(USE_BROKEN_COMPRESSION)
                case ZFILE_BROKEN:
                    compressor = "arb_weirdo"; // a non-existing command!
                    break;
#endif
            }

            if (!error) {
                char *pipeCmd = forOutput
                    ? GBS_global_string_copy("%s > %s", compressor, name)
                    : GBS_global_string_copy("%s %s < %s", compressor, decompress_flag, name);

                if (hideStderr) {
                    freeset(pipeCmd, GBS_global_string_copy("( %s 2>/dev/null )", pipeCmd));
                }

                // remove 'b' from mode (pipes are binary by default)
                char *impl_b_mode = strdup(mode);
                while (1) {
                    char *b = strchr(impl_b_mode, 'b');
                    if (!b) break;
                    strcpy(b, b+1);
                }

                if (forOutput) { // write to pipe
                    fp             = popen(pipeCmd, impl_b_mode);
                    if (!fp) error = GB_IO_error("writing to pipe", pipeCmd);
                }
                else { // read from pipe
                    fp             = popen(pipeCmd, impl_b_mode);
                    if (!fp) error = GB_IO_error("reading from pipe", pipeCmd);
                }

                if (!error) {
                    zfile_info[fp] = zinfo(forOutput, name, pipeCmd);
                }

                free(impl_b_mode);
                free(pipeCmd);
            }
        }
    }

    arb_assert(contradicted(fp, error));
    arb_assert(implicated(error, error[0])); // deny empty error
    return fp;
}

GB_ERROR ARB_zfclose(FILE *fp) {
    bool  fifo = GB_is_fifo(fp);
    zinfo info = zfile_info[fp];
    zfile_info.erase(fp);

    int res;
    if (fifo) {
        res = pclose(fp);
    }
    else {
        res = fclose(fp);
    }

    GB_ERROR error = NULL;
    if (res != 0) {
        int exited   = WIFEXITED(res);
        int status   = WEXITSTATUS(res);
#if defined(DEBUG)
        int signaled = WIFSIGNALED(res);
#endif

        if (exited) {
            if (status) {
                if (fifo) {
                    error = GBS_global_string("pipe %s\n"
                                              " file='%s'\n"
                                              " using cmd='%s'\n"
                                              " failed with exitcode=%i (broken pipe? corrupted archive?)\n",
                                              info.isOutputPipe() ? "writing to" : "reading from",
                                              info.get_filename(),
                                              info.get_pipecmd(),
                                              status);
                }
            }
        }
        if (!error) error = GB_IO_error("closing", info.get_filename());
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
    char         *buffer     = (char*)ARB_alloc(BUFFERSIZE+1);
    bytes_read               = fread(buffer, 1, BUFFERSIZE, in);
    arb_assert(bytes_read<BUFFERSIZE);
    buffer[bytes_read]       = 0;
    return buffer;
}

#define TEST_EXPECT_ZFOPEN_FAILS(name,mode,cmode,errpart) do{           \
        GB_ERROR  error = NULL;                                         \
        FILE     *fp    = ARB_zfopen(name, mode, cmode, error, false);  \
                                                                        \
        if (fp) {                                                       \
            TEST_EXPECT_NULL(error);                                    \
            error = ARB_zfclose(fp);                                    \
        }                                                               \
        else {                                                          \
            TEST_EXPECT_NULL(fp);                                       \
        }                                                               \
        TEST_REJECT_NULL(error);                                        \
        TEST_EXPECT_CONTAINS(error, errpart);                           \
    }while(0)

void TEST_compressed_io() {
    const char *inText  = "general/text.input";
    const char *outFile = "compressed.out";

    TEST_EXPECT_ZFOPEN_FAILS("",      "",   ZFILE_UNCOMPRESSED, "Invalid argument");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "a",  ZFILE_UNCOMPRESSED, "Cannot append to file using ARB_zfopen");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "r",  ZFILE_UNDEFINED,    "Invalid compression mode");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "w",  ZFILE_AUTODETECT,   "only works for input files");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "rt", ZFILE_AUTODETECT,   "Cannot use textmode");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "r+", ZFILE_AUTODETECT,   "Cannot open file in read and write mode");

#if defined(USE_BROKEN_COMPRESSION)
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "r", ZFILE_BROKEN,   "broken pipe");
    TEST_EXPECT_ZFOPEN_FAILS(outFile, "w", ZFILE_BROKEN,   "broken pipe");
#endif

    char         *testText;
    const size_t  TEST_TEXT_SIZE = 428;
    {
        GB_ERROR  error = NULL;
        FILE     *in    = ARB_zfopen(inText, "r", ZFILE_UNCOMPRESSED, error, false);
        TEST_EXPECT_NULL(error);
        TEST_REJECT_NULL(in);

        size_t bytes_read;
        testText = fileContent(in, bytes_read);
        TEST_EXPECT_EQUAL(bytes_read, TEST_TEXT_SIZE);

        TEST_EXPECT_NO_ERROR(ARB_zfclose(in));
    }

    int successful_compressions = 0;

    for (FileCompressionMode cmode = FileCompressionMode(ZFILE_AUTODETECT+1);
         cmode != ZFILE_UNDEFINED;
         cmode  = FileCompressionMode(cmode+1))
    {
        TEST_ANNOTATE(GBS_global_string("cmode=%i", int(cmode)));

        bool compressed_save_failed = false;
        {
            GB_ERROR  error = NULL;
            FILE     *out   = ARB_zfopen(outFile, "w", cmode, error, false);

            TEST_EXPECT_NO_ERROR(error);
            TEST_REJECT_NULL(out);

            TEST_EXPECT_DIFFERENT(EOF, fputs(testText, out));

            error = ARB_zfclose(out);
            if (error && strstr(error, "failed with exitcode=127") && cmode != ZFILE_UNCOMPRESSED) {
                // assume compression utility is not installed
                compressed_save_failed = true;
            }
            else {
                TEST_EXPECT_NO_ERROR(error);
            }
        }

        if (!compressed_save_failed) {
            for (int detect = 0; detect<=1; ++detect) {
                TEST_ANNOTATE(GBS_global_string("cmode=%i detect=%i", int(cmode), detect));

                GB_ERROR  error = NULL;
                FILE     *in    = ARB_zfopen(outFile, "r", detect ? ZFILE_AUTODETECT : cmode, error, false);

                TEST_REJECT(error);
                TEST_REJECT_NULL(in);

                size_t  bytes_read;
                char   *content = fileContent(in, bytes_read);
                TEST_EXPECT_NO_ERROR(ARB_zfclose(in));
                TEST_EXPECT_EQUAL(content, testText); // if this fails for detect==1 -> detection does not work
                free(content);
            }
            successful_compressions++;
        }
    }

    TEST_EXPECT(successful_compressions>=3); // at least ZFILE_UNCOMPRESSED, ZFILE_GZIP and ZFILE_BZIP should succeed

    free(testText);
    TEST_EXPECT_DIFFERENT(GB_unlink(outFile), -1);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

