.SUFFIXES: .o .c
 
OBJECTS = interface.o sequence.o showpair.o malign.o \
	 util.o trees.o gcgcheck.o prfalign.o pairalign.o \
	 calcgapcoeff.o calcprf1.o calcprf2.o calctree.o \
	 readmat.o alnscore.o random.o clustalw.o amenu.o
 
 
$(ARBHOME)/bin/clustalw : $(OBJECTS)
	 $(ACC) $(cflags) -o $@ $(OBJECTS) -lm
 
.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECTS:.o=.c)

# DO NOT DELETE THIS LINE -- make depend depends on it.


alnscore.o: clustalw.h general.h
amenu.o:  clustalw.h
amenu.o: general.h
calcgapcoeff.o: clustalw.h general.h
calcprf1.o: clustalw.h general.h
calcprf2.o: clustalw.h general.h
calctree.o: clustalw.h general.h
clustalw.o: clustalw.h general.h
interface.o: clustalw.h general.h param.h
malign.o:  clustalw.h
malign.o: general.h
pairalign.o: clustalw.h general.h
prfalign.o: clustalw.h general.h
readmat.o:  clustalw.h
readmat.o: general.h matrices.h
sequence.o: clustalw.h general.h
showpair.o: clustalw.h general.h
trees.o:  clustalw.h
trees.o: dayhoff.h general.h
util.o:  clustalw.h
util.o: general.h
