# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend

include AD_MOBJECTS.h


OBJECTS = $(GB_O) $(GB_T) $(GB_PP)

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)
.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)
.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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

links:
	ln -s ../ARBDB/*.[ch] .

# DO NOT DELETE THIS LINE -- make depend depends on it.

adsort.o: arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h ad_prot.h arbdbt.h
adsort.o: ad_t_prot.h

adlang1.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adlang1.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adlang1.o: ad_t_prot.h ad_t_lpro.h adGene.h

adstring.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adstring.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adstring.o: ad_t_prot.h ad_t_lpro.h

arbdb.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h ad_prot.h
arbdb.o: ad_lpro.h adtune.h adlmacros.h

ad_core.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
ad_core.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

admath.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
admath.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

adoptimize.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adoptimize.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adoptimize.o: ad_t_prot.h ad_t_lpro.h

adsystem.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adsystem.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adsystem.o: ad_t_prot.h ad_t_lpro.h

adindex.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adindex.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h adlundo.h

adperl.o: arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h ad_prot.h

adlink.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adlink.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

adsocket.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adsocket.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

adcomm.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adcomm.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

adhash.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adhash.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

adquery.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adquery.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

ad_save_load.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
ad_save_load.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h admap.h

adcompr.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adcompr.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adcompr.o: ad_t_prot.h ad_t_lpro.h

admalloc.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
admalloc.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h

ad_load.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
ad_load.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h admap.h

admap.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h ad_prot.h
admap.o: ad_lpro.h adtune.h adlmacros.h admap.h

adTest.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adTest.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h admap.h

adtune.o: adtune.h

adGene.o: adGene.h arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h ad_prot.h
adGene.o: arbdbt.h ad_t_prot.h

adtools.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adtools.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adtools.o: ad_t_prot.h ad_t_lpro.h

adseqcompr.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adseqcompr.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adseqcompr.o: ad_t_prot.h ad_t_lpro.h adseqcompr.h

adtables.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adtables.o: ad_prot.h ad_lpro.h adtune.h adlmacros.h arbdbt.h
adtables.o: ad_t_prot.h ad_t_lpro.h

adRevCompl.o: arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h ad_prot.h
adRevCompl.o: arbdbt.h ad_t_prot.h

arbdbpp.o: arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h ad_prot.h
arbdbpp.o: arbdbt.h ad_t_prot.h
