# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = ARB_Tree.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE)

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

ARB_Tree.o: ARB_Tree.hxx
ARB_Tree.o: $(ARBHOME)/INCLUDE/ad_cb.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/AliView.hxx
ARB_Tree.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
ARB_Tree.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
ARB_Tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arb_core.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arb_error.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arb_msg.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arb_string.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arbdb.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/arbtools.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/attributes.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/cb.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/cb_base.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/cbtypes.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/cxxforward.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/downcast.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/dupstr.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/gccver.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/RootedTree.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/smartptr.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/static_assert.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/test_global.h
ARB_Tree.o: $(ARBHOME)/INCLUDE/ttypes.h
