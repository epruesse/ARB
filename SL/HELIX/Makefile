# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = BI_helix.o BI_basepos.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE)

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

BI_basepos.o: BI_basepos.hxx
BI_basepos.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_basepos.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arb_assert.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arb_core.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arb_error.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arb_msg.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arb_string.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arbdb_base.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_basepos.o: $(ARBHOME)/INCLUDE/arbtools.h
BI_basepos.o: $(ARBHOME)/INCLUDE/attributes.h
BI_basepos.o: $(ARBHOME)/INCLUDE/cxxforward.h
BI_basepos.o: $(ARBHOME)/INCLUDE/downcast.h
BI_basepos.o: $(ARBHOME)/INCLUDE/dupstr.h
BI_basepos.o: $(ARBHOME)/INCLUDE/gccver.h
BI_basepos.o: $(ARBHOME)/INCLUDE/smartptr.h
BI_basepos.o: $(ARBHOME)/INCLUDE/static_assert.h
BI_basepos.o: $(ARBHOME)/INCLUDE/test_global.h

BI_helix.o: BI_helix.hxx
BI_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_assert.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_core.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_error.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_msg.h
BI_helix.o: $(ARBHOME)/INCLUDE/arb_string.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb_base.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbtools.h
BI_helix.o: $(ARBHOME)/INCLUDE/attributes.h
BI_helix.o: $(ARBHOME)/INCLUDE/cxxforward.h
BI_helix.o: $(ARBHOME)/INCLUDE/downcast.h
BI_helix.o: $(ARBHOME)/INCLUDE/dupstr.h
BI_helix.o: $(ARBHOME)/INCLUDE/gccver.h
BI_helix.o: $(ARBHOME)/INCLUDE/smartptr.h
BI_helix.o: $(ARBHOME)/INCLUDE/static_assert.h
BI_helix.o: $(ARBHOME)/INCLUDE/test_global.h
