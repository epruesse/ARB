.SUFFIXES: .o .c

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

depend:
	makedepend $(MAKEDEPENDINC) -p ../../bin/ $(PROGS:%=%.c)

clean:
	@rm $(PROGS:%=../../bin/%) *.o
# DO NOT DELETE THIS LINE -- make depend depends on it.

../../bin/clique.o: phylip.h  
../../bin/clique.o:  
../../bin/clique.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/clique.o:  
../../bin/clique.o:  
../../bin/clique.o:  
../../bin/clique.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/clique.o:  
../../bin/clique.o:  
../../bin/clique.o:  
../../bin/clique.o:  
../../bin/clique.o:  
../../bin/consense.o: phylip.h  
../../bin/consense.o:  
../../bin/consense.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/consense.o:  
../../bin/consense.o:  
../../bin/consense.o:  
../../bin/consense.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/consense.o:  
../../bin/consense.o:  
../../bin/consense.o: 
../../bin/consense.o: 
../../bin/consense.o:  
../../bin/consense.o:  
../../bin/contml.o: phylip.h  
../../bin/contml.o:  
../../bin/contml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/contml.o:  
../../bin/contml.o:  
../../bin/contml.o:  
../../bin/contml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/contml.o:  
../../bin/contml.o:  
../../bin/contml.o:  
../../bin/contml.o:  
../../bin/contml.o:  
../../bin/contrast.o: phylip.h  
../../bin/contrast.o:  
../../bin/contrast.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/contrast.o:  
../../bin/contrast.o:  
../../bin/contrast.o:  
../../bin/contrast.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/contrast.o:  
../../bin/contrast.o:  
../../bin/contrast.o: 
../../bin/contrast.o: 
../../bin/contrast.o:  
../../bin/contrast.o:  
../../bin/dnacomp.o: phylip.h  
../../bin/dnacomp.o:  
../../bin/dnacomp.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnacomp.o:  
../../bin/dnacomp.o:  
../../bin/dnacomp.o:  
../../bin/dnacomp.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnacomp.o:  
../../bin/dnacomp.o:  
../../bin/dnacomp.o:  
../../bin/dnacomp.o:  
../../bin/dnacomp.o:  
../../bin/dnadist.o: phylip.h  
../../bin/dnadist.o:  
../../bin/dnadist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnadist.o:  
../../bin/dnadist.o:  
../../bin/dnadist.o:  
../../bin/dnadist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnadist.o:  
../../bin/dnadist.o:  
../../bin/dnadist.o:  
../../bin/dnadist.o:  
../../bin/dnadist.o:  
../../bin/dnainvar.o: phylip.h  
../../bin/dnainvar.o:  
../../bin/dnainvar.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnainvar.o:  
../../bin/dnainvar.o:  
../../bin/dnainvar.o:  
../../bin/dnainvar.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnainvar.o:  
../../bin/dnainvar.o:  
../../bin/dnainvar.o: 
../../bin/dnainvar.o: 
../../bin/dnainvar.o:  
../../bin/dnainvar.o:  
../../bin/dnaml.o: phylip.h  
../../bin/dnaml.o:  
../../bin/dnaml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnaml.o:  
../../bin/dnaml.o:  
../../bin/dnaml.o:  
../../bin/dnaml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnaml.o:  
../../bin/dnaml.o:  
../../bin/dnaml.o:  
../../bin/dnaml.o:  
../../bin/dnaml.o:  
../../bin/dnamlk.o: phylip.h  
../../bin/dnamlk.o:  
../../bin/dnamlk.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnamlk.o:  
../../bin/dnamlk.o:  
../../bin/dnamlk.o:  
../../bin/dnamlk.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnamlk.o:  
../../bin/dnamlk.o:  
../../bin/dnamlk.o:  
../../bin/dnamlk.o:  
../../bin/dnamlk.o:  
../../bin/dnamove.o: phylip.h  
../../bin/dnamove.o:  
../../bin/dnamove.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnamove.o:  
../../bin/dnamove.o:  
../../bin/dnamove.o:  
../../bin/dnamove.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnamove.o:  
../../bin/dnamove.o:  
../../bin/dnamove.o:  
../../bin/dnamove.o:  
../../bin/dnamove.o:  
../../bin/dnapars.o: phylip.h  
../../bin/dnapars.o:  
../../bin/dnapars.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnapars.o:  
../../bin/dnapars.o:  
../../bin/dnapars.o:  
../../bin/dnapars.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnapars.o:  
../../bin/dnapars.o:  
../../bin/dnapars.o:  
../../bin/dnapars.o:  
../../bin/dnapars.o:  
../../bin/dnapenny.o: phylip.h  
../../bin/dnapenny.o:  
../../bin/dnapenny.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dnapenny.o:  
../../bin/dnapenny.o:  
../../bin/dnapenny.o:  
../../bin/dnapenny.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dnapenny.o:  
../../bin/dnapenny.o:  
../../bin/dnapenny.o: 
../../bin/dnapenny.o: 
../../bin/dnapenny.o:  
../../bin/dnapenny.o:  
../../bin/dolmove.o: phylip.h  
../../bin/dolmove.o:  
../../bin/dolmove.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dolmove.o:  
../../bin/dolmove.o:  
../../bin/dolmove.o:  
../../bin/dolmove.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dolmove.o:  
../../bin/dolmove.o:  
../../bin/dolmove.o:  
../../bin/dolmove.o:  
../../bin/dolmove.o:  
../../bin/dollop.o: phylip.h  
../../bin/dollop.o:  
../../bin/dollop.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dollop.o:  
../../bin/dollop.o:  
../../bin/dollop.o:  
../../bin/dollop.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dollop.o:  
../../bin/dollop.o:  
../../bin/dollop.o:  
../../bin/dollop.o:  
../../bin/dollop.o:  
../../bin/dolpenny.o: phylip.h  
../../bin/dolpenny.o:  
../../bin/dolpenny.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dolpenny.o:  
../../bin/dolpenny.o:  
../../bin/dolpenny.o:  
../../bin/dolpenny.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dolpenny.o:  
../../bin/dolpenny.o:  
../../bin/dolpenny.o: 
../../bin/dolpenny.o: 
../../bin/dolpenny.o:  
../../bin/dolpenny.o:  
../../bin/drawgram.o: drawgraphics.h phylip.h 
../../bin/drawgram.o:  
../../bin/drawgram.o: 
../../bin/drawgram.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/drawgram.o:  
../../bin/drawgram.o:  
../../bin/drawgram.o:  
../../bin/drawgram.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/drawgram.o:  
../../bin/drawgram.o:  
../../bin/drawgram.o: 
../../bin/drawgram.o: 
../../bin/drawgram.o:  
../../bin/drawgram.o:  
../../bin/drawtree.o: drawgraphics.h phylip.h 
../../bin/drawtree.o:  
../../bin/drawtree.o: 
../../bin/drawtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/drawtree.o:  
../../bin/drawtree.o:  
../../bin/drawtree.o:  
../../bin/drawtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/drawtree.o:  
../../bin/drawtree.o:  
../../bin/drawtree.o: 
../../bin/drawtree.o: 
../../bin/drawtree.o:  
../../bin/drawtree.o:  
../../bin/factor.o: phylip.h  
../../bin/factor.o:  
../../bin/factor.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/factor.o:  
../../bin/factor.o:  
../../bin/factor.o:  
../../bin/factor.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/factor.o:  
../../bin/factor.o:  
../../bin/factor.o:  
../../bin/factor.o:  
../../bin/factor.o:  
../../bin/fitch.o: phylip.h  
../../bin/fitch.o:  
../../bin/fitch.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/fitch.o:  
../../bin/fitch.o:  
../../bin/fitch.o:  
../../bin/fitch.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/fitch.o:  
../../bin/fitch.o:  
../../bin/fitch.o:  
../../bin/fitch.o:  
../../bin/fitch.o:  
../../bin/gendist.o: phylip.h  
../../bin/gendist.o:  
../../bin/gendist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/gendist.o:  
../../bin/gendist.o:  
../../bin/gendist.o:  
../../bin/gendist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/gendist.o:  
../../bin/gendist.o:  
../../bin/gendist.o:  
../../bin/gendist.o:  
../../bin/gendist.o:  
../../bin/kitsch.o: phylip.h  
../../bin/kitsch.o:  
../../bin/kitsch.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/kitsch.o:  
../../bin/kitsch.o:  
../../bin/kitsch.o:  
../../bin/kitsch.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/kitsch.o:  
../../bin/kitsch.o:  
../../bin/kitsch.o:  
../../bin/kitsch.o:  
../../bin/kitsch.o:  
../../bin/mix.o: phylip.h  
../../bin/mix.o:  
../../bin/mix.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/mix.o:  
../../bin/mix.o:  
../../bin/mix.o:  
../../bin/mix.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/mix.o:  
../../bin/mix.o:  
../../bin/mix.o:  
../../bin/mix.o:  
../../bin/mix.o:  
../../bin/move.o: phylip.h  
../../bin/move.o:  
../../bin/move.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/move.o:  
../../bin/move.o:  
../../bin/move.o:  
../../bin/move.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/move.o:  
../../bin/move.o:  
../../bin/move.o:  
../../bin/move.o:  
../../bin/move.o:  
../../bin/neighbor.o: phylip.h  
../../bin/neighbor.o:  
../../bin/neighbor.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/neighbor.o:  
../../bin/neighbor.o:  
../../bin/neighbor.o:  
../../bin/neighbor.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/neighbor.o:  
../../bin/neighbor.o:  
../../bin/neighbor.o: 
../../bin/neighbor.o: 
../../bin/neighbor.o:  
../../bin/neighbor.o:  
../../bin/penny.o: phylip.h  
../../bin/penny.o:  
../../bin/penny.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/penny.o:  
../../bin/penny.o:  
../../bin/penny.o:  
../../bin/penny.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/penny.o:  
../../bin/penny.o:  
../../bin/penny.o:  
../../bin/penny.o:  
../../bin/penny.o:  
../../bin/protdist.o: phylip.h  
../../bin/protdist.o:  
../../bin/protdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/protdist.o:  
../../bin/protdist.o:  
../../bin/protdist.o:  
../../bin/protdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/protdist.o:  
../../bin/protdist.o:  
../../bin/protdist.o: 
../../bin/protdist.o: 
../../bin/protdist.o:  
../../bin/protdist.o:  
../../bin/protpars.o: phylip.h  
../../bin/protpars.o:  
../../bin/protpars.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/protpars.o:  
../../bin/protpars.o:  
../../bin/protpars.o:  
../../bin/protpars.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/protpars.o:  
../../bin/protpars.o:  
../../bin/protpars.o: 
../../bin/protpars.o: 
../../bin/protpars.o:  
../../bin/protpars.o:  
../../bin/restml.o: phylip.h  
../../bin/restml.o:  
../../bin/restml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/restml.o:  
../../bin/restml.o:  
../../bin/restml.o:  
../../bin/restml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/restml.o:  
../../bin/restml.o:  
../../bin/restml.o:  
../../bin/restml.o:  
../../bin/restml.o:  
../../bin/retree.o: phylip.h  
../../bin/retree.o:  
../../bin/retree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/retree.o:  
../../bin/retree.o:  
../../bin/retree.o:  
../../bin/retree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/retree.o:  
../../bin/retree.o:  
../../bin/retree.o:  
../../bin/retree.o:  
../../bin/retree.o:  
../../bin/seqboot.o: phylip.h  
../../bin/seqboot.o:  
../../bin/seqboot.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/seqboot.o:  
../../bin/seqboot.o:  
../../bin/seqboot.o:  
../../bin/seqboot.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/seqboot.o:  
../../bin/seqboot.o:  
../../bin/seqboot.o:  
../../bin/seqboot.o:  
../../bin/seqboot.o:  
