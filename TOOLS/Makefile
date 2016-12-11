.SUFFIXES: .c .cxx .depend .o

TOOLS := \
	arb_2_ascii \
	arb_2_bin \
	arb_a2ps \
	arb_consensus_tree \
	arb_dnarates \
	arb_export_rates \
	arb_export_sequences \
	arb_export_tree \
	arb_flush_mem \
	arb_gene_probe \
	arb_message \
	arb_notify \
	arb_primer \
	arb_probe \
	arb_probe_match \
	arb_read_tree \
	arb_write_tree_comment \
	arb_replace \

ifeq ($(DEBUG),1)
# add all arb*test*.c[xx] files
TOOLS += $(subst .c,,$(wildcard arb*test*.c))
TOOLS += $(subst .cxx,,$(wildcard arb*test*.cxx))
else
ifeq ($(UNIT_TESTS),1)
TOOLS += arb_test
endif
endif

# ------------------------------------------------------------
# if you change lib-dependencies here,
# please update corresponding definitions in ../BINDEP/
# and in ../Makefile@TOOLSLIBDEPENDS

LIB_ARBDB               := $(LIBPATH) $(ARBDB_LIB) $(ARB_GLIB_LIBS)
LIBS_ARB_PROBE          := ../SERVERCNTRL/SERVERCNTRL.a ../PROBE_COM/client.a ../SL/REGEXPR/REGEXPR.a
LIBS_ARB_READ_TREE      := ../SL/TREE_READ/TREE_READ.a
LIBS_ARB_EXPORT_TREE    := ../SL/TREE_WRITE/TREE_WRITE.a ../XML/XML.a $(LIBS_ARB_READ_TREE)
LIBS_ARB_EXPORT_SEQ     := ../SL/SEQIO/SEQIO.a ../XML/XML.a ../SL/INSDEL/INSDEL.a ../SL/FILTER/FILTER.a
LIBS_ARB_CONSENSUS_TREE := $(LIBS_ARB_EXPORT_TREE) ../CONSENSUS_TREE/CONSENSUS_TREE.a

OBJECTS:=$(addsuffix .o,$(TOOLS))
BINARIES:=$(addprefix $(ARBHOME)/bin/,$(TOOLS))

all: $(BINARIES)

%.o: %.cxx 
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

%.o: %.c
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

$(ARBHOME)/bin/arb_probe: arb_probe.o $(LIBS_ARB_PROBE) $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $< $(EXECLIBS) $(LIBS_ARB_PROBE) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_probe_match: arb_probe_match.o $(LIBS_ARB_PROBE) $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $< $(EXECLIBS) $(LIBS_ARB_PROBE) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_read_tree: arb_read_tree.o  $(LIBS_ARB_READ_TREE)
	$(LINK_EXECUTABLE) $@ $< $(EXECLIBS) $(LIBS_ARB_READ_TREE) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_write_tree_comment: arb_write_tree_comment.o
	$(LINK_EXECUTABLE) $@ $< $(EXECLIBS) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_export_tree: arb_export_tree.o  $(LIBS_ARB_EXPORT_TREE)
	$(LINK_EXECUTABLE) $@ $< $(EXECLIBS) $(LIBS_ARB_EXPORT_TREE) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_export_sequences: arb_export_sequences.o  $(LIBS_ARB_EXPORT_SEQ)
	$(LINK_EXECUTABLE) $@ $< $(EXECLIBS) $(LIBS_ARB_EXPORT_SEQ) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_consensus_tree: arb_consensus_tree.o  $(LIBS_ARB_CONSENSUS_TREE) $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $< $(EXECLIBS) $(LIBS_ARB_CONSENSUS_TREE) $(LIB_ARBDB) 

$(ARBHOME)/bin/arb_a2ps: arb_a2ps.o $(use_ARB_main_C)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main_C) $< $(EXECLIBS) $(LIB_ARBDB)

$(ARBHOME)/bin/%: %.o $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $< $(EXECLIBS) $(LIB_ARBDB)

clean:
	rm -f $(BINARIES)
	rm -f $(OBJECTS)

proto:
	@echo "Nothing todo for proto in TOOLS"

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

arb_2_ascii.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_handlers.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/attributes.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/gccver.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_2_ascii.o: $(ARBHOME)/INCLUDE/test_global.h

arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/attributes.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/downcast.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/gccver.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_2_bin.o: $(ARBHOME)/INCLUDE/test_global.h

arb_consensus_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_defs.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_diff.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_file.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_progress.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_str.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_strarray.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/attributes.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/aw_base.hxx
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/command_output.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/CT_ctree.hxx
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/downcast.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/gccver.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/nds.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/test_global.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/test_unit.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/TreeNode.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/TreeRead.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/TreeWrite.h
arb_consensus_tree.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h

arb_dnarates.o: DNAml_rates_1_0.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/attributes.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
arb_dnarates.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/downcast.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/gccver.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_dnarates.o: $(ARBHOME)/INCLUDE/test_global.h

arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/attributes.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
arb_export_rates.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/downcast.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/gccver.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_export_rates.o: $(ARBHOME)/INCLUDE/test_global.h

arb_export_sequences.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_file.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/attributes.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/downcast.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/gccver.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/insdel.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/seqio.hxx
arb_export_sequences.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_export_sequences.o: $(ARBHOME)/INCLUDE/test_global.h

arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_handlers.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/attributes.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/aw_base.hxx
arb_export_tree.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/downcast.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/gccver.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/nds.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/test_global.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/TreeNode.h
arb_export_tree.o: $(ARBHOME)/INCLUDE/TreeWrite.h

arb_flush_mem.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_misc.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_progress.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/attributes.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/gccver.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_flush_mem.o: $(ARBHOME)/INCLUDE/test_global.h

arb_gene_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/adGene.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/attributes.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/downcast.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/gccver.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_gene_probe.o: $(ARBHOME)/INCLUDE/test_global.h

arb_message.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_message.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_message.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_message.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_message.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_message.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_message.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_message.o: $(ARBHOME)/INCLUDE/attributes.h
arb_message.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_message.o: $(ARBHOME)/INCLUDE/downcast.h
arb_message.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_message.o: $(ARBHOME)/INCLUDE/gccver.h
arb_message.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_message.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_message.o: $(ARBHOME)/INCLUDE/test_global.h

arb_notify.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_notify.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_notify.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_notify.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_notify.o: $(ARBHOME)/INCLUDE/attributes.h
arb_notify.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_notify.o: $(ARBHOME)/INCLUDE/downcast.h
arb_notify.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_notify.o: $(ARBHOME)/INCLUDE/gccver.h
arb_notify.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_notify.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_notify.o: $(ARBHOME)/INCLUDE/test_global.h

arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/attributes.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/downcast.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/gccver.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_perf_test.o: $(ARBHOME)/INCLUDE/test_global.h

arb_primer.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_primer.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_strarray.h
arb_primer.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_primer.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_primer.o: $(ARBHOME)/INCLUDE/attributes.h
arb_primer.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_primer.o: $(ARBHOME)/INCLUDE/downcast.h
arb_primer.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_primer.o: $(ARBHOME)/INCLUDE/gccver.h
arb_primer.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_primer.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_primer.o: $(ARBHOME)/INCLUDE/test_global.h

arb_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_probe.o: $(ARBHOME)/INCLUDE/aisc_global.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_defs.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_diff.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_strarray.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
arb_probe.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_probe.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_probe.o: $(ARBHOME)/INCLUDE/attributes.h
arb_probe.o: $(ARBHOME)/INCLUDE/bytestring.h
arb_probe.o: $(ARBHOME)/INCLUDE/client.h
arb_probe.o: $(ARBHOME)/INCLUDE/client_types.h
arb_probe.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_probe.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_probe.o: $(ARBHOME)/INCLUDE/gccver.h
arb_probe.o: $(ARBHOME)/INCLUDE/PT_com.h
arb_probe.o: $(ARBHOME)/INCLUDE/RegExpr.hxx
arb_probe.o: $(ARBHOME)/INCLUDE/servercntrl.h
arb_probe.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_probe.o: $(ARBHOME)/INCLUDE/test_global.h
arb_probe.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_probe_match.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/aisc_global.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/attributes.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/bytestring.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/client.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/client_types.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/gccver.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/PT_com.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_probe_match.o: $(ARBHOME)/INCLUDE/test_global.h

arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_defs.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/attributes.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/downcast.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/gccver.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/test_global.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/TreeNode.h
arb_read_tree.o: $(ARBHOME)/INCLUDE/TreeRead.h

arb_replace.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_file.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
arb_replace.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_replace.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_replace.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_replace.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_replace.o: $(ARBHOME)/INCLUDE/attributes.h
arb_replace.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_replace.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_replace.o: $(ARBHOME)/INCLUDE/gccver.h
arb_replace.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_replace.o: $(ARBHOME)/INCLUDE/test_global.h

arb_test.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_test.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_defs.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_diff.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_file.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_sleep.h
arb_test.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_test.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_test.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_test.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_test.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_test.o: $(ARBHOME)/INCLUDE/attributes.h
arb_test.o: $(ARBHOME)/INCLUDE/command_output.h
arb_test.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_test.o: $(ARBHOME)/INCLUDE/downcast.h
arb_test.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_test.o: $(ARBHOME)/INCLUDE/gccver.h
arb_test.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_test.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_test.o: $(ARBHOME)/INCLUDE/test_global.h
arb_test.o: $(ARBHOME)/INCLUDE/test_unit.h
arb_test.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h

arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arb_mem.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/attributes.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/downcast.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/gccver.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_write_tree_comment.o: $(ARBHOME)/INCLUDE/test_global.h
