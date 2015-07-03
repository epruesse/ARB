# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

AWTC_next_neighbours.o: awtc_next_neighbours.hxx
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/aisc_global.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_core.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_error.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_msg.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_string.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbtools.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/attributes.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/bytestring.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/cb.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/cb_base.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/cbtypes.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/client.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/client_types.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/cxxforward.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/downcast.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/dupstr.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/gccver.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/pos_range.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/PT_com.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/PT_global_defs.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/servercntrl.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/smartptr.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/static_assert.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/test_global.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/test_unit.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ttypes.h

AWTC_submission.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_core.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_error.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_msg.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_strarray.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_string.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbtools.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/attributes.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/cb.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/cb_base.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/cbtypes.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/cxxforward.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/downcast.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/dupstr.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/gccver.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/smartptr.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/static_assert.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/test_global.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/ttypes.h
