# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

AP_seq_dna.o: AP_seq_dna.hxx
AP_seq_dna.o: AP_sequence.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/attributes.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/downcast.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/gccver.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/test_global.h
AP_seq_dna.o: $(ARBHOME)/INCLUDE/TreeNode.h

AP_seq_protein.o: AP_seq_protein.hxx
AP_seq_protein.o: AP_sequence.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_str.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/attributes.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/downcast.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/gccver.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/test_global.h
AP_seq_protein.o: $(ARBHOME)/INCLUDE/TreeNode.h

AP_seq_simple_pro.o: AP_seq_simple_pro.hxx
AP_seq_simple_pro.o: AP_sequence.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/attributes.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/downcast.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/gccver.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/test_global.h
AP_seq_simple_pro.o: $(ARBHOME)/INCLUDE/TreeNode.h

AP_sequence.o: AP_sequence.hxx
AP_sequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_sequence.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_sequence.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_sequence.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_sequence.o: $(ARBHOME)/INCLUDE/attributes.h
AP_sequence.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_sequence.o: $(ARBHOME)/INCLUDE/downcast.h
AP_sequence.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_sequence.o: $(ARBHOME)/INCLUDE/gccver.h
AP_sequence.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_sequence.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_sequence.o: $(ARBHOME)/INCLUDE/test_global.h
