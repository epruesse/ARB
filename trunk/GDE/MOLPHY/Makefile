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


Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o:   
Dpstree.o:   
Dpstree.o:  
Dpstree.o:   
Dpstree.o:  
Dpstree.o:  
Dpstree.o:  
Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Dpstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Dpstree.o: matrixut.h molphy.h protml.h
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o:   
Ndistan.o:   
Ndistan.o:  
Ndistan.o:   
Ndistan.o:  
Ndistan.o:  
Ndistan.o:  
Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Ndistan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Ndistan.o: matrixut.h molphy.h protml.h
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:  
Njdist.o:   
Njdist.o:   
Njdist.o:   
Njdist.o:   
Njdist.o:  
Njdist.o:   
Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Njdist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Njdist.o: matrixut.h molphy.h tridist.h
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:   
Nmlklhd.o:   
Nmlklhd.o:  
Nmlklhd.o:   
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o:  
Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Nmlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Nmlklhd.o: matrixut.h molphy.h protml.h
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:   
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:   
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o:  
Nseqstat.o: 
Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Nseqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Nseqstat.o: matrixut.h molphy.h protst.h
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:   
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:   
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o:  
Ntranprb.o: 
Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Ntranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Ntranprb.o: matrixut.h molphy.h protml.h
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:  
Nucml.o:   
Nucml.o:   
Nucml.o:   
Nucml.o:   
Nucml.o:  
Nucml.o:   
Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Nucml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Nucml.o: matrixut.h molphy.h protml.h
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:  
Nucst.o:   
Nucst.o:   
Nucst.o:   
Nucst.o:   
Nucst.o:  
Nucst.o:   
Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Nucst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Nucst.o: matrixut.h molphy.h protst.h
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:  
Nuctpm.o:   
Nuctpm.o:   
Nuctpm.o:   
Nuctpm.o:   
Nuctpm.o:  
Nuctpm.o:   
Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Nuctpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Nuctpm.o: matrixut.h molphy.h protml.h
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o:   
Prottpm.o:   
Prottpm.o:  
Prottpm.o:   
Prottpm.o:  
Prottpm.o:  
Prottpm.o:  
Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
Prottpm.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
Prottpm.o: matrixut.h molphy.h protml.h
altree.o:  
altree.o:  
altree.o:  
altree.o:  
altree.o:  
altree.o:  
altree.o:  
altree.o:  
altree.o:  
altree.o:   
altree.o:   
altree.o:   
altree.o:   
altree.o:  
altree.o:   
altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
altree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
altree.o: matrixut.h molphy.h protml.h
distan.o:  
distan.o:  
distan.o:  
distan.o:  
distan.o:  
distan.o:  
distan.o:  
distan.o:  
distan.o:  
distan.o:   
distan.o:   
distan.o:   
distan.o:   
distan.o:  
distan.o:   
distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
distan.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
distan.o: matrixut.h molphy.h protml.h
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o:   
distproc.o:  
distproc.o:  
distproc.o:   
distproc.o:  
distproc.o:  
distproc.o:  
distproc.o: 
distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
distproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
distproc.o: matrixut.h molphy.h tridist.h
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:   
dyhfjtt.o:   
dyhfjtt.o:  
dyhfjtt.o:   
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o:  
dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
dyhfjtt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
dyhfjtt.o: matrixut.h molphy.h protml.h
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:  
getseq.o:   
getseq.o:   
getseq.o:   
getseq.o:   
getseq.o:  
getseq.o:   
getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
getseq.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
getseq.o: matrixut.h molphy.h prot_tml.h protml.h
matrixut.o:  
matrixut.o:  
matrixut.o:  
matrixut.o:   
matrixut.o: 
matrixut.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
matrixut.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
matrixut.o: matrixut.h
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:  
mlklhd.o:   
mlklhd.o:   
mlklhd.o:   
mlklhd.o:   
mlklhd.o:  
mlklhd.o:   
mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
mlklhd.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
mlklhd.o: matrixut.h molphy.h protml.h
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:  
mltree.o:   
mltree.o:   
mltree.o:   
mltree.o:   
mltree.o:  
mltree.o:   
mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
mltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
mltree.o: matrixut.h molphy.h protml.h
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o:   
mtrev22.o:   
mtrev22.o:  
mtrev22.o:   
mtrev22.o:  
mtrev22.o:  
mtrev22.o:  
mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
mtrev22.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
mtrev22.o: matrixut.h molphy.h protml.h
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o:   
mtrev24.o:   
mtrev24.o:  
mtrev24.o:   
mtrev24.o:  
mtrev24.o:  
mtrev24.o:  
mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
mtrev24.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
mtrev24.o: matrixut.h molphy.h protml.h
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o:   
mygetopt.o:  
mygetopt.o:  
mygetopt.o:   
mygetopt.o:  
mygetopt.o:  
mygetopt.o:  
mygetopt.o: 
mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
mygetopt.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
mygetopt.o: molphy.h
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o:   
njmtree.o:   
njmtree.o:  
njmtree.o:   
njmtree.o:  
njmtree.o:  
njmtree.o:  
njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
njmtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
njmtree.o: matrixut.h molphy.h protml.h
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:  
njproc.o:   
njproc.o:   
njproc.o:   
njproc.o:   
njproc.o:  
njproc.o:   
njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
njproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
njproc.o: matrixut.h molphy.h tridist.h
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:  
njtree.o:   
njtree.o:   
njtree.o:   
njtree.o:   
njtree.o:  
njtree.o:   
njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
njtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
njtree.o: matrixut.h molphy.h protml.h
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o:   
nucproc.o:   
nucproc.o:  
nucproc.o:   
nucproc.o:  
nucproc.o:  
nucproc.o:  
nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
nucproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
nucproc.o: matrixut.h molphy.h protml.h
protml.o:  
protml.o:  
protml.o:  
protml.o:  
protml.o:  
protml.o:  
protml.o:  
protml.o:  
protml.o:  
protml.o:   
protml.o:   
protml.o:   
protml.o:   
protml.o:  
protml.o:   
protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
protml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
protml.o: matrixut.h molphy.h protml.h
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o:   
protproc.o:  
protproc.o:  
protproc.o:   
protproc.o:  
protproc.o:  
protproc.o:  
protproc.o: 
protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
protproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
protproc.o: matrixut.h molphy.h protml.h
protst.o:  
protst.o:  
protst.o:  
protst.o:  
protst.o:  
protst.o:  
protst.o:  
protst.o:  
protst.o:  
protst.o:   
protst.o:   
protst.o:   
protst.o:   
protst.o:  
protst.o:   
protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
protst.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
protst.o: matrixut.h molphy.h protst.h
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:  
prtree.o:   
prtree.o:   
prtree.o:   
prtree.o:   
prtree.o:  
prtree.o:   
prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
prtree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
prtree.o: matrixut.h molphy.h protml.h
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:  
pstree.o:   
pstree.o:   
pstree.o:   
pstree.o:   
pstree.o:  
pstree.o:   
pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
pstree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
pstree.o: matrixut.h molphy.h protml.h
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:  
qltree.o:   
qltree.o:   
qltree.o:   
qltree.o:   
qltree.o:  
qltree.o:   
qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
qltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
qltree.o: matrixut.h molphy.h protml.h
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o:   
seqproc.o:   
seqproc.o:  
seqproc.o:   
seqproc.o:  
seqproc.o:  
seqproc.o:  
seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
seqproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
seqproc.o: matrixut.h molphy.h protml.h
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o:   
seqstat.o:   
seqstat.o:  
seqstat.o:   
seqstat.o:  
seqstat.o:  
seqstat.o:  
seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
seqstat.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
seqstat.o: matrixut.h molphy.h protst.h
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:  
sltree.o:   
sltree.o:   
sltree.o:   
sltree.o:   
sltree.o:  
sltree.o:   
sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
sltree.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
sltree.o: matrixut.h molphy.h protml.h
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o:   
totalml.o:   
totalml.o:  
totalml.o:   
totalml.o:  
totalml.o:  
totalml.o:  
totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
totalml.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
totalml.o: matrixut.h molphy.h totalml.h
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o:   
tranprb.o:   
tranprb.o:  
tranprb.o:   
tranprb.o:  
tranprb.o:  
tranprb.o:  
tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
tranprb.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
tranprb.o: matrixut.h molphy.h protml.h
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o:   
tridist.o:   
tridist.o:  
tridist.o:   
tridist.o:  
tridist.o:  
tridist.o:  
tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
tridist.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
tridist.o: matrixut.h molphy.h tridist.h
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o:   
triproc.o:   
triproc.o:  
triproc.o:   
triproc.o:  
triproc.o:  
triproc.o:  
triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
triproc.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
triproc.o: matrixut.h molphy.h tridist.h
