.SUFFIXES: .o .c .depend

LIBS    = -lm -lc
PROGS_old   =      clique consense contml contrast dnacomp dnadist \
                   dnainvar dnaml dnamlk dnamove dnapars dnapenny \
                   dolmove dollop dolpenny drawgram drawtree factor \
                   fitch gendist kitsch mix move neighbor penny \
                   protdist protpars restml retree seqboot

PROGS   =          clique consense contml contrast dnacomp dnadist \
                   dnainvar dnaml dnamlk dnamove dnapars dnapenny \
                   dolmove dollop dolpenny factor fitch gendist kitsch \
                   mix move neighbor pars penny proml promlk protdist \
                   protpars restdist restml retree seqboot treedist \
                   drawgram drawtree

BIN=$(ARBHOME)/bin
BINARIES=$(addprefix $(BIN)/,$(PROGS))

DLIBS=$(LIBS) $(XLIBS) -lXaw


all: $(BINARIES)

$(BIN)/clique:       clique.o  disc.o  phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/consense:     consense.o  phylip.o  cons.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/contml:       contml.o  cont.o  phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/contrast:     contrast.o  cont.o  phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnacomp:      dnacomp.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnadist:      dnadist.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnainvar:     dnainvar.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnaml:        dnaml.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnamlk:       dnamlk.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnamove:      dnamove.o seq.o moves.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnapenny:     dnapenny.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dnapars:      dnapars.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dolmove:      dolmove.o disc.o moves.o dollo.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dollop:       dollop.o disc.o dollo.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/dolpenny:     dolpenny.o disc.o dollo.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/drawgram:     drawgram.o draw.o draw2.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(DLIBS)

$(BIN)/drawtree:     drawtree.o draw.o draw2.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(DLIBS)

$(BIN)/factor:       factor.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/fitch:        fitch.o dist.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/gendist:      gendist.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/kitsch:       kitsch.o dist.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/mix:          mix.o disc.o wagner.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/move:         move.o disc.o moves.o wagner.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/neighbor:     neighbor.o dist.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/pars:	     pars.o  discrete.o  phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/penny:        penny.o disc.o wagner.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/proml:        proml.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/promlk:       promlk.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/protdist:     protdist.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/protpars:     protpars.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/restdist:     restdist.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/restml:       restml.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/retree:       retree.o moves.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/seqboot:      seqboot.o seq.o phylip.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)

$(BIN)/treedist:     treedist.o phylip.o cons.o
	$(ACC) $(cflags) -o $@ $^ $(LIBS)



#$(BIN)/clique: clique.o disc.o phylip.o
#	$(ACC) $(cflags) -o $@ $^ $(LIBS)

#$(BIN)/dnaml: dnaml.o seq.o phylip.o
#	$(ACC) $(cflags) -o $@ $^ $(LIBS)

#../../bin/dnaml:        dnaml.o dnaml2.o
#	$(ACC) $(cflags) -o $@ dnaml.o dnaml2.o $(LIBS)

#../../bin/dnamlk:       dnamlk.o dnamlk2.o
#	$(ACC) $(cflags) -o $@ dnamlk.o dnamlk2.o $(LIBS)

#../../bin/drawgram:     drawgram.o drawgraphics.o
#	$(ACC) $(cflags) -o $@ drawgram.o drawgraphics.o $(LIBS)

#../../bin/drawtree:     drawtree.o drawgraphics.o
#	$(ACC) $(cflags) -o $@ drawtree.o drawgraphics.o $(LIBS)

#../../bin/mix:     mix.o mix2.o
#	$(ACC) $(cflags) -o $@ mix.o mix2.o $(LIBS)

#../../bin/restml:     restml.o restml2.o
#	$(ACC) $(cflags) -o $@ restml.o restml2.o $(LIBS)

#../../bin/protml:     protml.o p2clib.o
#	$(ACC) $(cflags) -o $@ protml.o p2clib.o $(LIBS)

#$(BIN)/%: %.c
#	$(ACC) $(cflags) -o $@ $^ $(LIBS)

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
	rm -f $(BINARIES) *.o
# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

clique.o: disc.h
clique.o: phylip.h

consense.o: cons.h
consense.o: phylip.h

contml.o: cont.h
contml.o: phylip.h

contrast.o: cont.h
contrast.o: phylip.h

dnacomp.o: phylip.h
dnacomp.o: seq.h

dnadist.o: phylip.h
dnadist.o: seq.h

dnainvar.o: phylip.h
dnainvar.o: seq.h

dnaml.o: phylip.h
dnaml.o: seq.h

dnamlk.o: phylip.h
dnamlk.o: seq.h

dnamove.o: moves.h
dnamove.o: phylip.h
dnamove.o: seq.h

dnapars.o: phylip.h
dnapars.o: seq.h

dnapenny.o: phylip.h
dnapenny.o: seq.h

dollop.o: disc.h
dollop.o: dollo.h
dollop.o: phylip.h

dolmove.o: disc.h
dolmove.o: dollo.h
dolmove.o: moves.h
dolmove.o: phylip.h

dolpenny.o: disc.h
dolpenny.o: dollo.h
dolpenny.o: phylip.h

drawgram.o: draw.h
drawgram.o: phylip.h

drawtree.o: draw.h
drawtree.o: phylip.h

factor.o: phylip.h

fitch.o: dist.h
fitch.o: phylip.h

gendist.o: phylip.h

kitsch.o: dist.h
kitsch.o: phylip.h

mix.o: disc.h
mix.o: phylip.h
mix.o: wagner.h

move.o: disc.h
move.o: moves.h
move.o: phylip.h
move.o: wagner.h

neighbor.o: dist.h
neighbor.o: phylip.h

pars.o: discrete.h
pars.o: phylip.h

penny.o: disc.h
penny.o: phylip.h
penny.o: wagner.h

proml.o: phylip.h
proml.o: seq.h

promlk.o: phylip.h
promlk.o: seq.h

protdist.o: phylip.h
protdist.o: seq.h

protpars.o: phylip.h
protpars.o: seq.h

restdist.o: phylip.h
restdist.o: seq.h

restml.o: phylip.h
restml.o: seq.h

retree.o: moves.h
retree.o: phylip.h

seqboot.o: phylip.h
seqboot.o: seq.h

treedist.o: cons.h
treedist.o: phylip.h
