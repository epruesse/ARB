# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = \
	AWTC_next_neighbours.o \
	AWTC_seq_search.o \
	AWTC_fast_aligner.o \
	AWTC_ClustalV.o \
	AWTC_constructSequence.o \
	AWTC_submission.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AWTC_ClustalV.o: awtc_ClustalV.hxx
AWTC_ClustalV.o: awtc_seq_search.hxx
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWTC_constructSequence.o: awtc_constructSequence.hxx
AWTC_constructSequence.o: awtc_fast_aligner.hxx
AWTC_constructSequence.o: awtc_seq_search.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

AWTC_fast_aligner.o: awtc_ClustalV.hxx
AWTC_fast_aligner.o: awtc_fast_aligner.hxx
AWTC_fast_aligner.o: awtc_next_neighbours.hxx
AWTC_fast_aligner.o: awtc_seq_search.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/awt.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/island_hopping.h

AWTC_next_neighbours.o: awtc_next_neighbours.hxx
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/client.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/PT_com.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/servercntrl.h

AWTC_seq_search.o: awtc_seq_search.hxx
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWTC_submission.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_window.hxx
