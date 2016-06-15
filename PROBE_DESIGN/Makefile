# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend
CPP_OBJECTS = probe_design.o SaiProbeVisualization.o probe_match_parser.o probe_collection.o
$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $(@:.o=.cxx)  $(CXX_INCLUDES) $(XML_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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

probe_collection.o: probe_collection.hxx
probe_collection.o: $(ARBHOME)/INCLUDE/ad_prot.h
probe_collection.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
probe_collection.o: $(ARBHOME)/INCLUDE/arb_assert.h
probe_collection.o: $(ARBHOME)/INCLUDE/arb_core.h
probe_collection.o: $(ARBHOME)/INCLUDE/arb_error.h
probe_collection.o: $(ARBHOME)/INCLUDE/arb_msg.h
probe_collection.o: $(ARBHOME)/INCLUDE/arb_string.h
probe_collection.o: $(ARBHOME)/INCLUDE/arbdb.h
probe_collection.o: $(ARBHOME)/INCLUDE/arbdb_base.h
probe_collection.o: $(ARBHOME)/INCLUDE/arbdbt.h
probe_collection.o: $(ARBHOME)/INCLUDE/arbtools.h
probe_collection.o: $(ARBHOME)/INCLUDE/attributes.h
probe_collection.o: $(ARBHOME)/INCLUDE/cxxforward.h
probe_collection.o: $(ARBHOME)/INCLUDE/downcast.h
probe_collection.o: $(ARBHOME)/INCLUDE/dupstr.h
probe_collection.o: $(ARBHOME)/INCLUDE/gccver.h
probe_collection.o: $(ARBHOME)/INCLUDE/smartptr.h
probe_collection.o: $(ARBHOME)/INCLUDE/static_assert.h
probe_collection.o: $(ARBHOME)/INCLUDE/test_global.h

probe_design.o: probe_collection.hxx
probe_design.o: probe_gui.hxx
probe_design.o: probe_local.hxx
probe_design.o: probe_match_parser.hxx
probe_design.o: SaiProbeVisualization.hxx
probe_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
probe_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
probe_design.o: $(ARBHOME)/INCLUDE/adGene.h
probe_design.o: $(ARBHOME)/INCLUDE/aisc_global.h
probe_design.o: $(ARBHOME)/INCLUDE/AliView.hxx
probe_design.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
probe_design.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
probe_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_core.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_defs.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_error.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_file.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_misc.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_msg.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_progress.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_strarray.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_string.h
probe_design.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
probe_design.o: $(ARBHOME)/INCLUDE/arbdb.h
probe_design.o: $(ARBHOME)/INCLUDE/arbdb_base.h
probe_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
probe_design.o: $(ARBHOME)/INCLUDE/arbtools.h
probe_design.o: $(ARBHOME)/INCLUDE/attributes.h
probe_design.o: $(ARBHOME)/INCLUDE/aw_action.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_base.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_common.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_element.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_help.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_position.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_question.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_root.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_select.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_window.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_misc.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
probe_design.o: $(ARBHOME)/INCLUDE/bytestring.h
probe_design.o: $(ARBHOME)/INCLUDE/cb.h
probe_design.o: $(ARBHOME)/INCLUDE/cb_base.h
probe_design.o: $(ARBHOME)/INCLUDE/cbtypes.h
probe_design.o: $(ARBHOME)/INCLUDE/client.h
probe_design.o: $(ARBHOME)/INCLUDE/client_types.h
probe_design.o: $(ARBHOME)/INCLUDE/cxxforward.h
probe_design.o: $(ARBHOME)/INCLUDE/downcast.h
probe_design.o: $(ARBHOME)/INCLUDE/dupstr.h
probe_design.o: $(ARBHOME)/INCLUDE/gccver.h
probe_design.o: $(ARBHOME)/INCLUDE/GEN.hxx
probe_design.o: $(ARBHOME)/INCLUDE/items.h
probe_design.o: $(ARBHOME)/INCLUDE/iupac.h
probe_design.o: $(ARBHOME)/INCLUDE/PT_com.h
probe_design.o: $(ARBHOME)/INCLUDE/PT_server.h
probe_design.o: $(ARBHOME)/INCLUDE/rootAsWin.h
probe_design.o: $(ARBHOME)/INCLUDE/servercntrl.h
probe_design.o: $(ARBHOME)/INCLUDE/smartptr.h
probe_design.o: $(ARBHOME)/INCLUDE/static_assert.h
probe_design.o: $(ARBHOME)/INCLUDE/test_global.h
probe_design.o: $(ARBHOME)/INCLUDE/TreeCallbacks.hxx
probe_design.o: $(ARBHOME)/INCLUDE/TreeDisplay.hxx
probe_design.o: $(ARBHOME)/INCLUDE/TreeNode.h
probe_design.o: $(ARBHOME)/INCLUDE/ttypes.h

probe_match_parser.o: probe_match_parser.hxx
probe_match_parser.o: $(ARBHOME)/INCLUDE/ad_prot.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arb_assert.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arb_core.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arb_defs.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arb_error.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arb_msg.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arb_string.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arbdb.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arbdb_base.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arbdbt.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/arbtools.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/attributes.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/cxxforward.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/downcast.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/dupstr.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/gccver.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/smartptr.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/static_assert.h
probe_match_parser.o: $(ARBHOME)/INCLUDE/test_global.h

SaiProbeVisualization.o: probe_match_parser.hxx
SaiProbeVisualization.o: SaiProbeVisualization.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/ad_prot.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_assert.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_core.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_defs.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_error.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_msg.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_string.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arbdb.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arbdb_base.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arbdbt.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arbtools.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/attributes.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_action.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_base.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_common.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_element.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_help.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_select.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/cb.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/cb_base.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/cbtypes.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/cxxforward.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/downcast.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/dupstr.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/gccver.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/item_sel_list.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/items.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/nds.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/smartptr.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/static_assert.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/test_global.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/ttypes.h
