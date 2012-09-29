// =============================================================== //
//                                                                 //
//   File      : downcast.h                                        //
//   Purpose   : safely cast from base class to derived class      //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DOWNCAST_H
#define DOWNCAST_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#if defined(DEBUG)
#define SAFE_DOWNCASTS
#endif // DEBUG

#if defined(SAFE_DOWNCASTS)

template<class DERIVED>
inline DERIVED assert_downcasted(DERIVED expr) {
    arb_assert(expr); // impossible DOWNCAST (expr is not of type DERIVED)
    return expr;
}
template<class DERIVED, class BASE>
inline DERIVED safe_downcast(BASE expr) {
    return expr
        ? assert_downcasted<DERIVED>(dynamic_cast<DERIVED>(expr))
        : NULL;
}

#define DOWNCAST(totype, expr) safe_downcast<totype, typeof(expr)>(expr)

#else

#define DOWNCAST(totype, expr) ((totype)(expr))

#endif // SAFE_DOWNCASTS


// helper macro to overwrite accessor functions in derived classes
#define DEFINE_DOWNCAST_ACCESSORS(CLASS, NAME, VALUE)                   \
    CLASS *NAME() { return DOWNCAST(CLASS*, VALUE); }                   \
    const CLASS *NAME() const { return DOWNCAST(const CLASS*, VALUE); }

#else
#error downcast.h included twice
#endif // DOWNCAST_H
