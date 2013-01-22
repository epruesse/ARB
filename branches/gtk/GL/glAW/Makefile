# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = GLwMDrawA.o
CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS)

EXTENDED_CPPINCLUDES = $(CPPINCLUDES) -I$(ARBHOME)/WINDOW

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(EXTENDED_CPPINCLUDES) $(POST_COMPILE)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AW_window_ogl.o: aw_window_ogl.hxx
AW_window_ogl.o: GLwDrawA.h
AW_window_ogl.o: GLwMDrawA.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_msg.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arb_string.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/attributes.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/cb.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_window_ogl.o: $(ARBHOME)/INCLUDE/test_global.h
AW_window_ogl.o: $(ARBHOME)/WINDOW/aw_scalar.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW/aw_window_Xm.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/AW_area_management.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_assert.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_at.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_base.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_font_limits.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/AW_gtk_forward_declarations.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_gtk_migration_helpers.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_help.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_keysym.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_position.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_root.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_root_gtk.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_select.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_window.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/aw_xfig.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_clipable.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_device.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_device_click.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_device_print.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_device_size.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_simple_device.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_stylable.hxx
AW_window_ogl.o: $(ARBHOME)/WINDOW_GTK/devices/aw_zoomable.hxx

GLwMDrawA.o: GLwDrawA.c
GLwMDrawA.o: GLwDrawA.h
GLwMDrawA.o: GLwDrawAP.h
GLwMDrawA.o: GLwMDrawA.h
GLwMDrawA.o: GLwMDrawAP.h
