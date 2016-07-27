# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

cma_cxxflags:=$(cxxflags)
cma_cflags:= $(cflags)

SUPPRESS_WARNINGS=1# set to 0 to show warnings

ifeq ($(SUPPRESS_WARNINGS),1)
cma_cxxflags:= $(subst -Weffc++,,$(cma_cxxflags))
cma_cxxflags:= $(subst -Wctor-dtor-privacy,,$(cma_cxxflags))
cma_cxxflags:= $(subst -Wlogical-op,,$(cma_cxxflags))
cma_cxxflags:= $(subst -Wunused-local-typedefs,,$(cma_cxxflags))
cma_cxxflags:= $(cma_cxxflags) -Wno-deprecated-declarations -Wno-ignored-attributes
cma_cflags:= $(subst -Wshadow,,$(cma_cflags))
cma_cflags+= -Wno-unused-local-typedefs# this is included in -Wall
endif

cma_cflags+= -I../HEADERLIBS

$(MAIN): $(CPP_OBJECTS) warn
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $(MAIN) $(CPP_OBJECTS) || ( \
		echo "$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)"; \
		$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS) ; \
	)

warn:
ifeq ($(SUPPRESS_WARNINGS),1)
	@echo "------------------------------------------------------------------------------------------"
	@echo "Please note: Several compiler warnings have been disabled in RNACMA"
	@echo "(namely -Weffc++, -Wctor-dtor-privacy, -Wlogical-op, -Wshadow, -Wunused-local-typedefs,"
	@echo "  -Wdeprecated-declarations -Wignored-attributes) to avoid Eigen spams here"
	@echo "------------------------------------------------------------------------------------------"
endif

.cxx.o: warn
	$(A_CXX) $(cma_cflags) $(cma_cxxflags) -c $<  $(CXX_INCLUDES)  $(POST_COMPILE) $<


$(CPP_OBJECTS) : Makefile

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

AlignedSequenceLoader.o: AlignedSequenceLoader.h
AlignedSequenceLoader.o: Cma.h
AlignedSequenceLoader.o: dbconn.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/ad_prot.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arb_assert.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arb_core.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arb_error.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arb_msg.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arb_string.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arbdb.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arbdbt.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/arbtools.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/attributes.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/cxxforward.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/downcast.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/dupstr.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/gccver.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/smartptr.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/static_assert.h
AlignedSequenceLoader.o: $(ARBHOME)/INCLUDE/test_global.h

Analyser.o: AlignedSequenceLoader.h
Analyser.o: Analyser.h
Analyser.o: Cma.h
Analyser.o: dbconn.h
Analyser.o: $(ARBHOME)/INCLUDE/ad_prot.h
Analyser.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
Analyser.o: $(ARBHOME)/INCLUDE/arb_assert.h
Analyser.o: $(ARBHOME)/INCLUDE/arb_core.h
Analyser.o: $(ARBHOME)/INCLUDE/arb_error.h
Analyser.o: $(ARBHOME)/INCLUDE/arb_msg.h
Analyser.o: $(ARBHOME)/INCLUDE/arb_string.h
Analyser.o: $(ARBHOME)/INCLUDE/arbdb.h
Analyser.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Analyser.o: $(ARBHOME)/INCLUDE/arbdbt.h
Analyser.o: $(ARBHOME)/INCLUDE/arbtools.h
Analyser.o: $(ARBHOME)/INCLUDE/attributes.h
Analyser.o: $(ARBHOME)/INCLUDE/cxxforward.h
Analyser.o: $(ARBHOME)/INCLUDE/downcast.h
Analyser.o: $(ARBHOME)/INCLUDE/dupstr.h
Analyser.o: $(ARBHOME)/INCLUDE/gccver.h
Analyser.o: $(ARBHOME)/INCLUDE/smartptr.h
Analyser.o: $(ARBHOME)/INCLUDE/static_assert.h
Analyser.o: $(ARBHOME)/INCLUDE/test_global.h

Cma.o: Cma.h
Cma.o: $(ARBHOME)/INCLUDE/cxxforward.h
Cma.o: $(ARBHOME)/INCLUDE/gccver.h

dbconn.o: dbconn.h
dbconn.o: $(ARBHOME)/INCLUDE/ad_prot.h
dbconn.o: $(ARBHOME)/INCLUDE/arb_assert.h
dbconn.o: $(ARBHOME)/INCLUDE/arb_core.h
dbconn.o: $(ARBHOME)/INCLUDE/arb_error.h
dbconn.o: $(ARBHOME)/INCLUDE/arb_msg.h
dbconn.o: $(ARBHOME)/INCLUDE/arb_string.h
dbconn.o: $(ARBHOME)/INCLUDE/arbdb.h
dbconn.o: $(ARBHOME)/INCLUDE/arbdb_base.h
dbconn.o: $(ARBHOME)/INCLUDE/arbtools.h
dbconn.o: $(ARBHOME)/INCLUDE/attributes.h
dbconn.o: $(ARBHOME)/INCLUDE/cxxforward.h
dbconn.o: $(ARBHOME)/INCLUDE/dupstr.h
dbconn.o: $(ARBHOME)/INCLUDE/gccver.h
dbconn.o: $(ARBHOME)/INCLUDE/smartptr.h
dbconn.o: $(ARBHOME)/INCLUDE/test_global.h
