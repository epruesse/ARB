.SUFFIXES: .o .c

OBJECTS = clustalv.o amenu.o myers.o sequence.o showpair.o \
          upgma.o util.o trees.o gcgcheck.o


$(ARBHOME)/bin/clustalv : $(OBJECTS)
	$(ACC) $(cflags) -o $@ $(OBJECTS) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECTS:.o=.c)

# DO NOT DELETE THIS LINE -- make depend depends on it.

clustalv.o:  clustalv.h general.h matrices.h
amenu.o:   clustalv.h
amenu.o: general.h
myers.o:  clustalv.h general.h
sequence.o:   clustalv.h
sequence.o: general.h
showpair.o:  clustalv.h general.h
upgma.o:  clustalv.h general.h
util.o: clustalv.h general.h
trees.o:  clustalv.h general.h
