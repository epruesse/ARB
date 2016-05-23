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

AdminGui.o: AliAdmin.h
AdminGui.o: $(ARBHOME)/INCLUDE/ad_prot.h
AdminGui.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_assert.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_core.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_defs.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_error.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_msg.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_string.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdb.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdbt.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbtools.h
AdminGui.o: $(ARBHOME)/INCLUDE/attributes.h
AdminGui.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_question.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/cb.h
AdminGui.o: $(ARBHOME)/INCLUDE/cb_base.h
AdminGui.o: $(ARBHOME)/INCLUDE/cbtypes.h
AdminGui.o: $(ARBHOME)/INCLUDE/cxxforward.h
AdminGui.o: $(ARBHOME)/INCLUDE/downcast.h
AdminGui.o: $(ARBHOME)/INCLUDE/dupstr.h
AdminGui.o: $(ARBHOME)/INCLUDE/gccver.h
AdminGui.o: $(ARBHOME)/INCLUDE/insdel.h
AdminGui.o: $(ARBHOME)/INCLUDE/smartptr.h
AdminGui.o: $(ARBHOME)/INCLUDE/static_assert.h
AdminGui.o: $(ARBHOME)/INCLUDE/test_global.h
AdminGui.o: $(ARBHOME)/INCLUDE/ttypes.h

AliAdmin.o: AliAdmin.h
AliAdmin.o: $(ARBHOME)/INCLUDE/ad_prot.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_assert.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_core.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_defs.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_error.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_msg.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_string.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbdb.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbtools.h
AliAdmin.o: $(ARBHOME)/INCLUDE/attributes.h
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/cb.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cb_base.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cbtypes.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cxxforward.h
AliAdmin.o: $(ARBHOME)/INCLUDE/downcast.h
AliAdmin.o: $(ARBHOME)/INCLUDE/dupstr.h
AliAdmin.o: $(ARBHOME)/INCLUDE/gccver.h
AliAdmin.o: $(ARBHOME)/INCLUDE/smartptr.h
AliAdmin.o: $(ARBHOME)/INCLUDE/static_assert.h
AliAdmin.o: $(ARBHOME)/INCLUDE/test_global.h
AliAdmin.o: $(ARBHOME)/INCLUDE/test_unit.h
AliAdmin.o: $(ARBHOME)/INCLUDE/ttypes.h
