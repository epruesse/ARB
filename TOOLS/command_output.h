// ============================================================= //
//                                                               //
//   File      : command_output.h                                //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef COMMAND_OUTPUT_H
#define COMMAND_OUTPUT_H

#ifndef ARB_FILE_H
#include <arb_file.h>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef UT_VALGRINDED_H
#include <ut_valgrinded.h>
#endif

// --------------------------------------------------------------------------------

class CommandOutput : virtual Noncopyable {
    // support class to test CLI of arb-tools

    char     *stdoutput; // output from command
    char     *stderrput;
    GB_ERROR  error;

    void appendError(GB_ERROR err) {
        if (!error) error = err;
        else error = GBS_global_string("%s\n%s", error, err);
    }

    char *readAndUnlink(const char *file) {
        char *content = GB_read_file(file);
        if (!content) appendError(GB_await_error());
        int res = GB_unlink(file);
        if (res == -1) appendError(GB_await_error());
        return content;
    }
public:
    CommandOutput(const char *command, bool try_valgrind)
        : stdoutput(NULL), stderrput(NULL), error(NULL)
    {
        char *escaped = GBS_string_eval(command, "'=\\\\'", NULL);
        if (!escaped) {
            appendError(GB_await_error());
        }
        else {
            if (try_valgrind) make_valgrinded_call(escaped);

            pid_t pid = getpid();

            char *stdout_log = GBS_global_string_copy("stdout_%i.log", pid);
            char *stderr_log = GBS_global_string_copy("stderr_%i.log", pid);

            char *cmd = GBS_global_string_copy("bash -c '%s >%s 2>%s'", escaped, stdout_log, stderr_log);

            appendError(GBK_system(cmd));
            free(cmd);
            free(escaped);

            stdoutput = readAndUnlink(stdout_log);
            stderrput = readAndUnlink(stderr_log);

            free(stderr_log);
            free(stdout_log);
        }
        if (error) {
            printf("command '%s'\n"
                   "escaped command '%s'\n"
                   "stdout='%s'\n"
                   "stderr='%s'\n", 
                   command, escaped, stdoutput, stderrput);
        }
    }
    ~CommandOutput() {
        free(stderrput);
        free(stdoutput);
    }

    arb_test::match_expectation Equals(const char *expected_std, const char *expected_err) {
        using namespace arb_test;
        expectation_group expected(that(error).equals(NULL));
        if (expected_std) expected.add(that(stdoutput).equals(expected_std));
        if (expected_err) expected.add(that(stderrput).equals(expected_err));
        return all().ofgroup(expected);
    }
    arb_test::match_expectation Contains(const char *expected_std, const char *expected_err) {
        using namespace arb_test;
        expectation_group expected(that(error).equals(NULL));
        if (expected_std) expected.add(that(stdoutput).contains(expected_std));
        if (expected_err) expected.add(that(stderrput).contains(expected_err));
        return all().ofgroup(expected);
    }
    arb_test::match_expectation has_checksum(uint32_t expected_checksum) {
        uint32_t css      = GBS_checksum(stdoutput, false, "");
        uint32_t cse      = GBS_checksum(stderrput, false, "");
        uint32_t checksum = css^cse;
        using namespace arb_test;
        return all().of(that(error).equals(NULL),
                        that(checksum).equals(expected_checksum));
    }
};

// --------------------------------------------------------------------------------

#define TEST_OUTPUT_EQUALS(cmd, expected_std, expected_err)                                             \
    do {                                                                                                \
        bool try_valgrind = (expected_err == NULL);                                                     \
        TEST_EXPECT(CommandOutput(cmd, try_valgrind).Equals(expected_std, expected_err));               \
    } while(0)

#define TEST_OUTPUT_EQUALS__BROKEN(cmd, expected_std, expected_err)                                     \
    do {                                                                                                \
        bool try_valgrind = (expected_err == NULL);                                                     \
        TEST_EXPECT__BROKEN(CommandOutput(cmd, try_valgrind).Equals(expected_std, expected_err));       \
    } while(0)

#define TEST_OUTPUT_CONTAINS(cmd, expected_std, expected_err)                                           \
    do {                                                                                                \
        bool try_valgrind = (expected_err == NULL);                                                     \
        TEST_EXPECT(CommandOutput(cmd, try_valgrind).Contains(expected_std, expected_err));             \
    } while(0)

#define TEST_OUTPUT_CONTAINS__BROKEN(cmd, expected_std, expected_err)                                   \
    do {                                                                                                \
        bool try_valgrind = (expected_err == NULL);                                                     \
        TEST_EXPECT__BROKEN(CommandOutput(cmd, try_valgrind).Contains(expected_std, expected_err));     \
    } while(0)

#define TEST_OUTPUT_HAS_CHECKSUM(cmd,checksum)         TEST_EXPECT        (CommandOutput(cmd, false).has_checksum(checksum))
#define TEST_OUTPUT_HAS_CHECKSUM__BROKEN(cmd,checksum) TEST_EXPECT__BROKEN(CommandOutput(cmd, false).has_checksum(checksum))

#define TEST_STDOUT_EQUALS(cmd, expected_std) TEST_OUTPUT_EQUALS(cmd, expected_std, (const char *)NULL)
#define TEST_STDERR_EQUALS(cmd, expected_err) TEST_OUTPUT_EQUALS(cmd, (const char *)NULL, expected_err)

#define TEST_STDOUT_CONTAINS(cmd, part) TEST_OUTPUT_CONTAINS(cmd, part, "")

#else
#error command_output.h included twice
#endif // COMMAND_OUTPUT_H
