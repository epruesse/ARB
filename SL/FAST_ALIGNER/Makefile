# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

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

ClustalV.o: ClustalV.hxx
ClustalV.o: seq_search.hxx
ClustalV.o: $(ARBHOME)/INCLUDE/arb_assert.h
ClustalV.o: $(ARBHOME)/INCLUDE/arb_core.h
ClustalV.o: $(ARBHOME)/INCLUDE/arb_error.h
ClustalV.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ClustalV.o: $(ARBHOME)/INCLUDE/arbtools.h
ClustalV.o: $(ARBHOME)/INCLUDE/attributes.h
ClustalV.o: $(ARBHOME)/INCLUDE/aw_base.hxx
ClustalV.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
ClustalV.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
ClustalV.o: $(ARBHOME)/INCLUDE/cxxforward.h
ClustalV.o: $(ARBHOME)/INCLUDE/dupstr.h
ClustalV.o: $(ARBHOME)/INCLUDE/gccver.h
ClustalV.o: $(ARBHOME)/INCLUDE/smartptr.h
ClustalV.o: $(ARBHOME)/INCLUDE/test_global.h
ClustalV.o: $(ARBHOME)/INCLUDE/test_unit.h

fast_aligner.o: ClustalV.hxx
fast_aligner.o: fast_aligner.hxx
fast_aligner.o: seq_search.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_cb.h
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_assert.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_core.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_defs.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_error.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_msg.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_progress.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_string.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_unit_test.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbdb.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbdb_base.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbtools.h
fast_aligner.o: $(ARBHOME)/INCLUDE/attributes.h
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_base.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_question.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/cb.h
fast_aligner.o: $(ARBHOME)/INCLUDE/cb_base.h
fast_aligner.o: $(ARBHOME)/INCLUDE/cbtypes.h
fast_aligner.o: $(ARBHOME)/INCLUDE/cxxforward.h
fast_aligner.o: $(ARBHOME)/INCLUDE/downcast.h
fast_aligner.o: $(ARBHOME)/INCLUDE/dupstr.h
fast_aligner.o: $(ARBHOME)/INCLUDE/gccver.h
fast_aligner.o: $(ARBHOME)/INCLUDE/island_hopping.h
fast_aligner.o: $(ARBHOME)/INCLUDE/pos_range.h
fast_aligner.o: $(ARBHOME)/INCLUDE/PT_global_defs.h
fast_aligner.o: $(ARBHOME)/INCLUDE/RangeList.h
fast_aligner.o: $(ARBHOME)/INCLUDE/rootAsWin.h
fast_aligner.o: $(ARBHOME)/INCLUDE/smartptr.h
fast_aligner.o: $(ARBHOME)/INCLUDE/static_assert.h
fast_aligner.o: $(ARBHOME)/INCLUDE/test_global.h
fast_aligner.o: $(ARBHOME)/INCLUDE/test_unit.h
fast_aligner.o: $(ARBHOME)/INCLUDE/ttypes.h

seq_search.o: seq_search.hxx
seq_search.o: $(ARBHOME)/INCLUDE/ad_prot.h
seq_search.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
seq_search.o: $(ARBHOME)/INCLUDE/arb_assert.h
seq_search.o: $(ARBHOME)/INCLUDE/arb_core.h
seq_search.o: $(ARBHOME)/INCLUDE/arb_error.h
seq_search.o: $(ARBHOME)/INCLUDE/arb_msg.h
seq_search.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
seq_search.o: $(ARBHOME)/INCLUDE/arb_string.h
seq_search.o: $(ARBHOME)/INCLUDE/arbdb.h
seq_search.o: $(ARBHOME)/INCLUDE/arbdb_base.h
seq_search.o: $(ARBHOME)/INCLUDE/arbdbt.h
seq_search.o: $(ARBHOME)/INCLUDE/arbtools.h
seq_search.o: $(ARBHOME)/INCLUDE/attributes.h
seq_search.o: $(ARBHOME)/INCLUDE/aw_base.hxx
seq_search.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
seq_search.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
seq_search.o: $(ARBHOME)/INCLUDE/cxxforward.h
seq_search.o: $(ARBHOME)/INCLUDE/downcast.h
seq_search.o: $(ARBHOME)/INCLUDE/dupstr.h
seq_search.o: $(ARBHOME)/INCLUDE/gccver.h
seq_search.o: $(ARBHOME)/INCLUDE/smartptr.h
seq_search.o: $(ARBHOME)/INCLUDE/static_assert.h
seq_search.o: $(ARBHOME)/INCLUDE/test_global.h
seq_search.o: $(ARBHOME)/INCLUDE/test_helpers.h
seq_search.o: $(ARBHOME)/INCLUDE/test_unit.h
