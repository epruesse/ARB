# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = GLwMDrawA.o
CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS)

EXTENDED_CPPINCLUDES = $(CXX_INCLUDES) -I$(ARBHOME)/WINDOW

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(EXTENDED_CPPINCLUDES) $(POST_COMPILE) $<

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
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $(EXTENDED_CPPINCLUDES) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from GL)

AW_window_ogl.o: aw_window_ogl.hxx
AW_window_ogl.o: GLwDrawA.h
AW_window_ogl.o: GLwMDrawA.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_defs.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_msg.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_string.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/attributes.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_action.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_common.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_element.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_help.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
AW_window_ogl.o: $(ARBHOME)/INCLUDE/cb.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/cb_base.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/cbtypes.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/cxxforward.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/downcast.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/gccver.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/static_assert.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/test_global.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/ttypes.h

GLwMDrawA.o: GLwDrawA.c
GLwMDrawA.o: GLwDrawA.h
GLwMDrawA.o: GLwDrawAP.h
GLwMDrawA.o: GLwMDrawA.h
GLwMDrawA.o: GLwMDrawAP.h
