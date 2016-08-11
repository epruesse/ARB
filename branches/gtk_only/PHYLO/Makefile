# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

PH_data.o: PH_filter.hxx
PH_data.o: phylo.hxx
PH_data.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_data.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_core.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_error.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_mem.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_msg.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_string.h
PH_data.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_data.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PH_data.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_data.o: $(ARBHOME)/INCLUDE/arbtools.h
PH_data.o: $(ARBHOME)/INCLUDE/attributes.h
PH_data.o: $(ARBHOME)/INCLUDE/aw_base.hxx
PH_data.o: $(ARBHOME)/INCLUDE/cxxforward.h
PH_data.o: $(ARBHOME)/INCLUDE/downcast.h
PH_data.o: $(ARBHOME)/INCLUDE/dupstr.h
PH_data.o: $(ARBHOME)/INCLUDE/gccver.h
PH_data.o: $(ARBHOME)/INCLUDE/smartptr.h
PH_data.o: $(ARBHOME)/INCLUDE/static_assert.h
PH_data.o: $(ARBHOME)/INCLUDE/test_global.h

PH_display.o: PH_display.hxx
PH_display.o: PH_filter.hxx
PH_display.o: phwin.hxx
PH_display.o: phylo.hxx
PH_display.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_core.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_defs.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_error.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_mem.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_msg.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_string.h
PH_display.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_display.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PH_display.o: $(ARBHOME)/INCLUDE/arbtools.h
PH_display.o: $(ARBHOME)/INCLUDE/attributes.h
PH_display.o: $(ARBHOME)/INCLUDE/aw_action.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_base.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_common.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_element.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_help.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_position.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
PH_display.o: $(ARBHOME)/INCLUDE/cb.h
PH_display.o: $(ARBHOME)/INCLUDE/cb_base.h
PH_display.o: $(ARBHOME)/INCLUDE/cbtypes.h
PH_display.o: $(ARBHOME)/INCLUDE/cxxforward.h
PH_display.o: $(ARBHOME)/INCLUDE/downcast.h
PH_display.o: $(ARBHOME)/INCLUDE/dupstr.h
PH_display.o: $(ARBHOME)/INCLUDE/gccver.h
PH_display.o: $(ARBHOME)/INCLUDE/smartptr.h
PH_display.o: $(ARBHOME)/INCLUDE/static_assert.h
PH_display.o: $(ARBHOME)/INCLUDE/test_global.h
PH_display.o: $(ARBHOME)/INCLUDE/ttypes.h

PH_filt.o: PH_display.hxx
PH_filt.o: PH_filter.hxx
PH_filt.o: phwin.hxx
PH_filt.o: phylo.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_filt.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_core.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_defs.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_error.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_mem.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_msg.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_progress.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_string.h
PH_filt.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_filt.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PH_filt.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_filt.o: $(ARBHOME)/INCLUDE/arbtools.h
PH_filt.o: $(ARBHOME)/INCLUDE/attributes.h
PH_filt.o: $(ARBHOME)/INCLUDE/aw_action.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_base.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_common.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_element.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_help.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_position.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/cb.h
PH_filt.o: $(ARBHOME)/INCLUDE/cb_base.h
PH_filt.o: $(ARBHOME)/INCLUDE/cbtypes.h
PH_filt.o: $(ARBHOME)/INCLUDE/cxxforward.h
PH_filt.o: $(ARBHOME)/INCLUDE/downcast.h
PH_filt.o: $(ARBHOME)/INCLUDE/dupstr.h
PH_filt.o: $(ARBHOME)/INCLUDE/gccver.h
PH_filt.o: $(ARBHOME)/INCLUDE/smartptr.h
PH_filt.o: $(ARBHOME)/INCLUDE/static_assert.h
PH_filt.o: $(ARBHOME)/INCLUDE/test_global.h
PH_filt.o: $(ARBHOME)/INCLUDE/ttypes.h

PH_main.o: PH_display.hxx
PH_main.o: PH_filter.hxx
PH_main.o: phwin.hxx
PH_main.o: phylo.hxx
PH_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_core.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_defs.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_error.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_mem.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_strarray.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_string.h
PH_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PH_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_main.o: $(ARBHOME)/INCLUDE/arbtools.h
PH_main.o: $(ARBHOME)/INCLUDE/attributes.h
PH_main.o: $(ARBHOME)/INCLUDE/aw_action.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_base.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_common.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_element.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_help.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_position.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_question.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
PH_main.o: $(ARBHOME)/INCLUDE/cb.h
PH_main.o: $(ARBHOME)/INCLUDE/cb_base.h
PH_main.o: $(ARBHOME)/INCLUDE/cbtypes.h
PH_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
PH_main.o: $(ARBHOME)/INCLUDE/downcast.h
PH_main.o: $(ARBHOME)/INCLUDE/dupstr.h
PH_main.o: $(ARBHOME)/INCLUDE/gccver.h
PH_main.o: $(ARBHOME)/INCLUDE/macros.hxx
PH_main.o: $(ARBHOME)/INCLUDE/smartptr.h
PH_main.o: $(ARBHOME)/INCLUDE/static_assert.h
PH_main.o: $(ARBHOME)/INCLUDE/test_global.h
PH_main.o: $(ARBHOME)/INCLUDE/ttypes.h

PH_root.o: PH_filter.hxx
PH_root.o: phylo.hxx
PH_root.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_root.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_core.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_error.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_mem.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_msg.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_string.h
PH_root.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_root.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PH_root.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_root.o: $(ARBHOME)/INCLUDE/arbtools.h
PH_root.o: $(ARBHOME)/INCLUDE/attributes.h
PH_root.o: $(ARBHOME)/INCLUDE/aw_base.hxx
PH_root.o: $(ARBHOME)/INCLUDE/cxxforward.h
PH_root.o: $(ARBHOME)/INCLUDE/downcast.h
PH_root.o: $(ARBHOME)/INCLUDE/dupstr.h
PH_root.o: $(ARBHOME)/INCLUDE/gccver.h
PH_root.o: $(ARBHOME)/INCLUDE/smartptr.h
PH_root.o: $(ARBHOME)/INCLUDE/static_assert.h
PH_root.o: $(ARBHOME)/INCLUDE/test_global.h
