# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

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

macro_gui.o: macro_gui.hxx
macro_gui.o: macros.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/ad_prot.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_assert.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_core.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_defs.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_error.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_file.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_msg.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_string.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbdb.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbdb_base.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbtools.h
macro_gui.o: $(ARBHOME)/INCLUDE/attributes.h
macro_gui.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_at.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_base.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_button.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_common.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_common_gtk.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_device.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_file.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_help.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_position.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_root.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_root_gtk.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_window.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/cb.h
macro_gui.o: $(ARBHOME)/INCLUDE/cxxforward.h
macro_gui.o: $(ARBHOME)/INCLUDE/downcast.h
macro_gui.o: $(ARBHOME)/INCLUDE/dupstr.h
macro_gui.o: $(ARBHOME)/INCLUDE/gccver.h
macro_gui.o: $(ARBHOME)/INCLUDE/smartptr.h
macro_gui.o: $(ARBHOME)/INCLUDE/static_assert.h
macro_gui.o: $(ARBHOME)/INCLUDE/test_global.h

macros.o: macros.hxx
macros.o: recmac.hxx
macros.o: $(ARBHOME)/INCLUDE/ad_prot.h
macros.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
macros.o: $(ARBHOME)/INCLUDE/arb_assert.h
macros.o: $(ARBHOME)/INCLUDE/arb_core.h
macros.o: $(ARBHOME)/INCLUDE/arb_error.h
macros.o: $(ARBHOME)/INCLUDE/arb_msg.h
macros.o: $(ARBHOME)/INCLUDE/arb_string.h
macros.o: $(ARBHOME)/INCLUDE/arbdb.h
macros.o: $(ARBHOME)/INCLUDE/arbdb_base.h
macros.o: $(ARBHOME)/INCLUDE/arbdbt.h
macros.o: $(ARBHOME)/INCLUDE/arbtools.h
macros.o: $(ARBHOME)/INCLUDE/attributes.h
macros.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
macros.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
macros.o: $(ARBHOME)/INCLUDE/aw_base.hxx
macros.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
macros.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
macros.o: $(ARBHOME)/INCLUDE/aw_root.hxx
macros.o: $(ARBHOME)/INCLUDE/aw_root_gtk.hxx
macros.o: $(ARBHOME)/INCLUDE/cb.h
macros.o: $(ARBHOME)/INCLUDE/cxxforward.h
macros.o: $(ARBHOME)/INCLUDE/dupstr.h
macros.o: $(ARBHOME)/INCLUDE/gccver.h
macros.o: $(ARBHOME)/INCLUDE/smartptr.h
macros.o: $(ARBHOME)/INCLUDE/test_global.h

recmac.o: recmac.hxx
recmac.o: $(ARBHOME)/INCLUDE/ad_prot.h
recmac.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
recmac.o: $(ARBHOME)/INCLUDE/arb_assert.h
recmac.o: $(ARBHOME)/INCLUDE/arb_core.h
recmac.o: $(ARBHOME)/INCLUDE/arb_defs.h
recmac.o: $(ARBHOME)/INCLUDE/arb_error.h
recmac.o: $(ARBHOME)/INCLUDE/arb_file.h
recmac.o: $(ARBHOME)/INCLUDE/arb_msg.h
recmac.o: $(ARBHOME)/INCLUDE/arb_strarray.h
recmac.o: $(ARBHOME)/INCLUDE/arb_string.h
recmac.o: $(ARBHOME)/INCLUDE/arbdb.h
recmac.o: $(ARBHOME)/INCLUDE/arbdb_base.h
recmac.o: $(ARBHOME)/INCLUDE/arbdbt.h
recmac.o: $(ARBHOME)/INCLUDE/arbtools.h
recmac.o: $(ARBHOME)/INCLUDE/attributes.h
recmac.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_base.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
recmac.o: $(ARBHOME)/INCLUDE/cb.h
recmac.o: $(ARBHOME)/INCLUDE/cxxforward.h
recmac.o: $(ARBHOME)/INCLUDE/dupstr.h
recmac.o: $(ARBHOME)/INCLUDE/FileContent.h
recmac.o: $(ARBHOME)/INCLUDE/gccver.h
recmac.o: $(ARBHOME)/INCLUDE/smartptr.h
recmac.o: $(ARBHOME)/INCLUDE/test_global.h
recmac.o: $(ARBHOME)/INCLUDE/test_unit.h
