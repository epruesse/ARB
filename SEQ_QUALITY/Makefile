# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

CPP_OBJECTS = SQ_main.o SQ_functions.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

# DO NOT DELETE THIS LINE -- make depend depends on it.
