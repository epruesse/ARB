// ================================================================ //
//                                                                  //
//   File      : test_unit.h                                        //
//   Purpose   : include into test code                             //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef TEST_UNIT_H
#define TEST_UNIT_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _GLIBCXX_CSTDARG
#include <cstdarg>
#endif
#ifndef _CPP_CSTDLIB
#include <cstdlib>
#endif
#ifndef ERRNO_H
#include <errno.h>
#endif
#ifndef DUPSTR_H
#include <dupstr.h>
#endif

#if defined(_GLIBCXX_STRING)
#define TESTS_KNOW_STRING
#endif
#ifdef ARBDB_BASE_H
#define TESTS_KNOW_ARBDB
#endif


#define ENABLE_CRASH_TESTS // comment out this line to get rid of provoked SEGVs (e.g. while debugging test-code)

/* Note:
 * This file should not generate any static code.
 * Only place define's or inline functions here.
 * 
 * All macros named 'XXX__BROKEN' are intended to be used, when a
 * test is known to fail, but cannot be fixed atm for some reason
 *
 * Recommended test-assertion is TEST_EXPECTATION(that(..).xxx())
 * see examples in test-unit-tests in ../CORE/arb_string.cxx@UNIT_TESTS
 */

#define ANY_SETUP "any_env_setup"

namespace arb_test {

    // -------------
    //      str

    class str {
        char *s;
    public:
        str() : s(0) {}
        str(const str& other) : s(nulldup(other.s)) {}
        explicit str(char *S) : s(S) {}
        str& operator = (const str& other) {
            freedup(s, other.s);
            return *this;
        }
        str& operator = (char *S) {
            freeset(s, S);
            return *this;
        }
        ~str() { free(s); }

        bool exists() const { return s; }
        void assign(char *S) { s = S; }
        const char *value() const { return s; }
    };

    // -----------------------
    //      location info

#define WITHVALISTFROM(format,CODE)  do { va_list parg; va_start(parg, format); CODE; va_end(parg); } while(0)
#define VPRINTFORMAT(format)         WITHVALISTFROM(format, vfprintf(stderr, format, parg))
#define VCOMPILERMSG(msgtype,format) WITHVALISTFROM(format, vcompiler_msg(msgtype, format, parg))

    class locinfo //! stores source code location
    {
        const char *file;
        int         line;

        __attribute__((format(__printf__, 2, 0))) void vcompiler_msg(const char *message_type, const char *format, va_list parg) const {
            fprintf(stderr, "%s:%i: ", file, line);
            if (message_type) fprintf(stderr, "%s: ", message_type);
            vfprintf(stderr, format, parg);
        }
        
    public:
        locinfo() : file(0), line(0) {}
        locinfo(const char *file_, int line_) : file(file_), line(line_) {}

        bool exists() const { return file; }

        const char *get_file() const { return file; }
        int get_line() const { return line; }

        __attribute__((format(printf, 2, 3))) bool warningf(const char *format, ...) const {
            GlobalTestData& global = test_data();
            if (global.show_warnings) {
                FlushedOutput yes;
                VCOMPILERMSG("Warning", format);
                GlobalTestData::print_annotation();
                global.warnings++;
            }
            return global.show_warnings;
        }

        __attribute__((format(printf, 3, 4))) void errorf(bool fail, const char *format, ...) const {
            {
                FlushedOutput yes;
                VCOMPILERMSG("Error", format);
                GlobalTestData::print_annotation();
            }
            if (fail) TRIGGER_ASSERTION(false); // fake an assertion failure
        }
        __attribute__((format(printf, 3, 4))) void ioerrorf(bool fail, const char *format, ...) const {
            {
                FlushedOutput yes;
                VCOMPILERMSG("Error", format);
                fprintf(stderr, " (errno=%i='%s')", errno, strerror(errno));
                GlobalTestData::print_annotation();
            }
            if (fail) TRIGGER_ASSERTION(false); // fake an assertion failure
        }
    };

    // --------------------
    //      StaticCode

    struct StaticCode {
        static void printf(const char *format, ...) __attribute__((format(printf, 1, 2))) {
            FlushedOutputNoLF yes;
            VPRINTFORMAT(format);
        }
#undef VPRINTFORMAT
#undef VCOMPILERMSG
#undef WITHVALISTFROM

        static char *readable_string(const char *s) {
            // quote like C does!
            if (s) {
                size_t  len    = strlen(s)*4;
                char   *res = (char*)malloc(len+2+1);

                int j     = 0;
                res[j++] = '\"';
                for (int i = 0; s[i]; ++i) {
                    unsigned char c = static_cast<unsigned char>(s[i]);
                    char esc = 0;
                    switch (c) {
                        case '\n': esc = 'n'; break;
                        case '\t': esc = 't'; break;
                        case '\"': esc = '\"'; break;
                        case '\\': esc = '\\'; break;
                        default: if (c<10) esc = c-1+'1'; break;
                    }
                    if (esc) {
                        res[j++] = '\\';
                        res[j++] = esc;
                    }
                    else {
                        if (c >= 32 && c<127) {
                            res[j++] = c;
                        }
                        else {
                            j += sprintf(res+j, "\\x%02x", int(c));
                        }
                    }
                }
                res[j++] = '\"';
                res[j++] = 0;
                return res;
            }
            else {
                return strdup("(null)");
            }
        }
        static void print_readable_string(const char *s, FILE *out) {
            fputs(str(readable_string(s)).value(), out);
        }

        static char *vstrf(const char *format, va_list& parg) __attribute__((format(__printf__, 1, 0))) {
            static const size_t max_vstrf_size = 10000;
            static char         vstrf_buf[max_vstrf_size];

            int printed = vsnprintf(vstrf_buf, max_vstrf_size, format, parg);
            arb_assert(printed >= 0 && size_t(printed)<max_vstrf_size);

            char *result    = (char*)malloc(printed+1);
            memcpy(result, vstrf_buf, printed);
            result[printed] = 0;
            return result;
        }

        static char *strf(const char *format, ...) __attribute__((format(__printf__, 1, 2))) {
            va_list  parg;
            va_start(parg, format);
            char *result = vstrf(format, parg);
            va_end(parg);
            return result;
        }

    };

    inline char *val2readable(bool b) { return strdup(b ? "true" : "false"); }
    
    inline char *val2readable(int i) { return StaticCode::strf("%i", i); }
    inline char *val2hex(int i) { return StaticCode::strf("0x%x", i); }

    inline char *val2readable(long L) { return StaticCode::strf("%li", L); }
    inline char *val2hex(long L) { return StaticCode::strf("0x%lx", L); }

    inline char *val2readable(size_t z) { return StaticCode::strf("%zu", z); }
    inline char *val2hex(size_t z) { return StaticCode::strf("0x%zx", z); }

    // dont dup size_t:
#ifdef ARB_64
    inline char *val2readable(unsigned u) { return StaticCode::strf("%u", u); }
    inline char *val2hex(unsigned u) { return StaticCode::strf("0x%x", u); }
#else
    inline char *val2readable(long unsigned u) { return StaticCode::strf("%lu", u); }
    inline char *val2hex(long unsigned u) { return StaticCode::strf("0x%lx", u); }
#endif

    inline char *val2readable(double d) { return StaticCode::strf("%f", d); }

    inline char *val2readable(unsigned char c) { return c<32 ? StaticCode::strf(" ^%c (=0x%02x)", c+'A'-1, int(c)) : StaticCode::strf("'%c' (=0x%02x)", c, int(c)); }
    inline char *val2readable(const char *s) { return StaticCode::readable_string(s); }

#ifdef TESTS_KNOW_STRING
    inline char *val2readable(const std::string& s) { return StaticCode::readable_string(s.c_str()); }
#endif
#if defined(TESTS_KNOW_ARBDB)
    inline char *val2readable(const GBDATA* gbd) { return StaticCode::strf("%p", gbd); }
#endif

    // ------------------
    //      copy


    template <typename T>
    class copy //! makes char* copyable, so it can be handled like most other types
    {
        T t;
    public:
        copy(const T& t_) : t(t_) {}
        operator const T&() const { return t; }
    };

    template <>
    class copy<const char *> {
        str t;
    public:
        copy(const char *t_) : t(str(t_ ? strdup(t_) : NULL)) {}
        operator const char *() const { return t.value(); }
    };

    template <typename T> class copy< copy<T> > { copy(const copy<T>& t_); }; // avoid copies of copies

    template <typename T> inline copy<T> make_copy(const T& t) { return copy<T>(t); }

    inline copy<const char *> make_copy(const char *p) { return copy<const char *>(p); }
    inline copy<const char *> make_copy(char *p) { return copy<const char *>(p); }
    inline copy<const char *> make_copy(const unsigned char *p) { return copy<const char *>(reinterpret_cast<const char *>(p)); }
    inline copy<const char *> make_copy(unsigned char *p) { return copy<const char *>(reinterpret_cast<const char *>(p)); }
    inline copy<const char *> make_copy(const signed char *p) { return copy<const char *>(reinterpret_cast<const char *>(p)); }
    inline copy<const char *> make_copy(signed char *p) { return copy<const char *>(reinterpret_cast<const char *>(p)); }

    inline copy<unsigned char> make_copy(char p) { return copy<unsigned char>(p); }
    inline copy<unsigned char> make_copy(unsigned char p) { return copy<unsigned char>(p); }
    inline copy<unsigned char> make_copy(signed char p) { return copy<unsigned char>(p); }

    template <typename T> str readable(const copy<T>& v) { return str(val2readable(v)); }
    template <typename T> str readableHex(const copy<T>& v) { return str(val2readableHex(v)); }

    template <typename T> bool operator == (const copy<T>& v1, const copy<T>& v2) { return static_cast<const T&>(v1) == static_cast<const T&>(v2); }
    template <typename T> bool operator != (const copy<T>& v1, const copy<T>& v2) { return !(v1 == v2); }

    template <> inline bool operator == <const char *>(const copy<const char *>& v1, const copy<const char *>& v2) {
        const char *val1 = v1;
        const char *val2 = v2;

        return (val1 == val2) || (val1 && val2 && (strcmp(val1, val2) == 0));
    }


    template <typename T> inline void print(const T& t) {
        char *r = val2readable(make_copy(t));
        fputs(r, stderr);
        free(r);
    }
    template <typename T> inline void print_hex(const T& t) { fputs(val2hex(make_copy(t)), stderr); }

    template <typename T1, typename T2> inline void print_pair(T1 t1, T2 t2) {
        print(t1);
        fputs(",", stderr);
        print(t2);
    }
    template <typename T1, typename T2> inline void print_hex_pair(T1 t1, T2 t2) {
        print_hex(t1);
        fputc(',', stderr);
        print_hex(t2);
    }

    class epsilon_similar {
        double epsilon;
    public:
        epsilon_similar(double epsilon_) : epsilon(epsilon_) { arb_assert(epsilon>0.0); }
        bool operator()(const double& d1, const double& d2) const {
            double diff        = d1-d2;
            if (diff<0.0) diff = -diff; // do not use fabs() here
            return diff <= epsilon;
        }
    };

    struct containing {
        bool operator()(const char *str, const char *part) const { return strstr(str, part); }
#if defined(TESTS_KNOW_STRING)
        bool operator()(const std::string& str, const std::string& part) const { return strstr(str.c_str(), part.c_str()); }
#endif
    };

    // -------------------------------
    //      some output functions

    inline void print(const char *s) { fputs(s, stderr); }
    inline void print(char c) { fputc(c, stderr); }
    inline void print(int i) { fprintf(stderr, "%i", i); }

    inline void space() { print(' '); }
    inline void nl() { print('\n'); }
    
    inline void print_indent(int indent) { while (indent--) space(); }

    inline void spaced(const char *word) { space(); print(word); space(); }
    inline void select_spaced(bool first, const char *singular, const char *plural) { spaced(first ? singular : plural); }
    

#define HASTOBE_CLONABLE(type) virtual type *clone() const = 0
#define MAKE_CLONABLE(type)    type *clone() const OVERRIDE { return new type(*this); }

    // ------------------------------
    //      abstract expectation

    struct expectation //! something expected. can be fulfilled or not (and can explain why/not)
    {
        virtual ~expectation() {}
        HASTOBE_CLONABLE(expectation);

        virtual bool fulfilled() const              = 0;
        virtual void explain(int indent) const      = 0;
        virtual void dump_brief_description() const = 0;
    };

    // -------------------
    //      asserters

    class asserter : virtual Noncopyable {
        expectation *expected;
        locinfo      loc;
        const char  *code;

        virtual void announce_failure() const { TRIGGER_ASSERTION(false); }

        void err(const char *format) const { loc.errorf(false, format, code); }
        bool warn(const char *format) const { return loc.warningf(format, code); }
        
    public:
        asserter(const expectation& e, const char *nontmp_code, const char *file, int line)
            : expected(e.clone()),
              loc(file, line),
              code(nontmp_code)
        {}
        virtual ~asserter() { delete expected; }

        const char *get_code() const { return code; }

        void expect_that() const {
            if (!expected->fulfilled()) {
                err("Failed expectation '%s'");
                print("expectation fails because\n");
                expected->explain(2); print('\n');
                announce_failure();
            }
        }
        void expect_broken() const {
            if (expected->fulfilled()) {
                err("Previously broken expectation '%s' succeeds");
                announce_failure();
            }
            else {
                if (warn("Expectation '%s' known as broken (accepted until fixed)")) {
                    print("Broken because\n");
                    expected->explain(2); print('\n');
                }
            }
        }

        void expect_wanted_behavior() const {
            if (expected->fulfilled()) {
                err("Wanted behavior '%s' reached");
                announce_failure();
            }
            else {
                if (warn("Wanted behavior: '%s'")) {
                    print("Unsatisfied because\n");
                    expected->explain(2); print('\n');
                }
            }
        }

        void expect_brokenif(bool condition, const char *condcode) {
            GlobalTestData& global = test_data();

            char *old_annotation = nulldup(global.get_annotation());
            char *new_annotation = StaticCode::strf("when (%s) is %s; %s",
                                                    condcode,
                                                    str(val2readable(condition)).value(),
                                                    old_annotation ? old_annotation : "");

            global.annotate(new_annotation);
            if (condition) expect_broken(); else expect_that();
            global.annotate(old_annotation);

            free(new_annotation);
            free(old_annotation);
        }
    };

    class debug_asserter : public asserter {
        virtual void announce_failure() const OVERRIDE {
            print("<<< would trigger assertion now! >>>\n");
        }
    public:
        debug_asserter(const expectation& e, const char *code_, const char *file, int line)
            : asserter(e, code_, file, line)
        {}

        void debug_expectations() {
            fprintf(stderr, "-------------------- [Debugging expectations for '%s']\n", get_code());
            expect_that();
            expect_broken();
            expect_wanted_behavior();
        }
    };

    // ----------------------------------------
    //      matchable + matcher (abstract)

    struct matchable //! can be matched with corresponding matcher.
    {
        virtual ~matchable() {}
        HASTOBE_CLONABLE(matchable);

        virtual const char *name() const           = 0;
        virtual const char *readable_value() const = 0;
    };

    struct matcher //! can match things.
    {
        virtual ~matcher() {}
        HASTOBE_CLONABLE(matcher);

        virtual bool matches(const matchable& thing) const                      = 0;
        virtual void dump_expectation(const matchable& thing, int indent) const = 0;
        virtual void dump_brief_description(const matchable& thing) const       = 0;
    };

    // ----------------------------------------------
    //      expectation from matchable + matcher


    class match_expectation : public expectation //! expectation composed from matcher and corresponding matchable.
    {
        matchable *thing;
        matcher   *condition;
    public:
        match_expectation(const matchable& thing_, const matcher& condition_)
            : thing(thing_.clone()),
              condition(condition_.clone())
        {}
        match_expectation(const match_expectation& other)
            : thing(other.thing->clone()),
              condition(other.condition->clone())
        {}
        DECLARE_ASSIGNMENT_OPERATOR(match_expectation);
        MAKE_CLONABLE(match_expectation);
        ~match_expectation() OVERRIDE {
            delete thing;
            delete condition;
        }

        bool fulfilled() const OVERRIDE { return condition->matches(*thing); }
        bool unfulfilled() const { return !fulfilled(); }
        void explain(int indent) const OVERRIDE { condition->dump_expectation(*thing, indent); }
        void dump_brief_description() const OVERRIDE { condition->dump_brief_description(*thing); }
    };

    // -------------------
    //      predicate


    class predicate_description {
        const char  *primary;
        const char  *inverse;
        mutable str  tmp;

        void erase_last_from_tmp() const {
            char *t   = const_cast<char*>(tmp.value());
            int   len = strlen(t);
            t[len-1]  = 0;
        }

        static bool ends_with_s(const char *s) {
            int len          = strlen(s);
            return s[len-1] == 's';
        }

        const char *make(const char *desc, bool got) const {
            if (ends_with_s(desc)) {
                if (got) return desc;
                tmp = StaticCode::strf("doesnt %s", desc);
                erase_last_from_tmp();
            }
            else {
                tmp = StaticCode::strf("%s %s", got ? "is" : "isnt", desc);
            }
            return tmp.value();
        }

    public:
        predicate_description(const char *primary_) : primary(primary_), inverse(NULL) {}
        predicate_description(const char *primary_, const char *inverse_) : primary(primary_), inverse(inverse_) {}
        // cppcheck-suppress uninitMemberVar (fails to detect default ctor of 'str')
        predicate_description(const predicate_description& other) : primary(other.primary), inverse(other.inverse) {}
        DECLARE_ASSIGNMENT_OPERATOR(predicate_description);

        const char *make(bool expected, bool got) const {
            if (expected) return make(primary, got);
            if (inverse) return make(inverse, !got);
            return make(primary, got);
        }
    };

    template <typename FUNC>
    class predicate {
        FUNC                  pred;
        predicate_description description;
    public:
        predicate(FUNC pred_, const char *name) : pred(pred_), description(name) {}
        predicate(FUNC pred_, const char *name, const char *inverse) : pred(pred_), description(name, inverse) {}

        template <typename T> bool matches(const copy<T>& v1, const copy<T>& v2) const { return pred(v1, v2); }
        const char *describe(bool expected, bool got) const { return description.make(expected, got); }
    };

    template <typename FUNC> predicate<FUNC> make_predicate(FUNC func, const char *primary, const char *inverse) {
        return predicate<FUNC>(func, primary, inverse);
    }

    // ------------------------------------------
    //      matchable + matcher (for values)

    template <typename T> inline bool equals(const copy<T>& t1, const copy<T>& t2) { return t1 == t2; }
    template <typename T> inline bool less(const copy<T>& t1, const copy<T>& t2) { return t1 < t2; }
    template <typename T> inline bool more(const copy<T>& t1, const copy<T>& t2) { return t1 > t2; }

    template <typename T>
    class matchable_value : public matchable //! matchable for values
    {
        copy<T>      val;
        mutable str  readable;
        const char * code;
    public:
        matchable_value(copy<T> val_, const char *nontemp_code) : val(val_), code(nontemp_code) {}
        matchable_value(const matchable_value<T>& other) : val(other.val), readable(other.readable), code(other.code) {}
        DECLARE_ASSIGNMENT_OPERATOR(matchable_value);
        MAKE_CLONABLE(matchable_value<T>);

        const copy<T>& value() const { return val; }
        char *gen_description() const { return StaticCode::strf("%s (=%s)", code, val.readable()); }

        const char *name() const OVERRIDE { return code; }
        const char *readable_value() const OVERRIDE {
            if (!readable.exists()) readable = arb_test::readable(val);
            return readable.value();
        }

        template <typename U> inline match_expectation equals_expectation(bool invert, const U& other, const char *code) const;
        template <typename U> inline match_expectation lessThan_expectation(bool invert, const U& other, const char *code) const;
        template <typename U> inline match_expectation moreThan_expectation(bool invert, const U& other, const char *code) const;
        
        inline match_expectation null_expectation(bool wantNULL) const;

        template <typename FUNC> inline match_expectation predicate_expectation(bool wanted, predicate<FUNC> pred, matchable_value<T> arg) const;
        template <typename U, typename FUNC> inline match_expectation predicate_expectation(bool wanted, FUNC pred, const char *pred_code, const U& arg, const char *arg_code) const;
    };

    template <typename T, typename U>
    inline const matchable_value<T> make_matchable_value(const U& other, const char *code_) {
        return matchable_value<T>(T(other), code_);
    }
#if defined(TESTS_KNOW_STRING)
    template<>
    inline const matchable_value<const char*> make_matchable_value<const char *, std::string>(const std::string& other, const char *code_) {
        return matchable_value<const char *>(other.c_str(), code_);
    }
#endif
    template <typename T> template <typename U>
    inline match_expectation matchable_value<T>::equals_expectation(bool wanted, const U& other, const char *code_) const {
        return predicate_expectation(wanted, make_predicate(equals<T>, "equals", "differs"), make_matchable_value<T,U>(other, code_));
    }
    template <typename T> template <typename U>
    inline match_expectation matchable_value<T>::lessThan_expectation(bool wanted, const U& other, const char *code_) const {
        return predicate_expectation(wanted, make_predicate(less<T>, "less than", "more or equal"), make_matchable_value<T,U>(other, code_));
    }
    template <typename T> template <typename U>
    inline match_expectation matchable_value<T>::moreThan_expectation(bool wanted, const U& other, const char *code_) const {
        return predicate_expectation(wanted, make_predicate(more<T>, "more than", "less or equal"), make_matchable_value<T,U>(other, code_));
    }
    template <typename T>
    inline match_expectation matchable_value<T>::null_expectation(bool wantNULL) const {
        return equals_expectation(wantNULL, (T)NULL, "NULL");
    }

    template <typename T>
    class value_matcher : public matcher //! matcher for values
    {
        matchable_value<T> expected;
    public:
        value_matcher(const matchable_value<T>& expected_) : expected(expected_) {}
        virtual ~value_matcher() OVERRIDE {}

        virtual bool matches(const copy<T>& v1, const copy<T>& v2) const = 0;
        virtual const char *relation(bool isMatch) const                 = 0;

        const matchable_value<T>& get_expected() const { return expected; }

        bool matches(const matchable& thing) const OVERRIDE {
            const matchable_value<T>& value_thing = dynamic_cast<const matchable_value<T>&>(thing);
            return matches(value_thing.value(), expected.value());
        }

        void dump_expectation(const matchable& thing, int indent) const OVERRIDE {
            bool isMatch = matches(thing);
            print_indent(indent);
            fprintf(stderr, "'%s' %s '%s'", thing.name(), relation(isMatch), expected.name());

            const matchable_value<T>& value_thing = dynamic_cast<const matchable_value<T>&>(thing);
            if (equals<T>(value_thing.value(), expected.value())) {
                fprintf(stderr, " (both are %s)", value_thing.readable_value());
            }
            else {
                int diff = strlen(thing.name())-strlen(expected.name());

                print(", where\n");
                indent += 2;;
                print_indent(indent); fprintf(stderr, "'%s'%*s is %s, and\n", thing.name(),    (diff>0 ? 0 : -diff), "", thing.readable_value());
                print_indent(indent); fprintf(stderr, "'%s'%*s is %s",        expected.name(), (diff<0 ? 0 : diff),  "", expected.readable_value());
            }
        }
        void dump_brief_description(const matchable& thing) const OVERRIDE {
            print(thing.name());
            print('.');
            print(relation(true));
            print('('); print(expected.name()); print(')');
        }

    };

    // ---------------------------
    //      predicate_matcher


    template <typename T, typename FUNC>
    // cppcheck-suppress noConstructor (fails to detect template ctor)
    class predicate_matcher : public value_matcher<T> {
        predicate<FUNC> pred;
        bool            expected_result;

    public:
        predicate_matcher(bool wanted, predicate<FUNC> pred_, const matchable_value<T>& arg)
            : value_matcher<T>(arg),
              pred(pred_),
              expected_result(wanted)
        {}
        MAKE_CLONABLE(predicate_matcher);

        bool matches(const copy<T>& v1, const copy<T>& v2) const OVERRIDE { return correlated(pred.matches(v1, v2), expected_result); }
        const char *relation(bool isMatch) const OVERRIDE { return pred.describe(expected_result, correlated(isMatch, expected_result)); }
    };

    template <typename T> template <typename FUNC>
    inline match_expectation matchable_value<T>::predicate_expectation(bool wanted, predicate<FUNC> pred, matchable_value<T> arg) const {
        return match_expectation(*this, predicate_matcher<T,FUNC>(wanted, pred, arg));
    }
    template <typename T> template <typename U, typename FUNC>
    inline match_expectation matchable_value<T>::predicate_expectation(bool wanted, FUNC pred, const char *pred_code, const U& arg, const char *arg_code) const {
        return match_expectation(*this, predicate_matcher<T,FUNC>(wanted, predicate<FUNC>(pred, pred_code), make_matchable_value<T,U>(arg, arg_code)));
    }

    // ------------------------------------------------
    //      matchable + matcher (for expectations)

    const int MAX_GROUP_SIZE = 5;
    class expectation_group : public matchable //! group of expectation. matchable with group_matcher
    {
        int          count;
        expectation *depend_on[MAX_GROUP_SIZE];

        expectation_group& operator = (const expectation_group&); // forbidden
    protected:

    public:
        expectation_group() : count(0) { depend_on[0] = NULL; }
        expectation_group(const expectation& e) : count(1) {
            depend_on[0] = e.clone();
        }
        expectation_group(const expectation& e1, const expectation& e2) : count(2) {
            depend_on[0] = e1.clone();
            depend_on[1] = e2.clone();
        }
        expectation_group(const expectation_group& other) : count(other.count) {
            for (int i = 0; i<count; ++i) {
                depend_on[i] = other.depend_on[i]->clone();
            }
        }
        virtual ~expectation_group() OVERRIDE {
            for (int i = 0; i<count; ++i) {
                delete depend_on[i];
            }
        }
        MAKE_CLONABLE(expectation_group);

        expectation_group& add(const expectation& e) { depend_on[count++] = e.clone(); arb_assert(count <= MAX_GROUP_SIZE); return *this; }

        const char *name() const OVERRIDE {
            return "<expectation_group>";
        }
        const char *readable_value() const OVERRIDE {
            return "<value of expectation_group>";
        }

        const expectation& dependent(int i) const { arb_assert(i<count); return *depend_on[i]; }
        int size() const { return count; }
        int count_fulfilled() const {
            int ff = 0;
            for (int i = 0; i<count; ++i) {
                ff += dependent(i).fulfilled();
            }
            return ff;
        }
        void dump_some_expectations(int indent, bool fulfilled, bool unfulfilled) const {
            if (fulfilled||unfulfilled) {
                bool all    = fulfilled && unfulfilled;
                bool wanted = fulfilled;

                bool printed = false;
                for (int i = 0; i<size(); ++i) {
                    const expectation& e = dependent(i);

                    bool is_fulfilled = e.fulfilled();
                    if (all || is_fulfilled == wanted) {
                        if (printed) print('\n');
                        e.explain(indent);
                        printed = true;
                    }
                }
            }
        }
        void dump_brief_description() const {
            print("of(");
            bool printed = false;
            for (int i = 0; i<size(); ++i) {
                if (printed) print(", ");
                const expectation& e = dependent(i);
                e.dump_brief_description();
                printed = true;
            }
            print(')');
        }
    };

    struct group_match //! result of matching an expectation_group with a group_matcher
    {
        const int count;
        const int fulfilled;
        const int min_req;
        const int max_req;
        const int diff;

        int required(int what) const { return what == -1 ? count : what; }
        group_match(const expectation_group& group, int min, int max)
            : count(group.size()),
              fulfilled(group.count_fulfilled()),
              min_req(required(min)),
              max_req(required(max)),
              diff(fulfilled<min_req
                   ? fulfilled-min_req
                   : (fulfilled>max_req ? fulfilled-max_req : 0))
        {}


        inline static void is(int a) { select_spaced(a == 1, "is", "are"); }
        inline static void was(int a) { select_spaced(a < 2, "was", "were"); }
        inline static void amountzero(int a, const char *zero) { a ? print(a) : print(zero); }

        void dump_num_of(int amount, const char *thing) const {
            amountzero(amount, "no");
            space();
            print(thing);
            if (amount != 1) print('s');
        }

        void dump(const expectation_group& group, int indent) const {
            print_indent(indent);
            if (count == 1) {
                print("expectation ");
                print("'");
                group.dependent(0).dump_brief_description();
                print("' ");
                print(fulfilled ? "fulfilled" : "fails");
                print(diff ? " unexpectedly" : " as expected");
            }
            else {
                print("expected ");
                int that_many;
                if (min_req == max_req) {
                    if (diff>0 && min_req>0) print("only ");
                    that_many = min_req;
                }
                else {
                    if (diff) {
                        print("at"); select_spaced(diff<0, "least", "most");
                        that_many = diff<0 ? min_req : max_req;
                    }
                    else {
                        fprintf(stderr, "%i-", min_req);
                        that_many = max_req;
                    }
                }
                dump_num_of(that_many, "fulfilled expectation");
                space();
                group.dump_brief_description();
                nl();

                indent += 2;
                print_indent(indent);
                if (diff == 0) print("and "); else print("but ");

                if (diff<0 && fulfilled>0) print("only ");
                amountzero(fulfilled, "none"); is(fulfilled); print("fulfilled");
            }

            print(", because\n");
            bool show_fulfilled   = diff >= 0;
            bool show_unfulfilled = diff <= 0;
            group.dump_some_expectations(indent+2, show_fulfilled, show_unfulfilled);
        }
    };

    class group_matcher : public matcher //! matches expectation_group for degree of fulfilledness
    {
        int min, max;
        group_matcher(int min_, int max_) : min(min_), max(max_) {}
    public:
        MAKE_CLONABLE(group_matcher);

        bool matches(const matchable& thing) const OVERRIDE {
            return group_match(dynamic_cast<const expectation_group&>(thing), min, max).diff == 0;
        }

        void dump_expectation(const matchable& thing, int indent) const OVERRIDE {
            const expectation_group& group = dynamic_cast<const expectation_group&>(thing);
            group_match matching(group, min, max);
            matching.dump(group, indent);
        }

        // factories
        static group_matcher all() { return group_matcher(-1, -1); }
        static group_matcher none() { return group_matcher(0, 0); }
        static group_matcher atleast(int min_) { return group_matcher(min_, -1); }
        static group_matcher atmost(int max_) { return group_matcher(0, max_); }
        static group_matcher exactly(int amount) { return group_matcher(amount, amount); }

        // match_expectation factories
        match_expectation of(const expectation& e) const {
            return match_expectation(expectation_group(e), *this);
        }
        match_expectation of(const expectation& e1, const expectation& e2) const {
            return match_expectation(expectation_group(e1, e2), *this);
        }
        match_expectation of(const expectation& e1, const expectation& e2, const expectation& e3) const {
            return match_expectation(expectation_group(e1, e2).add(e3), *this);
        }
        match_expectation of(const expectation& e1, const expectation& e2, const expectation& e3, const expectation& e4) const {
            return match_expectation(expectation_group(e1, e2).add(e3).add(e4), *this);
        }

        match_expectation ofgroup(const expectation_group& group) {
            return match_expectation(group, *this);
        }

        void dump_brief_description(const matchable& thing) const OVERRIDE {
            if (max == -1) {
                if (min == -1) {
                    print("all");
                }
                else {
                    fprintf(stderr, "atleast(%i)", min);
                }
            }
            else if (max == 0) {
                print("none");
            }
            else if (min == max) {
                fprintf(stderr, "exactly(%i)", min);
            }
            else {
                fprintf(stderr, "[%i-%i]", min, max);
            }

            print('.');

            const expectation_group& group = dynamic_cast<const expectation_group&>(thing);
            group.dump_brief_description();
        }
    };

    // --------------------------
    //      helper functions

    
    template <typename T> const matchable_value<T> CREATE_matchable(const copy<T>& val, const char *code) { return matchable_value<T>(val, code); }

    inline group_matcher all() { return group_matcher::all(); }
    inline group_matcher none() { return group_matcher::none(); }
    inline group_matcher atleast(int min) { return group_matcher::atleast(min); }
    inline group_matcher atmost(int max) { return group_matcher::atmost(max); }
    inline group_matcher exactly(int amount) { return group_matcher::exactly(amount); }

    inline match_expectation wrong(const expectation& e) { return none().of(e); }
};

// --------------------------------------------------------------------------------

#define MATCHABLE_ARGS_UNTYPED(val) val, #val
#define MATCHABLE_ARGS_TYPED(val)   make_copy(val), #val

#define is_equal_to(val)      equals_expectation(true, MATCHABLE_ARGS_UNTYPED(val))
#define does_differ_from(val) equals_expectation(false, MATCHABLE_ARGS_UNTYPED(val))

#define is_equal_to_NULL()      null_expectation(true)
#define does_differ_from_NULL() null_expectation(false)

#define is_less_than(val) lessThan_expectation(true, MATCHABLE_ARGS_UNTYPED(val))
#define is_more_than(val) moreThan_expectation(true, MATCHABLE_ARGS_UNTYPED(val))

#define is_less_or_equal(val) moreThan_expectation(false, MATCHABLE_ARGS_UNTYPED(val))
#define is_more_or_equal(val) lessThan_expectation(false, MATCHABLE_ARGS_UNTYPED(val))

#define fulfills(pred,arg)    predicate_expectation(true, MATCHABLE_ARGS_UNTYPED(pred), MATCHABLE_ARGS_UNTYPED(arg))
#define contradicts(pred,arg) predicate_expectation(false, MATCHABLE_ARGS_UNTYPED(pred), MATCHABLE_ARGS_UNTYPED(arg))

#define does_contain(val)   fulfills(containing(),val)
#define doesnt_contain(val) contradicts(containing(),val)

#define that(thing) CREATE_matchable(MATCHABLE_ARGS_TYPED(thing))

#define TEST_EXPECTATION(EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_that(); } while(0)
#define TEST_EXPECTATION__BROKEN(EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_broken(); } while(0)
#define TEST_EXPECTATION__WANTED(EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_wanted_behavior(); } while(0)

#define TEST_EXPECTATION__BROKENIF(COND,EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_brokenif(COND,#COND); } while(0)

#define DEBUG_TEST_EXPECTATION(EXPCTN) do {                                             \
        using namespace arb_test;                                                       \
        debug_asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).                            \
            debug_expectations();                                                       \
        debug_asserter(wrong(EXPCTN), "wrong(" #EXPCTN ")", __FILE__, __LINE__).        \
            debug_expectations();                                                       \
    } while(0)

// --------------------------------------------------------------------------------

#define HERE arb_test::locinfo(__FILE__, __LINE__)

#define TEST_WARNING(format,strarg)           HERE.warningf(format, (strarg))
#define TEST_WARNING2(format,strarg1,strarg2) HERE.warningf(format, (strarg1), (strarg2))
#define TEST_ERROR(format,strarg)             HERE.errorf(true, format, (strarg))
#define TEST_ERROR2(format,strarg1,strarg2)   HERE.errorf(true, format, (strarg1), (strarg2))
#define TEST_IOERROR(format,strarg)           HERE.ioerrorf(true, format, (strarg))

// --------------------------------------------------------------------------------

#define TEST_FAILS_INSIDE_VALGRIND(THETEST) do {                \
        if (!GBK_running_on_valgrind()) {                       \
            THETEST;                                            \
            TEST_WARNING("valgrind fails for '%s'", #THETEST);  \
        }                                                       \
    } while(0)

// --------------------------------------------------------------------------------

#define TEST_EXPECT_ZERO(cond)             TEST_EXPECT_EQUAL(cond, 0)
#define TEST_EXPECT_ZERO__BROKEN(cond,got) TEST_EXPECT_EQUAL__BROKEN(cond, 0, got)
#define TEST_REJECT_ZERO(cond)             TEST_EXPECTATION(that(cond).does_differ_from(0))
#define TEST_REJECT_ZERO__BROKEN(cond)     TEST_EXPECTATION__BROKEN(that(cond).does_differ_from(0))

#define TEST_EXPECT_ZERO_OR_SHOW_ERRNO(iocond)                  \
    do {                                                        \
        if ((iocond))                                           \
            TEST_IOERROR("I/O-failure in '%s'", #iocond);       \
    } while(0)

// --------------------------------------------------------------------------------

#define MISSING_TEST(description)                       \
    TEST_WARNING("Missing test '%s'", #description)

// --------------------------------------------------------------------------------

namespace arb_test {
    inline match_expectation reports_error(const char *error) { return that(error).does_differ_from_NULL(); }
    inline match_expectation doesnt_report_error(const char *error) { return that(error).is_equal_to_NULL(); }
    inline match_expectation reported_error_contains(const char *error, const char *part) { return error ? that(error).does_contain(part) : that(error).does_differ_from_NULL(); }
};

#define TEST_EXPECT_ERROR_CONTAINS(call,part)         TEST_EXPECTATION        (reported_error_contains(call, part))
#define TEST_EXPECT_ERROR_CONTAINS__BROKEN(call,part) TEST_EXPECTATION__BROKEN(reported_error_contains(call, part))
#define TEST_EXPECT_ANY_ERROR(call)                   TEST_EXPECTATION        (reports_error(call))
#define TEST_EXPECT_ANY_ERROR__BROKEN(call)           TEST_EXPECTATION__BROKEN(reports_error(call))
#define TEST_EXPECT_NO_ERROR(call)                    TEST_EXPECTATION        (doesnt_report_error(call))
#define TEST_EXPECT_NO_ERROR__BROKEN(call)            TEST_EXPECTATION__BROKEN(doesnt_report_error(call))

// --------------------------------------------------------------------------------

#ifdef ARB_MSG_H

namespace arb_test {
    inline GB_ERROR get_exported_error() { return GB_have_error() ? GB_await_error() : NULL; }
    inline match_expectation no_forgotten_error_exported() { return that(get_exported_error()).is_equal_to_NULL(); }

    class calling {
        bool     result;
        GB_ERROR error;
    public:
        calling(bool call)
            : result(call),
              error(get_exported_error())
        {}

        // functions below try to make failing expectations more readable
        match_expectation returns_result() const { return that(result).is_equal_to(true); }
        match_expectation doesnt_return_result() const { return that(result).is_equal_to(false); }
        match_expectation exports_error() const { return that(error).does_differ_from_NULL(); }
        match_expectation doesnt_export_error() const { return that(error).is_equal_to_NULL(); }
        match_expectation exports_error_containing(const char *expected_part) const {
            return error ? that(error).does_contain(expected_part) : exports_error();
        }

        match_expectation either_result_or_error() const { return exactly(1).of(returns_result(), exports_error()); }

        match_expectation does_neither_return_result_nor_export_error() const {
            return all().of(doesnt_return_result(),
                            doesnt_export_error());
        }
        match_expectation returns_result_and_doesnt_export_error() const {
            return all().of(either_result_or_error(),
                            returns_result(),
                            doesnt_export_error());
        }
        match_expectation doesnt_return_result_but_exports_error_containing(const char *expected_part) const {
            return all().of(either_result_or_error(),
                            doesnt_return_result(), 
                            exports_error_containing(expected_part));
        }
    };
};

#define TEST_EXPECT_ERROR_CLEAR() TEST_EXPECTATION(no_forgotten_error_exported())

#define TEST_EXPECT_RESULT__NOERROREXPORTED(create_result)                                do { TEST_EXPECT_ERROR_CLEAR(); TEST_EXPECTATION        (calling((create_result)).returns_result_and_doesnt_export_error()); } while(0)
#define TEST_EXPECT_RESULT__NOERROREXPORTED__BROKEN(create_result)                        do { TEST_EXPECT_ERROR_CLEAR(); TEST_EXPECTATION__BROKEN(calling((create_result)).returns_result_and_doesnt_export_error()); } while(0)
#define TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(create_result,expected_part)         do { TEST_EXPECT_ERROR_CLEAR(); TEST_EXPECTATION        (calling((create_result)).doesnt_return_result_but_exports_error_containing(expected_part)); } while(0)
#define TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS__BROKEN(create_result,expected_part) do { TEST_EXPECT_ERROR_CLEAR(); TEST_EXPECTATION__BROKEN(calling((create_result)).doesnt_return_result_but_exports_error_containing(expected_part)); } while(0)
#define TEST_EXPECT_NORESULT__NOERROREXPORTED(create_result)                              do { TEST_EXPECT_ERROR_CLEAR(); TEST_EXPECTATION        (calling((create_result)).does_neither_return_result_nor_export_error()); } while(0)
#define TEST_EXPECT_NORESULT__NOERROREXPORTED__BROKEN(create_result)                      do { TEST_EXPECT_ERROR_CLEAR(); TEST_EXPECTATION__BROKEN(calling((create_result)).does_neither_return_result_nor_export_error()); } while(0)

#endif
// --------------------------------------------------------------------------------
// TEST_EXPECT_SEGFAULT and TEST_EXPECT_CODE_ASSERTION_FAILS
// only work if binary is linked with ARBDB

#ifdef ENABLE_CRASH_TESTS
# ifdef ARB_CORE_H

const bool DOES_SEGFAULT       = true;
const bool DOESNT_SEGFAULT     = false;
const bool FAILS_ASSERTION     = true;
const bool FULFILLS_ASSERTIONS = false;

#  ifdef ASSERTION_USED
inline arb_test::match_expectation expect_callback(void (*cb)(), bool expect_SEGV, bool expect_assert_fail, bool expectation_untestable_under_valgrind) {
    using namespace arb_test;

    bool& assertion_failed = test_data().assertion_failed;
    bool  old_state        = assertion_failed;

    expectation_group expected;
    if (GBK_running_on_valgrind()) {
        // GBK_raises_SIGSEGV(cb); // just call
        expected.add(that(expectation_untestable_under_valgrind).is_equal_to(true));
    }
    else {
        expected.add(that(GBK_raises_SIGSEGV(cb)).is_equal_to(expect_SEGV));
        expected.add(that(assertion_failed).is_equal_to(expect_assert_fail));
    }

    assertion_failed = old_state;
    return all().ofgroup(expected);
}
#  else
inline arb_test::match_expectation expect_callback(void (*cb)(), bool expect_SEGV, bool expectation_untestable_under_valgrind) {
    using namespace arb_test;
    if (GBK_running_on_valgrind()) {
        return that(expectation_untestable_under_valgrind).is_equal_to(true);
    }
    return that(GBK_raises_SIGSEGV(cb)).is_equal_to(expect_SEGV);
}
#  endif
# endif

# ifdef ASSERTION_USED

#  define TEST_EXPECT_CODE_ASSERTION_FAILS(cb)           TEST_EXPECTATION(expect_callback(cb, DOES_SEGFAULT, FAILS_ASSERTION, true))
#  define TEST_EXPECT_CODE_ASSERTION_FAILS__WANTED(cb)   TEST_EXPECTATION__WANTED(expect_callback(cb, DOES_SEGFAULT, FAILS_ASSERTION, false))
#  define TEST_EXPECT_CODE_ASSERTION_FAILS__UNWANTED(cb) TEST_EXPECTATION__WANTED(expect_callback(cb, DOESNT_SEGFAULT, FULFILLS_ASSERTIONS, false))
#  define TEST_EXPECT_SEGFAULT(cb)                       TEST_EXPECTATION(expect_callback(cb, DOES_SEGFAULT, FULFILLS_ASSERTIONS, true)) 
#  define TEST_EXPECT_SEGFAULT__WANTED(cb)               TEST_EXPECTATION__WANTED(expect_callback(cb, DOES_SEGFAULT, FULFILLS_ASSERTIONS, false)) 
#  define TEST_EXPECT_SEGFAULT__UNWANTED(cb)             TEST_EXPECTATION__WANTED(expect_callback(cb, DOESNT_SEGFAULT, FULFILLS_ASSERTIONS, false))

# else // ENABLE_CRASH_TESTS but no ASSERTION_USED (test segfaults in NDEBUG mode)

#  define TEST_EXPECT_CODE_ASSERTION_FAILS(cb)
#  define TEST_EXPECT_CODE_ASSERTION_FAILS__WANTED(cb)
#  define TEST_EXPECT_CODE_ASSERTION_FAILS__UNWANTED(cb)
#  define TEST_EXPECT_SEGFAULT(cb)                       TEST_EXPECTATION(expect_callback(cb, DOES_SEGFAULT, true)) 
#  define TEST_EXPECT_SEGFAULT__WANTED(cb)               TEST_EXPECTATION__WANTED(expect_callback(cb, DOES_SEGFAULT, false)) 
#  define TEST_EXPECT_SEGFAULT__UNWANTED(cb)             TEST_EXPECTATION__WANTED(expect_callback(cb, DOESNT_SEGFAULT, false))

# endif

#else // not ENABLE_CRASH_TESTS (i.e. skip these tests completely)

# define TEST_EXPECT_CODE_ASSERTION_FAILS(cb)
# define TEST_EXPECT_CODE_ASSERTION_FAILS__WANTED(cb)
# define TEST_EXPECT_CODE_ASSERTION_FAILS__UNWANTED(cb)
# define TEST_EXPECT_SEGFAULT(cb)
# define TEST_EXPECT_SEGFAULT__WANTED(cb)
# define TEST_EXPECT_SEGFAULT__UNWANTED(cb)

#endif

// --------------------------------------------------------------------------------

namespace arb_test {
    template <typename T>
    inline void expect_broken(const arb_test::matchable_value<T>& That, const T& want, const T& got) {
        TEST_EXPECTATION__BROKEN(That.is_equal_to(want));
        TEST_EXPECTATION(That.is_equal_to(got));
    }
};

#define TEST_EXPECT_EQUAL(expr,want)             TEST_EXPECTATION(that(expr).is_equal_to(want))
#define TEST_EXPECT_EQUAL__BROKEN(expr,want,got) do { TEST_EXPECTATION__BROKEN(that(expr).is_equal_to(want)); TEST_EXPECTATION(that(expr).is_equal_to(got)); } while(0)
#define TEST_EXPECT_EQUAL__IGNARG(expr,want,ign) TEST_EXPECTATION(that(expr).is_equal_to(want))

#define TEST_EXPECT_SIMILAR(expr,want,epsilon)         TEST_EXPECTATION(that(expr).fulfills(epsilon_similar(epsilon), want))
#define TEST_EXPECT_SIMILAR__BROKEN(expr,want,epsilon) TEST_EXPECTATION__BROKEN(that(expr).fulfills(epsilon_similar(epsilon), want))

#define TEST_EXPECT_DIFFERENT(expr,want)         TEST_EXPECTATION(that(expr).does_differ_from(want));
#define TEST_EXPECT_DIFFERENT__BROKEN(expr,want) TEST_EXPECTATION__BROKEN(that(expr).does_differ_from(want));

#define TEST_EXPECT_LESS(val,ref)              TEST_EXPECTATION(that(val).is_less_than(ref))
#define TEST_EXPECT_MORE(val,ref)              TEST_EXPECTATION(that(val).is_more_than(ref))
#define TEST_EXPECT_LESS_EQUAL(val,ref)        TEST_EXPECTATION(that(val).is_less_or_equal(ref))
#define TEST_EXPECT_MORE_EQUAL(val,ref)        TEST_EXPECTATION(that(val).is_more_or_equal(ref))
#define TEST_EXPECT_IN_RANGE(val,lower,higher) TEST_EXPECTATION(all().of(that(val).is_more_or_equal(lower),     \
                                                                         that(val).is_less_or_equal(higher)))

#define TEST_EXPECT_CONTAINS(str,part)         TEST_EXPECTATION(that(str).does_contain(part))
#define TEST_EXPECT_CONTAINS__BROKEN(str,part) TEST_EXPECTATION__BROKEN(that(str).does_contain(part))

#define TEST_EXPECT_NULL(n)             TEST_EXPECT_EQUAL(n, (typeof(n))NULL)
#define TEST_EXPECT_NULL__BROKEN(n,got) TEST_EXPECT_EQUAL__BROKEN(n, (typeof(n))NULL, got)
#define TEST_REJECT_NULL(n)             TEST_EXPECT_DIFFERENT(n, (typeof(n))NULL)
#define TEST_REJECT_NULL__BROKEN(n)     TEST_EXPECT_DIFFERENT__BROKEN(n, (typeof(n))NULL)

#define TEST_EXPECT(cond)         TEST_EXPECTATION(that(cond).is_equal_to(true))
#define TEST_EXPECT__BROKEN(cond) TEST_EXPECTATION__BROKEN(that(cond).is_equal_to(true))
#define TEST_REJECT(cond)         TEST_EXPECTATION(that(cond).is_equal_to(false))
#define TEST_REJECT__BROKEN(cond) TEST_EXPECTATION__BROKEN(that(cond).is_equal_to(false))

// --------------------------------------------------------------------------------
// the following macros only work when
// - tested module depends on ARBDB and
// - some ARBDB header has been included
// Otherwise the section is skipped completely.
//
// @@@ ARBDB->ARBCORE later

#ifdef ARB_DIFF_H

namespace arb_test {
    inline bool memory_is_equal(const void *mem1, const void *mem2, size_t size) {
        FlushedOutputNoLF yes;
        return ARB_test_mem_equal(reinterpret_cast<const unsigned char *>(mem1),
                                  reinterpret_cast<const unsigned char *>(mem2), size, 0) == size;
    }
    inline bool files_are_equal(const char *file1, const char *file2) {
        FlushedOutputNoLF yes;
        return ARB_files_are_equal(file1, file2);
    }
    inline bool textfiles_have_difflines(const char *file1, const char *file2, int expected_difflines) {
        FlushedOutputNoLF yes;
        return ARB_textfiles_have_difflines(file1, file2, expected_difflines, 0);
    }
    inline bool textfiles_have_difflines_ignoreDates(const char *file1, const char *file2, int expected_difflines) {
        FlushedOutputNoLF yes;
        return ARB_textfiles_have_difflines(file1, file2, expected_difflines, 1);
    }
};

#define TEST_COPY_FILE(src, dst) TEST_EXPECT_ZERO(system(GBS_global_string("cp '%s' '%s'", src, dst)))
#define TEST_DUMP_FILE(src, dst) TEST_EXPECT(system(GBS_global_string("hexdump -C '%s' > '%s'", src, dst)) == 0)

#define TEST_EXPECT_TEXTFILE_DIFFLINES(f1,f2,diff)         TEST_EXPECT(arb_test::textfiles_have_difflines(f1,f2, diff))
#define TEST_EXPECT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,diff) TEST_EXPECT__BROKEN(arb_test::textfiles_have_difflines(f1,f2, diff))

#define TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(f1,f2,diff)         TEST_EXPECT(arb_test::textfiles_have_difflines_ignoreDates(f1,f2, diff))
#define TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES__BROKEN(f1,f2,diff) TEST_EXPECT__BROKEN(arb_test::textfiles_have_difflines_ignoreDates(f1,f2, diff))

#define TEST_EXPECT_FILES_EQUAL(f1,f2)         TEST_EXPECT(arb_test::files_are_equal(f1,f2))
#define TEST_EXPECT_FILES_EQUAL__BROKEN(f1,f2) TEST_EXPECT__BROKEN(arb_test::files_are_equal(f1,f2))

#define TEST_EXPECT_TEXTFILES_EQUAL(f1,f2)         TEST_EXPECT_TEXTFILE_DIFFLINES(f1,f2,0)
#define TEST_EXPECT_TEXTFILES_EQUAL__BROKEN(f1,f2) TEST_EXPECT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,0)

#define TEST_EXPECT_MEM_EQUAL(m1,m2,size)         TEST_EXPECT(arb_test::memory_is_equal(m1,m2,size)) 
#define TEST_EXPECT_MEM_EQUAL__BROKEN(m1,m2,size) TEST_EXPECT__BROKEN(arb_test::memory_is_equal(m1,m2,size)) 

#else

#define WARN_MISS_ARBDIFF() need_include__arb_diff_h__BEFORE__test_unit_h

#define TEST_EXPECT_TEXTFILE_DIFFLINES(f1,f2,diff)                      WARN_MISS_ARBDIFF()
#define TEST_EXPECT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,diff)              WARN_MISS_ARBDIFF()
#define TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(f1,f2,diff)         WARN_MISS_ARBDIFF()
#define TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES__BROKEN(f1,f2,diff) WARN_MISS_ARBDIFF()
#define TEST_EXPECT_FILES_EQUAL(f1,f2)                                  WARN_MISS_ARBDIFF()
#define TEST_EXPECT_FILES_EQUAL__BROKEN(f1,f2)                          WARN_MISS_ARBDIFF()
#define TEST_EXPECT_TEXTFILES_EQUAL(f1,f2)                              WARN_MISS_ARBDIFF()
#define TEST_EXPECT_TEXTFILES_EQUAL__BROKEN(f1,f2)                      WARN_MISS_ARBDIFF()
#define TEST_EXPECT_MEM_EQUAL(m1,m2,size)                               WARN_MISS_ARBDIFF()
#define TEST_EXPECT_MEM_EQUAL__BROKEN(m1,m2,size)                       WARN_MISS_ARBDIFF()

#define memory_is_equal(m1,m2,size)                    WARN_MISS_ARBDIFF()
#define files_are_equal(f1,f2)                         WARN_MISS_ARBDIFF()
#define textfiles_have_difflines(f1,f2,ed)             WARN_MISS_ARBDIFF()
#define textfiles_have_difflines_ignoreDates(f1,f2,ed) WARN_MISS_ARBDIFF()

#endif // ARB_DIFF_H

// --------------------------------------------------------------------------------

#ifdef ARBDBT_H

namespace arb_test {
    inline match_expectation expect_newick_equals(NewickFormat format, const GBT_TREE *tree, const char *expected_newick) {
        char              *newick   = GBT_tree_2_newick(tree, format);
        match_expectation  expected = that(newick).is_equal_to(expected_newick);
        free(newick);
        return expected;
    }
    inline match_expectation saved_newick_equals(NewickFormat format, GBDATA *gb_main, const char *treename, const char *expected_newick) {
        expectation_group  expected;
        GB_transaction     ta(gb_main);
        GBT_TREE          *tree = GBT_read_tree(gb_main, treename, GBT_TREE_NodeFactory());

        expected.add(that(tree).does_differ_from_NULL());
        if (tree) {
            expected.add(expect_newick_equals(format, tree, expected_newick));
            delete tree;
        }
        return all().ofgroup(expected);
    }
};

#define TEST_EXPECT_NEWICK(format,tree,expected_newick)         TEST_EXPECTATION(arb_test::expect_newick_equals(format, tree, expected_newick))
#define TEST_EXPECT_NEWICK__BROKEN(format,tree,expected_newick) TEST_EXPECTATION__BROKEN(arb_test::expect_newick_equals(format, tree, expected_newick))

#define TEST_EXPECT_SAVED_NEWICK(format,gb_main,treename,expected_newick)         TEST_EXPECTATION(arb_test::saved_newick_equals(format, gb_main, treename, expected_newick))
#define TEST_EXPECT_SAVED_NEWICK__BROKEN(format,gb_main,treename,expected_newick) TEST_EXPECTATION__BROKEN(arb_test::saved_newick_equals(format, gb_main, treename, expected_newick))

#else

#define WARN_MISS_ARBDBT() need_include__arbdbt_h__BEFORE__test_unit_h

#define TEST_EXPECT_NEWICK(format,tree,expected_newick)         WARN_MISS_ARBDBT()
#define TEST_EXPECT_NEWICK__BROKEN(format,tree,expected_newick) WARN_MISS_ARBDBT()

#define TEST_EXPECT_SAVED_NEWICK(format,gb_main,treename,expected_newick)         WARN_MISS_ARBDBT()
#define TEST_EXPECT_SAVED_NEWICK__BROKEN(format,gb_main,treename,expected_newick) WARN_MISS_ARBDBT()

#endif

// --------------------------------------------------------------------------------

#define TEST_SETUP_GLOBAL_ENVIRONMENT(modulename) do {                                                          \
        arb_test::test_data().raiseLocalFlag(ANY_SETUP);                                                        \
        TEST_EXPECT_NO_ERROR(GBK_system(GBS_global_string("../test_environment setup %s",  (modulename))));     \
    } while(0)
// cleanup is done (by Makefile.suite) after all unit tests have been run

// --------------------------------------------------------------------------------

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
