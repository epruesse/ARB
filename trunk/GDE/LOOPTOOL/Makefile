.SUFFIXES: .o .c

OBJECTS = main.o Window.o menu.o ReadData.o Draw.o WriteData.o


$(ARBHOME)/bin/LoopTool: $(OBJECTS)
	$(ACC) -o $@ $(OBJECTS) -L$(OPENWINHOME)/lib -L$(XHOME)/lib -lm  -lxview  -lolgx -lX11

.c.o:
	$(ACC) -c $(cflags) $(AINCLUDES) -I$(OPENWINHOME)/include $<

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECTS:.o=.c)
# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o:   loop.h globals.h
Window.o:  loop.h globals.h
menu.o:  loop.h globals.h
ReadData.o:   loop.h
ReadData.o: globals.h
Draw.o:   loop.h globals.h
WriteData.o:   loop.h
WriteData.o: globals.h
