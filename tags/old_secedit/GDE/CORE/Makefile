
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
	$(ARBHOME)/MAKEBIN/aisc_mkpt -C *.c >functions.h.tmp
	arb_replace -l 'DummyRepaint*=DummyRepaint();' functions.h.tmp
	arb_replace -l 'RepaintNACan*=RepaintNACan();' functions.h.tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff functions.h.tmp functions.h

.c.o:
	$(ACC) $(cflags) $(AINCLUDES) -c -o $@ $<

clean:
	rm $(MAIN) $(C_OBJECTS)

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
BasicDisplay.o: $(ARBHOME)/INCLUDE/ad_prot.h
BasicDisplay.o: $(ARBHOME)/INCLUDE/arb_assert.h
BasicDisplay.o: $(ARBHOME)/INCLUDE/arbdb.h

BuiltIn.o: defines.h
BuiltIn.o: functions.h
BuiltIn.o: menudefs.h
BuiltIn.o: $(ARBHOME)/INCLUDE/ad_prot.h
BuiltIn.o: $(ARBHOME)/INCLUDE/arb_assert.h
BuiltIn.o: $(ARBHOME)/INCLUDE/arbdb.h

CutCopyPaste.o: defines.h
CutCopyPaste.o: functions.h
CutCopyPaste.o: menudefs.h
CutCopyPaste.o: $(ARBHOME)/INCLUDE/ad_prot.h
CutCopyPaste.o: $(ARBHOME)/INCLUDE/arb_assert.h
CutCopyPaste.o: $(ARBHOME)/INCLUDE/arbdb.h

DrawNA.o: defines.h
DrawNA.o: functions.h
DrawNA.o: menudefs.h
DrawNA.o: $(ARBHOME)/INCLUDE/ad_prot.h
DrawNA.o: $(ARBHOME)/INCLUDE/arb_assert.h
DrawNA.o: $(ARBHOME)/INCLUDE/arbdb.h

Edit.o: defines.h
Edit.o: functions.h
Edit.o: menudefs.h
Edit.o: $(ARBHOME)/INCLUDE/ad_prot.h
Edit.o: $(ARBHOME)/INCLUDE/arb_assert.h
Edit.o: $(ARBHOME)/INCLUDE/arbdb.h

EventHandler.o: defines.h
EventHandler.o: functions.h
EventHandler.o: menudefs.h
EventHandler.o: $(ARBHOME)/INCLUDE/ad_prot.h
EventHandler.o: $(ARBHOME)/INCLUDE/arb_assert.h
EventHandler.o: $(ARBHOME)/INCLUDE/arbdb.h

FileIO.o: defines.h
FileIO.o: functions.h
FileIO.o: menudefs.h
FileIO.o: $(ARBHOME)/INCLUDE/ad_prot.h
FileIO.o: $(ARBHOME)/INCLUDE/arb_assert.h
FileIO.o: $(ARBHOME)/INCLUDE/arbdb.h

Free.o: defines.h
Free.o: functions.h
Free.o: menudefs.h
Free.o: $(ARBHOME)/INCLUDE/ad_prot.h
Free.o: $(ARBHOME)/INCLUDE/arb_assert.h
Free.o: $(ARBHOME)/INCLUDE/arbdb.h

Genbank.o: defines.h
Genbank.o: functions.h
Genbank.o: menudefs.h
Genbank.o: $(ARBHOME)/INCLUDE/ad_prot.h
Genbank.o: $(ARBHOME)/INCLUDE/arb_assert.h
Genbank.o: $(ARBHOME)/INCLUDE/arbdb.h

HGLfile.o: defines.h
HGLfile.o: functions.h
HGLfile.o: menudefs.h
HGLfile.o: $(ARBHOME)/INCLUDE/ad_prot.h
HGLfile.o: $(ARBHOME)/INCLUDE/arb_assert.h
HGLfile.o: $(ARBHOME)/INCLUDE/arbdb.h

main.o: defines.h
main.o: functions.h
main.o: globals.h
main.o: icon_gde.bitmap
main.o: menudefs.h
main.o: $(ARBHOME)/INCLUDE/ad_prot.h
main.o: $(ARBHOME)/INCLUDE/arb_assert.h
main.o: $(ARBHOME)/INCLUDE/arbdb.h

ParseMenu.o: defines.h
ParseMenu.o: functions.h
ParseMenu.o: menudefs.h
ParseMenu.o: $(ARBHOME)/INCLUDE/ad_prot.h
ParseMenu.o: $(ARBHOME)/INCLUDE/arb_assert.h
ParseMenu.o: $(ARBHOME)/INCLUDE/arbdb.h

Scroll.o: defines.h
Scroll.o: functions.h
Scroll.o: menudefs.h
Scroll.o: $(ARBHOME)/INCLUDE/ad_prot.h
Scroll.o: $(ARBHOME)/INCLUDE/arb_assert.h
Scroll.o: $(ARBHOME)/INCLUDE/arbdb.h
