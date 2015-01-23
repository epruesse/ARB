# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

cb.o: cb.h
cb.o: cb_base.h
cb.o: cbtypes.h
cb.o: $(ARBHOME)/INCLUDE/arb_assert.h
cb.o: $(ARBHOME)/INCLUDE/arb_core.h
cb.o: $(ARBHOME)/INCLUDE/arbdb_base.h
cb.o: $(ARBHOME)/INCLUDE/arbtools.h
cb.o: $(ARBHOME)/INCLUDE/attributes.h
cb.o: $(ARBHOME)/INCLUDE/cxxforward.h
cb.o: $(ARBHOME)/INCLUDE/dupstr.h
cb.o: $(ARBHOME)/INCLUDE/gccver.h
cb.o: $(ARBHOME)/INCLUDE/smartptr.h
cb.o: $(ARBHOME)/INCLUDE/static_assert.h
cb.o: $(ARBHOME)/INCLUDE/test_global.h
cb.o: $(ARBHOME)/INCLUDE/test_unit.h
cb.o: $(ARBHOME)/INCLUDE/ttypes.h
