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
Alloc.o:  
Alloc.o:  
Alloc.o:  
Alloc.o:  
Alloc.o:  
Alloc.o:  
Alloc.o:   
Alloc.o:  
Alloc.o:   
Alloc.o:  
Alloc.o:  
Alloc.o:  
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Alloc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Alloc.o: global_defs.h
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o:  
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Consto01mask.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Consto01mask.o: global_defs.h
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o:  
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
DotPlotTool.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
DotPlotTool.o: global_defs.h plot.icon
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:   
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o:  
HGLfuncs.o: 
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
HGLfuncs.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
HGLfuncs.o: global_defs.h
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o:  
MAP_ChooseFile.o: 
MAP_ChooseFile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o:   
MakeCons.o:  
MakeCons.o:  
MakeCons.o:  
MakeCons.o: 
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MakeCons.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
MakeCons.o: global_defs.h
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:   
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o:  
PrintStrat.o: 
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PrintStrat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
PrintStrat.o: global_defs.h
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o:   
Translate.o:  
Translate.o:  
Translate.o:  
Translate.o: 
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Translate.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Translate.o: global_defs.h
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o:  
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
heapsortHGL.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
heapsortHGL.o: global_defs.h
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:   
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o:  
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
mapview.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
mapview.o: global_defs.h
