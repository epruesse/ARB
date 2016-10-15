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

Location.o: Location.h
Location.o: $(ARBHOME)/INCLUDE/ad_prot.h
Location.o: $(ARBHOME)/INCLUDE/adGene.h
Location.o: $(ARBHOME)/INCLUDE/arb_assert.h
Location.o: $(ARBHOME)/INCLUDE/arb_core.h
Location.o: $(ARBHOME)/INCLUDE/arb_error.h
Location.o: $(ARBHOME)/INCLUDE/arb_msg.h
Location.o: $(ARBHOME)/INCLUDE/arb_stdstr.h
Location.o: $(ARBHOME)/INCLUDE/arb_string.h
Location.o: $(ARBHOME)/INCLUDE/arbdb.h
Location.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Location.o: $(ARBHOME)/INCLUDE/arbtools.h
Location.o: $(ARBHOME)/INCLUDE/attributes.h
Location.o: $(ARBHOME)/INCLUDE/cxxforward.h
Location.o: $(ARBHOME)/INCLUDE/dupstr.h
Location.o: $(ARBHOME)/INCLUDE/gccver.h
Location.o: $(ARBHOME)/INCLUDE/smartptr.h
Location.o: $(ARBHOME)/INCLUDE/test_global.h
Location.o: $(ARBHOME)/INCLUDE/test_unit.h
