.SUFFIXES: .o .c .cxx .depend

STD_OBJECTS =	AW_xkey.o AW_xfont.o AW_nawar.o AW_button.o \
		AW_at.o AW_print.o AW_window.o \
		AW_device.o AW_Xm.o AW_click.o AW_size.o \
		AW_xfig.o AW_xfigfont.o AW_status.o AW_preset.o \
		AW_question.o AW_global_awars.o \

OPENGL_OBJECTS = GLwMDrawA.o GLwDrawA.o

ifeq ($(OPENGL),1)
OBJECTS=$(STD_OBJECTS) $(OPENGL_OBJECTS)
else
OBJECTS=$(STD_OBJECTS)
endif

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.cxx.o:
	$(CCPLIB) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
