.SUFFIXES: .o .c .depend

SOURCE_FILES = $(wildcard *.c)

OBJECTS = $(SOURCE_FILES:.c=.o)

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

alnscore.o: clustalw.h general.h

amenu.o: clustalw.h general.h

calcgapcoeff.o: clustalw.h general.h

calcprf1.o: clustalw.h general.h

calcprf2.o: clustalw.h general.h

calctree.o: clustalw.h general.h

clustalw.o: clustalw.h general.h

interface.o: clustalw.h general.h param.h

malign.o: clustalw.h general.h

pairalign.o: clustalw.h general.h

prfalign.o: clustalw.h general.h

readmat.o: clustalw.h general.h matrices.h

sequence.o: clustalw.h general.h

showpair.o: clustalw.h general.h

trees.o: clustalw.h general.h dayhoff.h

util.o: clustalw.h general.h
