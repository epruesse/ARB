.SUFFIXES: .o .c .depend

OBJECTS = main.o Window.o menu.o ReadData.o Draw.o WriteData.o


$(ARBHOME)/bin/LoopTool: $(OBJECTS)
	$(ACC) -o $@ $(OBJECTS) -L$(OPENWINHOME)/lib -L$(XHOME)/lib -lm  -lxview  -lolgx -lX11

.c.o:
	$(ACC) -c $(cflags) $(AINCLUDES) -I$(OPENWINHOME)/include $<

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: loop.h globals.h

Window.o: loop.h globals.h

menu.o: loop.h globals.h

ReadData.o: loop.h globals.h

Draw.o: loop.h globals.h

WriteData.o: loop.h globals.h
