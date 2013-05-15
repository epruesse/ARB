# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

OBJECTS = mkptypes.o
BINARY = aisc_mkpt

$(MAIN): $(BINARY)

$(BINARY): $(OBJECTS)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(OBJECTS) $(EXECLIBS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) $(BINARY)

#depends:
#	@echo "No depends here"

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@


# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

mkptypes.o: $(ARBHOME)/INCLUDE/arb_assert.h
mkptypes.o: $(ARBHOME)/INCLUDE/arb_core.h
mkptypes.o: $(ARBHOME)/INCLUDE/arb_early_check.h
mkptypes.o: $(ARBHOME)/INCLUDE/arbtools.h
mkptypes.o: $(ARBHOME)/INCLUDE/attributes.h
mkptypes.o: $(ARBHOME)/INCLUDE/cxxforward.h
mkptypes.o: $(ARBHOME)/INCLUDE/dupstr.h
mkptypes.o: $(ARBHOME)/INCLUDE/gccver.h
mkptypes.o: $(ARBHOME)/INCLUDE/static_assert.h
mkptypes.o: $(ARBHOME)/INCLUDE/test_global.h
mkptypes.o: $(ARBHOME)/INCLUDE/test_unit.h
