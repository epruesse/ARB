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

    GB_ERROR get_error() const { return error; }
    const char *get_stdoutput() const { return stdoutput; }
    const char *get_stderrput() const { return stderrput; }
};

// --------------------------------------------------------------------------------

#define TEST_OUTPUT_EQUALS(cmd, expected_std, expected_err)             \
    do {                                                                \
        CommandOutput out(cmd, expected_err == NULL);                   \
        TEST_ASSERT_NO_ERROR(out.get_error());                          \
        if (expected_std) { TEST_ASSERT_EQUAL(out.get_stdoutput(), expected_std); } \
        if (expected_err) { TEST_ASSERT_EQUAL(out.get_stderrput(), expected_err); } \
    } while(0)

#define TEST_OUTPUT_CONTAINS(cmd, expected_std, expected_err)           \
    do {                                                                \
        CommandOutput out(cmd, expected_err == NULL);                   \
        TEST_ASSERT_NO_ERROR(out.get_error());                          \
        if (expected_std) { TEST_ASSERT_CONTAINS(out.get_stdoutput(), expected_std); } \
        if (expected_err) { TEST_ASSERT_CONTAINS(out.get_stderrput(), expected_err); } \
    } while(0)

#define TEST_OUTPUT_HAS_CHECKSUM(cmd, checksum)                         \
    do {                                                                \
        CommandOutput out(cmd, false);                                  \
        TEST_ASSERT_NO_ERROR(out.get_error());                          \
        uint32_t      css = GBS_checksum(out.get_stdoutput(), false, ""); \
        uint32_t      cse = GBS_checksum(out.get_stderrput(), false, ""); \
        TEST_ASSERT_EQUAL(css^cse, uint32_t(checksum));                 \
    } while(0)

#define TEST_STDOUT_EQUALS(cmd, expected_std) TEST_OUTPUT_EQUALS(cmd, expected_std, (const char *)NULL)
#define TEST_STDERR_EQUALS(cmd, expected_err) TEST_OUTPUT_EQUALS(cmd, (const char *)NULL, expected_err)

#define TEST_STDOUT_CONTAINS(cmd, part) TEST_OUTPUT_CONTAINS(cmd, part, "")

#else
#error command_output.h included twice
#endif // COMMAND_OUTPUT_H
