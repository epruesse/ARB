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

refentries.o: refentries.h
refentries.o: $(ARBHOME)/INCLUDE/ad_prot.h
refentries.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
refentries.o: $(ARBHOME)/INCLUDE/arb_assert.h
refentries.o: $(ARBHOME)/INCLUDE/arb_core.h
refentries.o: $(ARBHOME)/INCLUDE/arb_defs.h
refentries.o: $(ARBHOME)/INCLUDE/arb_error.h
refentries.o: $(ARBHOME)/INCLUDE/arb_msg.h
refentries.o: $(ARBHOME)/INCLUDE/arb_strarray.h
refentries.o: $(ARBHOME)/INCLUDE/arb_string.h
refentries.o: $(ARBHOME)/INCLUDE/arbdb.h
refentries.o: $(ARBHOME)/INCLUDE/arbdb_base.h
refentries.o: $(ARBHOME)/INCLUDE/arbdbt.h
refentries.o: $(ARBHOME)/INCLUDE/arbtools.h
refentries.o: $(ARBHOME)/INCLUDE/attributes.h
refentries.o: $(ARBHOME)/INCLUDE/aw_action.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_base.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_common.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_common_gtk.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_element.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_help.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_position.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_root.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_window.hxx
refentries.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
refentries.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
refentries.o: $(ARBHOME)/INCLUDE/cb.h
refentries.o: $(ARBHOME)/INCLUDE/cb_base.h
refentries.o: $(ARBHOME)/INCLUDE/cbtypes.h
refentries.o: $(ARBHOME)/INCLUDE/cxxforward.h
refentries.o: $(ARBHOME)/INCLUDE/dbitem_set.h
refentries.o: $(ARBHOME)/INCLUDE/downcast.h
refentries.o: $(ARBHOME)/INCLUDE/dupstr.h
refentries.o: $(ARBHOME)/INCLUDE/gccver.h
refentries.o: $(ARBHOME)/INCLUDE/items.h
refentries.o: $(ARBHOME)/INCLUDE/smartptr.h
refentries.o: $(ARBHOME)/INCLUDE/static_assert.h
refentries.o: $(ARBHOME)/INCLUDE/test_global.h
refentries.o: $(ARBHOME)/INCLUDE/ttypes.h
