# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend
CPP_OBJECTS = demo.o
$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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

demo.o: demo.hxx
demo.o: $(ARBHOME)/INCLUDE/arb_assert.h
demo.o: $(ARBHOME)/INCLUDE/aw_device.hxx
demo.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
demo.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
demo.o: $(ARBHOME)/INCLUDE/aw_root.hxx
demo.o: $(ARBHOME)/INCLUDE/aw_window.hxx
