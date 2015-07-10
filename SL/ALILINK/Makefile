# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(OBJECTS) *.a

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

AdminGui.o: AliAdmin.h
AdminGui.o: $(ARBHOME)/INCLUDE/ad_prot.h
AdminGui.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_assert.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_core.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_error.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_msg.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_string.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdb.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdbt.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbtools.h
AdminGui.o: $(ARBHOME)/INCLUDE/attributes.h
AdminGui.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_question.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/cb.h
AdminGui.o: $(ARBHOME)/INCLUDE/cb_base.h
AdminGui.o: $(ARBHOME)/INCLUDE/cbtypes.h
AdminGui.o: $(ARBHOME)/INCLUDE/cxxforward.h
AdminGui.o: $(ARBHOME)/INCLUDE/downcast.h
AdminGui.o: $(ARBHOME)/INCLUDE/dupstr.h
AdminGui.o: $(ARBHOME)/INCLUDE/gccver.h
AdminGui.o: $(ARBHOME)/INCLUDE/insdel.h
AdminGui.o: $(ARBHOME)/INCLUDE/smartptr.h
AdminGui.o: $(ARBHOME)/INCLUDE/static_assert.h
AdminGui.o: $(ARBHOME)/INCLUDE/test_global.h
AdminGui.o: $(ARBHOME)/INCLUDE/ttypes.h

AliAdmin.o: AliAdmin.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_assert.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_core.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_error.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_msg.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbtools.h
AliAdmin.o: $(ARBHOME)/INCLUDE/attributes.h
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/cb.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cb_base.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cbtypes.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cxxforward.h
AliAdmin.o: $(ARBHOME)/INCLUDE/dupstr.h
AliAdmin.o: $(ARBHOME)/INCLUDE/gccver.h
AliAdmin.o: $(ARBHOME)/INCLUDE/smartptr.h
AliAdmin.o: $(ARBHOME)/INCLUDE/static_assert.h
AliAdmin.o: $(ARBHOME)/INCLUDE/test_global.h
AliAdmin.o: $(ARBHOME)/INCLUDE/ttypes.h

LinkAlignments.o: LinkAlignments.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/ad_prot.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arb_assert.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arb_core.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arb_error.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arb_msg.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arb_string.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arbdb.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arbdb_base.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arbdbt.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/arbtools.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/attributes.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/cxxforward.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/downcast.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/dupstr.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/gccver.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/smartptr.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/static_assert.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/test_global.h
LinkAlignments.o: $(ARBHOME)/INCLUDE/test_unit.h

TranslateRealign.o: TranslateRealign.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/ad_prot.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/AP_codon_table.hxx
TranslateRealign.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_assert.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_core.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_defs.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_error.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_handlers.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_msg.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_progress.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arb_string.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arbdb.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arbdbt.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/arbtools.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/attributes.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/aw_base.hxx
TranslateRealign.o: $(ARBHOME)/INCLUDE/aw_question.hxx
TranslateRealign.o: $(ARBHOME)/INCLUDE/cxxforward.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/downcast.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/dupstr.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/gccver.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/smartptr.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/static_assert.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/test_global.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/test_unit.h
TranslateRealign.o: $(ARBHOME)/INCLUDE/Translate.hxx
