# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend

include AD_MOBJECTS.h
OBJECTS = $(GB_O) $(GB_T) $(GB_PP) $(GB_X)

$(MAIN): $(OBJECTS)
	$(LINK_SHARED_LIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.c.o:
	$(CPPLIB) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

.cxx.o:
	$(CPPLIB) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

proto:	gproto lproto

lproto: lpro tlpro# local prototypes

lpro: $(GB_O:.o=.c)
	../MAKEBIN/aisc_mkpt -c "Internal database interface" -G -A -w ad_lpro.h -F ^gb_,^gbs_,^gbcm,^gbm_,^gbl_ $^ >ad_lpro.h.tmp
	../SOURCE_TOOLS/mv_if_diff ad_lpro.h.tmp ad_lpro.h

tlpro: $(GB_T:.o=.c)
	../MAKEBIN/aisc_mkpt -c "Internal toolkit" -G -A -w ad_t_lpro.h -F ^gb_,^gbt_,^gbs_ $^ >ad_t_lpro.h.tmp
	../SOURCE_TOOLS/mv_if_diff ad_t_lpro.h.tmp ad_t_lpro.h

gproto: pro ppro tpro kpro# global prototypes

pro: $(GB_O:.o=.c)
	../MAKEBIN/aisc_mkpt -c "ARB database interface" -G -A -w ad_prot.h -F ^GB_,^GEN_,^EXP_,^GBS_,^GBT_,^GBCM $^ >ad_prot.h.tmp
	../SOURCE_TOOLS/mv_if_diff ad_prot.h.tmp ad_prot.h

ppro: $(GB_O:.o=.c)
	../MAKEBIN/aisc_mkpt -c "ARB perl interface" -G -A -w ad_p_prot.h -F ^GBP_ $^ >ad_p_prot.h.tmp
	../SOURCE_TOOLS/mv_if_diff ad_p_prot.h.tmp ad_p_prot.h

tpro: $(GB_T:.o=.c)
	../MAKEBIN/aisc_mkpt -c "ARB toolkit" -G -A -w ad_t_prot.h -F ^GB_,^GEN_,^EXP_,^GBT_ $^ >ad_t_prot.h.tmp
	../SOURCE_TOOLS/mv_if_diff ad_t_prot.h.tmp ad_t_prot.h

kpro: $(GB_O:.o=.c)
	../MAKEBIN/aisc_mkpt -c "ARB kernel interface" -G -A -w ad_k_prot.h -F ^GBK_ $^ >ad_k_prot.h.tmp
	../SOURCE_TOOLS/mv_if_diff ad_k_prot.h.tmp ad_k_prot.h

wc:
	wc *.[ch]

clean:
	rm -f $(OBJECTS) *.a *.so

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

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ad_config.o: ad_config.h
ad_config.o: ad_k_prot.h
ad_config.o: ad_prot.h
ad_config.o: ad_t_prot.h
ad_config.o: arb_assert.h
ad_config.o: arbdb.h
ad_config.o: arbdb_base.h
ad_config.o: arbdbt.h
ad_config.o: $(ARBHOME)/INCLUDE/arb_error.h
ad_config.o: $(ARBHOME)/INCLUDE/arbtools.h
ad_config.o: $(ARBHOME)/INCLUDE/attributes.h
ad_config.o: $(ARBHOME)/INCLUDE/smartptr.h

ad_core.o: ad_k_prot.h
ad_core.o: ad_lpro.h
ad_core.o: ad_prot.h
ad_core.o: adlmacros.h
ad_core.o: adlocal.h
ad_core.o: adtune.h
ad_core.o: arb_assert.h
ad_core.o: arbdb.h
ad_core.o: arbdb_base.h
ad_core.o: $(ARBHOME)/INCLUDE/arb_error.h
ad_core.o: $(ARBHOME)/INCLUDE/arbtools.h
ad_core.o: $(ARBHOME)/INCLUDE/attributes.h
ad_core.o: $(ARBHOME)/INCLUDE/smartptr.h

ad_load.o: ad_k_prot.h
ad_load.o: ad_lpro.h
ad_load.o: ad_prot.h
ad_load.o: ad_t_lpro.h
ad_load.o: ad_t_prot.h
ad_load.o: adlmacros.h
ad_load.o: adlocal.h
ad_load.o: admap.h
ad_load.o: adtune.h
ad_load.o: arb_assert.h
ad_load.o: arbdb.h
ad_load.o: arbdb_base.h
ad_load.o: arbdbt.h
ad_load.o: $(ARBHOME)/INCLUDE/arb_error.h
ad_load.o: $(ARBHOME)/INCLUDE/arbtools.h
ad_load.o: $(ARBHOME)/INCLUDE/attributes.h
ad_load.o: $(ARBHOME)/INCLUDE/smartptr.h

ad_save_load.o: ad_k_prot.h
ad_save_load.o: ad_lpro.h
ad_save_load.o: ad_prot.h
ad_save_load.o: adlmacros.h
ad_save_load.o: adlocal.h
ad_save_load.o: admap.h
ad_save_load.o: adtune.h
ad_save_load.o: arb_assert.h
ad_save_load.o: arbdb.h
ad_save_load.o: arbdb_base.h
ad_save_load.o: $(ARBHOME)/INCLUDE/arb_error.h
ad_save_load.o: $(ARBHOME)/INCLUDE/arbtools.h
ad_save_load.o: $(ARBHOME)/INCLUDE/attributes.h
ad_save_load.o: $(ARBHOME)/INCLUDE/smartptr.h

adali.o: ad_k_prot.h
adali.o: ad_lpro.h
adali.o: ad_prot.h
adali.o: ad_t_lpro.h
adali.o: ad_t_prot.h
adali.o: adGene.h
adali.o: adlmacros.h
adali.o: adlocal.h
adali.o: adtune.h
adali.o: arb_assert.h
adali.o: arbdb.h
adali.o: arbdb_base.h
adali.o: arbdbt.h
adali.o: $(ARBHOME)/INCLUDE/arb_error.h
adali.o: $(ARBHOME)/INCLUDE/arbtools.h
adali.o: $(ARBHOME)/INCLUDE/attributes.h
adali.o: $(ARBHOME)/INCLUDE/smartptr.h

adChangeKey.o: ad_k_prot.h
adChangeKey.o: ad_prot.h
adChangeKey.o: ad_t_prot.h
adChangeKey.o: arb_assert.h
adChangeKey.o: arbdb.h
adChangeKey.o: arbdb_base.h
adChangeKey.o: arbdbt.h
adChangeKey.o: $(ARBHOME)/INCLUDE/arb_error.h
adChangeKey.o: $(ARBHOME)/INCLUDE/arbtools.h
adChangeKey.o: $(ARBHOME)/INCLUDE/attributes.h
adChangeKey.o: $(ARBHOME)/INCLUDE/smartptr.h

adcolumns.o: ad_k_prot.h
adcolumns.o: ad_lpro.h
adcolumns.o: ad_prot.h
adcolumns.o: ad_t_lpro.h
adcolumns.o: ad_t_prot.h
adcolumns.o: adGene.h
adcolumns.o: adlmacros.h
adcolumns.o: adlocal.h
adcolumns.o: adtune.h
adcolumns.o: arb_assert.h
adcolumns.o: arbdb.h
adcolumns.o: arbdb_base.h
adcolumns.o: arbdbt.h
adcolumns.o: $(ARBHOME)/INCLUDE/arb_error.h
adcolumns.o: $(ARBHOME)/INCLUDE/arbtools.h
adcolumns.o: $(ARBHOME)/INCLUDE/attributes.h
adcolumns.o: $(ARBHOME)/INCLUDE/smartptr.h

adcomm.o: ad_k_prot.h
adcomm.o: ad_lpro.h
adcomm.o: ad_prot.h
adcomm.o: adlmacros.h
adcomm.o: adlocal.h
adcomm.o: adtune.h
adcomm.o: arb_assert.h
adcomm.o: arbdb.h
adcomm.o: arbdb_base.h
adcomm.o: $(ARBHOME)/INCLUDE/arb_error.h
adcomm.o: $(ARBHOME)/INCLUDE/arbtools.h
adcomm.o: $(ARBHOME)/INCLUDE/attributes.h
adcomm.o: $(ARBHOME)/INCLUDE/smartptr.h

adcompr.o: ad_k_prot.h
adcompr.o: ad_lpro.h
adcompr.o: ad_prot.h
adcompr.o: ad_t_lpro.h
adcompr.o: ad_t_prot.h
adcompr.o: adlmacros.h
adcompr.o: adlocal.h
adcompr.o: adtune.h
adcompr.o: arb_assert.h
adcompr.o: arbdb.h
adcompr.o: arbdb_base.h
adcompr.o: arbdbt.h
adcompr.o: $(ARBHOME)/INCLUDE/arb_error.h
adcompr.o: $(ARBHOME)/INCLUDE/arbtools.h
adcompr.o: $(ARBHOME)/INCLUDE/attributes.h
adcompr.o: $(ARBHOME)/INCLUDE/smartptr.h

adExperiment.o: ad_k_prot.h
adExperiment.o: ad_prot.h
adExperiment.o: ad_t_prot.h
adExperiment.o: arb_assert.h
adExperiment.o: arbdb.h
adExperiment.o: arbdb_base.h
adExperiment.o: arbdbt.h
adExperiment.o: $(ARBHOME)/INCLUDE/arb_error.h
adExperiment.o: $(ARBHOME)/INCLUDE/arbtools.h
adExperiment.o: $(ARBHOME)/INCLUDE/attributes.h
adExperiment.o: $(ARBHOME)/INCLUDE/smartptr.h

adGene.o: ad_k_prot.h
adGene.o: ad_prot.h
adGene.o: ad_t_prot.h
adGene.o: adGene.h
adGene.o: arb_assert.h
adGene.o: arbdb.h
adGene.o: arbdb_base.h
adGene.o: arbdbt.h
adGene.o: $(ARBHOME)/INCLUDE/arb_error.h
adGene.o: $(ARBHOME)/INCLUDE/arbtools.h
adGene.o: $(ARBHOME)/INCLUDE/attributes.h
adGene.o: $(ARBHOME)/INCLUDE/smartptr.h

adhash.o: ad_k_prot.h
adhash.o: ad_lpro.h
adhash.o: ad_prot.h
adhash.o: adlmacros.h
adhash.o: adlocal.h
adhash.o: adtune.h
adhash.o: arb_assert.h
adhash.o: arbdb.h
adhash.o: arbdb_base.h
adhash.o: $(ARBHOME)/INCLUDE/arb_error.h
adhash.o: $(ARBHOME)/INCLUDE/arbtools.h
adhash.o: $(ARBHOME)/INCLUDE/attributes.h
adhash.o: $(ARBHOME)/INCLUDE/smartptr.h

adhashtools.o: ad_k_prot.h
adhashtools.o: ad_lpro.h
adhashtools.o: ad_prot.h
adhashtools.o: ad_t_lpro.h
adhashtools.o: ad_t_prot.h
adhashtools.o: adlmacros.h
adhashtools.o: adlocal.h
adhashtools.o: adtune.h
adhashtools.o: arb_assert.h
adhashtools.o: arbdb.h
adhashtools.o: arbdb_base.h
adhashtools.o: arbdbt.h
adhashtools.o: $(ARBHOME)/INCLUDE/arb_error.h
adhashtools.o: $(ARBHOME)/INCLUDE/arbtools.h
adhashtools.o: $(ARBHOME)/INCLUDE/attributes.h
adhashtools.o: $(ARBHOME)/INCLUDE/smartptr.h

adindex.o: ad_k_prot.h
adindex.o: ad_lpro.h
adindex.o: ad_prot.h
adindex.o: adlmacros.h
adindex.o: adlocal.h
adindex.o: adlundo.h
adindex.o: adtune.h
adindex.o: arb_assert.h
adindex.o: arbdb.h
adindex.o: arbdb_base.h
adindex.o: $(ARBHOME)/INCLUDE/arb_error.h
adindex.o: $(ARBHOME)/INCLUDE/arbtools.h
adindex.o: $(ARBHOME)/INCLUDE/attributes.h
adindex.o: $(ARBHOME)/INCLUDE/smartptr.h

aditem.o: ad_k_prot.h
aditem.o: ad_lpro.h
aditem.o: ad_prot.h
aditem.o: ad_t_lpro.h
aditem.o: ad_t_prot.h
aditem.o: adlmacros.h
aditem.o: adlocal.h
aditem.o: adtune.h
aditem.o: arb_assert.h
aditem.o: arbdb.h
aditem.o: arbdb_base.h
aditem.o: arbdbt.h
aditem.o: $(ARBHOME)/INCLUDE/arb_error.h
aditem.o: $(ARBHOME)/INCLUDE/arbtools.h
aditem.o: $(ARBHOME)/INCLUDE/attributes.h
aditem.o: $(ARBHOME)/INCLUDE/smartptr.h

adlang1.o: ad_k_prot.h
adlang1.o: ad_lpro.h
adlang1.o: ad_prot.h
adlang1.o: ad_t_lpro.h
adlang1.o: ad_t_prot.h
adlang1.o: adGene.h
adlang1.o: adlmacros.h
adlang1.o: adlocal.h
adlang1.o: adtune.h
adlang1.o: arb_assert.h
adlang1.o: arbdb.h
adlang1.o: arbdb_base.h
adlang1.o: arbdbt.h
adlang1.o: $(ARBHOME)/INCLUDE/arb_error.h
adlang1.o: $(ARBHOME)/INCLUDE/arbtools.h
adlang1.o: $(ARBHOME)/INCLUDE/attributes.h
adlang1.o: $(ARBHOME)/INCLUDE/smartptr.h

adlink.o: ad_k_prot.h
adlink.o: ad_lpro.h
adlink.o: ad_prot.h
adlink.o: adlmacros.h
adlink.o: adlocal.h
adlink.o: adtune.h
adlink.o: arb_assert.h
adlink.o: arbdb.h
adlink.o: arbdb_base.h
adlink.o: $(ARBHOME)/INCLUDE/arb_error.h
adlink.o: $(ARBHOME)/INCLUDE/arbtools.h
adlink.o: $(ARBHOME)/INCLUDE/attributes.h
adlink.o: $(ARBHOME)/INCLUDE/smartptr.h

admalloc.o: ad_k_prot.h
admalloc.o: ad_lpro.h
admalloc.o: ad_prot.h
admalloc.o: adlmacros.h
admalloc.o: adlocal.h
admalloc.o: adtune.h
admalloc.o: arb_assert.h
admalloc.o: arbdb.h
admalloc.o: arbdb_base.h
admalloc.o: $(ARBHOME)/INCLUDE/arb_error.h
admalloc.o: $(ARBHOME)/INCLUDE/arbtools.h
admalloc.o: $(ARBHOME)/INCLUDE/attributes.h
admalloc.o: $(ARBHOME)/INCLUDE/smartptr.h

admap.o: ad_k_prot.h
admap.o: ad_lpro.h
admap.o: ad_prot.h
admap.o: adlmacros.h
admap.o: adlocal.h
admap.o: admap.h
admap.o: adtune.h
admap.o: arb_assert.h
admap.o: arbdb.h
admap.o: arbdb_base.h
admap.o: $(ARBHOME)/INCLUDE/arb_error.h
admap.o: $(ARBHOME)/INCLUDE/arbtools.h
admap.o: $(ARBHOME)/INCLUDE/attributes.h
admap.o: $(ARBHOME)/INCLUDE/smartptr.h

admatch.o: ad_k_prot.h
admatch.o: ad_lpro.h
admatch.o: ad_prot.h
admatch.o: adlmacros.h
admatch.o: adlocal.h
admatch.o: adtune.h
admatch.o: arb_assert.h
admatch.o: arbdb.h
admatch.o: arbdb_base.h
admatch.o: $(ARBHOME)/INCLUDE/arb_error.h
admatch.o: $(ARBHOME)/INCLUDE/arbtools.h
admatch.o: $(ARBHOME)/INCLUDE/attributes.h
admatch.o: $(ARBHOME)/INCLUDE/smartptr.h

admath.o: ad_k_prot.h
admath.o: ad_lpro.h
admath.o: ad_prot.h
admath.o: adlmacros.h
admath.o: adlocal.h
admath.o: adtune.h
admath.o: arb_assert.h
admath.o: arbdb.h
admath.o: arbdb_base.h
admath.o: $(ARBHOME)/INCLUDE/arb_error.h
admath.o: $(ARBHOME)/INCLUDE/arbtools.h
admath.o: $(ARBHOME)/INCLUDE/attributes.h
admath.o: $(ARBHOME)/INCLUDE/smartptr.h

adname.o: ad_config.h
adname.o: ad_k_prot.h
adname.o: ad_lpro.h
adname.o: ad_prot.h
adname.o: ad_t_lpro.h
adname.o: ad_t_prot.h
adname.o: adlmacros.h
adname.o: adlocal.h
adname.o: adtune.h
adname.o: arb_assert.h
adname.o: arbdb.h
adname.o: arbdb_base.h
adname.o: arbdbt.h
adname.o: $(ARBHOME)/INCLUDE/arb_error.h
adname.o: $(ARBHOME)/INCLUDE/arbtools.h
adname.o: $(ARBHOME)/INCLUDE/attributes.h
adname.o: $(ARBHOME)/INCLUDE/smartptr.h

adoptimize.o: ad_k_prot.h
adoptimize.o: ad_lpro.h
adoptimize.o: ad_prot.h
adoptimize.o: ad_t_lpro.h
adoptimize.o: ad_t_prot.h
adoptimize.o: adlmacros.h
adoptimize.o: adlocal.h
adoptimize.o: adtune.h
adoptimize.o: arb_assert.h
adoptimize.o: arbdb.h
adoptimize.o: arbdb_base.h
adoptimize.o: arbdbt.h
adoptimize.o: $(ARBHOME)/INCLUDE/arb_error.h
adoptimize.o: $(ARBHOME)/INCLUDE/arbtools.h
adoptimize.o: $(ARBHOME)/INCLUDE/attributes.h
adoptimize.o: $(ARBHOME)/INCLUDE/smartptr.h

adperl.o: ad_k_prot.h
adperl.o: ad_prot.h
adperl.o: arb_assert.h
adperl.o: arbdb.h
adperl.o: arbdb_base.h
adperl.o: $(ARBHOME)/INCLUDE/arb_error.h
adperl.o: $(ARBHOME)/INCLUDE/arbtools.h
adperl.o: $(ARBHOME)/INCLUDE/attributes.h
adperl.o: $(ARBHOME)/INCLUDE/smartptr.h

adquery.o: ad_k_prot.h
adquery.o: ad_lpro.h
adquery.o: ad_prot.h
adquery.o: adlmacros.h
adquery.o: adlocal.h
adquery.o: adtune.h
adquery.o: arb_assert.h
adquery.o: arbdb.h
adquery.o: arbdb_base.h
adquery.o: $(ARBHOME)/INCLUDE/arb_error.h
adquery.o: $(ARBHOME)/INCLUDE/arbtools.h
adquery.o: $(ARBHOME)/INCLUDE/attributes.h
adquery.o: $(ARBHOME)/INCLUDE/smartptr.h

adRevCompl.o: ad_k_prot.h
adRevCompl.o: ad_prot.h
adRevCompl.o: ad_t_prot.h
adRevCompl.o: arb_assert.h
adRevCompl.o: arbdb.h
adRevCompl.o: arbdb_base.h
adRevCompl.o: arbdbt.h
adRevCompl.o: $(ARBHOME)/INCLUDE/arb_error.h
adRevCompl.o: $(ARBHOME)/INCLUDE/arbtools.h
adRevCompl.o: $(ARBHOME)/INCLUDE/attributes.h
adRevCompl.o: $(ARBHOME)/INCLUDE/smartptr.h

adseqcompr.o: ad_k_prot.h
adseqcompr.o: ad_lpro.h
adseqcompr.o: ad_prot.h
adseqcompr.o: ad_t_lpro.h
adseqcompr.o: ad_t_prot.h
adseqcompr.o: adlmacros.h
adseqcompr.o: adlocal.h
adseqcompr.o: adtune.h
adseqcompr.o: arb_assert.h
adseqcompr.o: arbdb.h
adseqcompr.o: arbdb_base.h
adseqcompr.o: arbdbt.h
adseqcompr.o: $(ARBHOME)/INCLUDE/arb_error.h
adseqcompr.o: $(ARBHOME)/INCLUDE/arbtools.h
adseqcompr.o: $(ARBHOME)/INCLUDE/attributes.h
adseqcompr.o: $(ARBHOME)/INCLUDE/smartptr.h

adsocket.o: ad_k_prot.h
adsocket.o: ad_lpro.h
adsocket.o: ad_prot.h
adsocket.o: adlmacros.h
adsocket.o: adlocal.h
adsocket.o: adtune.h
adsocket.o: arb_assert.h
adsocket.o: arbdb.h
adsocket.o: arbdb_base.h
adsocket.o: $(ARBHOME)/INCLUDE/arb_error.h
adsocket.o: $(ARBHOME)/INCLUDE/arbtools.h
adsocket.o: $(ARBHOME)/INCLUDE/attributes.h
adsocket.o: $(ARBHOME)/INCLUDE/smartptr.h

adsort.o: ad_k_prot.h
adsort.o: ad_prot.h
adsort.o: arb_assert.h
adsort.o: arbdb.h
adsort.o: arbdb_base.h
adsort.o: $(ARBHOME)/INCLUDE/arb_error.h
adsort.o: $(ARBHOME)/INCLUDE/arbtools.h
adsort.o: $(ARBHOME)/INCLUDE/attributes.h
adsort.o: $(ARBHOME)/INCLUDE/smartptr.h

adstring.o: ad_k_prot.h
adstring.o: ad_lpro.h
adstring.o: ad_prot.h
adstring.o: adlmacros.h
adstring.o: adlocal.h
adstring.o: adtune.h
adstring.o: arb_assert.h
adstring.o: arbdb.h
adstring.o: arbdb_base.h
adstring.o: $(ARBHOME)/INCLUDE/arb_error.h
adstring.o: $(ARBHOME)/INCLUDE/arbtools.h
adstring.o: $(ARBHOME)/INCLUDE/attributes.h
adstring.o: $(ARBHOME)/INCLUDE/smartptr.h

adsystem.o: ad_k_prot.h
adsystem.o: ad_lpro.h
adsystem.o: ad_prot.h
adsystem.o: ad_t_lpro.h
adsystem.o: ad_t_prot.h
adsystem.o: adlmacros.h
adsystem.o: adlocal.h
adsystem.o: adtune.h
adsystem.o: arb_assert.h
adsystem.o: arbdb.h
adsystem.o: arbdb_base.h
adsystem.o: arbdbt.h
adsystem.o: $(ARBHOME)/INCLUDE/arb_error.h
adsystem.o: $(ARBHOME)/INCLUDE/arbtools.h
adsystem.o: $(ARBHOME)/INCLUDE/attributes.h
adsystem.o: $(ARBHOME)/INCLUDE/smartptr.h

adtables.o: ad_k_prot.h
adtables.o: ad_lpro.h
adtables.o: ad_prot.h
adtables.o: ad_t_lpro.h
adtables.o: ad_t_prot.h
adtables.o: adlmacros.h
adtables.o: adlocal.h
adtables.o: adtune.h
adtables.o: arb_assert.h
adtables.o: arbdb.h
adtables.o: arbdb_base.h
adtables.o: arbdbt.h
adtables.o: $(ARBHOME)/INCLUDE/arb_error.h
adtables.o: $(ARBHOME)/INCLUDE/arbtools.h
adtables.o: $(ARBHOME)/INCLUDE/attributes.h
adtables.o: $(ARBHOME)/INCLUDE/smartptr.h

adtcp.o: ad_k_prot.h
adtcp.o: ad_lpro.h
adtcp.o: ad_prot.h
adtcp.o: ad_t_lpro.h
adtcp.o: ad_t_prot.h
adtcp.o: adlmacros.h
adtcp.o: adlocal.h
adtcp.o: adtune.h
adtcp.o: arb_assert.h
adtcp.o: arbdb.h
adtcp.o: arbdb_base.h
adtcp.o: arbdbt.h
adtcp.o: $(ARBHOME)/INCLUDE/arb_error.h
adtcp.o: $(ARBHOME)/INCLUDE/arbtools.h
adtcp.o: $(ARBHOME)/INCLUDE/attributes.h
adtcp.o: $(ARBHOME)/INCLUDE/smartptr.h

adTest.o: ad_k_prot.h
adTest.o: ad_lpro.h
adTest.o: ad_prot.h
adTest.o: adlmacros.h
adTest.o: adlocal.h
adTest.o: admap.h
adTest.o: adtune.h
adTest.o: arb_assert.h
adTest.o: arbdb.h
adTest.o: arbdb_base.h
adTest.o: $(ARBHOME)/INCLUDE/arb_error.h
adTest.o: $(ARBHOME)/INCLUDE/arbtools.h
adTest.o: $(ARBHOME)/INCLUDE/attributes.h
adTest.o: $(ARBHOME)/INCLUDE/smartptr.h

adtools.o: ad_k_prot.h
adtools.o: ad_lpro.h
adtools.o: ad_prot.h
adtools.o: ad_t_lpro.h
adtools.o: ad_t_prot.h
adtools.o: adlmacros.h
adtools.o: adlocal.h
adtools.o: adtune.h
adtools.o: arb_assert.h
adtools.o: arbdb.h
adtools.o: arbdb_base.h
adtools.o: arbdbt.h
adtools.o: $(ARBHOME)/INCLUDE/arb_error.h
adtools.o: $(ARBHOME)/INCLUDE/arbtools.h
adtools.o: $(ARBHOME)/INCLUDE/attributes.h
adtools.o: $(ARBHOME)/INCLUDE/smartptr.h

adtree.o: ad_k_prot.h
adtree.o: ad_lpro.h
adtree.o: ad_prot.h
adtree.o: ad_t_lpro.h
adtree.o: ad_t_prot.h
adtree.o: adlmacros.h
adtree.o: adlocal.h
adtree.o: adtune.h
adtree.o: arb_assert.h
adtree.o: arbdb.h
adtree.o: arbdb_base.h
adtree.o: arbdbt.h
adtree.o: $(ARBHOME)/INCLUDE/arb_error.h
adtree.o: $(ARBHOME)/INCLUDE/arbtools.h
adtree.o: $(ARBHOME)/INCLUDE/attributes.h
adtree.o: $(ARBHOME)/INCLUDE/smartptr.h

adtune.o: adtune.h

arbdb.o: ad_k_prot.h
arbdb.o: ad_lpro.h
arbdb.o: ad_prot.h
arbdb.o: adlmacros.h
arbdb.o: adlocal.h
arbdb.o: adtune.h
arbdb.o: arb_assert.h
arbdb.o: arbdb.h
arbdb.o: arbdb_base.h
arbdb.o: $(ARBHOME)/INCLUDE/arb_error.h
arbdb.o: $(ARBHOME)/INCLUDE/arbtools.h
arbdb.o: $(ARBHOME)/INCLUDE/attributes.h
arbdb.o: $(ARBHOME)/INCLUDE/smartptr.h

arbdbpp.o: ad_k_prot.h
arbdbpp.o: ad_prot.h
arbdbpp.o: ad_t_prot.h
arbdbpp.o: arb_assert.h
arbdbpp.o: arbdb.h
arbdbpp.o: arbdb_base.h
arbdbpp.o: arbdbt.h
arbdbpp.o: $(ARBHOME)/INCLUDE/arb_error.h
arbdbpp.o: $(ARBHOME)/INCLUDE/arbtools.h
arbdbpp.o: $(ARBHOME)/INCLUDE/attributes.h
arbdbpp.o: $(ARBHOME)/INCLUDE/smartptr.h
