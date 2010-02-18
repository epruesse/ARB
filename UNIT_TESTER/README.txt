
How to use ARB unit testing
---------------------------

1. Invocation

   cd $ARBHOME
   make unit_tests

2. Add new unit test

   Go to your_function() - which needs testing and
   add

       #if (UNIT_TESTS == 1)

           void TEST_your_function() {
           }

       #endif

   If TEST_your_function() does nothing special, it is assumed the unit test passed "ok".

   If your function fails an assertion (arb_assert) or raises SIGSEGV in any other manner,
   it is assumed the unit test failed and an error message is printed to the console.

3. Valgrinding test code

   if you set

      VALGRIND=1

   in UNIT_TESTER/Makefile, valgrind will be started on the test-binary after the normal
   unit tests passed with success.








