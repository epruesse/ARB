# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend .setup

include Makefile.setup

TESTER_OBJECTS = \
	UnitTester.o \

ENVIRONMENT_OBJECTS = \
	TestEnvironment.o \

CPP_OBJECTS = $(TESTER_OBJECTS) $(ENVIRONMENT_OBJECTS)

ARBROOT=..

TEST_ENVIRONMENT_LIBS = \
	$(ARBROOT)/PROBE_COM/client.a \
	$(ARBROOT)/SERVERCNTRL/SERVERCNTRL.a \


$(MAIN): $(TESTER_OBJECTS) test_environment
	$(LINK_STATIC_LIB) $(MAIN) $(TESTER_OBJECTS)  

$(CPP_OBJECTS): Makefile.setup Makefile

test_environment: $(ENVIRONMENT_OBJECTS) $(TESTER_OBJECTS) $(TEST_ENVIRONMENT_LIBS)
	$(LINK_EXECUTABLE) test_environment $(ENVIRONMENT_OBJECTS) $(TESTER_OBJECTS) $(LIBPATH) $(ARBDB_LIB) $(TEST_ENVIRONMENT_LIBS) $(EXECLIBS)

.cxx.o:
	$(CPP) $(cflags) -c $< -DDEADLOCKGUARD=$(DEADLOCKGUARD) $(CPPINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(CPP_OBJECTS) *.a test_environment 
	$(MAKE) -f Makefile.test clean


DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

TestEnvironment.o: test_global.h
TestEnvironment.o: test_unit.h
TestEnvironment.o: UnitTester.hxx
TestEnvironment.o: $(ARBHOME)/INCLUDE/ad_prot.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/aisc_global.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_assert.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_core.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_defs.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_error.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_msg.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_str.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_string.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbdb.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbtools.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/attributes.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/bytestring.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/client.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/client_types.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/dupstr.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/PT_com.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/servercntrl.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/smartptr.h

UnitTester.o: test_global.h
UnitTester.o: test_unit.h
UnitTester.o: UnitTester.hxx
UnitTester.o: $(ARBHOME)/INCLUDE/arb_assert.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_core.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_msg.h
UnitTester.o: $(ARBHOME)/INCLUDE/attributes.h
UnitTester.o: $(ARBHOME)/INCLUDE/dupstr.h
UnitTester.o: $(ARBHOME)/INCLUDE/SigHandler.h
UnitTester.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h
UnitTester.o: $(ARBHOME)/INCLUDE/valgrind.h
