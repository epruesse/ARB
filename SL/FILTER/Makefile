# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = AP_filter.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

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

AP_filter.o: AP_filter.hxx
AP_filter.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AP_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_filter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_filter.o: $(ARBHOME)/INCLUDE/attributes.h
AP_filter.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_filter.o: $(ARBHOME)/INCLUDE/inline.h
AP_filter.o: $(ARBHOME)/INCLUDE/smartptr.h
