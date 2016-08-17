# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	Translate.o \

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

Translate.o: Translate.hxx
Translate.o: $(ARBHOME)/INCLUDE/ad_prot.h
Translate.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
Translate.o: $(ARBHOME)/INCLUDE/AP_codon_table.hxx
Translate.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
Translate.o: $(ARBHOME)/INCLUDE/arb_assert.h
Translate.o: $(ARBHOME)/INCLUDE/arb_core.h
Translate.o: $(ARBHOME)/INCLUDE/arb_error.h
Translate.o: $(ARBHOME)/INCLUDE/arb_mem.h
Translate.o: $(ARBHOME)/INCLUDE/arb_msg.h
Translate.o: $(ARBHOME)/INCLUDE/arb_string.h
Translate.o: $(ARBHOME)/INCLUDE/arbdb.h
Translate.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Translate.o: $(ARBHOME)/INCLUDE/arbdbt.h
Translate.o: $(ARBHOME)/INCLUDE/arbtools.h
Translate.o: $(ARBHOME)/INCLUDE/attributes.h
Translate.o: $(ARBHOME)/INCLUDE/cxxforward.h
Translate.o: $(ARBHOME)/INCLUDE/downcast.h
Translate.o: $(ARBHOME)/INCLUDE/dupstr.h
Translate.o: $(ARBHOME)/INCLUDE/gccver.h
Translate.o: $(ARBHOME)/INCLUDE/smartptr.h
Translate.o: $(ARBHOME)/INCLUDE/static_assert.h
Translate.o: $(ARBHOME)/INCLUDE/test_global.h
