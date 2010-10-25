
How to use ARB unit testing
---------------------------

1. Invocation

   Edit config.makefile. Set UNIT_TESTS to 1

   cd $ARBHOME
   make unit_tests

   'make all' runs tests as well ('make build' doesn't)

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

   (see also 'Building tests' at end of file)


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

      Set WARN_LEVEL to 0 in Makefile.setup to disable warnings.


   Missing tests:

      You may drop a reminder like

          MISSING_TEST(describe what is missing);

      which will appear as warning during test run.


   Order of tests:

      Tests are executed in the order they appear in the code.
      Multiple Files are sorted alphabethically.

      Exceptions:
      - If test name starts with 'TEST_###' (where ### are digits), it declares a test with priority ###.
        The default priority is 100. Lower numbers mean higher priority, i.e. start more early. 
      - Other predefined priorities are
        - TEST_BASIC_ (=20)
        - TEST_EARLY_ (=50)
        - TEST_SLOW_  (=900)

        TEST_SLOW_... is meant to indicate "slow" tests (i.e. tests which need more than
        a second to execute).

   Order of test units:

      Tests from each unit (aka library) run consecutive.
      The order of the units is defined in ARB Makefile.


3. Valgrinding test code

   If you set

      VALGRIND=1

   in UNIT_TESTER/Makefile.setup, valgrind will be started on the test-binary after the normal
   unit tests passed with success.
   Valgrind errors/warnings will not raise an error or abort testing.

   If you set

      VALGRIND=2

   valgrind will be started BEFORE running tests.
   Useful if test fail unexpectedly and you suspect a memory bug.

   See also ENABLE_CRASH_TESTS in test_unit.h (all crash tests are reported by valgrind)


4. Check test-coverage

   set 'COVERAGE=1' in config.makefile and 'make rebuild'

   - prints out uncovered code to compile log
   - leaves info for partly covered code files in yourcode.cxx.cov

   Note:
        You can silence lines which are never reached while testing
        with a comment containing 'NEED_NO_COV'.

   call 'make clean_coverage_results' to get rid of .cov files


5. Running only some tests

   Set 'RESTRICT=expr' to '.' to run all tests.

   Set it to sth else to run only tests for matching source files.
   This is helpful if you'd like to check test-coverage.

   Set 'RESTRICT_LIB=lib1:lib2' to test only a few libs.


   Slow tests ('TEST_SLOW_...'; see above) will only run every 15 minutes.


6. Global test environment

   @@@ Not in SVN yet


7. Auto-patch generation

   Whenever unit tests complete successfully, a patch (svn diff) is generated and
   stored inside $ARBHOME/patches.arb/

   After DELETE_PATCHES_AFTER seconds the patch will be deleted automatically
   (see Makefile.setup).

8. Building tests

   To build and execute a test-executable for a previously untested subdir
   the following steps are necessary:

   - add the test to the list of performed tests in ../Makefile@UNITS_TESTED
   - if the test subject is a shared library, be sure to mention it a Makefile.test@SHAREDLIBS
   - if the test subject is an executable, be sure to mention it a Makefile.test@EXEOBJDIRS
