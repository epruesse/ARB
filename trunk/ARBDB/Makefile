# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend

include AD_MOBJECTS.h
OBJECTS = $(GB_O) $(GB_T) $(GB_PP)

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.c.o:
	$(CCLIB) $(cflags) -c $< $(AINCLUDES)
.cxx.o:
	$(CCPLIB) $(cflags) -c $< $(AINCLUDES)

proto:	pro tpro lproto

lproto: lpro tlpro

lpro: $(GB_O:.o=.c)
	rm -f ad_lpro.h
	../MAKEBIN/aisc_mkpt -g -z -C -F gb $(GB_O:.o=.c) >ad_lpro.h

tlpro: $(GB_T:.o=.c)
	rm -f ad_t_lpro.h
	../MAKEBIN/aisc_mkpt -g -z -C -F gb $(GB_T:.o=.c) >ad_t_lpro.h


pro: $(GB_O:.o=.c)
	rm -f ad_prot.h
	../MAKEBIN/aisc_mkpt -g -z -C -F GB $(GB_O:.o=.c) >ad_prot.h

tpro: $(GB_T:.o=.c)
	rm -f ad_t_prot.h
	../MAKEBIN/aisc_mkpt -g -z -C -F GB $(GB_T:.o=.c) >ad_t_prot.h

wc:
	wc *.[ch]
untouch:
	/usr/5bin/touch 0101010192 ad_lpro.h ad_t_lpro.h ad_prot.h ad_t_prot.h

luntouch:
	/usr/5bin/touch 0101010192 ad_lpro.h ad_t_lpro.h

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

adsort.o: arbdb.h arb_assert.h ad_prot.h arbdbt.h ad_t_prot.h

adlang1.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adlang1.o: adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h adGene.h

adstring.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adstring.o: adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h

arbdb.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
arbdb.o: adlmacros.h

ad_core.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
ad_core.o: adlmacros.h

admath.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
admath.o: adlmacros.h

adoptimize.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h
adoptimize.o: adtune.h adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h

adsystem.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adsystem.o: adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h

adindex.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adindex.o: adlmacros.h adlundo.h

adperl.o: arbdb.h arb_assert.h ad_prot.h

adlink.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adlink.o: adlmacros.h

adsocket.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adsocket.o: adlmacros.h

adcomm.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adcomm.o: adlmacros.h

adhash.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adhash.o: adlmacros.h

adquery.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adquery.o: adlmacros.h

ad_save_load.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h
ad_save_load.o: adtune.h adlmacros.h admap.h

adcompr.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adcompr.o: adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h

admalloc.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
admalloc.o: adlmacros.h

ad_load.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
ad_load.o: adlmacros.h admap.h

admap.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
admap.o: adlmacros.h admap.h

adTest.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adTest.o: adlmacros.h admap.h

adtune.o: adtune.h

adGene.o: adGene.h arbdb.h arb_assert.h ad_prot.h arbdbt.h ad_t_prot.h

adtools.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adtools.o: adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h

adseqcompr.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h
adseqcompr.o: adtune.h adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h
adseqcompr.o: adseqcompr.h

adtables.o: adlocal.h arb_assert.h arbdb.h ad_prot.h ad_lpro.h adtune.h
adtables.o: adlmacros.h arbdbt.h ad_t_prot.h ad_t_lpro.h

adRevCompl.o: arbdb.h arb_assert.h ad_prot.h arbdbt.h ad_t_prot.h

arbdbpp.o: arbdb.h arb_assert.h ad_prot.h arbdbt.h ad_t_prot.h
