# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx

include AD_MOBJECTS.h


OBJECTS = $(GB_O) $(GB_T) $(GB_PP)

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)
.c.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)
.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) *.c *.cxx
links:
	ln -s ../ARBDB/*.[ch] .

# DO NOT DELETE THIS LINE -- make depend depends on it.

adGene.o: adGene.h arbdb.h $(ARBHOME)/INCLUDE/arb_assert.h
adGene.o:  ./ad_prot.h arbdbt.h ./ad_t_prot.h
adReference.o:   arbdb.h
adReference.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adReference.o: adlocal.h adtune.h adlmacros.h 
adRevCompl.o:   arbdb.h
adRevCompl.o: $(ARBHOME)/INCLUDE/arb_assert.h
adRevCompl.o:  ./ad_prot.h arbdbt.h
adRevCompl.o: ./ad_t_prot.h
adTest.o:   adlocal.h adtune.h
adTest.o: adlmacros.h admap.h
ad_core.o:  adlocal.h adtune.h adlmacros.h
ad_load.o:   adlocal.h
ad_load.o: adtune.h adlmacros.h admap.h
ad_save_load.o:  adlocal.h adtune.h adlmacros.h
ad_save_load.o: admap.h
adcomm.o:  adlocal.h adtune.h adlmacros.h
adcompr.o:   adlocal.h adtune.h
adcompr.o: adlmacros.h arbdbt.h ./ad_t_prot.h
adhash.o:  adlocal.h adtune.h adlmacros.h
adindex.o:   adlocal.h
adindex.o: adtune.h adlmacros.h adlundo.h
adlang1.o:  adlocal.h adtune.h adlmacros.h arbdbt.h
adlang1.o: ./ad_t_prot.h adGene.h arbdb.h
adlang1.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adlink.o:  adlocal.h adtune.h adlmacros.h arbdb.h
adlink.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
admalloc.o: adlocal.h adtune.h adlmacros.h
admap.o:  adlocal.h adtune.h adlmacros.h admap.h
admath.o:   adlocal.h
admath.o: adtune.h adlmacros.h
adoptimize.o:  adlocal.h adtune.h adlmacros.h
adoptimize.o: arbdbt.h ./ad_t_prot.h
adperl.o:  arbdb.h
adperl.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adquery.o:  adlocal.h adtune.h adlmacros.h
adseqcompr.o:  adlocal.h adtune.h adlmacros.h arbdb.h
adseqcompr.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adseqcompr.o: arbdbt.h ./ad_t_prot.h adseqcompr.h
adsocket.o:   adlocal.h
adsocket.o: adtune.h adlmacros.h 
adsort.o:   arbdb.h
adsort.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adsort.o: arbdbt.h ./ad_t_prot.h
adstring.o:  adlocal.h adtune.h adlmacros.h arbdbt.h
adstring.o: ./ad_t_prot.h  
adsystem.o:  adlocal.h adtune.h adlmacros.h
adsystem.o: arbdbt.h ./ad_t_prot.h
adtables.o:  adlocal.h adtune.h adlmacros.h arbdb.h
adtables.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adtables.o: arbdbt.h ./ad_t_prot.h
adtools.o:  adlocal.h adtune.h adlmacros.h arbdb.h
adtools.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
adtools.o: arbdbt.h ./ad_t_prot.h
adtune.o: adtune.h
arbdb.o:   adlocal.h adtune.h
arbdb.o: adlmacros.h arbdb.h
arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
arbdbpp.o:  arbdb.h
arbdbpp.o: $(ARBHOME)/INCLUDE/arb_assert.h ./ad_prot.h
arbdbpp.o: arbdbt.h ./ad_t_prot.h
