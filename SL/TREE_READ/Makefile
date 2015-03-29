# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = TreeRead.o TreeTools.o
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

TreeRead.o: TreeRead.h
TreeRead.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeRead.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_defs.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_file.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_msg_fwd.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
TreeRead.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeRead.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeRead.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeRead.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeRead.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeRead.o: $(ARBHOME)/INCLUDE/attributes.h
TreeRead.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeRead.o: $(ARBHOME)/INCLUDE/downcast.h
TreeRead.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeRead.o: $(ARBHOME)/INCLUDE/gccver.h
TreeRead.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeRead.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeRead.o: $(ARBHOME)/INCLUDE/test_global.h
TreeRead.o: $(ARBHOME)/INCLUDE/test_unit.h
TreeRead.o: $(ARBHOME)/INCLUDE/TreeNode.h

TreeTools.o: TreeRead.h
TreeTools.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeTools.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeTools.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeTools.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeTools.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeTools.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeTools.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeTools.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeTools.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeTools.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeTools.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeTools.o: $(ARBHOME)/INCLUDE/attributes.h
TreeTools.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeTools.o: $(ARBHOME)/INCLUDE/downcast.h
TreeTools.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeTools.o: $(ARBHOME)/INCLUDE/gccver.h
TreeTools.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeTools.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeTools.o: $(ARBHOME)/INCLUDE/test_global.h
TreeTools.o: $(ARBHOME)/INCLUDE/TreeNode.h
