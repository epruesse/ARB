.SUFFIXES: .c .cxx .depend .o

TOOLS := \
	arb_2_ascii \
	arb_2_bin \
	arb_a2ps \
	arb_count_chars \
	arb_dnarates \
	arb_export_rates \
	arb_export_tree \
	arb_gene_probe \
	arb_message \
	arb_notify \
	arb_primer \
	arb_probe \
	arb_proto_2_xsub \
	arb_read_tree \
	arb_replace \
	arb_swap_rnastr \

ifeq ($(DEBUG),1)
# add all arb*test*.c[xx] files
TOOLS += $(subst .c,,$(wildcard arb*test*.c))
TOOLS += $(subst .cxx,,$(wildcard arb*test*.cxx))
endif

LIB_ARBDB            := $(LIBPATH) $(ARBDB_LIB)
LIBS_ARB_PROBE       := ../SERVERCNTRL/SERVERCNTRL.a ../PROBE_COM/client.a
LIBS_ARB_READ_TREE   := ../SL/TREE_READ/TREE_READ.a
LIBS_ARB_EXPORT_TREE := ../SL/TREE_WRITE/TREE_WRITE.a ../XML/XML.a $(LIBS_ARB_READ_TREE)
LIBS_ARB_PROTO_2_XSUB:= ../SL/FILE_BUFFER/FILE_BUFFER.a

OBJECTS:=$(addsuffix .o,$(TOOLS))
BINARIES:=$(addprefix $(ARBHOME)/bin/,$(TOOLS))

all: $(BINARIES)

%.o: %.cxx 
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

%.o: %.c
	$(ACC) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

$(ARBHOME)/bin/arb_probe: arb_probe.o $(LIBS_ARB_PROBE)
	$(LINK_EXECUTABLE) $@ $< $(LIBS_ARB_PROBE) $(LIB_ARBDB) $(EXECLIBS) 

$(ARBHOME)/bin/arb_read_tree: arb_read_tree.o  $(LIBS_ARB_READ_TREE)
	$(LINK_EXECUTABLE) $@ $< $(LIBS_ARB_READ_TREE) $(LIB_ARBDB) $(EXECLIBS) 

$(ARBHOME)/bin/arb_export_tree: arb_export_tree.o  $(LIBS_ARB_EXPORT_TREE)
	$(LINK_EXECUTABLE) $@ $< $(LIBS_ARB_EXPORT_TREE) $(LIB_ARBDB) $(EXECLIBS) 

$(ARBHOME)/bin/arb_proto_2_xsub: arb_proto_2_xsub.o  $(LIBS_ARB_PROTO_2_XSUB)
	$(LINK_EXECUTABLE) $@ $< $(LIBS_ARB_PROTO_2_XSUB) $(LIB_ARBDB) $(EXECLIBS)

$(ARBHOME)/bin/%: %.o
	$(LINK_EXECUTABLE) $@ $< $(LIB_ARBDB) $(EXECLIBS)

clean:
	rm -f $(BINARIES)
	rm -f $(OBJECTS)

depends:
	$(MAKE) "DEBUG=1" depends_debug

DEPENDS = $(OBJECTS:.o=.depend)
depends_debug: $(DEPENDS) 
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

arb_2_ascii.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/attributes.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/attributes.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_count_chars.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/attributes.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_count_chars.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_dnarates.o: DNAml_rates_1_0.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/attributes.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
arb_dnarates.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/attributes.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
arb_export_rates.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/attributes.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
arb_export_tree.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
arb_export_tree.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/TreeWrite.h

arb_gene_probe.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/adGene.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/attributes.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_message.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_message.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_message.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_message.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_message.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_message.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_message.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_message.o: $(ARBHOME)/INCLUDE/attributes.h
arb_message.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_message.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_notify.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_notify.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_notify.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_notify.o: $(ARBHOME)/INCLUDE/attributes.h
arb_notify.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_notify.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/attributes.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_primer.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_primer.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_primer.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_primer.o: $(ARBHOME)/INCLUDE/attributes.h
arb_primer.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_primer.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_probe.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_probe.o: $(ARBHOME)/INCLUDE/attributes.h
arb_probe.o: $(ARBHOME)/INCLUDE/client.h
arb_probe.o: $(ARBHOME)/INCLUDE/client_types.h
arb_probe.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_probe.o: $(ARBHOME)/INCLUDE/PT_com.h
arb_probe.o: $(ARBHOME)/INCLUDE/servercntrl.h
arb_probe.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/attributes.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/FileBuffer.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/attributes.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/TreeRead.h

arb_replace.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_replace.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_replace.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_replace.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_replace.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_replace.o: $(ARBHOME)/INCLUDE/attributes.h
arb_replace.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_replace.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/attributes.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_swap_rnastr.o: $(ARBHOME)/INCLUDE/smartptr.h

arb_test.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_test.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_test.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_test.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_test.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_test.o: $(ARBHOME)/INCLUDE/attributes.h
arb_test.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_test.o: $(ARBHOME)/INCLUDE/smartptr.h
