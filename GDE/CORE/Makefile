
.SUFFIXES: .o .c

CCOBJS= ParseMenu.o main.o BasicDisplay.o EventHandler.o FileIO.o \
DrawNA.o Free.o BuiltIn.o Edit.o Genbank.o Scroll.o ChooseFile.o \
CutCopyPaste.o HGLfile.o arbdb_io.o

LIBS= -L$(OPENWINHOME)/lib -L$(XHOME)/lib -lm -lxview -lolgx -lX11 -L$(ARBHOME)/LIBLINK -lARBDO $(SYSLIBS)

$(MAIN): $(ARBHOME)/bin/arb_gde

$(ARBHOME)/bin/arb_gde: $(CCOBJS)
	$(ACC) $(cflags) -o $@ $(CCOBJS) $(LIBS)

update: $(ARBHOME)/bin/arb_gde

proto:
	rm -f functions.h
	../../MAKEBIN/aisc_mkpt -C *.c >functions.h
	arb_replace -l 'DummyRepaint*=DummyRepaint();' functions.h
	arb_replace -l 'RepaintNACan*=RepaintNACan();' functions.h

.c.o:
	$(ACC) $(cflags) $(AINCLUDES) -c -o $@ $<

depend:
	makedepend $(MAKEDEPENDINC) $(CCOBJS:.o=.c)

# DO NOT DELETE THIS LINE -- make depend depends on it.

ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
ParseMenu.o:   menudefs.h
ParseMenu.o: defines.h functions.h
main.o:   
main.o: 
main.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
main.o:  
main.o:  
main.o:  
main.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
main.o:   menudefs.h
main.o: defines.h functions.h globals.h icon_gde.bitmap
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  menudefs.h defines.h functions.h
EventHandler.o:  
EventHandler.o:  
EventHandler.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
EventHandler.o:  
EventHandler.o:  menudefs.h defines.h functions.h
FileIO.o:  
FileIO.o:  
FileIO.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
FileIO.o:  
FileIO.o:  
FileIO.o:  
FileIO.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
FileIO.o:   menudefs.h
FileIO.o: defines.h functions.h
DrawNA.o:  
DrawNA.o:  
DrawNA.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  menudefs.h defines.h functions.h
Free.o:   
Free.o: 
Free.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
Free.o:  
Free.o:  
Free.o:  
Free.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
Free.o:   menudefs.h
Free.o: defines.h functions.h
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
BuiltIn.o:  
BuiltIn.o:  menudefs.h defines.h functions.h
Edit.o:  
Edit.o:  
Edit.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h menudefs.h
Edit.o: defines.h functions.h
Genbank.o:  
Genbank.o:  
Genbank.o: 
Genbank.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:   
Genbank.o:  
Genbank.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
Genbank.o:   menudefs.h
Genbank.o: defines.h functions.h
Scroll.o:  
Scroll.o:  
Scroll.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  menudefs.h defines.h functions.h
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o: 
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
CutCopyPaste.o:  
CutCopyPaste.o:  menudefs.h defines.h functions.h
HGLfile.o:  
HGLfile.o:  
HGLfile.o: 
HGLfile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:   
HGLfile.o:  
HGLfile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
HGLfile.o:   menudefs.h
HGLfile.o: defines.h functions.h
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
arbdb_io.o:  $(ARBHOME)/INCLUDE/arbdb.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h menudefs.h defines.h
arbdb_io.o: functions.h
