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

interface.o:  
interface.o:  
interface.o:  
interface.o:  
interface.o:   clustalw.h
interface.o: general.h param.h
sequence.o:  
sequence.o:  
sequence.o:  
sequence.o: clustalw.h general.h
showpair.o:  
showpair.o:  
showpair.o:  
showpair.o: clustalw.h general.h
malign.o:  
malign.o:  
malign.o:  
malign.o: clustalw.h general.h
util.o: 
util.o:  
util.o:  
util.o:   
util.o:   clustalw.h general.h
trees.o:  
trees.o:  
trees.o:  
trees.o: clustalw.h general.h dayhoff.h
gcgcheck.o:  
gcgcheck.o: 
prfalign.o: 
prfalign.o:  
prfalign.o:  
prfalign.o:  clustalw.h general.h
pairalign.o: 
pairalign.o:  
pairalign.o:  
pairalign.o:  clustalw.h general.h
calcgapcoeff.o:  
calcgapcoeff.o:  
calcgapcoeff.o:  
calcgapcoeff.o: clustalw.h general.h
calcprf1.o:  
calcprf1.o:  
calcprf1.o:  
calcprf1.o: clustalw.h general.h
calcprf2.o:  
calcprf2.o:  
calcprf2.o:  
calcprf2.o: clustalw.h general.h
calctree.o: 
calctree.o:  
calctree.o:  
calctree.o:  
calctree.o:   clustalw.h
calctree.o: general.h
readmat.o:  
readmat.o:  
readmat.o:  
readmat.o:  clustalw.h general.h matrices.h
alnscore.o: 
alnscore.o:  
alnscore.o: clustalw.h general.h
random.o: 
clustalw.o:  
clustalw.o:  
clustalw.o:  clustalw.h general.h
amenu.o:  
amenu.o:  
amenu.o:  
amenu.o:  
amenu.o:  
amenu.o:   clustalw.h general.h
