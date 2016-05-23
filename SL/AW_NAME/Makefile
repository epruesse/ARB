# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS =
CPP_OBJECTS = \
	AW_rename.o \
	AW_names_admin.o \

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

AW_names_admin.o: AW_rename.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_defs.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_file.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_msg.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_string.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/attributes.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_advice.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_question.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/cb.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/cb_base.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/cbtypes.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/cxxforward.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/downcast.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/gccver.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/static_assert.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/test_global.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/ttypes.h

AW_rename.o: AW_rename.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/aisc_global.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_defs.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_msg.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_progress.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_string.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_rename.o: $(ARBHOME)/INCLUDE/attributes.h
AW_rename.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_question.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/bytestring.h
AW_rename.o: $(ARBHOME)/INCLUDE/cb.h
AW_rename.o: $(ARBHOME)/INCLUDE/cb_base.h
AW_rename.o: $(ARBHOME)/INCLUDE/cbtypes.h
AW_rename.o: $(ARBHOME)/INCLUDE/client.h
AW_rename.o: $(ARBHOME)/INCLUDE/client_types.h
AW_rename.o: $(ARBHOME)/INCLUDE/cxxforward.h
AW_rename.o: $(ARBHOME)/INCLUDE/downcast.h
AW_rename.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_rename.o: $(ARBHOME)/INCLUDE/gccver.h
AW_rename.o: $(ARBHOME)/INCLUDE/names_client.h
AW_rename.o: $(ARBHOME)/INCLUDE/servercntrl.h
AW_rename.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_rename.o: $(ARBHOME)/INCLUDE/static_assert.h
AW_rename.o: $(ARBHOME)/INCLUDE/test_global.h
AW_rename.o: $(ARBHOME)/INCLUDE/ttypes.h
