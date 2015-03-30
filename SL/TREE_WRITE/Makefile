# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = TreeWrite.o
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

TreeWrite.o: TreeWrite.h
TreeWrite.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeWrite.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeWrite.o: $(ARBHOME)/INCLUDE/attributes.h
TreeWrite.o: $(ARBHOME)/INCLUDE/aw_base.hxx
TreeWrite.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeWrite.o: $(ARBHOME)/INCLUDE/downcast.h
TreeWrite.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeWrite.o: $(ARBHOME)/INCLUDE/gccver.h
TreeWrite.o: $(ARBHOME)/INCLUDE/nds.h
TreeWrite.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeWrite.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeWrite.o: $(ARBHOME)/INCLUDE/test_global.h
TreeWrite.o: $(ARBHOME)/INCLUDE/xml.hxx
