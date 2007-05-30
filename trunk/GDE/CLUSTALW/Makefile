.SUFFIXES: .o .c .depend

SOURCE_FILES = $(wildcard *.c)

OBJECTS = $(SOURCE_FILES:.c=.o)

BINARY=$(ARBHOME)/bin/clustalw

$(BINARY) : $(OBJECTS)
	 $(ACC) $(cflags) -o $@ $(OBJECTS) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

clean:
	rm -f *.o $(BINARY)

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

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

alnscore.o: clustalw.h
alnscore.o: general.h

amenu.o: clustalw.h
amenu.o: general.h

calcgapcoeff.o: clustalw.h
calcgapcoeff.o: general.h

calcprf1.o: clustalw.h
calcprf1.o: general.h

calcprf2.o: clustalw.h
calcprf2.o: general.h

calctree.o: clustalw.h
calctree.o: general.h

clustalw.o: clustalw.h
clustalw.o: general.h

interface.o: clustalw.h
interface.o: general.h
interface.o: param.h

malign.o: clustalw.h
malign.o: general.h

pairalign.o: clustalw.h
pairalign.o: general.h

prfalign.o: clustalw.h
prfalign.o: general.h

readmat.o: clustalw.h
readmat.o: general.h
readmat.o: matrices.h

sequence.o: clustalw.h
sequence.o: general.h

showpair.o: clustalw.h
showpair.o: general.h

trees.o: clustalw.h
trees.o: dayhoff.h
trees.o: general.h

util.o: clustalw.h
util.o: general.h
