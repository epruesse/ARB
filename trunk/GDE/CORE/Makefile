
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

ParseMenu.o:  menudefs.h defines.h functions.h
main.o:  menudefs.h defines.h functions.h
main.o: globals.h icon_gde.bitmap
BasicDisplay.o:  menudefs.h defines.h functions.h
EventHandler.o: menudefs.h defines.h functions.h
FileIO.o:  menudefs.h defines.h functions.h
DrawNA.o:  menudefs.h defines.h functions.h
Free.o:  menudefs.h defines.h functions.h
BuiltIn.o:   menudefs.h
BuiltIn.o: defines.h functions.h
Edit.o:  menudefs.h
Edit.o: defines.h functions.h
Genbank.o:  menudefs.h defines.h functions.h
Scroll.o:  menudefs.h defines.h functions.h
CutCopyPaste.o: menudefs.h defines.h functions.h
HGLfile.o:  menudefs.h defines.h functions.h
arbdb_io.o:  $(ARBHOME)/INCLUDE/arbdb.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h menudefs.h defines.h
arbdb_io.o: functions.h
