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

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
	$(MAKE) ./prd_test

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


PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:  /usr/include/g++/cstdio
PRD_Design.c.o:
PRD_Design.c.o: /usr/include/g++/iostream
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/exception
PRD_Design.c.o:
PRD_Design.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/new
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o:
PRD_Design.c.o: PRD_Globals.hxx
PRD_Design.c.o: PRD_Design.hxx PRD_Item.hxx PRD_Node.hxx PRD_Range.hxx
PRD_Design.c.o: PRD_SearchFIFO.hxx PRD_SequenceIterator.hxx
PRD_Item.c.o:
PRD_Item.c.o:
PRD_Item.c.o:
PRD_Item.c.o: /usr/include/g++/cstdio
PRD_Item.c.o:
PRD_Item.c.o:
PRD_Item.c.o:
PRD_Item.c.o:
PRD_Item.c.o: PRD_Item.hxx
PRD_Node.c.o:
PRD_Node.c.o:
PRD_Node.c.o:
PRD_Node.c.o: /usr/include/g++/cstdio
PRD_Node.c.o:
PRD_Node.c.o:
PRD_Node.c.o:
PRD_Node.c.o:
PRD_Node.c.o: PRD_Node.hxx
PRD_Range.c.o:
PRD_Range.c.o:
PRD_Range.c.o:  /usr/include/g++/iostream
PRD_Range.c.o:
PRD_Range.c.o:
PRD_Range.c.o:
PRD_Range.c.o:
PRD_Range.c.o:
PRD_Range.c.o: PRD_Range.hxx
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o: /usr/include/g++/cstdio
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o:
PRD_SearchFIFO.c.o: PRD_Node.hxx PRD_Range.hxx PRD_SearchFIFO.hxx
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o: /usr/include/g++/cstdio
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o:
PRD_SequenceIterator.c.o: PRD_SequenceIterator.hxx
