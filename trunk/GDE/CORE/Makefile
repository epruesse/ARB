
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


BasicDisplay.o: defines.h functions.h menudefs.h
BuiltIn.o:  defines.h
BuiltIn.o: functions.h menudefs.h
CutCopyPaste.o: defines.h functions.h menudefs.h
DrawNA.o:  defines.h
DrawNA.o: functions.h menudefs.h
Edit.o:  defines.h
Edit.o: functions.h menudefs.h
EventHandler.o: defines.h functions.h menudefs.h
FileIO.o:  defines.h
FileIO.o: functions.h menudefs.h
Free.o:  defines.h
Free.o: functions.h menudefs.h
Genbank.o:  defines.h
Genbank.o: functions.h menudefs.h
HGLfile.o:  defines.h
HGLfile.o: functions.h menudefs.h
ParseMenu.o: defines.h functions.h menudefs.h
Scroll.o:  defines.h
Scroll.o: functions.h menudefs.h
arbdb_io.o: /home/yadhu/ARB/INCLUDE/ad_prot.h
arbdb_io.o: /home/yadhu/ARB/INCLUDE/ad_t_prot.h
arbdb_io.o: /home/yadhu/ARB/INCLUDE/arbdb.h /home/yadhu/ARB/INCLUDE/arbdbt.h
arbdb_io.o:  defines.h
arbdb_io.o: functions.h menudefs.h
main.o:  defines.h
main.o: functions.h globals.h icon_gde.bitmap menudefs.h
