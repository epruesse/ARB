# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = AW_helix.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

clean:
	@-rm $(OBJECTS)
	@-rm *.a

DEPENDS = $(OBJECTS:.o=.depend)
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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AW_helix.o: AW_helix.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_helix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
