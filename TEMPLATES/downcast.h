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
#ifndef STATIC_ASSERT_H
#include "static_assert.h"
#endif

#if defined(DEBUG)
#define SAFE_DOWNCASTS
#endif // DEBUG

// --------------------

namespace ARB_type_traits { // according to boost-type_traits or std-type_traits
                            // (remove when available in std for lowest supported gcc-version)
    typedef char (&yes)[1];
    typedef char (&no)[2];

    template <typename B, typename D>
    struct Host {
        operator B*() const;
        operator D*();
    };

    template <typename B, typename D>
    struct is_base_of {
        template <typename T> static yes check(D*, T);
        static no check(B*, int);

        static const bool value = sizeof(check(Host<B,D>(), int())) == sizeof(yes);
    };

    template <typename T, typename U>
    struct is_same {
        static T *t;
        static U *u;

        static yes check(T*, T*);
        static no  check(...);

        static const bool value = sizeof(check(t, u)) == sizeof(yes);
    };

    template< class T > struct remove_const          { typedef T type; };
    template< class T > struct remove_const<const T> { typedef T type; };

    template< class T > struct remove_volatile             { typedef T type; };
    template< class T > struct remove_volatile<volatile T> { typedef T type; };

    template< class T > struct remove_cv { typedef typename remove_volatile<typename remove_const<T>::type>::type type; };

};

// -----------------------------------------
//      detect and deref pointer-types:


template<typename> struct dereference {
    static const bool possible = false;
};
template<typename T> struct dereference<T*> {
    static const bool possible = true;
    typedef T type;
};
template<typename T> struct dereference<T*const> {
    static const bool possible = true;
    typedef T type;
};

// ----------------------------------------------
// DOWNCAST from DERIVED* to BASE*
// - uses dynamic_cast in DEBUG mode and checks for failure
// - uses plain old cast in NDEBUG mode

#if defined(SAFE_DOWNCASTS)

template<class DERIVED>
inline DERIVED assert_downcasted(DERIVED expr) {
    arb_assert(expr); // impossible DOWNCAST (expr is not of type DERIVED)
    return expr;
}

template<class DERIVED, class BASE>
inline DERIVED *safe_pointer_downcast(BASE *expr) {
    STATIC_ASSERT_ANNOTATED(dereference<BASE   >::possible == false, "got BASE** (expected BASE*)");
    STATIC_ASSERT_ANNOTATED(dereference<DERIVED>::possible == false, "got DERIVED** (expected DERIVED*)");

    typedef typename ARB_type_traits::remove_cv<BASE   >::type NCV_BASE;
    typedef typename ARB_type_traits::remove_cv<DERIVED>::type NCV_DERIVED;

    STATIC_ASSERT_ANNOTATED((ARB_type_traits::is_base_of<BASE,DERIVED>::value ||
                             ARB_type_traits::is_same<NCV_BASE, NCV_DERIVED>::value),
                             "downcast only allowed from base type to derived type");

    return expr
        ? assert_downcasted<DERIVED*>(dynamic_cast<DERIVED*>(expr))
        : NULL;
}

template<class DERIVED_PTR, class BASE_PTR>
inline DERIVED_PTR safe_downcast(BASE_PTR expr) {
    STATIC_ASSERT_ANNOTATED(dereference<BASE_PTR   >::possible == true, "expected 'BASE*' as source type");
    STATIC_ASSERT_ANNOTATED(dereference<DERIVED_PTR>::possible == true, "expected 'DERIVED*' as target type");

    typedef typename dereference<BASE_PTR   >::type BASE;
    typedef typename dereference<DERIVED_PTR>::type DERIVED;

    return safe_pointer_downcast<DERIVED,BASE>(expr);
}

#define DOWNCAST(totype,expr)      safe_downcast<totype,typeof(expr)>(expr)

#else

#define DOWNCAST(totype,expr)      ((totype)(expr))

#endif // SAFE_DOWNCASTS


// helper macro to overwrite accessor functions in derived classes
#define DEFINE_DOWNCAST_ACCESSORS(CLASS, NAME, VALUE)                   \
    CLASS *NAME() { return DOWNCAST(CLASS*, VALUE); }                   \
    const CLASS *NAME() const { return DOWNCAST(const CLASS*, VALUE); }

#else
#error downcast.h included twice
#endif // DOWNCAST_H
