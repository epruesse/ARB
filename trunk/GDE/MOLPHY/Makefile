.SUFFIXES: .o .c

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


Nucml.c : protml.c optimtpm.c abratio.c
	$(CP) protml.c Nucml.c
Nucml.o : Nucml.c protml.h molphy.h optimtpm.c abratio.c
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nucml.c
Ntranprb.c : tranprb.c
	$(CP) tranprb.c Ntranprb.c
Ntranprb.o : Ntranprb.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Ntranprb.c
Ndistan.c : distan.c
	$(CP) distan.c Ndistan.c
Ndistan.o : Ndistan.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Ndistan.c
Nmlklhd.c : mlklhd.c
	$(CP) mlklhd.c Nmlklhd.c
Nmlklhd.o : Nmlklhd.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nmlklhd.c
nucproc.o : nucproc.c protml.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c nucproc.c
Nucst.c : protst.c
	$(CP) protst.c Nucst.c
Nucst.o : Nucst.c protst.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nucst.c
Nseqstat.c : seqstat.c
	$(CP) seqstat.c Nseqstat.c
Nseqstat.o : Nseqstat.c protst.h molphy.h
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nseqstat.c


Njdist.c : tridist.c
	$(CP) tridist.c Njdist.c
Njdist.o : Njdist.c tridist.h molphy.h
	$(CC) $(CFLAGS) $(DEFINE) $(NJLE) -c Njdist.c
sddist.o : sddist.c sddist.h molphy.h
	$(CC) $(CFLAGS) $(DEFINE) $(SDLE) -c sddist.c
Dpstree.c : pstree.c
	$(CP) pstree.c Dpstree.c
Dpstree.o : Dpstree.c tridist.h molphy.h
	$(CC) $(CFLAGS) $(DEFINE) $(NJLE) -c Dpstree.c


Prottpm.c : protml.c
	$(CP) protml.c Prottpm.c
Prottpm.o : Prottpm.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(DEFINE) -c Prottpm.c
Nuctpm.c : protml.c
	$(CP) protml.c Nuctpm.c
Nuctpm.o : Nuctpm.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(NUCLE) $(DEFINE) -c Nuctpm.c

tpmproc.o : tpmproc.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(DEFINE) -c tpmproc.c
Ntpmproc.c : tpmproc.c
	$(CP) tpmproc.c Ntpmproc.c
Ntpmproc.o : Ntpmproc.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(NUCLE) $(DEFINE) -c Ntpmproc.c

Trnprbt.c : tranprb.c
	$(CP) tranprb.c Trnprbt.c
Trnprbt.o : Trnprbt.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(DEFINE) -c Trnprbt.c
Ntrnprbt.c : tranprb.c
	$(CP) tranprb.c Ntrnprbt.c
Ntrnprbt.o : Ntrnprbt.c protml.h molphy.h
	$(CC) $(CFLAGS) -DTPM $(NUCLE) $(DEFINE) -c Ntrnprbt.c

Nucdml.c : protdml.c
	$(CP) protdml.c Nucdml.c
Nucdml.o : Nucdml.c protdml.h molphy.h optimtpm.c abratio.c
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Nucdml.c
Ndmlproc.c : dmlproc.c
	$(CP) dmlproc.c Ndmlproc.c
Ndmlproc.o : Ndmlproc.c protdml.h molphy.h optimtpm.c abratio.c
	$(CC) $(CFLAGS) $(NUCLE) $(DEFINE) -c Ndmlproc.c

# others

clean :
	rm -f core *.o $(PRGS)

depend:
	makedepend $(MAKEDEPENDINC) -p ../../bin/ *.c

# DO NOT DELETE THIS LINE -- make depend depends on it.

../../bin/Dpstree.o: protml.h molphy.h 
../../bin/Dpstree.o:  
../../bin/Dpstree.o: 
../../bin/Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Dpstree.o:  
../../bin/Dpstree.o:  
../../bin/Dpstree.o:  
../../bin/Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Dpstree.o:  
../../bin/Dpstree.o:  
../../bin/Dpstree.o:  
../../bin/Dpstree.o:  
../../bin/Dpstree.o: 
../../bin/Dpstree.o:  
../../bin/Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Dpstree.o:   matrixut.h
../../bin/Ndistan.o: protml.h molphy.h 
../../bin/Ndistan.o:  
../../bin/Ndistan.o: 
../../bin/Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Ndistan.o:  
../../bin/Ndistan.o:  
../../bin/Ndistan.o:  
../../bin/Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Ndistan.o:  
../../bin/Ndistan.o:  
../../bin/Ndistan.o:  
../../bin/Ndistan.o:  
../../bin/Ndistan.o: 
../../bin/Ndistan.o:  
../../bin/Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Ndistan.o:   matrixut.h
../../bin/Njdist.o: tridist.h molphy.h 
../../bin/Njdist.o:  
../../bin/Njdist.o: 
../../bin/Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Njdist.o:  
../../bin/Njdist.o:  
../../bin/Njdist.o:  
../../bin/Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Njdist.o:  
../../bin/Njdist.o:  
../../bin/Njdist.o:  
../../bin/Njdist.o:  
../../bin/Njdist.o:  
../../bin/Njdist.o: 
../../bin/Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Njdist.o:   matrixut.h
../../bin/Nmlklhd.o: protml.h molphy.h 
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o: 
../../bin/Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o: 
../../bin/Nmlklhd.o:  
../../bin/Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Nmlklhd.o:   matrixut.h
../../bin/Nseqstat.o: protst.h molphy.h 
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o: 
../../bin/Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o: 
../../bin/Nseqstat.o:  
../../bin/Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Nseqstat.o:   matrixut.h
../../bin/Ntranprb.o: protml.h molphy.h 
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o: 
../../bin/Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o: 
../../bin/Ntranprb.o:  
../../bin/Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Ntranprb.o:   matrixut.h
../../bin/Nucml.o: protml.h molphy.h 
../../bin/Nucml.o:  
../../bin/Nucml.o: 
../../bin/Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Nucml.o:  
../../bin/Nucml.o:  
../../bin/Nucml.o:  
../../bin/Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Nucml.o:  
../../bin/Nucml.o:  
../../bin/Nucml.o:  
../../bin/Nucml.o:  
../../bin/Nucml.o:  
../../bin/Nucml.o: 
../../bin/Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Nucml.o:   matrixut.h
../../bin/Nucst.o: protst.h molphy.h 
../../bin/Nucst.o:  
../../bin/Nucst.o: 
../../bin/Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Nucst.o:  
../../bin/Nucst.o:  
../../bin/Nucst.o:  
../../bin/Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Nucst.o:  
../../bin/Nucst.o:  
../../bin/Nucst.o:  
../../bin/Nucst.o:  
../../bin/Nucst.o:  
../../bin/Nucst.o: 
../../bin/Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Nucst.o:   matrixut.h
../../bin/Nuctpm.o: protml.h molphy.h 
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o: 
../../bin/Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o:  
../../bin/Nuctpm.o: 
../../bin/Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Nuctpm.o:   matrixut.h
../../bin/Prottpm.o: protml.h molphy.h 
../../bin/Prottpm.o:  
../../bin/Prottpm.o: 
../../bin/Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/Prottpm.o:  
../../bin/Prottpm.o:  
../../bin/Prottpm.o:  
../../bin/Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/Prottpm.o:  
../../bin/Prottpm.o:  
../../bin/Prottpm.o:  
../../bin/Prottpm.o:  
../../bin/Prottpm.o: 
../../bin/Prottpm.o:  
../../bin/Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/Prottpm.o:   matrixut.h
../../bin/altree.o: protml.h molphy.h 
../../bin/altree.o:  
../../bin/altree.o: 
../../bin/altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/altree.o:  
../../bin/altree.o:  
../../bin/altree.o:  
../../bin/altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/altree.o:  
../../bin/altree.o:  
../../bin/altree.o:  
../../bin/altree.o:  
../../bin/altree.o:  
../../bin/altree.o: 
../../bin/altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/altree.o:   matrixut.h
../../bin/distan.o: protml.h molphy.h 
../../bin/distan.o:  
../../bin/distan.o: 
../../bin/distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/distan.o:  
../../bin/distan.o:  
../../bin/distan.o:  
../../bin/distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/distan.o:  
../../bin/distan.o:  
../../bin/distan.o:  
../../bin/distan.o:  
../../bin/distan.o:  
../../bin/distan.o: 
../../bin/distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/distan.o:   matrixut.h
../../bin/distproc.o: tridist.h molphy.h 
../../bin/distproc.o:  
../../bin/distproc.o: 
../../bin/distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/distproc.o:  
../../bin/distproc.o:  
../../bin/distproc.o:  
../../bin/distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/distproc.o:  
../../bin/distproc.o:  
../../bin/distproc.o:  
../../bin/distproc.o:  
../../bin/distproc.o: 
../../bin/distproc.o:  
../../bin/distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/distproc.o:   matrixut.h
../../bin/dyhfjtt.o: protml.h molphy.h 
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o: 
../../bin/dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o: 
../../bin/dyhfjtt.o:  
../../bin/dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/dyhfjtt.o:   matrixut.h
../../bin/getseq.o: protml.h molphy.h 
../../bin/getseq.o:  
../../bin/getseq.o: 
../../bin/getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/getseq.o:  
../../bin/getseq.o:  
../../bin/getseq.o:  
../../bin/getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/getseq.o:  
../../bin/getseq.o:  
../../bin/getseq.o:  
../../bin/getseq.o:  
../../bin/getseq.o:  
../../bin/getseq.o: 
../../bin/getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/getseq.o:   matrixut.h
../../bin/getseq.o: prot_tml.h
../../bin/matrixut.o:  
../../bin/matrixut.o:  
../../bin/matrixut.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/matrixut.o:  
../../bin/matrixut.o:  
../../bin/matrixut.o:  
../../bin/matrixut.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/matrixut.o:  matrixut.h
../../bin/mlklhd.o: protml.h molphy.h 
../../bin/mlklhd.o:  
../../bin/mlklhd.o: 
../../bin/mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/mlklhd.o:  
../../bin/mlklhd.o:  
../../bin/mlklhd.o:  
../../bin/mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/mlklhd.o:  
../../bin/mlklhd.o:  
../../bin/mlklhd.o:  
../../bin/mlklhd.o:  
../../bin/mlklhd.o:  
../../bin/mlklhd.o: 
../../bin/mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/mlklhd.o:   matrixut.h
../../bin/mltree.o: protml.h molphy.h 
../../bin/mltree.o:  
../../bin/mltree.o: 
../../bin/mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/mltree.o:  
../../bin/mltree.o:  
../../bin/mltree.o:  
../../bin/mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/mltree.o:  
../../bin/mltree.o:  
../../bin/mltree.o:  
../../bin/mltree.o:  
../../bin/mltree.o:  
../../bin/mltree.o: 
../../bin/mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/mltree.o:   matrixut.h
../../bin/mtrev22.o: protml.h molphy.h 
../../bin/mtrev22.o:  
../../bin/mtrev22.o: 
../../bin/mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/mtrev22.o:  
../../bin/mtrev22.o:  
../../bin/mtrev22.o:  
../../bin/mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/mtrev22.o:  
../../bin/mtrev22.o:  
../../bin/mtrev22.o:  
../../bin/mtrev22.o:  
../../bin/mtrev22.o: 
../../bin/mtrev22.o:  
../../bin/mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/mtrev22.o:   matrixut.h
../../bin/mtrev24.o: protml.h molphy.h 
../../bin/mtrev24.o:  
../../bin/mtrev24.o: 
../../bin/mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/mtrev24.o:  
../../bin/mtrev24.o:  
../../bin/mtrev24.o:  
../../bin/mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/mtrev24.o:  
../../bin/mtrev24.o:  
../../bin/mtrev24.o:  
../../bin/mtrev24.o:  
../../bin/mtrev24.o: 
../../bin/mtrev24.o:  
../../bin/mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/mtrev24.o:   matrixut.h
../../bin/mygetopt.o: molphy.h  
../../bin/mygetopt.o:  
../../bin/mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/mygetopt.o:  
../../bin/mygetopt.o:  
../../bin/mygetopt.o:  
../../bin/mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/mygetopt.o:  
../../bin/mygetopt.o:  
../../bin/mygetopt.o:  
../../bin/mygetopt.o:  
../../bin/mygetopt.o: 
../../bin/mygetopt.o:  
../../bin/mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/mygetopt.o:  
../../bin/njmtree.o: protml.h molphy.h 
../../bin/njmtree.o:  
../../bin/njmtree.o: 
../../bin/njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/njmtree.o:  
../../bin/njmtree.o:  
../../bin/njmtree.o:  
../../bin/njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/njmtree.o:  
../../bin/njmtree.o:  
../../bin/njmtree.o:  
../../bin/njmtree.o:  
../../bin/njmtree.o: 
../../bin/njmtree.o:  
../../bin/njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/njmtree.o:   matrixut.h
../../bin/njproc.o: tridist.h molphy.h 
../../bin/njproc.o:  
../../bin/njproc.o: 
../../bin/njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/njproc.o:  
../../bin/njproc.o:  
../../bin/njproc.o:  
../../bin/njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/njproc.o:  
../../bin/njproc.o:  
../../bin/njproc.o:  
../../bin/njproc.o:  
../../bin/njproc.o:  
../../bin/njproc.o: 
../../bin/njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/njproc.o:   matrixut.h
../../bin/njtree.o: protml.h molphy.h 
../../bin/njtree.o:  
../../bin/njtree.o: 
../../bin/njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/njtree.o:  
../../bin/njtree.o:  
../../bin/njtree.o:  
../../bin/njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/njtree.o:  
../../bin/njtree.o:  
../../bin/njtree.o:  
../../bin/njtree.o:  
../../bin/njtree.o:  
../../bin/njtree.o: 
../../bin/njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/njtree.o:   matrixut.h
../../bin/nucproc.o: protml.h molphy.h 
../../bin/nucproc.o:  
../../bin/nucproc.o: 
../../bin/nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/nucproc.o:  
../../bin/nucproc.o:  
../../bin/nucproc.o:  
../../bin/nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/nucproc.o:  
../../bin/nucproc.o:  
../../bin/nucproc.o:  
../../bin/nucproc.o:  
../../bin/nucproc.o: 
../../bin/nucproc.o:  
../../bin/nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/nucproc.o:   matrixut.h
../../bin/protml.o: protml.h molphy.h 
../../bin/protml.o:  
../../bin/protml.o: 
../../bin/protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/protml.o:  
../../bin/protml.o:  
../../bin/protml.o:  
../../bin/protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/protml.o:  
../../bin/protml.o:  
../../bin/protml.o:  
../../bin/protml.o:  
../../bin/protml.o:  
../../bin/protml.o: 
../../bin/protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/protml.o:   matrixut.h
../../bin/protproc.o: protml.h molphy.h 
../../bin/protproc.o:  
../../bin/protproc.o: 
../../bin/protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/protproc.o:  
../../bin/protproc.o:  
../../bin/protproc.o:  
../../bin/protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/protproc.o:  
../../bin/protproc.o:  
../../bin/protproc.o:  
../../bin/protproc.o:  
../../bin/protproc.o: 
../../bin/protproc.o:  
../../bin/protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/protproc.o:   matrixut.h
../../bin/protst.o: protst.h molphy.h 
../../bin/protst.o:  
../../bin/protst.o: 
../../bin/protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/protst.o:  
../../bin/protst.o:  
../../bin/protst.o:  
../../bin/protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/protst.o:  
../../bin/protst.o:  
../../bin/protst.o:  
../../bin/protst.o:  
../../bin/protst.o:  
../../bin/protst.o: 
../../bin/protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/protst.o:   matrixut.h
../../bin/prtree.o: protml.h molphy.h 
../../bin/prtree.o:  
../../bin/prtree.o: 
../../bin/prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/prtree.o:  
../../bin/prtree.o:  
../../bin/prtree.o:  
../../bin/prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/prtree.o:  
../../bin/prtree.o:  
../../bin/prtree.o:  
../../bin/prtree.o:  
../../bin/prtree.o:  
../../bin/prtree.o: 
../../bin/prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/prtree.o:   matrixut.h
../../bin/pstree.o: protml.h molphy.h 
../../bin/pstree.o:  
../../bin/pstree.o: 
../../bin/pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/pstree.o:  
../../bin/pstree.o:  
../../bin/pstree.o:  
../../bin/pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/pstree.o:  
../../bin/pstree.o:  
../../bin/pstree.o:  
../../bin/pstree.o:  
../../bin/pstree.o:  
../../bin/pstree.o: 
../../bin/pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/pstree.o:   matrixut.h
../../bin/qltree.o: protml.h molphy.h 
../../bin/qltree.o:  
../../bin/qltree.o: 
../../bin/qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/qltree.o:  
../../bin/qltree.o:  
../../bin/qltree.o:  
../../bin/qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/qltree.o:  
../../bin/qltree.o:  
../../bin/qltree.o:  
../../bin/qltree.o:  
../../bin/qltree.o:  
../../bin/qltree.o: 
../../bin/qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/qltree.o:   matrixut.h
../../bin/seqproc.o: protml.h molphy.h 
../../bin/seqproc.o:  
../../bin/seqproc.o: 
../../bin/seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/seqproc.o:  
../../bin/seqproc.o:  
../../bin/seqproc.o:  
../../bin/seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/seqproc.o:  
../../bin/seqproc.o:  
../../bin/seqproc.o:  
../../bin/seqproc.o:  
../../bin/seqproc.o: 
../../bin/seqproc.o:  
../../bin/seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/seqproc.o:   matrixut.h
../../bin/seqstat.o: protst.h molphy.h 
../../bin/seqstat.o:  
../../bin/seqstat.o: 
../../bin/seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/seqstat.o:  
../../bin/seqstat.o:  
../../bin/seqstat.o:  
../../bin/seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/seqstat.o:  
../../bin/seqstat.o:  
../../bin/seqstat.o:  
../../bin/seqstat.o:  
../../bin/seqstat.o: 
../../bin/seqstat.o:  
../../bin/seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/seqstat.o:   matrixut.h
../../bin/sltree.o: protml.h molphy.h 
../../bin/sltree.o:  
../../bin/sltree.o: 
../../bin/sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/sltree.o:  
../../bin/sltree.o:  
../../bin/sltree.o:  
../../bin/sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/sltree.o:  
../../bin/sltree.o:  
../../bin/sltree.o:  
../../bin/sltree.o:  
../../bin/sltree.o:  
../../bin/sltree.o: 
../../bin/sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/sltree.o:   matrixut.h
../../bin/totalml.o: totalml.h molphy.h 
../../bin/totalml.o:  
../../bin/totalml.o: 
../../bin/totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/totalml.o:  
../../bin/totalml.o:  
../../bin/totalml.o:  
../../bin/totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/totalml.o:  
../../bin/totalml.o:  
../../bin/totalml.o:  
../../bin/totalml.o:  
../../bin/totalml.o: 
../../bin/totalml.o:  
../../bin/totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/totalml.o:   matrixut.h
../../bin/tranprb.o: protml.h molphy.h 
../../bin/tranprb.o:  
../../bin/tranprb.o: 
../../bin/tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/tranprb.o:  
../../bin/tranprb.o:  
../../bin/tranprb.o:  
../../bin/tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/tranprb.o:  
../../bin/tranprb.o:  
../../bin/tranprb.o:  
../../bin/tranprb.o:  
../../bin/tranprb.o: 
../../bin/tranprb.o:  
../../bin/tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/tranprb.o:   matrixut.h
../../bin/tridist.o: tridist.h molphy.h 
../../bin/tridist.o:  
../../bin/tridist.o: 
../../bin/tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/tridist.o:  
../../bin/tridist.o:  
../../bin/tridist.o:  
../../bin/tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/tridist.o:  
../../bin/tridist.o:  
../../bin/tridist.o:  
../../bin/tridist.o:  
../../bin/tridist.o: 
../../bin/tridist.o:  
../../bin/tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/tridist.o:   matrixut.h
../../bin/triproc.o: tridist.h molphy.h 
../../bin/triproc.o:  
../../bin/triproc.o: 
../../bin/triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
../../bin/triproc.o:  
../../bin/triproc.o:  
../../bin/triproc.o:  
../../bin/triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
../../bin/triproc.o:  
../../bin/triproc.o:  
../../bin/triproc.o:  
../../bin/triproc.o:  
../../bin/triproc.o: 
../../bin/triproc.o:  
../../bin/triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
../../bin/triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
../../bin/triproc.o:   matrixut.h
