# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c

include AD_MOBJECTS.h

ACC_OBJECTS = $(GB_O) $(GB_T)

$(MAIN): $(ACC_OBJECTS)
	$(ARLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(ACC_OBJECTS)
	touch $@
.c.o:
	$(CCLIB) $(cflags) -c $< $(AINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(ACC_OBJECTS:.o=.c)
links:
	ln -s ../ARBDB/*.[ch] .

# DO NOT DELETE THIS LINE -- make depend depends on it.

adsort.o:   arbdb.h
adsort.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adsort.o: arbdbt.h ./ad_t_prot.h
adlang1.o:  adlocal.h
adlang1.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adlang1.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h adGene.h
adstring.o:  adlocal.h
adstring.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adstring.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
arbdb.o:   adlocal.h
arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
arbdb.o: ./ad_prot.h adtune.h adlmacros.h
ad_core.o:  adlocal.h
ad_core.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
ad_core.o: ./ad_prot.h adtune.h adlmacros.h
admath.o:   adlocal.h
admath.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
admath.o: ./ad_prot.h adtune.h adlmacros.h
adoptimize.o:  adlocal.h
adoptimize.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adoptimize.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
adsystem.o:  adlocal.h
adsystem.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adsystem.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
adindex.o:   adlocal.h
adindex.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adindex.o: ./ad_prot.h adtune.h adlmacros.h adlundo.h
adperl.o:  arbdb.h
adperl.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adlink.o:  adlocal.h
adlink.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adlink.o: ./ad_prot.h adtune.h adlmacros.h
adsocket.o:   adlocal.h
adsocket.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adsocket.o: ./ad_prot.h adtune.h adlmacros.h 
adcomm.o:  adlocal.h
adcomm.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adcomm.o: ./ad_prot.h adtune.h adlmacros.h 
adhash.o:  adlocal.h
adhash.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adhash.o: ./ad_prot.h adtune.h adlmacros.h
adquery.o:  adlocal.h
adquery.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adquery.o: ./ad_prot.h adtune.h adlmacros.h
ad_save_load.o:  adlocal.h
ad_save_load.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
ad_save_load.o: ./ad_prot.h adtune.h adlmacros.h admap.h
adcompr.o:   adlocal.h
adcompr.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adcompr.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
admalloc.o: adlocal.h $(ARBHOME)/INCLUDE/arb_assert.h
admalloc.o: arbdb.h ./ad_prot.h adtune.h adlmacros.h
ad_load.o:   adlocal.h
ad_load.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
ad_load.o: ./ad_prot.h adtune.h adlmacros.h admap.h
admap.o:  adlocal.h
admap.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
admap.o: ./ad_prot.h adtune.h adlmacros.h admap.h
adTest.o:   adlocal.h
adTest.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adTest.o: ./ad_prot.h adtune.h adlmacros.h admap.h
adtune.o: adtune.h
adGene.o: adGene.h arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h
adGene.o:  ./ad_prot.h arbdbt.h ./ad_t_prot.h
adtools.o:  adlocal.h
adtools.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adtools.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
adseqcompr.o:  adlocal.h
adseqcompr.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adseqcompr.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
adseqcompr.o: adseqcompr.h
adtables.o:  adlocal.h
adtables.o: $(ARBHOME)/INCLUDE/arb_assert.h arbdb.h
adtables.o: ./ad_prot.h adtune.h adlmacros.h arbdbt.h ./ad_t_prot.h
adRevCompl.o:   arbdb.h
adRevCompl.o: $(ARBHOME)/INCLUDE/arb_assert.h
adRevCompl.o:  ./ad_prot.h arbdbt.h
adRevCompl.o: ./ad_t_prot.h
