
.SUFFIXES: .o .c .depend

C_OBJECTS= ParseMenu.o main.o BasicDisplay.o EventHandler.o FileIO.o \
DrawNA.o Free.o BuiltIn.o Edit.o Genbank.o Scroll.o ChooseFile.o \
CutCopyPaste.o HGLfile.o arbdb_io.o

LIBS= -L$(OPENWINHOME)/lib -L$(XHOME)/lib -lm -lxview -lolgx -lX11 -L$(ARBHOME)/LIBLINK -lARBDO $(SYSLIBS)

$(MAIN): $(ARBHOME)/bin/arb_gde

$(ARBHOME)/bin/arb_gde: $(C_OBJECTS)
	$(ACC) $(cflags) -o $@ $(C_OBJECTS) $(LIBS)

update: $(ARBHOME)/bin/arb_gde

proto: functions.h

$(C_OBJECTS) : functions.h

functions.h: $(C_OBJECTS:.o=.c)
	rm -f functions.h
	../../MAKEBIN/aisc_mkpt -C *.c >functions.h
	arb_replace -l 'DummyRepaint*=DummyRepaint();' functions.h
	arb_replace -l 'RepaintNACan*=RepaintNACan();' functions.h

.c.o:
	$(ACC) $(cflags) $(AINCLUDES) -c -o $@ $<

DEPENDS = $(C_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

arbdb_io.o: defines.h
arbdb_io.o: functions.h
arbdb_io.o: menudefs.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdb.h
arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h

BasicDisplay.o: defines.h
BasicDisplay.o: functions.h
BasicDisplay.o: menudefs.h

BuiltIn.o: defines.h
BuiltIn.o: functions.h
BuiltIn.o: menudefs.h

CutCopyPaste.o: defines.h
CutCopyPaste.o: functions.h
CutCopyPaste.o: menudefs.h

DrawNA.o: defines.h
DrawNA.o: functions.h
DrawNA.o: menudefs.h

Edit.o: defines.h
Edit.o: functions.h
Edit.o: menudefs.h

EventHandler.o: defines.h
EventHandler.o: functions.h
EventHandler.o: menudefs.h

FileIO.o: defines.h
FileIO.o: functions.h
FileIO.o: menudefs.h

Free.o: defines.h
Free.o: functions.h
Free.o: menudefs.h

Genbank.o: defines.h
Genbank.o: functions.h
Genbank.o: menudefs.h

HGLfile.o: defines.h
HGLfile.o: functions.h
HGLfile.o: menudefs.h

main.o: defines.h
main.o: functions.h
main.o: globals.h
main.o: icon_gde.bitmap
main.o: menudefs.h

ParseMenu.o: defines.h
ParseMenu.o: functions.h
ParseMenu.o: menudefs.h

Scroll.o: defines.h
Scroll.o: functions.h
Scroll.o: menudefs.h
