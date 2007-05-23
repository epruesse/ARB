
.SUFFIXES: .o .c .cxx .depend

OBJECTS = trs_hash.o tree_lib.o trs_server.o

$(ARBHOME)/bin/trs_server: $(OBJECTS)
	$(CPP) -o $@ $(STATIC) $(OBJECTS)


# $(OBJECTS:%.o=$(ARBHOME)/irsbin/%)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

proto:
	../MAKEBIN/aisc_mkpt -g $(OBJECTS:.o=.cxx) >trs_proto.h.tmp
	../SOURCE_TOOLS/mv_if_diff trs_proto.h.tmp trs_proto.h

$(ARBHOME)/irsbin/%: %.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIB2)

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

tree_lib.o: tree_lib.hxx
tree_lib.o: trs_proto.h
tree_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
tree_lib.o: $(ARBHOME)/INCLUDE/arb_assert.h
tree_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
tree_lib.o: $(ARBHOME)/INCLUDE/cat_tree.hxx

trs_server.o: tree_lib.hxx
trs_server.o: trs_proto.h
trs_server.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
