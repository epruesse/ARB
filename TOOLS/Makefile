
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


arb_2_ascii.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_ascii.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c:   
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c:  
arb_2_ascii.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_2_ascii.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_2_bin.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_bin.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_2_bin.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_bin.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c:   
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c:  
arb_2_bin.c: 
arb_2_bin.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_2_bin.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:   
arb_a2ps:   
arb_a2ps:  
arb_a2ps:  
arb_a2ps:  
arb_a2ps:   
arb_a2ps:  
arb_a2ps: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_a2ps: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_count_chars.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_count_chars.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_count_chars.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_count_chars.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c:  
arb_count_chars.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_count_chars.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_dnarates: $(ARBHOME)/INCLUDE/ad_prot.h
arb_dnarates: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_dnarates: $(ARBHOME)/INCLUDE/arbdb.h
arb_dnarates: $(ARBHOME)/INCLUDE/arbdbt.h
arb_dnarates: $(ARBHOME)/INCLUDE/aw_awars.hxx
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:   
arb_dnarates:  
arb_dnarates:  
arb_dnarates:  
arb_dnarates:   
arb_dnarates: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
arb_dnarates: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_dnarates: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_dnarates: DNAml_rates_1_0.h
arb_export_rates.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_rates.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_rates.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_rates.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_rates.c: $(ARBHOME)/INCLUDE/aw_awars.hxx
arb_export_rates.c:  
arb_export_rates.c: 
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c:  
arb_export_rates.c: 
arb_export_rates.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_export_rates.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_export_tree.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_tree.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_tree.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_tree.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c:  
arb_export_tree.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_export_tree.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_jim.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_jim.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_jim.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_jim.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_jim.c:  
arb_jim.c:  
arb_jim.c:  
arb_jim.c:  
arb_jim.c:  
arb_jim.c:  
arb_jim.c:   
arb_jim.c:  
arb_jim.c:  
arb_jim.c:  
arb_jim.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_jim.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_newick_2_otb.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_newick_2_otb.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_newick_2_otb.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_newick_2_otb.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_newick_2_otb.c: $(ARBHOME)/INCLUDE/cat_tree.hxx
arb_newick_2_otb.c:  
arb_newick_2_otb.c: 
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c:  
arb_newick_2_otb.c: 
arb_newick_2_otb.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_newick_2_otb.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_perf_test.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_perf_test.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_perf_test.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_perf_test.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c:  
arb_perf_test.c: 
arb_perf_test.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_perf_test.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_primer.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_primer.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_primer.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_primer.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_primer.c:  
arb_primer.c:  
arb_primer.c:  
arb_primer.c:  
arb_primer.c:  
arb_primer.c:  
arb_primer.c:   
arb_primer.c:  
arb_primer.c:  
arb_primer.c:  
arb_primer.c:  
arb_primer.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_primer.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_probe.c: $(ARBHOME)/INCLUDE/PT_com.h
arb_probe.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_probe.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_probe.c: $(ARBHOME)/INCLUDE/client.h
arb_probe.c: $(ARBHOME)/INCLUDE/servercntrl.h
arb_probe.c:  
arb_probe.c:  
arb_probe.c:  
arb_probe.c:  
arb_probe.c:  
arb_probe.c:  
arb_probe.c:   
arb_probe.c:   
arb_probe.c:  
arb_probe.c:  
arb_probe.c:  
arb_probe.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_probe.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_proto_2_xsub.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_proto_2_xsub.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c: 
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c:  
arb_proto_2_xsub.c: 
arb_proto_2_xsub.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_proto_2_xsub.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_read_tree.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_read_tree.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_read_tree.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_read_tree.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c:  
arb_read_tree.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_read_tree.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_replace.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_replace.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_replace.c:  
arb_replace.c:  
arb_replace.c:  
arb_replace.c:  
arb_replace.c:  
arb_replace.c:  
arb_replace.c:   
arb_replace.c:  
arb_replace.c:  
arb_replace.c:  
arb_replace.c: 
arb_replace.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_replace.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_swap_rnastr.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_swap_rnastr.c: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_swap_rnastr.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_swap_rnastr.c: $(ARBHOME)/INCLUDE/arbdbt.h
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c:  
arb_swap_rnastr.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_swap_rnastr.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
arb_test.c: $(ARBHOME)/INCLUDE/ad_prot.h
arb_test.c: $(ARBHOME)/INCLUDE/arbdb.h
arb_test.c:  
arb_test.c:  
arb_test.c:  
arb_test.c:   
arb_test.c:  
arb_test.c: 
arb_test.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arb_test.c: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
