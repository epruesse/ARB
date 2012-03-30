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
 * Recommended test-assertion is TEST_EXPECT(that(..).xxx())
 * see examples in test-unit-tests in ../CORE/arb_string.cxx@UNIT_TESTS
 */

#define TEST_ASSERT(cond) test_assert(cond, false)

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

        __attribute__((format(printf, 2, 3))) void warningf(const char *format, ...) const {
            GlobalTestData& global = test_data();
            if (global.show_warnings) {
                FlushedOutput yes;
                VCOMPILERMSG("Warning", format);
                GlobalTestData::print_annotation();
                global.warnings++;
            }
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

    inline char *val2readable(unsigned char c) { return c<32 ? StaticCode::strf(" ^%c", c+'A'-1) : StaticCode::strf("'%c'", c); }
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


    template <typename T> inline void print(const T& t) { fputs(val2readable(make_copy(t)), stderr); }
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
        epsilon_similar(double epsilon_) : epsilon(epsilon_) {}
        bool operator()(const double& d1, const double& d2) const {
            double diff = d1-d2;
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

    inline bool files_are_equal(const char *file1, const char *file2) {
        const char    *error = NULL;
        FILE          *fp1   = fopen(file1, "rb");
        FlushedOutput  yes;

        if (!fp1) {
            StaticCode::printf("can't open '%s'", file1);
            error = "i/o error";
        }
        else {
            FILE *fp2 = fopen(file2, "rb");
            if (!fp2) {
                StaticCode::printf("can't open '%s'", file2);
                error = "i/o error";
            }
            else {
                const int      BLOCKSIZE    = 4096;
                unsigned char *buf1         = (unsigned char*)malloc(BLOCKSIZE);
                unsigned char *buf2         = (unsigned char*)malloc(BLOCKSIZE);
                int            equal_bytes  = 0;

                while (!error) {
                    int read1  = fread(buf1, 1, BLOCKSIZE, fp1);
                    int read2  = fread(buf2, 1, BLOCKSIZE, fp2);
                    int common = read1<read2 ? read1 : read2;

                    if (!common) {
                        if (read1 != read2) error = "filesize differs";
                        break;
                    }

                    if (memcmp(buf1, buf2, common) == 0) {
                        equal_bytes += common;
                    }
                    else {
                        int x = 0;
                        while (buf1[x] == buf2[x]) {
                            x++;
                            equal_bytes++;
                        }
                        error = "content differs";

                        // x is the position inside the current block
                        const int DUMP       = 7;
                        int       y1         = x >= DUMP ? x-DUMP : 0;
                        int       y2         = (x+DUMP)>common ? common : (x+DUMP);
                        int       blockstart = equal_bytes-x;

                        for (int y = y1; y <= y2; y++) {
                            fprintf(stderr, "[0x%04x]", blockstart+y);
                            print_pair(buf1[y], buf2[y]);
                            fputc(' ', stderr);
                            print_hex_pair(buf1[y], buf2[y]);
                            if (x == y) fputs("                     <- diff", stderr);
                            fputc('\n', stderr);
                        }
                        if (y2 == common) {
                            fputs("[end of block - truncated]\n", stderr);
                        }
                    }
                }

                if (error) StaticCode::printf("files_are_equal: equal_bytes=%i\n", equal_bytes);
                test_assert(error || equal_bytes, true); // comparing empty files is nonsense

                free(buf2);
                free(buf1);
                fclose(fp2);
            }
            fclose(fp1);
        }

        if (error) StaticCode::printf("files_are_equal(%s, %s) fails: %s\n", file1, file2, error);
        return !error;
    }


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
#define MAKE_CLONABLE(type)    type *clone() const { return new type(*this); }

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
        void warn(const char *format) const { loc.warningf(format, code); }
        
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
                warn("Expectation '%s' known as broken (accepted until fixed)");
                print("Broken because\n");
                expected->explain(2); print('\n');
            }
        }
        void expect_wanted_behavior() const {
            if (expected->fulfilled()) {
                err("Wanted behavior '%s' reached");
                announce_failure();
            }
            else {
                warn("Wanted behavior: '%s'");
                print("Unsatisfied because\n");
                expected->explain(2); print('\n');
            }
        }

    };

    class debug_asserter : public asserter {
        virtual void announce_failure() const {
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
        ~match_expectation() {
            delete thing;
            delete condition;
        }

        bool fulfilled() const { return condition->matches(*thing); }
        void explain(int indent) const { condition->dump_expectation(*thing, indent); }
        void dump_brief_description() const { condition->dump_brief_description(*thing); }
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
        predicate_description(const predicate_description& other) : primary(other.primary), inverse(other.inverse) {}
        DECLARE_ASSIGNMENT_OPERATOR(predicate_description);

        const char *make(bool expected, bool got) const {
            if (expected) return make(primary, got);
            if (inverse) return make(inverse, !got);
            return make(primary, !got);
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

        const char *name() const { return code; }
        const char *readable_value() const {
            if (!readable.exists()) readable = arb_test::readable(val);
            return readable.value();
        }

        template <typename U> inline match_expectation equals_expectation(bool invert, const U& other, const char *code) const;
        template <typename U> inline match_expectation lessThan_expectation(bool invert, const U& other, const char *code) const;
        template <typename U> inline match_expectation moreThan_expectation(bool invert, const U& other, const char *code) const;
        
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
    class value_matcher : public matcher //! matcher for values
    {
        matchable_value<T> expected;
    public:
        value_matcher(const matchable_value<T>& expected_) : expected(expected_) {}
        virtual ~value_matcher() {}

        virtual bool matches(const copy<T>& v1, const copy<T>& v2) const       = 0;
        virtual const char *relation(bool isMatch) const                       = 0;

        const matchable_value<T>& get_expected() const { return expected; }

        bool matches(const matchable& thing) const {
            const matchable_value<T>& value_thing = dynamic_cast<const matchable_value<T>&>(thing);
            return matches(value_thing.value(), expected.value());
        }

        void dump_expectation(const matchable& thing, int indent) const {
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
        void dump_brief_description(const matchable& thing) const {
            print(thing.name());
            print('.');
            print(relation(true));
            print('('); print(expected.name()); print(')');
        }

    };

    // ---------------------------
    //      predicate_matcher


    template <typename T, typename FUNC>
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

        bool matches(const copy<T>& v1, const copy<T>& v2) const { return correlated(pred.matches(v1, v2), expected_result); }
        const char *relation(bool isMatch) const { return pred.describe(expected_result, correlated(isMatch, expected_result)); }
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

    const int MAX_GROUP_SIZE = 3;
    class expectation_group : public matchable //! group of expectation. matchable with group_matcher
    {
        int          count;
        expectation *depend_on[MAX_GROUP_SIZE];

        expectation_group& operator = (const expectation_group&); // forbidden
    protected:

    public:
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
        virtual ~expectation_group() {
            for (int i = 0; i<count; ++i) {
                delete depend_on[i];
            }
        }
        MAKE_CLONABLE(expectation_group);

        expectation_group& add(const expectation& e) { depend_on[count++] = e.clone(); return *this; }

        const char *name() const {
            return "<expectation_group>";
        }
        const char *readable_value() const {
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

        bool matches(const matchable& thing) const {
            return group_match(dynamic_cast<const expectation_group&>(thing), min, max).diff == 0;
        }

        void dump_expectation(const matchable& thing, int indent) const {
            const expectation_group& group = dynamic_cast<const expectation_group&>(thing);
            group_match matching(group, min, max);
            matching.dump(group, indent);
        }

        // factories
        static group_matcher all() { return group_matcher(-1, -1); }
        static group_matcher none() { return group_matcher(0, 0); }
        static group_matcher atleast(int min_) { return group_matcher(min_, -1); }
        static group_matcher atmost(int max_) { return group_matcher(0, max_); }
        static group_matcher exacly(int amount) { return group_matcher(amount, amount); }

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

        void dump_brief_description(const matchable& thing) const {
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
    inline group_matcher exacly(int amount) { return group_matcher::exacly(amount); }

    inline match_expectation wrong(const expectation& e) { return none().of(e); }
};

// --------------------------------------------------------------------------------

#define MATCHABLE_ARGS_UNTYPED(val) val, #val
#define MATCHABLE_ARGS_TYPED(val)   make_copy(val), #val

#define equals(val)  equals_expectation(true, MATCHABLE_ARGS_UNTYPED(val))
#define differs(val) equals_expectation(false, MATCHABLE_ARGS_UNTYPED(val))

#define less_than(val) lessThan_expectation(true, MATCHABLE_ARGS_UNTYPED(val))
#define more_than(val) moreThan_expectation(true, MATCHABLE_ARGS_UNTYPED(val))

#define less_or_equal(val) moreThan_expectation(false, MATCHABLE_ARGS_UNTYPED(val))
#define more_or_equal(val) lessThan_expectation(false, MATCHABLE_ARGS_UNTYPED(val))

#define is(pred,arg)     predicate_expectation(true, MATCHABLE_ARGS_UNTYPED(pred), MATCHABLE_ARGS_UNTYPED(arg))
#define is_not(pred,arg) predicate_expectation(false, MATCHABLE_ARGS_UNTYPED(pred), MATCHABLE_ARGS_UNTYPED(arg))

#define that(thing) CREATE_matchable(MATCHABLE_ARGS_TYPED(thing))

#define TEST_EXPECT(EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_that(); } while(0)
#define TEST_EXPECT__BROKEN(EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_broken(); } while(0)
#define TEST_EXPECT__WANTED(EXPCTN) do { using namespace arb_test; asserter(EXPCTN, #EXPCTN, __FILE__, __LINE__).expect_wanted_behavior(); } while(0)

#define DEBUG_TEST_EXPECT(EXPCTN) do {                                                  \
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

#define TEST_ASSERT__BROKEN(cond)                                       \
    do {                                                                \
        if (cond)                                                       \
            TEST_ERROR("Formerly broken test '%s' succeeds", #cond);    \
        else                                                            \
            TEST_WARNING("Known broken behavior ('%s' fails)", #cond);  \
    } while (0)

#define TEST_ASSERT_ZERO(cond)         TEST_EXPECT(that(cond).equals(0))
#define TEST_ASSERT_ZERO__BROKEN(cond) TEST_EXPECT__BROKEN(that(cond).equals(0))

#define TEST_ASSERT_ZERO_OR_SHOW_ERRNO(iocond)                  \
    do {                                                        \
        if ((iocond))                                           \
            TEST_IOERROR("I/O-failure in '%s'", #iocond);       \
    } while(0)

// --------------------------------------------------------------------------------

#define MISSING_TEST(description)                       \
    TEST_WARNING("Missing test '%s'", #description)

// --------------------------------------------------------------------------------

#define TEST_ASSERT_NO_ERROR(error_cond)                                \
    do {                                                                \
        const char *error_ = (error_cond);                              \
        if (error_) {                                                   \
            TEST_ERROR2("call '%s' returns unexpected error '%s'",      \
                        #error_cond, error_);                           \
        }                                                               \
    } while (0)

#define TEST_ASSERT_ERROR(error_cond)                                   \
    do {                                                                \
        if (!(error_cond)) {                                            \
            TEST_ERROR("Expected to see error from '%s'", #error_cond); \
        }                                                               \
    } while (0)


#define TEST_ASSERT_NO_ERROR__BROKEN(error_cond)                                \
    do {                                                                        \
        const char *error_ = (error_cond);                                      \
        if (error_) {                                                           \
            TEST_WARNING2("Known broken behavior ('%s' reports error '%s')",    \
                          #error_cond, error_);                                 \
        }                                                                       \
        else {                                                                  \
            TEST_ERROR("Formerly broken test '%s' succeeds (reports no error)", \
                       #error_cond);                                            \
        }                                                                       \
    } while (0)

#define TEST_ASSERT_ERROR__BROKEN(error_cond)                                           \
    do {                                                                                \
        const char *error_ = (error_cond);                                              \
        if (!error_) {                                                                  \
            TEST_WARNING("Known broken behavior ('%s' fails to report error)",          \
                         #error_cond);                                                  \
        }                                                                               \
        else {                                                                          \
            TEST_ERROR2("Former broken test '%s' succeeds (reports error '%s')",        \
                        #error_cond, error_);                                           \
        }                                                                               \
    } while (0)


// --------------------------------------------------------------------------------

#define TEST_EXPORTED_ERROR() (GB_have_error() ? GB_await_error() : NULL)

#define TEST_CLEAR_EXPORTED_ERROR()                                     \
    do {                                                                \
        const char *error_ = TEST_EXPORTED_ERROR();                     \
        if (error_) {                                                   \
            TEST_WARNING("detected and cleared exported error '%s'",    \
                         error_);                                       \
        }                                                               \
    } while (0)

#define TEST_ASSERT_NORESULT__ERROREXPORTED__CHECKERROR(create_result,equal,contains)    \
    do {                                                                                \
        TEST_CLEAR_EXPORTED_ERROR();                                                    \
        bool have_result = (create_result);                                             \
        const char *error_ = TEST_EXPORTED_ERROR();                                     \
        if (have_result) {                                                              \
            if (error_) {                                                               \
                TEST_WARNING("Error '%s' exported (when result returned)",              \
                             error_);                                                   \
            }                                                                           \
            TEST_ERROR("Expected '%s' to return NULL", #create_result);                 \
        }                                                                               \
        else if (!error_) {                                                             \
            TEST_ERROR("'%s' (w/o result) should always export error",                  \
                       #create_result);                                                 \
        }                                                                               \
        if (equal) TEST_ASSERT_EQUAL(error_, equal);                                    \
        if (contains) TEST_ASSERT_CONTAINS(error_, contains);                           \
    } while (0)


#define TEST_ASSERT_NORESULT__ERROREXPORTED(create_result)                   TEST_ASSERT_NORESULT__ERROREXPORTED__CHECKERROR(create_result,(void*)NULL,(void*)NULL)
#define TEST_ASSERT_NORESULT__ERROREXPORTED_EQUALS(create_result,expected)   TEST_ASSERT_NORESULT__ERROREXPORTED__CHECKERROR(create_result,expected,(void*)NULL)
#define TEST_ASSERT_NORESULT__ERROREXPORTED_CONTAINS(create_result,expected) TEST_ASSERT_NORESULT__ERROREXPORTED__CHECKERROR(create_result,(void*)NULL,expected)


#define TEST_ASSERT_RESULT__NOERROREXPORTED(create_result)                                      \
    do {                                                                                        \
        TEST_CLEAR_EXPORTED_ERROR();                                                            \
        bool have_result = (create_result);                                                     \
        const char *error_ = TEST_EXPORTED_ERROR();                                             \
        if (have_result) {                                                                      \
            if (error_) {                                                                       \
                TEST_ERROR("Error '%s' exported (when result returned)",                        \
                           error_);                                                             \
            }                                                                                   \
        }                                                                                       \
        else {                                                                                  \
            if (!error_) {                                                                      \
                TEST_WARNING("'%s' (w/o result) should always export error",                    \
                             #create_result);                                                   \
            }                                                                                   \
            else {                                                                              \
                TEST_WARNING("exported error is '%s'", error_);                                 \
            }                                                                                   \
            TEST_ERROR2("Expected '%s' to return sth (exported=%s)", #create_result, error_);   \
        }                                                                                       \
    } while (0)

// --------------------------------------------------------------------------------
// TEST_ASSERT_SEGFAULT and TEST_ASSERT_CODE_ASSERTION_FAILS
// only work if binary is linked with ARBDB

#ifdef ENABLE_CRASH_TESTS
# ifdef ARB_CORE_H

const bool DOES_SEGFAULT       = true;
const bool DOESNT_SEGFAULT     = false;
const bool FAILS_ASSERTION     = true;
const bool FULFILLS_ASSERTIONS = false;

#  ifdef ASSERTION_USED
inline arb_test::match_expectation expect_callback(void (*cb)(), bool expect_SEGV, bool expect_assert_fail) {
    using namespace arb_test;

    bool& assertion_failed = test_data().assertion_failed;
    bool  old_state        = assertion_failed;

    expect_assert_fail = expect_assert_fail && !GBK_running_on_valgrind(); // under valgrind assertions never fail

    match_expectation SEGV       = that(GBK_raises_SIGSEGV(cb,true)).equals(expect_SEGV);
    match_expectation assertfail = that(assertion_failed).equals(expect_assert_fail);

    match_expectation expected = all().of(SEGV, assertfail);

    assertion_failed = old_state;
    return expected;
}
#  else
inline arb_test::match_expectation expect_callback(void (*cb)(), bool expect_SEGV) {
    using namespace arb_test;
    return that(GBK_raises_SIGSEGV(cb,true)).equals(expect_SEGV);
}
#  endif
# endif

# ifdef ASSERTION_USED

#  define TEST_ASSERT_CODE_ASSERTION_FAILS(cb)           TEST_EXPECT(expect_callback(cb, DOES_SEGFAULT, FAILS_ASSERTION))
#  define TEST_ASSERT_CODE_ASSERTION_FAILS__UNWANTED(cb) TEST_EXPECT__WANTED(expect_callback(cb, DOESNT_SEGFAULT, FULFILLS_ASSERTIONS))
#  define TEST_ASSERT_SEGFAULT(cb)                       TEST_EXPECT(expect_callback(cb, DOES_SEGFAULT, FULFILLS_ASSERTIONS)) 
#  define TEST_ASSERT_SEGFAULT__UNWANTED(cb)             TEST_EXPECT__WANTED(expect_callback(cb, DOESNT_SEGFAULT, FULFILLS_ASSERTIONS))

# else // ENABLE_CRASH_TESTS but no ASSERTION_USED (test segfaults in NDEBUG mode)

#  define TEST_ASSERT_CODE_ASSERTION_FAILS(cb)
#  define TEST_ASSERT_CODE_ASSERTION_FAILS__UNWANTED(cb)
#  define TEST_ASSERT_SEGFAULT(cb)                       TEST_EXPECT(expect_callback(cb, DOES_SEGFAULT)) 
#  define TEST_ASSERT_SEGFAULT__UNWANTED(cb)             TEST_EXPECT__WANTED(expect_callback(cb, DOESNT_SEGFAULT))

# endif

#else // not ENABLE_CRASH_TESTS (i.e. skip these tests completely)

# define TEST_ASSERT_CODE_ASSERTION_FAILS(cb)
# define TEST_ASSERT_CODE_ASSERTION_FAILS__UNWANTED(cb)
# define TEST_ASSERT_SEGFAULT(cb)
# define TEST_ASSERT_SEGFAULT__UNWANTED(cb)

#endif

// --------------------------------------------------------------------------------

#define TEST_ASSERT_EQUAL(e1,t2)         TEST_EXPECT(that(e1).equals(t2))
#define TEST_ASSERT_EQUAL__BROKEN(e1,t2) TEST_EXPECT__BROKEN(that(e1).equals(t2))

#define TEST_ASSERT_SIMILAR(e1,t2,epsilon)         TEST_EXPECT(that(e1).is(epsilon_similar(epsilon), t2))
#define TEST_ASSERT_SIMILAR__BROKEN(e1,t2,epsilon) TEST_EXPECT__BROKEN(that(e1).is(epsilon_similar(epsilon), t2))

#define TEST_ASSERT_DIFFERENT(e1,t2)         TEST_EXPECT(that(e1).differs(t2));
#define TEST_ASSERT_DIFFERENT__BROKEN(e1,t2) TEST_EXPECT__BROKEN(that(e1).differs(t2));

#define TEST_ASSERT_LOWER_EQUAL(lower,upper)  TEST_EXPECT(that(lower).less_or_equal(upper))
#define TEST_ASSERT_LOWER(lower,upper)        TEST_EXPECT(that(lower).less_than(upper))
#define TEST_ASSERT_IN_RANGE(val,lower,upper) TEST_EXPECT(all().of(that(val).more_or_equal(lower), that(val).less_or_equal(upper)))

#define TEST_ASSERT_CONTAINS(str, part) TEST_EXPECT(that(str).is(containing(), part))

#define TEST_ASSERT_NULL(n)            TEST_ASSERT_EQUAL(n, (typeof(n))NULL)
#define TEST_ASSERT_NULL__BROKEN(n)    TEST_ASSERT_EQUAL__BROKEN(n, (typeof(n))NULL)
#define TEST_ASSERT_NOTNULL(n)         TEST_ASSERT_DIFFERENT(n, (typeof(n))NULL)
#define TEST_ASSERT_NOTNULL__BROKEN(n) TEST_ASSERT_DIFFERENT__BROKEN(n, (typeof(n))NULL)

// --------------------------------------------------------------------------------
// the following macros only work when
// - tested module depends on ARBDB and
// - some ARBDB header has been included
// Otherwise the section is skipped completely.
//
// @@@ ARBDB->ARBCORE later

#ifdef AD_PROT_H

namespace arb_test {
    inline bool test_mem_equal(const void *mem1, const void *mem2, size_t size) {
        FlushedOutputNoLF yes;
        return GB_test_mem_equal(reinterpret_cast<const unsigned char *>(mem1),
                                 reinterpret_cast<const unsigned char *>(mem2), size) == size;
    }
    inline bool test_files_equal(const char *file1, const char *file2) {
        FlushedOutputNoLF yes;
        return GB_test_files_equal(file1, file2);
    }
    inline bool test_textfile_difflines(const char *file1, const char *file2, int expected_difflines) {
        FlushedOutputNoLF yes;
        return GB_test_textfile_difflines(file1, file2, expected_difflines, 0);
    }
    inline bool test_textfile_difflines_ignoreDates(const char *file1, const char *file2, int expected_difflines) {
        FlushedOutputNoLF yes;
        return GB_test_textfile_difflines(file1, file2, expected_difflines, 1);
    }
};

#define TEST_COPY_FILE(src, dst) TEST_ASSERT(system(GBS_global_string("cp '%s' '%s'", src, dst)) == 0)

#define TEST_ASSERT_TEXTFILE_DIFFLINES(f1,f2,diff)         TEST_ASSERT(arb_test::test_textfile_difflines(f1,f2, diff))
#define TEST_ASSERT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,diff) TEST_ASSERT__BROKEN(arb_test::test_textfile_difflines(f1,f2, diff))

#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES(f1,f2,diff)         TEST_ASSERT(arb_test::test_textfile_difflines_ignoreDates(f1,f2, diff))
#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES__BROKEN(f1,f2,diff) TEST_ASSERT__BROKEN(arb_test::test_textfile_difflines_ignoreDates(f1,f2, diff))

#define TEST_ASSERT_FILES_EQUAL(f1,f2)         TEST_ASSERT(arb_test::test_files_equal(f1,f2))
#define TEST_ASSERT_FILES_EQUAL__BROKEN(f1,f2) TEST_ASSERT__BROKEN(arb_test::test_files_equal(f1,f2))

#define TEST_ASSERT_MEM_EQUAL(m1,m2,size)         TEST_ASSERT(arb_test::test_mem_equal(m1,m2,size))
#define TEST_ASSERT_MEM_EQUAL__BROKEN(m1,m2,size) TEST_ASSERT__BROKEN(arb_test::test_mem_equal(m1,m2,size))

#define TEST_ASSERT_TEXTFILES_EQUAL(f1,f2)         TEST_ASSERT_TEXTFILE_DIFFLINES(f1,f2,0)
#define TEST_ASSERT_TEXTFILES_EQUAL__BROKEN(f1,f2) TEST_ASSERT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,0)

#else

#define WARN_MISS_ADPROT() need_include__ad_prot_h__BEFORE__test_unit_h

#define TEST_ASSERT_TEXTFILE_DIFFLINES(f1,f2,diff)                      WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,diff)              WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES(f1,f2,diff)         WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES__BROKEN(f1,f2,diff) WARN_MISS_ADPROT()
#define TEST_ASSERT_FILES_EQUAL(f1,f2)                                  WARN_MISS_ADPROT()
#define TEST_ASSERT_FILES_EQUAL__BROKEN(f1,f2)                          WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILES_EQUAL(f1,f2)                              WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILES_EQUAL__BROKEN(f1,f2)                      WARN_MISS_ADPROT()

#endif // AD_PROT_H

// --------------------------------------------------------------------------------

#define TEST_SETUP_GLOBAL_ENVIRONMENT(modulename) do {                                                          \
        arb_test::test_data().raiseLocalFlag(ANY_SETUP);                                                        \
        TEST_ASSERT_NO_ERROR(GBK_system(GBS_global_string("../test_environment setup %s",  (modulename))));      \
    } while(0)
// cleanup is done (by Makefile.suite) after all unit tests have been run

// --------------------------------------------------------------------------------

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
