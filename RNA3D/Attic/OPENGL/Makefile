# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = OpenGLGraphics.o Renderer.o TextureMapping.o


$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

OpenGLGraphics.o: ../Globals.hxx
OpenGLGraphics.o: GlobalHeader.hxx
OpenGLGraphics.o: Globals.hxx
OpenGLGraphics.o: OpenGLGraphics.hxx

Renderer.o: ../Globals.hxx
Renderer.o: GlobalHeader.hxx
Renderer.o: Globals.hxx
Renderer.o: OpenGLGraphics.hxx
Renderer.o: Renderer.hxx
Renderer.o: TextureMapping.hxx

TextureMapping.o: ../Globals.hxx
TextureMapping.o: GlobalHeader.hxx
TextureMapping.o: Globals.hxx
TextureMapping.o: TextureMapping.hxx
