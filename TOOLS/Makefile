
LIB = $(LIBPATH) -lARBDO $(SYSLIBS)

#LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS) -lCrun
LIB2 = $(LIBPATH) -lARBDB $(SYSLIBS) $(CCPLIBS)

.SUFFIXES: .c .cxx .depend

TOOLS = \
    arb_a2ps arb_primer arb_swap_rnastr arb_2_ascii \
	arb_newick_2_otb arb_dnarates \
	arb_2_bin arb_replace arb_read_tree \
	arb_proto_2_xsub arb_probe arb_test arb_export_rates arb_export_tree arb_count_chars \
	arb_perf_test arb_message arb_gene_probe arb_notify

TARGETS = $(addprefix $(ARBHOME)/bin/,$(TOOLS))

#all: $(OBJECT:%.o=$(ARBHOME)/bin/%)
all: $(TARGETS)

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

#DEPENDS = $(OBJECT:.o=.depend)
DEPENDS = $(addsuffix .depend,$(TOOLS))

depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- -p$(ARBHOME)/bin/ -o "" $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- -p$(ARBHOME)/bin/ -o "" $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

$(ARBHOME)/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_2_ascii: $(ARBHOME)/INCLUDE/arbdb.h

$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_2_bin: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_count_chars: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_dnarates: $(ARBHOME)/INCLUDE/aw_awars.hxx
$(ARBHOME)/bin/arb_dnarates: DNAml_rates_1_0.h

$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_export_rates: $(ARBHOME)/INCLUDE/aw_awars.hxx

$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_export_tree: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_gene_probe: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_gene_probe: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_gene_probe: $(ARBHOME)/INCLUDE/adGene.h
$(ARBHOME)/bin/arb_gene_probe: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_gene_probe: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_gene_probe: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_message: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_message: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_message: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_message: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_message: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/arbdbt.h
$(ARBHOME)/bin/arb_newick_2_otb: $(ARBHOME)/INCLUDE/cat_tree.hxx

$(ARBHOME)/bin/arb_notify: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_notify: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_notify: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_notify: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_notify: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_perf_test: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_primer: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/client.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/PT_com.h
$(ARBHOME)/bin/arb_probe: $(ARBHOME)/INCLUDE/servercntrl.h

$(ARBHOME)/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_proto_2_xsub: $(ARBHOME)/INCLUDE/arbdb.h

$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_read_tree: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_replace: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_replace: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_replace: $(ARBHOME)/INCLUDE/arbdb.h

$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/ad_t_prot.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/arb_swap_rnastr: $(ARBHOME)/INCLUDE/arbdbt.h

$(ARBHOME)/bin/arb_test: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/arb_test: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/arb_test: $(ARBHOME)/INCLUDE/arbdb.h
