# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = RegExpr.o
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

RegExpr.o: RegExpr.hxx
RegExpr.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
RegExpr.o: $(ARBHOME)/INCLUDE/ad_prot.h
RegExpr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RegExpr.o: $(ARBHOME)/INCLUDE/arb_assert.h
RegExpr.o: $(ARBHOME)/INCLUDE/arb_core.h
RegExpr.o: $(ARBHOME)/INCLUDE/arb_error.h
RegExpr.o: $(ARBHOME)/INCLUDE/arbdb.h
RegExpr.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RegExpr.o: $(ARBHOME)/INCLUDE/arbdbt.h
RegExpr.o: $(ARBHOME)/INCLUDE/arbtools.h
RegExpr.o: $(ARBHOME)/INCLUDE/attributes.h
RegExpr.o: $(ARBHOME)/INCLUDE/dupstr.h
RegExpr.o: $(ARBHOME)/INCLUDE/smartptr.h
RegExpr.o: $(ARBHOME)/INCLUDE/test_global.h
