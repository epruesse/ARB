# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx

CPP_OBJECTS = a3.o a3_seq.o a3_ptree1.o a3_ptree2.o a3_helix.o a3_darray.o a3_ali.o a3_matrix.o a3_arbdb.o a3_bihelix.o
C_OBJECTS   = a3_basen.o

$(MAIN): $(CPP_OBJECTS) $(C_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS) $(C_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

clean:
	@-rm $(CPP_OBJECTS) $(C_OBJECTS)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx) $(C_OBJECTS:.o=.c)

# DO NOT DELETE THIS LINE -- make depend depends on it.

a3.o:   a3_arbdb.hxx
a3.o: $(ARBHOME)/INCLUDE/arbdb.h
a3.o: $(ARBHOME)/INCLUDE/arb_assert.h
a3.o: $(ARBHOME)/INCLUDE/ad_prot.h
a3.o: $(ARBHOME)/INCLUDE/arbdbt.h
a3.o: $(ARBHOME)/INCLUDE/ad_t_prot.h a3_bihelix.hxx
a3.o: a3_ali.hxx a3_ptree.hxx a3_seq.hxx  a3_types.h
a3.o: a3_helix.hxx a3_darray.hxx
a3_seq.o:  a3_basen.h a3_seq.hxx
a3_seq.o:  a3_types.h
a3_ptree1.o: a3_basen.h a3_ptree.hxx a3_seq.hxx 
a3_ptree1.o: a3_types.h
a3_ptree2.o: a3_basen.h a3_ptree.hxx a3_seq.hxx 
a3_ptree2.o: a3_types.h
a3_helix.o: a3_helix.hxx a3_types.h a3_darray.hxx a3_ali.hxx a3_ptree.hxx
a3_helix.o: a3_seq.hxx a3_matrix.hxx
a3_darray.o:  a3_darray.hxx
a3_ali.o:  a3_ali.hxx
a3_ali.o: a3_ptree.hxx a3_seq.hxx a3_types.h a3_helix.hxx a3_darray.hxx
a3_matrix.o: a3_matrix.hxx a3_darray.hxx
a3_arbdb.o:   a3_arbdb.hxx
a3_arbdb.o: $(ARBHOME)/INCLUDE/arbdb.h
a3_arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h
a3_arbdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
a3_arbdb.o: $(ARBHOME)/INCLUDE/arbdbt.h
a3_arbdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
a3_bihelix.o:   a3_bihelix.hxx
a3_bihelix.o: a3_arbdb.hxx $(ARBHOME)/INCLUDE/arbdb.h
a3_bihelix.o: $(ARBHOME)/INCLUDE/arb_assert.h
a3_bihelix.o: $(ARBHOME)/INCLUDE/ad_prot.h
a3_bihelix.o: $(ARBHOME)/INCLUDE/arbdbt.h
a3_bihelix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
a3_basen.o: a3_basen.h
