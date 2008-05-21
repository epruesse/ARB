.SUFFIXES: .o .c .depend

INSTALLDIR = ../../bin

LIBS = -lm -lc
CP = cp
NUCLE = -DNUC
NJLE = -DNJ
SDLE = -DSD
DEFINE = #
CFLAGS = $(cflags)
CC=$(ACC)
PRGS = protml
# protst nucml nucst njdist totalml
# prottpm nuctpm tmlprot protdml nucdml tridist sddist

OBJML = getseq.o seqproc.o mltree.o altree.o qltree.o sltree.o \
	njtree.o njmtree.o prtree.o pstree.o matrixut.o mygetopt.o
OBJPML = protml.o protproc.o dyhfjtt.o mtrev24.o tranprb.o distan.o mlklhd.o
OBJNML = Nucml.o nucproc.o Ntranprb.o Ndistan.o Nmlklhd.o

OBJST = getseq.o seqproc.o matrixut.o mygetopt.o
OBJPST = protst.o seqstat.o protproc.o
OBJNST = Nucst.o Nseqstat.o nucproc.o

OBJD = distproc.o Dpstree.o matrixut.o mygetopt.o
OBJTD = tridist.o triproc.o
OBJND = Njdist.o njproc.o
OBJSD = sddist.o sdproc.o

OBJTO = totalml.o matrixut.o mygetopt.o

OBJPTPM = Prottpm.o tpmproc.o protproc.o dyhfjtt.o mtrev24.o Trnprbt.o distan.o mlklhd.o
OBJNTPM = Nuctpm.o Ntpmproc.o nucproc.o Ntrnprbt.o Ndistan.o Nmlklhd.o

OBJTML = getseq.o seqproc.o triadml.o mltree.o prtree.o matrixut.o mygetopt.o
OBJTMLP = tmlprot.o protproc.o dyhfjtt.o mtrev24.o tranprb.o distan.o mlklhd.o

OBJPDML = protdml.o dmlproc.o protproc.o dyhfjtt.o mtrev24.o tranprb.o distan.o mlklhd.o
OBJNDML = Nucdml.o Ndmlproc.o nucproc.o Ntranprb.o Ndistan.o Nmlklhd.o

# all objects:
OBJECTS = Dpstree.o Ndistan.o Njdist.o Nmlklhd.o Nseqstat.o Ntranprb.o Nucml.o Nucst.o Nuctpm.o \
Prottpm.o abratio.o altree.o distan.o distproc.o dyhfjtt.o getseq.o matrixut.o mlklhd.o \
mltree.o mtrev22.o mtrev24.o mygetopt.o njmtree.o njproc.o njtree.o nucproc.o optimtpm.o \
protml.o protproc.o protst.o prtree.o pstree.o qltree.o seqproc.o seqstat.o sltree.o \
totalml.o tranprb.o tridist.o triproc.o

SOURCES_TML = $(OBJML:.o=.c) $(OBJPML:.o=.c)

all : prot_tml.h $(PRGS:%=../../bin/%)

prot_tml.h: $(SOURCES_TML)
	rm -f $@
	../../MAKEBIN/aisc_mkpt $(SOURCES_TML) >$@

$(INSTALLDIR)/protml : $(OBJML) $(OBJPML)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJML) $(OBJPML) $(LIBS)

$(INSTALLDIR)/nucml : $(OBJML) $(OBJNML)
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -o $@ $(OBJML) $(OBJNML) $(LIBS)

$(INSTALLDIR)/protst : $(OBJST) $(OBJPST)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJST) $(OBJPST) $(LIBS)

$(INSTALLDIR)/nucst : $(OBJST) $(OBJNST)
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -o $@ $(OBJST) $(OBJNST) $(LIBS)

$(INSTALLDIR)/tridist : $(OBJD) $(OBJTD)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJD) $(OBJTD) $(LIBS)

$(INSTALLDIR)/njdist : $(OBJD) $(OBJND)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJD) $(OBJND) $(LIBS)

$(INSTALLDIR)/totalml : $(OBJTO)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJTO) $(LIBS)

$(INSTALLDIR)/sddist : $(OBJD) $(OBJSD)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJD) $(OBJSD) $(LIBS)

$(INSTALLDIR)/prottpm : $(OBJML) $(OBJPTPM)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJML) $(OBJPTPM) $(LIBS)

$(INSTALLDIR)/nuctpm : $(OBJML) $(OBJNTPM)
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -o $@ $(OBJML) $(OBJNTPM) $(LIBS)

$(INSTALLDIR)/tmlprot : $(OBJTML) $(OBJTMLP)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJTML) $(OBJTMLP) $(LIBS)

$(INSTALLDIR)/protdml : $(OBJML) $(OBJPDML)
	$(CC) $(CFLAGS) $(DEFINE) -o $@ $(OBJML) $(OBJPDML) $(LIBS)

$(INSTALLDIR)/nucdml : $(OBJML) $(OBJNDML)
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -o $@ $(OBJML) $(OBJNDML) $(LIBS)

# default
.c.o :
	$(ACC) -c $(cflags) $<


#Nucml.c : protml.c optimtpm.c abratio.c
#	$(CP) protml.c Nucml.c
Nucml.o : Nucml.c protml.h molphy.h optimtpm.c abratio.c
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nucml.c
#Ntranprb.c : tranprb.c
#	$(CP) tranprb.c Ntranprb.c
Ntranprb.o : Ntranprb.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Ntranprb.c
#Ndistan.c : distan.c
#	$(CP) distan.c Ndistan.c
Ndistan.o : Ndistan.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Ndistan.c
#Nmlklhd.c : mlklhd.c
#	$(CP) mlklhd.c Nmlklhd.c
Nmlklhd.o : Nmlklhd.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nmlklhd.c
nucproc.o : nucproc.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c nucproc.c
#Nucst.c : protst.c
#	$(CP) protst.c Nucst.c
Nucst.o : Nucst.c protst.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nucst.c
#Nseqstat.c : seqstat.c
#	$(CP) seqstat.c Nseqstat.c
Nseqstat.o : Nseqstat.c protst.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nseqstat.c


#Njdist.c : tridist.c
#	$(CP) tridist.c Njdist.c
Njdist.o : Njdist.c tridist.h molphy.h
	$(CC) $(CFLAGS) $(DEFINE) $(NJLE) -c Njdist.c
sddist.o : sddist.c sddist.h molphy.h
	$(CC) $(CFLAGS) $(DEFINE) $(SDLE) -c sddist.c
#Dpstree.c : pstree.c
#	$(CP) pstree.c Dpstree.c
Dpstree.o : Dpstree.c tridist.h molphy.h
	$(CC) $(CFLAGS) $(DEFINE) $(NJLE) -c Dpstree.c


#Prottpm.c : protml.c
#	$(CP) protml.c Prottpm.c
Prottpm.o : Prottpm.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(DEFINE) -c Prottpm.c
#Nuctpm.c : protml.c
#	$(CP) protml.c Nuctpm.c
Nuctpm.o : Nuctpm.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(NUCLE) $(DEFINE) -c Nuctpm.c

tpmproc.o : tpmproc.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(DEFINE) -c tpmproc.c
#Ntpmproc.c : tpmproc.c
#	$(CP) tpmproc.c Ntpmproc.c
Ntpmproc.o : Ntpmproc.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(NUCLE) $(DEFINE) -c Ntpmproc.c

#Trnprbt.c : tranprb.c
#	$(CP) tranprb.c Trnprbt.c
Trnprbt.o : Trnprbt.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(DEFINE) -c Trnprbt.c
#Ntrnprbt.c : tranprb.c
#	$(CP) tranprb.c Ntrnprbt.c
Ntrnprbt.o : Ntrnprbt.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(NUCLE) $(DEFINE) -c Ntrnprbt.c

#Nucdml.c : protdml.c
#	$(CP) protdml.c Nucdml.c
Nucdml.o : Nucdml.c protdml.h molphy.h optimtpm.c abratio.c
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nucdml.c
#Ndmlproc.c : dmlproc.c
#	$(CP) dmlproc.c Ndmlproc.c
Ndmlproc.o : Ndmlproc.c protdml.h molphy.h optimtpm.c abratio.c
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Ndmlproc.c

# others

clean :
	rm -f $(OBJECTS) $(PRGS)

DEPENDS = $(SOURCES_TML:.c=.depend)
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

altree.o: ../../INCLUDE/arb_assert.h
altree.o: matrixut.h
altree.o: molphy.h
altree.o: protml.h

distan.o: ../../INCLUDE/arb_assert.h
distan.o: matrixut.h
distan.o: molphy.h
distan.o: protml.h

dyhfjtt.o: ../../INCLUDE/arb_assert.h
dyhfjtt.o: matrixut.h
dyhfjtt.o: molphy.h
dyhfjtt.o: protml.h

getseq.o: ../../INCLUDE/arb_assert.h
getseq.o: matrixut.h
getseq.o: molphy.h
getseq.o: prot_tml.h
getseq.o: protml.h

matrixut.o: ../../INCLUDE/arb_assert.h
matrixut.o: matrixut.h
matrixut.o: mo_assert.h

mlklhd.o: ../../INCLUDE/arb_assert.h
mlklhd.o: matrixut.h
mlklhd.o: molphy.h
mlklhd.o: protml.h

mltree.o: ../../INCLUDE/arb_assert.h
mltree.o: matrixut.h
mltree.o: molphy.h
mltree.o: protml.h

mtrev24.o: ../../INCLUDE/arb_assert.h
mtrev24.o: matrixut.h
mtrev24.o: molphy.h
mtrev24.o: protml.h

mygetopt.o: molphy.h

njmtree.o: ../../INCLUDE/arb_assert.h
njmtree.o: matrixut.h
njmtree.o: molphy.h
njmtree.o: protml.h

njtree.o: ../../INCLUDE/arb_assert.h
njtree.o: matrixut.h
njtree.o: molphy.h
njtree.o: prot_tml.h
njtree.o: protml.h

protml.o: ../../INCLUDE/arb_assert.h
protml.o: matrixut.h
protml.o: molphy.h
protml.o: protml.h

protproc.o: ../../INCLUDE/arb_assert.h
protproc.o: matrixut.h
protproc.o: molphy.h
protproc.o: protml.h

prtree.o: ../../INCLUDE/arb_assert.h
prtree.o: matrixut.h
prtree.o: molphy.h
prtree.o: protml.h

pstree.o: ../../INCLUDE/arb_assert.h
pstree.o: matrixut.h
pstree.o: molphy.h
pstree.o: protml.h

qltree.o: ../../INCLUDE/arb_assert.h
qltree.o: matrixut.h
qltree.o: molphy.h
qltree.o: protml.h

seqproc.o: ../../INCLUDE/arb_assert.h
seqproc.o: matrixut.h
seqproc.o: molphy.h
seqproc.o: protml.h

sltree.o: ../../INCLUDE/arb_assert.h
sltree.o: matrixut.h
sltree.o: molphy.h
sltree.o: protml.h

tranprb.o: ../../INCLUDE/arb_assert.h
tranprb.o: matrixut.h
tranprb.o: molphy.h
tranprb.o: protml.h
