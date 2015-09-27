# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

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

chartable.o: chartable.h
chartable.o: consensus.h
chartable.o: $(ARBHOME)/INCLUDE/arb_assert.h
chartable.o: $(ARBHOME)/INCLUDE/arb_core.h
chartable.o: $(ARBHOME)/INCLUDE/arb_defs.h
chartable.o: $(ARBHOME)/INCLUDE/arb_msg.h
chartable.o: $(ARBHOME)/INCLUDE/arbdb_base.h
chartable.o: $(ARBHOME)/INCLUDE/arbtools.h
chartable.o: $(ARBHOME)/INCLUDE/attributes.h
chartable.o: $(ARBHOME)/INCLUDE/aw_base.hxx
chartable.o: $(ARBHOME)/INCLUDE/cxxforward.h
chartable.o: $(ARBHOME)/INCLUDE/dupstr.h
chartable.o: $(ARBHOME)/INCLUDE/gccver.h
chartable.o: $(ARBHOME)/INCLUDE/iupac.h
chartable.o: $(ARBHOME)/INCLUDE/pos_range.h
chartable.o: $(ARBHOME)/INCLUDE/test_global.h
chartable.o: $(ARBHOME)/INCLUDE/test_unit.h

consensus.o: consensus.h
consensus.o: $(ARBHOME)/INCLUDE/aw_base.hxx
