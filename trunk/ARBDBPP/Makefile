# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend
CPP_OBJECTS = adtsequence.o adt_edit.o adtali.o adextended.o adseq.o adspecies.o admain.o adali.o  liste.o
$(MAIN): $(CPP_OBJECTS)
	$(ARLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(CPP_OBJECTS)
	touch $@
.cxx.o:
	$(CCPLIB) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a *.so

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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

adali.o: arbdb++.hxx
adali.o: $(ARBHOME)/INCLUDE/ad_prot.h
adali.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
adali.o: $(ARBHOME)/INCLUDE/arb_assert.h
adali.o: $(ARBHOME)/INCLUDE/arbdb.h
adali.o: $(ARBHOME)/INCLUDE/arbdbt.h
adali.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

adextended.o: arbdb++.hxx
adextended.o: $(ARBHOME)/INCLUDE/ad_prot.h
adextended.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
adextended.o: $(ARBHOME)/INCLUDE/arb_assert.h
adextended.o: $(ARBHOME)/INCLUDE/arbdb.h
adextended.o: $(ARBHOME)/INCLUDE/arbdbt.h
adextended.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

admain.o: arbdb++.hxx
admain.o: $(ARBHOME)/INCLUDE/ad_prot.h
admain.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
admain.o: $(ARBHOME)/INCLUDE/arb_assert.h
admain.o: $(ARBHOME)/INCLUDE/arbdb.h
admain.o: $(ARBHOME)/INCLUDE/arbdbt.h
admain.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

adseq.o: arbdb++.hxx
adseq.o: $(ARBHOME)/INCLUDE/ad_prot.h
adseq.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
adseq.o: $(ARBHOME)/INCLUDE/arb_assert.h
adseq.o: $(ARBHOME)/INCLUDE/arbdb.h
adseq.o: $(ARBHOME)/INCLUDE/arbdbt.h
adseq.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

adspecies.o: arbdb++.hxx
adspecies.o: $(ARBHOME)/INCLUDE/ad_prot.h
adspecies.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
adspecies.o: $(ARBHOME)/INCLUDE/arb_assert.h
adspecies.o: $(ARBHOME)/INCLUDE/arbdb.h
adspecies.o: $(ARBHOME)/INCLUDE/arbdbt.h
adspecies.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

adt_edit.o: adtools.hxx
adt_edit.o: arbdb++.hxx
adt_edit.o: $(ARBHOME)/INCLUDE/ad_prot.h
adt_edit.o: $(ARBHOME)/INCLUDE/arb_assert.h
adt_edit.o: $(ARBHOME)/INCLUDE/arbdb.h
adt_edit.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

adtali.o: adtools.hxx
adtali.o: arbdb++.hxx
adtali.o: $(ARBHOME)/INCLUDE/ad_prot.h
adtali.o: $(ARBHOME)/INCLUDE/arb_assert.h
adtali.o: $(ARBHOME)/INCLUDE/arbdb.h
adtali.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

adtsequence.o: adtools.hxx
adtsequence.o: arbdb++.hxx
adtsequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
adtsequence.o: $(ARBHOME)/INCLUDE/arb_assert.h
adtsequence.o: $(ARBHOME)/INCLUDE/arbdb.h
adtsequence.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx

liste.o: arbdb++.hxx
liste.o: $(ARBHOME)/INCLUDE/ad_prot.h
liste.o: $(ARBHOME)/INCLUDE/arb_assert.h
liste.o: $(ARBHOME)/INCLUDE/arbdb.h
liste.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
