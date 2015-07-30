# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = AP_Tree.o
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

AP_Tree.o: AP_Tree.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/ad_cb.h
AP_Tree.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
AP_Tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_Tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_Tree.o: $(ARBHOME)/INCLUDE/AliView.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arb_progress.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_Tree.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_Tree.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_Tree.o: $(ARBHOME)/INCLUDE/attributes.h
AP_Tree.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AP_Tree.o: $(ARBHOME)/INCLUDE/cb.h
AP_Tree.o: $(ARBHOME)/INCLUDE/cb_base.h
AP_Tree.o: $(ARBHOME)/INCLUDE/cbtypes.h
AP_Tree.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_Tree.o: $(ARBHOME)/INCLUDE/downcast.h
AP_Tree.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_Tree.o: $(ARBHOME)/INCLUDE/gccver.h
AP_Tree.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_Tree.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_Tree.o: $(ARBHOME)/INCLUDE/test_global.h
AP_Tree.o: $(ARBHOME)/INCLUDE/TreeNode.h
AP_Tree.o: $(ARBHOME)/INCLUDE/ttypes.h
