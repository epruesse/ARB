# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

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

nds.o: nds.h
nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
nds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
nds.o: $(ARBHOME)/INCLUDE/arb_assert.h
nds.o: $(ARBHOME)/INCLUDE/arb_core.h
nds.o: $(ARBHOME)/INCLUDE/arb_error.h
nds.o: $(ARBHOME)/INCLUDE/arb_mem.h
nds.o: $(ARBHOME)/INCLUDE/arb_msg.h
nds.o: $(ARBHOME)/INCLUDE/arb_msg_fwd.h
nds.o: $(ARBHOME)/INCLUDE/arb_string.h
nds.o: $(ARBHOME)/INCLUDE/arbdb.h
nds.o: $(ARBHOME)/INCLUDE/arbdb_base.h
nds.o: $(ARBHOME)/INCLUDE/arbdbt.h
nds.o: $(ARBHOME)/INCLUDE/arbtools.h
nds.o: $(ARBHOME)/INCLUDE/attributes.h
nds.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_base.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_file.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_select.hxx
nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
nds.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
nds.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
nds.o: $(ARBHOME)/INCLUDE/cb.h
nds.o: $(ARBHOME)/INCLUDE/cb_base.h
nds.o: $(ARBHOME)/INCLUDE/cbtypes.h
nds.o: $(ARBHOME)/INCLUDE/cxxforward.h
nds.o: $(ARBHOME)/INCLUDE/downcast.h
nds.o: $(ARBHOME)/INCLUDE/dupstr.h
nds.o: $(ARBHOME)/INCLUDE/gccver.h
nds.o: $(ARBHOME)/INCLUDE/item_sel_list.h
nds.o: $(ARBHOME)/INCLUDE/items.h
nds.o: $(ARBHOME)/INCLUDE/smartptr.h
nds.o: $(ARBHOME)/INCLUDE/static_assert.h
nds.o: $(ARBHOME)/INCLUDE/test_global.h
nds.o: $(ARBHOME)/INCLUDE/test_unit.h
nds.o: $(ARBHOME)/INCLUDE/TreeNode.h
nds.o: $(ARBHOME)/INCLUDE/ttypes.h
