# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cpp .depend

# --------------------------------------------------------------------------------

# objects for library
CPP_OBJECTS =   \
		GAGenom.o \
		GAGenomGene.o \
		GAGenomFeatureTable.o \
		GAGenomGeneLocation.o \
		GAGenomReference.o \
		GAGenomFeatureTableSource.o \
		GAGenomEmbl.o \
		GAGenomGeneEmbl.o \
		GAGenomFeatureTableEmbl.o \
		GAGenomGeneLocationEmbl.o \
		GAGenomReferenceEmbl.o \
		GAGenomFeatureTableSourceEmbl.o \
		GAGenomGenBank.o \
		GAGenomGeneGenBank.o \
		GAGenomFeatureTableGenBank.o \
		GAGenomGeneLocationGenBank.o \
		GAGenomReferenceGenBank.o \
		GAGenomFeatureTableSourceGenBank.o \
		GAGenomDDBJ.o \
		GAParser.o	\
		GAGenomGeneDDBJ.o \
		GAGenomFeatureTableDDBJ.o \
		GAGenomGeneLocationDDBJ.o \
		GAGenomReferenceDDBJ.o \
		GAGenomFeatureTableSourceDDBJ.o \
		GAGenomUtilities.o \
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

GAGenom.o: GAGenom.h

GAGenomDDBJ.o: GAGenom.h
GAGenomDDBJ.o: GAGenomDDBJ.h
GAGenomDDBJ.o: GAGenomFeatureTable.h
GAGenomDDBJ.o: GAGenomFeatureTableDDBJ.h
GAGenomDDBJ.o: GAGenomFeatureTableSource.h
GAGenomDDBJ.o: GAGenomFeatureTableSourceDDBJ.h
GAGenomDDBJ.o: GAGenomGene.h
GAGenomDDBJ.o: GAGenomGeneDDBJ.h
GAGenomDDBJ.o: GAGenomGeneLocation.h
GAGenomDDBJ.o: GAGenomGeneLocationDDBJ.h
GAGenomDDBJ.o: GAGenomReference.h
GAGenomDDBJ.o: GAGenomReferenceDDBJ.h
GAGenomDDBJ.o: GAGenomUtilities.h
GAGenomDDBJ.o: GAParser.h

GAGenomEmbl.o: GAGenom.h
GAGenomEmbl.o: GAGenomEmbl.h
GAGenomEmbl.o: GAGenomFeatureTable.h
GAGenomEmbl.o: GAGenomFeatureTableEmbl.h
GAGenomEmbl.o: GAGenomFeatureTableSource.h
GAGenomEmbl.o: GAGenomFeatureTableSourceEmbl.h
GAGenomEmbl.o: GAGenomGene.h
GAGenomEmbl.o: GAGenomGeneEmbl.h
GAGenomEmbl.o: GAGenomGeneLocation.h
GAGenomEmbl.o: GAGenomGeneLocationEmbl.h
GAGenomEmbl.o: GAGenomReference.h
GAGenomEmbl.o: GAGenomReferenceEmbl.h
GAGenomEmbl.o: GAGenomUtilities.h
GAGenomEmbl.o: GAParser.h

GAGenomFeatureTable.o: GAGenomFeatureTable.h
GAGenomFeatureTable.o: GAParser.h

GAGenomFeatureTableDDBJ.o: GAGenomFeatureTable.h
GAGenomFeatureTableDDBJ.o: GAGenomFeatureTableDDBJ.h
GAGenomFeatureTableDDBJ.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableDDBJ.o: GAGenomFeatureTableSourceDDBJ.h
GAGenomFeatureTableDDBJ.o: GAGenomGene.h
GAGenomFeatureTableDDBJ.o: GAGenomGeneDDBJ.h
GAGenomFeatureTableDDBJ.o: GAGenomGeneLocation.h
GAGenomFeatureTableDDBJ.o: GAGenomGeneLocationDDBJ.h
GAGenomFeatureTableDDBJ.o: GAGenomUtilities.h
GAGenomFeatureTableDDBJ.o: GAParser.h

GAGenomFeatureTableEmbl.o: GAGenomFeatureTable.h
GAGenomFeatureTableEmbl.o: GAGenomFeatureTableEmbl.h
GAGenomFeatureTableEmbl.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableEmbl.o: GAGenomFeatureTableSourceEmbl.h
GAGenomFeatureTableEmbl.o: GAGenomGene.h
GAGenomFeatureTableEmbl.o: GAGenomGeneEmbl.h
GAGenomFeatureTableEmbl.o: GAGenomGeneLocation.h
GAGenomFeatureTableEmbl.o: GAGenomGeneLocationEmbl.h
GAGenomFeatureTableEmbl.o: GAGenomUtilities.h
GAGenomFeatureTableEmbl.o: GAParser.h

GAGenomFeatureTableGenBank.o: GAGenomFeatureTable.h
GAGenomFeatureTableGenBank.o: GAGenomFeatureTableGenBank.h
GAGenomFeatureTableGenBank.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableGenBank.o: GAGenomFeatureTableSourceGenBank.h
GAGenomFeatureTableGenBank.o: GAGenomGene.h
GAGenomFeatureTableGenBank.o: GAGenomGeneGenBank.h
GAGenomFeatureTableGenBank.o: GAGenomGeneLocation.h
GAGenomFeatureTableGenBank.o: GAGenomGeneLocationGenBank.h
GAGenomFeatureTableGenBank.o: GAGenomUtilities.h
GAGenomFeatureTableGenBank.o: GAParser.h

GAGenomFeatureTableSource.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableSource.o: GAParser.h

GAGenomFeatureTableSourceDDBJ.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableSourceDDBJ.o: GAGenomFeatureTableSourceDDBJ.h
GAGenomFeatureTableSourceDDBJ.o: GAGenomUtilities.h
GAGenomFeatureTableSourceDDBJ.o: GAParser.h

GAGenomFeatureTableSourceEmbl.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableSourceEmbl.o: GAGenomFeatureTableSourceEmbl.h
GAGenomFeatureTableSourceEmbl.o: GAGenomUtilities.h
GAGenomFeatureTableSourceEmbl.o: GAParser.h

GAGenomFeatureTableSourceGenBank.o: GAGenomFeatureTableSource.h
GAGenomFeatureTableSourceGenBank.o: GAGenomFeatureTableSourceGenBank.h
GAGenomFeatureTableSourceGenBank.o: GAGenomUtilities.h
GAGenomFeatureTableSourceGenBank.o: GAParser.h

GAGenomGenBank.o: GAGenom.h
GAGenomGenBank.o: GAGenomFeatureTable.h
GAGenomGenBank.o: GAGenomFeatureTableGenBank.h
GAGenomGenBank.o: GAGenomFeatureTableSource.h
GAGenomGenBank.o: GAGenomFeatureTableSourceGenBank.h
GAGenomGenBank.o: GAGenomGenBank.h
GAGenomGenBank.o: GAGenomGene.h
GAGenomGenBank.o: GAGenomGeneGenBank.h
GAGenomGenBank.o: GAGenomGeneLocation.h
GAGenomGenBank.o: GAGenomGeneLocationGenBank.h
GAGenomGenBank.o: GAGenomReference.h
GAGenomGenBank.o: GAGenomReferenceGenBank.h
GAGenomGenBank.o: GAGenomUtilities.h
GAGenomGenBank.o: GAParser.h

GAGenomGene.o: GAGenomGene.h
GAGenomGene.o: GAParser.h

GAGenomGeneDDBJ.o: GAGenomGene.h
GAGenomGeneDDBJ.o: GAGenomGeneDDBJ.h
GAGenomGeneDDBJ.o: GAGenomGeneLocation.h
GAGenomGeneDDBJ.o: GAGenomGeneLocationDDBJ.h
GAGenomGeneDDBJ.o: GAGenomUtilities.h
GAGenomGeneDDBJ.o: GAParser.h

GAGenomGeneEmbl.o: GAGenomGene.h
GAGenomGeneEmbl.o: GAGenomGeneEmbl.h
GAGenomGeneEmbl.o: GAGenomGeneLocation.h
GAGenomGeneEmbl.o: GAGenomGeneLocationEmbl.h
GAGenomGeneEmbl.o: GAGenomUtilities.h
GAGenomGeneEmbl.o: GAParser.h

GAGenomGeneGenBank.o: GAGenomGene.h
GAGenomGeneGenBank.o: GAGenomGeneGenBank.h
GAGenomGeneGenBank.o: GAGenomGeneLocation.h
GAGenomGeneGenBank.o: GAGenomGeneLocationGenBank.h
GAGenomGeneGenBank.o: GAGenomUtilities.h
GAGenomGeneGenBank.o: GAParser.h

GAGenomGeneLocation.o: GAGenomGeneLocation.h
GAGenomGeneLocation.o: GAParser.h

GAGenomGeneLocationDDBJ.o: GAGenomGeneLocation.h
GAGenomGeneLocationDDBJ.o: GAGenomGeneLocationDDBJ.h
GAGenomGeneLocationDDBJ.o: GAGenomUtilities.h
GAGenomGeneLocationDDBJ.o: GAParser.h

GAGenomGeneLocationEmbl.o: GAGenomGeneLocation.h
GAGenomGeneLocationEmbl.o: GAGenomGeneLocationEmbl.h
GAGenomGeneLocationEmbl.o: GAGenomUtilities.h
GAGenomGeneLocationEmbl.o: GAParser.h

GAGenomGeneLocationGenBank.o: GAGenomGeneLocation.h
GAGenomGeneLocationGenBank.o: GAGenomGeneLocationGenBank.h
GAGenomGeneLocationGenBank.o: GAGenomUtilities.h
GAGenomGeneLocationGenBank.o: GAParser.h

GAGenomImport.o: ../GENOM_IMPORT/GAGenomFeatureTableEmbl.h
GAGenomImport.o: ../GENOM_IMPORT/GAGenomFeatureTableSourceEmbl.h
GAGenomImport.o: ../GENOM_IMPORT/GAGenomGeneEmbl.h
GAGenomImport.o: ../GENOM_IMPORT/GAGenomGeneLocationEmbl.h
GAGenomImport.o: ../GENOM_IMPORT/GAGenomReferenceEmbl.h
GAGenomImport.o: GAGenom.h
GAGenomImport.o: GAGenomEmbl.h
GAGenomImport.o: GAGenomFeatureTable.h
GAGenomImport.o: GAGenomFeatureTableEmbl.h
GAGenomImport.o: GAGenomFeatureTableSource.h
GAGenomImport.o: GAGenomFeatureTableSourceEmbl.h
GAGenomImport.o: GAGenomGene.h
GAGenomImport.o: GAGenomGeneEmbl.h
GAGenomImport.o: GAGenomGeneLocation.h
GAGenomImport.o: GAGenomGeneLocationEmbl.h
GAGenomImport.o: GAGenomImport.h
GAGenomImport.o: GAGenomReference.h
GAGenomImport.o: GAGenomReferenceEmbl.h
GAGenomImport.o: GAGenomUtilities.h
GAGenomImport.o: GAParser.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/ad_prot.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/adGene.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/arb_assert.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/arbdb.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/arbdbt.h
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/awt.hxx
GAGenomImport.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx

GAGenomReference.o: GAGenomReference.h
GAGenomReference.o: GAParser.h

GAGenomReferenceDDBJ.o: GAGenomReference.h
GAGenomReferenceDDBJ.o: GAGenomReferenceDDBJ.h
GAGenomReferenceDDBJ.o: GAGenomUtilities.h
GAGenomReferenceDDBJ.o: GAParser.h

GAGenomReferenceEmbl.o: GAGenomReference.h
GAGenomReferenceEmbl.o: GAGenomReferenceEmbl.h
GAGenomReferenceEmbl.o: GAGenomUtilities.h
GAGenomReferenceEmbl.o: GAParser.h

GAGenomReferenceGenBank.o: GAGenomReference.h
GAGenomReferenceGenBank.o: GAGenomReferenceGenBank.h
GAGenomReferenceGenBank.o: GAGenomUtilities.h
GAGenomReferenceGenBank.o: GAParser.h

GAGenomUtilities.o: GAGenomUtilities.h

GAParser.o: GAParser.h
