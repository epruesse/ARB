# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cpp .depend

# --------------------------------------------------------------------------------

# objects for library
CPP_OBJECTS =   \
		GeneEmbl.o \
		GenomEmbl.o \
		GenomFeatureTableEmbl.o \
		GenomLocation.o \
		GenomReferenceEmbl.o \
		GenomSourceFeatureEmbl.o \
		GenomUtilities.o \

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
	rm *.o
	rm *.a
	rm *.bak
	rm *~
	rm *#
	rm *\%

#.IGNORE:
#	clean

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

