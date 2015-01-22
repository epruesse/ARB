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

refentries.o: refentries.h
refentries.o: $(ARBHOME)/INCLUDE/ad_prot.h
refentries.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
refentries.o: $(ARBHOME)/INCLUDE/arb_assert.h
refentries.o: $(ARBHOME)/INCLUDE/arb_core.h
refentries.o: $(ARBHOME)/INCLUDE/arb_error.h
refentries.o: $(ARBHOME)/INCLUDE/arb_msg.h
refentries.o: $(ARBHOME)/INCLUDE/arb_strarray.h
refentries.o: $(ARBHOME)/INCLUDE/arb_string.h
refentries.o: $(ARBHOME)/INCLUDE/arbdb.h
refentries.o: $(ARBHOME)/INCLUDE/arbdb_base.h
refentries.o: $(ARBHOME)/INCLUDE/arbdbt.h
refentries.o: $(ARBHOME)/INCLUDE/arbtools.h
refentries.o: $(ARBHOME)/INCLUDE/attributes.h
refentries.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_base.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_root.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_window.hxx
refentries.o: $(ARBHOME)/INCLUDE/cb.h
refentries.o: $(ARBHOME)/INCLUDE/cb_base.h
refentries.o: $(ARBHOME)/INCLUDE/cbtypes.h
refentries.o: $(ARBHOME)/INCLUDE/cxxforward.h
refentries.o: $(ARBHOME)/INCLUDE/dbitem_set.h
refentries.o: $(ARBHOME)/INCLUDE/downcast.h
refentries.o: $(ARBHOME)/INCLUDE/dupstr.h
refentries.o: $(ARBHOME)/INCLUDE/gccver.h
refentries.o: $(ARBHOME)/INCLUDE/items.h
refentries.o: $(ARBHOME)/INCLUDE/smartptr.h
refentries.o: $(ARBHOME)/INCLUDE/static_assert.h
refentries.o: $(ARBHOME)/INCLUDE/test_global.h
refentries.o: $(ARBHOME)/INCLUDE/ttypes.h
