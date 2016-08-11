#include "arb_pattern.h"
#include "arb_core.h"
#include "arb_strbuf.h"
#include "arb_string.h"
#include "arb_msg.h"

#include <wordexp.h>


/**
 * Performs shell-like path expansion on @param str:
 *  - Tilde-Expansion
 *  - Environment variable substitution
 *  - Command substitition
 *  - Arithmetic expansion
 *  - Wildcard expansion
 *  - Quote removal
 * 
 * The implementation uses wordexp (see man wordexp).
 *
 *  @return Expanded string (must be freed)
 */
char* arb_shell_expand(const char* str) {
    char      *expanded = NULL;
    wordexp_t  result;
    GB_ERROR   error    = NULL;

    switch (wordexp(str, &result, 0)) {
        case 0:
        break;
    case WRDE_BADCHAR:
        error = "Illegal character";
        break;
    case WRDE_BADVAL:
        error = "Undefined variable referenced";
        break; 
    case WRDE_CMDSUB:
        error = "Command substitution not allowed";
        break;
    case WRDE_NOSPACE:
        error = "Out of memory";
        break;
    case WRDE_SYNTAX:
        error = "Syntax error";
        break;
    default:
        error = "Unknown error";
    }

    if (error) {
        GB_export_errorf("Encountered error \"%s\" while expanding \"%s\"",
                         error, str);
        expanded = ARB_strdup(str);
    }
    else {
        if (result.we_wordc == 0) {
            expanded = ARB_strdup("");
        }
        else {
            GBS_strstruct *out = GBS_stropen(strlen(str)+100);
            GBS_strcat(out, result.we_wordv[0]);
            for (unsigned int i = 1; i < result.we_wordc; i++) {
                GBS_chrcat(out, ' ');
                GBS_strcat(out, result.we_wordv[i]);
            }

            expanded = GBS_strclose(out);
        }
        wordfree(&result);
    }
    return expanded;
}

////////// UNIT TESTS ///////////

#ifdef UNIT_TESTS

#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_arb_shell_expand() {
    char *res;

    res = arb_shell_expand("");
    TEST_REJECT(GB_have_error());
    TEST_EXPECT_EQUAL(res, "");
    free(res);

    res = arb_shell_expand("test");
    TEST_REJECT(GB_have_error());
    TEST_EXPECT_EQUAL(res, "test");
    free(res);

    res = arb_shell_expand("$ARBHOME");
    TEST_REJECT(GB_have_error());
    TEST_EXPECT_EQUAL(res, getenv("ARBHOME"));
    free(res);

    res = arb_shell_expand("$ARBHOME&");
    TEST_EXPECT(GB_have_error());
    GB_await_error();
    TEST_EXPECT_EQUAL(res, "$ARBHOME&");
    free(res);
    
}



#endif
