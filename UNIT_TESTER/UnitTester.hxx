// ================================================================ //
//                                                                  //
//   File      : UnitTester.hxx                                     //
//   Purpose   : unit testing - test one unit                       //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef UNITTESTER_HXX
#define UNITTESTER_HXX

typedef void (*UnitTest_function)();

struct UnitTest_simple {
    UnitTest_function  fun;
    const char        *name;
    const char        *location;
};

struct UnitTester {
    UnitTester(const char *libname, const UnitTest_simple *simple_tests);
};

#else
#error UnitTester.hxx included twice
#endif // UNITTESTER_HXX
