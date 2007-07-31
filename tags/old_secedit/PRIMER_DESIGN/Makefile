# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPPINCLUDES = $(CPPINCLUDES) #-I~/STL

CPP_OBJECTS =   \
	PRD_Pair.o \
	PRD_Item.o \
	PRD_Node.o \
	PRD_Range.o \
	PRD_SequenceIterator.o \
	PRD_SearchFIFO.o \
	PRD_Design.o \
	primer_design.o \

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

./prd_test: $(MAIN) PRD_main.o
	$(CPP) $(lflags) -o $@ PRD_main.o $(MAIN)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

#.IGNORE:
#	clean

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

PRD_Design.o: PRD_Design.hxx
PRD_Design.o: PRD_Globals.hxx
PRD_Design.o: PRD_Item.hxx
PRD_Design.o: PRD_Node.hxx
PRD_Design.o: PRD_Pair.hxx
PRD_Design.o: PRD_Range.hxx
PRD_Design.o: PRD_SearchFIFO.hxx
PRD_Design.o: PRD_SequenceIterator.hxx
PRD_Design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arbdb.h

PRD_Item.o: PRD_Globals.hxx
PRD_Item.o: PRD_Item.hxx
PRD_Item.o: PRD_SequenceIterator.hxx

PRD_Node.o: PRD_Globals.hxx
PRD_Node.o: PRD_Node.hxx

PRD_Pair.o: PRD_Globals.hxx
PRD_Pair.o: PRD_Item.hxx
PRD_Pair.o: PRD_Pair.hxx

PRD_Range.o: PRD_Globals.hxx
PRD_Range.o: PRD_Range.hxx

PRD_SearchFIFO.o: PRD_Globals.hxx
PRD_SearchFIFO.o: PRD_Node.hxx
PRD_SearchFIFO.o: PRD_Range.hxx
PRD_SearchFIFO.o: PRD_SearchFIFO.hxx

PRD_SequenceIterator.o: PRD_Globals.hxx
PRD_SequenceIterator.o: PRD_SequenceIterator.hxx

primer_design.o: PRD_Design.hxx
primer_design.o: PRD_Globals.hxx
primer_design.o: PRD_Item.hxx
primer_design.o: PRD_Node.hxx
primer_design.o: PRD_Pair.hxx
primer_design.o: PRD_Range.hxx
primer_design.o: PRD_SequenceIterator.hxx
primer_design.o: primer_design.hxx
primer_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
primer_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
primer_design.o: $(ARBHOME)/INCLUDE/adGene.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
primer_design.o: $(ARBHOME)/INCLUDE/arbdb.h
primer_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
primer_design.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_root.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_window.hxx
primer_design.o: $(ARBHOME)/INCLUDE/awt.hxx
primer_design.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
primer_design.o: $(ARBHOME)/INCLUDE/GEN.hxx
