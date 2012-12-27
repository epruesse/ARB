// ================================================================= //
//                                                                   //
//   File      : ps_assert.hxx                                       //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2012   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef PS_ASSERT_HXX
#define PS_ASSERT_HXX

#define CRASH() do { *(int *)0=0; } while (0)

#ifndef NDEBUG
# define ps_assert(bed) do { if (!(bed)) CRASH(); } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define ps_assert(bed)
#endif // NDEBUG

#else
#error ps_assert.hxx included twice
#endif // PS_ASSERT_HXX
