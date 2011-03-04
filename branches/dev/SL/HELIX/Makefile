# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = BI_helix.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) *.a

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

BI_helix.o: BI_helix.hxx
BI_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_assert.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_core.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_error.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_msg.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_string.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb_base.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbtools.h
BI_helix.o: $(ARBHOME)/INCLUDE/attributes.h
BI_helix.o: $(ARBHOME)/INCLUDE/dupstr.h
BI_helix.o: $(ARBHOME)/INCLUDE/smartptr.h
BI_helix.o: $(ARBHOME)/INCLUDE/test_global.h
