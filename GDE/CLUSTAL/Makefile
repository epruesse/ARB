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


amenu.o:  clustalv.h
amenu.o: general.h
clustalv.o: clustalv.h general.h matrices.h
myers.o:  clustalv.h
myers.o: general.h
sequence.o: clustalv.h general.h
showpair.o: clustalv.h general.h
trees.o:  clustalv.h
trees.o: general.h
upgma.o:  clustalv.h
upgma.o: general.h
util.o:  clustalv.h
util.o: general.h
