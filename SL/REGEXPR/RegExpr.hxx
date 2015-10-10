// ============================================================= //
//                                                               //
//   File      : RegExpr.hxx                                     //
//   Purpose   : Wrapper for ARBDB regular expressions           //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2009   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef REGEXPR_HXX
#define REGEXPR_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define re_assert(cond) arb_assert(cond)

#ifndef _GLIBCXX_STRING
#include <string>
#endif

struct GBS_regex;

class RegMatch {
    size_t start;
    size_t behind_last;

public:
    bool didMatch() const { return start != std::string::npos; }

    RegMatch() : start(std::string::npos), behind_last(std::string::npos) {}
    RegMatch(size_t start_, size_t behind_last_)
        : start(start_)
        , behind_last(behind_last_)
    {
        re_assert(start != std::string::npos);
        re_assert(behind_last != std::string::npos);
        re_assert(start <= behind_last);
    }

    size_t pos() const { return start; }
    size_t len() const { return behind_last-start; }

    size_t posBehindMatch() const { return behind_last; }

    std::string extract(const std::string& s) const {
        re_assert(didMatch());
        return s.substr(pos(), len());
    }
};


class RegExpr : virtual Noncopyable {
    //! for regexpression format see http://help.arb-home.de/reg.html#Syntax_of_POSIX_extended_regular_expressions_as_used_in_ARB

    std::string expression;                         // the regular expression
    bool        ignore_case;

    mutable GBS_regex *comreg;                      // compiled regular expression (NULL if not compiled yet)
    mutable RegMatch  *matches;                     // set by match (NULL if failed or not performed yet)

    void compile() const;
    void perform_match(const char *str, size_t offset) const;

public:
    RegExpr(const std::string& expression_, bool ignore_case);
    ~RegExpr();

    size_t subexpr_count() const;

    // Note: calling 'match()' invalidates results from previous 'match()' and 'subexpr_match()'-calls
    const RegMatch *match(const std::string& versus, size_t offset = 0) const;
    const RegMatch *subexpr_match(size_t subnr) const; // get subexpression match from last 'match()'
};

#else
#error RegExpr.hxx included twice
#endif // REGEXPR_HXX
