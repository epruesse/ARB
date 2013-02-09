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

//  Base class for classes that may not be copied, neither via copy
//  constructor or assignment operator.

class Noncopyable {
    Noncopyable(const Noncopyable&);
    Noncopyable& operator=(const Noncopyable&);
public:
    Noncopyable() {}
};

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

#define DEFINE_NAMED_ITERATORS(type,name)               \
    typedef type::iterator name##Iter;                  \
    typedef type::const_iterator name##CIter;           \
    typedef type::reverse_iterator name##RIter;         \
    typedef type::const_reverse_iterator name##CRIter

#define DEFINE_ITERATORS(type) DEFINE_NAMED_ITERATORS(type,type)

template<typename T>
class LocallyModify {
    T& var;
    T  prevValue;
public:
    LocallyModify(T& var_, T localValue) : var(var_), prevValue(var) { var = localValue; }
    ~LocallyModify() { var = prevValue; }

    T old_value() const { return prevValue; }
};

#else
#error arbtools.h included twice
#endif // ARBTOOLS_H

