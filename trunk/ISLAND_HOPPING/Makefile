# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cpp

CPP_OBJECTS = island_hopping.o i-hopper.o align.o memory.o trnsprob.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.c.o:
	$(ACC) $(cflags) -DARB -c $<  $(CPPINCLUDES)
.cpp.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cpp) $(CPP_OBJECTS:.o=.c)
# DO NOT DELETE

island_hopping.o: i-hopper.h defs.h
i-hopper.o:  
i-hopper.o:  
i-hopper.o: 
i-hopper.o:  
i-hopper.o:  
i-hopper.o:  
i-hopper.o: 
i-hopper.o: 
i-hopper.o:   memory.h
i-hopper.o: i-hopper.h defs.h
align.o:   memory.h
align.o: trnsprob.h i-hopper.h defs.h
trnsprob.o:  defs.h memory.h trnsprob.h
