# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	GUI_aliview.o \
	ColumnStat.o \
	AWT_filter.o \

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

AWT_filter.o: awt_filter.hxx
AWT_filter.o: ga_local.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_cb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_core.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_defs.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_msg.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_str.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_string.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_filter.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_select.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/cb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/cb_base.h
AWT_filter.o: $(ARBHOME)/INCLUDE/cbtypes.h
AWT_filter.o: $(ARBHOME)/INCLUDE/cxxforward.h
AWT_filter.o: $(ARBHOME)/INCLUDE/downcast.h
AWT_filter.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_filter.o: $(ARBHOME)/INCLUDE/gccver.h
AWT_filter.o: $(ARBHOME)/INCLUDE/smartptr.h
AWT_filter.o: $(ARBHOME)/INCLUDE/static_assert.h
AWT_filter.o: $(ARBHOME)/INCLUDE/test_global.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ttypes.h

ColumnStat.o: ColumnStat.hxx
ColumnStat.o: ga_local.h
ColumnStat.o: $(ARBHOME)/INCLUDE/ad_prot.h
ColumnStat.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ColumnStat.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_assert.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_core.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_defs.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_error.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_msg.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_string.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbdb.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbdbt.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbtools.h
ColumnStat.o: $(ARBHOME)/INCLUDE/attributes.h
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_action.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_base.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_common.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_element.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_help.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_position.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_select.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/cb.h
ColumnStat.o: $(ARBHOME)/INCLUDE/cb_base.h
ColumnStat.o: $(ARBHOME)/INCLUDE/cbtypes.h
ColumnStat.o: $(ARBHOME)/INCLUDE/cxxforward.h
ColumnStat.o: $(ARBHOME)/INCLUDE/downcast.h
ColumnStat.o: $(ARBHOME)/INCLUDE/dupstr.h
ColumnStat.o: $(ARBHOME)/INCLUDE/gccver.h
ColumnStat.o: $(ARBHOME)/INCLUDE/smartptr.h
ColumnStat.o: $(ARBHOME)/INCLUDE/static_assert.h
ColumnStat.o: $(ARBHOME)/INCLUDE/test_global.h
ColumnStat.o: $(ARBHOME)/INCLUDE/ttypes.h

GUI_aliview.o: awt_filter.hxx
GUI_aliview.o: ColumnStat.hxx
GUI_aliview.o: ga_local.h
GUI_aliview.o: gui_aliview.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/AliView.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_assert.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_core.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_msg.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbtools.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/attributes.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/cxxforward.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/dupstr.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/gccver.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/test_global.h
