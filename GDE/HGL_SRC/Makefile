.SUFFIXES: .o .c

CC = $(ACC)
cflags = -g

MFILE = 
INCDIR = -I$(OPENWINHOME)/include -I$(XHOME)/include
LIBDIR = -L$(OPENWINHOME)/lib -L$(XHOME)/lib
LIBS = -lxview -lolgx -lX11  $(SYSLIBS)

libs.o = Alloc.o HGLfuncs.o
PROGS = mapview MakeCons Consto01mask PrintStrat Translate heapsortHGL DotPlotTool

all:	$(PROGS:%=$(ARBHOME)/bin/%)

.c.o :
	$(CC) -c $(cflags) $< $(INCDIR)
depend:
	makedepend $(MAKEDEPENDINC) *.c
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

Alloc.o:  
Alloc.o:  
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
Alloc.o:  
Alloc.o:  
Alloc.o:  
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
Alloc.o:  global_defs.h 
Alloc.o: 
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
Alloc.o:   
Alloc.o: 
Consto01mask.o: global_defs.h  
Consto01mask.o:  
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o: 
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
Consto01mask.o:  
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
Consto01mask.o:  
Consto01mask.o:  
DotPlotTool.o: global_defs.h  
DotPlotTool.o:  
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o: 
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
DotPlotTool.o:  
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  plot.icon
HGLfuncs.o: global_defs.h  
HGLfuncs.o:  
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o: 
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
HGLfuncs.o:  
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
HGLfuncs.o:   
HGLfuncs.o: 
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o: 
MAP_ChooseFile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
MakeCons.o:   global_defs.h
MakeCons.o:  
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
MakeCons.o:  
MakeCons.o: 
PrintStrat.o: global_defs.h  
PrintStrat.o:  
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o: 
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
PrintStrat.o:  
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
PrintStrat.o:   
PrintStrat.o: 
Translate.o:  
Translate.o:  
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
Translate.o:  
Translate.o:  global_defs.h 
Translate.o: 
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
Translate.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
heapsortHGL.o:  global_defs.h
heapsortHGL.o:  
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
heapsortHGL.o:  
heapsortHGL.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o: 
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:   global_defs.h
mapview.o:   
mapview.o:  
mapview.o:  
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
mapview.o:  
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
mapview.o:  
