
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
ParseMenu.o:  
ParseMenu.o:  
ParseMenu.o:  menudefs.h defines.h functions.h
main.o:   
main.o:  
main.o:  
main.o:   menudefs.h
main.o: defines.h functions.h globals.h icon_gde.bitmap
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  
BasicDisplay.o:  menudefs.h defines.h functions.h
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o:  
EventHandler.o:   menudefs.h
EventHandler.o: defines.h functions.h
FileIO.o:  
FileIO.o:  
FileIO.o:  
FileIO.o:  
FileIO.o:  menudefs.h defines.h functions.h
DrawNA.o:   
DrawNA.o:  
DrawNA.o:  
DrawNA.o:   
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  
DrawNA.o:  menudefs.h defines.h functions.h
Free.o:   
Free.o:  
Free.o:  
Free.o:   menudefs.h
Free.o: defines.h functions.h
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o:  
BuiltIn.o:   menudefs.h defines.h
BuiltIn.o: functions.h
Edit.o:  
Edit.o:   menudefs.h
Edit.o: defines.h functions.h
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:  
Genbank.o:   
Genbank.o:   menudefs.h
Genbank.o: defines.h functions.h
Scroll.o:   
Scroll.o:  
Scroll.o:  
Scroll.o:   
Scroll.o:  
Scroll.o:  
Scroll.o:  
Scroll.o:  menudefs.h defines.h functions.h
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
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:  
CutCopyPaste.o:   menudefs.h
CutCopyPaste.o: defines.h functions.h
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:  
HGLfile.o:   
HGLfile.o:   menudefs.h
HGLfile.o: defines.h functions.h
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o:  
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdb.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h menudefs.h
arbdb_io.o: defines.h functions.h
