# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =   \
	PRD_design.o \

$(MAIN): $(CPP_OBJECTS) PRD_main.o
	$(AR) $(MAIN) $(CPP_OBJECTS)
	$(MAKE) ./prd_test

./prd_test:
	$(CPP) $(lflags) -o $@ PRD_main.o $(MAIN)


.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm *.o
	rm *.a
	rm *.bak
	rm *~
	rm *#
	rm *\%

#.IGNORE:
#	clean

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE

PRD_design.o: PRD_design.hxx
