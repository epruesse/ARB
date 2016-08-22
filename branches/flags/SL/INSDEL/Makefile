# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

proto:
	../../AISC_MKPTPS/aisc_mkpt -P -w insdel.h insdel.cxx >insdel.h.tmp
	../../SOURCE_TOOLS/mv_if_diff insdel.h.tmp insdel.h

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

insdel.o: insdel.h
insdel.o: $(ARBHOME)/INCLUDE/ad_prot.h
insdel.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
insdel.o: $(ARBHOME)/INCLUDE/adGene.h
insdel.o: $(ARBHOME)/INCLUDE/arb_assert.h
insdel.o: $(ARBHOME)/INCLUDE/arb_core.h
insdel.o: $(ARBHOME)/INCLUDE/arb_defs.h
insdel.o: $(ARBHOME)/INCLUDE/arb_diff.h
insdel.o: $(ARBHOME)/INCLUDE/arb_error.h
insdel.o: $(ARBHOME)/INCLUDE/arb_mem.h
insdel.o: $(ARBHOME)/INCLUDE/arb_msg.h
insdel.o: $(ARBHOME)/INCLUDE/arb_progress.h
insdel.o: $(ARBHOME)/INCLUDE/arb_string.h
insdel.o: $(ARBHOME)/INCLUDE/arb_unit_test.h
insdel.o: $(ARBHOME)/INCLUDE/arbdb.h
insdel.o: $(ARBHOME)/INCLUDE/arbdb_base.h
insdel.o: $(ARBHOME)/INCLUDE/arbdbt.h
insdel.o: $(ARBHOME)/INCLUDE/arbtools.h
insdel.o: $(ARBHOME)/INCLUDE/attributes.h
insdel.o: $(ARBHOME)/INCLUDE/cxxforward.h
insdel.o: $(ARBHOME)/INCLUDE/downcast.h
insdel.o: $(ARBHOME)/INCLUDE/dupstr.h
insdel.o: $(ARBHOME)/INCLUDE/gccver.h
insdel.o: $(ARBHOME)/INCLUDE/pos_range.h
insdel.o: $(ARBHOME)/INCLUDE/RangeList.h
insdel.o: $(ARBHOME)/INCLUDE/smartptr.h
insdel.o: $(ARBHOME)/INCLUDE/static_assert.h
insdel.o: $(ARBHOME)/INCLUDE/test_global.h
insdel.o: $(ARBHOME)/INCLUDE/test_unit.h
