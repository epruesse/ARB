# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	AP_seq_dna.o \
	AP_seq_protein.o \
	AP_seq_simple_pro.o \
	AP_sequence.o \

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

AP_seq_dna.o: AP_parsimony_defaults.hxx
AP_seq_dna.o: AP_seq_dna.hxx
AP_seq_dna.o: AP_sequence.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/attributes.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/inline.h

AP_seq_protein.o: AP_parsimony_defaults.hxx
AP_seq_protein.o: AP_seq_protein.hxx
AP_seq_protein.o: AP_sequence.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/attributes.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/inline.h

AP_seq_simple_pro.o: AP_seq_simple_pro.hxx
AP_seq_simple_pro.o: AP_sequence.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/attributes.h

AP_sequence.o: AP_sequence.hxx
AP_sequence.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AP_sequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_sequence.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_sequence.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_sequence.o: $(ARBHOME)/INCLUDE/attributes.h
