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

changekey.o: item_sel_list.h
changekey.o: items.h
changekey.o: $(ARBHOME)/INCLUDE/ad_prot.h
changekey.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
changekey.o: $(ARBHOME)/INCLUDE/arb_assert.h
changekey.o: $(ARBHOME)/INCLUDE/arb_core.h
changekey.o: $(ARBHOME)/INCLUDE/arb_defs.h
changekey.o: $(ARBHOME)/INCLUDE/arb_error.h
changekey.o: $(ARBHOME)/INCLUDE/arb_msg.h
changekey.o: $(ARBHOME)/INCLUDE/arb_strarray.h
changekey.o: $(ARBHOME)/INCLUDE/arb_string.h
changekey.o: $(ARBHOME)/INCLUDE/arbdb.h
changekey.o: $(ARBHOME)/INCLUDE/arbdb_base.h
changekey.o: $(ARBHOME)/INCLUDE/arbdbt.h
changekey.o: $(ARBHOME)/INCLUDE/arbtools.h
changekey.o: $(ARBHOME)/INCLUDE/attributes.h
changekey.o: $(ARBHOME)/INCLUDE/aw_action.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_base.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_common.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_element.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_help.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_position.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_root.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_select.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_window.hxx
changekey.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
changekey.o: $(ARBHOME)/INCLUDE/cb.h
changekey.o: $(ARBHOME)/INCLUDE/cb_base.h
changekey.o: $(ARBHOME)/INCLUDE/cbtypes.h
changekey.o: $(ARBHOME)/INCLUDE/cxxforward.h
changekey.o: $(ARBHOME)/INCLUDE/downcast.h
changekey.o: $(ARBHOME)/INCLUDE/dupstr.h
changekey.o: $(ARBHOME)/INCLUDE/gccver.h
changekey.o: $(ARBHOME)/INCLUDE/smartptr.h
changekey.o: $(ARBHOME)/INCLUDE/static_assert.h
changekey.o: $(ARBHOME)/INCLUDE/test_global.h
changekey.o: $(ARBHOME)/INCLUDE/ttypes.h

item_sel_list.o: item_sel_list.h
item_sel_list.o: items.h
item_sel_list.o: $(ARBHOME)/INCLUDE/ad_cb.h
item_sel_list.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
item_sel_list.o: $(ARBHOME)/INCLUDE/ad_prot.h
item_sel_list.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_assert.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_core.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_defs.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_error.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_msg.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arb_string.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arbdb.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arbdb_base.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arbdbt.h
item_sel_list.o: $(ARBHOME)/INCLUDE/arbtools.h
item_sel_list.o: $(ARBHOME)/INCLUDE/attributes.h
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_action.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_base.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_common.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_element.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_help.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_position.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_root.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_select.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_window.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
item_sel_list.o: $(ARBHOME)/INCLUDE/cb.h
item_sel_list.o: $(ARBHOME)/INCLUDE/cb_base.h
item_sel_list.o: $(ARBHOME)/INCLUDE/cbtypes.h
item_sel_list.o: $(ARBHOME)/INCLUDE/cxxforward.h
item_sel_list.o: $(ARBHOME)/INCLUDE/downcast.h
item_sel_list.o: $(ARBHOME)/INCLUDE/dupstr.h
item_sel_list.o: $(ARBHOME)/INCLUDE/gccver.h
item_sel_list.o: $(ARBHOME)/INCLUDE/smartptr.h
item_sel_list.o: $(ARBHOME)/INCLUDE/static_assert.h
item_sel_list.o: $(ARBHOME)/INCLUDE/test_global.h
item_sel_list.o: $(ARBHOME)/INCLUDE/test_unit.h
item_sel_list.o: $(ARBHOME)/INCLUDE/ttypes.h

itemtools.o: items.h
itemtools.o: $(ARBHOME)/INCLUDE/arb_assert.h
itemtools.o: $(ARBHOME)/INCLUDE/arb_core.h
itemtools.o: $(ARBHOME)/INCLUDE/arb_defs.h
itemtools.o: $(ARBHOME)/INCLUDE/arb_error.h
itemtools.o: $(ARBHOME)/INCLUDE/arb_msg.h
itemtools.o: $(ARBHOME)/INCLUDE/arbdb_base.h
itemtools.o: $(ARBHOME)/INCLUDE/arbtools.h
itemtools.o: $(ARBHOME)/INCLUDE/attributes.h
itemtools.o: $(ARBHOME)/INCLUDE/aw_action.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_base.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_common.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_element.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_help.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_position.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_root.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_window.hxx
itemtools.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
itemtools.o: $(ARBHOME)/INCLUDE/cb.h
itemtools.o: $(ARBHOME)/INCLUDE/cb_base.h
itemtools.o: $(ARBHOME)/INCLUDE/cbtypes.h
itemtools.o: $(ARBHOME)/INCLUDE/cxxforward.h
itemtools.o: $(ARBHOME)/INCLUDE/downcast.h
itemtools.o: $(ARBHOME)/INCLUDE/dupstr.h
itemtools.o: $(ARBHOME)/INCLUDE/gccver.h
itemtools.o: $(ARBHOME)/INCLUDE/smartptr.h
itemtools.o: $(ARBHOME)/INCLUDE/static_assert.h
itemtools.o: $(ARBHOME)/INCLUDE/test_global.h
itemtools.o: $(ARBHOME)/INCLUDE/ttypes.h

species.o: item_sel_list.h
species.o: items.h
species.o: $(ARBHOME)/INCLUDE/ad_prot.h
species.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
species.o: $(ARBHOME)/INCLUDE/arb_assert.h
species.o: $(ARBHOME)/INCLUDE/arb_core.h
species.o: $(ARBHOME)/INCLUDE/arb_defs.h
species.o: $(ARBHOME)/INCLUDE/arb_error.h
species.o: $(ARBHOME)/INCLUDE/arb_msg.h
species.o: $(ARBHOME)/INCLUDE/arb_string.h
species.o: $(ARBHOME)/INCLUDE/arbdb.h
species.o: $(ARBHOME)/INCLUDE/arbdb_base.h
species.o: $(ARBHOME)/INCLUDE/arbdbt.h
species.o: $(ARBHOME)/INCLUDE/arbtools.h
species.o: $(ARBHOME)/INCLUDE/attributes.h
species.o: $(ARBHOME)/INCLUDE/aw_action.hxx
species.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
species.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
species.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
species.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
species.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
species.o: $(ARBHOME)/INCLUDE/aw_base.hxx
species.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
species.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
species.o: $(ARBHOME)/INCLUDE/aw_common.hxx
species.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
species.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
species.o: $(ARBHOME)/INCLUDE/aw_element.hxx
species.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
species.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
species.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
species.o: $(ARBHOME)/INCLUDE/aw_help.hxx
species.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
species.o: $(ARBHOME)/INCLUDE/aw_position.hxx
species.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
species.o: $(ARBHOME)/INCLUDE/aw_root.hxx
species.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
species.o: $(ARBHOME)/INCLUDE/aw_select.hxx
species.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
species.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
species.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
species.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
species.o: $(ARBHOME)/INCLUDE/cb.h
species.o: $(ARBHOME)/INCLUDE/cb_base.h
species.o: $(ARBHOME)/INCLUDE/cbtypes.h
species.o: $(ARBHOME)/INCLUDE/cxxforward.h
species.o: $(ARBHOME)/INCLUDE/downcast.h
species.o: $(ARBHOME)/INCLUDE/dupstr.h
species.o: $(ARBHOME)/INCLUDE/gccver.h
species.o: $(ARBHOME)/INCLUDE/smartptr.h
species.o: $(ARBHOME)/INCLUDE/static_assert.h
species.o: $(ARBHOME)/INCLUDE/test_global.h
species.o: $(ARBHOME)/INCLUDE/ttypes.h
