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

clustalv.o:  
clustalv.o:  
clustalv.o:  clustalv.h general.h
clustalv.o: matrices.h
amenu.o:  
amenu.o:  
amenu.o:  
amenu.o: clustalv.h general.h
myers.o:  
myers.o:  
myers.o:  clustalv.h general.h
sequence.o:  
sequence.o:  
sequence.o:  
sequence.o: clustalv.h general.h
showpair.o:  
showpair.o:  
showpair.o:  
showpair.o: clustalv.h general.h
upgma.o:  
upgma.o:  
upgma.o:  clustalv.h general.h
util.o: 
util.o:  
util.o:  
util.o:   
util.o:   clustalv.h general.h
trees.o:  
trees.o:  
trees.o:  
trees.o: clustalv.h general.h
gcgcheck.o:  
gcgcheck.o: 
