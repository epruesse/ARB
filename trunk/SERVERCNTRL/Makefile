# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx
OBJECTS = servercntrl.o
$(MAIN): $(OBJECTS)
	$(AR) $(@) $(OBJECTS)
.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)
.cxx.o: 
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)
proto:
	rm -f servercntrl_prototypes.h
	../MAKEBIN/aisc_mkpt -C $(OBJECTS:.o=.c) >servercntrl_prototypes.h
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.
