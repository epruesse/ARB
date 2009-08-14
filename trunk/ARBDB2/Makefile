# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .depend

HeaderFile = AD_MOBJECTS.h
CheckHEADER := $(shell if [ -e $(HeaderFile) ] ; then echo 1 ; else echo 0 ; fi)
ifeq ($(CheckHEADER),1)
    include $(HeaderFile)
endif

ACC_OBJECTS = $(GB_O) $(GB_T) $(GB_X)

$(MAIN): $(ACC_OBJECTS)
	$(LINK_SHARED_LIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(ACC_OBJECTS)
	touch $@
.c.o:
	$(ACCLIB) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(ACC_OBJECTS) *.a *.so

DEPENDS = $(ACC_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

links:
	ln -s ../ARBDB/*.[ch] .

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ad_config.o: ad_config.h
ad_config.o: ad_prot.h
ad_config.o: ad_t_prot.h
ad_config.o: arbdb.h
ad_config.o: arbdbt.h
ad_config.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ad_config.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_config.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ad_config.o: $(ARBHOME)/INCLUDE/attributes.h

ad_core.o: ad_lpro.h
ad_core.o: ad_prot.h
ad_core.o: adlmacros.h
ad_core.o: adlocal.h
ad_core.o: adtune.h
ad_core.o: arbdb.h
ad_core.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ad_core.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_core.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ad_core.o: $(ARBHOME)/INCLUDE/attributes.h

ad_load.o: ad_lpro.h
ad_load.o: ad_prot.h
ad_load.o: ad_t_lpro.h
ad_load.o: ad_t_prot.h
ad_load.o: adlmacros.h
ad_load.o: adlocal.h
ad_load.o: admap.h
ad_load.o: adtune.h
ad_load.o: arbdb.h
ad_load.o: arbdbt.h
ad_load.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ad_load.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_load.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ad_load.o: $(ARBHOME)/INCLUDE/attributes.h

ad_save_load.o: ad_lpro.h
ad_save_load.o: ad_prot.h
ad_save_load.o: adlmacros.h
ad_save_load.o: adlocal.h
ad_save_load.o: admap.h
ad_save_load.o: adtune.h
ad_save_load.o: arbdb.h
ad_save_load.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ad_save_load.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_save_load.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ad_save_load.o: $(ARBHOME)/INCLUDE/attributes.h

adali.o: ad_lpro.h
adali.o: ad_prot.h
adali.o: ad_t_lpro.h
adali.o: ad_t_prot.h
adali.o: adGene.h
adali.o: adlmacros.h
adali.o: adlocal.h
adali.o: adtune.h
adali.o: arbdb.h
adali.o: arbdbt.h
adali.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adali.o: $(ARBHOME)/INCLUDE/arb_assert.h
adali.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adali.o: $(ARBHOME)/INCLUDE/attributes.h

adChangeKey.o: ad_prot.h
adChangeKey.o: ad_t_prot.h
adChangeKey.o: arbdb.h
adChangeKey.o: arbdbt.h
adChangeKey.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adChangeKey.o: $(ARBHOME)/INCLUDE/arb_assert.h
adChangeKey.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adChangeKey.o: $(ARBHOME)/INCLUDE/attributes.h

adcolumns.o: ad_lpro.h
adcolumns.o: ad_prot.h
adcolumns.o: ad_t_lpro.h
adcolumns.o: ad_t_prot.h
adcolumns.o: adGene.h
adcolumns.o: adlmacros.h
adcolumns.o: adlocal.h
adcolumns.o: adtune.h
adcolumns.o: arbdb.h
adcolumns.o: arbdbt.h
adcolumns.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adcolumns.o: $(ARBHOME)/INCLUDE/arb_assert.h
adcolumns.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adcolumns.o: $(ARBHOME)/INCLUDE/attributes.h

adcomm.o: ad_lpro.h
adcomm.o: ad_prot.h
adcomm.o: adlmacros.h
adcomm.o: adlocal.h
adcomm.o: adtune.h
adcomm.o: arbdb.h
adcomm.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adcomm.o: $(ARBHOME)/INCLUDE/arb_assert.h
adcomm.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adcomm.o: $(ARBHOME)/INCLUDE/attributes.h

adcompr.o: ad_lpro.h
adcompr.o: ad_prot.h
adcompr.o: ad_t_lpro.h
adcompr.o: ad_t_prot.h
adcompr.o: adlmacros.h
adcompr.o: adlocal.h
adcompr.o: adtune.h
adcompr.o: arbdb.h
adcompr.o: arbdbt.h
adcompr.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adcompr.o: $(ARBHOME)/INCLUDE/arb_assert.h
adcompr.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adcompr.o: $(ARBHOME)/INCLUDE/attributes.h

adExperiment.o: ad_prot.h
adExperiment.o: ad_t_prot.h
adExperiment.o: arbdb.h
adExperiment.o: arbdbt.h
adExperiment.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adExperiment.o: $(ARBHOME)/INCLUDE/arb_assert.h
adExperiment.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adExperiment.o: $(ARBHOME)/INCLUDE/attributes.h

adGene.o: ad_prot.h
adGene.o: ad_t_prot.h
adGene.o: adGene.h
adGene.o: arbdb.h
adGene.o: arbdbt.h
adGene.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adGene.o: $(ARBHOME)/INCLUDE/arb_assert.h
adGene.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adGene.o: $(ARBHOME)/INCLUDE/attributes.h

adhash.o: ad_lpro.h
adhash.o: ad_prot.h
adhash.o: adlmacros.h
adhash.o: adlocal.h
adhash.o: adtune.h
adhash.o: arbdb.h
adhash.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adhash.o: $(ARBHOME)/INCLUDE/arb_assert.h
adhash.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adhash.o: $(ARBHOME)/INCLUDE/attributes.h

adhashtools.o: ad_lpro.h
adhashtools.o: ad_prot.h
adhashtools.o: ad_t_lpro.h
adhashtools.o: ad_t_prot.h
adhashtools.o: adlmacros.h
adhashtools.o: adlocal.h
adhashtools.o: adtune.h
adhashtools.o: arbdb.h
adhashtools.o: arbdbt.h
adhashtools.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adhashtools.o: $(ARBHOME)/INCLUDE/arb_assert.h
adhashtools.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adhashtools.o: $(ARBHOME)/INCLUDE/attributes.h

adindex.o: ad_lpro.h
adindex.o: ad_prot.h
adindex.o: adlmacros.h
adindex.o: adlocal.h
adindex.o: adlundo.h
adindex.o: adtune.h
adindex.o: arbdb.h
adindex.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adindex.o: $(ARBHOME)/INCLUDE/arb_assert.h
adindex.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adindex.o: $(ARBHOME)/INCLUDE/attributes.h

aditem.o: ad_lpro.h
aditem.o: ad_prot.h
aditem.o: ad_t_lpro.h
aditem.o: ad_t_prot.h
aditem.o: adlmacros.h
aditem.o: adlocal.h
aditem.o: adtune.h
aditem.o: arbdb.h
aditem.o: arbdbt.h
aditem.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
aditem.o: $(ARBHOME)/INCLUDE/arb_assert.h
aditem.o: $(ARBHOME)/INCLUDE/arbdb_base.h
aditem.o: $(ARBHOME)/INCLUDE/attributes.h

adlang1.o: ad_lpro.h
adlang1.o: ad_prot.h
adlang1.o: ad_t_lpro.h
adlang1.o: ad_t_prot.h
adlang1.o: adGene.h
adlang1.o: adlmacros.h
adlang1.o: adlocal.h
adlang1.o: adtune.h
adlang1.o: arbdb.h
adlang1.o: arbdbt.h
adlang1.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adlang1.o: $(ARBHOME)/INCLUDE/arb_assert.h
adlang1.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adlang1.o: $(ARBHOME)/INCLUDE/attributes.h

adlink.o: ad_lpro.h
adlink.o: ad_prot.h
adlink.o: adlmacros.h
adlink.o: adlocal.h
adlink.o: adtune.h
adlink.o: arbdb.h
adlink.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adlink.o: $(ARBHOME)/INCLUDE/arb_assert.h
adlink.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adlink.o: $(ARBHOME)/INCLUDE/attributes.h

admalloc.o: ad_lpro.h
admalloc.o: ad_prot.h
admalloc.o: adlmacros.h
admalloc.o: adlocal.h
admalloc.o: adtune.h
admalloc.o: arbdb.h
admalloc.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
admalloc.o: $(ARBHOME)/INCLUDE/arb_assert.h
admalloc.o: $(ARBHOME)/INCLUDE/arbdb_base.h
admalloc.o: $(ARBHOME)/INCLUDE/attributes.h

admap.o: ad_lpro.h
admap.o: ad_prot.h
admap.o: adlmacros.h
admap.o: adlocal.h
admap.o: admap.h
admap.o: adtune.h
admap.o: arbdb.h
admap.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
admap.o: $(ARBHOME)/INCLUDE/arb_assert.h
admap.o: $(ARBHOME)/INCLUDE/arbdb_base.h
admap.o: $(ARBHOME)/INCLUDE/attributes.h

admatch.o: ad_lpro.h
admatch.o: ad_prot.h
admatch.o: adlmacros.h
admatch.o: adlocal.h
admatch.o: adtune.h
admatch.o: arbdb.h
admatch.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
admatch.o: $(ARBHOME)/INCLUDE/arb_assert.h
admatch.o: $(ARBHOME)/INCLUDE/arbdb_base.h
admatch.o: $(ARBHOME)/INCLUDE/attributes.h

admath.o: ad_lpro.h
admath.o: ad_prot.h
admath.o: adlmacros.h
admath.o: adlocal.h
admath.o: adtune.h
admath.o: arbdb.h
admath.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
admath.o: $(ARBHOME)/INCLUDE/arb_assert.h
admath.o: $(ARBHOME)/INCLUDE/arbdb_base.h
admath.o: $(ARBHOME)/INCLUDE/attributes.h

adname.o: ad_config.h
adname.o: ad_lpro.h
adname.o: ad_prot.h
adname.o: ad_t_lpro.h
adname.o: ad_t_prot.h
adname.o: adlmacros.h
adname.o: adlocal.h
adname.o: adtune.h
adname.o: arbdb.h
adname.o: arbdbt.h
adname.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adname.o: $(ARBHOME)/INCLUDE/arb_assert.h
adname.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adname.o: $(ARBHOME)/INCLUDE/attributes.h

adoptimize.o: ad_lpro.h
adoptimize.o: ad_prot.h
adoptimize.o: ad_t_lpro.h
adoptimize.o: ad_t_prot.h
adoptimize.o: adlmacros.h
adoptimize.o: adlocal.h
adoptimize.o: adtune.h
adoptimize.o: arbdb.h
adoptimize.o: arbdbt.h
adoptimize.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adoptimize.o: $(ARBHOME)/INCLUDE/arb_assert.h
adoptimize.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adoptimize.o: $(ARBHOME)/INCLUDE/attributes.h

adperl.o: ad_prot.h
adperl.o: arbdb.h
adperl.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adperl.o: $(ARBHOME)/INCLUDE/arb_assert.h
adperl.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adperl.o: $(ARBHOME)/INCLUDE/attributes.h

adquery.o: ad_lpro.h
adquery.o: ad_prot.h
adquery.o: adlmacros.h
adquery.o: adlocal.h
adquery.o: adtune.h
adquery.o: arbdb.h
adquery.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adquery.o: $(ARBHOME)/INCLUDE/arb_assert.h
adquery.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adquery.o: $(ARBHOME)/INCLUDE/attributes.h

adRevCompl.o: ad_prot.h
adRevCompl.o: ad_t_prot.h
adRevCompl.o: arbdb.h
adRevCompl.o: arbdbt.h
adRevCompl.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adRevCompl.o: $(ARBHOME)/INCLUDE/arb_assert.h
adRevCompl.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adRevCompl.o: $(ARBHOME)/INCLUDE/attributes.h

adseqcompr.o: ad_lpro.h
adseqcompr.o: ad_prot.h
adseqcompr.o: ad_t_lpro.h
adseqcompr.o: ad_t_prot.h
adseqcompr.o: adlmacros.h
adseqcompr.o: adlocal.h
adseqcompr.o: adtune.h
adseqcompr.o: arbdb.h
adseqcompr.o: arbdbt.h
adseqcompr.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adseqcompr.o: $(ARBHOME)/INCLUDE/arb_assert.h
adseqcompr.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adseqcompr.o: $(ARBHOME)/INCLUDE/attributes.h

adsocket.o: ad_lpro.h
adsocket.o: ad_prot.h
adsocket.o: adlmacros.h
adsocket.o: adlocal.h
adsocket.o: adtune.h
adsocket.o: arbdb.h
adsocket.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adsocket.o: $(ARBHOME)/INCLUDE/arb_assert.h
adsocket.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adsocket.o: $(ARBHOME)/INCLUDE/attributes.h

adsort.o: ad_prot.h
adsort.o: arbdb.h
adsort.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adsort.o: $(ARBHOME)/INCLUDE/arb_assert.h
adsort.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adsort.o: $(ARBHOME)/INCLUDE/attributes.h

adstring.o: ad_lpro.h
adstring.o: ad_prot.h
adstring.o: adlmacros.h
adstring.o: adlocal.h
adstring.o: adtune.h
adstring.o: arbdb.h
adstring.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adstring.o: $(ARBHOME)/INCLUDE/arb_assert.h
adstring.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adstring.o: $(ARBHOME)/INCLUDE/attributes.h

adsystem.o: ad_lpro.h
adsystem.o: ad_prot.h
adsystem.o: ad_t_lpro.h
adsystem.o: ad_t_prot.h
adsystem.o: adlmacros.h
adsystem.o: adlocal.h
adsystem.o: adtune.h
adsystem.o: arbdb.h
adsystem.o: arbdbt.h
adsystem.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adsystem.o: $(ARBHOME)/INCLUDE/arb_assert.h
adsystem.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adsystem.o: $(ARBHOME)/INCLUDE/attributes.h

adtables.o: ad_lpro.h
adtables.o: ad_prot.h
adtables.o: ad_t_lpro.h
adtables.o: ad_t_prot.h
adtables.o: adlmacros.h
adtables.o: adlocal.h
adtables.o: adtune.h
adtables.o: arbdb.h
adtables.o: arbdbt.h
adtables.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adtables.o: $(ARBHOME)/INCLUDE/arb_assert.h
adtables.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adtables.o: $(ARBHOME)/INCLUDE/attributes.h

adtcp.o: ad_lpro.h
adtcp.o: ad_prot.h
adtcp.o: ad_t_lpro.h
adtcp.o: ad_t_prot.h
adtcp.o: adlmacros.h
adtcp.o: adlocal.h
adtcp.o: adtune.h
adtcp.o: arbdb.h
adtcp.o: arbdbt.h
adtcp.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adtcp.o: $(ARBHOME)/INCLUDE/arb_assert.h
adtcp.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adtcp.o: $(ARBHOME)/INCLUDE/attributes.h

adTest.o: ad_lpro.h
adTest.o: ad_prot.h
adTest.o: adlmacros.h
adTest.o: adlocal.h
adTest.o: admap.h
adTest.o: adtune.h
adTest.o: arbdb.h
adTest.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adTest.o: $(ARBHOME)/INCLUDE/arb_assert.h
adTest.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adTest.o: $(ARBHOME)/INCLUDE/attributes.h

adtools.o: ad_lpro.h
adtools.o: ad_prot.h
adtools.o: ad_t_lpro.h
adtools.o: ad_t_prot.h
adtools.o: adlmacros.h
adtools.o: adlocal.h
adtools.o: adtune.h
adtools.o: arbdb.h
adtools.o: arbdbt.h
adtools.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adtools.o: $(ARBHOME)/INCLUDE/arb_assert.h
adtools.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adtools.o: $(ARBHOME)/INCLUDE/attributes.h

adtree.o: ad_lpro.h
adtree.o: ad_prot.h
adtree.o: ad_t_lpro.h
adtree.o: ad_t_prot.h
adtree.o: adlmacros.h
adtree.o: adlocal.h
adtree.o: adtune.h
adtree.o: arbdb.h
adtree.o: arbdbt.h
adtree.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
adtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
adtree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
adtree.o: $(ARBHOME)/INCLUDE/attributes.h

adtune.o: adtune.h

arbdb.o: ad_lpro.h
arbdb.o: ad_prot.h
arbdb.o: adlmacros.h
arbdb.o: adlocal.h
arbdb.o: adtune.h
arbdb.o: arbdb.h
arbdb.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h
arbdb.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arbdb.o: $(ARBHOME)/INCLUDE/attributes.h
