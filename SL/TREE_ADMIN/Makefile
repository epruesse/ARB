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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

TreeAdmin.o: TreeAdmin.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_defs.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/attributes.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_at.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_base.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_common.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_common_gtk.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_help.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_position.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_root_gtk.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_select.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_xfig.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/cb.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_clipable.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_clipable.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_device_click.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_device_gtk.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_device_impl.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_device_impl.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_device_print.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_device_size.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_simple_device.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_simple_device.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_stylable.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_stylable.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_zoomable.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/devices/aw_zoomable.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/downcast.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/gccver.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/test_global.h
