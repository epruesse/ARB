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


BIN=../../bin

DLIBS=$(LIBS) $(XLIBS) -lXaw


all:	$(addprefix $(BIN)/,$(PROGS))


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
	@rm $(PROGS:%=../../bin/%) *.o
# DO NOT DELETE THIS LINE -- make depend depends on it.

clique.o: phylip.h disc.h

consense.o: phylip.h cons.h

contml.o: phylip.h cont.h

contrast.o: phylip.h cont.h

dnacomp.o: phylip.h seq.h

dnadist.o: phylip.h seq.h

dnainvar.o: phylip.h seq.h

dnaml.o: phylip.h seq.h

dnamlk.o: phylip.h seq.h

dnamove.o: phylip.h moves.h seq.h

dnapars.o: phylip.h seq.h

dnapenny.o: phylip.h seq.h

dolmove.o: phylip.h moves.h disc.h dollo.h

dollop.o: phylip.h disc.h dollo.h

dolpenny.o: phylip.h disc.h dollo.h

drawgram.o: draw.h phylip.h

drawtree.o: draw.h phylip.h

factor.o: phylip.h

fitch.o: phylip.h dist.h

gendist.o: phylip.h

kitsch.o: phylip.h dist.h

mix.o: phylip.h disc.h wagner.h

move.o: phylip.h disc.h moves.h wagner.h

neighbor.o: phylip.h dist.h

penny.o: phylip.h disc.h wagner.h

protdist.o: phylip.h seq.h

protpars.o: phylip.h seq.h

restml.o: phylip.h seq.h

retree.o: phylip.h moves.h

seqboot.o: phylip.h seq.h
