.SUFFIXES: .o .c .depend

CC = $(ACC)
cflags = -g

MFILE =
INCDIR = -I$(OPENWINHOME)/include -I$(XHOME)/include
LIBDIR = -L$(OPENWINHOME)/lib -L$(XHOME)/lib
LIBS = -lxview -lolgx -lX11  $(SYSLIBS)

libs.o = Alloc.o HGLfuncs.o
PROGS = mapview MakeCons Consto01mask PrintStrat Translate heapsortHGL DotPlotTool
OBJECTS = Alloc.o Consto01mask.o DotPlotTool.o HGLfuncs.o MAP_ChooseFile.o MakeCons.o \
		PrintStrat.o Translate.o heapsortHGL.o mapview.o

all:	$(PROGS:%=$(ARBHOME)/bin/%)

.c.o :
	$(CC) -c $(cflags) $< $(INCDIR)

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

$(ARBHOME)/bin/mapview: mapview.c MAP_ChooseFile.o $(libs.o)
	$(CC) $(cflags) -o $@ $(@F).c MAP_ChooseFile.o $(libs.o) $(INCDIR) $(LIBDIR) $(LIBS)

$(ARBHOME)/bin/DotPlotTool: DotPlotTool.c MAP_ChooseFile.o $(libs.o)
	$(CC) $(cflags) -o $@ $(@F).c MAP_ChooseFile.o $(libs.o) \
$(INCDIR) $(LIBDIR) $(LIBS)

$(ARBHOME)/bin/MakeCons: MakeCons.c $(libs.o) $(MKFILE)
	$(CC) $(cflags) -o $@ $(@F).c $(libs.o) $(SYSLIBS)

$(ARBHOME)/bin/Translate: Translate.c $(libs.o) $(MKFILE)
	$(CC) $(cflags) -o $@ $(@F).c $(libs.o) $(SYSLIBS)

$(ARBHOME)/bin/heapsortHGL: heapsortHGL.c $(libs.o) $(MKFILE)
	$(CC) $(cflags) -o $@ $(@F).c $(libs.o) $(SYSLIBS)

$(ARBHOME)/bin/PrintStrat: PrintStrat.c $(libs.o) $(MKFILE)
	$(CC) $(cflags) -o $@ $(@F).c $(libs.o) $(SYSLIBS)

$(ARBHOME)/bin/Consto01mask: Consto01mask.c $(libs.o) $(MKFILE)
	$(CC) $(cflags) -o $@ $(@F).c $(libs.o) $(SYSLIBS)
# DO NOT DELETE THIS LINE -- make depend depends on it.

Alloc.o: global_defs.h

Consto01mask.o: global_defs.h

DotPlotTool.o: global_defs.h plot.icon

HGLfuncs.o: global_defs.h

MakeCons.o: global_defs.h

PrintStrat.o: global_defs.h

Translate.o: global_defs.h

heapsortHGL.o: global_defs.h

mapview.o: global_defs.h
