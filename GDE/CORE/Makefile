
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


BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o: 
BasicDisplay.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
BasicDisplay.o: defines.h functions.h menudefs.h
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o:   
BuiltIn.o:   
BuiltIn.o: 
BuiltIn.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
BuiltIn.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
BuiltIn.o: functions.h menudefs.h
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o:  
ChooseFile.o: 
ChooseFile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ChooseFile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o: 
CutCopyPaste.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
CutCopyPaste.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
CutCopyPaste.o: defines.h functions.h menudefs.h
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o: 
DrawNA.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
DrawNA.o: functions.h menudefs.h
Edit.o:   
Edit.o:  
Edit.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
Edit.o: functions.h menudefs.h
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o: 
EventHandler.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
EventHandler.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
EventHandler.o: defines.h functions.h menudefs.h
FileIO.o:  
FileIO.o:  
FileIO.o:  
FileIO.o:   
FileIO.o:  
FileIO.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
FileIO.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
FileIO.o: functions.h menudefs.h
Free.o:  
Free.o:  
Free.o:  
Free.o:   
Free.o:  
Free.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Free.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
Free.o: functions.h menudefs.h
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:   
Genbank.o:  
Genbank.o:  
Genbank.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Genbank.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
Genbank.o: functions.h menudefs.h
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:   
HGLfile.o:  
HGLfile.o:  
HGLfile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
HGLfile.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
HGLfile.o: functions.h menudefs.h
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o: 
ParseMenu.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ParseMenu.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ParseMenu.o: defines.h functions.h menudefs.h
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o: 
Scroll.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
Scroll.o: functions.h menudefs.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdb.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o:   
arbdb_io.o: 
arbdb_io.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
arbdb_io.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
arbdb_io.o: functions.h menudefs.h
main.o:  
main.o:  
main.o:  
main.o:   
main.o:  
main.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
main.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h defines.h
main.o: functions.h globals.h icon_gde.bitmap menudefs.h
