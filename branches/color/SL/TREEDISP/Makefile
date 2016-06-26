# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .hxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

proto:
	../../AISC_MKPTPS/aisc_mkpt -P -w TreeCallbacks.hxx TreeCallbacks.cxx >TreeCallbacks.hxx.tmp
	../../SOURCE_TOOLS/mv_if_diff TreeCallbacks.hxx.tmp TreeCallbacks.hxx

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

irstree_display.o: TreeDisplay.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/ad_prot.h
irstree_display.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
irstree_display.o: $(ARBHOME)/INCLUDE/AliView.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/AP_TreeColors.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/arb_assert.h
irstree_display.o: $(ARBHOME)/INCLUDE/arb_core.h
irstree_display.o: $(ARBHOME)/INCLUDE/arb_error.h
irstree_display.o: $(ARBHOME)/INCLUDE/arb_msg.h
irstree_display.o: $(ARBHOME)/INCLUDE/arb_string.h
irstree_display.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/arbdb.h
irstree_display.o: $(ARBHOME)/INCLUDE/arbdb_base.h
irstree_display.o: $(ARBHOME)/INCLUDE/arbdbt.h
irstree_display.o: $(ARBHOME)/INCLUDE/arbtools.h
irstree_display.o: $(ARBHOME)/INCLUDE/attributes.h
irstree_display.o: $(ARBHOME)/INCLUDE/aw_base.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_device.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_position.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_window.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/cb.h
irstree_display.o: $(ARBHOME)/INCLUDE/cb_base.h
irstree_display.o: $(ARBHOME)/INCLUDE/cbtypes.h
irstree_display.o: $(ARBHOME)/INCLUDE/cxxforward.h
irstree_display.o: $(ARBHOME)/INCLUDE/downcast.h
irstree_display.o: $(ARBHOME)/INCLUDE/dupstr.h
irstree_display.o: $(ARBHOME)/INCLUDE/gccver.h
irstree_display.o: $(ARBHOME)/INCLUDE/nds.h
irstree_display.o: $(ARBHOME)/INCLUDE/smartptr.h
irstree_display.o: $(ARBHOME)/INCLUDE/static_assert.h
irstree_display.o: $(ARBHOME)/INCLUDE/test_global.h
irstree_display.o: $(ARBHOME)/INCLUDE/TreeNode.h
irstree_display.o: $(ARBHOME)/INCLUDE/ttypes.h

TreeCallbacks.o: TreeCallbacks.hxx
TreeCallbacks.o: TreeDisplay.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/AliView.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/attributes.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_advice.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_base.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_position.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/cb.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/cb_base.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/cbtypes.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/downcast.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/gccver.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/mode_text.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/test_global.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/TreeNode.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ttypes.h

TreeDisplay.o: ../../WINDOW/aw_common.hxx
TreeDisplay.o: TreeCallbacks.hxx
TreeDisplay.o: TreeDisplay.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_cb.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_colorset.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/AliView.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_TreeColors.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_TreeShader.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_defs.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_diff.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_strarray.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/attributes.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_base.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_position.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_question.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/cb.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/cb_base.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/cbtypes.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/downcast.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/gccver.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/item_shader.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/items.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/nds.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/test_global.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/test_unit.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/TreeNode.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ttypes.h
