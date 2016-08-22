// ============================================================= //
//                                                               //
//   File      : static_assert.h                                 //
//   Purpose   : compile time assertion                          //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef STATIC_ASSERT_H
#define STATIC_ASSERT_H

#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif

#if Cxx11

#define STATIC_ASSERT(const_expression)                      static_assert(const_expression,#const_expression)
#define STATIC_ASSERT_ANNOTATED(const_expression,failReason) static_assert(const_expression,failReason)

#else

namespace arb_compile_assertion {
    template <bool> struct is;
    template <> struct is<true> { enum { value = 1 }; };
    template<int x> struct static_assert_test{};
}

#define CA_JOIN(X,Y) X##Y
#define CA_JOINTYPE(X,Y) CA_JOIN(X, Y)
#define CA_UNIQUETYPE(typename) CA_JOINTYPE(typename,__LINE__)

#define COMPILE_ASSERTED_TYPE(const_expression)              ::arb_compile_assertion::static_assert_test<sizeof(::arb_compile_assertion::is< (bool)( const_expression ) >)>
#define STATIC_ASSERT(const_expression)                      typedef COMPILE_ASSERTED_TYPE(const_expression) CA_UNIQUETYPE(_arb_compile_assertion_typedef_)
#define STATIC_ASSERT_ANNOTATED(const_expression,annotation) STATIC_ASSERT(const_expression)

#endif

#else
#error static_assert.h included twice
#endif // STATIC_ASSERT_H
