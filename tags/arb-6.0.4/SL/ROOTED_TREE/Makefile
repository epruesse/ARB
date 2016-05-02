# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

SOURCES=$(wildcard *.cxx)
OBJECTS=$(subst .cxx,.o,$(SOURCES))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

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

RootedTree.o: RootedTree.h
RootedTree.o: $(ARBHOME)/INCLUDE/ad_prot.h
RootedTree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RootedTree.o: $(ARBHOME)/INCLUDE/arb_assert.h
RootedTree.o: $(ARBHOME)/INCLUDE/arb_core.h
RootedTree.o: $(ARBHOME)/INCLUDE/arb_error.h
RootedTree.o: $(ARBHOME)/INCLUDE/arb_msg.h
RootedTree.o: $(ARBHOME)/INCLUDE/arb_string.h
RootedTree.o: $(ARBHOME)/INCLUDE/arbdb.h
RootedTree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RootedTree.o: $(ARBHOME)/INCLUDE/arbdbt.h
RootedTree.o: $(ARBHOME)/INCLUDE/arbtools.h
RootedTree.o: $(ARBHOME)/INCLUDE/attributes.h
RootedTree.o: $(ARBHOME)/INCLUDE/cxxforward.h
RootedTree.o: $(ARBHOME)/INCLUDE/downcast.h
RootedTree.o: $(ARBHOME)/INCLUDE/dupstr.h
RootedTree.o: $(ARBHOME)/INCLUDE/gccver.h
RootedTree.o: $(ARBHOME)/INCLUDE/smartptr.h
RootedTree.o: $(ARBHOME)/INCLUDE/static_assert.h
RootedTree.o: $(ARBHOME)/INCLUDE/test_global.h
