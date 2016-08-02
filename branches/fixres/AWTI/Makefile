# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = \
	AWTI_import.o \
	AWTI_export.o \

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(CPP_OBJECTS) *.a

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

AWTI_export.o: awti_export.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTI_export.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTI_export.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_core.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_error.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_file.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_mem.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_msg.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_progress.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_str.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_string.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arbtools.h
AWTI_export.o: $(ARBHOME)/INCLUDE/attributes.h
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_file.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/awt_filter.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/cb.h
AWTI_export.o: $(ARBHOME)/INCLUDE/cb_base.h
AWTI_export.o: $(ARBHOME)/INCLUDE/cbtypes.h
AWTI_export.o: $(ARBHOME)/INCLUDE/cxxforward.h
AWTI_export.o: $(ARBHOME)/INCLUDE/downcast.h
AWTI_export.o: $(ARBHOME)/INCLUDE/dupstr.h
AWTI_export.o: $(ARBHOME)/INCLUDE/gccver.h
AWTI_export.o: $(ARBHOME)/INCLUDE/seqio.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/smartptr.h
AWTI_export.o: $(ARBHOME)/INCLUDE/static_assert.h
AWTI_export.o: $(ARBHOME)/INCLUDE/test_global.h
AWTI_export.o: $(ARBHOME)/INCLUDE/ttypes.h

AWTI_import.o: awti_imp_local.hxx
AWTI_import.o: awti_import.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTI_import.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTI_import.o: $(ARBHOME)/INCLUDE/adGene.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_core.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_error.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_file.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_mem.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_msg.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_progress.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_str.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_strarray.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_string.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arbtools.h
AWTI_import.o: $(ARBHOME)/INCLUDE/attributes.h
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_advice.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_file.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_question.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_select.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/awt.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/cb.h
AWTI_import.o: $(ARBHOME)/INCLUDE/cb_base.h
AWTI_import.o: $(ARBHOME)/INCLUDE/cbtypes.h
AWTI_import.o: $(ARBHOME)/INCLUDE/cxxforward.h
AWTI_import.o: $(ARBHOME)/INCLUDE/downcast.h
AWTI_import.o: $(ARBHOME)/INCLUDE/dupstr.h
AWTI_import.o: $(ARBHOME)/INCLUDE/gccver.h
AWTI_import.o: $(ARBHOME)/INCLUDE/GEN.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/GenomeImport.h
AWTI_import.o: $(ARBHOME)/INCLUDE/item_sel_list.h
AWTI_import.o: $(ARBHOME)/INCLUDE/items.h
AWTI_import.o: $(ARBHOME)/INCLUDE/macros.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/seqio.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/smartptr.h
AWTI_import.o: $(ARBHOME)/INCLUDE/static_assert.h
AWTI_import.o: $(ARBHOME)/INCLUDE/test_global.h
AWTI_import.o: $(ARBHOME)/INCLUDE/ttypes.h
