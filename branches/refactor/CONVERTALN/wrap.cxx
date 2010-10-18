#include "seq.h"
#include "wrap.h"
#include "reader.h"

int WrapMode::wrap_pos(const char *str, int wrapCol) const {
    // returns the first position lower equal 'wrapCol' after which splitting
    // is possible (according to separators of WrapMode)
    //
    // returns 'wrapCol' if no such position exists (fallback)
    //
    // throws if WrapMode is disabled

    if (!allowed_to_wrap()) throw_errorf(50, "Oversized content - no wrapping allowed here (content='%s')", str);

    ca_assert(wrapCol <= (int)strlen(str)); // to short to wrap

    const char *wrapAfter = get_seps();
    ca_assert(wrapAfter);
    int i = wrapCol;
    for (; i >= 0 && !occurs_in(str[i], wrapAfter); --i) {}
    return i >= 0 ? i : wrapCol;
}

const char *WrapMode::print_return_wrapped(Writer& write, const char * const content, const int len, const int rest_width, WrapBug behavior) const {
    // @@@ currently reproduces all bugs found in the previously six individual wrapping functions

    ca_assert(content[len] == '\n');

    if (len<(rest_width+1)) {
        write.out(content);
        return NULL; // no rest
    }

    int split_after = wrap_pos(content, rest_width);

    if (behavior&WRAPBUG_WRAP_AT_SPACE && split_after == 0) split_after = wrap_pos(content+1, rest_width-1)+1;
    if (behavior&WRAPBUG_WRAP_BEFORE_SEP && split_after>0) split_after--;

    ca_assert(split_after>0);
    if ((behavior&WRAPBUG_WRAP_BEFORE_SEP) == 0 && occurs_in(content[split_after], " \n")) split_after--;
    ca_assert(split_after >= 0);

    int continue_at = split_after+1;
    if (behavior&WRAPBUG_WRAP_AT_SPACE && split_after == rest_width) {
        // don't correct 'continue_at' in this case to emulate existing bug
    }
    else {
        while (occurs_in(content[continue_at], " \n")) continue_at++;
    }

    if (behavior&WRAPBUG_WRAP_BEFORE_SEP && content[split_after+1] == ' ' && content[split_after+2] == ' ') split_after++;

    ca_assert(content[split_after] != '\n');
    fputs_len(content, split_after+1, write);
    write.out('\n');

    ca_assert(content[len] == '\n');
    ca_assert(len>continue_at);

    return content+continue_at;
}

void WrapMode::print(Writer& write, const char *first_prefix, const char *other_prefix, const char *content, int max_width, WrapBug behavior) const {
    ca_assert(has_content(content));

    int len        = strlen(content)-1;
    int prefix_len = strlen(first_prefix);

    write.out(first_prefix);
    const char *rest = print_return_wrapped(write, content, len, max_width-prefix_len, behavior);

    if (rest) {
        prefix_len = strlen(other_prefix);

        while (rest) {
            len     -= rest-content;
            content  = rest;

            write.out(other_prefix);
            rest = print_return_wrapped(write, content, len, max_width-prefix_len, behavior);
        }
    }
}

