# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

CPP_OBJECTS = AWTC_next_neighbours.o AWTC_rename.o AWTC_names_admin.o \
	AWTC_seq_search.o AWTC_fast_aligner.o AWTC_ClustalV.o AWTC_constructSequence.o \
	AWTC_submission.o 
# AWTC_names_client.o
$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/servercntrl.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/PT_com.h
AWTC_next_neighbours.o: $(ARBHOME)/INCLUDE/client.h
AWTC_next_neighbours.o: ./awtc_next_neighbours.hxx
AWTC_rename.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_rename.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_rename.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_rename.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_rename.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_rename.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_rename.o: $(ARBHOME)/INCLUDE/aw_window.hxx awtc_rename.hxx
AWTC_rename.o: $(ARBHOME)/INCLUDE/names_client.h
AWTC_rename.o: $(ARBHOME)/INCLUDE/servercntrl.h
AWTC_rename.o: $(ARBHOME)/INCLUDE/client.h
AWTC_names_admin.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_names_admin.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_names_admin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_names_admin.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_names_admin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_names_admin.o: $(ARBHOME)/INCLUDE/awt.hxx awtc_rename.hxx
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_seq_search.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_seq_search.o: ./awtc_seq_search.hxx awtc_assert.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/island_hopping.h
AWTC_fast_aligner.o: ./awtc_next_neighbours.hxx ./awtc_seq_search.hxx
AWTC_fast_aligner.o: awtc_assert.hxx awtc_ClustalV.hxx awtc_fast_aligner.hxx
AWTC_fast_aligner.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_ClustalV.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_ClustalV.o: awtc_ClustalV.hxx ./awtc_seq_search.hxx awtc_assert.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_constructSequence.o: awtc_fast_aligner.hxx
AWTC_constructSequence.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
AWTC_constructSequence.o: ./awtc_seq_search.hxx awtc_assert.hxx
AWTC_constructSequence.o: awtc_constructSequence.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTC_submission.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
