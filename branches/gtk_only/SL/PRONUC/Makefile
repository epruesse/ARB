# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	AP_pro_a_nucs.o \
	AP_codon_table.o \
	iupac.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

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

AP_codon_table.o: AP_codon_table.hxx
AP_codon_table.o: iupac.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_str.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/attributes.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/gccver.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/test_global.h
AP_codon_table.o: $(ARBHOME)/INCLUDE/test_unit.h

AP_pro_a_nucs.o: AP_codon_table.hxx
AP_pro_a_nucs.o: AP_pro_a_nucs.hxx
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_cb.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_core.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_error.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_mem.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_msg.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_str.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_string.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbtools.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/attributes.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/cb.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/cb_base.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/cbtypes.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/cxxforward.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/downcast.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/dupstr.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/gccver.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/smartptr.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/static_assert.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/test_global.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/test_unit.h
AP_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ttypes.h

iupac.o: iupac.h
iupac.o: $(ARBHOME)/INCLUDE/arb_assert.h
iupac.o: $(ARBHOME)/INCLUDE/arb_core.h
iupac.o: $(ARBHOME)/INCLUDE/arbdb_base.h
iupac.o: $(ARBHOME)/INCLUDE/arbtools.h
iupac.o: $(ARBHOME)/INCLUDE/attributes.h
iupac.o: $(ARBHOME)/INCLUDE/cxxforward.h
iupac.o: $(ARBHOME)/INCLUDE/dupstr.h
iupac.o: $(ARBHOME)/INCLUDE/gccver.h
iupac.o: $(ARBHOME)/INCLUDE/test_global.h
iupac.o: $(ARBHOME)/INCLUDE/test_unit.h
