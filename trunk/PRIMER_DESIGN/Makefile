# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

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
	$(AR) $(MAIN) $(CPP_OBJECTS)
#	$(MAKE) ./prd_test

./prd_test: $(MAIN) PRD_main.o
	$(CPP) $(lflags) -o $@ PRD_main.o $(MAIN)


.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

clean:
	-rm *.o *.a *~ prd_test *# *\%

#.IGNORE:
#	clean

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)


# DO NOT DELETE

PRD_Pair.o: PRD_Pair.hxx PRD_Item.hxx PRD_Globals.hxx 
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Pair.o:  
PRD_Item.o: PRD_Item.hxx PRD_Globals.hxx 
PRD_Item.o:  
PRD_Item.o:  
PRD_Item.o:  
PRD_Item.o:  PRD_SequenceIterator.hxx
PRD_Item.o:  
PRD_Item.o:  
PRD_Item.o:  
PRD_Item.o:  
PRD_Item.o:  
PRD_Item.o: 
PRD_Node.o: PRD_Node.hxx PRD_Globals.hxx 
PRD_Node.o:  
PRD_Node.o:  
PRD_Node.o:  
PRD_Node.o: 
PRD_Range.o: PRD_Range.hxx PRD_Globals.hxx 
PRD_Range.o:  
PRD_Range.o:  
PRD_Range.o:  
PRD_Range.o: 
PRD_SequenceIterator.o: PRD_SequenceIterator.hxx
PRD_SearchFIFO.o: PRD_SearchFIFO.hxx PRD_Globals.hxx 
PRD_SearchFIFO.o:  
PRD_SearchFIFO.o:  
PRD_SearchFIFO.o:  
PRD_SearchFIFO.o:  PRD_Node.hxx PRD_Range.hxx
PRD_Design.o: PRD_Globals.hxx  
PRD_Design.o:  
PRD_Design.o:  
PRD_Design.o:  
PRD_Design.o: PRD_Design.hxx PRD_Range.hxx PRD_Node.hxx PRD_Item.hxx
PRD_Design.o: PRD_Pair.hxx $(ARBHOME)/INCLUDE/arbdb.h
PRD_Design.o:  
PRD_Design.o:  
PRD_Design.o: 
PRD_Design.o: $(ARBHOME)/INCLUDE/ad_prot.h PRD_SequenceIterator.hxx
PRD_Design.o: PRD_SearchFIFO.hxx
primer_design.o: $(ARBHOME)/INCLUDE/arbdb.h 
primer_design.o:  
primer_design.o:  
primer_design.o:  
primer_design.o: 
primer_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
primer_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
primer_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
primer_design.o: $(ARBHOME)/INCLUDE/aw_root.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_window.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
primer_design.o: $(ARBHOME)/INCLUDE/awt.hxx primer_design.hxx
primer_design.o: PRD_Design.hxx PRD_Range.hxx PRD_Globals.hxx
primer_design.o:  
primer_design.o:  
primer_design.o:  PRD_Node.hxx PRD_Item.hxx
primer_design.o: PRD_Pair.hxx PRD_SequenceIterator.hxx
