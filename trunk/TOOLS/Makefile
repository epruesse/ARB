
LIB = $(LIBPATH) -lARBDO $(SYSLIBS)

#LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS) -lCrun
LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS) 

.SUFFIXES: .o .c .cxx

OBJECT = \
	arb_a2ps.o arb_primer.o arb_swap_rnastr.o arb_2_ascii.o \
	arb_newick_2_otb.o arb_jim.o arb_dnarates.o \
	arb_2_bin.o arb_replace.o arb_read_tree.o \
	arb_proto_2_xsub.o arb_probe.o arb_test.o arb_export_rates.o arb_export_tree.o arb_count_chars.o \
	arb_perf_test.o

all: $(OBJECT:%.o=$(ARBHOME)/bin/%)

$(ARBHOME)/bin/arb_probe: arb_probe.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) ../SERVERCNTRL/SERVERCNTRL.a ../PROBE_COM/client.a $(LIB2)

$(ARBHOME)/bin/arb_newick_2_otb: arb_newick_2_otb.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) ../CAT/CAT.a ../LIBLINK/libARBDB.a $(SYSLIBS)

$(ARBHOME)/bin/arb_dnarates: arb_dnarates.c
	$(ACC) $(cflags) -o $@ $< $(AINCLUDES) $(LIB)

$(ARBHOME)/bin/%: %.c
	$(ACC) $(cflags) -o $@ $< $(AINCLUDES)

$(ARBHOME)/bin/%: %.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIB2)

depend:
	$(MAKEDEPEND) -o '' -p $(ARBHOME)/bin/ $(MAKEDEPENDINC) $(OBJECT:.o=.c*)

# DO NOT DELETE THIS LINE -- make depend depends on it.

/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_a2ps: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_primer: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/cat_tree.hxx
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_jim: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: DNAml_rates_1_0.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: $(ARBHOME)/INCLUDE/aw_awars.hxx
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_dnarates: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_2_bin: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_replace: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_read_tree: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: $(ARBHOME)/INCLUDE/PT_com.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: $(ARBHOME)/INCLUDE/client.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_probe: $(ARBHOME)/INCLUDE/servercntrl.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_rates: $(ARBHOME)/INCLUDE/aw_awars.hxx
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_export_tree: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_count_chars: $(ARBHOME)/INCLUDE/ad_t_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: 
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arbdb.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: $(ARBHOME)/INCLUDE/ad_prot.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arbdbt.h
/trance1/ARB/source/ARB/TOOLS/bin/arb_perf_test: $(ARBHOME)/INCLUDE/ad_t_prot.h
