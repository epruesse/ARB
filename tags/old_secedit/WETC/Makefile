# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = WETC_main.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

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

WETC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/awt.hxx
