
LIB = $(LIBPATH) -lARBDO $(SYSLIBS)

#LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS) -lCrun
LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS) 

.SUFFIXES: .o .c .cxx

OBJECT = \
	arb_a2ps.o arb_primer.o arb_swap_rnastr.o arb_2_ascii.o \
	arb_newick_2_otb.o arb_dnarates.o \
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

# DO NOT DELETE

$(ARBHOME)/bin/arb_a2ps:  
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps:  
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps:  
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps:  
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps:  
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_a2ps: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer: 
$(ARBHOME)/bin/arb_primer:  
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: 
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: 
$(ARBHOME)/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: 
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/cat_tree.hxx
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates:  
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: 
$(ARBHOME)/bin/arb_dnarates: DNAml_rates_1_0.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/aw_awars.hxx
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: 
$(ARBHOME)/bin/arb_dnarates.c: DNAml_rates_1_0.h
$(ARBHOME)/bin/arb_dnarates.c: $(ARBHOME)/INCLUDE/aw_awars.hxx
$(ARBHOME)/bin/arb_dnarates.c: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_dnarates.c: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_dnarates.c: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_dnarates.c: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_dnarates.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_2_bin:  
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_2_bin: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: 
$(ARBHOME)/bin/arb_replace: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_replace: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_replace: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: 
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: 
$(ARBHOME)/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_probe:  
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe:  
$(ARBHOME)/bin/arb_probe: 
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/PT_com.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/client.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/servercntrl.h
$(ARBHOME)/bin/arb_test:  
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test:  
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_test: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_test: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_test: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_test: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: 
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/aw_awars.hxx
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: 
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: 
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: 
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/ad_t_prot.h
