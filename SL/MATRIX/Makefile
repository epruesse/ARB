# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = AP_matrix.o
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

AP_matrix.o: AP_matrix.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_matrix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_defs.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_matrix.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_matrix.o: $(ARBHOME)/INCLUDE/attributes.h
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AP_matrix.o: $(ARBHOME)/INCLUDE/cb.h
AP_matrix.o: $(ARBHOME)/INCLUDE/cb_base.h
AP_matrix.o: $(ARBHOME)/INCLUDE/cbtypes.h
AP_matrix.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_matrix.o: $(ARBHOME)/INCLUDE/downcast.h
AP_matrix.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_matrix.o: $(ARBHOME)/INCLUDE/gccver.h
AP_matrix.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_matrix.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_matrix.o: $(ARBHOME)/INCLUDE/test_global.h
AP_matrix.o: $(ARBHOME)/INCLUDE/ttypes.h
