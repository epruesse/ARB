# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cpp .depend

# --------------------------------------------------------------------------------

# objects for library
CPP_OBJECTS =   \
		GenomEmbl.o \
    GeneEmbl.o \
		GenomFeatureTableEmbl.o \
		GenomLocation.o \
		GenomReferenceEmbl.o \
		GenomSourceFeatureEmbl.o \
		GenomUtilities.o \
		GenomImport.o \

#		GenomArbConnection.o \

CREATE_TEST_EXECUTABLE=1

# additional objects for executable
EXE_CPP_OBJECTS =   \
		MainTest.o \

# --------------------------------------------------------------------------------

ifeq ($(CREATE_TEST_EXECUTABLE),0)

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

else

$(MAIN): $(CPP_OBJECTS) $(EXE_CPP_OBJECTS) Makefile
	$(XAR) $(MAIN) $(CPP_OBJECTS)
	$(CPP) $(lflags) -o test_import $(MAIN) $(EXE_CPP_OBJECTS)

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

GeneEmbl.o: GeneEmbl.h
GeneEmbl.o: GenomLocation.h
GeneEmbl.o: GenomUtilities.h

GenomEmbl.o: GeneEmbl.h
GenomEmbl.o: GenomEmbl.h
GenomEmbl.o: GenomFeatureTableEmbl.h
GenomEmbl.o: GenomLocation.h
GenomEmbl.o: GenomReferenceEmbl.h
GenomEmbl.o: GenomSourceFeatureEmbl.h
GenomEmbl.o: GenomUtilities.h

GenomFeatureTableEmbl.o: GeneEmbl.h
GenomFeatureTableEmbl.o: GenomFeatureTableEmbl.h
GenomFeatureTableEmbl.o: GenomLocation.h
GenomFeatureTableEmbl.o: GenomSourceFeatureEmbl.h
GenomFeatureTableEmbl.o: GenomUtilities.h

GenomLocation.o: GenomLocation.h

GenomReferenceEmbl.o: GenomReferenceEmbl.h
GenomReferenceEmbl.o: GenomUtilities.h

GenomSourceFeatureEmbl.o: GenomSourceFeatureEmbl.h
GenomSourceFeatureEmbl.o: GenomUtilities.h

GenomUtilities.o: GenomUtilities.h
