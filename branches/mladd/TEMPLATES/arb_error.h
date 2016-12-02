// ================================================================ //
//                                                                  //
//   File      : arb_error.h                                        //
//   Purpose   : replacement for GB_ERROR                           //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2009   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_ERROR_H
#define ARB_ERROR_H

// ARB_ERROR is a dropin replacement for GB_ERROR (C++ only)
//
// If CHECK_ERROR_DROP is undefined (e.g. in NDEBUG version)
// there should be nearly no runtime overhead compared with plain GB_ERROR
//
// If CHECK_ERROR_DROP is defined
// the following actions cause an assertion failure:
//
// * not "using" an error (regardless whether an error occurred or not)
// * "using" an error several times (regardless whether an error occurred or not)
// * overwriting an error (if error is not NULL)
// * declaring errors as global variables
//
// Code changes needed:
// * replace GB_ERROR by ARB_ERROR
// * use error.deliver() whereever an existing error is "used"
// * rewrite code that uses 'static GB_ERROR' (no longer allowed)
//
// "Using" an error means e.g.
// * display error to user (e.g. aw_message_if())
// * construct a new error based on an error (e.g. GB_end_transaction)
//
// Sometimes an error result is known to be always no error.
// In this case use error.expect_no_error() to mark the error as "used"


#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

#if defined(DEBUG)
#define CHECK_ERROR_DROP
#if defined(DEVEL_RALF)
// #define EXTRA_ERROR_INFO
#endif // DEVEL_RALF
#endif // DEBUG

#if defined(CHECK_ERROR_DROP)

class ARB_undropable_error : virtual Noncopyable {
    GB_ERROR    gberr;
    bool        delivered;
#if defined(EXTRA_ERROR_INFO)
    const char *file;
    size_t      line;
#endif // EXTRA_ERROR_INFO
public:
    ARB_undropable_error(GB_ERROR gberr_) : gberr(gberr_), delivered(false)
#if defined(EXTRA_ERROR_INFO)
                                          , file(__FILE__), line(__LINE__)
#endif // EXTRA_ERROR_INFO
    {}
    ~ARB_undropable_error() {
        arb_assert(delivered);                      // oops - error has been dropped

        // Note: if this fails from inside exit() a static ARB_ERROR was declared
        // This is no longer allowed
    }

    GB_ERROR deliver() {
        GB_ERROR err = gberr;
        drop();
        return err;
    }

    GB_ERROR look() const { return gberr; }

    void drop() {
        arb_assert(!delivered);                     // oops - error delivered/dropped twice
        gberr     = NULL;
        delivered = true;
    }

    void Set(GB_ERROR err) {
        arb_assert(gberr == NULL);                  // oops - you are trying to overwrite an existing error
        gberr     = err;
        delivered = false;
    }
};

#endif // CHECK_ERROR_DROP



class ARB_ERROR {
#if defined(CHECK_ERROR_DROP)
    mutable SmartPtr<ARB_undropable_error> error;
    GB_ERROR look() const { return error->look(); }
public:
    explicit ARB_ERROR() : error(new ARB_undropable_error(NULL)) {}
    ARB_ERROR(GB_ERROR gberr) : error(new ARB_undropable_error(gberr)) {}
    GB_ERROR deliver() const { return error->deliver(); }
    ARB_ERROR& operator = (GB_ERROR gberr) { error->Set(gberr); return *this; }
#else
    GB_ERROR error;
    GB_ERROR look() const { return error; }
public:
    explicit ARB_ERROR() : error(NULL) {}
    ARB_ERROR(GB_ERROR gberr) : error(gberr) {}
    GB_ERROR deliver() const { return error; }
    ARB_ERROR& operator = (GB_ERROR gberr) { error = gberr; return *this; }
#endif // CHECK_ERROR_DROP

    // common for both ARB_ERROR flavors:
private:
    bool occurred() const { return look() != NULL; }

public:
    ARB_ERROR(const ARB_ERROR& err) : error(err.error) {}
    ARB_ERROR& operator = (const ARB_ERROR& err) {  // assigning an error to a new error
        if (error != err.error) *this = err.deliver(); // delivers the error
        return *this;
    }

    operator bool() const { return occurred(); } // needed for 'if (!error)' and similar

    void set_handled() const {
        arb_assert(occurred()); // oops - tried to handle an error that did not occur
        deliver();              // deliver and drop
    }

    void expect_no_error() const {
        arb_assert(!occurred()); // oops - no error expected here
        deliver();               // mark impossible errors as "used"
    }

    GB_ERROR preserve() const { return look(); } // look, but do not deliver

#if defined(DEBUG)
    void dump() const {
        printf("ARB_ERROR='%s'\n", look());
    }
#endif // DEBUG
};

class Validity {
    // basically a bool telling whether sth is valid
    // (if invalid -> knows why)
    const char *why_invalid;
public:
    Validity() : why_invalid(0) {}
    Validity(bool valid, const char *why_invalid_)
        : why_invalid(valid ? 0 : why_invalid_)
    {}
    Validity(const Validity& other) : why_invalid(other.why_invalid) {}
    Validity& operator = (const Validity& other) {
#if defined(ASSERTION_USED)
        arb_assert(!why_invalid); // attempt to overwrite invalidity!
#endif
        why_invalid = other.why_invalid;
        return *this;
    }

    operator bool() const { return !why_invalid; }
    const char *why_not() const { return why_invalid; }

    void assert_is_valid() const {
#if defined(ASSERTION_USED)
        if (why_invalid) {
            fprintf(stderr, "Invalid: %s\n", why_invalid);
            arb_assert(0);
        }
#endif
    }
};

#else
#error arb_error.h included twice
#endif // ARB_ERROR_H
