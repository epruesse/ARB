# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

SOURCES=$(wildcard *.cxx)
OBJECTS=$(subst .cxx,.o,$(SOURCES))

# --------------------------------------------------------------------------------

$(MAIN): $(OBJECTS) 
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)
 
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) *.a

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
	-@rm Makefile.bak # get rid of bak (generated for some dubious reason only in this directory)
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ptwrapper.o: $(ARBHOME)/INCLUDE/arb_assert.h
ptwrapper.o: $(ARBHOME)/INCLUDE/arb_core.h
ptwrapper.o: $(ARBHOME)/INCLUDE/arb_msg.h
ptwrapper.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
ptwrapper.o: $(ARBHOME)/INCLUDE/arbtools.h
ptwrapper.o: $(ARBHOME)/INCLUDE/attributes.h
ptwrapper.o: $(ARBHOME)/INCLUDE/dupstr.h
ptwrapper.o: $(ARBHOME)/INCLUDE/test_global.h
ptwrapper.o: $(ARBHOME)/INCLUDE/test_unit.h
ptwrapper.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h
