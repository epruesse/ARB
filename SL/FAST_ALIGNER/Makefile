# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))
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

AWTC_constructSequence.o: awtc_constructSequence.hxx
AWTC_constructSequence.o: awtc_seq_search.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arb_error.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbtools.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/attributes.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/dupstr.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/smartptr.h

AWTC_seq_search.o: awtc_seq_search.hxx
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arb_error.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arbtools.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/attributes.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/dupstr.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/smartptr.h

ClustalV.o: awtc_seq_search.hxx
ClustalV.o: ClustalV.hxx
ClustalV.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ClustalV.o: $(ARBHOME)/INCLUDE/ad_prot.h
ClustalV.o: $(ARBHOME)/INCLUDE/arb_assert.h
ClustalV.o: $(ARBHOME)/INCLUDE/arb_error.h
ClustalV.o: $(ARBHOME)/INCLUDE/arbdb.h
ClustalV.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ClustalV.o: $(ARBHOME)/INCLUDE/arbtools.h
ClustalV.o: $(ARBHOME)/INCLUDE/attributes.h
ClustalV.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ClustalV.o: $(ARBHOME)/INCLUDE/dupstr.h
ClustalV.o: $(ARBHOME)/INCLUDE/smartptr.h

fast_aligner.o: awtc_seq_search.hxx
fast_aligner.o: ClustalV.hxx
fast_aligner.o: fast_aligner.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
fast_aligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_assert.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arb_error.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbdb.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbdb_base.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
fast_aligner.o: $(ARBHOME)/INCLUDE/arbtools.h
fast_aligner.o: $(ARBHOME)/INCLUDE/attributes.h
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_device.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_position.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx
fast_aligner.o: $(ARBHOME)/INCLUDE/dupstr.h
fast_aligner.o: $(ARBHOME)/INCLUDE/island_hopping.h
fast_aligner.o: $(ARBHOME)/INCLUDE/smartptr.h
