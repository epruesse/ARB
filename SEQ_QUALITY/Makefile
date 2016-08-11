# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = \
	SQ_GroupData.o \
	SQ_ambiguities.o \
	SQ_functions.o \
	SQ_main.o \

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

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

SQ_ambiguities.o: SQ_ambiguities.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/ad_prot.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arb_assert.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arb_core.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arb_error.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arb_mem.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arb_msg.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arb_string.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arbdb.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arbdb_base.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/arbtools.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/attributes.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/cxxforward.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/dupstr.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/gccver.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/smartptr.h
SQ_ambiguities.o: $(ARBHOME)/INCLUDE/test_global.h

SQ_functions.o: SQ_ambiguities.h
SQ_functions.o: SQ_functions.h
SQ_functions.o: SQ_GroupData.h
SQ_functions.o: SQ_helix.h
SQ_functions.o: SQ_physical_layout.h
SQ_functions.o: $(ARBHOME)/INCLUDE/ad_prot.h
SQ_functions.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SQ_functions.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_assert.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_core.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_defs.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_error.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_mem.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_msg.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_progress.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arb_string.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arbdb.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arbdb_base.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arbdbt.h
SQ_functions.o: $(ARBHOME)/INCLUDE/arbtools.h
SQ_functions.o: $(ARBHOME)/INCLUDE/attributes.h
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_action.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_base.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_common.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_element.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_help.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
SQ_functions.o: $(ARBHOME)/INCLUDE/cb.h
SQ_functions.o: $(ARBHOME)/INCLUDE/cb_base.h
SQ_functions.o: $(ARBHOME)/INCLUDE/cbtypes.h
SQ_functions.o: $(ARBHOME)/INCLUDE/cxxforward.h
SQ_functions.o: $(ARBHOME)/INCLUDE/downcast.h
SQ_functions.o: $(ARBHOME)/INCLUDE/dupstr.h
SQ_functions.o: $(ARBHOME)/INCLUDE/gccver.h
SQ_functions.o: $(ARBHOME)/INCLUDE/smartptr.h
SQ_functions.o: $(ARBHOME)/INCLUDE/static_assert.h
SQ_functions.o: $(ARBHOME)/INCLUDE/test_global.h
SQ_functions.o: $(ARBHOME)/INCLUDE/TreeNode.h
SQ_functions.o: $(ARBHOME)/INCLUDE/ttypes.h

SQ_GroupData.o: SQ_GroupData.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/arb_assert.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/arb_core.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/attributes.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/cxxforward.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/dupstr.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/gccver.h
SQ_GroupData.o: $(ARBHOME)/INCLUDE/test_global.h

SQ_main.o: seq_quality.h
SQ_main.o: SQ_functions.h
SQ_main.o: SQ_GroupData.h
SQ_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
SQ_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_core.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_defs.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_error.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_mem.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_progress.h
SQ_main.o: $(ARBHOME)/INCLUDE/arb_string.h
SQ_main.o: $(ARBHOME)/INCLUDE/arbdb.h
SQ_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
SQ_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
SQ_main.o: $(ARBHOME)/INCLUDE/arbtools.h
SQ_main.o: $(ARBHOME)/INCLUDE/attributes.h
SQ_main.o: $(ARBHOME)/INCLUDE/aw_action.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_base.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_common.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_element.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_help.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/awt_filter.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
SQ_main.o: $(ARBHOME)/INCLUDE/cb.h
SQ_main.o: $(ARBHOME)/INCLUDE/cb_base.h
SQ_main.o: $(ARBHOME)/INCLUDE/cbtypes.h
SQ_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
SQ_main.o: $(ARBHOME)/INCLUDE/downcast.h
SQ_main.o: $(ARBHOME)/INCLUDE/dupstr.h
SQ_main.o: $(ARBHOME)/INCLUDE/gccver.h
SQ_main.o: $(ARBHOME)/INCLUDE/smartptr.h
SQ_main.o: $(ARBHOME)/INCLUDE/static_assert.h
SQ_main.o: $(ARBHOME)/INCLUDE/test_global.h
SQ_main.o: $(ARBHOME)/INCLUDE/TreeNode.h
SQ_main.o: $(ARBHOME)/INCLUDE/ttypes.h
