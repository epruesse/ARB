# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = Camera.o Main.o Menu.o Mouse.o Structure3D.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

Camera.o: Camera.hxx
Camera.o: GlobalHeader.hxx
Camera.o: Structure3D.hxx

Main.o: Camera.hxx
Main.o: GlobalHeader.hxx
Main.o: Globals.hxx
Main.o: Main.hxx
Main.o: Menu.hxx
Main.o: Mouse.hxx
Main.o: OPENGL/OpenGLGraphics.hxx
Main.o: OPENGL/Renderer.hxx
Main.o: OPENGL/TextureMapping.hxx
Main.o: Structure3D.hxx

Menu.o: GlobalHeader.hxx
Menu.o: Menu.hxx

Mouse.o: GlobalHeader.hxx
Mouse.o: Mouse.hxx

Structure3D.o: GlobalHeader.hxx
Structure3D.o: Globals.hxx
Structure3D.o: OPENGL/OpenGLGraphics.hxx
Structure3D.o: Structure3D.hxx
