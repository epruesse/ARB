# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend .setup

first: $(MAIN)

include Makefile.setup.include

LOCAL_DEFINES=-DSIMPLE_ARB_ASSERT
LOCAL_MAKEDEPENDFLAGS=$(MAKEDEPENDFLAGS) $(LOCAL_DEFINES)

TESTER_OBJECTS = \
	UnitTester.o \

ENVIRONMENT_OBJECTS = \
	TestEnvironment.o \

ALL_OBJECTS = $(TESTER_OBJECTS) $(ENVIRONMENT_OBJECTS)

ARBROOT=..

TEST_ENVIRONMENT_LIBS = \
	$(ARBROOT)/SERVERCNTRL/SERVERCNTRL.a \
	$(ARBROOT)/PROBE_COM/client.a \


$(MAIN): $(TESTER_OBJECTS) test_environment
	$(LINK_STATIC_LIB) $(MAIN) $(TESTER_OBJECTS)  

$(ALL_OBJECTS): Makefile.setup.local Makefile

test_environment: $(ALL_OBJECTS) $(TEST_ENVIRONMENT_LIBS)
	$(LINK_EXECUTABLE) test_environment $(ALL_OBJECTS) $(EXECLIBS) $(LIBPATH) $(ARBDB_LIB) $(TEST_ENVIRONMENT_LIBS) $(ARB_GLIB_LIBS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) $(LOCAL_DEFINES) -c $< -DDEADLOCKGUARD=$(DEADLOCKGUARD) $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(ALL_OBJECTS) *.a test_environment skipslow.stamp
	rm -rf logs tests tests.slow
	$(MAKE) -f Makefile.test clean


DEPENDS = $(ALL_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(LOCAL_MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(LOCAL_MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(LOCAL_MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

TestEnvironment.o: ../SOURCE_TOOLS/arb_main.h
TestEnvironment.o: test_global.h
TestEnvironment.o: test_unit.h
TestEnvironment.o: UnitTester.hxx
TestEnvironment.o: $(ARBHOME)/INCLUDE/ad_prot.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/aisc_global.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_assert.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_core.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_defs.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_diff.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_error.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_file.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_mem.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_msg.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_str.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_string.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbdb.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbtools.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/attributes.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/bytestring.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/client.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/client_types.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/cxxforward.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/dupstr.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/gccver.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/PT_com.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/servercntrl.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/smartptr.h

UnitTester.o: ../SOURCE_TOOLS/arb_main.h
UnitTester.o: test_global.h
UnitTester.o: test_unit.h
UnitTester.o: UnitTester.hxx
UnitTester.o: $(ARBHOME)/INCLUDE/arb_assert.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_core.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_msg.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_pathlen.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
UnitTester.o: $(ARBHOME)/INCLUDE/arbtools.h
UnitTester.o: $(ARBHOME)/INCLUDE/attributes.h
UnitTester.o: $(ARBHOME)/INCLUDE/cxxforward.h
UnitTester.o: $(ARBHOME)/INCLUDE/dupstr.h
UnitTester.o: $(ARBHOME)/INCLUDE/gccver.h
UnitTester.o: $(ARBHOME)/INCLUDE/SigHandler.h
UnitTester.o: $(ARBHOME)/INCLUDE/static_assert.h
UnitTester.o: $(ARBHOME)/INCLUDE/SuppressOutput.h
UnitTester.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h
UnitTester.o: $(ARBHOME)/INCLUDE/valgrind.h
