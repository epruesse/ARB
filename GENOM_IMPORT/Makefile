# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cpp .depend

# --------------------------------------------------------------------------------

# objects for library
CPP_OBJECTS =   \
		GAARB.o \
		GALogger.o \
		GAFile.o \
		GAEmbl.o \
		GADDBJ.o \
		GAGenBank.o \
		GASourceFileSwitcher.o \
		GAGenomImport.o \


CREATE_TEST_EXECUTABLE=0

# --------------------------------------------------------------------------------

ifeq ($(CREATE_TEST_EXECUTABLE),0)

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

else

$(MAIN): $(CPP_OBJECTS) Makefile
	$(XAR) $(MAIN) $(CPP_OBJECTS)
	$(CPP) $(lflags) $(MAIN) $(EXE_CPP_OBJECTS)

endif


.cpp.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

clean:
	-rm *.o
	-rm *.a
	-rm *.bak
	-rm *~
	-rm *#
	-rm *\%
ifeq ($(CREATE_TEST_EXECUTABLE),1)
	-rm test_import
endif

#.IGNORE:
#	clean

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cpp.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

GAARB.o: GAARB.h
GAARB.o: $(ARBHOME)/INCLUDE/ad_prot.h
GAARB.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GAARB.o: $(ARBHOME)/INCLUDE/adGene.h
GAARB.o: $(ARBHOME)/INCLUDE/arb_assert.h
GAARB.o: $(ARBHOME)/INCLUDE/arbdb.h
GAARB.o: $(ARBHOME)/INCLUDE/arbdbt.h
GAARB.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GAARB.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GAARB.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GAARB.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GAARB.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GAARB.o: $(ARBHOME)/INCLUDE/awt.hxx

GADDBJ.o: GAARB.h
GADDBJ.o: GADDBJ.h
GADDBJ.o: GAFile.h
GADDBJ.o: GALogger.h
GADDBJ.o: $(ARBHOME)/INCLUDE/ad_prot.h
GADDBJ.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GADDBJ.o: $(ARBHOME)/INCLUDE/adGene.h
GADDBJ.o: $(ARBHOME)/INCLUDE/arb_assert.h
GADDBJ.o: $(ARBHOME)/INCLUDE/arbdb.h
GADDBJ.o: $(ARBHOME)/INCLUDE/arbdbt.h
GADDBJ.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GADDBJ.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GADDBJ.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GADDBJ.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GADDBJ.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GADDBJ.o: $(ARBHOME)/INCLUDE/awt.hxx

GAEmbl.o: GAARB.h
GAEmbl.o: GAEmbl.h
GAEmbl.o: GAFile.h
GAEmbl.o: GALogger.h
GAEmbl.o: $(ARBHOME)/INCLUDE/ad_prot.h
GAEmbl.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GAEmbl.o: $(ARBHOME)/INCLUDE/adGene.h
GAEmbl.o: $(ARBHOME)/INCLUDE/arb_assert.h
GAEmbl.o: $(ARBHOME)/INCLUDE/arbdb.h
GAEmbl.o: $(ARBHOME)/INCLUDE/arbdbt.h
GAEmbl.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GAEmbl.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GAEmbl.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GAEmbl.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GAEmbl.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GAEmbl.o: $(ARBHOME)/INCLUDE/awt.hxx

GAFile.o: GAARB.h
GAFile.o: GAFile.h
GAFile.o: GALogger.h
GAFile.o: $(ARBHOME)/INCLUDE/ad_prot.h
GAFile.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GAFile.o: $(ARBHOME)/INCLUDE/adGene.h
GAFile.o: $(ARBHOME)/INCLUDE/arb_assert.h
GAFile.o: $(ARBHOME)/INCLUDE/arbdb.h
GAFile.o: $(ARBHOME)/INCLUDE/arbdbt.h
GAFile.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GAFile.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GAFile.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GAFile.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GAFile.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GAFile.o: $(ARBHOME)/INCLUDE/awt.hxx

GAGenBank.o: GAARB.h
GAGenBank.o: GAFile.h
GAGenBank.o: GAGenBank.h
GAGenBank.o: GALogger.h
GAGenBank.o: $(ARBHOME)/INCLUDE/ad_prot.h
GAGenBank.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GAGenBank.o: $(ARBHOME)/INCLUDE/adGene.h
GAGenBank.o: $(ARBHOME)/INCLUDE/arb_assert.h
GAGenBank.o: $(ARBHOME)/INCLUDE/arbdb.h
GAGenBank.o: $(ARBHOME)/INCLUDE/arbdbt.h
GAGenBank.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GAGenBank.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GAGenBank.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GAGenBank.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GAGenBank.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GAGenBank.o: $(ARBHOME)/INCLUDE/awt.hxx

GAGenomImport.o: GAARB.h
GAGenomImport.o: GADDBJ.h
GAGenomImport.o: GAEmbl.h
GAGenomImport.o: GAFile.h
GAGenomImport.o: GAGenBank.h
GAGenomImport.o: GALogger.h
GAGenomImport.o: GASourceFileSwitcher.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/ad_prot.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/adGene.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/arb_assert.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/arbdb.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/arbdbt.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/awt.hxx

GALogger.o: GALogger.h

GASourceFileSwitcher.o: GASourceFileSwitcher.h
