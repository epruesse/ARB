.SUFFIXES: .o .c .depend

LIBS    = -lm -lc
PROGS   =          clique consense contml contrast dnacomp dnadist \
                   dnainvar dnaml dnamlk dnamove dnapars dnapenny \
                   dolmove dollop dolpenny drawgram drawtree factor \
                   fitch gendist kitsch mix move neighbor penny \
                   protdist protpars restml retree seqboot

all:	$(PROGS:%=../../bin/%)

../../bin/dnaml:        dnaml.o dnaml2.o
	$(ACC) $(cflags) -o $@ dnaml.o dnaml2.o $(LIBS)

../../bin/dnamlk:       dnamlk.o dnamlk2.o
	$(ACC) $(cflags) -o $@ dnamlk.o dnamlk2.o $(LIBS)

../../bin/drawgram:     drawgram.o drawgraphics.o
	$(ACC) $(cflags) -o $@ drawgram.o drawgraphics.o $(LIBS)

../../bin/drawtree:     drawtree.o drawgraphics.o
	$(ACC) $(cflags) -o $@ drawtree.o drawgraphics.o $(LIBS)

../../bin/mix:     mix.o mix2.o
	$(ACC) $(cflags) -o $@ mix.o mix2.o $(LIBS)

../../bin/restml:     restml.o restml2.o
	$(ACC) $(cflags) -o $@ restml.o restml2.o $(LIBS)

../../bin/protml:     protml.o p2clib.o
	$(ACC) $(cflags) -o $@ protml.o p2clib.o $(LIBS)

../../bin/%: %.c
	$(ACC) $(cflags) -o $@ $< $(LIBS)

.c.o :
	$(ACC) -c $(cflags) $<

DEPENDS = $(notdir $(PROGS:%=%.depend))
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

clean:
	@rm $(PROGS:%=../../bin/%) *.o
# DO NOT DELETE THIS LINE -- make depend depends on it.

clique.o: phylip.h

consense.o: phylip.h

contml.o: phylip.h

contrast.o: phylip.h

dnacomp.o: phylip.h

dnadist.o: phylip.h

dnainvar.o: phylip.h

dnaml.o: phylip.h

dnamlk.o: phylip.h

dnamove.o: phylip.h

dnapars.o: phylip.h

dnapenny.o: phylip.h

dolmove.o: phylip.h

dollop.o: phylip.h

dolpenny.o: phylip.h

drawgram.o: drawgraphics.h phylip.h

drawtree.o: drawgraphics.h phylip.h

factor.o: phylip.h

fitch.o: phylip.h

gendist.o: phylip.h

kitsch.o: phylip.h

mix.o: phylip.h

move.o: phylip.h

neighbor.o: phylip.h

penny.o: phylip.h

protdist.o: phylip.h

protpars.o: phylip.h

restml.o: phylip.h

retree.o: phylip.h

seqboot.o: phylip.h
