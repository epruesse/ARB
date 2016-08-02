.SUFFIXES: .o .cxx .depend

OBJS = CT_part.o CT_hash.o CT_ntree.o CT_rbtree.o CT_dtree.o CT_ctree.o CT_mem.o

$(MAIN): $(OBJS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<
clean:
	rm -f $(OBJS) *.a

DEPENDS = $(OBJS:.o=.depend)
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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

CT_ctree.o: CT_ctree.hxx
CT_ctree.o: CT_def.hxx
CT_ctree.o: CT_hash.hxx
CT_ctree.o: CT_mem.hxx
CT_ctree.o: CT_ntree.hxx
CT_ctree.o: CT_part.hxx
CT_ctree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_ctree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_core.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_error.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_mem.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_msg.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_progress.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_strarray.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_string.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arbdbt.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arbtools.h
CT_ctree.o: $(ARBHOME)/INCLUDE/attributes.h
CT_ctree.o: $(ARBHOME)/INCLUDE/cxxforward.h
CT_ctree.o: $(ARBHOME)/INCLUDE/downcast.h
CT_ctree.o: $(ARBHOME)/INCLUDE/dupstr.h
CT_ctree.o: $(ARBHOME)/INCLUDE/gccver.h
CT_ctree.o: $(ARBHOME)/INCLUDE/smartptr.h
CT_ctree.o: $(ARBHOME)/INCLUDE/static_assert.h
CT_ctree.o: $(ARBHOME)/INCLUDE/test_global.h
CT_ctree.o: $(ARBHOME)/INCLUDE/TreeNode.h

CT_dtree.o: CT_ctree.hxx
CT_dtree.o: CT_def.hxx
CT_dtree.o: CT_hash.hxx
CT_dtree.o: CT_mem.hxx
CT_dtree.o: CT_part.hxx
CT_dtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_dtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_core.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_error.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_mem.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_msg.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_progress.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_strarray.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_string.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arbdbt.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arbtools.h
CT_dtree.o: $(ARBHOME)/INCLUDE/attributes.h
CT_dtree.o: $(ARBHOME)/INCLUDE/cxxforward.h
CT_dtree.o: $(ARBHOME)/INCLUDE/downcast.h
CT_dtree.o: $(ARBHOME)/INCLUDE/dupstr.h
CT_dtree.o: $(ARBHOME)/INCLUDE/gccver.h
CT_dtree.o: $(ARBHOME)/INCLUDE/smartptr.h
CT_dtree.o: $(ARBHOME)/INCLUDE/static_assert.h
CT_dtree.o: $(ARBHOME)/INCLUDE/test_global.h
CT_dtree.o: $(ARBHOME)/INCLUDE/TreeNode.h

CT_hash.o: CT_ctree.hxx
CT_hash.o: CT_def.hxx
CT_hash.o: CT_hash.hxx
CT_hash.o: CT_mem.hxx
CT_hash.o: CT_ntree.hxx
CT_hash.o: CT_part.hxx
CT_hash.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_hash.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_core.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_error.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_mem.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_msg.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_progress.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_sort.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_strarray.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_string.h
CT_hash.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_hash.o: $(ARBHOME)/INCLUDE/arbdb_base.h
CT_hash.o: $(ARBHOME)/INCLUDE/arbdbt.h
CT_hash.o: $(ARBHOME)/INCLUDE/arbtools.h
CT_hash.o: $(ARBHOME)/INCLUDE/attributes.h
CT_hash.o: $(ARBHOME)/INCLUDE/cxxforward.h
CT_hash.o: $(ARBHOME)/INCLUDE/downcast.h
CT_hash.o: $(ARBHOME)/INCLUDE/dupstr.h
CT_hash.o: $(ARBHOME)/INCLUDE/gccver.h
CT_hash.o: $(ARBHOME)/INCLUDE/smartptr.h
CT_hash.o: $(ARBHOME)/INCLUDE/static_assert.h
CT_hash.o: $(ARBHOME)/INCLUDE/test_global.h
CT_hash.o: $(ARBHOME)/INCLUDE/TreeNode.h

CT_ntree.o: CT_def.hxx
CT_ntree.o: CT_mem.hxx
CT_ntree.o: CT_ntree.hxx
CT_ntree.o: CT_part.hxx
CT_ntree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_ntree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_core.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_error.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_mem.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_msg.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_string.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arbdbt.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arbtools.h
CT_ntree.o: $(ARBHOME)/INCLUDE/attributes.h
CT_ntree.o: $(ARBHOME)/INCLUDE/cxxforward.h
CT_ntree.o: $(ARBHOME)/INCLUDE/downcast.h
CT_ntree.o: $(ARBHOME)/INCLUDE/dupstr.h
CT_ntree.o: $(ARBHOME)/INCLUDE/gccver.h
CT_ntree.o: $(ARBHOME)/INCLUDE/smartptr.h
CT_ntree.o: $(ARBHOME)/INCLUDE/static_assert.h
CT_ntree.o: $(ARBHOME)/INCLUDE/test_global.h

CT_part.o: CT_def.hxx
CT_part.o: CT_mem.hxx
CT_part.o: CT_part.hxx
CT_part.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_part.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_core.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_error.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_mem.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_msg.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_strarray.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_string.h
CT_part.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_part.o: $(ARBHOME)/INCLUDE/arbdb_base.h
CT_part.o: $(ARBHOME)/INCLUDE/arbdbt.h
CT_part.o: $(ARBHOME)/INCLUDE/arbtools.h
CT_part.o: $(ARBHOME)/INCLUDE/attributes.h
CT_part.o: $(ARBHOME)/INCLUDE/cxxforward.h
CT_part.o: $(ARBHOME)/INCLUDE/downcast.h
CT_part.o: $(ARBHOME)/INCLUDE/dupstr.h
CT_part.o: $(ARBHOME)/INCLUDE/gccver.h
CT_part.o: $(ARBHOME)/INCLUDE/smartptr.h
CT_part.o: $(ARBHOME)/INCLUDE/static_assert.h
CT_part.o: $(ARBHOME)/INCLUDE/test_global.h
CT_part.o: $(ARBHOME)/INCLUDE/test_unit.h

CT_rbtree.o: CT_ctree.hxx
CT_rbtree.o: CT_def.hxx
CT_rbtree.o: CT_mem.hxx
CT_rbtree.o: CT_ntree.hxx
CT_rbtree.o: CT_part.hxx
CT_rbtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_core.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_error.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_mem.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_msg.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_progress.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_sort.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_strarray.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_string.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arbdbt.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arbtools.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/attributes.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/cxxforward.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/downcast.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/dupstr.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/gccver.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/smartptr.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/static_assert.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/test_global.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/TreeNode.h
