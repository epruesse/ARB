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

ptclean.o: ptclean.h
ptclean.o: $(ARBHOME)/INCLUDE/ad_prot.h
ptclean.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_assert.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_core.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_diff.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_error.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_file.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_msg.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_progress.h
ptclean.o: $(ARBHOME)/INCLUDE/arb_string.h
ptclean.o: $(ARBHOME)/INCLUDE/arbdb.h
ptclean.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ptclean.o: $(ARBHOME)/INCLUDE/arbdbt.h
ptclean.o: $(ARBHOME)/INCLUDE/arbtools.h
ptclean.o: $(ARBHOME)/INCLUDE/attributes.h
ptclean.o: $(ARBHOME)/INCLUDE/cxxforward.h
ptclean.o: $(ARBHOME)/INCLUDE/downcast.h
ptclean.o: $(ARBHOME)/INCLUDE/dupstr.h
ptclean.o: $(ARBHOME)/INCLUDE/gccver.h
ptclean.o: $(ARBHOME)/INCLUDE/smartptr.h
ptclean.o: $(ARBHOME)/INCLUDE/static_assert.h
ptclean.o: $(ARBHOME)/INCLUDE/test_global.h
ptclean.o: $(ARBHOME)/INCLUDE/test_unit.h
