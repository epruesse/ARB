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

namespace arb_compile_assertion {
    template <bool> struct is;
    template <> struct is<true> { enum { value = 1 }; };
    template<int x> struct static_assert_test{};
}

#define CA_JOINTYPE(X,Y) X##Y
#define CA_UNIQUETYPE(typename) CA_JOINTYPE(typename,__LINE__)

#define COMPILE_ASSERT(const_expression)                                \
    typedef ::arb_compile_assertion::static_assert_test<sizeof(::arb_compile_assertion::is< (bool)( const_expression ) >)> \
    CA_UNIQUETYPE(_arb_compile_assertion_typedef_)

#else
#error static_assert.h included twice
#endif // STATIC_ASSERT_H
