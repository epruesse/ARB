# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

CPP_OBJECTS = AWTI_import.o \
		      AWTI_export.o \

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE
