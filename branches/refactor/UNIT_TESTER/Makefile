# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

TESTER_OBJECTS = \
	UnitTester.o \

GLOBAL_OBJECTS = \
	TestEnvironment.o \

CPP_OBJECTS = $(TESTER_OBJECTS) $(GLOBAL_OBJECTS)

$(MAIN): $(TESTER_OBJECTS) test_environment
	$(LINK_STATIC_LIB) $(MAIN) $(TESTER_OBJECTS)

test_environment: $(GLOBAL_OBJECTS) $(TESTER_OBJECTS)
	$(LINK_EXECUTABLE) test_environment $(GLOBAL_OBJECTS) $(TESTER_OBJECTS) $(LIBPATH) $(ARBDB_LIB)  

.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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
TestEnvironment.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/ad_prot.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_assert.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_core.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_defs.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_error.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arb_str.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbdb.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/arbtools.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/attributes.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/dupstr.h
TestEnvironment.o: $(ARBHOME)/INCLUDE/smartptr.h

UnitTester.o: test_global.h
UnitTester.o: test_unit.h
UnitTester.o: UnitTester.hxx
UnitTester.o: $(ARBHOME)/INCLUDE/arb_assert.h
UnitTester.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
UnitTester.o: $(ARBHOME)/INCLUDE/SigHandler.h
