# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx

include AD_MOBJECTS.h
OBJECTS = $(GB_O) $(GB_T) $(GB_PP)

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.c.o:
	$(CCPLIB) $(cflags) -c $< $(AINCLUDES)
.cxx.o:
	$(CCPLIB) $(cflags) -c $< $(AINCLUDES)

proto:	pro tpro lproto

lproto: lpro tlpro

lpro: $(GB_O:.o=.c)
	rm -f ad_lpro.h
	../MAKEBIN/aisc_mkpt -z -C -F gb $(GB_O:.o=.c) >ad_lpro.h
#	../MAKEBIN/aisc_mkpt $(GB_O:.o=.c) |awk '$$2 ~ /gb/ {print;}' >ad_lpro.h

tlpro: $(GB_T:.o=.c)
	rm -f ad_t_lpro.h
	../MAKEBIN/aisc_mkpt -z -C -F gb $(GB_T:.o=.c) >ad_t_lpro.h
#	../MAKEBIN/aisc_mkpt $(GB_T:.o=.c) |awk '$$2 ~ /gb/ {print;}' >ad_t_lpro.h


pro: $(GB_O:.o=.c)
	rm -f ad_prot.h
	../MAKEBIN/aisc_mkpt -z -C -F GB $(GB_O:.o=.c) >ad_prot.h
#	../MAKEBIN/aisc_mkpt $(GB_O:.o=.c) |awk '$$2 ~ /GB/ {print;}' >ad_prot.h

tpro: $(GB_T:.o=.c)
	rm -f ad_t_prot.h
	../MAKEBIN/aisc_mkpt -z -C -F GB $(GB_T:.o=.c) >ad_t_prot.h
#	../MAKEBIN/aisc_mkpt $(GB_T:.o=.c) |awk '$$2 ~ /GB/ {print;}' >ad_t_prot.h

wc:
	wc *.[ch]
untouch:
	/usr/5bin/touch 0101010192 ad_lpro.h ad_t_lpro.h ad_prot.h ad_t_prot.h

luntouch:
	/usr/5bin/touch 0101010192 ad_lpro.h ad_t_lpro.h

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) *.c *.cxx
# DO NOT DELETE THIS LINE -- make depend depends on it.

adReference.o:  
adReference.o:  
adReference.o:  
adReference.o:  
adReference.o:  
adReference.o:  
adReference.o:  
adReference.o:  
adReference.o:   arbdb.h
adReference.o: ./ad_prot.h adlocal.h adtune.h adlmacros.h
adReference.o: 
adTest.o:  
adTest.o:  
adTest.o:  
adTest.o:  
adTest.o:   
adTest.o:  
adTest.o:  
adTest.o:  
adTest.o:   
adTest.o:  adlocal.h adtune.h adlmacros.h admap.h
ad_core.o:  
ad_core.o:  
ad_core.o:  
ad_core.o:  
ad_core.o:   
ad_core.o:  
ad_core.o:  
ad_core.o:  
ad_core.o:   
ad_core.o: adlocal.h adtune.h adlmacros.h
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:   
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:   
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  
ad_load.o:  adlocal.h adtune.h adlmacros.h
ad_load.o: admap.h
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:  
ad_save_load.o:   adlocal.h
ad_save_load.o: adtune.h adlmacros.h admap.h
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:   
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:   
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  adlocal.h adtune.h adlmacros.h
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:   
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcomm.o:  
adcompr.o:  
adcompr.o:  
adcompr.o:  
adcompr.o:  
adcompr.o:   
adcompr.o:  
adcompr.o:  
adcompr.o:  
adcompr.o:   
adcompr.o:  adlocal.h adtune.h adlmacros.h arbdbt.h
adcompr.o: ./ad_t_prot.h ./ad_t_lpro.h
adhash.o:  
adhash.o:  
adhash.o:  
adhash.o:  
adhash.o:   
adhash.o:  
adhash.o:  
adhash.o:  
adhash.o:   
adhash.o:  adlocal.h adtune.h adlmacros.h
adindex.o:  
adindex.o:  
adindex.o:  
adindex.o:  
adindex.o:   
adindex.o:  
adindex.o:  
adindex.o:  
adindex.o:   
adindex.o: adlocal.h adtune.h adlmacros.h adlundo.h
adlang1.o:  
adlang1.o:  
adlang1.o:  
adlang1.o:  
adlang1.o:   
adlang1.o:  
adlang1.o:  
adlang1.o:  
adlang1.o:  
adlang1.o:  
adlang1.o:   
adlang1.o:  adlocal.h adtune.h adlmacros.h arbdbt.h
adlang1.o: ./ad_t_prot.h ./ad_t_lpro.h
adlink.o:  
adlink.o:  
adlink.o:  
adlink.o:  
adlink.o:   
adlink.o:  
adlink.o:  
adlink.o:  
adlink.o:   adlocal.h adtune.h
adlink.o: adlmacros.h arbdb.h ./ad_prot.h
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:  
admalloc.o:   
admalloc.o:  
admalloc.o:  
admalloc.o: adlocal.h adtune.h adlmacros.h
admap.o:  
admap.o:  
admap.o:  
admap.o:  
admap.o:   
admap.o:  
admap.o:  
admap.o:  
admap.o:   adlocal.h adtune.h
admap.o: adlmacros.h admap.h
admath.o:  
admath.o:  
admath.o:  
admath.o:  
admath.o:  
admath.o:  
admath.o:  
admath.o:   
admath.o:  
admath.o:  
admath.o:  
admath.o:  adlocal.h adtune.h adlmacros.h
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  
adoptimize.o:  adlocal.h adtune.h adlmacros.h
adperl.o:  
adperl.o:  
adperl.o:  
adperl.o:  
adperl.o:  
adperl.o:   
adperl.o:  
adperl.o:  
adperl.o:   arbdb.h
adperl.o: ./ad_prot.h
adquery.o:  
adquery.o:  
adquery.o:  
adquery.o:  
adquery.o:   
adquery.o:  
adquery.o:  
adquery.o:  
adquery.o:   
adquery.o:  adlocal.h adtune.h adlmacros.h
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:  
adseqcompr.o:   adlocal.h adtune.h
adseqcompr.o: adlmacros.h arbdb.h ./ad_prot.h arbdbt.h ./ad_t_prot.h
adseqcompr.o: ./ad_t_lpro.h adseqcompr.h
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:   
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o:  
adsocket.o: adlocal.h adtune.h adlmacros.h 
adsocket.o: 
adsort.o:  
adsort.o:  
adsort.o:  
adsort.o:  
adsort.o:   
adsort.o:  
adsort.o:  
adsort.o:  
adsort.o:   
adsort.o:  arbdb.h ./ad_prot.h arbdbt.h ./ad_t_prot.h
adsort.o: ./ad_t_lpro.h
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:   
adstring.o: adlocal.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
adstring.o: ./ad_t_lpro.h  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o:  
adstring.o: 
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:  
adsystem.o:   adlocal.h
adsystem.o: adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h ./ad_t_lpro.h
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  
adtables.o:  adlocal.h adtune.h adlmacros.h arbdb.h
adtables.o: ./ad_prot.h arbdbt.h ./ad_t_prot.h ./ad_t_lpro.h
adtools.o:  
adtools.o:  
adtools.o:  
adtools.o:  
adtools.o:   
adtools.o:  
adtools.o:  
adtools.o:  
adtools.o:   
adtools.o:  
adtools.o:  
adtools.o:  
adtools.o:  adlocal.h adtune.h adlmacros.h arbdb.h
adtools.o: ./ad_prot.h arbdbt.h ./ad_t_prot.h ./ad_t_lpro.h
adtune.o: adtune.h
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:   
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:   
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  
arbdb.o:  adlocal.h adtune.h adlmacros.h arbdb.h
arbdb.o: ./ad_prot.h
arbdbpp.o:  
arbdbpp.o:  
arbdbpp.o:  
arbdbpp.o:   arbdb.h
arbdbpp.o: ./ad_prot.h arbdbt.h ./ad_t_prot.h ./ad_t_lpro.h
