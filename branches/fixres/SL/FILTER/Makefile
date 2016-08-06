# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

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

AP_filter.o: AP_filter.hxx
AP_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_filter.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_filter.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_filter.o: $(ARBHOME)/INCLUDE/attributes.h
AP_filter.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_filter.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_filter.o: $(ARBHOME)/INCLUDE/gccver.h
AP_filter.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_filter.o: $(ARBHOME)/INCLUDE/test_global.h

RangeList.o: RangeList.h
RangeList.o: $(ARBHOME)/INCLUDE/arb_assert.h
RangeList.o: $(ARBHOME)/INCLUDE/arb_core.h
RangeList.o: $(ARBHOME)/INCLUDE/arb_mem.h
RangeList.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
RangeList.o: $(ARBHOME)/INCLUDE/arbtools.h
RangeList.o: $(ARBHOME)/INCLUDE/attributes.h
RangeList.o: $(ARBHOME)/INCLUDE/cxxforward.h
RangeList.o: $(ARBHOME)/INCLUDE/dupstr.h
RangeList.o: $(ARBHOME)/INCLUDE/gccver.h
RangeList.o: $(ARBHOME)/INCLUDE/pos_range.h
RangeList.o: $(ARBHOME)/INCLUDE/test_global.h
RangeList.o: $(ARBHOME)/INCLUDE/test_unit.h
