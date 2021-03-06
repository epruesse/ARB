//  ==================================================================== //
//                                                                       //
//    File      : arbtools.h                                             //
//    Purpose   : small general purpose helper classes                   //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2003           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef ARBTOOLS_H
#define ARBTOOLS_H

#ifndef _GLIBCXX_NEW
#include <new>
#endif
#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif
#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif

//  Base class for classes that may not be copied, neither via copy
//  constructor or assignment operator.
//  (Note: see also class RefPtr below)

#ifdef Cxx11
struct Noncopyable {
    Noncopyable(const Noncopyable& other) = delete;
    Noncopyable& operator= (const Noncopyable&) = delete;
    Noncopyable() {}
};
#else
class Noncopyable {
    Noncopyable(const Noncopyable&);
    Noncopyable& operator = (const Noncopyable&);
public:
    Noncopyable() {}
};
#endif


// helper macros to make inplace-reconstruction less obfuscated
#define INPLACE_RECONSTRUCT(type,this)          \
    do {                                        \
        (this)->~type();                        \
        ::new(this) type();                     \
    } while(0)

#define INPLACE_COPY_RECONSTRUCT(type,this,other)       \
    do {                                                \
        (this)->~type();                                \
        ::new(this) type(other);                        \
    } while(0)

#define DECLARE_ASSIGNMENT_OPERATOR(T)                  \
    T& operator = (const T& other) {                    \
        INPLACE_COPY_RECONSTRUCT(T, this, other);       \
        return *this;                                   \
    }


// class to hold a pointer to an object owned by somebody else.
// (useful to avoid warnings about missing copy-ctor/op=)
template <typename T> class RefPtr {
    T *ptr;
public:
    RefPtr(T *ptr_) : ptr(ptr_) {}
    RefPtr(const RefPtr<T>& other) : ptr(other.ptr) {}
    DECLARE_ASSIGNMENT_OPERATOR(RefPtr<T>);
    ~RefPtr() {}

    operator T*() const { return ptr; }

    const T *operator->() const { return ptr; }
    T *operator->() { return ptr; }

    const T& operator*() const { return *ptr; }
    T& operator*() { return *ptr; }
};


// generic below/above predicates
template<typename T>
class isBelow {
    T t;
public:
    isBelow(T t_) : t(t_) {}
    bool operator()(T o) { return o<t; }
};

template<typename T>
class isAbove {
    T t;
public:
    isAbove(T t_) : t(t_) {}
    bool operator()(T o) { return o>t; }
};


// typedef iterator types
#define DEFINE_NAMED_ITERATORS(type,name)               \
    typedef type::iterator name##Iter;                  \
    typedef type::const_iterator name##CIter;           \
    typedef type::reverse_iterator name##RIter;         \
    typedef type::const_reverse_iterator name##CRIter

#define DEFINE_ITERATORS(type) DEFINE_NAMED_ITERATORS(type,type)


// locally modify a value, restore on destruction
template<typename T>
class LocallyModify : virtual Noncopyable {
    T& var;
    T  prevValue;
public:
    LocallyModify(T& var_, T localValue) : var(var_), prevValue(var) { var = localValue; }
    ~LocallyModify() { var = prevValue; }

    T old_value() const { return prevValue; }
};

// disallow_type
// (useful to deny template instantiation with specific types)
template <typename T, typename U> struct different_types {};
template <typename T> struct different_types<T,T>;
template <typename T, typename U> struct disallow_type {
    // Usage example:
    //     disallow_type<T, AW_CL>::here();
    // where 'T' is a template parameter

    different_types<T,U> wrong_type_used;
    static inline void here(){}
};

// StrictlyAliased_BasePtrRef allows to pass a 'DERIVED*&'
// to a function which expects a 'BASE*&'
// without breaking strict aliasing rules
template <typename DERIVED, typename BASE>
class StrictlyAliased_BasePtrRef : virtual Noncopyable {
    DERIVED*&  user_ptr;
    BASE      *forwarded_ptr;

public:
    StrictlyAliased_BasePtrRef(DERIVED*& ptr)
        : user_ptr(ptr),
          forwarded_ptr(ptr)
    {}
    ~StrictlyAliased_BasePtrRef() {
        user_ptr = static_cast<DERIVED*>(forwarded_ptr);
    }

    BASE*& forward() { return forwarded_ptr; }
};

// NAN/INF checks
// replace c99 macros isnan, isnormal and isinf. isinf is broken for gcc 4.4.3; see ../CORE/arb_diff.cxx@isinf

template <typename T> inline bool is_nan(const T& n) { return n != n; }
template <typename T> inline bool is_nan_or_inf(const T& n) { return is_nan(0*n); }
template <typename T> inline bool is_normal(const T& n) { return n != 0 && !is_nan_or_inf(n); }
template <typename T> inline bool is_inf(const T& n) { return !is_nan(n) && is_nan_or_inf(n); }

inline int double_cmp(const double d1, const double d2) {
    /*! returns <0 if d1<d2, >0 if d1>d2 (i.e. this function behaves like strcmp) */
    double d = d1-d2;
    return d<0 ? -1 : (d>0 ? 1 : 0);
}

template <typename NUM>
inline int calc_digits(NUM val) {
    /*! calculate output length of val (w/o sign) */
    return val ? log10(val)+1 : 1;
}
template <typename NUM>
inline int calc_signed_digits(NUM val) {
    /*! calculate output length of val (with sign) */
    return val<0 ? calc_digits(-val)+1 : calc_digits(val);
}

#else
#error arbtools.h included twice
#endif // ARBTOOLS_H

