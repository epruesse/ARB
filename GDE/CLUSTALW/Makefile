.SUFFIXES: .o .c .depend

OBJECTS = interface.o sequence.o showpair.o malign.o \
	 util.o trees.o gcgcheck.o prfalign.o pairalign.o \
	 calcgapcoeff.o calcprf1.o calcprf2.o calctree.o \
	 readmat.o alnscore.o random.o clustalw.o amenu.o


$(ARBHOME)/bin/clustalw : $(OBJECTS)
	 $(ACC) $(cflags) -o $@ $(OBJECTS) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

interface.o: clustalw.h general.h param.h

sequence.o: clustalw.h general.h

showpair.o: clustalw.h general.h

malign.o: clustalw.h general.h

util.o: clustalw.h general.h

trees.o: clustalw.h general.h dayhoff.h

prfalign.o: clustalw.h general.h

pairalign.o: clustalw.h general.h

calcgapcoeff.o: clustalw.h general.h

calcprf1.o: clustalw.h general.h

calcprf2.o: clustalw.h general.h

calctree.o: clustalw.h general.h

readmat.o: clustalw.h general.h matrices.h

alnscore.o: clustalw.h general.h

clustalw.o: clustalw.h general.h

amenu.o: clustalw.h general.h
