# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	NJ.o \

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

NJ.o: neighbourjoin.hxx
NJ.o: NJ.hxx
NJ.o: $(ARBHOME)/INCLUDE/ad_prot.h
NJ.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NJ.o: $(ARBHOME)/INCLUDE/AP_matrix.hxx
NJ.o: $(ARBHOME)/INCLUDE/arb_assert.h
NJ.o: $(ARBHOME)/INCLUDE/arb_core.h
NJ.o: $(ARBHOME)/INCLUDE/arb_diff.h
NJ.o: $(ARBHOME)/INCLUDE/arb_error.h
NJ.o: $(ARBHOME)/INCLUDE/arb_msg.h
NJ.o: $(ARBHOME)/INCLUDE/arb_string.h
NJ.o: $(ARBHOME)/INCLUDE/arbdb.h
NJ.o: $(ARBHOME)/INCLUDE/arbdb_base.h
NJ.o: $(ARBHOME)/INCLUDE/arbdbt.h
NJ.o: $(ARBHOME)/INCLUDE/arbtools.h
NJ.o: $(ARBHOME)/INCLUDE/attributes.h
NJ.o: $(ARBHOME)/INCLUDE/cxxforward.h
NJ.o: $(ARBHOME)/INCLUDE/downcast.h
NJ.o: $(ARBHOME)/INCLUDE/dupstr.h
NJ.o: $(ARBHOME)/INCLUDE/gccver.h
NJ.o: $(ARBHOME)/INCLUDE/smartptr.h
NJ.o: $(ARBHOME)/INCLUDE/static_assert.h
NJ.o: $(ARBHOME)/INCLUDE/test_global.h
NJ.o: $(ARBHOME)/INCLUDE/test_unit.h
NJ.o: $(ARBHOME)/INCLUDE/TreeNode.h
