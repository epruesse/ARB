
How to use ARB unit testing
---------------------------

1. Invocation

   cd $ARBHOME
   make unit_tests


2. Add unit test code

   Go to the unit containing your_function() and add something like
   the following to the end of the file.

       #if (UNIT_TESTS == 1)

       #include <test_unit.h>

       void TEST_your_function() {
       }

       #endif

   The names of the test functions are important:
       - they have to start with 'TEST_' and
       - need to be visible at global scope.


   Test result:

      If TEST_your_function() does nothing special, it is assumed the unit test passed "ok".

      If your function fails an assertion (TEST_ASSERT) or raises SIGSEGV in any other manner,
      it is assumed the unit test failed and an error message is printed to the console.

      (Any other test function in the same unit will nevertheless get called.)


   Failing tests:

          If you have some broken behavior that you cannot fix now, please do NOT leave
          the test as "failing". Instead change TEST_ASSERT into

              TEST_ASSERT_BROKEN(failing_condition);

          That will print a warning as reminder as long as the condition fails.
          When the behavior was fixed (so that the condition is fulfilled now),
          it will abort the test as failed!
          Just change TEST_ASSERT_BROKEN back into TEST_ASSERT then.

   Missing tests:

           You may drop a reminder like

               MISSING_TEST(describe what is missing);

           which will appear as warning during test run.


3. Valgrinding test code

   if you set

      VALGRIND=1

   in UNIT_TESTER/Makefile, valgrind will be started on the test-binary after the normal
   unit tests passed with success.
   Valgrind errors/warnings will not raise an error or abort testing.








