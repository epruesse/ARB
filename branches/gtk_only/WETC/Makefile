# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = WETC_main.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $(@:.o=.cxx)  $(CXX_INCLUDES) $(POST_COMPILE) $<

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

WETC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_core.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_defs.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_error.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_string.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbtools.h
WETC_main.o: $(ARBHOME)/INCLUDE/attributes.h
WETC_main.o: $(ARBHOME)/INCLUDE/aw_action.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_base.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_common.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_element.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_help.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_position.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/awt.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/cb.h
WETC_main.o: $(ARBHOME)/INCLUDE/cb_base.h
WETC_main.o: $(ARBHOME)/INCLUDE/cbtypes.h
WETC_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
WETC_main.o: $(ARBHOME)/INCLUDE/downcast.h
WETC_main.o: $(ARBHOME)/INCLUDE/dupstr.h
WETC_main.o: $(ARBHOME)/INCLUDE/gccver.h
WETC_main.o: $(ARBHOME)/INCLUDE/smartptr.h
WETC_main.o: $(ARBHOME)/INCLUDE/static_assert.h
WETC_main.o: $(ARBHOME)/INCLUDE/test_global.h
WETC_main.o: $(ARBHOME)/INCLUDE/ttypes.h
