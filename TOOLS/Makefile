
LIB = $(LIBPATH) -lARBDO $(SYSLIBS)

#LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS) -lCrun
LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS)

.SUFFIXES: .o .c .cxx .depend

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

DEPENDS = $(OBJECT:.o=.depend)
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

arb_primer.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_primer.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_primer.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/ad_prot.h

arb_newick_2_otb.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_newick_2_otb.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_newick_2_otb.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_newick_2_otb.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_newick_2_otb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_newick_2_otb.o: $(ARBHOME)/INCLUDE/cat_tree.hxx

arb_dnarates.o: DNAml_rates_1_0.h $(ARBHOME)/INCLUDE/aw_awars.hxx
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_replace.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_replace.o: $(ARBHOME)/INCLUDE/ad_prot.h

arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/ad_prot.h

arb_probe.o: $(ARBHOME)/INCLUDE/PT_com.h
arb_probe.o: $(ARBHOME)/INCLUDE/client.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_probe.o: $(ARBHOME)/INCLUDE/servercntrl.h

arb_test.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_test.o: $(ARBHOME)/INCLUDE/ad_prot.h

arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/aw_awars.hxx

arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_count_chars.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/ad_t_prot.h

arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
