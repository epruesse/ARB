# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c

OBJECTS = mkptypes.o
PROG = aisc_mkpt

$(MAIN): $(PROG)

$(PROG):$(OBJECTS)
	$(ACC) $(cflags) -o $@ $(OBJECTS)

.c.o: 
	$(ACC) $(cflags) -c $<

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.c)

# DO NOT DELETE THIS LINE -- make depend depends on it.

mkptypes.o:  mkptypes.h patchlev.h
